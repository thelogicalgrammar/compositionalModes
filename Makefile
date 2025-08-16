# Define where Fleet lives (directory containing src)
FLEET_ROOT=../Fleet

EXTRA_FLAGS ?=

include $(FLEET_ROOT)/Fleet.mk

all:
	g++ -I../ Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS) $(EXTRA_FLAGS)
local:
	g++-10 -I../ -I../../ Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS) $(EXTRA_FLAGS)
debuglocal:
	g++-10 -Wall -Wextra -pedantic -I../ Main.cpp -o main -g $(FLEET_FLAGS) $(FLEET_INCLUDE) -I /usr/include/eigen3/ $(FLEET_LIBS) $(SANITARY_FLAGS) $(EXTRA_FLAGS)
	# g++-10  -I../ -I../../ Main.cpp -o main -g -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I /usr/include/eigen3/ $(FLEET_LIBS) -lbfd -ldl $(EXTRA_FLAGS)
static:
	g++ -I../../ Main.cpp -o main -O3 -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS) $(EXTRA_FLAGS)
debug:
	g++ -Wall -Wextra -pedantic -I../ Main.cpp -o main -g $(FLEET_FLAGS) $(FLEET_INCLUDE) -I /usr/include/eigen3/ $(FLEET_LIBS) $(SANITARY_FLAGS) $(EXTRA_FLAGS)
	# When using backward.hpp to find source of segfaults, use this and uncomment DEBUG lines in main
	# g++ -I../ Main.cpp -o main -g -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I /usr/include/eigen3/ $(FLEET_LIBS) -lbfd -ldl $(EXTRA_FLAGS)
clang:
	clang++ -I../../ Main.cpp -o main -O2 $(CLANG_FLAGS) $(FLEET_INCLUDE) $(FLEET_LIBS)
profiled:
	g++ -I../../ Main.cpp -o main -g -pg -fprofile-arcs -ftest-coverage $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS) $(EXTRA_FLAGS)
conda:
	x86_64-conda-linux-gnu-gcc -I../../ Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS) $(EXTRA_FLAGS)
