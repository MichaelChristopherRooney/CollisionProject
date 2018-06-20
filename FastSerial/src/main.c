#include <stdio.h>

void simulation_init();
void simulation_run();
void simulation_cleanup();

int main(void) {
	simulation_init();
	simulation_run();
	simulation_cleanup();
	printf("Press enter to exit...\n");
	getchar();
	return 0;
}
