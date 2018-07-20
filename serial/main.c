#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grid.h"
#include "params.h"
#include "sector.h"
#include "vector_3.h"

void simulation_init(char *fp, union vector_3d *grid_size, double time_limit);
void simulation_run();
void simulation_cleanup();

static void run(char *fp, union vector_3d *grid_size) {
	simulation_init(fp, grid_size, 10.0);
	clock_t start = clock();
	simulation_run();
	clock_t end = clock();
	float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	printf("Time taken in seconds: %f\n", seconds);
#ifdef RECORD_STATS
	printf("Number of sphere on sphere collisions: %d\n", grid->num_two_sphere_collisions);
	printf("Number of collisions with grid boundary: %d\n", grid->num_grid_collisions);
	printf("Number of transfers between sectors (if used): %d\n", grid->num_sector_transfers);
#endif
	simulation_cleanup();
}

/*
static void compare_results(struct sphere_s *results_1, struct sphere_s *results_2) {
	double max_pos_err = 0.0;
	double max_vel_err = 0.0;
	int i;
	for (i = 0; i < NUM_SPHERES; i++) {
		struct sphere_s s1 = results_1[i];
		struct sphere_s s2 = results_2[i];
		enum axis a;
		for (a = X_AXIS; a <= Z_AXIS; a++) {
			if (fabs(s1.pos.vals[a] - s2.pos.vals[a]) > max_pos_err) {
				max_pos_err = fabs(s1.pos.vals[a] - s2.pos.vals[a]);
			}
			if (fabs(s1.vel.vals[a] - s2.vel.vals[a]) > max_vel_err) {
				max_vel_err = fabs(s1.vel.vals[a] - s2.vel.vals[a]);
			}
		}
	}
	printf("vel abs err: %.17g\n", max_vel_err);
	printf("pos abs err: %.17g\n", max_pos_err);
}
*/

int main(int argc, char *argv[]) {
	parse_args(argc, argv);
	union vector_3d grid_end = { .x = 1000.0,.y = 1000.0, .z = 1000.0 };
	run("dd.bin", &grid_end);
	printf("Press enter to exit...\n");
	getchar();
	return 0;
}
