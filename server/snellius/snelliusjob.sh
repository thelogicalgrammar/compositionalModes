#!/bin/bash
#SBATCH --ntasks 16
#SBATCH -p rome
#SBATCH -t 100:00:00

ulimit -s unlimited

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
ID=$(date +"%Y%m%d_%H%M%S")
./main \
	--steps 			100000 	 		\
	--nobs 				50	 	 		\
	--csize 			5 		 		\
	--likelihoodweight 	30	 			\
	--searchdepth		2		 		\
	--fname 			"data/${ID}"	\
	--ct				16
	
RET=$?
if [ $RET -ne 0 ]; then
    if [ -f core ]; then
        echo "Program crashed with exit code $RET. Generating backtrace..."
        gdb -batch -ex "thread apply all backtrace full" -ex "quit" ./main core > backtrace.txt
        echo "Backtrace saved to backtrace.txt"
    else
        echo "Program crashed but no core file found."
    fi
fi

# ./main --steps 10000 --nobs 5 --csize 5 --likelihoodweight 30 --searchdepth 2 --fname "data/10" --ct 16

# ./main --steps 10000 --nobs 100 --likelihoodweight 50 --chains 16
