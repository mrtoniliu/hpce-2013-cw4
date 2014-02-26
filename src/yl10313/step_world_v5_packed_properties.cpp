#include "heat.hpp"

#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <memory>
#include <cstdio>

// OpenCL define:
#define __CL_ENABLE_EXCEPTIONS
#define __CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include "CL/cl.hpp"


// kernel things
#include <fstream>
#include <streambuf>



namespace hpce{

namespace yl10313{


	std::string LoadSource(const char *fileName)
	{
		// Don't forget to change your_login here
		std::string baseDir="src/yl10313";
		if(getenv("HPCE_CL_SRC_DIR")){
			baseDir=getenv("HPCE_CL_SRC_DIR");
		}
		
		std::string fullName=baseDir+"/"+fileName;
		
		std::ifstream src(fullName, std::ios::in | std::ios::binary);
		if(!src.is_open())
			throw std::runtime_error("LoadSource : Couldn't load cl file from '"+fullName+"'.");
		
		return std::string(
			(std::istreambuf_iterator<char>(src)), // Node the extra brackets.
            std::istreambuf_iterator<char>()
		);
	}

void StepWorldV5PackedProperties(world_t &world, float dt, unsigned n)
{
	// Choose a platform
	std::vector<cl::Platform> platforms;
 	cl::Platform::get(&platforms);
 	if(platforms.size()==0)
     	throw std::runtime_error("No OpenCL platforms found.");

    std::cerr<<"Found "<<platforms.size()<<" platforms\n";
	for(unsigned i=0;i<platforms.size();i++){
		std::string vendor=platforms[0].getInfo<CL_PLATFORM_VENDOR>();
		std::cerr<<"  Platform "<<i<<" : "<<vendor<<"\n";
	}

	// use ENV to choose platform
	int selectedPlatform=0;
	if(getenv("HPCE_SELECT_PLATFORM")){
		selectedPlatform=atoi(getenv("HPCE_SELECT_PLATFORM"));
	}
	std::cerr<<"Choosing platform "<<selectedPlatform<<"\n";
	cl::Platform platform=platforms.at(selectedPlatform);


	// Choose a device
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
		if(devices.size()==0){
			throw std::runtime_error("No opencl devices found.\n");
	}
	std::cerr<<"Found "<<devices.size()<<" devices\n";
	for(unsigned i=0;i<devices.size();i++){
		std::string name=devices[i].getInfo<CL_DEVICE_NAME>();
		std::cerr<<"  Device "<<i<<" : "<<name<<"\n";
	}

	int selectedDevice=0;
	if(getenv("HPCE_SELECT_DEVICE")){
   		selectedDevice=atoi(getenv("HPCE_SELECT_DEVICE"));
	}
	std::cerr<<"Choosing device "<<selectedDevice<<"\n";
	cl::Device device=devices.at(selectedDevice);


	// create OpenCL context
	cl::Context context(devices);

	// Load cl source
	std::string kernelSource=hpce::yl10313::LoadSource("step_world_v5_kernel.cl");
	
	cl::Program::Sources sources;	// A vector of (data,length) pairs
	sources.push_back(std::make_pair(kernelSource.c_str(), kernelSource.size()+1));	// push on our single string
	
	cl::Program program(context, sources);
	try{
		program.build(devices);
	}catch(...){
		for(unsigned i=0;i<devices.size();i++){
			std::cerr<<"Log for device "<<devices[i].getInfo<CL_DEVICE_NAME>()<<":\n\n";
			std::cerr<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[i])<<"\n\n";
		}
		throw;
	}



	// Declare the variables
	unsigned w=world.w, h=world.h;
	float outer=world.alpha*dt;		// We spread alpha to other cells per time
	float inner=1-outer/4;				// Anything that doesn't spread stays


	size_t cbBuffer=4*world.w*world.h;
	cl::Buffer buffProperties(context, CL_MEM_READ_ONLY, cbBuffer);
	cl::Buffer buffState(context, CL_MEM_READ_WRITE, cbBuffer);
	cl::Buffer buffBuffer(context, CL_MEM_READ_WRITE, cbBuffer);

	// Set kernel parameter
	cl::Kernel kernel(program, "kernel_xy");

	// bind the kernel arguments
	kernel.setArg(0, buffState);
	kernel.setArg(1, inner);
	kernel.setArg(2, outer);
	kernel.setArg(3, buffBuffer);
	kernel.setArg(4, buffProperties);

	// Create a command queue
	cl::CommandQueue queue(context, device);

	std::vector<uint32_t> packed(world.properties.begin(), world.properties.end());
	for (unsigned y = 0; y < h; y++)
	{
		for (unsigned x = 0; x < w; x++)
		{
			unsigned index=y*w + x;
			// packed[index]=world.properties[index];
			if (!packed[index]){
				if ( !( packed[index-w] & Cell_Insulator) )
				{
					packed[index] += 0x4;
				}

				if ( !( packed[index+w] & Cell_Insulator) )
				{
					packed[index] += 0x8;
				}

				if ( !( packed[index-1] & Cell_Insulator) )
				{
					packed[index] += 0x10;
				}

				if ( !( packed[index+1] & Cell_Insulator) )
				{
					packed[index] += 0x20;
				}
			}
				
		}
	}

		// setting up the iteration space
	// Always start iterations at x=0, y=0
	cl::NDRange offset(0, 0);		

	// Global size must match the original loops	
	cl::NDRange globalSize(w, h);	
	// We don't care about local size
	cl::NDRange localSize=cl::NullRange;	

	// Copy the fixed data over to the GPU
	// Here the properties is constant array across all iteration
	// "buffProperties" : is the GPU buffer name
	// "CL_TRUE" : A flag to indicate we want synchronous operation, so the function will not complete until the copy has finished. 
	// "0" : The starting offset within the GPU buffer.
	// "cbBuffer" : The number of bytes to copy.
	// "&world.properties[0]"" : Pointer to the data in host memory (DDR in this case) we want to copy.
	queue.enqueueWriteBuffer(buffProperties, CL_TRUE, 0, cbBuffer, &packed[0]);

		
	// } // end of for(t...
	queue.enqueueWriteBuffer(buffState, CL_TRUE, 0, cbBuffer, &world.state[0]);

	for (int t = 0; t < n; ++t)
	{

		queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize);

		queue.enqueueBarrier();

		// All cells have now been calculated and placed in buffer, so we replace
		// the old state with the new state
		std::swap(buffState, buffBuffer);

		kernel.setArg(0, buffState);
		kernel.setArg(3, buffBuffer);

		// We have moved the world forwards in time
		world.t += dt;

	} // end of for(t...

	queue.enqueueReadBuffer(buffBuffer, CL_TRUE, 0, cbBuffer, &world.state[0]);
}

}; // namepspace yl10313
	
}; // namepspace hpce


int main(int argc, char *argv[])
{
	float dt=0.1;
	unsigned n=1;
	bool binary=false;
	
	if(argc>1){
		dt=strtof(argv[1], NULL);
	}
	if(argc>2){
		n=atoi(argv[2]);
	}
	if(argc>3){
		if(atoi(argv[3]))
			binary=true;
	}
	
	try{
		hpce::world_t world=hpce::LoadWorld(std::cin);
		std::cerr<<"Loaded world with w="<<world.w<<", h="<<world.h<<std::endl;
		
		std::cerr<<"Stepping by dt="<<dt<<" for n="<<n<<std::endl;
		hpce::yl10313::StepWorldV5PackedProperties(world, dt, n);
		
		hpce::SaveWorld(std::cout, world, binary);
	}catch(const std::exception &e){
		std::cerr<<"Exception : "<<e.what()<<std::endl;
		return 1;
	}
		
	return 0;
}

