#include "CodeGen.h"
#include "Lex.h"
#include "Container.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>


struct cg_Label
{
	const char* id;
};

enum cg_Kind
{
	cg_Variable,
	cg_IntConstant,
	cg_StringConstant,
	cg_NumberConstant
};
typedef enum cg_Kind cg_Kind;

struct cg_Var
{
	cg_Type type;
	cg_Kind kind;
	union {
		const char* s;
		struct {
			size_t isSet : 1;
			size_t id : (sizeof(size_t)*8 - 1);
		} temp;
		int i;
		double d;
	} v;
};

#define cg_MAX_LABELS 1000
#define cg_MAX_VARS 1000

struct cg_Context
{
	const mem_Allocator* allocator;
	ct_FixArray(cg_Label, cg_MAX_LABELS) labels;
	ct_FixArray(cg_Var, cg_MAX_VARS) vars;

	/* Id of next allocated variable id */	
	int tempVariableId;
};

/******************************************************************************/
static void
useTempVar(cg_Context* c, cg_Var* var)
{
	if (var->kind == cg_Variable && var->v.temp.isSet)
	{
		assert(c->tempVariableId > 0);
		--c->tempVariableId;
	}
}

static void
printLabel(cg_Label* label)
{
	printf("%s", label->id);
}

static void
printVar(cg_Var* var)
{
	switch(var->kind) {
	case cg_Variable:
		if (var->v.temp.isSet)
			printf("T%lu", var->v.temp.id);
		else
			printf("%s", var->v.s);
		break;
	case cg_IntConstant:
		printf("%d", var->v.i);
		break;
	case cg_NumberConstant:
		printf("%f", var->v.d);
		break;
	case cg_StringConstant:
		printf("\"%s\"", var->v.s);
		break;
	default:
		assert(0);
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
/******************************************************************************/

cg_Context* 
cg_newContext(const mem_Allocator* allocator)
{
	cg_Context* c = (cg_Context*)(allocator->allocMem)(sizeof(cg_Context));
	c->allocator = allocator;
	ct_fixArrayInit(&c->labels);
	ct_fixArrayInit(&c->vars);

	/* test */
	{
		cg_Var var;
		var.v.s = 0;
		var.v.temp.isSet = 1u;
		assert((size_t)(var.v.s) == 1u);
		return c;
	}
}

void 
cg_deleteContext(cg_Context* c)
{
	if (!c)
		return;

	(c->allocator->freeMem)(c);
}

cg_Label* 
cg_newLabel(cg_Context* c, const char* name)
{
	assert(ct_fixArraySize(&c->labels) < cg_MAX_LABELS);
	ct_fixArrayPushBackRaw(&c->labels);
	ct_fixArrayLast(&c->labels)->id = name;
	return ct_fixArrayLast(&c->labels);
}

cg_Var*	
cg_newVar(cg_Context* c, const char* name, cg_Type type)
{
	cg_Var* v;
	assert(ct_fixArraySize(&c->vars) < cg_MAX_VARS);
	ct_fixArrayPushBackRaw(&c->vars);
	v = ct_fixArrayLast(&c->vars);
	v->type = type;
	v->kind = cg_Variable;
	if (name) {
		v->v.s = name;
	} else {
		v->v.temp.isSet = 1u;
		v->v.temp.id = c->tempVariableId++;
	}

	return v;
}

cg_Var*		
cg_newTempVar(cg_Context* c, cg_Type type)
{
	return cg_newVar(c, 0, type);
}

cg_Var*	
cg_newIntConstant(cg_Context* c, int value)
{
	cg_Var* v;
	assert(ct_fixArraySize(&c->vars) < cg_MAX_VARS);
	ct_fixArrayPushBackRaw(&c->vars);
	v = ct_fixArrayLast(&c->vars);
	v->type = cg_Int;
	v->kind = cg_IntConstant;
	v->v.i = value;
	return v;
}

cg_Var*	
cg_newNumberConstant(cg_Context* c, double value)
{
	cg_Var* v;
	assert(ct_fixArraySize(&c->vars) < cg_MAX_VARS);
	ct_fixArrayPushBackRaw(&c->vars);
	v = ct_fixArrayLast(&c->vars);
	v->type = cg_Number;
	v->kind = cg_NumberConstant;
	v->v.d = value;
	return v;
}

cg_Var*	
cg_newStringConstant(cg_Context* c, const char* value)
{
	cg_Var* v;
	assert(ct_fixArraySize(&c->vars) < cg_MAX_VARS);
	ct_fixArrayPushBackRaw(&c->vars);
	v = ct_fixArrayLast(&c->vars);
	v->type = cg_String;
	v->kind = cg_StringConstant;
	v->v.s = value;
	return v;
}

void
cg_emitLabel(cg_Context* c, cg_Label* label)
{
	printLabel(label);
	printf(":\n");
}

void
cg_emitBeginFunc(cg_Context* c)
{
	printf("\tbeginFunc\n");
}

void
cg_emitEndFunc(cg_Context* c)
{
	printf("\tendFunc\n");
}

void
cg_emitAssign(cg_Context* c, cg_Var* result, cg_Var* var)
{
	useTempVar(c, var);

	assert(var->type != cg_Auto);
	if (result->type == cg_Auto) {
		result->type = var->type;
	} else {
		assert(result->type == var->type);
	}
	
	printf("\t");
	printVar(result);
	printf(" = ");
	printVar(var);
	printf("\n");
}

void
cg_emitBinOp(cg_Context* c, cg_Var* result, cg_Var* var1, cg_BinOp op, cg_Var* var2)
{
	useTempVar(c, var1);
	useTempVar(c, var2);

	assert(var1->type != cg_Auto);
	assert(var2->type != cg_Auto);
	assert(var1->type == var2->type);
	if (result->type == cg_Auto) {
		result->type = var1->type;
	} else {
		assert(result->type == var1->type);
	}
	
	printf("\t");
	printVar(result);
	printf(" = ");
	printVar(var1);
	printf(" %s ", binOpToString(op));
	printVar(var2);
	printf("\n");
}

void
cg_emitUnaryOp(cg_Context* c, cg_Var* result, cg_UnaryOp op, cg_Var* var)
{
	useTempVar(c, var);
	
	assert(var->type != cg_Auto);
	if (result->type == cg_Auto) {
		result->type = var->type;
	} else {
		assert(result->type == var->type);
	}

	printf("\t");
	printVar(result);
	printf(" = ");
	printf(" %s", unaryOpToString(op));
	printVar(var);
	printf("\n");
}

void
cg_emitIfFalseGoto(cg_Context* c, cg_Var* var, cg_Label* label)
{
	useTempVar(c, var);
	
	assert(var->type != cg_Auto);
	
	printf("\tifFalse ");
	printVar(var);
	printf(" then goto ");
	printLabel(label);
	printf("\n");
}

void
cg_emitGoto(cg_Context* c, cg_Label* label)
{
	printf("\tgoto ");
	printLabel(label);
	printf("\n");
}


