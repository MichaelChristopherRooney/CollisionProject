#include <stdio.h>

#include "grid.h"

void simulation_init(char *fp, union vector_3i *divs, union vector_3d *grid_start, union vector_3d *grid_end);
void simulation_run();
void simulation_cleanup();

int main(void) {
	union vector_3i divs = { .x = 1, .y = 1, .z = 1 };
	union vector_3d grid_start = { .x = 0.0, .y = 0.0, .z = 0.0 };
	union vector_3d grid_end = { .x = 1000.0, .y = 1000.0, .z = 1000.0 };
	simulation_init("dd_data.bin", &divs, &grid_start, &grid_end);
	simulation_run();
	FILE *out = fopen("out.txt", "w");
	int i;
	for (i = 0; i < NUM_SPHERES; i++) {
		fprintf(out, "%f, %f, %f\n", spheres[i].pos.x, spheres[i].pos.y, spheres[i].pos.z);
	}
	fclose(out);
	simulation_cleanup();
	printf("Press enter to exit...\n");
	getchar();
	return 0;
}
