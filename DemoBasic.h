#pragma once
#include "Types.h"
#include "Api.h"
#include "Memory.h"
#include <stdlib.h>

struct mc_MachineCode;

struct mc_MachineCode* 
demobasic_compile(const char* sourceCode, size_t sourceCodeLength, 
		  const api_FunctionDesc* api, u32 apiCount, 
		  mem_Allocator* allocator);


