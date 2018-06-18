#include <stdio.h>

void run_tests();

void simulation_init();
void simulation_run();
void simulation_cleanup();

int main(void) {
	simulation_init();
	simulation_run();
	simulation_cleanup();
	//run_tests();
	printf("Press enter to exit...\n");
	getchar();
	return 0;
}
