#include "grid.h"
#include "sector.h"
#include "vector_3.h"

void simulation_init(int argc, char *argv[], double time_limit);
void simulation_run();
void simulation_cleanup();

void write_sphere_initial_state(const struct sphere_s *sphere);
void write_initial_iteration_stats();

struct simulation_s {
	// Can be indexed using axis enum
	int sector_dims[3];
	union vector_3d grid_size;
	struct sector_s ***sectors;
	// flat array that points to sectors[0][0][0], can be indexed by sector id or node rank
	struct sector_s *sectors_flat;
	int num_sectors;
	double time_limit;
	double elapsed_time;
	bool xy_check_needed;
	bool xz_check_needed;
	bool yz_check_needed;
	int num_two_sphere_collisions;
	int num_grid_collisions;
	int num_sector_transfers;
	int64_t total_num_spheres;
	int iteration_number;

};

struct simulation_s sim_data;
