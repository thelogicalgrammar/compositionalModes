#!/bin/bash
#SBATCH --ntasks 16
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

DEFAULTLIK = 40
# optional argument for likeweights
ARG1=${1:-$DEFAULTLIK}

# ID is a string with current time
ID=$(date +"%Y%m%d_%H%M%S")
./main \
	--steps 			100000 	 		\
	--nobs 				250	 	 		\
	--csize 			5 		 		\
	--likelihoodweight 	$ARG1	 		\
	--searchdepth		2		 		\
	--fname 			"data/${ID}"	\
	--ct				16

# ./main --steps 10000 --nobs 5 --csize 5 --likelihoodweight 30 --searchdepth 2 --fname "data/10" --ct 16
