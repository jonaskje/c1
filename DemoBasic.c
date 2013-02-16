#include "DemoBasic.h"
#include "Lex.h"
#include <stdio.h>

typedef struct demobasic_Context demobasic_Context;
struct demobasic_Context
{
	lex_Context lex;
	const demobasic_Allocator* allocator;
};

static void parseError(demobasic_Context* c)
{
	printf("Parse error at %3d:%3d\n", c->lex.line, c->lex.col);
	exit(-2);
}

static int getToken(demobasic_Context* c)
{
	return c->lex.tok;
}

static void checkToken(demobasic_Context* c, int token)
{
	if (c->lex.tok != token)
		parseError(c);
	lex_nextToken(&c->lex);
}

static void checkTokenId(demobasic_Context* c, const char* id)
{
	if (c->lex.tok != tokID || 0 != strcmp(id, c->lex.id))
		parseError(c);
	lex_nextToken(&c->lex);
}

static void skip(demobasic_Context* c)
{
	while (c->lex.tok == tokWHITE)
		lex_nextToken(&c->lex);
}

static void skipToken(demobasic_Context* c, int token)
{
	skip(c);
	checkToken(c, token);
}

static void parseExpression(demobasic_Context* c)
{
	int token = getToken(c);
	if (token == tokNUMCONST)
	{

	}
}

static void parseStm(demobasic_Context* c)
{
	if (getToken(c) == tokID)
	{
		lex_nextToken(&c->lex);
		skipToken(c, tokEQ);
		skip(c);
		parseExpression(c);
		skipToken(c, tokNEWLINE);
	}

}

static void parseBody(demobasic_Context* c, int endToken)
{
	while (getToken(c) != endToken)
	{
		skip(c);
		parseStm(c);
	}
}

static void parseProgram(demobasic_Context* c)
{
	skip(c);
	parseBody(c, tokEOF);
}

demobasic_MachineCode* 
demobasic_compile(const char* sourceCode, size_t sourceCodeLength, const demobasic_Allocator* allocator)
{
	demobasic_Context c;
	c.allocator = allocator;
	lex_initContext(&c.lex, sourceCode, sourceCodeLength);
	lex_nextToken(&c.lex);
	parseProgram(&c);
	return 0;
}


#if 0
	int token;
	while((token = lex_nextToken(&lex)) >= 0)
	{
		switch(token)
		{
		case tokID:
			printf("id       (%3d:%3d): '%s'\n", lex.line, lex.col, lex.id);
			break;
		case tokNUMCONST:
			printf("numconst (%3d:%3d): %d\n", lex.line, lex.col, lex.numConst);
			break;
		default:
			printf("tok      (%3d:%3d): %d\n", lex.line, lex.col, token);
			break;
		}
	}

	if (token < tokEOF) {
		printf("ERROR: %d\n", token);
	}
#endif
