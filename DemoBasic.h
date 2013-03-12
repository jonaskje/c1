#pragma once
#include "Types.h"
#include "Memory.h"
#include <stdlib.h>

typedef struct demobasic_RuntimeApi demobasic_RuntimeApi;
struct demobasic_RuntimeApi {
	void (*printValue)(i64 v);
};


struct mc_MachineCode;

struct mc_MachineCode* 
demobasic_compile(const char* sourceCode, size_t sourceCodeLength, mem_Allocator* allocator);


