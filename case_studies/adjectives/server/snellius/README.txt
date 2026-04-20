Snellius job scripts for the `adjectives` case study.

Before submitting, ensure Main.cpp has the `adjectives` case study active
(its config.h included before types.h, its hypothesis.h after agent.h).

Don't forget to write `g++ -I../../../` in the Makefile to make eigen3 available
(or whatever path contains the eigen3 folder).

The parent folder of compositionalModes should contain:
- eigen3
- json.hpp
- Fleet

Submit from this directory (case_studies/adjectives/server/snellius):
  sbatch snelliusjob.sh <likweight> <num_adjs> <pragmatic> <exclude_empty_adjs>

Note: the CLI flag is still `--exclude-empty-qs` on the `main` binary; for the
adjectives case study it maps internally to the equivalent "exclude empty
adjectives" behavior.

Code tested with:
- g++-10 (Ubuntu 10.5.0-1ubuntu1~20.04) 10.5.0 (for `make local`)
- g++ (GCC) 11.3.0 (for `make`)
