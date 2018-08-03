#pragma once

#include <stdint.h>

void init_binary_file();
void write_final_state();
void delete_old_files();
void write_final_time_to_file();
void close_data_file();
void save_sphere_state_to_file(uint64_t iteration_num, double time_elapsed);
