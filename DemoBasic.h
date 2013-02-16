#pragma once
#include <stdlib.h>

typedef struct demobasic_Allocator demobasic_Allocator;
struct demobasic_Allocator
{
	void* (*allocMem)(size_t size);
	void (*freeMem)(void* p);
};

typedef struct demobasic_MachineCode demobasic_MachineCode;
struct demobasic_MachineCode
{
	const char* code;
	size_t codeSize; 
};

demobasic_MachineCode* 
demobasic_compile(const char* sourceCode, size_t sourceCodeLength, const demobasic_Allocator* allocator);


