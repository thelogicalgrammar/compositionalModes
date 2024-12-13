#!/bin/bash
#SBATCH --ntasks 16
#SBATCH -p rome
#SBATCH -t 100:00:00

# segfault happens when the stack size is too small
# so I set it to unlimited
ulimit -s unlimited

# Run this code in a file with source setup.sh
module load 2022
# Turns out that g++-11 works for compiling the models
module load Eigen/3.4.0-GCCcore-11.3.0
# This to run "make debug" so you can use --gdwarf-5
module load binutils/2.38-GCCcore-11.3.0
# To run in parallel
module load parallel/20220722-GCCcore-11.3.0

cd ../../
make debug
gdb -batch -ex "run" -ex "thread apply all backtrace full" -ex "quit" --args ./main \
	--steps 			10000 	 		\
	--nobs 				1000	 	 	\
	--csize 			5 		 		\
	--likelihoodweight 	30	 			\
	--searchdepth		2		 		\
	--fname 			"data/${ID}"	\
	--ct				16
	
# ./main --steps 10000 --nobs 5 --csize 5 --likelihoodweight 30 --searchdepth 2 --fname "data/10" --ct 16
