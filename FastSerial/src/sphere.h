#pragma once

#include <stdint.h>

#include "vector_3.h"

// Velocity is given as metres per second
// Position is given in metres and is the center of the sphere.
// For now assuming all spheres have the same radius
struct sphere_s {
	union vector_3d vel;
	union vector_3d pos;
	double radius;
	double mass;
};

struct sphere_list_s {
	struct sphere_s *sphere;
	struct sphere_list_s *prev;
	struct sphere_list_s *next;
};

void update_sphere_position(struct sphere_s *s, double t);