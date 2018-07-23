#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grid.h"
#include "mpi_vars.h"
#include "params.h"



void simulation_init(double time_limit);
void simulation_run();
void simulation_cleanup();

static void run() {
	simulation_init(10.0);
	/*
	clock_t start = clock();
	simulation_run();
	clock_t end = clock();
	float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	printf("Time taken in seconds: %f\n", seconds);
	printf("Number of sphere on sphere collisions: %d\n", grid->num_two_sphere_collisions);
	printf("Number of collisions with grid boundary: %d\n", grid->num_grid_collisions);
	printf("Number of transfers between sectors (if used): %d\n", grid->num_sector_transfers);
	simulation_cleanup();
	*/
}

static void init(int argc, char *argv[]){
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &RANK);
	MPI_Comm_size(MPI_COMM_WORLD, &NUM_NODES);
	parse_args(argc, argv);
	MPI_Cart_create(MPI_COMM_WORLD, NUM_DIMS, SECTOR_DIMS, PERIODS, REORDER, &GRID_COMM);
	MPI_Cart_coords(GRID_COMM, RANK, NUM_DIMS, COORDS);
}

int main(int argc, char *argv[]) {
	init(argc, argv);
	//printf("Rank %d is at %d, %d, %d\n", RANK, COORDS[0], COORDS[1], COORDS[2]);
	run();
	MPI_Comm_free(&GRID_COMM);
	MPI_Finalize();	
	return 0;
}

