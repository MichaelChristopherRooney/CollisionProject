#include <stdio.h>

#include "grid.h"

void simulation_init();
void simulation_run();
void simulation_cleanup();

int main(void) {
	simulation_init();
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
