rm ./*.spheres 2> /dev/null
rm ../serial/*.spheres 2> /dev/null
rm ../mpi/*.spheres 2> /dev/null
gcc config_generator.c
./a.out
cp ./*.spheres ../serial
cp ./*.spheres ../mpi
rm a.out
