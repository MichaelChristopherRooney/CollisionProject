#pragma once

struct vector_3_s {
	double x;
	double y;
	double z;
};

double get_vector_3_magnitude(const struct vector_3_s *v);
double get_vector_3_dot_product(const struct vector_3_s *v1, const struct vector_3_s *v2);
double get_shortest_angle_between_vector_3(const double dp, const double mag1, const double mag2);
void normalise_vector_3(struct vector_3_s *v);