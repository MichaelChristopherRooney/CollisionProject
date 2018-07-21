#include <float.h>
#include <stdlib.h>
#include <stdio.h>

#include "collision.h"
#include "grid.h"
#include "params.h"
#include "vector_3.h"

static FILE *initial_state_fp;

// Loads spheres from the specified inital state file
// This file contains every sphere, and we need to check if the sphere belongs
// to the sector the current MPI node is responsible for.
static void load_spheres() {
	// result is just to shut up gcc's warnings
	int result = fread(&NUM_SPHERES, sizeof(int64_t), 1, initial_state_fp);
	spheres = calloc(NUM_SPHERES, sizeof(struct sphere_s));
	int64_t i;
	for(i = 0; i < NUM_SPHERES; i++){
		result = fread(&spheres[i].id, sizeof(int64_t), 1, initial_state_fp);
		result = fread(&spheres[i].pos.x, sizeof(double), 1, initial_state_fp);
		result = fread(&spheres[i].pos.y, sizeof(double), 1, initial_state_fp);
		result = fread(&spheres[i].pos.z, sizeof(double), 1, initial_state_fp);
		result = fread(&spheres[i].vel.x, sizeof(double), 1, initial_state_fp);
		result = fread(&spheres[i].vel.y, sizeof(double), 1, initial_state_fp);
		result = fread(&spheres[i].vel.z, sizeof(double), 1, initial_state_fp);
		result = fread(&spheres[i].mass, sizeof(double), 1, initial_state_fp);
		result = fread(&spheres[i].radius, sizeof(double), 1, initial_state_fp);
	}
}

// Note: size of grid in each dimension should be divisible by number of
// sectors in that dimension.
static void init_sectors() {
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
	double x_inc = grid->size.x / SECTOR_DIMS[X_AXIS];
	double y_inc = grid->size.y / SECTOR_DIMS[Y_AXIS];
	double z_inc = grid->size.z / SECTOR_DIMS[Z_AXIS];
	int count = 0;
	for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
		for (j = 0; j < SECTOR_DIMS[Y_AXIS]; j++) {
			for (k = 0; k < SECTOR_DIMS[Z_AXIS]; k++) {
				struct sector_s *s = &grid->sectors[i][j][k];
				s->start.x = x_inc * i;
				s->end.x = s->start.x + x_inc;
				s->start.y = y_inc * j;
				s->end.y = s->start.y + y_inc;
				s->start.z = z_inc * k;
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

// Loads the grid from the initial state file
void init_grid(double time_limit) {
	initial_state_fp = fopen(initial_state_file, "rb");
	grid = calloc(1, sizeof(struct grid_s));
	// result is just to shut up gcc's warnings
	int result = fread(&grid->size.x, sizeof(double), 1, initial_state_fp);
	result = fread(&grid->size.y, sizeof(double), 1, initial_state_fp);
	result = fread(&grid->size.z, sizeof(double), 1, initial_state_fp);
	init_sectors();
	grid->elapsed_time = 0.0;
	grid->time_limit = time_limit;
	load_spheres();
	grid->num_two_sphere_collisions = 0;
	grid->num_grid_collisions = 0;
	grid->num_sector_transfers = 0;
	fclose(initial_state_fp);
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
		grid->num_grid_collisions++;
	} else if (event_details.type == COL_TWO_SPHERES) {
		apply_bounce_between_spheres(event_details.sphere_1, event_details.sphere_2);
		grid->num_two_sphere_collisions++;
	} else if (event_details.type == COL_SPHERE_WITH_SECTOR) {
		remove_sphere_from_sector(event_details.source_sector, event_details.sphere_1);
		add_sphere_to_sector(event_details.dest_sector, event_details.sphere_1);
		grid->num_sector_transfers++;
	}
}

// For debugging
// Ensures each sphere is located within the sector responsible for it.
// Helps catch any issues with transfering spheres between sectors.
static void sanity_check() {
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

double update_grid() {
	//sanity_check();
	// First reset records.
	event_details.time = DBL_MAX;
	event_details.sphere_1 = NULL;
	event_details.sphere_2 = NULL;
	event_details.source_sector = NULL;
	event_details.dest_sector = NULL;
	event_details.type = COL_NONE;
	event_details.grid_axis = AXIS_NONE;
	// Now find event + time of event
	find_event_times_for_all_sectors();
	find_partial_crossing_events_for_all_sectors();
	// Final event may take place after time limit, so cut it short
	if (grid->time_limit - grid->elapsed_time < event_details.time) {
		event_details.time = grid->time_limit - grid->elapsed_time;
		event_details.type = COL_NONE;
	}
	// Lastly move forward to the next event
	update_spheres();
	//sanity_check();
	return event_details.time;
}
