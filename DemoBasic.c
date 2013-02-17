#include "DemoBasic.h"
#include "Lex.h"
#include "CodeGen.h"
#include "Container.h"
#include <stdio.h>

typedef struct VarDecl VarDecl;
struct VarDecl
{
	char id[lex_MAX_ID_LENGTH];
};

typedef struct demobasic_Context demobasic_Context;
struct demobasic_Context
{
	lex_Context lex;
	cg_Context* cg;
	const mem_Allocator* allocator;
	ct_FixArray(VarDecl, 1000) varDecls;
};

/************************************************************************/
static void parseBody(demobasic_Context* c, int endToken1, int endToken2);
static void parseFactor(demobasic_Context* c);
static void parseTerm2(demobasic_Context* c);
static void parseTerm1(demobasic_Context* c);
static void parseExp10(demobasic_Context* c);
static void parseExp9(demobasic_Context* c);
static void parseExp8(demobasic_Context* c);
static void parseExp7(demobasic_Context* c);
static void parseExp6(demobasic_Context* c);
static void parseExp5(demobasic_Context* c);
static void parseExp4(demobasic_Context* c);
static void parseExp3(demobasic_Context* c);
static void parseExp2(demobasic_Context* c);
static void parseExpression(demobasic_Context* c);
static void parseError(demobasic_Context* c);
/************************************************************************/

static const VarDecl* findVarDecl(demobasic_Context* c, const char* id)
{
	VarDecl* it;
	ct_fixArrayForEach(&c->varDecls, it) {
		if (0 == strcmp(id, it->id))
			return it;
	}
	return 0;
}
static const VarDecl* findOrAddVarDecl(demobasic_Context* c, const char* id)
{
	const VarDecl* it = findVarDecl(c, id);
	if (it) {
		return it;
	} else {
		VarDecl newDecl;
		strcpy(newDecl.id, id);
		ct_fixArrayPushBack(&c->varDecls, &newDecl);
		return ct_fixArrayLast(&c->varDecls);
	}
}

static const VarDecl* findOrFailVarDecl(demobasic_Context* c, const char* id)
{
	const VarDecl* it = findVarDecl(c, id);
	if (it) {
		return it;
	} else {
		parseError(c);
		return 0;
	}
}

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

/******************************************************************************
  Expressions
******************************************************************************/

static void parseExpression(demobasic_Context* c) /* exp1 */
{
	parseExp3(c);
	skip(c);
	parseExp2(c);
}

static void parseExp2(demobasic_Context* c)
{
	if (getToken(c) == tokLOGICALOR) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExpression(c);
		cg_pushBinOp(c->cg, cg_LOGICALOR);
	} else {
		/* something else, but this is ok */
	}
}

static void parseExp3(demobasic_Context* c)
{
	parseExp5(c);
	skip(c);
	parseExp4(c);
}

static void parseExp4(demobasic_Context* c)
{
	if (getToken(c) == tokLOGICALAND) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExp3(c);
		cg_pushBinOp(c->cg, cg_LOGICALAND);
	} else {
		/* something else, but this is ok */
	}
}

static void parseExp5(demobasic_Context* c)
{
	parseExp7(c);
	skip(c);
	parseExp6(c);
}

static void parseExp6(demobasic_Context* c)
{
	if (getToken(c) == tokEQ) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExp5(c);
		cg_pushBinOp(c->cg, cg_EQ);
	} else if (getToken(c) == tokNE) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExp5(c);
		cg_pushBinOp(c->cg, cg_NE);
	} else {
		/* something else, but this is ok */
	}
}

static void parseExp7(demobasic_Context* c)
{
	parseExp9(c);
	skip(c);
	parseExp8(c);
}

static void parseExp8(demobasic_Context* c)
{
	if (getToken(c) == tokLT) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExp7(c);
		cg_pushBinOp(c->cg, cg_LT);
	} else if (getToken(c) == tokGT) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExp7(c);
		cg_pushBinOp(c->cg, cg_GT);
	} else if (getToken(c) == tokLE) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExp7(c);
		cg_pushBinOp(c->cg, cg_LE);
	} else if (getToken(c) == tokGE) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExp7(c);
		cg_pushBinOp(c->cg, cg_GE);
	} else {
		/* something else, but this is ok */
	}
}

static void parseExp9(demobasic_Context* c)
{
	parseTerm1(c);
	skip(c);
	parseExp10(c);
}

static void parseExp10(demobasic_Context* c)
{
	if (getToken(c) == tokPLUS) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExp9(c);
		cg_pushBinOp(c->cg, cg_PLUS);
	} else if (getToken(c) == tokMINUS) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExp9(c);
		cg_pushBinOp(c->cg, cg_MINUS);
	} else {
		/* something else, but this is ok */
	}
}

static void parseTerm1(demobasic_Context* c)
{
	parseFactor(c);
	skip(c);
	parseTerm2(c);
}

static void parseTerm2(demobasic_Context* c)
{
	if (getToken(c) == tokMULT) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm1(c);
		cg_pushBinOp(c->cg, cg_MULT);
	} else if (getToken(c) == tokDIV) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm1(c);
		cg_pushBinOp(c->cg, cg_DIV);
	} else if (getToken(c) == tokMOD) {
		lex_nextToken(&c->lex);
		skip(c);
		parseTerm1(c);
		cg_pushBinOp(c->cg, cg_MOD);
	} else {
		/* something else, but this is ok */
	}
}

static void parseFactor(demobasic_Context* c)
{
	if (getToken(c) == tokNUMCONST) {	/* const */
		cg_pushNumConst(c->cg, c->lex.numConst);
		lex_nextToken(&c->lex);
	} else if (getToken(c) == tokID) {	/* var */
		const VarDecl* var = findOrFailVarDecl(c, c->lex.id);
		cg_pushVar(c->cg, var->id);
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
		cg_pushUnaryOp(c->cg, cg_UNARYMINUS);
	} else if (getToken(c) == tokLOGICALNOT) {	/* not exp */
		lex_nextToken(&c->lex);
		skip(c);
		parseFactor(c);
		cg_pushUnaryOp(c->cg, cg_LOGICALNOT);
	} else {
		parseError(c);
	}
}

static void parseStm(demobasic_Context* c)
{
	if (getToken(c) == tokID) {
		const VarDecl* var = findOrAddVarDecl(c, c->lex.id);
		cg_pushVar(c->cg, var->id);
		lex_nextToken(&c->lex);
		skipToken(c, tokEQ);
		skip(c);
		parseExpression(c);
		cg_assign(c->cg);
	} else if (getToken(c) == tokIF) {
		lex_nextToken(&c->lex);
		skip(c);
		parseExpression(c);
		skipToken(c, tokTHEN);
		if (getToken(c) == tokNEWLINE) {
			cg_Label* label1;
			lex_nextToken(&c->lex);
			skip(c);
			label1 = cg_ifFalseGoto(c->cg);
			parseBody(c, tokENDIF, tokELSE);
			if (getToken(c) == tokENDIF) {
				lex_nextToken(&c->lex);
				cg_label(c->cg, label1);
			} else if (getToken(c) == tokELSE) {
				cg_Label* label2;
				lex_nextToken(&c->lex);
				label2 = cg_goto(c->cg);
				cg_label(c->cg, label1);
				skipToken(c, tokNEWLINE);
				parseBody(c, tokENDIF, tokDONTMATCH);
				checkToken(c, tokENDIF);
				cg_label(c->cg, label2);
			} else {
				parseError(c);
			}
		} else {
			/* Single line if */
			cg_Label* label;
			lex_nextToken(&c->lex);
			skip(c);
			label = cg_ifFalseGoto(c->cg);
			parseStm(c);
			cg_label(c->cg, label);
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
demobasic_compile(const char* sourceCode, size_t sourceCodeLength, const mem_Allocator* allocator)
{
	demobasic_Context c;
	c.allocator = allocator;
	ct_fixArrayInit(&c.varDecls);
	c.cg = cg_newContext(allocator);
	lex_initContext(&c.lex, sourceCode, sourceCodeLength);
	lex_nextToken(&c.lex);
	parseProgram(&c);
	cg_deleteContext(c.cg);
	return 0;
}

