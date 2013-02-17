#include "DemoBasic.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

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

int main(int argc, char** argv)
{
	int ret = 0;
	char* code = 0;
	size_t size;
	mem_Allocator allocator;
	demobasic_MachineCode* machineCode = 0;

	if (0 != readFile(argv[1], &code, &size))
	{
		printf ("Could not open input file\n");
		ret = -1;
		goto done;
	}

	allocator.allocMem = malloc;
	allocator.freeMem = free;

	machineCode = demobasic_compile(code, size, &allocator);
done:
	free(machineCode);
	free(code);
	return ret;
}
