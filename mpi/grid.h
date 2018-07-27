#pragma once

#include "collision.h"
#include "sector.h"
#include "sphere.h"
#include "vector_3.h"

// This is the area in which the collisions take place. 
// Imagine the grid is a 3D box with all collisions taking place inside it.
// Normally x_start, y_start and z_start will all be 0, but the code is 
// built to work with other positive values.
// Note that all values here MUST be positive. 
struct grid_s {
	union vector_3d size;
	struct sector_s ***sectors;
	// flat array that points to sectors[0][0][0], can be indexed by sector id or node rank
	struct sector_s *sectors_flat;
	double time_limit;
	double elapsed_time;
	bool xy_check_needed;
	bool xz_check_needed;
	bool yz_check_needed;
	int num_two_sphere_collisions;
	int num_grid_collisions;
	int num_sector_transfers;
};

struct grid_s *grid; // The grid used by the simulation.

// All spheres in the grid.
// Hardcoded for testing
int64_t NUM_SPHERES;
struct sphere_s *spheres;

void init_grid(double time_limit);
double update_grid(int i);
