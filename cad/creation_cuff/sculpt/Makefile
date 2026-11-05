CXX ?= g++
CXXFLAGS ?= -O3 -march=native -ffast-math -fopenmp -std=c++17 -Wall -Wno-unused-function
mesher: mesher.cpp model.h sdf.h common.h band.h human.h robot.h
	$(CXX) $(CXXFLAGS) -o $@ mesher.cpp
clean:
	rm -f mesher
