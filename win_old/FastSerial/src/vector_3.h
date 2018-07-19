#pragma once

#include <stdint.h>

enum axis {
	X_AXIS = 0,
	Y_AXIS = 1,
	Z_AXIS = 2,
	AXIS_NONE = -1
};

// Can either access x/y/z directly, or access vals[X/Y/Z_AXIS]
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