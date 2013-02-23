#include "EmitC.h"
#include "CodeGenInternal.h"
#include "Memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

/******************************************************************************/

typedef struct ec_Context ec_Context;
struct ec_Context 
{
	cg_Backend backend; /* Must be the first member */
	mem_Allocator* allocator;
	int indent;
};

#define ec_getContext(be) ((ec_Context*)(be))

/******************************************************************************/

static void
printLabel(cg_Label* label)
{
	if (label->id == 0)
		printf("L%04d", label->tempId);
	else
		printf("%s", label->id);
}

static void
printVar(cg_Var* var)
{
	switch(var->kind) {
	case cg_TempVariable:		printf("T%d", var->v.i); break;
	case cg_Variable:		printf("%s", var->v.s); break;
	case cg_IntConstant:		printf("%d", var->v.i); break;
	case cg_NumberConstant:		printf("%f", var->v.d); break;
	case cg_StringConstant:		printf("\"%s\"", var->v.s); break;
	default:			assert(0);
	}
}

static const char* 
binOpToString(cg_BinOp op)
{
	switch(op) {
	case cg_EQ:		return "=";
	case cg_NE:		return "<>";
	case cg_LT:		return "<";
	case cg_GT:		return ">";
	case cg_LE:		return "<=";
	case cg_GE:		return ">=";
	case cg_LOGICALAND:	return "and";
	case cg_LOGICALOR:	return "or";
	case cg_MULT:		return "*";
	case cg_DIV:		return "/";
	case cg_MOD:		return "%";
	case cg_PLUS:		return "+";
	case cg_MINUS:		return "-";
	}
}

static const char* 
unaryOpToString(cg_UnaryOp op)
{
	switch(op) {
	case cg_UNARYMINUS:		return "-";
	case cg_LOGICALNOT:		return "not";
	}
}

const char* getIndentString(int n) 
{
	static const char space[] = "                                ";
	static const int len = sizeof(space)/sizeof(space[0]);
	n = 4 * n;
	if (n >= len)
		return space;
	else
		return space + len - n;
}

/******************************************************************************/

static void 
ec_emitLabel(cg_Backend* backend, cg_Label* label)
{
	ec_Context* c = ec_getContext(backend);	
}

static void 
ec_emitBeginFunc(cg_Backend* backend, cg_Label* label)
{
	ec_Context* c = ec_getContext(backend);	

	printf("%sint ", getIndentString(c->indent));
	printLabel(label);
	printf("(void) \n%s{\n", getIndentString(c->indent));

	++c->indent;
}

static void
ec_emitEndFunc(cg_Backend* backend)
{
	ec_Context* c = ec_getContext(backend);	
	
	printf("%s}\n\n", getIndentString(--c->indent));
}

static void
ec_emitAssign(cg_Backend* backend, cg_Var* result, cg_Var* var)
{
	ec_Context* c = ec_getContext(backend);	
	assert(var->type == cg_Int);
	assert(result->type == cg_Int);
	/*printf("%sint %s = %s\n", getIndentString(c->indent), */
}

static void
ec_emitBinOp(cg_Backend* backend, cg_Var* result, cg_Var* var1, cg_BinOp op, cg_Var* var2)
{
	ec_Context* c = ec_getContext(backend);	
}

static void
ec_emitUnaryOp(cg_Backend* backend, cg_Var* result, cg_UnaryOp op, cg_Var* var)
{
	ec_Context* c = ec_getContext(backend);	
}

static void
ec_emitIfFalseGoto(cg_Backend* backend, cg_Var* var, cg_Label* label)
{
	ec_Context* c = ec_getContext(backend);	
}

static void
ec_emitGoto(cg_Backend* backend, cg_Label* label)
{
	ec_Context* c = ec_getContext(backend);	
}

/******************************************************************************/


cg_Backend*
ec_newCCodeBackend(mem_Allocator* allocator)
{
	ec_Context* c = (ec_Context*)(allocator->allocMem)(sizeof(ec_Context));
	cg_Backend* be = &c->backend;
	be->emitLabel = ec_emitLabel;
	be->emitBeginFunc = ec_emitBeginFunc;
	be->emitEndFunc = ec_emitEndFunc;
	be->emitAssign = ec_emitAssign;
	be->emitBinOp = ec_emitBinOp;
	be->emitUnaryOp = ec_emitUnaryOp;
	be->emitIfFalseGoto = ec_emitIfFalseGoto;
	be->emitGoto = ec_emitGoto;
	c->allocator = allocator;
	c->indent = 0;
	return &c->backend;
}
