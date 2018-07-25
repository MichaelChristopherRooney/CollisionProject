#pragma once

#include "collision.h"
#include "sphere.h"

// This is the event data that is used locally
// Pointers to spheres and sectors are used so that the correct object in 
// memory can be modified.
struct event_s {
	double time; // When the event happens
	enum collision_type type; // What the next event is.
	struct sphere_s *sphere_1; // Sphere that hits the grid, transfers to another sector, or is the first sphere in a sphere on sphere collision.
	struct sphere_s *sphere_2; // Second sphere in a sphere on sphere collision, otherwise NULL
	enum axis grid_axis; // If the event is a sphere bouncing off the grid record the axis
	// If a sphere moves between sectors these record the details.
	struct sector_s *source_sector;
	struct sector_s *dest_sector;
};

struct event_s event_details; // tracks local next event

double global_soonest_time; // soonest time as agreed on by all nodes

void reduce_events();
void init_events();
void reset_event_details();
void set_event_details(
	const double time, const enum collision_type type, const struct sphere_s *sphere_1, 
	const struct sphere_s *sphere_2, const enum axis grid_axis, const struct sector_s *source_sector,
	const struct sector_s *dest_sector
);

