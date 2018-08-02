#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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

void simulation_init(double time_limit) {
	init_grid(time_limit);
	delete_old_files();
	init_binary_file();
}

void simulation_run() {
	int i = 1; // start at 1 as 0 is iteration num for the initial state
	while (sim_data.elapsed_time < sim_data.time_limit) {
		sim_data.elapsed_time += update_grid();
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
		free(sim_data.sectors[0][0]);
		int i;
		for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
			free(sim_data.sectors[i]);
		}
		free(sim_data.sectors);
	}
	free(sim_data.spheres);
	close_data_file();
}
