#include "mpi_vars.h"

const int REORDER = 1;
const int NUM_DIMS = 3;
const int PERIODS[3] = { 0, 0, 0 }; // no pbc at all

int RANK;
int NUM_NODES;
MPI_Comm GRID_COMM;
int COORDS[3];

struct sector_s *SECTOR;
