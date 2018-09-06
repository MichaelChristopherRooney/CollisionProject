#include "io.h"
#include "mpi_vars.h"
#include "sector.h"
#include "simulation.h"
#include "sphere.h"
#include "wrapper.h"

// Updates the spheres position.
// 't' is the time since the last position update, not the time since the start
// of the simulation. 
// Note: this is not intended to apply any collision effects, that happens
// elsewhere.
void update_sphere_position(struct sphere_s *s, const double t) {
	s->pos.x = s->pos.x + (s->vel.x * t);
	s->pos.y = s->pos.y + (s->vel.y * t);
	s->pos.z = s->pos.z + (s->vel.z * t);
}

// Loads spheres from the specified inital state file
// This file contains every sphere, and we need to check if the sphere belongs
// to the sector the current MPI node is responsible for.
void load_spheres(FILE *initial_state_fp) {
	fread_wrapper(&sim_data.total_num_spheres, sizeof(int64_t), 1, initial_state_fp);
	write_num_spheres();
	struct sphere_s in;
	int64_t i;
	for(i = 0; i < sim_data.total_num_spheres; i++){
		fread_wrapper(&in.id, sizeof(int64_t), 1, initial_state_fp);
		fread_wrapper(&in.pos.x, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&in.pos.y, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&in.pos.z, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&in.vel.x, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&in.vel.y, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&in.vel.z, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&in.mass, sizeof(double), 1, initial_state_fp);
		fread_wrapper(&in.radius, sizeof(double), 1, initial_state_fp);
		write_sphere_initial_state(&in);
		struct sector_s *temp = find_sector_that_sphere_belongs_to(&in);
		if(temp->is_local_neighbour){
			// sphere array will be resized later if needed
			// for now make sure the largest radius is tracked though
			temp->num_spheres++; // will check for resizing later
			set_largest_radius_after_insertion(temp, &in);
		} else if(ALL_HELP || temp == SECTOR || temp->is_neighbour){
			add_sphere_to_sector(temp, &in);		
		}
	}
	write_initial_iteration_stats();
}

// For details on how this works see:
// https://www.gamasutra.com/view/feature/131424/pool_hall_lessons_fast_accurate_.php?page=3
void apply_bounce_between_spheres(struct sphere_s *s1, struct sphere_s *s2) {
	union vector_3d rel_pos;
	rel_pos.x = s1->pos.x - s2->pos.x; 
	rel_pos.y = s1->pos.y - s2->pos.y; 
	rel_pos.z = s1->pos.z - s2->pos.z;
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

static void update_neighbour_spheres(){
	int i, j;
	for(i = 0; i < SECTOR->num_neighbours; i++){
		int id = SECTOR->neighbour_ids[i];
		struct sector_s *sector = &sim_data.sectors_flat[id];
		if(sector->is_local_neighbour){
			continue;
		}
		for(j = 0; j < sector->num_spheres; j++){
			struct sphere_s *s = &(sector->spheres[j]);
			update_sphere_position(s, next_event->time);
		}
	}
}

void update_my_spheres(){
	int i;
	for (i = 0; i < SECTOR->num_spheres; i++) {
		struct sphere_s *s = &(SECTOR->spheres[i]);
		update_sphere_position(s, next_event->time);
	}
}

// Updates spheres belonging to sectors that aren't local neighbours, 
// including the local sector.
// Used when ALL_HELP is true
static void update_all_spheres(){
	int i, j;
	for(i = 0; i < sim_data.num_sectors; i++){
		struct sector_s *sector = &sim_data.sectors_flat[i];
		if(sector->is_local_neighbour){
			continue;
		}
		for(j = 0; j < sector->num_spheres; j++){
			struct sphere_s *s = &(sector->spheres[j]);
			update_sphere_position(s, next_event->time);
		}
	}
}
// This updates the positions and velocities of each sphere once the next
// event and the time it occurs are known.
// If ALL_HELP is enabled then update all spheres except those that belong to 
// local neighbours.
// Otherwise update own spheres and those of non-local neighbours
// Finally update dummy copies of spheres involved in the event.
void update_spheres() {
	if(ALL_HELP){
		update_all_spheres();
	} else {
		update_my_spheres();
		update_neighbour_spheres();
	}
	update_sphere_position(&next_event->sphere_1, next_event->time);
	update_sphere_position(&next_event->sphere_2, next_event->time);
}
