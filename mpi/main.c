#include <mpi.h>
#include <stdio.h>
#include <time.h>

#include "mpi_vars.h"
#include "simulation.h"

int main(int argc, char *argv[]) {
	clock_t start = clock();
	simulation_init(argc, argv, 10.0);
	simulation_run();
	simulation_cleanup();
	clock_t end = clock();
	float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	if(WORLD_RANK == 0){ // grid has been destroyed by now, use world rank
		printf("Time taken in seconds: %f\n", seconds);
	}
	MPI_Finalize();	
	return 0;
}

