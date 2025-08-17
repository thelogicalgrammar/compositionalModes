#!/bin/bash
#SBATCH --ntasks 16
#SBATCH -p rome
#SBATCH -t 50:00:00

# Run this code in a file with source setup.sh
module load 2022
# Turns out that g++-11 works for compiling the models
module load Eigen/3.4.0-GCCcore-11.3.0
# This to run "make debug" so you can use --gdwarf-5
module load binutils/2.38-GCCcore-11.3.0
# To run in parallel
module load parallel/20220722-GCCcore-11.3.0

# default values
DEFAULTLIK=40
DEFAULTNUMQUANTS=3
DEFAULTPRAGMATIC=1

# optional argument for likeweights
ARG1=${1:-$DEFAULTLIK}
# optional argument for num_quants
ARG2=${2:-$DEFAULTNUMQUANTS}
# optional argument for pragmatic
ARG3=${3:-$DEFAULTPRAGMATIC}

# Compile script (assuming sbatch is run from the ./server/snellius directory)
cd ../../
make EXTRA_FLAGS="-DNUM_QUANTS=$ARG2"

# ID is a string with current time up to milliseconds
ID=$(date +"%Y%m%d_%H%M%S_%3N")
./main \
	--steps 			100000 	 		\
	--nobs 				500		 	 	\
	--csize 			5 		 		\
	--likelihoodweight 	$ARG1	 		\
	--searchdepth		2		 		\
	--fname 			"data/${ID}"	\
	--pragmatic			$ARG3			\
	--ct				16

# ./main --steps 10000 --nobs 5 --csize 5 --likelihoodweight 30 --searchdepth 2 --fname "data/10" --ct 16
