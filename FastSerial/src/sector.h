#pragma once

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
};

#define NUM_SECTORS_X 2
#define NUM_SECTORS_Y 2
#define NUM_SECTORS_Z 1

void add_sphere_to_sector(struct sector_s *sector, struct sphere_s *sphere);