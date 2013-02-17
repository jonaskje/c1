#pragma once
#include "Memory.h"
#include <stdlib.h>

typedef struct demobasic_MachineCode demobasic_MachineCode;
struct demobasic_MachineCode
{
	const char* code;
	size_t codeSize; 
};

demobasic_MachineCode* 
demobasic_compile(const char* sourceCode, size_t sourceCodeLength, const mem_Allocator* allocator);


