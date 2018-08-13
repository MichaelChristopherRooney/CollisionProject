#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "params.h"
#include "simulation.h"

static void run() {
	simulation_init();
	clock_t start = clock();
	simulation_run();
	clock_t end = clock();
	float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	printf("Time taken in seconds: %f\n", seconds);
	printf("Number of sphere on sphere collisions: %d\n", stats.num_two_sphere_collisions);
	printf("Number of collisions with grid boundary: %d\n", stats.num_grid_collisions);
	if(sim_data.num_sectors > 1){
		printf("Number of transfers between sectors: %d\n", stats.num_sector_transfers);
		printf("Number of partial crossings: %d\n", stats.num_partial_crossings);
	}
	simulation_cleanup();
}

int main(int argc, char *argv[]) {
	parse_args(argc, argv);
	run();
	return 0;
}
