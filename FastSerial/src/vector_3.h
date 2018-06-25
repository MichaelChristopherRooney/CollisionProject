#pragma once

enum coord {
	X_COORD = 0,
	Y_COORD = 1,
	Z_COORD = 2,
};

// Can either access x/y/z directly, or access coords[X/Y/Z_COORD]
union vector_3_s {
	struct {
		double x;
		double y;
		double z;
	};
	double coords[3];
};

double get_vector_3_magnitude(const union vector_3_s *v);
double get_vector_3_dot_product(const union vector_3_s *v1, const union vector_3_s *v2);
double get_shortest_angle_between_vector_3(const double dp, const double mag1, const double mag2);
void normalise_vector_3(union vector_3_s *v);