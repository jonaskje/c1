#include "DemoBasic.h"
#include "MachineCode.h"
#include "CodeGen.h"
#include "Types.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <GL/glfw.h>

__forceinline i64 clampi64(i64 v, i64 lo, i64 hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

__forceinline float toColorF(i64 v)
{
	return ((float)clampi64(v, 0, 255))/255.f;
}


typedef struct ApiContext ApiContext;
struct ApiContext 
{
	int graphicsInitialized;
	int windowWidth;
	int windowHeight;
};

ApiContext g_apiContext;

static void 
api_printValue(i64 value)
{
	printf("%li\n", value);
}

static void 
api_graphics(i64 width, i64 height)
{
	if (g_apiContext.graphicsInitialized)
		return;
	
	glfwInit();
	glfwOpenWindow(width, height, 0, 0, 0, 0, 16, 0, GLFW_WINDOW);
	glfwSwapInterval(1); /* vsync */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-width/2, width - width/2, -height/2, height - height/2, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glTranslatef(0.375f, 0.375f, 0);


	glDisable(GL_DEPTH_TEST);
	/* glClearColor(127, 127, 127, 255); */
	glClearColor(0, 0, 0, 255);


	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers();

	
	g_apiContext.graphicsInitialized = 1;
	g_apiContext.windowHeight = height;
	g_apiContext.windowWidth = width;
}

static void 
api_cleanup(void)
{
	if (g_apiContext.graphicsInitialized) 
		glfwTerminate();
	
	memset(&g_apiContext, 0, sizeof(g_apiContext));
}

static void
api_color(i64 r, i64 g, i64 b, i64 a)
{
	const float col[4] = { toColorF(r), toColorF(g), toColorF(b), toColorF(a) };
	glColor4fv(col);
}

static void
api_line(i64 x0, i64 y0, i64 x1, i64 y1)
{
	glBegin(GL_LINES);
	glVertex2f((float)x0, (float)y0);
	glVertex2f((float)x1, (float)y1);
	glEnd();
}

static i64
api_display(void)
{
	if (!g_apiContext.graphicsInitialized) {
		glfwTerminate();
		return 0;
	}

	glFinish();
	glfwSwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);

	return (!glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_OPENED));
}

struct demobasic_ApiEntry g_apiEntries[] = {
	cg_Auto, "printValue", { cg_Int },
	cg_Auto, "graphics", { cg_Int, cg_Int },
	cg_Int, "display", { 0 },
	cg_Auto, "color", { cg_Int, cg_Int, cg_Int, cg_Int },
	cg_Auto, "line", { cg_Int, cg_Int, cg_Int, cg_Int }
};

static const void* g_runtimeApi[] = {
	(void*)&api_printValue,
	(void*)&api_graphics,
	(void*)&api_display,
	(void*)&api_color,
	(void*)&api_line,
};

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
execute(mc_MachineCode* mc)
{
	size_t Capacity = 1024*1024;
	int result = 0;
	u8* execMem = allocCodeMemory(Capacity);
	DemoBasicFunc* f = (DemoBasicFunc*)(execMem);
	memcpy(execMem, (u8*)mc + mc->codeOffset, mc->codeSize);
	result = f(g_runtimeApi);
	freeCodeMemory(execMem, Capacity);
	return result;
}

static int
runCode(mem_Allocator* allocator, const char* code, size_t size)
{
	int result;
	mc_MachineCode* machineCode = demobasic_compile(code, size, g_apiEntries,
							sizeof(g_apiEntries)/sizeof(g_apiEntries[0]), 
							allocator);
	result = execute(machineCode);
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
	api_cleanup();
	free(code);
	return ret;
}
