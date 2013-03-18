#include "Api.h"
#include "CodeGen.h"
#include "Container.h"
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

	float currentColor[4]; /* rgba */
	float lineWidth;

	struct ApiPenContext pen;

	ct_FixArray(float, 5000000) verts;
	ct_FixArray(float, 5000000) colors;
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

	g_apiContext.currentColor[0] = 1.f;
	g_apiContext.currentColor[1] = 1.f;
	g_apiContext.currentColor[2] = 1.f;
	g_apiContext.currentColor[3] = 1.f;

	g_apiContext.lineWidth = 1.f;
	
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
	g_apiContext.currentColor[0] = toColorF(r);
	g_apiContext.currentColor[1] = toColorF(g);
	g_apiContext.currentColor[2] = toColorF(b);
	g_apiContext.currentColor[3] = toColorF(a);
}

static void
api_lineInternal(float x0, float y0, float x1, float y1)
{
	int i;
	for (i = 0; i < 4; ++i) {
		ct_fixArrayPushBackValue(&g_apiContext.colors, g_apiContext.currentColor[0]);
		ct_fixArrayPushBackValue(&g_apiContext.colors, g_apiContext.currentColor[1]);
		ct_fixArrayPushBackValue(&g_apiContext.colors, g_apiContext.currentColor[2]);
		ct_fixArrayPushBackValue(&g_apiContext.colors, g_apiContext.currentColor[3]);
	}

	{
		float w = g_apiContext.lineWidth / 2.f;

		float dx = x1 - x0;
		float dy = y1 - y0;
		float tx, ty;
		
		float len = sqrtf(dx * dx + dy * dy);
		
		dx = w * dx/len;
		dy = w * dy/len;

		tx = -dy;
		ty = dx;

		ct_fixArrayPushBackValue(&g_apiContext.verts, x0 + tx - dx);
		ct_fixArrayPushBackValue(&g_apiContext.verts, y0 + ty - dy);
		
		ct_fixArrayPushBackValue(&g_apiContext.verts, x0 - tx - dx);
		ct_fixArrayPushBackValue(&g_apiContext.verts, y0 - ty - dy);
		
		ct_fixArrayPushBackValue(&g_apiContext.verts, x1 - tx + dx);
		ct_fixArrayPushBackValue(&g_apiContext.verts, y1 - ty + dy);
		
		ct_fixArrayPushBackValue(&g_apiContext.verts, x1 + tx + dx);
		ct_fixArrayPushBackValue(&g_apiContext.verts, y1 + ty + dy);
	}
}

static void
api_line(i64 x0i, i64 y0i, i64 x1i, i64 y1i)
{
	if (x0i == x1i && y0i == y1i)
		return;
	api_lineInternal((float)x0i, (float)y0i, (float)x1i, (float)y1i);
}

static i64
api_display(void)
{
	if (!g_apiContext.graphicsInitialized) {
		g_apiContext.verts.n = 0;
		g_apiContext.colors.n = 0;
		return 0;
	}

	glBegin(GL_QUADS);
	{	
		const int n = g_apiContext.verts.n/8;
		int i;
		for (i = 0; i < n; ++i) {
			glColor4fv(ct_fixArrayItem(&g_apiContext.colors, i * 16 + 0));
			glVertex2fv(ct_fixArrayItem(&g_apiContext.verts, i * 8 + 0));

			glColor4fv(ct_fixArrayItem(&g_apiContext.colors, i * 16 + 4));
			glVertex2fv(ct_fixArrayItem(&g_apiContext.verts, i * 8 + 2));

			glColor4fv(ct_fixArrayItem(&g_apiContext.colors, i * 16 + 8));
			glVertex2fv(ct_fixArrayItem(&g_apiContext.verts, i * 8 + 4));

			glColor4fv(ct_fixArrayItem(&g_apiContext.colors, i * 16 + 12));
			glVertex2fv(ct_fixArrayItem(&g_apiContext.verts, i * 8 + 6));
		}
	}
	glEnd();	

	g_apiContext.verts.n = 0;
	g_apiContext.colors.n = 0;

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
		api_lineInternal((float)x, (float)y, (float)g_apiContext.pen.x, (float)g_apiContext.pen.y);
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

static void
api_lineWidth(i64 thickness)
{
	if (thickness > 0)
		g_apiContext.lineWidth = (float)thickness;
	else
		g_apiContext.lineWidth = 1.f;
}

/* return [-1000, 1000] */
static i64
api_sin(i64 milliDegrees)
{
	return (i64)(1000.f * sinf((float)milliDegrees/1000.f*3.14159265f/180.f));
}

/* return [-1000, 1000] */
static i64
api_cos(i64 milliDegrees)
{
	return (i64)(1000.f * cosf((float)milliDegrees/1000.f*3.14159265f/180.f));
}

/* End Pen */

struct api_FunctionDesc g_apiEntries[] = {
	cg_Auto,	"printValue",	{ cg_Int },
	cg_Auto,	"graphics",	{ cg_Int, cg_Int },
	cg_Int,		"display",	{ 0 },
	cg_Auto,	"color",	{ cg_Int, cg_Int, cg_Int, cg_Int },
	cg_Auto,	"line",		{ cg_Int, cg_Int, cg_Int, cg_Int },
	cg_Int,		"random",	{ cg_Int, cg_Int },
	cg_Auto,	"lineWidth",	{ cg_Int },

	/* Pen */
	cg_Auto,	"penDown",	{ 0 },
	cg_Auto,	"penUp",	{ 0 },
	cg_Auto,	"penForward",	{ cg_Int },
	cg_Auto,	"penRotate",	{ cg_Int },
	cg_Auto,	"penReset",	{ 0 },

	/* Math */
	cg_Int,		"sin",		{ cg_Int },
	cg_Int,		"cos",		{ cg_Int },

};

static const void* g_runtimeApi[] = {
	(void*)&api_printValue,
	(void*)&api_graphics,
	(void*)&api_display,
	(void*)&api_color,
	(void*)&api_line,
	(void*)&api_random,
	(void*)&api_lineWidth,
	
	/* Pen */
	(void*)&api_penDown,
	(void*)&api_penUp,
	(void*)&api_penForward,
	(void*)&api_penRotate,
	(void*)&api_penReset,
	
	/* Math */
	(void*)&api_sin,
	(void*)&api_cos,
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

