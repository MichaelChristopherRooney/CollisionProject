#pragma once

#include <stdbool.h>

#include "sphere.h"
#include "vector_3.h"

enum direction {
	DIR_POSITIVE = 0,
	DIR_NEGATIVE = 1
};

struct sector_s {
	union vector_3d start;
	union vector_3d end;
	struct sphere_s *spheres;
	int64_t num_spheres;
	int64_t max_spheres;
	// Location in sector array
	union vector_3i pos;
	double largest_radius; // Radius of the largest sphere in the sector.
	bool largest_radius_shared; // If many spheres have the same radius as the largest radius
	int64_t num_largest_radius_shared; // How many spheres shared the largest radius
	bool is_neighbour; // used by the local node to track neighbours
	int id;
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
