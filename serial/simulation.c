#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "grid.h"
#include "io.h"
#include "params.h"
#include "wrapper.h"
#include "vector_3.h"

static void compare_results() {
	if(compare_file == NULL){
		return;
	}
	FILE *fp = fopen(compare_file, "rb");
	int64_t comp_num_spheres;
	 fread_wrapper(&comp_num_spheres, sizeof(int64_t), 1, fp);
	if(comp_num_spheres != NUM_SPHERES){
		printf("Error: cannot compare files as number of spheres differs\n");
		return;
	}
	double max_pos_err = 0.0;
	double max_vel_err = 0.0;
	int i;
	for (i = 0; i < NUM_SPHERES; i++) {
		struct sphere_s s = spheres[i];
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
	while (grid->elapsed_time < grid->time_limit) {
		grid->elapsed_time += update_grid();
		if (grid->elapsed_time >= grid->time_limit) {
			write_final_time_to_file();
			break;
		}
		save_sphere_state_to_file(i, grid->elapsed_time);
		i++;
	}
	write_final_state();
	compare_results();
}

void simulation_cleanup() {
	if (grid->uses_sectors) {
		free(grid->sectors[0][0]);
		int i;
		for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
			free(grid->sectors[i]);
		}
		free(grid->sectors);
	}
	free(spheres);
	close_data_file();
	free(grid);
}
