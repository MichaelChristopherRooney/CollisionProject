void simulation_init(int argc, char *argv[], double time_limit);
void simulation_run();
void simulation_cleanup();

void write_sphere_initial_state(const struct sphere_s *sphere);
void write_initial_iteration_stats();

int ITERATION_NUMBER;
