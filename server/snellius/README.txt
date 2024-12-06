
Don't forget to write the g++ -I../../../ to the makefile to make eigen3 available (or whatever path contains the eigen3 folder).

The parent folder of compositionalModes should contain:
- eigen3
- json.hpp
- Fleet

Code was tested with:
- g++-10 (Ubuntu 10.5.0-1ubuntu1~20.04) 10.5.0 (for make local)
- g++ (GCC) 11.3.0 (for make)
