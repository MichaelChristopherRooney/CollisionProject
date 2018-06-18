#pragma once

#include "sphere.h"

enum AXIS {
	X_AXIS = 0,
	Y_AXIS = 1,
	Z_AXIS = 2,
	AXIS_NONE = -1
};

// This is the area in which the collisions take place. 
// Imagine the grid is a 3D box with all collisions taking place inside it.
// Normally x_start, y_start and z_start will all be 0, but the code is 
// built to work with other positive values.
// Note that all values here MUST be positive. 
struct grid_s {
	double x_start;
	double y_start;
	double z_start;
	double x_end;
	double y_end;
	double z_end;
};

struct grid_s *grid; // The grid used by the simulation.

// All spheres in the grid.
// Hardcoded for testing
//#define NUM_SPHERES 5
int NUM_SPHERES;
struct sphere_s *spheres;

void init_grid();
double update_grid();