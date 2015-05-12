#pragma once
#include <string>
#include <CL\cl.h>


#define CHECK_ERROR(actual, msg) \
if (actual != CL_SUCCESS) \
{ \
	std::cout << "Error: " << msg << " call failed with code: " << actual << std::endl; \
	std::cout << "Location : " << __FILE__ << ":" << __LINE__ << std::endl; \
	return false; \
}

class OpenCLBasic
{
public:
	OpenCLBasic(const char *KernelPath);
	~OpenCLBasic();

	void ExecuteKernel(int len);
	bool AllocateBuffers(int len);


protected: // Variables
	cl_platform_id platform;
	cl_device_id* deviceIds;
	cl_uint deviceCount; 
	cl_context context;
	cl_command_queue* commandQueues;
	cl_program program;
	const char* KernelFile;
	
	cl_kernel kernel;

	cl_mem inputBuffer;
	cl_mem outputBuffer;


protected: // Methods
	bool SetupOpenCL();
	bool CompileCode();


private: // Helper
	bool convertToString(const char *filename, std::string& s);

};
