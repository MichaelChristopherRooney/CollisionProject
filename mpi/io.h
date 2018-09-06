#pragma once

#include "params.h"
#include "sphere.h"

void write_sphere_initial_state(const struct sphere_s *sphere);
void write_initial_iteration_stats();
void write_num_spheres();
void write_grid_dimms();
void write_iteration_data(struct sphere_s *s1, struct sphere_s *s2);
void seek_one_sphere();
void seek_two_spheres();
void init_output_file();
void save_final_state_file();
void compare_results();
