#!/bin/bash
#SBATCH --ntasks 16
#SBATCH -p rome
#SBATCH -t 100:00:00

# Quantifiers-composite case study — requires the `quantifiers_composite`
# case study to be selected via the includes in Main.cpp.

module load 2022
module load Eigen/3.4.0-GCCcore-11.3.0
module load binutils/2.38-GCCcore-11.3.0
module load parallel/20220722-GCCcore-11.3.0

DEFAULTLIK=40
DEFAULTNUMQUANTS=3
DEFAULTPRAGMATIC=1
DEFAULT_EXCLUDE_EMPTY_QS=0

ARG1=${1:-$DEFAULTLIK}
ARG2=${2:-$DEFAULTNUMQUANTS}
ARG3=${3:-$DEFAULTPRAGMATIC}
ARG4=${4:-$DEFAULT_EXCLUDE_EMPTY_QS}

# Assuming sbatch is run from case_studies/quantifiers_composite/server/snellius,
# jump 4 levels up to the compositionalModes root.
cd ../../../../
make CASESTUDY=quantifiers_composite EXTRA_FLAGS="-DNUM_QUANTS=$ARG2"

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
