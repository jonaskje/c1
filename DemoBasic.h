#pragma once
#include "Memory.h"
#include <stdlib.h>

struct mc_MachineCode;

struct mc_MachineCode* 
demobasic_compile(const char* sourceCode, size_t sourceCodeLength, mem_Allocator* allocator);


