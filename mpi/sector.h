#pragma once

#include <stdbool.h>

#include "sphere.h"
#include "vector_3.h"

enum direction {
	DIR_POSITIVE = 0,
	DIR_NEGATIVE = 1
};

#define SECTOR_DEFAULT_MAX_SPHERES 2000

#define MAX_HOSTNAME_LENGTH 256

// Format is "xxx-size.bin" or "xxx-spheres.bin" where xxx is sector id
// Using 20 gives more space than will ever be needed.
#define SECTOR_MAX_FILENAME_LENGTH 20

struct sector_s {
	union vector_3d start;
	union vector_3d end;
	struct sphere_s *spheres;
	int64_t num_spheres;
	int64_t *num_spheres_ptr;
	int64_t max_spheres;
	// Location in sector array
	union vector_3i pos;
	double largest_radius; // Radius of the largest sphere in the sector.
	bool largest_radius_shared; // If many spheres have the same radius as the largest radius
	int64_t num_largest_radius_shared; // How many spheres shared the largest radius
	bool is_neighbour; // used by the local node to track neighbours
	// If sectors are neighbours and the processes responsible for them are
	// also on the same machine then use file backed shared memory to store spheres.
	// This allows processes to skip updating spheres for neighbours on the same machine.
	// However the memory for spheres may be resized so this needs to be communicated.
	// To get around this the number of spheres is also stored in file 
	// backed memory so other processes know when to resize.
	char size_filename[SECTOR_MAX_FILENAME_LENGTH];
	char spheres_filename[SECTOR_MAX_FILENAME_LENGTH];
	int size_fd;
	int spheres_fd;
	bool is_local_neighbour; // used by local process to track if other processes are logical and physical neighbours
	int id;
	char hostname[MAX_HOSTNAME_LENGTH];
};

// Used when iterating over axes and nned to access sector adjacent on the current axis.
// Allows sectors to be found in a generic way. 
const int SECTOR_MODIFIERS[2][3][3];

bool does_sphere_belong_to_sector(const struct sphere_s *sphere, const struct sector_s *sector);
struct sector_s *find_sector_that_sphere_belongs_to(struct sphere_s *sphere);
void add_sphere_to_sector(struct sector_s *sector, const struct sphere_s *sphere);
void remove_sphere_from_sector(struct sector_s *sector, const struct sphere_s *sphere);
struct sector_s *get_adjacent_sector_non_diagonal(const struct sector_s *sector, const enum axis a, const enum direction dir);
void add_sphere_to_sector(struct sector_s *sector, const struct sphere_s *sphere);
void init_sectors();
