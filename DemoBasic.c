#include "DemoBasic.h"
#include "Lex.h"
#include "CodeGen.h"
#include "Container.h"
#include <stdio.h>

typedef struct VarDecl VarDecl;
struct VarDecl
{
	cg_Type type;
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
static cg_Var* parseFactor(demobasic_Context* c);
static cg_Var* parseTerm2(demobasic_Context* c, cg_Var* lhs);
static cg_Var* parseTerm1(demobasic_Context* c);
static cg_Var* parseExp10(demobasic_Context* c, cg_Var* lhs);
static cg_Var* parseExp9(demobasic_Context* c);
static cg_Var* parseExp8(demobasic_Context* c, cg_Var* lhs);
static cg_Var* parseExp7(demobasic_Context* c);
static cg_Var* parseExp6(demobasic_Context* c, cg_Var* lhs);
static cg_Var* parseExp5(demobasic_Context* c);
static cg_Var* parseExp4(demobasic_Context* c, cg_Var* lhs);
static cg_Var* parseExp3(demobasic_Context* c);
static cg_Var* parseExp2(demobasic_Context* c, cg_Var* lhs);
static cg_Var* parseExpression(demobasic_Context* c);
static void parseError(demobasic_Context* c);
/************************************************************************/

static VarDecl* findVarDecl(demobasic_Context* c, const char* id)
{
	VarDecl* it;
	ct_fixArrayForEach(&c->varDecls, it) {
		if (0 == strcmp(id, it->id))
			return it;
	}
	return 0;
}
static VarDecl* findOrAddVarDecl(demobasic_Context* c, const char* id)
{
	VarDecl* it = findVarDecl(c, id);
	if (it) {
		return it;
	} else {
		VarDecl newDecl;
		newDecl.type = cg_Auto;
		strcpy(newDecl.id, id);
		ct_fixArrayPushBack(&c->varDecls, &newDecl);
		return ct_fixArrayLast(&c->varDecls);
	}
}

static VarDecl* findOrFailVarDecl(demobasic_Context* c, const char* id)
{
	VarDecl* it = findVarDecl(c, id);
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

static cg_Var* doBinaryOpNode(demobasic_Context* c, cg_Var* lhs, cg_Var* (rhsFunc)(demobasic_Context*), cg_BinOp op)
{
	cg_Var* result;
	cg_Var* rhs;
	lex_nextToken(&c->lex);
	skip(c);
	rhs = rhsFunc(c);
	result = cg_newTempVar(c->cg, rhs);
	cg_emitBinOp(c->cg, result, lhs, op, rhs);
	return result;
}

/******************************************************************************
  Expressions
******************************************************************************/

static cg_Var* parseExpression(demobasic_Context* c) /* exp1 */
{
	cg_Var* lhs = parseExp3(c);
	skip(c);
	return parseExp2(c, lhs);
}

static cg_Var* parseExp2(demobasic_Context* c, cg_Var* lhs)
{
	switch(getToken(c)) {
	case tokLOGICALOR:	return doBinaryOpNode(c, lhs, parseExpression, cg_LOGICALOR);
	default:		return lhs;
	}
}

static cg_Var* parseExp3(demobasic_Context* c)
{
	cg_Var* lhs = parseExp5(c);
	skip(c);
	return parseExp4(c, lhs);
}

static cg_Var* parseExp4(demobasic_Context* c, cg_Var* lhs)
{
	switch(getToken(c)) {
	case tokLOGICALAND:	return doBinaryOpNode(c, lhs, parseExp3, cg_LOGICALAND);
	default:		return lhs;
	}
}

static cg_Var* parseExp5(demobasic_Context* c)
{
	cg_Var* lhs = parseExp7(c);
	skip(c);
	return parseExp6(c, lhs);
}

static cg_Var* parseExp6(demobasic_Context* c, cg_Var* lhs)
{
	switch(getToken(c)) {
	case tokEQ:		return doBinaryOpNode(c, lhs, parseExp5, cg_EQ);
	case tokNE:		return doBinaryOpNode(c, lhs, parseExp5, cg_NE);
	default:		return lhs;
	}
}

static cg_Var* parseExp7(demobasic_Context* c)
{
	cg_Var* lhs = parseExp9(c);
	skip(c);
	return parseExp8(c, lhs);
}

static cg_Var* parseExp8(demobasic_Context* c, cg_Var* lhs)
{
	switch(getToken(c)) {
	case tokLT:		return doBinaryOpNode(c, lhs, parseExp7, cg_LT);
	case tokGT:		return doBinaryOpNode(c, lhs, parseExp7, cg_GT);
	case tokLE:		return doBinaryOpNode(c, lhs, parseExp7, cg_LE);
	case tokGE:		return doBinaryOpNode(c, lhs, parseExp7, cg_GE);
	default:		return lhs;
	}
}

static cg_Var* parseExp9(demobasic_Context* c)
{
	cg_Var* lhs = parseTerm1(c);
	skip(c);
	return parseExp10(c, lhs);
}

static cg_Var* parseExp10(demobasic_Context* c, cg_Var* lhs)
{
	switch(getToken(c)) {
	case tokPLUS:		return doBinaryOpNode(c, lhs, parseExp9, cg_PLUS);
	case tokMINUS:		return doBinaryOpNode(c, lhs, parseExp9, cg_MINUS);
	default:		return lhs;
	}
}

static cg_Var* parseTerm1(demobasic_Context* c)
{
	cg_Var* lhs = parseFactor(c);
	skip(c);
	return parseTerm2(c, lhs);
}

static cg_Var* parseTerm2(demobasic_Context* c, cg_Var* lhs)
{
	switch(getToken(c)) {
	case tokMULT:		return doBinaryOpNode(c, lhs, parseTerm1, cg_MULT);
	case tokDIV:		return doBinaryOpNode(c, lhs, parseTerm1, cg_DIV);
	case tokMOD:		return doBinaryOpNode(c, lhs, parseTerm1, cg_MOD);
	default:		return lhs;
	}
}

static cg_Var* parseFactor(demobasic_Context* c)
{
	cg_Var* temp;
	cg_Var* result = 0;
	if (getToken(c) == tokNUMCONST) {	/* const */
		result = cg_newIntConstant(c->cg, c->lex.numConst);
		lex_nextToken(&c->lex);
	} else if (getToken(c) == tokID) {	/* var */
		VarDecl* var = findOrFailVarDecl(c, c->lex.id);
		result = cg_newVar(c->cg, var->id, var->type);
		lex_nextToken(&c->lex);
	} else if (getToken(c) == tokLPAR) {	/* ( exp1 ) */
		lex_nextToken(&c->lex);
		skip(c);
		result = parseExpression(c);
		skipToken(c, tokRPAR);
	} else if (getToken(c) == tokMINUS) {	/* -factor */
		lex_nextToken(&c->lex);
		skip(c);
		temp = parseFactor(c);
		result = cg_newTempVar(c->cg, temp);
		cg_emitUnaryOp(c->cg, result, cg_UNARYMINUS, temp);
	} else if (getToken(c) == tokLOGICALNOT) {	/* not exp */
		lex_nextToken(&c->lex);
		skip(c);
		temp = parseFactor(c);
		result = cg_newTempVar(c->cg, temp);
		/* TODO: check integer type */
		cg_emitUnaryOp(c->cg, result, cg_LOGICALNOT, temp);
	} else {
		parseError(c);
	}
	return result;
}

static void parseStm(demobasic_Context* c)
{
	if (getToken(c) == tokID) {
		VarDecl* var = findOrAddVarDecl(c, c->lex.id);
		cg_Var* result = cg_newVar(c->cg, c->lex.id, var->type);
		lex_nextToken(&c->lex);
		skipToken(c, tokEQ);
		skip(c);
		cg_emitAssign(c->cg, result, parseExpression(c));
		var->type = cg_varType(c->cg, result); /* Copy the type for future use of var */
	} else if (getToken(c) == tokIF) {
		cg_Var* expr;
		cg_Label* label1 = cg_newTempLabel(c->cg);
		lex_nextToken(&c->lex);
		skip(c);
		expr = parseExpression(c);
		skipToken(c, tokTHEN);
		if (getToken(c) == tokNEWLINE) {
			lex_nextToken(&c->lex);
			skip(c);
			cg_emitIfFalseGoto(c->cg, expr, label1);
			parseBody(c, tokENDIF, tokELSE);
			if (getToken(c) == tokENDIF) {
				lex_nextToken(&c->lex);
				cg_emitLabel(c->cg, label1);
			} else if (getToken(c) == tokELSE) {
				cg_Label* label2 = cg_newTempLabel(c->cg);
				lex_nextToken(&c->lex);
				cg_emitGoto(c->cg, label2);
				cg_emitLabel(c->cg, label1);
				skipToken(c, tokNEWLINE);
				parseBody(c, tokENDIF, tokDONTMATCH);
				checkToken(c, tokENDIF);
				cg_emitLabel(c->cg, label2);
			} else {
				parseError(c);
			}
		} else {
			/* Single line if */
			lex_nextToken(&c->lex);
			skip(c);
			cg_emitIfFalseGoto(c->cg, expr, label1);
			parseStm(c);
			cg_emitLabel(c->cg, label1);
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
	cg_Label* functionName = cg_newLabel(c->cg, "main");
	cg_emitBeginFunc(c->cg, functionName);
	skip(c);
	parseBody(c, tokEOF, tokDONTMATCH);
	cg_emitEndFunc(c->cg);
}

demobasic_MachineCode* 
demobasic_compile(const char* sourceCode, size_t sourceCodeLength, mem_Allocator* allocator)
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

