#pragma once

#include <stdbool.h>

#include "sphere.h"
#include "vector_3.h"

struct sector_s {
	union vector_3d start;
	union vector_3d end;
	struct sphere_list_s *head;
	int num_spheres;
	// Location in sector array
	union vector_3i pos;
	double largest_radius; // Radius of the largest sphere in the sector.
	bool largest_radius_shared; // If many spheres have the same radius as the largest radius
	int num_largest_radius_shared; // How many spheres shared the largest radius
};

#define NUM_SECTORS_X 2
#define NUM_SECTORS_Y 2
#define NUM_SECTORS_Z 1

// Can be indexed using coord enum
const int SECTOR_DIMS[3];

// Used when iterating over axes and nned to access sector adjacent on the current axis.
const int SECTOR_MODIFIERS[3][3];

void add_sphere_to_sector(struct sector_s *sector, const struct sphere_s *sphere);
void remove_sphere_from_sector(struct sector_s *sector, const struct sphere_s *sphere);
struct sector_s *get_sector_in_negative_direction(const struct sector_s *sector, const enum coord c);
struct sector_s *get_sector_in_positive_direction(const struct sector_s *sector, const enum coord c);
