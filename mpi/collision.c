#include <float.h>
#include <math.h>

#include "collision.h"
#include "event.h"
#include "grid.h"
#include "mpi_vars.h"
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
	double dist = 0.0;
	if (axis_vel > 0.0) {
		dist = bound_end - axis_pos - radius;
	} else {
		dist = bound_start - axis_pos + radius;
	}
	return dist / axis_vel;
}

// Note: a2, a3 may be AXIS_NONE and dir_2, dir_3 may be DIR_NONE.
// In those cases the modifier will be 0 so the position on those axes won't change.
static struct sector_s *get_sector_adjacent_on_axes(
		const struct sector_s *s, 
		const enum axis a1, const enum axis a2, const enum axis a3,
		const enum direction dir_1, const enum direction dir_2, const enum direction dir_3
	){
	int x = s->pos.x + SECTOR_MODIFIERS[dir_1][a1][X_AXIS] + SECTOR_MODIFIERS[dir_2][a2][X_AXIS] + SECTOR_MODIFIERS[dir_3][a3][X_AXIS];
	int y = s->pos.y + SECTOR_MODIFIERS[dir_1][a1][Y_AXIS] + SECTOR_MODIFIERS[dir_2][a2][Y_AXIS] + SECTOR_MODIFIERS[dir_3][a3][Y_AXIS];
	int z = s->pos.z + SECTOR_MODIFIERS[dir_1][a1][Z_AXIS] + SECTOR_MODIFIERS[dir_2][a2][Z_AXIS] + SECTOR_MODIFIERS[dir_3][a3][Z_AXIS];
	return &sim_data.sectors[x][y][z];
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
					*dest = get_sector_adjacent_on_axes(sector, a, AXIS_NONE, AXIS_NONE, DIR_POSITIVE, DIR_NONE, DIR_NONE);
				} else {
					*dest = get_sector_adjacent_on_axes(sector, a, AXIS_NONE, AXIS_NONE, DIR_NEGATIVE, DIR_NONE, DIR_NONE);
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
static void find_partial_crossing_events_between_sphere_and_sector(struct sphere_s *sphere_1, struct sector_s *sector_1, struct sector_s *sector_2) {
	int j;
	for (j = 0; j < sector_2->num_spheres; j++) {
		struct sphere_s *sphere_2 = &sector_2->spheres[j];
		double time = find_collision_time_spheres(sphere_1, sphere_2);
		set_event_details(time, COL_TWO_SPHERES_PARTIAL_CROSSING, sphere_1, sphere_2, AXIS_NONE, sector_1, sector_2);
	}
}

// Returns true if the sphere is within the minimum distance from an adjacent
// sector to need partial crossing checks.
static bool is_sphere_within_range_of_sector_one_axis(
		const struct sphere_s *sphere, const union vector_3d new_pos, 
		const struct sector_s *dest, const enum axis a, const enum direction dir
	){
	return
		(dir == DIR_POSITIVE && new_pos.vals[a] >= dest->start.vals[a] - sphere->radius - dest->largest_radius) ||
		(dir == DIR_NEGATIVE && new_pos.vals[a] <= dest->end.vals[a] + sphere->radius + dest->largest_radius);
}

static bool is_sphere_within_range_of_sector_two_axis(
		const struct sphere_s *sphere, const union vector_3d new_pos, 
		const struct sector_s *dest, const enum axis a1, const enum axis a2,
		const enum direction dir_1, const enum direction dir_2
	){
		return
			is_sphere_within_range_of_sector_one_axis(sphere, new_pos, dest, a1, dir_1) &&
			is_sphere_within_range_of_sector_one_axis(sphere, new_pos, dest, a2, dir_2);
}

static bool is_sphere_within_range_of_sector_three_axis(
		const struct sphere_s *sphere, const union vector_3d new_pos, 
		const struct sector_s *dest, const enum axis a1, const enum axis a2, const enum axis a3,
		const enum direction dir_1, const enum direction dir_2, const enum direction dir_3
	){
		return
			is_sphere_within_range_of_sector_one_axis(sphere, new_pos, dest, a1, dir_1) &&
			is_sphere_within_range_of_sector_one_axis(sphere, new_pos, dest, a2, dir_2) &&
			is_sphere_within_range_of_sector_one_axis(sphere, new_pos, dest, a3, dir_3);
}

// Returns true if the sphere is heading towards the containing sector's 
// boundary along the specified axis in the specified direction.
static bool is_sphere_heading_towards_sector_one_axis(
		const struct sphere_s *sphere, const struct sector_s *sector, 
		const enum axis a, const enum direction dir
	){
	return
		(dir == DIR_POSITIVE && sphere->vel.vals[a] >= 0.0 && sector->pos.vals[a] != sim_data.sector_dims[a] - 1) ||
		(dir == DIR_NEGATIVE && sphere->vel.vals[a] <= 0.0 && sector->pos.vals[a] != 0);
}

static bool is_sphere_heading_towards_sector_two_axis(
		const struct sphere_s *sphere, const struct sector_s *sector, 
		const enum axis a1, const enum axis a2,
		const enum direction dir_1, const enum direction dir_2
	){
	return
		is_sphere_heading_towards_sector_one_axis(sphere, sector, a1, dir_1) &&
		is_sphere_heading_towards_sector_one_axis(sphere, sector, a2, dir_2);
}

static bool is_sphere_heading_towards_sector_three_axis(
		const struct sphere_s *sphere, const struct sector_s *sector, 
		const enum axis a1, const enum axis a2, const enum axis a3,
		const enum direction dir_1, const enum direction dir_2, const enum direction dir_3
	){
	return
		is_sphere_heading_towards_sector_one_axis(sphere, sector, a1, dir_1) &&
		is_sphere_heading_towards_sector_one_axis(sphere, sector, a2, dir_2) &&
		is_sphere_heading_towards_sector_one_axis(sphere, sector, a3, dir_3);
}

// Checks for partial crossings with sectors that are immediately adjacent to 
// the left/right, top/bottom or front/back.
static void find_partial_crossing_events_for_sector_directly_adjacent(struct sphere_s *sphere, struct sector_s *sector, const union vector_3d new_pos) {
	enum axis a;
	enum direction dir;
	for(dir = DIR_POSITIVE; dir <= DIR_NEGATIVE; dir++){
		for (a = X_AXIS; a <= Z_AXIS; a++) {
			if (is_sphere_heading_towards_sector_one_axis(sphere, sector, a, dir)) {
				struct sector_s *sector_2 = get_sector_adjacent_on_axes(sector, a, AXIS_NONE, AXIS_NONE, dir, DIR_NONE, DIR_NONE);
				if (is_sphere_within_range_of_sector_one_axis(sphere, new_pos, sector_2, a, dir)) {
					find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
				}
			}
		}
	}
}

static void find_partial_crossing_events_for_sector_diagonally_adjacent_helper(
		struct sphere_s *sphere, struct sector_s *sector, const union vector_3d new_pos, 
		const enum axis a1, const enum axis a2, const enum direction dir_1, const enum direction dir_2
	){
	if(is_sphere_heading_towards_sector_two_axis(sphere, sector, a1, a2, dir_1, dir_2)){
		struct sector_s *sector_2 = get_sector_adjacent_on_axes(sector, a1, a2, AXIS_NONE, dir_1, dir_2, DIR_NONE);
		if(is_sphere_within_range_of_sector_two_axis(sphere, new_pos, sector_2, a1, a2, dir_1, dir_2)){
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
		}
	}	
}

// Checks for partial crossings with sectors that are diagonally adjacent along two axes.
static void find_partial_crossing_events_for_sector_diagonally_adjacent(struct sphere_s *sphere, struct sector_s *sector, const union vector_3d new_pos) {
	// x/y combinations
	if (sim_data.xy_check_needed) {
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, X_AXIS, Y_AXIS, DIR_POSITIVE, DIR_POSITIVE);
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, X_AXIS, Y_AXIS, DIR_POSITIVE, DIR_NEGATIVE);
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, X_AXIS, Y_AXIS, DIR_NEGATIVE, DIR_POSITIVE);
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, X_AXIS, Y_AXIS, DIR_NEGATIVE, DIR_NEGATIVE);
	}
	// x/z combinations
	if (sim_data.xz_check_needed) {
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, X_AXIS, Z_AXIS, DIR_POSITIVE, DIR_POSITIVE);
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, X_AXIS, Z_AXIS, DIR_POSITIVE, DIR_NEGATIVE);
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, X_AXIS, Z_AXIS, DIR_NEGATIVE, DIR_POSITIVE);
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, X_AXIS, Z_AXIS, DIR_NEGATIVE, DIR_NEGATIVE);
	}
	// y/z combinations
	if (sim_data.yz_check_needed) {
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, Y_AXIS, Z_AXIS, DIR_POSITIVE, DIR_POSITIVE);
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, Y_AXIS, Z_AXIS, DIR_POSITIVE, DIR_NEGATIVE);
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, Y_AXIS, Z_AXIS, DIR_NEGATIVE, DIR_POSITIVE);
		find_partial_crossing_events_for_sector_diagonally_adjacent_helper(sphere, sector, new_pos, Y_AXIS, Z_AXIS, DIR_NEGATIVE, DIR_NEGATIVE);
	}
}

// Checks for partial crossings with sectors that are diagonally adjacent along three axes.
// Ex: if passed sector at [0][0][0] it will check [1][1][1].
static void find_partial_crossing_events_for_sector_diagonally_adjacent_three_axes(struct sphere_s *sphere, struct sector_s *sector, const union vector_3d new_pos) {
	if(!sim_data.xyz_check_needed){
		return;
	}
	enum direction dir_1, dir_2, dir_3;
	for(dir_1 = DIR_POSITIVE; dir_1 <= DIR_NEGATIVE; dir_1++){
		for(dir_2 = DIR_POSITIVE; dir_2 <= DIR_NEGATIVE; dir_2++){
			for(dir_3 = DIR_POSITIVE; dir_3 <= DIR_NEGATIVE; dir_3++){
				if (is_sphere_heading_towards_sector_three_axis(sphere, sector, X_AXIS, Y_AXIS, Z_AXIS, dir_1, dir_2, dir_3)) {
					struct sector_s *sector_2 = get_sector_adjacent_on_axes(sector, X_AXIS, Y_AXIS, Z_AXIS, dir_1, dir_2, dir_3);
					if (is_sphere_within_range_of_sector_three_axis(sphere, new_pos, sector_2, X_AXIS, Y_AXIS, Z_AXIS, dir_1, dir_2, dir_3)) {
						find_partial_crossing_events_between_sphere_and_sector(sphere, sector, sector_2);
					}
				}
			}
		}
	}
}
// For each sphere check which sectors it is going towards.
// If by the time of the current soonest event it is within a certain distance 
// of any sector it is travelling towards we must check for partial crossings.
static void find_partial_crossing_events_for_sector(struct sector_s *sector) {
	int64_t i;
	for (i = 0; i < sector->num_spheres; i++) {
		struct sphere_s *sphere = &sector->spheres[i];
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
// If each sector is finding its own times start_offset will be 0 and inc will be 1.
// If neighbours are helping a sector then start_offset will be >= 0 depending on
// the neighbour's id and inc will be num_neighbours + 1
// If all processes are helping one sector then start_offset will be the process'
// rank in the grid and inc will be NUM_NODES.
static void find_collision_times_between_spheres_in_sector(struct sector_s *sector, int64_t start_offset, int64_t inc) {
	int64_t i, j;
	for (i = 0; i < sector->num_spheres - 1; i++) {
		struct sphere_s *s1 = &sector->spheres[i];
		for (j = i + 1 + start_offset; j < sector->num_spheres; j = j + inc) {
			struct sphere_s *s2 = &sector->spheres[j];
			double time = find_collision_time_spheres(s1, s2);
			set_event_details(time, COL_TWO_SPHERES, s1, s2, AXIS_NONE, sector, NULL);
		}
	}
}

// Finds time to both collide with grid and to cross sector boundaries.
// TODO: Can optimise further so that grid boundaries are not checked if another
// sector will be entered first.
// If each sector is finding its own times start_offset will be 0 and inc will be 1.
// If neighbours are helping a sector then start_offset will be >= 0 depending on
// the neighbour's id and inc will be num_neighbours + 1
// If all processes are helping one sector then start_offset will be the process'
// rank in the grid and inc will be NUM_NODES.
static void find_collision_times_grid_boundary_for_sector(struct sector_s *sector, int64_t start_offset, int64_t inc) {
	enum axis axis = COL_NONE;
	int64_t i;
	for (i = start_offset; i < sector->num_spheres; i = i + inc) {
		struct sphere_s *sphere = &sector->spheres[i];
		double time = find_collision_time_grid(sphere, &axis);
		set_event_details(time, COL_SPHERE_WITH_GRID, sphere, NULL, axis, sector, NULL);
		struct sector_s *temp_dest = NULL;
		time = find_collision_time_sector(sector, sphere, &temp_dest);
		set_event_details(time, COL_SPHERE_WITH_SECTOR, sphere, NULL, AXIS_NONE, sector, temp_dest);
	}
}

static void find_event_times_all_help(struct sector_s *sector_to_help){
	if(SECTOR->id != sector_to_help->id){
		helping = true; // changes behaviour of set_event_details
		reset_event_details_helping();
	} else {
		reset_event_details();
	}
	find_collision_times_between_spheres_in_sector(sector_to_help, GRID_RANK, NUM_NODES);
	find_collision_times_grid_boundary_for_sector(sector_to_help, GRID_RANK, NUM_NODES);
	reduce_help_events(sector_to_help);
	helping = false;
}

static void find_event_times_neighbours_help(struct sector_s *sector_to_help){
	if(SECTOR->id != sector_to_help->id){
		helping = true; // changes behaviour of set_event_details
		reset_event_details_helping();
	} else {
		reset_event_details();
	}
	if(SECTOR->id == sector_to_help->id || sector_to_help->is_neighbour){ // non-neighbours skip this and send DBL_MAX as a time in the reduce
		int64_t start;
		if(sector_to_help->id == SECTOR->id){
			start = 0;
		} else {
			start = sector_to_help->my_id_index + 1;
		}
		find_collision_times_between_spheres_in_sector(sector_to_help, start, sector_to_help->num_neighbours + 1);
		find_collision_times_grid_boundary_for_sector(sector_to_help, start, sector_to_help->num_neighbours + 1);
	}
	reduce_help_events(sector_to_help);
	helping = false;
}

static void find_event_times_normal(struct sector_s *sector){
	if(PRIOR_TIME_VALID == false){
		reset_event_details();
		find_collision_times_between_spheres_in_sector(sector, 0, 1);
		find_collision_times_grid_boundary_for_sector(sector, 0, 1);
	}
}

// If ALL_HELP is enabled then all processes will help find event times for the
// sectors with invalid times.
// Otherwise each sector just finds its own event time.
// Partial crossings are not checked when helping sectors.
// Each process does this itself. 
// The NUM_NODES check is needed to prevent the helping functions being
// used on the first iteration if NUM_NODES is 1 or 2.
void find_event_times() {	
	if(ALL_HELP && num_invalid == 1 && NUM_NODES > 1){
		find_event_times_all_help(invalid_1);
	} else if(ALL_HELP && num_invalid == 2 && NUM_NODES > 2){
		find_event_times_all_help(invalid_1);
		find_event_times_all_help(invalid_2);
	} else if(!ALL_HELP && num_invalid == 1 && NUM_NODES > 1){
		find_event_times_neighbours_help(invalid_1);
	} else if(!ALL_HELP && num_invalid == 2 && NUM_NODES > 2){
		find_event_times_neighbours_help(invalid_1);
		find_event_times_neighbours_help(invalid_2);
	} else {
		find_event_times_normal(SECTOR);
	}
	find_partial_crossing_events_for_sector(SECTOR);

}
