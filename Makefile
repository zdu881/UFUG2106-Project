CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
.PHONY: all clean stage2 stage2-source
all: build/baseline build/synth
build/%: src/%.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@
clean:
	rm -f build/baseline build/synth build/synth_stage2
stage2: build/synth_stage2

build/synth_stage2: $(wildcard src/round51/*.hpp)

stage2-source: artifacts/UFUG2106_Group2_Stage2_Source.zip

artifacts/UFUG2106_Group2_Stage2_Source.zip: src/synth_stage2.cpp src/round51 $(wildcard src/round51/*) releases/stage2/README.md
	mkdir -p artifacts
	python3 -m zipfile -c $@ src/synth_stage2.cpp src/round51 releases/stage2/README.md
