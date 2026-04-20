#!/bin/bash

# Parameter sweep over:
#   1. likelihood weight
#   2. number of adjectives
#   3. pragmatic setting

# for k in 0 1; do
#     for j in 20 30 40; do
#         for i in 2 3 4 5; do
#             sbatch snelliusjob.sh $j $i $k
#             sleep 1
#         done
#     done
# done

#                       likweight   num_adjs    pragmatic
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
