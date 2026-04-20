Snellius job scripts for the `adjectives` case study.

Case study is selected at build time via `make CASESTUDY=adjectives`; snelliusjob.sh
already passes this, so no Main.cpp edits are needed.

Don't forget to write `g++ -I../../../` in the Makefile to make eigen3 available
(or whatever path contains the eigen3 folder).

The parent folder of compositionalModes should contain:
- eigen3
- json.hpp
- Fleet

Submit from this directory (case_studies/adjectives/server/snellius):
  sbatch snelliusjob.sh <likweight> <num_adjs> <pragmatic> <exclude_empty>

The `--exclude-empty` CLI flag is case-study-agnostic on the `main` binary;
each case study defines internally what "empty" means (see hypothesis.h).

Code tested with:
- g++-10 (Ubuntu 10.5.0-1ubuntu1~20.04) 10.5.0 (for `make local`)
- g++ (GCC) 11.3.0 (for `make`)
