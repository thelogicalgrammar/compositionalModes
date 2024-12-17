# Define where Fleet lives (directory containing src)
FLEET_ROOT=../Fleet

include $(FLEET_ROOT)/Fleet.mk


all:
	g++ -I../ Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)
local:
	g++-10 -I../ -I../../ Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)
debuglocal:
	g++-10 -Wall -Wextra -pedantic -I../ -I../../ Main.cpp -o main -g $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)
static:
	g++ -I../../ Main.cpp -o main -O3 -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)
debug:
	g++ -Wall -Wextra -pedantic -I../ Main.cpp -o main -g $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)
clang:
	clang++ -I../../ Main.cpp -o main -O2 $(CLANG_FLAGS) $(FLEET_INCLUDE) $(FLEET_LIBS)
profiled:
	g++ -I../../ Main.cpp -o main -g -pg -fprofile-arcs -ftest-coverage $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)
conda:
	x86_64-conda-linux-gnu-gcc -I../../ Main.cpp -o main -O2 $(FLEET_FLAGS) $(FLEET_INCLUDE) -I  /usr/include/eigen3/ $(FLEET_LIBS)
