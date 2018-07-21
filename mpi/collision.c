#include <float.h>
#include <math.h>

#include "collision.h"
#include "grid.h"
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
double find_collision_time_spheres(const struct sphere_s *s1, const struct sphere_s *s2) {
	union vector_3d rel_vel = { 
		.x = s1->vel.x - s2->vel.x, 
		.y = s1->vel.y - s2->vel.y, 
		.z = s1->vel.z - s2->vel.z 
	};
	union vector_3d rel_pos = { 
		.x = s2->pos.x - s1->pos.x, 
		.y = s2->pos.y - s1->pos.y, 
		.z = s2->pos.z - s1->pos.z 
	};
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
double find_collision_time_sector(const struct sector_s *sector, const struct sphere_s *sphere, struct sector_s **dest) {
	double time = DBL_MAX;
	enum axis a;
	for (a = X_AXIS; a <= Z_AXIS; a++) {
		if (sphere->vel.vals[a] != 0.0 && !(sector->pos.vals[a] == 0.0 && sphere->vel.vals[a] < 0.0) && !(sector->pos.vals[a] == SECTOR_DIMS[a] - 1 && sphere->vel.vals[a] > 0.0)) {
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
double find_collision_time_grid(const struct sphere_s *s, enum axis *col_axis) {
	double time = DBL_MAX;
	*col_axis = AXIS_NONE;
	enum axis a;
	for (a = X_AXIS; a <= Z_AXIS; a++) {
		if (s->vel.vals[a] != 0) {
			double temp_time = find_time_to_cross_boundary(0.0, grid->size.vals[a], s->vel.vals[a], s->pos.vals[a], s->radius);
			if (temp_time < time) {
				time = temp_time;
				*col_axis = a;
			}
		}
	}
	return time;
}

// For details on how this works see:
// https://www.gamasutra.com/view/feature/131424/pool_hall_lessons_fast_accurate_.php?page=3
void apply_bounce_between_spheres(struct sphere_s *s1, struct sphere_s *s2) {
	union vector_3d rel_pos = { 
		.x = s1->pos.x - s2->pos.x, 
		.y = s1->pos.y - s2->pos.y, 
		.z = s1->pos.z - s2->pos.z 
	};
	normalise_vector_3d(&rel_pos);
	double dp1 = get_vector_3d_dot_product(&rel_pos, &s1->vel);
	double dp2 = get_vector_3d_dot_product(&rel_pos, &s2->vel);
	double p = (2.0 * (dp1 - dp2)) / (s1->mass + s2->mass);
	// Sphere one first
	s1->vel.x = s1->vel.x - (p * s2->mass * rel_pos.x);
	s1->vel.y = s1->vel.y - (p * s2->mass * rel_pos.y);
	s1->vel.z = s1->vel.z - (p * s2->mass * rel_pos.z);
	// Now sphere two
	s2->vel.x = s2->vel.x + (p * s1->mass * rel_pos.x);
	s2->vel.y = s2->vel.y + (p * s1->mass * rel_pos.y);
	s2->vel.z = s2->vel.z + (p * s1->mass * rel_pos.z);
}
