#include "DemoBasic.h"
#include "Lex.h"
#include "CodeGen.h"
#include "Container.h"
#include "MachineCode.h"
#include <stdio.h>

typedef struct VarDecl VarDecl;
struct VarDecl
{
	cg_Var* var;
	char id[lex_MAX_ID_LENGTH];
};

typedef struct Label Label;
struct Label 
{
	int defined;
	cg_Label* label;
	char id[lex_MAX_ID_LENGTH];
};

typedef struct demobasic_Context demobasic_Context;
struct demobasic_Context
{
	lex_Context lex;
	cg_Context* cg;
	const mem_Allocator* allocator;
	ct_FixArray(VarDecl, 1000) varDecls;
	ct_FixArray(Label, 1000) labels;
	u32 apiEntryCount;
	const demobasic_ApiEntry* apiEntry;
};


/************************************************************************/
static void parseBody(demobasic_Context* c, int endToken1, int endToken2);
static cg_Var* parseFactor(demobasic_Context* c);
static cg_Var* parseTerm1(demobasic_Context* c);
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

static VarDecl* 
findVarDecl(demobasic_Context* c, const char* id)
{
	VarDecl* it;
	ct_fixArrayForEach(&c->varDecls, it) {
		if (0 == strcmp(id, it->id))
			return it;
	}
	return 0;
}

static VarDecl* 
findOrAddVarDecl(demobasic_Context* c, const char* id)
{
	VarDecl* it = findVarDecl(c, id);
	if (it) {
		return it;
	} else {
		VarDecl newDecl;
		strcpy(newDecl.id, id);
		if (0 == strcmp(id, "_retval"))
			newDecl.var = cg_newVar(c->cg, newDecl.id, cg_Auto, cg_IsRetVal);
		else
			newDecl.var = cg_newVar(c->cg, newDecl.id, cg_Auto, 0);
		ct_fixArrayPushBack(&c->varDecls, &newDecl);
		return ct_fixArrayLast(&c->varDecls);
	}
}

static VarDecl* 
findOrFailVarDecl(demobasic_Context* c, const char* id)
{
	VarDecl* it = findVarDecl(c, id);
	if (it) {
		return it;
	} else {
		parseError(c);
		return 0;
	}
}

/************************************************************************/

static cg_Label* 
addLabel(demobasic_Context* c, const char* id, int defined)
{
	Label* it;
	ct_fixArrayPushBackRaw(&c->labels);
	it = ct_fixArrayLast(&c->labels);
	strcpy(it->id, id);
	it->label = cg_newLabel(c->cg, it->id);
	it->defined = defined;
	return it->label;
}

static cg_Label* 
referenceLabel(demobasic_Context* c, const char* id)
{
	Label* it;
	ct_fixArrayForEach(&c->labels, it) {
		if (0 == strcmp(id, it->id))
			return it->label;
	}
	return addLabel(c, id, 0);
}

static cg_Label* 
defineLabel(demobasic_Context* c, const char* id)
{
	Label* it;
	ct_fixArrayForEach(&c->labels, it) {
		if (0 == strcmp(id, it->id)) {
			if (it->defined) {
				parseError(c); /* Defined twice */
			} else {
				it->defined = 1;
				return it->label;
			}
		}
	}
	return addLabel(c, id, 1);
}

static void 
checkLabelReferences(demobasic_Context* c)
{
	Label* it;
	ct_fixArrayForEach(&c->labels, it) {
		if (!it->defined)
			parseError(c); /* Referenced but not defined */
	}
}

/************************************************************************/

static void 
parseError(demobasic_Context* c)
{
	printf("Parse error at %3d:%3d\n", c->lex.line, c->lex.col);
	exit(-2);
}

static void 
eatToken(demobasic_Context* c)
{
	lex_nextToken(&c->lex);
}

static int 
getToken(demobasic_Context* c)
{
	while (c->lex.tok == tokWHITE)
		eatToken(c);
/*	printf("%d ", c->lex.tok); */
	return c->lex.tok;
}

static void 
expectToken(demobasic_Context* c, int token)
{
	if (getToken(c) != token)
		parseError(c);
}

static void 
expectAndEatToken(demobasic_Context* c, int token)
{
	expectToken(c, token);
	eatToken(c);
}

static cg_Var* 
doBinaryOpNode(demobasic_Context* c, cg_Var* lhs, cg_Var* (rhsFunc)(demobasic_Context*), cg_BinOp op)
{
	cg_Var* result;
	cg_Var* rhs;
	eatToken(c);
	rhs = rhsFunc(c);
	result = cg_newTempVar(c->cg, rhs);
	cg_emitBinOp(c->cg, result, lhs, op, rhs);
	return result;
}

/******************************************************************************
  Expressions
******************************************************************************/

static cg_Var* 
parseExpression(demobasic_Context* c) /* exp1 */
{
	cg_Var* lhs = parseExp3(c);
	return parseExp2(c, lhs);
}

static cg_Var* 
parseExp2(demobasic_Context* c, cg_Var* lhs)
{
	switch(getToken(c)) {
	case tokOR:		return doBinaryOpNode(c, lhs, parseExpression, cg_OR);
	default:		return lhs;
	}
}

static cg_Var* 
parseExp3(demobasic_Context* c)
{
	cg_Var* lhs = parseExp5(c);
	return parseExp4(c, lhs);
}

static cg_Var* 
parseExp4(demobasic_Context* c, cg_Var* lhs)
{
	switch(getToken(c)) {
	case tokAND:		return doBinaryOpNode(c, lhs, parseExp3, cg_AND);
	default:		return lhs;
	}
}

static cg_Var* 
parseExp5(demobasic_Context* c)
{
	cg_Var* lhs = parseExp7(c);
	return parseExp6(c, lhs);
}

static cg_Var* 
parseExp6(demobasic_Context* c, cg_Var* lhs)
{
	switch(getToken(c)) {
	case tokEQ:		return doBinaryOpNode(c, lhs, parseExp5, cg_EQ);
	case tokNE:		return doBinaryOpNode(c, lhs, parseExp5, cg_NE);
	default:		return lhs;
	}
}

static cg_Var* 
parseExp7(demobasic_Context* c)
{
	cg_Var* lhs = parseExp9(c);
	return parseExp8(c, lhs);
}

static cg_Var* 
parseExp8(demobasic_Context* c, cg_Var* lhs)
{
	switch(getToken(c)) {
	case tokLT:		return doBinaryOpNode(c, lhs, parseExp7, cg_LT);
	case tokGT:		return doBinaryOpNode(c, lhs, parseExp7, cg_GT);
	case tokLE:		return doBinaryOpNode(c, lhs, parseExp7, cg_LE);
	case tokGE:		return doBinaryOpNode(c, lhs, parseExp7, cg_GE);
	default:		return lhs;
	}
}

static cg_Var* 
parseExp9(demobasic_Context* c)
{
	cg_Var* lhs = parseTerm1(c);
	cg_Var* rhs;
	cg_Var* acc = lhs;
	cg_BinOp op;

	for(;;)	
	{
		switch(getToken(c)) {
		case tokPLUS:		op = cg_PLUS; break;
		case tokMINUS:		op = cg_MINUS; break;
		default:		return acc;
		}
		
		eatToken(c);
		rhs = parseTerm1(c);
		acc = cg_newTempVar(c->cg, rhs);
		cg_emitBinOp(c->cg, acc, lhs, op, rhs);
		lhs = acc;
	}
}

static cg_Var* 
parseTerm1(demobasic_Context* c)
{
	cg_Var* lhs = parseFactor(c);
	cg_Var* rhs;
	cg_Var* acc = lhs;
	cg_BinOp op;

	for(;;)	
	{
		switch(getToken(c)) {
		case tokMULT:		op = cg_MULT; break;
		case tokDIV:		op = cg_DIV; break;
		case tokMOD:		op = cg_MOD; break;
		default:		return acc;
		}
		
		eatToken(c);
		rhs = parseFactor(c);
		acc = cg_newTempVar(c->cg, rhs);
		cg_emitBinOp(c->cg, acc, lhs, op, rhs);
		lhs = acc;
	}
}

static const demobasic_ApiEntry*
findFunction(demobasic_Context* c, const char *id)
{
	int i;
	for (i = 0; i < (int)c->apiEntryCount; ++i) {
		if (0 == strcasecmp(id, c->apiEntry[i].id)) {
			return &c->apiEntry[i];
		}
	}
	return 0;
}

static cg_Var*
parseFunction(demobasic_Context* c, const demobasic_ApiEntry* entry, int ignoreReturnValue)
{
	u8 v;
	int i;

	eatToken(c);
	
	if (!entry) {
		parseError(c);
		return 0;
	}

	expectAndEatToken(c, tokLPAR);
	/* Function call */
	cg_emitBeginFuncCall(c->cg, (int)(entry - c->apiEntry));
	i = 0;
	while ((v = entry->argType[i++])) {
		assert(v == cg_Int);
		if (i > 1)
			expectAndEatToken(c, tokCOMMA);
		cg_emitPushArg(c->cg, parseExpression(c));
	}
	expectAndEatToken(c, tokRPAR);
	return cg_emitEndFuncCall(c->cg, ignoreReturnValue);
}

static cg_Var* 
parseFactor(demobasic_Context* c)
{
	cg_Var* temp;
	cg_Var* result = 0;
	if (getToken(c) == tokNUMCONST) {	/* const */
		result = cg_newIntConstant(c->cg, c->lex.numConst);
		eatToken(c);
	} else if (getToken(c) == tokID) {	/* var */
		const demobasic_ApiEntry* entry = findFunction(c, c->lex.id);
		if (entry) {
			/* It's a function call */
			result = parseFunction(c, entry, 0);
		} else {
			/* It's a variable */
			VarDecl* var = findOrFailVarDecl(c, c->lex.id);
			result = var->var;
			eatToken(c);
		}
	} else if (getToken(c) == tokLPAR) {	/* ( exp1 ) */
		eatToken(c);
		result = parseExpression(c);
		expectAndEatToken(c, tokRPAR);
	} else if (getToken(c) == tokMINUS) {	/* -factor */
		eatToken(c);
		temp = parseFactor(c);
		result = cg_newTempVar(c->cg, temp);
		cg_emitUnaryOp(c->cg, result, cg_UNARYMINUS, temp);
	} else if (getToken(c) == tokNOT) {	/* not exp */
		eatToken(c);
		temp = parseFactor(c);
		result = cg_newTempVar(c->cg, temp);
		/* TODO: check integer type */
		cg_emitUnaryOp(c->cg, result, cg_NOT, temp);
	} else {
		parseError(c);
	}
	return result;
}

static void 
parseStm(demobasic_Context* c)
{
	if (getToken(c) == tokID) {
		const demobasic_ApiEntry* entry = findFunction(c, c->lex.id);
		if (entry) {
			/* It's a function call */
			parseFunction(c, entry, 1);
		} else {
			/* It's a variable assignment */
			VarDecl* var = findOrAddVarDecl(c, c->lex.id);
			eatToken(c);
			expectAndEatToken(c, tokEQ);
			cg_emitAssign(c->cg, var->var, parseExpression(c));
		}
	} else if (getToken(c) == tokIF) {
		cg_Var* expr;
		cg_Label* label1 = cg_newTempLabel(c->cg);
		eatToken(c);
		expr = parseExpression(c);
		expectAndEatToken(c, tokTHEN);
		if (getToken(c) == tokNEWLINE) {
			eatToken(c);
			cg_emitIfFalseGoto(c->cg, expr, label1);
			parseBody(c, tokENDIF, tokELSE);
			if (getToken(c) == tokENDIF) {
				eatToken(c);
				cg_emitLabel(c->cg, label1);
			} else if (getToken(c) == tokELSE) {
				cg_Label* label2 = cg_newTempLabel(c->cg);
				eatToken(c);
				cg_emitGoto(c->cg, label2);
				cg_emitLabel(c->cg, label1);
				expectAndEatToken(c, tokNEWLINE);
				parseBody(c, tokENDIF, tokDONTMATCH);
				expectAndEatToken(c, tokENDIF);
				cg_emitLabel(c->cg, label2);
			} else {
				parseError(c);
			}
		} else {
			/* Single line if */
			cg_emitIfFalseGoto(c->cg, expr, label1);
			parseStm(c);
			cg_emitLabel(c->cg, label1);
		}

	} else if (getToken(c) == tokNEWLINE) { /* empty line */
		/* Do nothing here. Handled by parseStmLine */
	} else if (getToken(c) == tokCOLON) { /* label */
		eatToken(c);
		expectToken(c, tokID);
		cg_emitLabel(c->cg, defineLabel(c, c->lex.id));
		eatToken(c);
	} else if (getToken(c) == tokGOTO) {
		eatToken(c);
		expectToken(c, tokID);
		cg_emitGoto(c->cg, referenceLabel(c, c->lex.id));
		eatToken(c);
	} else {
		parseError(c);
	}

}

static void 
parseStmLine(demobasic_Context* c)
{
	parseStm(c);
	expectAndEatToken(c, tokNEWLINE); 
}

static void 
parseBody(demobasic_Context* c, int endToken1, int endToken2)
{
	while (getToken(c) != endToken1 && getToken(c) != endToken2)
		parseStmLine(c);
}

static void 
parseProgram(demobasic_Context* c)
{
	cg_Label* functionName = cg_newLabel(c->cg, "main");
	cg_emitBeginFunc(c->cg, functionName);
	parseBody(c, tokEOF, tokDONTMATCH);
	checkLabelReferences(c);
	cg_emitEndFunc(c->cg);
	cg_finalize(c->cg);
}

mc_MachineCode* 
demobasic_compile(const char* sourceCode, size_t sourceCodeLength, 
		  const demobasic_ApiEntry* api, u32 apiCount, 
		  mem_Allocator* allocator)
{
	const size_t Capacity = 1024*1024;
	demobasic_Context c;
	mc_MachineCode* result = (mc_MachineCode*)(allocator->allocMem)(Capacity);
	memset(result, 0, sizeof(mc_MachineCode));
	result->capacity = Capacity;
	result->size = sizeof(mc_MachineCode);

	c.allocator = allocator;
	ct_fixArrayInit(&c.varDecls);
	ct_fixArrayInit(&c.labels);

	c.apiEntry = api;
	c.apiEntryCount = apiCount;

	c.cg = cg_newContext(allocator, result);
	lex_initContext(&c.lex, sourceCode, sourceCodeLength);
	eatToken(&c);
	parseProgram(&c);

	cg_deleteContext(c.cg);

	/* TODO: trim demobasic_MachineCode (realloc?) */
	return result;
}
