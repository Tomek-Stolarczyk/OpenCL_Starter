#include "stdafx.h"
#include "OpenCLBasic.h"
#include <fstream>
#include <iostream>
#include <time.h>


OpenCLBasic::OpenCLBasic(const char *filePath)
{
	std::string tmp = std::string(filePath);
	const char* cnsttemp = tmp.c_str();
	KernelFile = cnsttemp;
	if (!SetupOpenCL())
		return;
}


OpenCLBasic::~OpenCLBasic()
{
	cl_uint status;
	status = clReleaseKernel(kernel);
	status = clReleaseProgram(program);
	status = clReleaseMemObject(inputBuffer);
	status = clReleaseMemObject(outputBuffer);
	for (unsigned int i = 0; i < deviceCount; i++)
		status = clReleaseCommandQueue(commandQueues[i]);
	status = clReleaseContext(context);
	delete[] deviceIds;
}

bool OpenCLBasic::SetupOpenCL()
{
	/*Step1: Getting platforms and choose the first one.*/
	cl_uint numPlatforms;
	cl_int	status;
	status = clGetPlatformIDs(NULL, NULL, &numPlatforms);
	CHECK_ERROR(status, "clGetPlatformID's");

	std::cout << "Total number of platforms found: " << numPlatforms << std::endl;

	cl_platform_id* totalPlatforms = new cl_platform_id[numPlatforms];

	status = clGetPlatformIDs(numPlatforms, totalPlatforms, NULL);
	CHECK_ERROR(status, "clGetPlatformIDs");

	char pbuf[100];
	for (unsigned int i = 0; i < numPlatforms; i++)
	{
		status = clGetPlatformInfo(totalPlatforms[i],
			CL_PLATFORM_VENDOR,
			sizeof(pbuf),
			pbuf,
			NULL);
		CHECK_ERROR(status, "clGetPlatformInfo");

		std::cout << "Platform num  " << i << ": " << pbuf << std::endl;
	}

	platform = totalPlatforms[0];
	status = clGetPlatformInfo(platform,
		CL_PLATFORM_VENDOR,
		sizeof(pbuf),
		pbuf,
		NULL);

	CHECK_ERROR(status, "clGetPlatformInfo");

	std::cout << "Using Platform Vendor " << pbuf << " for Hardware acceleration." << std::endl;

	delete[] totalPlatforms;

	cl_device_type dType = CL_DEVICE_TYPE_GPU;

	// Get number of devices available
	deviceCount = 0;
	status = clGetDeviceIDs(platform, dType, 0, NULL, &deviceCount);
	CHECK_ERROR(status, "clGetDeviceIDs");

	std::cout << "Total GPU's found: " << deviceCount << std::endl;

	deviceIds = new cl_device_id[deviceCount];

	status = clGetDeviceIDs(platform, dType, deviceCount, deviceIds, NULL);
	CHECK_ERROR(status, "clGetDeviceIDs");

	// Print device index and device names
	for (cl_uint i = 0; i < deviceCount; ++i)
	{
		char deviceName[1024];
		status = clGetDeviceInfo(deviceIds[i], CL_DEVICE_NAME, sizeof(deviceName),
			deviceName, NULL);
		CHECK_ERROR(status, "clGetDeviceInfo");

		std::cout << "Device num " << i << ": " << deviceName
			<< " with ID " << deviceIds[i] << std::endl;
	}

	cl_context_properties cps[3] =
	{
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)platform,
		0
	};

	context = clCreateContextFromType(cps, dType, NULL, NULL, &status);
	CHECK_ERROR(status, "clCreateContextFromType");

	size_t deviceListSize;
	status = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceListSize);
	if ((int)(deviceListSize / sizeof(cl_device_id)) != deviceCount)
	{
		std::cout << "device list size from context different from total device count;" << std::endl;
		return false;
	}


	//Setup the command queues.
	commandQueues = new cl_command_queue[deviceCount];
	for (unsigned int i = 0; i < deviceCount; i++)
	{
		commandQueues[i] = clCreateCommandQueue(context, deviceIds[i], 0, &status);
		CHECK_ERROR(status, "clCreateCommandQueue");
	}


	CompileCode();

	kernel = clCreateKernel(program, "helloworld", &status);
	CHECK_ERROR(status, "clCreateKernel");


	return 0;
}

bool OpenCLBasic::CompileCode()
{
	cl_int status;

	std::string sourceStr;
	if (!convertToString(KernelFile, sourceStr))
		return false;

	const char *source = sourceStr.c_str();

	size_t sourceSize[] = { strlen(source) };
	program = clCreateProgramWithSource(context, 1, &source, sourceSize, &status);
	CHECK_ERROR(status, "clCreateProgramWithSource");

	status = clBuildProgram(program, deviceCount, deviceIds, { "-Werror" }, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		if (status == CL_BUILD_PROGRAM_FAILURE)
		{
			cl_int logStatus;
			char *buildLog = NULL;
			size_t buildLogSize = 0;
			logStatus = clGetProgramBuildInfo(
				program,
				deviceIds[0],
				CL_PROGRAM_BUILD_LOG,
				buildLogSize,
				buildLog,
				&buildLogSize);
			CHECK_ERROR(logStatus, "clGetProgramBuildInfo failed.");
			buildLog = (char*)malloc(buildLogSize);
			memset(buildLog, 0, buildLogSize);
			logStatus = clGetProgramBuildInfo(
				program,
				deviceIds[0],
				CL_PROGRAM_BUILD_LOG,
				buildLogSize,
				buildLog,
				NULL);
			std::cout << " \n\t\t\tBUILD LOG\n";
			std::cout << " ************************************************\n";
			std::cout << buildLog << std::endl;
			std::cout << " ************************************************\n";
			free(buildLog);
		}
		CHECK_ERROR(status, "clBuildProgram failed.");
	}

	return true;
}

void OpenCLBasic::ExecuteKernel(int len)
{
	time_t timer, timer2;
	time(&timer);

	std::cout << "executing kernel on 1 device" << std::endl;
	size_t global_size = 1;
	size_t local_size = 1;
	clEnqueueNDRangeKernel(commandQueues[0], kernel, 1, NULL, &global_size, &local_size, 0 ,NULL, NULL);

	int *output_buffer = (int *)malloc(sizeof(int)*len);
	memset(output_buffer, 0, sizeof(int)*len);
	clEnqueueReadBuffer(commandQueues[0], outputBuffer, CL_TRUE, 0, len * sizeof(int), output_buffer, 0, NULL, NULL);

	std::cout << output_buffer[1] << ".";
	for (int i = 2; i<len; i++)
		std::cout << output_buffer[i];
	std::cout << "\n Done!\n";

	time(&timer2);
	std::cout << "calculated pi in " << difftime(timer2, timer) << " seconds\n";

}

bool OpenCLBasic::AllocateBuffers(int len)
{
	cl_int	status;
	cl_mem tempBuffer;
	std::cout << "allocating OpenCL buffers" << std::endl;

	int *input_len = (int *)malloc(sizeof(int));
	*input_len = len;
	int *output_buffer = (int *)malloc(sizeof(int)*len);
	memset(output_buffer, 0, sizeof(int)*len);
	int *Temp_buffer = (int *)malloc(sizeof(int)*len);
	memset(Temp_buffer, 0, sizeof(int)*len);

	inputBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(int), (void *)input_len, &status);
	CHECK_ERROR(status, "clCreateBuffer");
	outputBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int)*len, (void *)output_buffer, &status);
	CHECK_ERROR(status, "clCreateBuffer");
	tempBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int)*len, (void *)Temp_buffer, &status);
	CHECK_ERROR(status, "clCreateBuffer");


	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inputBuffer);
	CHECK_ERROR(status, "clSetKernelArg");
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&outputBuffer);
	CHECK_ERROR(status, "clSetKernelArg");
	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&tempBuffer);
	CHECK_ERROR(status, "clSetKernelArg");

	return true;
}


/* convert the kernel file into a string */
bool OpenCLBasic::convertToString(const char *filename, std::string& s)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if (f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size + 1];
		if (!str)
		{
			f.close();
			return 0;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return true;
	}
	std::cout << "Error: failed to open file " << filename << std::endl;
	return false;
}

