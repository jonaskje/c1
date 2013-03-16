#include "Api.h"
#include "CodeGen.h"
#include <GL/glfw.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
	int fullscreen = 1;

	if (g_apiContext.graphicsInitialized)
		return;
	
	glfwInit();

	glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);

	if (fullscreen)
		glfwOpenWindow(width, height, 0, 0, 0, 0, 16, 0, GLFW_FULLSCREEN);
	else
		glfwOpenWindow(width, height, 0, 0, 0, 0, 16, 0, GLFW_WINDOW);
	
	glfwSwapInterval(1); /* vsync */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-width/2, width - width/2, -height/2, height - height/2, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glTranslatef(0.375f, 0.375f, 0);


	glDisable(GL_DEPTH_TEST);
	
	glClearColor(0, 0, 0, 255);


	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers();

	
	g_apiContext.graphicsInitialized = 1;
	g_apiContext.windowHeight = height;
	g_apiContext.windowWidth = width;
}

static void
api_color(i64 r, i64 g, i64 b, i64 a)
{
	const float col[4] = { toColorF(r), toColorF(g), toColorF(b), toColorF(a) };
	
	if (!g_apiContext.graphicsInitialized)
		return;

	glColor4fv(col);
}

static void
api_line(i64 x0, i64 y0, i64 x1, i64 y1)
{
	if (!g_apiContext.graphicsInitialized)
		return;

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

static i64
api_random(i64 lo, i64 hi)
{
	return (rand() % (hi - lo + 1)) + lo;
}

struct api_FunctionDesc g_apiEntries[] = {
	cg_Auto,	"printValue",	{ cg_Int },
	cg_Auto,	"graphics",	{ cg_Int, cg_Int },
	cg_Int,		"display",	{ 0 },
	cg_Auto,	"color",	{ cg_Int, cg_Int, cg_Int, cg_Int },
	cg_Auto,	"line",		{ cg_Int, cg_Int, cg_Int, cg_Int },
	cg_Int,		"random",	{ cg_Int, cg_Int }
};

static const void* g_runtimeApi[] = {
	(void*)&api_printValue,
	(void*)&api_graphics,
	(void*)&api_display,
	(void*)&api_color,
	(void*)&api_line,
	(void*)&api_random,
};


api_Functions 
api_getFunctions()
{
	api_Functions f;
	f.count = sizeof(g_runtimeApi)/sizeof(g_runtimeApi[0]);
	f.descs = g_apiEntries;
	f.functions = g_runtimeApi;
	return f;
}

void 
api_shutdown(void)
{
	if (g_apiContext.graphicsInitialized) 
		glfwTerminate();
	
	memset(&g_apiContext, 0, sizeof(g_apiContext));
}

