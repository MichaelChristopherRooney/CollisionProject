#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "collision.h"
#include "event.h"
#include "grid.h"
#include "io.h"
#include "params.h"
#include "simulation.h"
#include "wrapper.h"
#include "vector_3.h"

static void compare_results() {
	if(compare_file == NULL){
		return;
	}
	FILE *fp = fopen(compare_file, "rb");
	int64_t comp_num_spheres;
	fread_wrapper(&comp_num_spheres, sizeof(int64_t), 1, fp);
	if(comp_num_spheres != sim_data.total_num_spheres){
		printf("Error: cannot compare files as number of spheres differs\n");
		return;
	}
	double max_pos_err = 0.0;
	double max_vel_err = 0.0;
	int i;
	for (i = 0; i < sim_data.total_num_spheres; i++) {
		struct sphere_s s = sim_data.spheres[i];
		union vector_3d vel_comp;
		union vector_3d pos_comp;
		fread_wrapper(&vel_comp, sizeof(union vector_3d), 1, fp);
		fread_wrapper(&pos_comp, sizeof(union vector_3d), 1, fp);
		enum axis a;
		for (a = X_AXIS; a <= Z_AXIS; a++) {
			if (fabs(s.pos.vals[a] - pos_comp.vals[a]) > max_pos_err) {
				max_pos_err = fabs(s.pos.vals[a] - pos_comp.vals[a]);
			}
			if (fabs(s.vel.vals[a] - vel_comp.vals[a]) > max_vel_err) {
				max_vel_err = fabs(s.vel.vals[a] - vel_comp.vals[a]);
			}
		}
	}
	printf("vel abs err: %.17g\n", max_vel_err);
	printf("pos abs err: %.17g\n", max_pos_err);
}

static void init_stats(){
	stats.num_two_sphere_collisions = 0;
	stats.num_grid_collisions = 0;
	stats.num_sector_transfers = 0;
	stats.num_partial_crossings = 0;
}

void simulation_init(double time_limit) {
	FILE *initial_state_fp = fopen(initial_state_file, "rb");
	sim_data.elapsed_time = 0.0;
	sim_data.time_limit = time_limit;
	init_grid(initial_state_fp);
	init_stats();
	init_sectors();
	load_spheres(initial_state_fp);
	delete_old_files();
	init_binary_file();
}

// For debugging
// Ensures each sphere is located within the sector responsible for it.
// Helps catch any issues with transfering spheres between sectors.
static void sanity_check() {
	if (sim_data.num_sectors == 1) {
		return;
	}
	int i;
	for(i = 0; i < sim_data.num_sectors; i++){
		struct sector_s *s = &sim_data.sectors_flat[i];
		int j;
		for (j = 0; j < s->num_spheres; j++) {
			struct sphere_s *sphere = s->spheres[j];
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
				exit(1);
			}
		}
	}
}

static void do_simulation_iteration(){
	sanity_check();
	// First reset records.
	reset_event();
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
	sim_data.elapsed_time += event_details.time;
}

void simulation_run() {
	int i = 1; // start at 1 as 0 is iteration num for the initial state
	while (sim_data.elapsed_time < sim_data.time_limit) {
		do_simulation_iteration();
		if (sim_data.elapsed_time >= sim_data.time_limit) {
			write_final_time_to_file();
			break;
		}
		save_sphere_state_to_file(i, sim_data.elapsed_time);
		i++;
	}
	write_final_state();
	compare_results();
}

void simulation_cleanup() {
	if (sim_data.num_sectors > 1) {
		free(sim_data.sectors_flat);
		int i;
		for (i = 0; i < sim_data.sector_dims[X_AXIS]; i++) {
			free(sim_data.sectors[i]);
		}
		free(sim_data.sectors);
	}
	free(sim_data.spheres);
	close_data_file();
}
