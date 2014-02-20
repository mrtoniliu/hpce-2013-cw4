CPPFLAGS=-I include -Wall -std=c++11 -O2 -lOpenCL
CXX=clang++

EXECUTABLES = src/test_opencl src/make_world src/step_world src/render_world

all: $(EXECUTABLES)

src/make_world: src/heat.o
src/step_world: src/heat.o
src/render_world: src/heat.o

clean:
	rm -f $(EXECUTABLES)
	rm -f src/*.o
