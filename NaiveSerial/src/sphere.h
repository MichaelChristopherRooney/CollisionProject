#pragma once

#include <stdint.h>

#include "vector_3.h"

// Velocity is given as metres per second
// Position is given in metres and is the center of the sphere.
// For now assuming all spheres have the same radius
struct sphere_s {
	struct vector_3_s vel;
	struct vector_3_s pos;
	double radius;
	double mass;
};

void update_sphere_position(struct sphere_s *s, double t);