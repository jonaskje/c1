#include "DemoBasic.h"
#include "MachineCode.h"
#include "Types.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

static int readFile(const char* filename, char** bufptr, size_t* size)
{
	FILE *f = fopen(filename, "rb");

	if (!f) {
		return errno;
	}
	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*bufptr = (char*)malloc(*size + 1);
	if (!*bufptr) {
		fclose(f);
		return ENOMEM;
	}
	fread(*bufptr, 1, *size, f);
	(*bufptr)[*size++] = '\0';

	fclose(f);
	return 0;
}

static u8*
allocCodeMemory(size_t size)
{
	u8* result = mmap(0, size, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
	assert(result != (u8*)~0ul);
	return result;
}

static void
freeCodeMemory(u8* mem, size_t size)
{
	if (!mem)
		return;
	munmap(mem, size);
}

typedef long (DemoBasicFunc)(void);

static void 
execute(mc_MachineCode* mc)
{
	size_t Capacity = 1024*1024;
	u8* execMem = allocCodeMemory(Capacity);
	DemoBasicFunc* f = (DemoBasicFunc*)(execMem);
	memcpy(execMem, (u8*)mc + mc->codeOffset, mc->codeSize);
	printf ("Result = %ld\n", f());
	freeCodeMemory(execMem, Capacity);
}

int main(int argc, char** argv)
{
	int ret = 0;
	char* code = 0;
	size_t size;
	mem_Allocator allocator;
	mc_MachineCode* machineCode = 0;

	if (0 != readFile(argv[1], &code, &size))
	{
		printf ("Could not open input file\n");
		ret = -1;
		goto done;
	}

	allocator.allocMem = malloc;
	allocator.freeMem = free;

	machineCode = demobasic_compile(code, size, &allocator);

	execute(machineCode);

	printf("Size: %u\n", machineCode->codeSize);
done:
	free(machineCode);
	free(code);
	return ret;
}
