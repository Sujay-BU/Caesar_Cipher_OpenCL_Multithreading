#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define NUMBER_OF_THREADS 4

char* text; //Stores the text to be encrypted
const int shift = 100; //The key to be shifted by
char* programSource; //Contains the kernel text

//Structure that stores the structure of the threads
struct thread_structure {
	int start, stop;
};

//Threads function
DWORD WINAPI caesarCipherThreads(LPVOID parameter) {
	struct thread_structure* threads = (struct thread_structure*) parameter;
	for (int i = threads->start; i < threads->stop; i++)
		(isupper(text[i]) != 0 ? (text[i] = (char)(((int)text[i] + shift - 65) % 26 + 65)) : (islower(text[i]) != 0 ? (text[i] = (char)(((int)text[i] + shift - 97) % 26 + 97)) : NULL)); //Performs the shift even if the key is larger than 26 based on if the character passed if lowercase or uppercase.
}


//Function to read kernels from another file
char* getOpenCLProgramFromFile(const char* fn)
{
	FILE* programFile;
	size_t program_size;
	programFile = fopen(fn, "rb");
	if (!programFile) {
		printf("Failed to load program\n");
		exit(1);
	}
	fseek(programFile, 0, SEEK_END);
	program_size = ftell(programFile);
	rewind(programFile);
	programSource = (char*)malloc(program_size + 1);
	programSource[program_size] = '\0';
	fread(programSource, sizeof(char), program_size, programFile);
	fclose(programFile);
	return programSource;
}

//Callback function
void CL_CALLBACK callback_function(cl_event event, cl_int status, void* data) {
	printf("\n%s\n", (char*)data);
}

int main(void)
{
	text = (char*)malloc(strlen("hello there") + 1);
	strcpy(text, "hello there");
	const unsigned int length = strlen(text) + 1;

	int choice;
	printf("Enter 1 to use the pthread method,\n");
	printf("Enter 2 to use the openCL method,\n");
	printf("Enter 3 to use the 50%% pthread and openCL method,\n");
	printf("Enter -1 to exit:\n");
	scanf("%d", &choice);

	if (choice == 1) {
		printf("\n");
		HANDLE tids[NUMBER_OF_THREADS];
		struct thread_structure threads[NUMBER_OF_THREADS];

		for (int i = 0; i < NUMBER_OF_THREADS; i++) {
			threads[i].start = i * (length - 1) / NUMBER_OF_THREADS;
			threads[i].stop = (i + 1) * (length - 1) / NUMBER_OF_THREADS;
		}

		printf("Text before encryption: %s\n", text);

		for (int i = 0; i < NUMBER_OF_THREADS; i++)	//For loop to create threads
			tids[i] = CreateThread(NULL, NULL, caesarCipherThreads, (LPVOID)&threads[i], NULL, NULL);

		for (int i = 0; i < NUMBER_OF_THREADS; i++)	//For loop to wait for threads
			WaitForSingleObject(tids[i], INFINITE);

		printf("Text after encryption: %s\n", text);
	}
	else if (choice == 2) {
		printf("\n");

		cl_int openClReturnCode;
		cl_platform_id platform_id;
		cl_uint numberOfPlatforms;

		/* check first platform id */
		clGetPlatformIDs(1, &platform_id, &numberOfPlatforms);

		/* get 1st GPU device id */
		cl_device_id device_id;
		clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);

		/* create a context */
		cl_context context = clCreateContext(0, 1, &device_id, NULL, NULL, &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating a Context: [OK]\n" : "Creating a Context: [FAILED] with code %d\n", openClReturnCode);

		/* create a queue */
		cl_command_queue commandsQueue = clCreateCommandQueue(context, device_id, 0, &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating a Command Queue: [OK]\n" : "Creating a Command Queue: [FAILED] with code %d\n", openClReturnCode);

		/* create program from source */
		const char* programs = getOpenCLProgramFromFile("OpenCLProgram.cl");
		cl_program program = clCreateProgramWithSource(context, 1, (const char**)&programs, NULL, &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating Program from Source: [OK]\n" : "Creating Program from Source: [FAILED] with code %d\n", openClReturnCode);


		/* build the program  */
		openClReturnCode = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
		printf(openClReturnCode == 0 ? "Building Program: [OK]\n" : "Building Program: [FAILED] with code %d\n", openClReturnCode);

		/* create kernel  */
		cl_kernel kernel = clCreateKernel(program, "caesarCipher", &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating first Kernel from Program: [OK]\n" : "Creating first Kernel from Program: [FAILED] with code %d\n", openClReturnCode);

		/* create device memory for input and output */
		cl_mem input = clCreateBuffer(context, CL_MEM_READ_WRITE, length, NULL, &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating Buffer: [OK]\n" : "Creating Buffer: [FAILED] with code %d\n", openClReturnCode);

		cl_mem result = clCreateBuffer(context, CL_MEM_READ_WRITE, length, NULL, &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating Buffer: [OK]\n" : "Creating Buffer: [FAILED] with code %d\n", openClReturnCode);

		printf("------------------------------------------------------------------------------------------------------------------------");

		/* transfer host data to device memory */
		openClReturnCode = clEnqueueWriteBuffer(commandsQueue, input, CL_TRUE, 0, length, text, 0, NULL, NULL);
		printf(openClReturnCode == 0 ? "Copying Data to Buffer: [OK]\n" : "Copying Data to Buffer: [FAILED] with code %d\n", openClReturnCode);

		/* set kernel's arguments */
		openClReturnCode = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input); /* for input  */
		printf(openClReturnCode == 0 ? "Setting Parameter0 (__global char* input): [OK]\n" : "Setting Parameter0 (__global char* input): [FAILED] with code %d\n", openClReturnCode);

		openClReturnCode = clSetKernelArg(kernel, 1, sizeof(cl_mem), &result); /* for input  */
		printf(openClReturnCode == 0 ? "Setting Parameter1 (__global char* result): [OK]\n" : "Setting Parameter1 (__global char* result): [FAILED] with code %d\n", openClReturnCode);

		openClReturnCode = clSetKernelArg(kernel, 2, sizeof(shift), &shift); /* for count/size of input/output */
		printf(openClReturnCode == 0 ? "Setting Parameter2 (const int shift): [OK]\n" : "Setting Parameter2 (const int shift): [FAILED] with code %d\n", openClReturnCode);

		openClReturnCode = clSetKernelArg(kernel, 3, sizeof(length), &length); /* for count/size of input/output */
		printf(openClReturnCode == 0 ? "Setting Parameter3 (const unsigned int length): [OK]\n" : "Setting Parameter3 (const unsigned int length): [FAILED] with code %d\n", openClReturnCode);

		//Set local size
		size_t local = 4;

		/* set global size */
		size_t global = ceil(length / (float)local) * local;

		/* enqueue kernel (which asks the device to run all work items) */
		openClReturnCode = clEnqueueNDRangeKernel(commandsQueue, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
		printf(openClReturnCode == 0 ? "Started the Work: [OK]\n" : "Started the Work: [FAILED] with code %d\n", openClReturnCode);

		/* wait for device to finish */
		clFinish(commandsQueue);

		printf("------------------------------------------------------------------------------------------------------------------------");
		/* read output from device memeory to host memory */
		printf("The original text is: %s\n", text);
		clEnqueueReadBuffer(commandsQueue, result, CL_TRUE, 0, length, text, 0, NULL, NULL);
		printf("The encrypted text is: %s\n", text);

		//Releasing memory
		clReleaseDevice(device_id);
		clReleaseMemObject(input);
		clReleaseMemObject(result);
		clReleaseProgram(program);
		clReleaseKernel(kernel);
		clReleaseCommandQueue(commandsQueue);
		clReleaseContext(context);
	} 
	else if (choice == 3) {
		printf("\n");

		cl_int openClReturnCode;
		cl_platform_id platform_id;
		cl_uint numberOfPlatforms;
		cl_event gpuWorkEvent;

		/* check first platform id */
		clGetPlatformIDs(1, &platform_id, &numberOfPlatforms);

		/* get 1st GPU device id */
		cl_device_id device_id;
		clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);

		/* create a context */
		cl_context context = clCreateContext(0, 1, &device_id, NULL, NULL, &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating a Context: [OK]\n" : "Creating a Context: [FAILED] with code %d\n", openClReturnCode);

		/* create a queue */
		cl_command_queue commandsQueue = clCreateCommandQueue(context, device_id, 0, &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating a Command Queue: [OK]\n" : "Creating a Command Queue: [FAILED] with code %d\n", openClReturnCode);

		/* create program from source */
		const char* programs = getOpenCLProgramFromFile("OpenCLProgram.cl");
		cl_program program = clCreateProgramWithSource(context, 1, (const char**)&programs, NULL, &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating Program from Source: [OK]\n" : "Creating Program from Source: [FAILED] with code %d\n", openClReturnCode);

		/* build the program  */
		openClReturnCode = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
		printf(openClReturnCode == 0 ? "Building Program: [OK]\n" : "Building Program: [FAILED] with code %d\n", openClReturnCode);

		/* create kernel  */
		cl_kernel kernel = clCreateKernel(program, "caesarCipherOne", &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating first Kernel from Program: [OK]\n" : "Creating first Kernel from Program: [FAILED] with code %d\n", openClReturnCode);

		/* create device memory for input and output */
		cl_mem input = clCreateBuffer(context, CL_MEM_READ_WRITE, length, NULL, &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating Buffer: [OK]\n" : "Creating Buffer: [FAILED] with code %d\n", openClReturnCode);
		
		const unsigned int new_length = length * 0.5;
		cl_mem result = clCreateBuffer(context, CL_MEM_READ_WRITE, length-new_length, NULL, &openClReturnCode);
		printf(openClReturnCode == 0 ? "Creating Buffer: [OK]\n" : "Creating Buffer: [FAILED] with code %d\n", openClReturnCode);

		printf("------------------------------------------------------------------------------------------------------------------------");

		/* transfer host data to device memory */
		openClReturnCode = clEnqueueWriteBuffer(commandsQueue, input, CL_TRUE, 0, length, text, 0, NULL, NULL);
		printf(openClReturnCode == 0 ? "Copying Data to Buffer: [OK]\n" : "Copying Data to Buffer: [FAILED] with code %d\n", openClReturnCode);

		/* set kernel's arguments */
		openClReturnCode = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input); /* for input  */
		printf(openClReturnCode == 0 ? "Setting Parameter0 (__global char* input): [OK]\n" : "Setting Parameter0 (__global char* input): [FAILED] with code %d\n", openClReturnCode);

		openClReturnCode = clSetKernelArg(kernel, 1, sizeof(cl_mem), &result); /* for input  */
		printf(openClReturnCode == 0 ? "Setting Parameter1 (__global char* result): [OK]\n" : "Setting Parameter1 (__global char* result): [FAILED] with code %d\n", openClReturnCode);

		openClReturnCode = clSetKernelArg(kernel, 2, sizeof(shift), &shift); /* for count/size of input/output */
		printf(openClReturnCode == 0 ? "Setting Parameter2 (const int shift): [OK]\n" : "Setting Parameter2 (const int shift): [FAILED] with code %d\n", openClReturnCode);

		openClReturnCode = clSetKernelArg(kernel, 3, sizeof(length), &length); /* for count/size of input/output */
		printf(openClReturnCode == 0 ? "Setting Parameter3 (const unsigned int length): [OK]\n" : "Setting Parameter3 (const unsigned int length): [FAILED] with code %d\n", openClReturnCode);

		openClReturnCode = clSetKernelArg(kernel, 4, sizeof(new_length), &new_length); /* for count/size of input/output */
		printf(openClReturnCode == 0 ? "Setting Parameter4 (const unsigned int new_length): [OK]\n" : "Setting Parameter4 (const unsigned int new_length): [FAILED] with code %d\n", openClReturnCode);

		//Set local size
		size_t local = 4;

		/* set global size */
		size_t global = ceil(length / (float)local) * local;

		/* enqueue kernel (which asks the device to run all work items) */
		openClReturnCode = clEnqueueNDRangeKernel(commandsQueue, kernel, 1, NULL, &global, &local, 0, NULL, &gpuWorkEvent);
		printf(openClReturnCode == 0 ? "Started the Work: [OK]\n" : "Started the Work: [FAILED] with code %d\n", openClReturnCode);
		openClReturnCode = clSetEventCallback(gpuWorkEvent, CL_COMPLETE, &callback_function, "The GPU has completed it's work!");
		if (openClReturnCode != 0) {
			printf("Could not set callback function");
		}

		HANDLE tids[NUMBER_OF_THREADS];
		struct thread_structure threads[NUMBER_OF_THREADS];

		for (int i = 0; i < NUMBER_OF_THREADS; i++) {
			threads[i].start = i * new_length / NUMBER_OF_THREADS; //be careful here
			threads[i].stop = (i + 1) * new_length / NUMBER_OF_THREADS;
		}

		printf("------------------------------------------------------------------------------------------------------------------------\n");

		printf("Text before encryption: %s\n", text);

		for (int i = 0; i < NUMBER_OF_THREADS; i++)	//For loop to create threads
			tids[i] = CreateThread(NULL, NULL, caesarCipherThreads, (LPVOID)&threads[i], NULL, NULL);

		for (int i = 0; i < NUMBER_OF_THREADS; i++)	//For loop to wait for threads
			WaitForSingleObject(tids[i], INFINITE);

		/* wait for device to finish */
		clFinish(commandsQueue);

	//-----------------------------------------------------------------------------------------------------------------------------
		/* read output from device memeory to host memory */
		char* res;
		res = (char*)malloc(length-new_length);
		clEnqueueReadBuffer(commandsQueue, result, CL_TRUE, 0, length-new_length, res, 0, NULL, NULL);
		
		for (int i = new_length; i < (length - 1); i++) {
			text[i] = res[i - new_length];
		}

		printf("The encrypted text is: %s\n", text);

		//Releasing memory
		clReleaseEvent(gpuWorkEvent);
		clReleaseDevice(device_id);
		clReleaseMemObject(input);
		clReleaseMemObject(result);
		clReleaseProgram(program);
		clReleaseKernel(kernel);
		clReleaseCommandQueue(commandsQueue);
		clReleaseContext(context);
		free(res);
	}
	else {
		printf("\nExiting the program!\n");
	}

	free(text);
	free(programSource);
	return 0;
}