#include <stdlib.h>

#include "simulation.h"
#include "sphere.h"
#include "wrapper.h"

// Updates the spheres position.
// 't' is the time since the last position update, not the time since the start
// of the simulation. 
// Note: this is not intended to apply any collision effects, that happens
// elsewhere.
void update_sphere_position(struct sphere_s *s, double t) {
	s->pos.x = s->pos.x + (s->vel.x * t);
	s->pos.y = s->pos.y + (s->vel.y * t);
	s->pos.z = s->pos.z + (s->vel.z * t);
}

// Loads spheres from the specified inital state file
void load_spheres(FILE *initial_state_fp) {
	fread_wrapper(&sim_data.total_num_spheres, sizeof(int64_t), 1, initial_state_fp);
	sim_data.spheres = calloc(sim_data.total_num_spheres, sizeof(struct sphere_s));
	int64_t i;
	for(i = 0; i < sim_data.total_num_spheres; i++){
		fread_wrapper(&sim_data.spheres[i].id, sizeof(int64_t), 1, initial_state_fp);
		fread_wrapper(&sim_data.spheres[i].pos.x, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&sim_data.spheres[i].pos.y, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&sim_data.spheres[i].pos.z, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&sim_data.spheres[i].vel.x, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&sim_data.spheres[i].vel.y, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&sim_data.spheres[i].vel.z, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&sim_data.spheres[i].mass, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&sim_data.spheres[i].radius, sizeof(double), 1, initial_state_fp);
		if (sim_data.num_sectors > 1) {
			add_sphere_to_correct_sector(&sim_data.spheres[i]);
		}
	}
}
