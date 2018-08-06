#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "event.h"
#include "io.h"
#include "mpi_vars.h"
#include "simulation.h"

struct event_s helping_event_details; // tracks next event if helping another node
struct transmit_event_s *help_event_buffer; // if node is helped this stores times comptued by other nodes
struct transmit_event_s help_event_to_send;
struct transmit_event_s *event_buffer; // receive buffer 

static void prepare_event_to_send(){
	event_to_send.time = event_details.time;
	event_to_send.type = event_details.type;
	event_to_send.sphere_1 = *event_details.sphere_1;
	if(event_details.sphere_2 != NULL){
		event_to_send.sphere_2 = *event_details.sphere_2;
	} else {
		event_to_send.sphere_2.id = -1;
	}
	event_to_send.grid_axis = event_details.grid_axis;
	event_to_send.source_sector_id = event_details.source_sector->id;
	if(event_details.dest_sector != NULL){
		event_to_send.dest_sector_id = event_details.dest_sector->id;
	} else {
		event_to_send.dest_sector_id = -1;
	}
}

static void prepare_help_event_to_send(){
	help_event_to_send.time = helping_event_details.time;
	help_event_to_send.type = helping_event_details.type;
	help_event_to_send.sphere_1 = *helping_event_details.sphere_1;
	if(helping_event_details.sphere_2 != NULL){
		help_event_to_send.sphere_2 = *helping_event_details.sphere_2;
	} else {
		help_event_to_send.sphere_2.id = -1;
	}
	help_event_to_send.grid_axis = helping_event_details.grid_axis;
	help_event_to_send.source_sector_id = helping_event_details.source_sector->id;
	if(helping_event_details.dest_sector != NULL){
		help_event_to_send.dest_sector_id = helping_event_details.dest_sector->id;
	} else {
		help_event_to_send.dest_sector_id = -1;
	}
}

static void copy_received_help_to_event_details(){
	int i;
	int soonest = GRID_RANK;
	for(i = 0; i < NUM_NODES; i++){
		if(help_event_buffer[i].time < event_details.time){
			soonest = i;
		}
	}
	struct transmit_event_s *e = &help_event_buffer[soonest];
	/*printf("Node %d has received help event\n", GRID_RANK);
	printf("Soonest is %d\n", soonest);
	printf("Soonest is as follows:\n");
	printf("Time: %.17g\n", e->time);
	printf("Type: %d\n", e->type);
	printf("Sphere one id: %ld and sector id: %ld\n", e->sphere_1.id, e->sphere_1.sector_id);
	printf("Sphere two id: %ld and sector id: %ld\n", e->sphere_2.id, e->sphere_2.sector_id);
	printf("Source sector id: %d\n", e->source_sector_id);
	printf("Dest sector id: %d\n", e->dest_sector_id);*/
	event_details.time = e->time;
	event_details.type = e->type;
	event_details.grid_axis = e->grid_axis;
	event_details.sphere_1 = &SECTOR->spheres[e->sphere_1.sector_id];
	if(e->sphere_2.sector_id != -1){
		event_details.sphere_2 = &SECTOR->spheres[e->sphere_2.sector_id];
	}
	event_details.source_sector = &sim_data.sectors_flat[e->source_sector_id];
	if(e->dest_sector_id != -1){
		event_details.dest_sector = &sim_data.sectors_flat[e->dest_sector_id];
	}
	printf("Soonest time after reduce is %f\n", event_details.time);
}

// All nodes helping send their soonest event for the helped sector to the
// helped sector.
// The helped sector then finds its own soonest event by coomparing these.
void reduce_all_help_events_one_invalid(){
	if(SECTOR->id == invalid_1->id){
		prepare_event_to_send();
		MPI_Gather(
			&event_to_send, sizeof(struct transmit_event_s), MPI_CHAR,
			help_event_buffer, sizeof(struct transmit_event_s), MPI_CHAR, 
			invalid_1->id, GRID_COMM
		);
		copy_received_help_to_event_details();
	} else {
		prepare_help_event_to_send();
		MPI_Gather(
			&help_event_to_send, sizeof(struct transmit_event_s), MPI_CHAR,
			NULL, sizeof(struct event_s), MPI_CHAR,
			invalid_1->id, GRID_COMM
		);
	}
}

// TODO: use derived data type rather than sending MPI_CHAR * sizeof(...) number of bytes
void reduce_events(){
	prepare_event_to_send();
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
	help_event_buffer = malloc(NUM_NODES * sizeof(struct transmit_event_s));
	num_invalid = NUM_NODES;
	reset_event_details();
	reset_event_details_helping();
	MPI_Datatype enum_type = get_axis_enum_size();
	helping = false;
	// TODO: sphere struct as MPI datatype
	// TODO: transmit_event_s as MPI datatype
};

void reset_event_details_helping(){
	helping_event_details.time = DBL_MAX;
	helping_event_details.sphere_1 = NULL;
	helping_event_details.sphere_2 = NULL;
	helping_event_details.source_sector = NULL;
	helping_event_details.dest_sector = NULL;
	helping_event_details.type = COL_NONE;
	helping_event_details.grid_axis = AXIS_NONE;
}

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
	if(helping){
		if(time < helping_event_details.time){
			helping_event_details.time = time;
			helping_event_details.type = type;
			helping_event_details.sphere_1 = sphere_1;
			helping_event_details.sphere_2 = sphere_2;
			helping_event_details.grid_axis = grid_axis;
			helping_event_details.source_sector = source_sector;
			helping_event_details.dest_sector = dest_sector;
		}
	} else {
		if(time < event_details.time){
			event_details.time = time;
			event_details.type = type;
			event_details.sphere_1 = sphere_1;
			event_details.sphere_2 = sphere_2;
			event_details.grid_axis = grid_axis;
			event_details.source_sector = source_sector;
			event_details.dest_sector = dest_sector;
		}
	}
}

// Sphere bounces off grid boundary.
static void apply_sphere_with_grid_event(){
	struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
	struct sphere_s *sphere = NULL;
	if(ALL_HELP && !source->is_local_neighbour){
		sphere = &source->spheres[next_event->sphere_1.sector_id];
		sphere->vel.vals[next_event->grid_axis] *= -1.0;
	} else if((source->is_neighbour && !source->is_local_neighbour) || source->id == SECTOR->id){
		sphere = &source->spheres[next_event->sphere_1.sector_id];
		sphere->vel.vals[next_event->grid_axis] *= -1.0;
	}
	if(source->id != SECTOR->id){
		seek_one_sphere();
	} else {
		write_iteration_data(sphere, NULL);
	}
	if(source->id == SECTOR->id){
		PRIOR_TIME_VALID = false;
	}
}

// Sphere on sphere collisions.
// Both spheres within the same sector.
static void apply_sphere_on_sphere_event(){
	struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
	struct sphere_s *s1 = NULL;
	struct sphere_s *s2 = NULL;
	if(ALL_HELP && !source->is_local_neighbour){
		s1 = &source->spheres[next_event->sphere_1.sector_id];
		s2 = &source->spheres[next_event->sphere_2.sector_id];
		apply_bounce_between_spheres(s1, s2);
	} else if((source->is_neighbour && !source->is_local_neighbour) || SECTOR->id == next_event->source_sector_id){
		s1 = &source->spheres[next_event->sphere_1.sector_id];
		s2 = &source->spheres[next_event->sphere_2.sector_id];
		apply_bounce_between_spheres(s1, s2);
	}
	if(source->id != SECTOR->id){
		seek_two_spheres();
	} else {
		write_iteration_data(s1, s2);
	}
	if(source->id == SECTOR->id){
		PRIOR_TIME_VALID = false;
	}
}

// Sphere moves from one sector to another
// Only non local neighbours should update their copy of the sphere.
static void apply_sphere_transfer_event(){
	// Only used if a local neighbour resizes and the local file backed memory needs to be remapped.
	bool resize_needed = false; 
	struct sphere_s *sphere = &next_event->sphere_1;
	struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
	struct sector_s *dest = &sim_data.sectors_flat[next_event->dest_sector_id];
	if(source->is_local_neighbour){
		source->num_spheres--;
		set_largest_radius_after_removal(source, sphere);
	} else if(ALL_HELP || source->is_neighbour || SECTOR->id == next_event->source_sector_id){
		remove_sphere_from_sector(source, sphere);
	}
	if(dest->is_local_neighbour){
		dest->num_spheres++;
		set_largest_radius_after_insertion(dest, sphere);
		if(dest->num_spheres >= dest->max_spheres){
			resize_needed = true;
		}
	} else if(ALL_HELP || dest->is_neighbour || SECTOR->id == next_event->dest_sector_id){
		add_sphere_to_sector(dest, sphere);
	}
	if(dest->id != SECTOR->id){
		seek_one_sphere();
	} else {
		write_iteration_data(sphere, NULL);
		if(sphere->sector_id < 0){
			printf("!!!!!!!!\n");
			exit(1);
		}
	}
	MPI_Barrier(GRID_COMM); 
	// If resize_needed is true then the above barrier ensures the
	// process responsible for the sector has already called ftruncate.
	if(resize_needed){
		resize_sphere_array(dest);
	}
	if(source->id == SECTOR->id || dest->id == SECTOR->id){
		PRIOR_TIME_VALID = false;
	}
}

// Sphere colliding with sphere in another sector.
static void apply_partial_crossing_event(){
	struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
	struct sector_s *dest = &sim_data.sectors_flat[next_event->dest_sector_id];
	struct sphere_s *s1 = NULL;
	struct sphere_s *s2 = NULL;
	if(source->is_local_neighbour){
		s1 = &next_event->sphere_1; // dummy
	} else if(ALL_HELP || source->is_neighbour || source->id == SECTOR->id){
		s1 = &source->spheres[next_event->sphere_1.sector_id]; // local copy
	}
	if(dest->is_local_neighbour){
		s2 = &next_event->sphere_2; // dummy
	} else if(ALL_HELP || dest->is_neighbour || dest->id == SECTOR->id){
		s2 = &dest->spheres[next_event->sphere_2.sector_id]; // local copy
	}
	if(s1 != NULL && s2 != NULL){
		apply_bounce_between_spheres(s1, s2);
	}
	if(source->id != SECTOR->id){
		seek_two_spheres();
	} else {
		write_iteration_data(s1, s2);
	}
	if(source->id == SECTOR->id || dest->id == SECTOR->id){
		PRIOR_TIME_VALID = false;
	}
}

// Apply events and write changes to file.
// In each case the source sector is responsible for writing data.
// Each other sector must update their file pointer however.
// TODO: clean this up
void apply_event(){
	PRIOR_TIME_VALID = true;
	if (next_event->type == COL_SPHERE_WITH_GRID) {
		apply_sphere_with_grid_event();
		invalid_1 = &sim_data.sectors_flat[next_event->source_sector_id];
		num_invalid = 1;
		stats.num_grid_collisions++;
	} else if (next_event->type == COL_TWO_SPHERES) {
		apply_sphere_on_sphere_event();
		invalid_1 = &sim_data.sectors_flat[next_event->source_sector_id];
		num_invalid = 1;
		stats.num_two_sphere_collisions++;
	} else if (next_event->type == COL_SPHERE_WITH_SECTOR) {
		apply_sphere_transfer_event();
		invalid_1 = &sim_data.sectors_flat[next_event->source_sector_id];
		invalid_2 = &sim_data.sectors_flat[next_event->dest_sector_id];
		num_invalid = 2;
		stats.num_sector_transfers++;
	} else if(next_event->type == COL_TWO_SPHERES_PARTIAL_CROSSING){
		apply_partial_crossing_event();
		invalid_1 = &sim_data.sectors_flat[next_event->source_sector_id];
		invalid_2 = &sim_data.sectors_flat[next_event->dest_sector_id];
		num_invalid = 2;
		stats.num_partial_crossings++;
	}
}

