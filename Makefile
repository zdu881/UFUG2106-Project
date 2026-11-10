CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
.PHONY: all clean
all: build/baseline build/synth
build/baseline: src/baseline.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@
clean:
	rm -f build/baseline build/synth
build/synth: src/synth.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@
