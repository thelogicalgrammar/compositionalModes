#!/bin/bash
#SBATCH -n 16
#SBATCH -p rome
#SBATCH -t 50:00

# Run this code in a file with source setup.sh
module load 2022
# Turns out that g++-11 works for compiling the models
module load Eigen/3.4.0-GCCcore-11.3.0
# This to run "make debug" so you can use --gdwarf-5
module load binutils/2.38-GCCcore-11.3.0

# Compile script
cd ../../
make
# Execute script 
./main \
	--ngenerations 	5 		\
	--nagents 		16 		\
	--nobs 			100 	\
	--csize 		5 		\
	--pright 		0.9999 	\
	--fnameaddition ""

