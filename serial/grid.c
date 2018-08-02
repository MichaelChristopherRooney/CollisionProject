#include <float.h>
#include <stdlib.h>
#include <stdio.h>

#include "collision.h"
#include "event.h"
#include "grid.h"
#include "params.h"
#include "simulation.h"
#include "wrapper.h"
#include "vector_3.h"

// Loads the grid from the initial state file
void init_grid(double time_limit) {
	FILE *initial_state_fp = fopen(initial_state_file, "rb");
	fread_wrapper(&sim_data.grid_size.x, sizeof(double), 1, initial_state_fp);
	fread_wrapper(&sim_data.grid_size.y, sizeof(double), 1, initial_state_fp);
	fread_wrapper(&sim_data.grid_size.z, sizeof(double), 1, initial_state_fp);
	sim_data.num_sectors = SECTOR_DIMS[X_AXIS] * SECTOR_DIMS[Y_AXIS] * SECTOR_DIMS[Z_AXIS];
	if(sim_data.num_sectors > 1){
		sim_data.xy_check_needed = SECTOR_DIMS[X_AXIS] > 1 && SECTOR_DIMS[Y_AXIS] > 1;
		sim_data.xz_check_needed = SECTOR_DIMS[X_AXIS] > 1 && SECTOR_DIMS[Z_AXIS] > 1;
		sim_data.yz_check_needed = SECTOR_DIMS[Y_AXIS] > 1 && SECTOR_DIMS[Z_AXIS] > 1;
		init_sectors();
	}
	sim_data.elapsed_time = 0.0;
	sim_data.time_limit = time_limit;
	load_spheres(initial_state_fp);
	sim_data.num_two_sphere_collisions = 0;
	sim_data.num_grid_collisions = 0;
	sim_data.num_sector_transfers = 0;
	fclose(initial_state_fp);
}

// This updates the positions and velocities of each sphere once the next
// event and the time it occurs are known.
static void update_spheres() {
	int i;
	for (i = 0; i < sim_data.total_num_spheres; i++) {
		struct sphere_s *s = &(sim_data.spheres[i]);
		update_sphere_position(s, event_details.time);
	}
}

static void apply_event(){
	if (event_details.type == COL_SPHERE_WITH_GRID) {
		event_details.sphere_1->vel.vals[event_details.grid_axis] *= -1.0;
		sim_data.num_grid_collisions++;
	} else if (event_details.type == COL_TWO_SPHERES) {
		apply_bounce_between_spheres(event_details.sphere_1, event_details.sphere_2);
		sim_data.num_two_sphere_collisions++;
	} else if (event_details.type == COL_SPHERE_WITH_SECTOR) {
		remove_sphere_from_sector(event_details.source_sector, event_details.sphere_1);
		add_sphere_to_sector(event_details.dest_sector, event_details.sphere_1);
		sim_data.num_sector_transfers++;
	}
}

// For debugging
// Ensures each sphere is located within the sector responsible for it.
// Helps catch any issues with transfering spheres between sectors.
static void sanity_check() {
	if (sim_data.num_sectors == 1) {
		return;
	}
	int x, y, z;
	for (x = 0; x < SECTOR_DIMS[X_AXIS]; x++) {
		for (y = 0; y < SECTOR_DIMS[Y_AXIS]; y++) {
			for (z = 0; z < SECTOR_DIMS[Z_AXIS]; z++) {
				struct sector_s *s = &sim_data.sectors[x][y][z];
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

// Used to find the next event when domain decomposition is not used.
static void find_event_times_no_dd() {
	int i, j;
	for (i = 0; i < sim_data.total_num_spheres; i++) {
		struct sphere_s *s1 = &(sim_data.spheres[i]);
		enum axis a;
		double time = find_collision_time_grid(s1, &a);
		if (time < event_details.time) {
			event_details.type = COL_SPHERE_WITH_GRID;
			event_details.grid_axis = a;
			event_details.time = time;
			event_details.sphere_1 = s1;
			event_details.sphere_2 = NULL;
		}
		for (j = i + 1; j < sim_data.total_num_spheres; j++) {
			struct sphere_s *s2 = &(sim_data.spheres[j]);
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

double update_grid() {
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
	if (sim_data.num_sectors > 1) { // domain decomposition
		find_event_times_for_all_sectors();
		find_partial_crossing_events_for_all_sectors();
	} else { // no domain decomposition 
		find_event_times_no_dd();
	}
	// Final event may take place after time limit, so cut it short
	if (sim_data.time_limit - sim_data.elapsed_time < event_details.time) {
		event_details.time = sim_data.time_limit - sim_data.elapsed_time;
		update_spheres();
	} else {
		update_spheres();
		apply_event();
	}
	sanity_check();
	return event_details.time;
}
