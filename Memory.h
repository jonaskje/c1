#pragma once

#include <stdlib.h>

typedef struct mem_Allocator mem_Allocator;
struct mem_Allocator
{
	void* (*allocMem)(size_t size);
	void (*freeMem)(void* p);
};

