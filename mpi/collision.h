#pragma once

#include <stdbool.h>

#include "sector.h"
#include "sphere.h"

enum collision_type {
	COL_TWO_SPHERES = 0,
	COL_SPHERE_WITH_GRID = 1,
	COL_SPHERE_WITH_SECTOR = 2,
	COL_TWO_SPHERES_PARTIAL_CROSSING = 3,
	COL_NONE = -1
};

void find_event_times_for_sector(const struct sector_s *sector);
