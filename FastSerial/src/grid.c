#include <float.h>
#include <stdlib.h>
#include <stdio.h>

#include "collision.h"
#include "grid.h"

static int count = 0; // number of spheres that have been initialised

// Creates a number of spheres in a line
static void create_spheres(int num, double x_start, double y_start, double z_start, double x_inc, double y_inc, double z_inc) {
	double x = x_start;
	double y = y_start;
	double z = z_start;
	int i;
	for (i = 0; i < num; i++) {
		spheres[count].id = count;
		spheres[count].pos.x = x;
		spheres[count].pos.y = y;
		spheres[count].pos.z = z;
		spheres[count].vel.x = rand() / (RAND_MAX + 1.0);
		spheres[count].vel.y = rand() / (RAND_MAX + 1.0);
		spheres[count].vel.z = rand() / (RAND_MAX + 1.0);
		spheres[count].mass = 1.0;
		spheres[count].radius = 1.0;
		if (grid->uses_sectors) {
			add_sphere_to_correct_sector(&spheres[count]);
		}
		x += x_inc;
		y += y_inc;
		z += z_inc;
		count++;
	}
}

static void create_sphere(double x, double y, double z, double x_vel, double y_vel, double z_vel) {
	spheres[count].id = count;
	spheres[count].pos.x = x;
	spheres[count].pos.y = y;
	spheres[count].pos.z = z;
	spheres[count].vel.x = x_vel;
	spheres[count].vel.y = y_vel;
	spheres[count].vel.z = z_vel;
	spheres[count].mass = 1.0;
	spheres[count].radius = 1.0;
	if (grid->uses_sectors) {
		add_sphere_to_correct_sector(&spheres[count]);
	}
	count++;
}

// Generates spheres with random velocities;
// Position is hardcoded for now
static void init_spheres() {
	NUM_SPHERES = 4000;
	spheres = calloc(NUM_SPHERES, sizeof(struct sphere_s));
	srand(123);
	create_spheres(250, 10.0, 10.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 40.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 100.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 140.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 360.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 390.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 460.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 490.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 510.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 540.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 610.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 640.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 860.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 890.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 960.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 990.0, 10.0, 3.5, 0.0, 0.0);
}

// Note: size of grid in each dimension should be divisible by number of
// sectors in that dimension.
static void init_sectors(union vector_3i *divs) {
	SECTOR_DIMS[X_AXIS] = divs->x;
	SECTOR_DIMS[Y_AXIS] = divs->y;
	SECTOR_DIMS[Z_AXIS] = divs->z;
	grid->sectors = calloc(SECTOR_DIMS[X_AXIS], sizeof(struct sector_s **));
	struct sector_s *z_arr = calloc(SECTOR_DIMS[X_AXIS] * SECTOR_DIMS[Y_AXIS] * SECTOR_DIMS[Z_AXIS], sizeof(struct sector_s));
	int i, j, k;
	for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
		grid->sectors[i] = calloc(SECTOR_DIMS[Y_AXIS], sizeof(struct sector_s *));
		for (j = 0; j < SECTOR_DIMS[Y_AXIS]; j++) {
			int idx = (i * SECTOR_DIMS[Y_AXIS] * SECTOR_DIMS[Z_AXIS]) + (j * SECTOR_DIMS[Z_AXIS]);
			grid->sectors[i][j] = &z_arr[idx];
		}
	}
	double x_inc = (grid->end.x - grid->start.x) / SECTOR_DIMS[X_AXIS];
	double y_inc = (grid->end.y - grid->start.y) / SECTOR_DIMS[Y_AXIS];
	double z_inc = (grid->end.z - grid->start.z) / SECTOR_DIMS[Z_AXIS];
	int count = 0;
	for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
		for (j = 0; j < SECTOR_DIMS[Y_AXIS]; j++) {
			for (k = 0; k < SECTOR_DIMS[Z_AXIS]; k++) {
				struct sector_s *s = &grid->sectors[i][j][k];
				s->start.x = grid->start.x + x_inc * i;
				s->end.x = s->start.x + x_inc;
				s->start.y = grid->start.y + y_inc * j;
				s->end.y = s->start.y + y_inc;
				s->start.z = grid->start.z + z_inc * k;
				s->end.z = s->start.z + z_inc;
				s->pos.x = i;
				s->pos.y = j;
				s->pos.z = k;
				s->num_spheres = 0;
				s->max_spheres = 2000;
				s->spheres = calloc(s->max_spheres, sizeof(struct sphere_s *));
				count++;
			}
		}
	}
}

// Using hardcoded values for now
void init_grid(union vector_3i *divs, union vector_3d *grid_start, union vector_3d *grid_end) {
	grid = calloc(1, sizeof(struct grid_s));
	grid->start.x = grid_start->x;
	grid->start.y = grid_start->y;
	grid->start.z = grid_start->z;
	grid->end.x = grid_end->x;
	grid->end.y = grid_end->y;
	grid->end.z = grid_end->z;
	if (divs->x == 1 && divs->y == 1 && divs->z == 1) {
		grid->uses_sectors = false;
	} else {
		grid->uses_sectors = true;
		init_sectors(divs);
	}
	init_spheres();
#ifdef RECORD_STATS
	grid->num_two_sphere_collisions = 0;
	grid->num_grid_collisions = 0;
	grid->num_sector_transfers = 0;
#endif
}

// This updates the positions and velocities of each sphere once the next
// event and the time it occurs are known.
static void update_spheres() {
	int i;
	for (i = 0; i < NUM_SPHERES; i++) {
		struct sphere_s *s = &(spheres[i]);
		update_sphere_position(s, event_details.time);
	}
	if (event_details.type == COL_SPHERE_WITH_GRID) {
		event_details.sphere_1->vel.vals[event_details.grid_axis] *= -1.0;
#ifdef RECORD_STATS
		grid->num_grid_collisions++;
#endif
	} else if (event_details.type == COL_TWO_SPHERES) {
		apply_bounce_between_spheres(event_details.sphere_1, event_details.sphere_2);
#ifdef RECORD_STATS
		grid->num_two_sphere_collisions++;
#endif
	} else if (event_details.type == COL_SPHERE_WITH_SECTOR) {
		remove_sphere_from_sector(event_details.source_sector, event_details.sphere_1);
		add_sphere_to_sector(event_details.dest_sector, event_details.sphere_1);
#ifdef RECORD_STATS
		grid->num_sector_transfers++;
#endif
	}
}

// For debugging
// Ensures each sphere is located within the sector responsible for it.
// Helps catch any issues with transfering spheres between sectors.
static void sanity_check() {
	if (grid->uses_sectors == false) {
		return;
	}
	for (int x = 0; x < SECTOR_DIMS[X_AXIS]; x++) {
		for (int y = 0; y < SECTOR_DIMS[Y_AXIS]; y++) {
			for (int z = 0; z < SECTOR_DIMS[Z_AXIS]; z++) {
				struct sector_s *s = &grid->sectors[x][y][z];
				int i;
				for (i = 0; i < s->num_spheres; i++) {
					struct sphere_s *sphere = s->spheres[i];
					int error = 0;
					enum axis a;
					for (a = X_AXIS; a <= Z_AXIS; a++) {
						if (sphere->pos.vals[a] > s->end.vals[a]) {
							error = 1;
						}
						if (sphere->pos.vals[a] < s->start.vals[a]) {
							error = 1;
						}
					}
					if (error) {
						printf("Sphere not in correct sector!\n");
						getchar();
						exit(1);
					}
				}
			}
		}
	}
}

static void find_event_times_no_dd() {
	int i, j;
	for (i = 0; i < NUM_SPHERES; i++) {
		struct sphere_s *s1 = &(spheres[i]);
		enum AXIS axis;
		double time = find_collision_time_grid(s1, &axis);
		if (time < event_details.time) {
			event_details.type = COL_SPHERE_WITH_GRID;
			event_details.grid_axis = axis;
			event_details.time = time;
			event_details.sphere_1 = s1;
			event_details.sphere_2 = NULL;
		}
		for (j = i + 1; j < NUM_SPHERES; j++) {
			struct sphere_s *s2 = &(spheres[j]);
			time = find_collision_time_spheres(s1, s2);
			if (time < event_details.time) {
				event_details.type = COL_TWO_SPHERES;
				event_details.grid_axis = AXIS_NONE;
				event_details.time = time;
				event_details.sphere_1 = s1;
				event_details.sphere_2 = s2;
			}
		}
	}
}

double update_grid(double limit, double time_elapsed) {
	sanity_check();
	// First reset records.
	event_details.time = DBL_MAX;
	event_details.sphere_1 = NULL;
	event_details.sphere_2 = NULL;
	event_details.source_sector = NULL;
	event_details.dest_sector = NULL;
	event_details.type = COL_NONE;
	event_details.grid_axis = AXIS_NONE;
	// Now find event + time of event
	if (grid->uses_sectors) { // domain decomposition
		find_event_times_for_all_sectors();
		find_partial_crossing_events_for_all_sectors();
	} else { // no domain decomposition 
		find_event_times_no_dd();
	}
	// Final event may take place after time limit, so cut it short
	if (limit - time_elapsed < event_details.time) {
		event_details.time = limit - time_elapsed;
		event_details.type = COL_NONE;
	}
	// Lastly move forward to the next event
	update_spheres();
	sanity_check();
	return event_details.time;
}