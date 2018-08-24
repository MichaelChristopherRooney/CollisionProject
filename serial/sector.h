#pragma once

#include <stdbool.h>

#include "event.h"
#include "vector_3.h"

enum direction {
	DIR_POSITIVE = 0,
	DIR_NEGATIVE = 1,
	DIR_NONE = 2
};
struct sector_s {
	union vector_3d start;
	union vector_3d end;
	struct sphere_s **spheres;
	int64_t num_spheres;
	int64_t max_spheres;
	union vector_3i pos; // Location in sector array
	double largest_radius; // Radius of the largest sphere in the sector.
	bool largest_radius_shared; // If many spheres have the same radius as the largest radius
	int64_t num_largest_radius_shared; // How many spheres shared the largest radius
	int id;
	bool prior_time_valid; // If last known event time is valid for the next iteration.
};

struct event_s *sector_events;

// Used when iterating over axes and need to access sector adjacent on the current axis.
// Allows sectors to be found in a generic way. 
const int SECTOR_MODIFIERS[3][4][3];

void add_sphere_to_sector(struct sector_s *sector, struct sphere_s *sphere);
void remove_sphere_from_sector(struct sector_s *sector, const struct sphere_s *sphere);
void add_sphere_to_correct_sector(struct sphere_s *sphere);
void init_sectors();
