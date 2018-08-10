#pragma once

#include <stdbool.h>

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

// This is the event data that is sent/received to/from other nodes.
// Instead of using pointers as above it uses sector ids and includes the full
// sphere struct.
// This is because pointers will no longer be valid once sent to another node.
// The local event_details struct will have its data copied here for sending.
struct transmit_event_s {
	double time;
	enum collision_type type;
	struct sphere_s sphere_1;
	struct sphere_s sphere_2;
	enum axis grid_axis;
	int source_sector_id;
	int dest_sector_id;
};

struct event_s event_details; // tracks local next event
struct transmit_event_s event_to_send;
struct transmit_event_s *next_event; // agreed upon by all nodes

struct sector_s *invalid_1;
struct sector_s *invalid_2;
int num_invalid;
bool helping; // is the node helping another node this iteration

void reduce_events();
void reduce_help_events(struct sector_s *sector_to_help);
void init_events();
void reset_event_details();
void reset_event_details_helping();
void set_event_details(
	const double time, const enum collision_type type, struct sphere_s *sphere_1, 
	struct sphere_s *sphere_2, const enum axis grid_axis, struct sector_s *source_sector,
	struct sector_s *dest_sector
);
void apply_event();

