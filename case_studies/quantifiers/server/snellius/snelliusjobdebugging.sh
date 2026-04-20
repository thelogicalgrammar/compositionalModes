#!/bin/bash
#SBATCH --ntasks 16
#SBATCH -p rome
#SBATCH -t 100:00:00

# Debug variant for the quantifiers case study.

module load 2022
module load Eigen/3.4.0-GCCcore-11.3.0
module load binutils/2.38-GCCcore-11.3.0
module load parallel/20220722-GCCcore-11.3.0

cd ../../../../
make debug CASESTUDY=quantifiers

ID=$(date +"%Y%m%d_%H%M%S")
./main \
	--steps 			200000 	 		\
	--nobs 				200	 	 		\
	--csize 			5 		 		\
	--likelihoodweight 	30	 			\
	--searchdepth		2		 		\
	--fname 			"data/${ID}"	\
	--ct				16				\
	--simtype			"DEBUG"
