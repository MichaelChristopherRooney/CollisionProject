#!/bin/bash

#SBATCH -n 1        # 8 cores = 1 node on lonsdale
#SBATCH -p compute
#SBATCH -t 0:10:00
#SBATCH -U mschpc
#SBATCH -J thesis
#SBATCH --reservation=application

# source the module commands
source /etc/profile.d/modules.sh

# load the modules used
module load apps libs cports openmpi

# run it
./prog -i 4000.spheres -o data.bin -c final.bin -x 4 -y 4


