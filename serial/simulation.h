#pragma once

#include <stdint.h>

#include "sector.h"
#include "sphere.h"

struct simulation_s {
	// Can be indexed using axis enum
	int sector_dims[3];
	union vector_3d grid_size;
	struct sector_s ***sectors;
	// flat array that points to sectors[0][0][0], can be indexed by sector id
	struct sector_s *sectors_flat;
	int num_sectors;
	double time_limit;
	double elapsed_time;
	bool xy_check_needed;
	bool xz_check_needed;
	bool yz_check_needed;
	int64_t total_num_spheres;
	int iteration_number;
	struct sphere_s *spheres;
};

struct simulation_s sim_data;

struct stats_s {
	int num_two_sphere_collisions;
	int num_grid_collisions;
	int num_sector_transfers;
	int num_partial_crossings;
};

struct stats_s stats;

void simulation_init(double time_limit);
void simulation_run();
void simulation_cleanup();
