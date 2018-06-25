#pragma once

#include <stdbool.h>

#include "sphere.h"

struct sector_s {
	double x_start;
	double y_start;
	double z_start;
	double x_end;
	double y_end;
	double z_end;
	struct sphere_list_s *head;
	int num_spheres;
	// Location in sector array
	int x;
	int y;
	int z;
	double largest_radius; // Radius of the largest sphere in the sector.
	bool largest_radius_shared; // If many spheres have the same radius as the largest radius
	int num_largest_radius_shared; // How many spheres shared the largest radius
};

#define NUM_SECTORS_X 2
#define NUM_SECTORS_Y 2
#define NUM_SECTORS_Z 1

void add_sphere_to_sector(struct sector_s *sector, const struct sphere_s *sphere);
void remove_sphere_from_sector(struct sector_s *sector, const struct sphere_s *sphere);