# Sculpted Creation Ring: compiled SDF -> mesh -> casting files -> renders.
#   make            build the mesher
#   make export     raw mesh at H mm voxels, repaired + decimated to TARGET triangles
#   make renders    product shots of the exported mesh
#   make check      bore clearance and mesh sanity
CXX ?= g++
CXXFLAGS ?= -O3 -march=native -fopenmp -std=c++17 -Wall -Wno-unused-function  # no -ffast-math: it breaks fmod/atan2 consistency and plants spikes
PY ?= /opt/bpyenv/bin/python
H ?= 0.022
TARGET ?= 1200000
OUT ?= ../export
BUILD ?= build
HDRS = model.h sdf.h common.h band.h human.h robot.h

all: mesher clearance

mesher: mesher.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ mesher.cpp

clearance: clearance.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ clearance.cpp

$(BUILD)/raw.ply: mesher
	mkdir -p $(BUILD)
	./mesher --h $(H) --part all --ply $@

export: $(BUILD)/raw.ply
	$(PY) finish_mesh.py $(BUILD)/raw.ply $(BUILD)/creation_ring_sculpted --target $(TARGET)
	cp $(BUILD)/creation_ring_sculpted.stl $(BUILD)/creation_ring_sculpted.3mf $(OUT)/

check: clearance
	./clearance
	$(PY) check_mesh.py $(BUILD)/creation_ring_sculpted.ply

renders:
	$(PY) render.py --mesh $(BUILD)/creation_ring_sculpted.ply --out $(OUT)/creation_ring_sculpted.png --res 1600x1600 --samples 128 --view photo

clean:
	rm -rf mesher clearance $(BUILD)

.PHONY: all export check renders clean
