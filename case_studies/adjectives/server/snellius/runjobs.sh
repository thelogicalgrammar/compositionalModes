#!/bin/bash

# Research question: which threshold (POS) is optimal when `not` is excluded
# from the signals, as the probability of an entity being a target varies?
#
# `notP` is removed from the lexicon in hypothesis.h (add_PMs = false), so
# the speaker cannot compensate for a misplaced threshold by negating.
#
# Sweep dimensions:
#   - pTarget (5 values): the variable of interest
#   - lengthWeight (4 values): penalty on avg produced-sentence length
#       0 = off (new ARGMAX tiebreaker still prefers shorter on exact ties)
#       0.5–2 = active penalty; >2 collapses to "always shortest" (see notes
#       in project memory)
#   - searchDepth: controls the achievable length range (and run cost)

# args:                 likW  num_adjs  pragmatic  excl_empty  pTarget  lengthW  searchD

# --- Primary: pTarget × lengthWeight at pragmatic=1, searchDepth=2 (20 jobs) ---
for ptarget in 0.1 0.25 0.5 0.75 0.9; do
    for lengthweight in 0 0.5 1 2; do
        sbatch snelliusjob.sh  20   3       1          0          $ptarget  $lengthweight  2
        sleep 2
    done
done

# --- Secondary (uncomment): same grid at searchDepth=3 to expand the
#     achievable length range (20 jobs). Note: enumeration grows fast with depth.
# for ptarget in 0.1 0.25 0.5 0.75 0.9; do
#     for lengthweight in 0 0.5 1 2; do
#         sbatch snelliusjob.sh  20   3       1          0          $ptarget  $lengthweight  3
#         sleep 2
#     done
# done

# --- Literal-mode counterpart (uncomment) at searchDepth=2 (10 jobs):
# for ptarget in 0.1 0.25 0.5 0.75 0.9; do
#     for lengthweight in 0 1; do
#         sbatch snelliusjob.sh  20   3       0          0          $ptarget  $lengthweight  2
#         sleep 2
#     done
# done
