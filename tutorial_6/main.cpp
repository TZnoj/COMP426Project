#define CL_TARGET_OPENCL_VERSION 200
#define __CL_ENABLE_EXCEPTIONS

// has to be first! 
#include "Dependencies\glew\include\GL\glew.h"

// define includes
#include "definitions.h"

// system includes
#include <ctime>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <stdexcept>
//#include <vector>
//#include <exception>

// OpenGL includes
#if defined (__APPLE__) || defined(MACOSX)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <GLUT/glut.h>
#ifndef glutCloseFunc
#define glutCloseFunc glutWMCloseFunc
#endif
#else
#include "Dependencies\freeglut\include\GL\freeglut.h"

#endif

// OpenCL includes
#include "Dependencies\OpenCL-SDK\external\OpenCL-Headers\CL\opencl.h"

// my includes
#include "kernels.h"


// variables
cl_uint2 m_positions[WIDTH * HEIGHT];
cl_uint2* p_pos = m_positions;

cl_uchar3 m_colors[WIDTH * HEIGHT];
cl_uchar3* p_col = m_colors;

cl_uchar3 m_colors_read[WIDTH * HEIGHT];
cl_uchar3* p_read = m_colors_read;

cl_ulong m_rand_ulong[10];
cl_ulong* p_ulong_rand = m_rand_ulong;

cl_ulong current_time = 0;
cl_ulong* p_current_time = &current_time;

cl_int err; // for returning errors

cl_uint num_platforms;
cl_platform_id platform_ids[1];

cl_uint num_devices;
cl_device_id device_ids[2]; // only two devices expected 0 is GPU 1 is CPU

cl_context m_context; // 1 platform, 2 devices on my laptop

cl_command_queue queue_gpu, queue_cpu; // for a laptop with integrated graphics

cl_program program;

cl_kernel generate_random;

cl_mem color_buffer, color_copy_buffer, rand_input_buffer, time_input_buffer;

size_t color_data_size = (sizeof(cl_uchar3) * (WIDTH * HEIGHT)), rand_data_size = (sizeof(cl_ulong) * 10), neighbors_data_size = (sizeof(cl_ulong) * 80), time_data_size = sizeof(cl_ulong);

char title[] = "Assignment 1 Simulation";

// forward declarations
void clean_up();
void display();
void opencl_setup();
void update(int);

void display()
{
	// Do_work()

	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_POINT_SMOOTH);
	glPointSize(1);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(cl_uchar3), p_col);
	glVertexPointer(2, GL_INT, sizeof(cl_uint2), p_pos);

	glDrawArrays(GL_POINTS, 0, WIDTH * HEIGHT);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glutSwapBuffers();
}

void clean_up()
{
	if (time_input_buffer != NULL) { clReleaseKernel(generate_random); }
	if (time_input_buffer != NULL) { clReleaseProgram(program); }
	if (time_input_buffer != NULL) { clReleaseCommandQueue(queue_gpu); }
	if (time_input_buffer != NULL) { clReleaseMemObject(color_buffer); }
	if (time_input_buffer != NULL) { clReleaseMemObject(color_copy_buffer); }
	if (time_input_buffer != NULL) { clReleaseMemObject(time_input_buffer); }
	if (rand_input_buffer != NULL) { clReleaseMemObject(rand_input_buffer); }
	if (time_input_buffer != NULL) { clReleaseContext(m_context); }
	if (time_input_buffer != NULL) { clReleaseDevice(device_ids[0]); }
}

void opencl_setup()
{
	// Find Platforms
	err = clGetPlatformIDs(0, nullptr, &num_platforms);

	std::cout << "\nNumber of Platforms are " << num_platforms << "!" << endl;


	// get device ids
	err = clGetPlatformIDs(num_platforms, platform_ids, &num_platforms);

	err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);

	std::cout << "There are " << num_devices << " Device(s) the Platform!" << endl;

	err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_ALL, num_devices, device_ids, nullptr);

	std::cout << "\nChecking  Device " << 1 << "..." << endl;


	// Determine Device Types
	cl_device_type m_type;
	clGetDeviceInfo(device_ids[0], CL_DEVICE_TYPE, sizeof(m_type), &m_type, nullptr);
	if (m_type & CL_DEVICE_TYPE_CPU)
	{
		err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_CPU, 1, &device_ids[0], nullptr);
	}
	else if (m_type & CL_DEVICE_TYPE_GPU)
	{
		err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_GPU, 1, &device_ids[0], nullptr);
	}
	else if (m_type & CL_DEVICE_TYPE_ACCELERATOR)
	{
		err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_ACCELERATOR, 1, &device_ids[0], nullptr);
	}
	else if (m_type & CL_DEVICE_TYPE_DEFAULT)
	{
		err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_DEFAULT, 1, &device_ids[0], nullptr);
	}
	else
	{
		std::cerr << "\nDevice " << 1 << " is unknowned!" << endl;
	}


	// Create Context
	const cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform_ids[0], 0 };

	m_context = clCreateContext(properties, num_devices, device_ids, nullptr, nullptr, &err);


	// Setup Command Queues
	queue_gpu = clCreateCommandQueueWithProperties(m_context, device_ids[0], 0, &err);

	// initialize arrays
	for (cl_uint i = 0; i < (WIDTH * HEIGHT); ++i)
	{
		m_positions[i] = { {i % WIDTH, i / HEIGHT} };
		cl_uchar r = (rand() * 255) % 256;
		cl_uchar g = (rand() * 255) % 256;
		cl_uchar b = (rand() * 255) % 256;
		m_colors[i] = { { r, g, b } };
		m_colors_read[i] = { { r, g, b } };
	}


	const char* source[4] = { char_generate_random };
	cl_uint count = 4;


	// Create Program with all kernels
	program = clCreateProgramWithSource(m_context, count, source, nullptr, &err);


	// Build Program
	err = clBuildProgram(program, num_devices, device_ids, nullptr, nullptr, nullptr);


	// Create Kernels
	generate_random = clCreateKernel(program, "generate_random", &err);


	// Setup Buffers
	color_buffer = clCreateBuffer(m_context, CL_MEM_READ_WRITE, (sizeof(cl_uchar3) * (WIDTH * HEIGHT)), nullptr, &err);

	color_copy_buffer = clCreateBuffer(m_context, CL_MEM_READ_WRITE, (sizeof(cl_uchar3) * (WIDTH * HEIGHT)), nullptr, &err);
}

void update(int value)
{
	glutPostRedisplay();
	glutTimerFunc(1000 / 30, update, 0);
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(static_cast<int>(WINDOW_WIDTH * 1.5), static_cast<int>(WINDOW_HEIGHT * 1.5));

	glutCreateWindow(title);
	glewInit();

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	gluOrtho2D(-0.5, static_cast<double>(WINDOW_WIDTH) - 0.5, -0.5, static_cast<double>(WINDOW_HEIGHT) - 0.5);

	// Initialize others here
	opencl_setup();

	glutDisplayFunc(display);
	// Initially set to 1s for to start
	glutTimerFunc(1000, update, 0); // from TA's demo, 1s to startup 

#if defined (__APPLE__) || defined(MACOSX)
	atexit(cleanup);
#else
	glutCloseFunc(clean_up);
#endif

	glutMainLoop();

	return 0;
}