#!/bin/bash

# NOTE: running ./runjobs.sh was giving me problems
# So I just did it manually for the different settings

# This is a parameter sweep over 
#   1. the likelihood weight
#   2. the number of quantifiers
#   3. the pragmatic setting

# run once for literal and once for pragmatic
# for k in 0 1; do
#     # run jobs for 20, 30, 40 likelihood weight
#     for j in 20 30 40; do
#         # run jobs for 2, 3, 4, 5 quantifiers
#         for i in 2 3 4 5; do
#             sbatch snelliusjob.sh $j $i $k
#             # wait 1 second
#             sleep 1
#         done
#     done
# done

#                       likweight   num_quants  pragmatic
sbatch snelliusjob.sh   20          3           0
wait 2
sbatch snelliusjob.sh   30          3           0
wait 2
sbatch snelliusjob.sh   40          3           0
wait 2
sbatch snelliusjob.sh   20          3           1
wait 2
sbatch snelliusjob.sh   30          3           1
wait 2
sbatch snelliusjob.sh   40          3           1