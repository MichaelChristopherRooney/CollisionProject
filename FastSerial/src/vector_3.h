#pragma once

#include <stdint.h>

enum coord {
	X_COORD = 0,
	Y_COORD = 1,
	Z_COORD = 2,
};

// Can either access x/y/z directly, or access coords[X/Y/Z_COORD]
union vector_3d {
	struct {
		double x;
		double y;
		double z;
	};
	double vals[3];
};

union vector_3i {
	struct {
		uint32_t x;
		uint32_t y;
		uint32_t z;
	};
	uint32_t vals[3];
};


double get_vector_3d_magnitude(const union vector_3d *v);
double get_vector_3d_dot_product(const union vector_3d *v1, const union vector_3d *v2);
double get_shortest_angle_between_vector_3d(const double dp, const double mag1, const double mag2);
void normalise_vector_3d(union vector_3d *v);