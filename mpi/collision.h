#pragma once

#include <stdbool.h>

#include "sector.h"
#include "sphere.h"

enum collision_type {
	COL_TWO_SPHERES = 0,
	COL_SPHERE_WITH_GRID = 1,
	COL_SPHERE_WITH_SECTOR = 2,
	COL_NONE = -1
};

double find_collision_time_spheres(const struct sphere_s *s1, const struct sphere_s *s2);
double find_collision_time_grid(const struct sphere_s *s, enum axis *col_axis);
double find_collision_time_sector(const struct sector_s *sector, const struct sphere_s *sphere, struct sector_s **dest);
void apply_bounce_between_spheres(struct sphere_s *s1, struct sphere_s *s2);
