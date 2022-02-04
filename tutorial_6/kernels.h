#pragma once

#include <string>

using namespace std;

const string generate_random_kernel = // should be done on the CPU
"__kernel void generate_random(__global unsigned long* p_current_time, __global unsigned long* p_ulong_rand)\n" // Line 104
"{\n"
"	unsigned int gid = get_global_id(0);\n" //,count = 0;\n"
"	unsigned long width = 500, height = 500;\n"
"	bool loop_condition = true;\n"
"	do\n" 
"	{\n"
"		unsigned long seed = p_current_time + gid;\n" // From:
"		seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);\n"
"		unsigned long result = (seed >> 16) % (width * height);\n" // or seed >> 16;
"		if ( !( (result > 1 && result < (width - 2)) || (result > width + 1 && result < (width * 2) - 2) || (result % (width - 1) == 0) || (result % (width - 2) == 0) || (result % (width) == 0) || (result % (width + 1) == 0) || (result > ((width * height) - width + 1) && result < (width * height) - 2) || (result > ((width * height) - (width * 2) + 1) && result < (width * height) - width - 2) ) )\n" // excluding edges for simplicity
"		{\n"
"			p_ulong_rand[gid] = result;\n"
"			loop_condition = false;\n"
"		}\n"
"	}\n"
"	while(loop_condition);\n"
"   barrier(CLK_GLOBAL_MEM_FENCE);\n"
"}\n"; // Line 18

// const size_t generate_random_t = generate_random_ulong_kernel.length();
const char* char_generate_random = { (generate_random_kernel.c_str()) };