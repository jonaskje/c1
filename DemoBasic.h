#pragma once
#include "Types.h"
#include "Memory.h"
#include <stdlib.h>

struct mc_MachineCode;

typedef struct demobasic_ApiEntry demobasic_ApiEntry;
struct demobasic_ApiEntry {
	u8 returnType;
	const char* id;
	u8 argType[8];
};

struct mc_MachineCode* 
demobasic_compile(const char* sourceCode, size_t sourceCodeLength, 
		  const demobasic_ApiEntry* api, u32 apiCount, 
		  mem_Allocator* allocator);


