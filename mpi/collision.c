#include <float.h>
#include <math.h>

#include "collision.h"
#include "event.h"
#include "grid.h"
#include "simulation.h"
#include "vector_3.h"

// Adapted from: https://www.gamasutra.com/view/feature/131424/pool_hall_lessons_fast_accurate_.php?page=2
// Finds the time at which the two spheres will collide.
// If the spheres will not collide on their current paths then returns DBL_MAX.
// Note this assumes that the velocity of both particles is constant before the collision.
// Performs the following tests to see if the spheres will collide.
// First test:
//	1) Obtain velocity of s1 relative to s2.
//	2) Obtain position of s2 relative to s1.
//	3) If the shortest angle between these two vectors is >= 90 then the 
//	   spheres will not collide on their current paths so return.
// Second test:
//	1) Using relative velocity and position find the shortest distance 
//	   between the two points along their current paths.
//	2) If this is larger than the sum of their radii then the spheres will 
//	   not collide so return.
// If these tests pass then we know the spheres will collide. 
// We then have to figure out the actual point of collision, as the shortest
// distance will very likely be when the spheres pass through one another.
// Trigonometry is used to figure out where the spheres collide.
// TODO: better comments on trig part
static double find_collision_time_spheres(const struct sphere_s *s1, const struct sphere_s *s2) {
	union vector_3d rel_vel;
	rel_vel.x = s1->vel.x - s2->vel.x; 
	rel_vel.y = s1->vel.y - s2->vel.y; 
	rel_vel.z = s1->vel.z - s2->vel.z;
	union vector_3d rel_pos;
	rel_pos.x = s2->pos.x - s1->pos.x; 
	rel_pos.y = s2->pos.y - s1->pos.y; 
	rel_pos.z = s2->pos.z - s1->pos.z;
	double dp = get_vector_3d_dot_product(&rel_vel, &rel_pos);
	double vel_vec_mag = get_vector_3d_magnitude(&rel_vel);
	double pos_vec_mag = get_vector_3d_magnitude(&rel_pos);
	double angle = get_shortest_angle_between_vector_3d(dp, vel_vec_mag, pos_vec_mag);
	if (angle >= 3.14159265358979323846 / 2.0) { //check if >= 90 degrees (note angle is in radians)
		return DBL_MAX;
	}
	double r_total = s1->radius + s2->radius;
	double shortest_dist = sin(angle) * pos_vec_mag;
	if (shortest_dist > r_total) {
		return DBL_MAX;
	}
	double d = sqrt((pos_vec_mag * pos_vec_mag) - (shortest_dist * shortest_dist));
	double t = sqrt((r_total * r_total) - (shortest_dist * shortest_dist));
	double dist_to_col = d - t;
	// d should always be larger than t, but due to precision issues it may be
	// very slightly smaller than t causing dist_to_col to be negative.
	// This can occur when the shortest distance is 0.0.
	// If this happens (d-t) should be 0.0, so just return 0.0
	if (dist_to_col < 0.0) {
		return 0.0;
	}
	return  dist_to_col / vel_vec_mag;
}

// Finds the time taken to cross the boundary on the specified axis in the grid or sector.
// axis_vel and axis_pos are the velocity/position of the sphere on that axis.
// If checking against the grid then bound_start will be 0.0, if checking against
// a sector then it will be the sector's starting pos on the axis
static double find_time_to_cross_boundary(const double bound_start, const double bound_end, const double axis_vel, const double axis_pos, const double radius) {
	double dist = 0;
	if (axis_vel > 0) {
		dist = bound_end - axis_pos - radius;
	} else {
		dist = bound_start - axis_pos + radius;
	}
	return dist / axis_vel;
}

// Finds the time when the sphere will pass into another sector
// Note: we want to know when the center of the sphere crosses the sector boundary so we set
// radius to 0 when calling find_time_to_cross_boundary().
static double find_collision_time_sector(const struct sector_s *sector, const struct sphere_s *sphere, struct sector_s **dest) {
	double time = DBL_MAX;
	enum axis a;
	for (a = X_AXIS; a <= Z_AXIS; a++) {
		if (sphere->vel.vals[a] != 0.0 && !(sector->pos.vals[a] == 0.0 && sphere->vel.vals[a] < 0.0) && !(sector->pos.vals[a] == sim_data.sector_dims[a] - 1 && sphere->vel.vals[a] > 0.0)) {
			double temp_time = find_time_to_cross_boundary(sector->start.vals[a], sector->end.vals[a], sphere->vel.vals[a], sphere->pos.vals[a], 0.0);
			if (temp_time < time) {
				time = temp_time;
				if (sphere->vel.vals[a] > 0.0) {
					*dest = get_adjacent_sector_non_diagonal(sector, a, DIR_POSITIVE);
				} else {
					*dest = get_adjacent_sector_non_diagonal(sector, a, DIR_NEGATIVE);
				}
			}
		}
	}
	return time;
}

// Finds the time when the sphere will collide with the grid on its current path.
// "col_axis" will default to AXIS_NONE, and will keep that value if the sphere is stationary.
// The time will be DBL_MAX if the sphere is stationary so that we can still
// compare other times to see if they are sooner.
// TODO: in the extremely unlikely event that the sphere perfectly hits a corner
// of the grid then two iterations will be needed before it bounces properly - it
// would be good if this could be handled in a single iteration instead. 
static double find_collision_time_grid(const struct sphere_s *s, enum axis *col_axis) {
	double time = DBL_MAX;
	*col_axis = AXIS_NONE;
	enum axis a;
	for (a = X_AXIS; a <= Z_AXIS; a++) {
		if (s->vel.vals[a] != 0) {
			double temp_time = find_time_to_cross_boundary(0.0, sim_data.grid_size.vals[a], s->vel.vals[a], s->pos.vals[a], s->radius);
			if (temp_time < time) {
				time = temp_time;
				*col_axis = a;
			}
		}
	}
	return time;
}

// Given a sphere that is known to be heading towards the given sector
// check if the sphere will collide with spheres in the sector.
static void find_partial_crossing_events_between_sphere_and_sector(const struct sphere_s *sphere_1, const struct sector_s *sector_1, const struct sector_s *sector_2) {
	int j;
	for (j = 0; j < sector_2->num_spheres; j++) {
		struct sphere_s *sphere_2 = &sector_2->spheres[j];
		double time = find_collision_time_spheres(sphere_1, sphere_2);
		if (time < event_details.time) {
			set_event_details(time, COL_TWO_SPHERES_PARTIAL_CROSSING, sphere_1, sphere_2, AXIS_NONE, sector_1, sector_2);
		}
	}
}

// Checks for partial crossings with sectors that are immediately adjacent to 
// the left/right, top/bottom or front/back.
static void find_partial_crossing_events_for_sector_directly_adjacent(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos) {
	enum axis a;
	for (a = X_AXIS; a <= Z_AXIS; a++) { // right/up/forward on x/y/z axis
		if (sphere->vel.vals[a] >= 0.0 && sector->pos.vals[a] != sim_data.sector_dims[a] - 1) {
			int s_x = sector->pos.x + SECTOR_MODIFIERS[DIR_POSITIVE][a][X_AXIS];
			int s_y = sector->pos.y + SECTOR_MODIFIERS[DIR_POSITIVE][a][Y_AXIS];
			int s_z = sector->pos.z + SECTOR_MODIFIERS[DIR_POSITIVE][a][Z_AXIS];
			struct sector_s *sector_2 = &sim_data.sectors[s_x][s_y][s_z];
			if (new_pos.vals[a] >= sector_2->start.vals[a] - sphere->radius - sector_2->largest_radius) {
				find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
			}
		}
	}
	for (a = X_AXIS; a <= Z_AXIS; a++) { // left/down/behind on x/y/z axis
		if (sphere->vel.vals[a] <= 0.0 && sector->pos.vals[a] != 0) {
			int s_x = sector->pos.x + SECTOR_MODIFIERS[DIR_NEGATIVE][a][X_AXIS];
			int s_y = sector->pos.y + SECTOR_MODIFIERS[DIR_NEGATIVE][a][Y_AXIS];
			int s_z = sector->pos.z + SECTOR_MODIFIERS[DIR_NEGATIVE][a][Z_AXIS];
			struct sector_s *sector_2 = &sim_data.sectors[s_x][s_y][s_z];
			if (new_pos.vals[a] <= sector_2->end.vals[a] + sphere->radius + sector_2->largest_radius) {
				find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
			}
		}
	}
}

// Positive in both axes
static void find_partial_crossing_events_for_sector_diagonally_adjacent_pos_pos(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos, enum axis a1, enum axis a2) {
	if (sphere->vel.vals[a1] >= 0.0 && sector->pos.vals[a1] != sim_data.sector_dims[a1] - 1 && sphere->vel.vals[a2] >= 0.0 && sector->pos.vals[a2] != sim_data.sector_dims[a2] - 1) {
		int x = sector->pos.x + SECTOR_MODIFIERS[DIR_POSITIVE][a1][X_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][X_AXIS];
		int y = sector->pos.y + SECTOR_MODIFIERS[DIR_POSITIVE][a1][Y_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][Y_AXIS];
		int z = sector->pos.z + SECTOR_MODIFIERS[DIR_POSITIVE][a1][Z_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][Z_AXIS];
		struct sector_s *sector_2 = &sim_data.sectors[x][y][z];
		if (new_pos.vals[a1] >= sector_2->start.vals[a1] - sphere->radius - sector_2->largest_radius && new_pos.vals[a2] >= sector_2->start.vals[a2] - sphere->radius - sector_2->largest_radius) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
}

// Positive in a1 and negative in a2
static void find_partial_crossing_events_for_sector_diagonally_adjacent_pos_neg(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos, enum axis a1, enum axis a2) {
	if (sphere->vel.vals[a1] >= 0.0 && sector->pos.vals[a1] != sim_data.sector_dims[a1] - 1 && sphere->vel.vals[a2] <= 0.0 && sector->pos.vals[a2] != 0) {
		int x = sector->pos.x + SECTOR_MODIFIERS[DIR_POSITIVE][a1][X_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][X_AXIS];
		int y = sector->pos.y + SECTOR_MODIFIERS[DIR_POSITIVE][a1][Y_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][Y_AXIS];
		int z = sector->pos.z + SECTOR_MODIFIERS[DIR_POSITIVE][a1][Z_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][Z_AXIS];
		struct sector_s *sector_2 = &sim_data.sectors[x][y][z];
		if (new_pos.vals[a1] >= sector_2->start.vals[a1] - sphere->radius - sector_2->largest_radius && new_pos.vals[a2] <= sector_2->end.vals[a2] + sphere->radius + sector_2->largest_radius) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
}

// Negative in a1 and positive in a2
static void find_partial_crossing_events_for_sector_diagonally_adjacent_neg_pos(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos, enum axis a1, enum axis a2) {
	if (sphere->vel.vals[a1] <= 0.0 && sector->pos.vals[a1] != 0 && sphere->vel.vals[a2] >= 0.0 && sector->pos.vals[a2] != sim_data.sector_dims[a2] - 1) {
		int x = sector->pos.x + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][X_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][X_AXIS];
		int y = sector->pos.y + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][Y_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][Y_AXIS];
		int z = sector->pos.z + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][Z_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][Z_AXIS];
		struct sector_s *sector_2 = &sim_data.sectors[x][y][z];
		if (new_pos.vals[a1] <= sector_2->end.vals[a1] + sphere->radius + sector_2->largest_radius && new_pos.vals[a2] >= sector_2->start.vals[a2] - sphere->radius - sector_2->largest_radius) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
}

// Negative in both axes
static void find_partial_crossing_events_for_sector_diagonally_adjacent_neg_neg(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos, enum axis a1, enum axis a2) {
	if (sphere->vel.vals[a1] <= 0.0 && sector->pos.vals[a1] != 0 && sphere->vel.vals[a2] <= 0.0 && sector->pos.vals[a2] != 0) {
		int x = sector->pos.x + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][X_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][X_AXIS];
		int y = sector->pos.y + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][Y_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][Y_AXIS];
		int z = sector->pos.z + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][Z_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][Z_AXIS];
		struct sector_s *sector_2 = &sim_data.sectors[x][y][z];
		if (new_pos.vals[a1] <= sector_2->end.vals[a1] + sphere->radius + sector_2->largest_radius && new_pos.vals[a2] >= sector_2->end.vals[a2] + sphere->radius + sector_2->largest_radius) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
}

// Checks for partial crossings with sectors that are diagonally adjacent along two axes.
static void find_partial_crossing_events_for_sector_diagonally_adjacent(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos) {
	// x/y combinations
	if (sim_data.xy_check_needed) {
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_pos(sphere, sector, new_pos, X_AXIS, Y_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_neg(sphere, sector, new_pos, X_AXIS, Y_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_pos(sphere, sector, new_pos, X_AXIS, Y_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_neg(sphere, sector, new_pos, X_AXIS, Y_AXIS);
	}
	// x/z combinations
	if (sim_data.xz_check_needed) {
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_pos(sphere, sector, new_pos, X_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_neg(sphere, sector, new_pos, X_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_pos(sphere, sector, new_pos, X_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_neg(sphere, sector, new_pos, X_AXIS, Z_AXIS);
	}
	// y/z combinations
	if (sim_data.yz_check_needed) {
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_pos(sphere, sector, new_pos, Y_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_neg(sphere, sector, new_pos, Y_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_pos(sphere, sector, new_pos, Y_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_neg(sphere, sector, new_pos, Y_AXIS, Z_AXIS);
	}
}

// Checks for partial crossings with sectors that are diagonally adjacent along three axes.
// Ex: if passed sector at [0][0][0] it will check [1][1][1].
// TODO: make this more generic
static void find_partial_crossing_events_for_sector_diagonally_adjacent_three_axes(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos) {
	bool heading_towards;
	bool within_range;
	// all positive
	heading_towards = 
		sphere->vel.x >= 0.0 && sector->pos.x != sim_data.sector_dims[X_AXIS] - 1
		&& sphere->vel.y >= 0.0 && sector->pos.y != sim_data.sector_dims[Y_AXIS] - 1 
		&& sphere->vel.z >= 0.0 && sector->pos.z != sim_data.sector_dims[Z_AXIS] - 1;
	if (heading_towards) {
		struct sector_s *sector_2 = &sim_data.sectors[sector->pos.x + 1][sector->pos.y + 1][sector->pos.z + 1];
		within_range =
			new_pos.x >= sector_2->start.x - sphere->radius - sector_2->largest_radius
			&& new_pos.y >= sector_2->start.y - sphere->radius - sector_2->largest_radius
			&& new_pos.z >= sector_2->start.z - sphere->radius - sector_2->largest_radius;
		if (within_range) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
	// all negative
	heading_towards =
		sphere->vel.x <= 0.0 && sector->pos.x != 0
		&& sphere->vel.y <= 0.0 && sector->pos.y != 0
		&& sphere->vel.z <= 0.0 && sector->pos.z != 0;
	if (heading_towards) {
		struct sector_s *sector_2 = &sim_data.sectors[sector->pos.x - 1][sector->pos.y - 1][sector->pos.z - 1];
		within_range =
			new_pos.x <= sector_2->end.x + sphere->radius + sector_2->largest_radius
			&& new_pos.y <= sector_2->end.y + sphere->radius + sector_2->largest_radius
			&& new_pos.z <= sector_2->end.z + sphere->radius + sector_2->largest_radius;
		if (within_range) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
	// x positive, y positive, z negative
	heading_towards =
		sphere->vel.x >= 0.0 && sector->pos.x != sim_data.sector_dims[X_AXIS] - 1
		&& sphere->vel.y >= 0.0 && sector->pos.y != sim_data.sector_dims[Y_AXIS] - 1
		&& sphere->vel.z <= 0.0 && sector->pos.z != 0;
	if (heading_towards) {
		struct sector_s *sector_2 = &sim_data.sectors[sector->pos.x + 1][sector->pos.y + 1][sector->pos.z - 1];
		within_range =
			new_pos.x >= sector_2->start.x - sphere->radius - sector_2->largest_radius
			&& new_pos.y >= sector_2->start.y - sphere->radius - sector_2->largest_radius
			&& new_pos.z <= sector_2->end.z + sphere->radius + sector_2->largest_radius;
		if (within_range) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
	// x positive, y negative, z positive
	heading_towards =
		sphere->vel.x >= 0.0 && sector->pos.x != sim_data.sector_dims[X_AXIS] - 1
		&& sphere->vel.y <= 0.0 && sector->pos.y != 0
		&& sphere->vel.z >= 0.0 && sector->pos.z != sim_data.sector_dims[Z_AXIS] - 1;
	if (heading_towards) {
		struct sector_s *sector_2 = &sim_data.sectors[sector->pos.x + 1][sector->pos.y - 1][sector->pos.z + 1];
		within_range =
			new_pos.x >= sector_2->start.x - sphere->radius - sector_2->largest_radius
			&& new_pos.y <= sector_2->end.y + sphere->radius + sector_2->largest_radius
			&& new_pos.z >= sector_2->start.z - sphere->radius - sector_2->largest_radius;
		if (within_range) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
	// x positive, y negative, z negative
	heading_towards =
		sphere->vel.x >= 0.0 && sector->pos.x != sim_data.sector_dims[X_AXIS] - 1
		&& sphere->vel.y <= 0.0 && sector->pos.y != 0
		&& sphere->vel.z <= 0.0 && sector->pos.z != 0;
	if (heading_towards) {
		struct sector_s *sector_2 = &sim_data.sectors[sector->pos.x + 1][sector->pos.y - 1][sector->pos.z - 1];
		within_range =
			new_pos.x >= sector_2->start.x - sphere->radius - sector_2->largest_radius
			&& new_pos.y <= sector_2->end.y + sphere->radius + sector_2->largest_radius
			&& new_pos.z <= sector_2->end.z + sphere->radius + sector_2->largest_radius;
		if (within_range) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
	// x negative, y positive, z positive
	heading_towards =
		sphere->vel.x <= 0.0 && sector->pos.x != 0
		&& sphere->vel.y >= 0.0 && sector->pos.y != sim_data.sector_dims[Y_AXIS] - 1
		&& sphere->vel.z >= 0.0 && sector->pos.z != sim_data.sector_dims[Z_AXIS] - 1;
	if (heading_towards) {
		struct sector_s *sector_2 = &sim_data.sectors[sector->pos.x - 1][sector->pos.y + 1][sector->pos.z + 1];
		within_range =
			new_pos.x <= sector_2->end.x + sphere->radius + sector_2->largest_radius
			&& new_pos.y >= sector_2->start.y - sphere->radius - sector_2->largest_radius
			&& new_pos.z >= sector_2->start.z - sphere->radius - sector_2->largest_radius;
		if (within_range) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
	// x negative, y positive, z negative
	heading_towards =
		sphere->vel.x <= 0.0 && sector->pos.x != 0
		&& sphere->vel.y >= 0.0 && sector->pos.y != sim_data.sector_dims[Y_AXIS] - 1
		&& sphere->vel.z <= 0.0 && sector->pos.z != 0;
	if (heading_towards) {
		struct sector_s *sector_2 = &sim_data.sectors[sector->pos.x - 1][sector->pos.y + 1][sector->pos.z - 1];
		within_range =
			new_pos.x <= sector_2->end.x + sphere->radius + sector_2->largest_radius
			&& new_pos.y >= sector_2->start.y - sphere->radius - sector_2->largest_radius
			&& new_pos.z <= sector_2->end.z + sphere->radius + sector_2->largest_radius;
		if (within_range) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
	// x negative, y negative, z positive
	heading_towards =
		sphere->vel.x <= 0.0 && sector->pos.x != 0
		&& sphere->vel.y <= 0.0 && sector->pos.y != 0
		&& sphere->vel.z >= 0.0 && sector->pos.z != sim_data.sector_dims[Z_AXIS] - 1;
	if (heading_towards) {
		struct sector_s *sector_2 = &sim_data.sectors[sector->pos.x - 1][sector->pos.y - 1][sector->pos.z + 1];
		within_range =
			new_pos.x <= sector_2->end.x + sphere->radius + sector_2->largest_radius
			&& new_pos.y <= sector_2->end.y + sphere->radius + sector_2->largest_radius
			&& new_pos.z >= sector_2->start.z - sphere->radius - sector_2->largest_radius;
		if (within_range) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}
}

// For each sphere check which sectors it is going towards.
// If by the time of the current soonest event it is within a certain distance 
// of any sector it is travelling towards we must check for partial crossings.
static void find_partial_crossing_events_for_sector(const struct sector_s *sector) {
	int i;
	for (i = 0; i < sector->num_spheres; i++) {
		const struct sphere_s *sphere = &sector->spheres[i];
		union vector_3d new_pos;
		new_pos.x = sphere->pos.x + (sphere->vel.x * event_details.time);
		new_pos.y = sphere->pos.y + (sphere->vel.y * event_details.time);
		new_pos.z = sphere->pos.z + (sphere->vel.z * event_details.time);
		find_partial_crossing_events_for_sector_directly_adjacent(sphere, sector, new_pos);
		find_partial_crossing_events_for_sector_diagonally_adjacent(sphere, sector, new_pos);
		find_partial_crossing_events_for_sector_diagonally_adjacent_three_axes(sphere, sector, new_pos);
	}
}

// Finds when all spheres in a given sector will collide with other and returns
// the soonest time.
static void find_collision_times_between_spheres_in_sector(const struct sector_s *sector) {
	int i, j;
	for (i = 0; i < sector->num_spheres - 1; i++) {
		struct sphere_s *s1 = &sector->spheres[i];
		for (j = i + 1; j < sector->num_spheres; j++) {
			struct sphere_s *s2 = &sector->spheres[j];
			double time = find_collision_time_spheres(s1, s2);
			if (time < event_details.time) {
				set_event_details(time, COL_TWO_SPHERES, s1, s2, AXIS_NONE, sector, NULL);
			}
		}
	}
}

// Finds time to both collide with grid and to cross sector boundaries.
// Can optimise further so that grid boundaries are not checked if another
// sector will be entered first.
static void find_collision_times_grid_boundary_for_sector(const struct sector_s *sector) {
	enum axis axis = COL_NONE;
	int i;
	for (i = 0; i < sector->num_spheres; i++) {
		struct sphere_s *sphere = &sector->spheres[i];
		double time = find_collision_time_grid(sphere, &axis);
		if (time < event_details.time) {
			set_event_details(time, COL_SPHERE_WITH_GRID, sphere, NULL, axis, sector, NULL);
		}
		struct sector_s *temp_dest = NULL;
		time = find_collision_time_sector(sector, sphere, &temp_dest);
		if (time < event_details.time) {
			set_event_details(time, COL_SPHERE_WITH_SECTOR, sphere, NULL, AXIS_NONE, sector, temp_dest);
		}
	}
}

// Given a sector finds the soonest occuring event.
// The event will be either two spheres colliding, a sphere colliding with a grid
// boundary, or a sphere passing into another sector.
// Any partial crossings will be handled once this function has been called for each sector.
void find_event_times_for_sector(const struct sector_s *sector) {
	if (sector->num_spheres == 0) {
		return;
	}
	find_collision_times_between_spheres_in_sector(sector);
	find_collision_times_grid_boundary_for_sector(sector);
	find_partial_crossing_events_for_sector(sector);
}
