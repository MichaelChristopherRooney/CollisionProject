#include <float.h>
#include <stdlib.h>

#include "event.h"
#include "mpi_vars.h"

// This is the event data that is sent/received to/from other nodes.
// Instead of using pointers as above it uses sector ids and includes the full
// sphere struct.
// This is because pointers will no longer be valid once sent to another node.
// The local event_details struct will have its data copied here for sending.
struct transmit_event_s {
	double time;
	enum collision_type type;
	struct sphere_s sphere_1;
	struct sphere_s sphere_2;
	enum axis grid_axis;
	int source_sector_id;
	int dest_sector_id;
};

struct transmit_event_s event_to_send;

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
	}
}

void reduce_events(){
	prepare_event_to_send();
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

