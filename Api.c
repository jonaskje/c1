#include "Api.h"
#include "CodeGen.h"
#include <GL/glfw.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

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

struct ApiPenContext
{
	int isDown;
	i64 rotation;
	double x, y;
};

typedef struct ApiContext ApiContext;
struct ApiContext 
{
	int graphicsInitialized;
	int windowWidth;
	int windowHeight;

	struct ApiPenContext pen;
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
	int fullscreen = 0;

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

/* Pen */

static void
api_penDown(void)
{
	if (g_apiContext.graphicsInitialized)
		g_apiContext.pen.isDown = 1;
}

static void
api_penUp(void)
{
	g_apiContext.pen.isDown = 0;
}

static void
api_penForward(i64 amount)
{
	double rot = ((double)g_apiContext.pen.rotation)*(3.14159265/180.0);
	double x = g_apiContext.pen.x;
	double y = g_apiContext.pen.y;
	g_apiContext.pen.x -= sin(rot) * (double)amount;
	g_apiContext.pen.y += cos(rot) * (double)amount;

	if (g_apiContext.pen.isDown)
	{
		glBegin(GL_LINES);
		glVertex2f((float)x, (float)y);
		glVertex2f((float)g_apiContext.pen.x, (float)g_apiContext.pen.y);
		glEnd();
	}
}

static void
api_penRotate(i64 degrees)
{
	g_apiContext.pen.rotation = (g_apiContext.pen.rotation + degrees) % 360;
}

static void
api_penReset(void)
{
	memset(&g_apiContext.pen, 0, sizeof(g_apiContext.pen));
}

/* End Pen */

struct api_FunctionDesc g_apiEntries[] = {
	cg_Auto,	"printValue",	{ cg_Int },
	cg_Auto,	"graphics",	{ cg_Int, cg_Int },
	cg_Int,		"display",	{ 0 },
	cg_Auto,	"color",	{ cg_Int, cg_Int, cg_Int, cg_Int },
	cg_Auto,	"line",		{ cg_Int, cg_Int, cg_Int, cg_Int },
	cg_Int,		"random",	{ cg_Int, cg_Int },

	/* Pen */
	cg_Auto,	"penDown",	{ 0 },
	cg_Auto,	"penUp",	{ 0 },
	cg_Auto,	"penForward",	{ cg_Int },
	cg_Auto,	"penRotate",	{ cg_Int },
	cg_Auto,	"penReset",	{ 0 },

};

static const void* g_runtimeApi[] = {
	(void*)&api_printValue,
	(void*)&api_graphics,
	(void*)&api_display,
	(void*)&api_color,
	(void*)&api_line,
	(void*)&api_random,
	
	/* Pen */
	(void*)&api_penDown,
	(void*)&api_penUp,
	(void*)&api_penForward,
	(void*)&api_penRotate,
	(void*)&api_penReset,
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

