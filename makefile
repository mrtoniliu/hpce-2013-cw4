CXX = clang++
CPPFLAGS = -I include -O2 -Wall -std=c++11
LDFLAGS = -L /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk/System/Library/Frameworks/OpenCL.framework/Versions/A/Libraries
OPENCL_DIR = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk/System/Library/Frameworks/OpenCL.framework/Versions/A
LDLIBS = -lOpenCL

CPPFLAGS += -I $(OPENCL_DIR)

bin/test_opencl: src/test_opencl.cpp
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) $^ -o $@ -framework OpenCL

bin/render_world: src/render_world.cpp src/heat.cpp
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) $^ -o $@ 

bin/step_world: src/step_world.cpp src/heat.cpp
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) $^ -o $@ 

bin/make_world: src/make_world.cpp src/heat.cpp
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) $^ -o $@ 

bin/step_world_v1_lambda: src/yl10313/step_world_v1_lambda.cpp src/heat.cpp
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) $^ -o $@ 

bin/step_world_v2_function: src/yl10313/step_world_v2_function.cpp src/heat.cpp
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) $^ -o $@ 

bin/step_world_v3_opencl: src/yl10313/step_world_v3_opencl.cpp src/heat.cpp
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) $^ -o $@ -framework OpenCL

bin/step_world_v4_double_buffered: src/yl10313/step_world_v4_double_buffered.cpp src/heat.cpp
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) $^ -o $@ -framework OpenCL

bin/step_world_v5_packed_properties: src/yl10313/step_world_v5_packed_properties.cpp src/heat.cpp
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) $^ -o $@ -framework OpenCL


all: bin/render_world bin/step_world \
	bin/make_world bin/test_opencl \
	bin/step_world_v1_lambda\
	bin/step_world_v2_function \
	bin/step_world_v3_opencl \
	bin/step_world_v4_double_buffered \
	bin/step_world_v5_packed_properties



clean: 
	rm -f bin/*.o
	rm -f bin/*
	rm -f src/*.o
	rm -f tmp/*
	rm -rf tmp
	rm -rf bin
	rm -f *.bmp
	rm -f *.txt


diffv1: 
	-mkdir -p tmp
	./bin/make_world 100 0.1 | ./bin/step_world 0.1 100000 > tmp/temp0
	./bin/make_world 100 0.1 | ./bin/step_world_v1_lambda 0.1 100000 > tmp/temp1
	diff tmp/temp0 tmp/temp1

diffv2:
	-mkdir -p tmp
	./bin/make_world 100 0.1 | ./bin/step_world 0.1 100000 > tmp/temp0
	./bin/make_world 100 0.1 | ./bin/step_world_v2_function 0.1 100000 > tmp/temp2
	diff tmp/temp0 tmp/temp2

diffv3:
	-mkdir -p tmp
	./bin/make_world 100 0.1 | ./bin/step_world 0.1 100000 > tmp/temp0
	./bin/make_world 100 0.1 | ./bin/step_world_v3_opencl 0.1 100000 > tmp/temp3
	diff tmp/temp0 tmp/temp3

diffv4:
	-mkdir -p tmp
	./bin/make_world 100 0.1 | ./bin/step_world 0.1 100000 > tmp/temp0
	./bin/make_world 100 0.1 | ./bin/step_world_v4_double_buffered 0.1 100000 > tmp/temp4
	diff tmp/temp0 tmp/temp4

diffv5:
	-mkdir -p tmp
	./bin/make_world 100 0.1 | ./bin/step_world 0.1 100000 > tmp/temp0
	./bin/make_world 100 0.1 | ./bin/step_world_v5_packed_properties 0.1 100000 > tmp/temp5
	diff tmp/temp0 tmp/temp5

