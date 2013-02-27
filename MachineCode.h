#pragma once

#include "Types.h"

typedef struct mc_MachineCode mc_MachineCode;
struct mc_MachineCode
{
	u32 capacity;			/* Total capacity of the machine code blob */
	u32 size;			/* Currently used bytes of the capacity */
	u32 codeOffset;
	u32 codeSize; 
};

