#pragma once

#include "event_type.h"
#include "vector_3.h"

struct event_s {
	double time; // When the event happens
	enum event_type type; // What the next event is.
	struct sphere_s *sphere_1; // Sphere that hits the grid, transfers to another sector, or is the first sphere in a sphere on sphere collision.
	struct sphere_s *sphere_2; // Second sphere in a sphere on sphere collision, otherwise NULL
	enum axis grid_axis; // If the event is a sphere bouncing off the grid record the axis
	// If a sphere moves between sectors these record the details.
	struct sector_s *source_sector;
	struct sector_s *dest_sector;
};

struct event_s event_details;

void apply_event();
void reset_event();
void reset_sector_event(struct sector_s *s);
void find_soonest_event_from_sectors();
void set_event_details(
	const double time, const enum event_type type, struct sphere_s *sphere_1, 
	struct sphere_s *sphere_2, const enum axis grid_axis, struct sector_s *source_sector,
	struct sector_s *dest_sector
);

