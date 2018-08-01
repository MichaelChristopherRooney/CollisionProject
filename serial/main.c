#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grid.h"
#include "params.h"
#include "sector.h"
#include "vector_3.h"

void simulation_init(double time_limit);
void simulation_run();
void simulation_cleanup();

static void run() {
	simulation_init(10.0);
	clock_t start = clock();
	simulation_run();
	clock_t end = clock();
	float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	printf("Time taken in seconds: %f\n", seconds);
	printf("Number of sphere on sphere collisions: %d\n", grid->num_two_sphere_collisions);
	printf("Number of collisions with grid boundary: %d\n", grid->num_grid_collisions);
	printf("Number of transfers between sectors (if used): %d\n", grid->num_sector_transfers);
	simulation_cleanup();
}

int main(int argc, char *argv[]) {
	parse_args(argc, argv);
	run();
	return 0;
}
