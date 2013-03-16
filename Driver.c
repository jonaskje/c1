#include "DemoBasic.h"
#include "MachineCode.h"
#include "CodeGen.h"
#include "Types.h"
#include "Api.h"
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

typedef long (DemoBasicFunc)(const void** api);

static int 
execute(mc_MachineCode* mc, const void** api)
{
	size_t Capacity = 1024*1024;
	int result = 0;
	u8* execMem = allocCodeMemory(Capacity);
	DemoBasicFunc* f = (DemoBasicFunc*)(execMem);
	memcpy(execMem, (u8*)mc + mc->codeOffset, mc->codeSize);
	result = f(api);
	freeCodeMemory(execMem, Capacity);
	return result;
}

static int
runCode(mem_Allocator* allocator, const char* code, size_t size)
{
	int result;
	api_Functions api = api_getFunctions();
	mc_MachineCode* machineCode = demobasic_compile(code, size, api.descs, api.count, allocator);
	result = execute(machineCode, api.functions);
	free(machineCode);
	return result;
}

static int
runTests(mem_Allocator* allocator, char* code, size_t size)
{
	int testsRun = 0;
	int testsOk  = 0;
	char* p = code;
	const char* pend = code + size;

	if(size == 0 || code[0] != '@')
		return 0;

	while (p != pend) {
		const char* idStart = p + 1;
		const char* codeStart;
		int expectedResult = 1;
		int result = 0;
		while(p != pend && *p != '\n')
			++p;
		if (p != pend)
			*p++ = 0;
		codeStart = p;
		
		while(p != pend	&& *p != '@')
			++p;
		
		result = runCode(allocator, codeStart, p - codeStart);

		++testsRun;
		if (result == expectedResult)
			++testsOk;
		
		printf("%-40s: %s\n", idStart, (result == expectedResult) ? "Ok" : "Fail");
	}

	if (testsRun == testsOk)
		printf("\nOK\n");
	else
		printf("\nFAILED: %d out of %d\n", (testsRun - testsOk), testsRun);

	return 1;
}

int main(int argc, char** argv)
{
	int ret = 0;
	char* code = 0;
	size_t size;
	mem_Allocator allocator;

	if (0 != readFile(argv[1], &code, &size))
	{
		printf ("Could not open input file\n");
		ret = -1;
		goto done;
	}

	allocator.allocMem = malloc;
	allocator.freeMem = free;

	if (!runTests(&allocator, code, size))
		runCode(&allocator, code, size);

done:
	api_shutdown();
	free(code);
	return ret;
}
