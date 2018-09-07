# CollisionProject

The serial and MPI versions exist in different directories.

To build either just cd into the directory and type make.

Before running the code data sets must be generated.

These are too large to include in this repository.

To generate the data sets cd into the config_generator directory.

Then make run.sh executable and run it.

Data sets will then be created in the serial/ and mpi/ directories.

Note that some of these data sets are quite large so you will want to delete them afterwards.

The visualiser can accept the binary files produced by the collision detection programs.

These are specified by the -i arg passed to the collision detection programs.

The visualiser is Windows only and requires Visual Studio to use.
