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
	double time_limit;
	double elapsed_time;
	struct sector_s ***sectors;
	bool uses_sectors;
	bool xy_check_needed;
	bool xz_check_needed;
	bool yz_check_needed;
	int num_two_sphere_collisions;
	int num_grid_collisions;
	int num_sector_transfers;
};

struct grid_s *grid; // The grid used by the simulation.

struct event_s {
	double time; // When the event happens
	enum collision_type type; // What the next event is.
	struct sphere_s *sphere_1; // Sphere that hits the grid, transfers to another sector, or is the first sphere in a sphere on sphere collision.
	struct sphere_s *sphere_2; // Second sphere in a sphere on sphere collision, otherwise NULL
	enum axis grid_axis; // If the event is a sphere bouncing off the grid record the axis
	// If a sphere moves between sectors these record the details.
	struct sector_s *source_sector;
	struct sector_s *dest_sector;
};

struct event_s event_details;

// All spheres in the grid.
// Hardcoded for testing
int64_t NUM_SPHERES;
struct sphere_s *spheres;

void init_grid(double time_limit);
double update_grid();
