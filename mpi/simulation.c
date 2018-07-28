#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "event.h"
#include "grid.h"
#include "io.h"
#include "mpi_vars.h"
#include "params.h"
#include "simulation.h"
#include "vector_3.h"


/*
Need to rewrite this for MPI version
static void compare_results() {
	if(compare_file == NULL){
		return;
	}
	FILE *fp = fopen(compare_file, "rb");
	int64_t comp_num_spheres;
	// result is just to shut up gcc's warnings
	int result = fread(&comp_num_spheres, sizeof(int64_t), 1, fp);
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
		result = fread(&vel_comp, sizeof(union vector_3d), 1, fp);
		result = fread(&pos_comp, sizeof(union vector_3d), 1, fp);
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
*/
void simulation_init(int argc, char *argv[], double time_limit) {
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &WORLD_RANK);
	MPI_Comm_size(MPI_COMM_WORLD, &NUM_NODES);
	parse_args(argc, argv);
	MPI_Cart_create(MPI_COMM_WORLD, NUM_DIMS, SECTOR_DIMS, PERIODS, REORDER, &GRID_COMM);
	MPI_Comm_rank(GRID_COMM, &GRID_RANK);
	MPI_Cart_coords(GRID_COMM, GRID_RANK, NUM_DIMS, COORDS);
	ITERATION_NUMBER = 0;
	init_output_file();
	init_grid(time_limit);
}

void simulation_run() {
	ITERATION_NUMBER = 1; // start at 1 as 0 is iteration num for the initial state
	while (grid->elapsed_time < grid->time_limit) {	
		grid->elapsed_time += update_grid();
		MPI_Barrier(GRID_COMM);
		//save_sphere_state_to_file(i, grid->elapsed_time);
		ITERATION_NUMBER++;
	}
	//write_final_state();
	//compare_results();
}

void simulation_cleanup() {
	free(SECTOR->spheres);
	int i;
	for(i = 0; i < grid->num_sectors; i++){
		struct sector_s *s = &grid->sectors_flat[i];
		if(s->is_neighbour){
			free(s->spheres);
		}
	}
	free(grid->sectors[0][0]);
	for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
		free(grid->sectors[i]);
	}
	free(grid->sectors);
	free(grid);
	MPI_File_close(&MPI_OUTPUT_FILE);
	MPI_Comm_free(&GRID_COMM);
}

