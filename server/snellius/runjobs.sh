#!/bin/bash

# This is a parameter sweep over 
#   1. the number of quantifiers
#   2. the likelihood weight
#   3. the pragmatic setting

# run once for literal and once for pragmatic
for k in 0 1; do
    # run jobs for 20, 30, 40 likelihood weight
    for j in 20 30 40; do
        # run jobs for 2, 3, 4, 5 quantifiers
        for i in 2 3 4 5; do
            sbatch snelliusjob.sh $j $i $k
            # wait 1 second
            sleep 1
        done
    done
done