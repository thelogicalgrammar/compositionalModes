#!/bin/bash
#SBATCH --ntasks 16
#SBATCH -p rome
#SBATCH -t 100:00:00

# Adjectives case study — requires the `adjectives` case study to be
# selected via the includes in Main.cpp.

module load 2022
module load Eigen/3.4.0-GCCcore-11.3.0
module load binutils/2.38-GCCcore-11.3.0
module load parallel/20220722-GCCcore-11.3.0

DEFAULTLIK=40
DEFAULTNUMADJS=3
DEFAULTPRAGMATIC=1
DEFAULT_EXCLUDE_EMPTY_ADJS=0

ARG1=${1:-$DEFAULTLIK}
ARG2=${2:-$DEFAULTNUMADJS}
ARG3=${3:-$DEFAULTPRAGMATIC}
ARG4=${4:-$DEFAULT_EXCLUDE_EMPTY_ADJS}

# Assuming sbatch is run from case_studies/adjectives/server/snellius,
# jump 4 levels up to the compositionalModes root.
cd ../../../../
make CASESTUDY=adjectives EXTRA_FLAGS="-DNUM_ADJS=$ARG2"

ID=$(date +"%Y%m%d_%H%M%S_%3N")
./main \
	--steps 			200000 	 		\
	--nobs 				500		 	 	\
	--csize 			5 		 		\
	--likelihoodweight 	$ARG1	 		\
	--searchdepth		2		 		\
	--fname 			"data/${ID}"	\
	--pragmatic			$ARG3			\
	--exclude-empty-qs	$ARG4			\
	--ct				16				\
	--simtype			"TRADEOFF"
