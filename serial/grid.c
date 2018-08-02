#include "simulation.h"
#include "wrapper.h"

// Loads the grid from the initial state file
void init_grid(FILE *initial_state_fp) {
	fread_wrapper(&sim_data.grid_size.x, sizeof(double), 1, initial_state_fp);
	fread_wrapper(&sim_data.grid_size.y, sizeof(double), 1, initial_state_fp);
	fread_wrapper(&sim_data.grid_size.z, sizeof(double), 1, initial_state_fp);
}

