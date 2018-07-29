#include "io.h"
#include "mpi_vars.h"
#include "sector.h"
#include "simulation.h"
#include "sphere.h"

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
// This file contains every sphere, and we need to check if the sphere belongs
// to the sector the current MPI node is responsible for.
void load_spheres(FILE *initial_state_fp) {
	// result is just to shut up gcc's warnings
	// TODO: get rid of this global and use grid->total_spheres where needed
	int result = fread(&sim_data.total_num_spheres, sizeof(int64_t), 1, initial_state_fp);
	write_num_spheres();
	struct sphere_s in;
	int64_t i;
	for(i = 0; i < sim_data.total_num_spheres; i++){
		result = fread(&in.id, sizeof(int64_t), 1, initial_state_fp);
		result = fread(&in.pos.x, sizeof(double), 1, initial_state_fp);
		result = fread(&in.pos.y, sizeof(double), 1, initial_state_fp);
		result = fread(&in.pos.z, sizeof(double), 1, initial_state_fp);
		result = fread(&in.vel.x, sizeof(double), 1, initial_state_fp);
		result = fread(&in.vel.y, sizeof(double), 1, initial_state_fp);
		result = fread(&in.vel.z, sizeof(double), 1, initial_state_fp);
		result = fread(&in.mass, sizeof(double), 1, initial_state_fp);
		result = fread(&in.radius, sizeof(double), 1, initial_state_fp);
		write_sphere_initial_state(&in);
		struct sector_s *temp = find_sector_that_sphere_belongs_to(&in);
		if(temp == SECTOR || temp->is_neighbour){
			// want to copy it if it belongs to local node or a neighbour
			add_sphere_to_sector(temp, &in);
		}
	}
	write_initial_iteration_stats();
}

// For details on how this works see:
// https://www.gamasutra.com/view/feature/131424/pool_hall_lessons_fast_accurate_.php?page=3
void apply_bounce_between_spheres(struct sphere_s *s1, struct sphere_s *s2) {
	union vector_3d rel_pos = { 
		.x = s1->pos.x - s2->pos.x, 
		.y = s1->pos.y - s2->pos.y, 
		.z = s1->pos.z - s2->pos.z 
	};
	normalise_vector_3d(&rel_pos);
	double dp1 = get_vector_3d_dot_product(&rel_pos, &s1->vel);
	double dp2 = get_vector_3d_dot_product(&rel_pos, &s2->vel);
	double p = (2.0 * (dp1 - dp2)) / (s1->mass + s2->mass);
	// Sphere one first
	s1->vel.x = s1->vel.x - (p * s2->mass * rel_pos.x);
	s1->vel.y = s1->vel.y - (p * s2->mass * rel_pos.y);
	s1->vel.z = s1->vel.z - (p * s2->mass * rel_pos.z);
	// Now sphere two
	s2->vel.x = s2->vel.x + (p * s1->mass * rel_pos.x);
	s2->vel.y = s2->vel.y + (p * s1->mass * rel_pos.y);
	s2->vel.z = s2->vel.z + (p * s1->mass * rel_pos.z);
}
