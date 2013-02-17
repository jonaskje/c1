#include "DemoBasic.h"
#include "Lex.h"
#include <stdio.h>

typedef struct demobasic_Context demobasic_Context;
struct demobasic_Context
{
	lex_Context lex;
	const demobasic_Allocator* allocator;
};

/************************************************************************/
static void parseBody(demobasic_Context* c, int endToken1, int endToken2);
static void parseExpression(demobasic_Context* c);
static void parseTerm5(demobasic_Context* c);
static void parseTerm4(demobasic_Context* c);
static void parseTerm3(demobasic_Context* c);
static void parseTerm2(demobasic_Context* c);
static void parseTerm1(demobasic_Context* c);
/************************************************************************/

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

static void parseFactor(demobasic_Context* c)
{
	if (getToken(c) == tokNUMCONST) {	/* const */
		lex_nextToken(&c->lex);
	} else if (getToken(c) == tokID) {	/* var */
		lex_nextToken(&c->lex);
	} else if (getToken(c) == tokLPAR) {	/* ( exp1 ) */
		lex_nextToken(&c->lex);
		skip(c);
		parseExpression(c);
		skipToken(c, tokRPAR);
	} else if (getToken(c) == tokMINUS) {	/* -factor */
		lex_nextToken(&c->lex);
		skip(c);
		parseFactor(c);
	} else {
		parseError(c);
	}
}

static void parseTerm6(demobasic_Context* c)
{
	if (getToken(c) == tokEQ) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm5(c);
	} else if (getToken(c) == tokNE) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm5(c);
	} else {
		/* something else, but this is ok */
	}
}

static void parseTerm5(demobasic_Context* c)
{
	parseFactor(c);
	skip(c);
	parseTerm6(c);
}

static void parseTerm4(demobasic_Context* c)
{
	if (getToken(c) == tokLT) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm3(c);
	} else if (getToken(c) == tokGT) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm3(c);
	} else if (getToken(c) == tokLE) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm3(c);
	} else if (getToken(c) == tokGE) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm3(c);
	} else {
		/* something else, but this is ok */
	}
}

static void parseTerm3(demobasic_Context* c)
{
	parseTerm5(c);
	skip(c);
	parseTerm4(c);
}

static void parseTerm2(demobasic_Context* c)
{
	if (getToken(c) == tokMULT) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm1(c);
	} else if (getToken(c) == tokDIV) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm1(c);
	} else if (getToken(c) == tokMOD) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm1(c);
	} else {
		/* something else, but this is ok */
	}
}

static void parseTerm1(demobasic_Context* c)
{
	parseTerm3(c);
	skip(c);
	parseTerm2(c);
}

static void parseExp2(demobasic_Context* c)
{
	if (getToken(c) == tokPLUS) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExpression(c);
	} else if (getToken(c) == tokMINUS) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExpression(c);
	} else {
		/* something else, but this is ok */
	}
}

static void parseExpression(demobasic_Context* c) /* exp1 */
{
	parseTerm1(c);
	skip(c);
	parseExp2(c);
}

static void parseStm(demobasic_Context* c)
{
	if (getToken(c) == tokID) {
		lex_nextToken(&c->lex);
		skipToken(c, tokEQ);
		skip(c);
		parseExpression(c);
	} else if (getToken(c) == tokIF) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExpression(c);
		skipToken(c, tokTHEN);
		if (getToken(c) == tokNEWLINE) {
			lex_nextToken(&c->lex);
			skip(c);
			parseBody(c, tokENDIF, tokELSE);
			if (getToken(c) == tokENDIF) {
				lex_nextToken(&c->lex);
			} else if (getToken(c) == tokELSE) {
				lex_nextToken(&c->lex);
				skipToken(c, tokNEWLINE);
				parseBody(c, tokENDIF, tokDONTMATCH);
				checkToken(c, tokENDIF);
			} else {
				parseError(c);
			}
		} else {
			/* Single line if */
			lex_nextToken(&c->lex);
			skip(c);
			parseStm(c);
		}

	} else if (getToken(c) == tokNEWLINE) { /* empty line */
	} else {
		parseError(c);
	}

}

static void parseStmLine(demobasic_Context* c)
{
	parseStm(c);
	skipToken(c, tokNEWLINE); 
}

static void parseBody(demobasic_Context* c, int endToken1, int endToken2)
{
	while (getToken(c) != endToken1 && getToken(c) != endToken2) {
		skip(c);
		parseStmLine(c);
	}
}

static void parseProgram(demobasic_Context* c)
{
	skip(c);
	parseBody(c, tokEOF, tokDONTMATCH);
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
