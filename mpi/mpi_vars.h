#include <mpi.h>
#include "sector.h"

const int REORDER;
const int NUM_DIMS;
const int PERIODS[3];

int RANK;
int NUM_NODES;
MPI_Comm GRID_COMM;
int COORDS[3];

struct sector_s *SECTOR;
