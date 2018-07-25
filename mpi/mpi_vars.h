#pragma once

#include <mpi.h>

#include "event.h"
#include "sector.h"

const int REORDER;
const int NUM_DIMS;
const int PERIODS[3];

int WORLD_RANK; // rank in MPI_COMM_WORLD
int GRID_RANK; // rank in GRID_COMM
int NUM_NODES;
MPI_Comm GRID_COMM;
int COORDS[3];

int GRID_RANK_NEXT_EVENT; // the node with the soonest event

struct sector_s *SECTOR;

