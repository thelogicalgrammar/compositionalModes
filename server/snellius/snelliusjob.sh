#!/bin/bash
#SBATCH --ntasks 16
#SBATCH --cpus-per-task 1
#SBATCH -p rome
#SBATCH -t 100:00:00

# Run this code in a file with source setup.sh
module load 2022
# Turns out that g++-11 works for compiling the models
module load Eigen/3.4.0-GCCcore-11.3.0
# This to run "make debug" so you can use --gdwarf-5
module load binutils/2.38-GCCcore-11.3.0
# To run in parallel
module load parallel/20220722-GCCcore-11.3.0

# Compile script (assuming sbatch is run from the ./server/snellius directory)
cd ../../
make

# ID is a string with current time
ID = $(date +"%Y%m%d_%H%M%S")
seq 1 16 | parallel -j 16 ./main \
	--ngenerations 	100 	 \
	--nagents 		10 		 \
	--nobs 			100	 	 \
	--csize 		5 		 \
	--pright 		0.9999 	 \
	--pmutation		0.0		 \
	--commselectionstrength 0.0 \
	--fname "{$ID}/run_{}" \
	--ct			4
	

# ./main --ngenerations 1000 --nagents 16 --nobs 100 --csize 5 --pright 0.9999 --pmutation 0.05 --fnameaddition "" --ct 4
