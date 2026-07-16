#!/bin/bash
#SBATCH --ntasks 16
#SBATCH -p rome
#SBATCH -t 100:00:00

# Adjectives case study — selected at build time via `make CASESTUDY=adjectives` below.

module load 2022
module load Eigen/3.4.0-GCCcore-11.3.0
module load binutils/2.38-GCCcore-11.3.0
module load parallel/20220722-GCCcore-11.3.0

DEFAULTLIK=60
DEFAULTNUMADJS=5
DEFAULTPRAGMATIC=1
DEFAULT_EXCLUDE_EMPTY=0
DEFAULTPTARGET=0.5
DEFAULTLENGTHWEIGHT=0
DEFAULTSEARCHDEPTH=2

ARG1=${1:-$DEFAULTLIK}
ARG2=${2:-$DEFAULTNUMADJS}
ARG3=${3:-$DEFAULTPRAGMATIC}
ARG4=${4:-$DEFAULT_EXCLUDE_EMPTY}
ARG5=${5:-$DEFAULTPTARGET}
ARG6=${6:-$DEFAULTLENGTHWEIGHT}
ARG7=${7:-$DEFAULTSEARCHDEPTH}

# Assuming sbatch is run from case_studies/adjectives/server/snellius,
# jump 4 levels up to the compositionalModes root.
cd ../../../../
make CASESTUDY=adjectives EXTRA_FLAGS="-DNUM_ADJS=$ARG2"

ID=$(date +"%Y%m%d_%H%M%S_%3N")
./main \
	--steps 			2000 	 		\
	--ptarget			$ARG5			\
	--nobs 				500		 	 	\
	--csize 			5 		 		\
	--likelihoodweight 	$ARG1	 		\
	--lengthweight		$ARG6			\
	--searchdepth		$ARG7			\
	--fname 			"data/${ID}"	\
	--pragmatic			$ARG3			\
	--exclude-empty		$ARG4			\
	--ct				16				\
	--simtype			"TRADEOFF"
