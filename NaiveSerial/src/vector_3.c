#include <math.h>

#include "vector_3.h"

double get_vector_3d_magnitude(const union vector_3d *v) {
	return sqrt((v->x*v->x) + (v->y*v->y) + (v->z*v->z));
}

// Given two sets of xyz values returns the dot product
double get_vector_3d_dot_product(const union vector_3d *v1, const union vector_3d *v2) {
	return (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z);
}

// Given the already computed dot product and magnitudes returns the angle between two xyz vectors.
double get_shortest_angle_between_vector_3d(const double dp, const double mag1, const double mag2) {
	double cos_inv = dp / (mag1 * mag2);
	return acos(cos_inv);
}

void normalise_vector_3d(union vector_3d *v) {
	double mag = get_vector_3d_magnitude(v);
	v->x = v->x / mag;
	v->y = v->y / mag;
	v->z = v->z / mag;
}