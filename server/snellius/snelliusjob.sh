#!/bin/bash
#SBATCH -n 16
#SBATCH -t 5:00

# Run this code in a file with source setup.sh
module load 2022
# Turns out that g++-11 works for compiling the models
module load Eigen/3.4.0-GCCcore-11.3.0
# also add this to run "make debug" so you can use --gdwarf-5
module load binutils/2.38-GCCcore-11.3.0
 
#Copy input file to scratch and create output directory on scratch
cp $HOME/big_input_file "$TMPDIR"
mkdir "$TMPDIR"/output_dir
 
#Execute script (newline)
python $HOME/my_program.py \
	"$TMPDIR"/big_input_file "$TMPDIR"/output_dir
 
#Copy output directory from scratch to home
cp -r "$TMPDIR"/output_dir $HOME
