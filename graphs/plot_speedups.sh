#!/usr/bin/gnuplot -persist

set terminal postscript eps enhanced color font 'Helvetica,10'
set output 'serial_50000_speedup.eps'
set xlabel 'Number of sectors'
set ylabel 'Speedup vs no domain decomposition'
set xtics ('2' 0, '4' 1, '8' 2, '12' 3, '16' 4 ,'24' 5, '32' 6, '48' 7, '64' 8, '256' 9, '1024' 10, '4096' 11)
plot 'serial_50000_speedup.txt' using 0:2 with linespoints lt rgb 'red' title 'Number of sectors vs speedup, 500000 spheres'

set output 'mpi_all_50000_speedup.eps'
set xlabel 'Number of cores/sectors'
set ylabel 'Speedup vs serial with same number of sectors'
set xtics ('2' 0, '4' 1, '8' 2, '12' 3, '16' 4 ,'24' 5, '32' 6, '48' 7, '64' 8)
plot 'mpi_all_50000_speedup.txt' using 0:2 with linespoints lt rgb 'red' title 'MPI all help speedup vs equivilant serial', \
'mpi_neighbour_50000_speedup.txt' using 0:2 with linespoints lt rgb 'blue' title 'MPI neighbour help speedup vs equivilant serial'

exit
