#include <float.h>
#include <stdlib.h>
#include <stdio.h>

#include "event.h"
#include "io.h"
#include "mpi_vars.h"
#include "simulation.h"

struct transmit_event_s *event_buffer; // receive buffer 

static void prepare_event_to_send(){
	event_to_send.time = event_details.time;
	event_to_send.type = event_details.type;
	if(event_details.sphere_1 != NULL){
		event_to_send.sphere_1 = *event_details.sphere_1;
	}
	if(event_details.sphere_2 != NULL){
		event_to_send.sphere_2 = *event_details.sphere_2;
	}
	event_to_send.grid_axis = event_details.grid_axis;
	if(event_details.source_sector != NULL){
		event_to_send.source_sector_id = event_details.source_sector->id;
	}
	if(event_details.dest_sector != NULL){
		event_to_send.dest_sector_id = event_details.dest_sector->id;
	} else {
		event_to_send.dest_sector_id = -1;
	}
}

// TODO: use derived data type rather than sending MPI_CHAR * sizeof(...) number of bytes
void reduce_events(){
	prepare_event_to_send();
	//printf("%d soonest time is %f\n", GRID_RANK, event_details.time);
	MPI_Allgather(
		&event_to_send, sizeof(struct transmit_event_s), MPI_CHAR,
		event_buffer, sizeof(struct transmit_event_s), MPI_CHAR, GRID_COMM
	);
	double soonest_time = DBL_MAX;
	int i;
	for(i = 0; i < NUM_NODES; i++){
		if(event_buffer[i].time < soonest_time){
			GRID_RANK_NEXT_EVENT = i;
			soonest_time = event_buffer[i].time;
		}
	}
	next_event = &event_buffer[GRID_RANK_NEXT_EVENT];
	if(GRID_RANK == 0){
		//printf("Soonest time is %f from rank %d\n", next_event->time, GRID_RANK_NEXT_EVENT);
	}
}

// Seems tricky to set the axis enum to a given size at compile time so 
// determine it at runtime.
// Only done once.
static MPI_Datatype get_axis_enum_size(){
	size_t s = sizeof(enum axis);
	if(s == 4){
		return MPI_INT;
	}
	printf("TODO: other enum sizes\n");
	exit(1);
}

void init_events(){
	event_buffer = malloc(NUM_NODES * sizeof(struct transmit_event_s));
	MPI_Datatype enum_type = get_axis_enum_size();
	// TODO: sphere struct as MPI datatype
	// TODO: transmit_event_s as MPI datatype
};

void reset_event_details(){
	event_details.time = DBL_MAX;
	event_details.sphere_1 = NULL;
	event_details.sphere_2 = NULL;
	event_details.source_sector = NULL;
	event_details.dest_sector = NULL;
	event_details.type = COL_NONE;
	event_details.grid_axis = AXIS_NONE;
}

void set_event_details(
	const double time, const enum collision_type type, const struct sphere_s *sphere_1, 
	const struct sphere_s *sphere_2, const enum axis grid_axis, const struct sector_s *source_sector,
	const struct sector_s *dest_sector
){
	event_details.time = time;
	event_details.type = type;
	event_details.sphere_1 = sphere_1;
	event_details.sphere_2 = sphere_2;
	event_details.grid_axis = grid_axis;
	event_details.source_sector = source_sector;
	event_details.dest_sector = dest_sector;
}

// Sphere bounces off grid boundary.
static void apply_sphere_with_grid_event(){
	struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
	if((source->is_neighbour && !source->is_local_neighbour) || source->id == SECTOR->id){
		struct sphere_s *sphere = &source->spheres[next_event->sphere_1.sector_id];
		sphere->vel.vals[event_details.grid_axis] *= -1.0;
		if(source->id == SECTOR->id){
			write_iteration_data(sphere, NULL);
		}
	}
	if(source->id != SECTOR->id){
		seek_one_sphere();
	}
}

// Sphere on sphere collisions.
// Both spheres within the same sector.
static void apply_sphere_on_sphere_event(){
	struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
	if((source->is_neighbour && !source->is_local_neighbour) || SECTOR->id == next_event->source_sector_id){
		struct sphere_s *s1 = &source->spheres[next_event->sphere_1.sector_id];
		struct sphere_s *s2 = &source->spheres[next_event->sphere_2.sector_id];
		apply_bounce_between_spheres(s1, s2);
		if(source->id == SECTOR->id){
			write_iteration_data(s1, s2);
		}
	}
	if(source->id != SECTOR->id){
		seek_two_spheres();
	}
}


// Sphere moves from one sector to another
static void apply_sphere_transfer_event(){
	// Only used if a local neighbour resizes and the local file backed memory needs to be remapped.
	bool resize_needed = false; 
	struct sphere_s *sphere = &next_event->sphere_1;
	struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
	if(source->is_local_neighbour){
		source->num_spheres--;
		set_largest_radius_after_removal(source, sphere);
	} else if(source->is_neighbour || SECTOR->id == next_event->source_sector_id){
		remove_sphere_from_sector(source, sphere);
	}
	struct sector_s *dest = &sim_data.sectors_flat[next_event->dest_sector_id];
	if(dest->is_local_neighbour){
		dest->num_spheres++;
		set_largest_radius_after_insertion(dest, sphere);
		if(dest->num_spheres >= dest->max_spheres){
			resize_needed = true;
		}
	} else if(dest->is_neighbour || SECTOR->id == next_event->dest_sector_id){
		add_sphere_to_sector(dest, sphere);
		if(dest->id == SECTOR->id){
			write_iteration_data(sphere, NULL);
		}
	}
	if(dest->id != SECTOR->id){
		seek_one_sphere();
	}
	MPI_Barrier(GRID_COMM); 
	// If resize_needed is true then the above barrier ensures the
	// process responsible for the sector has already called ftruncate.
	if(resize_needed){
		resize_sphere_array(dest);
	}
}

// Sphere colliding with sphere in another sector.
static void apply_partial_crossing_event(){
	struct sphere_s *s1 = NULL;
	struct sphere_s *s2 = NULL;
	struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
	struct sector_s *dest = &sim_data.sectors_flat[next_event->dest_sector_id];
	// If source and dest are neighbours to the local or are the local node then bounce both spheres.
	// If one of them but not the other is a neighbour or the local node then bounce the relevant sphere
	// using the copied data in next_event.
	// This will overwrite the copy which is fine.
	if((source->is_neighbour || SECTOR->id == next_event->source_sector_id) && (dest->is_neighbour || SECTOR->id == next_event->dest_sector_id)){		
		if(source->is_local_neighbour){
			s1 = &next_event->sphere_1; // dummy
		} else {
			s1 = &source->spheres[next_event->sphere_1.sector_id]; // local copy
		}
		if(dest->is_local_neighbour){
			s2 = &next_event->sphere_2; // dummy
		} else {
			s2 = &dest->spheres[next_event->sphere_2.sector_id]; // local copy
		}
		apply_bounce_between_spheres(s1, s2);
		if(source->id == SECTOR->id){
			write_iteration_data(s1, s2);
		}
	} else if((source->is_neighbour && !source->is_local_neighbour) || SECTOR->id == next_event->source_sector_id){
		s1 = &source->spheres[next_event->sphere_1.sector_id];
		s2 = &next_event->sphere_2;
		apply_bounce_between_spheres(s1, s2);
	} else if((dest->is_neighbour && !dest->is_local_neighbour) || SECTOR->id == next_event->dest_sector_id){
		s1 = &next_event->sphere_1;
		s2 = &dest->spheres[next_event->sphere_2.sector_id];
		apply_bounce_between_spheres(s1, s2);
	}
	if(source->id != SECTOR->id){
		seek_two_spheres();
	}
}

// Apply events and write changes to file.
// In each case the source sector is responsible for writing data.
// Each other sector must update their file pointer however.
// TODO: clean this up
void apply_event(){
	if (next_event->type == COL_SPHERE_WITH_GRID) {
		apply_sphere_with_grid_event();
		stats.num_grid_collisions++;
	} else if (next_event->type == COL_TWO_SPHERES) {
		apply_sphere_on_sphere_event();
		stats.num_two_sphere_collisions++;
	} else if (next_event->type == COL_SPHERE_WITH_SECTOR) {
		apply_sphere_transfer_event();
		stats.num_sector_transfers++;
	} else if(next_event->type == COL_TWO_SPHERES_PARTIAL_CROSSING){
		apply_partial_crossing_event();
		stats.num_partial_crossings++;
	}
}

