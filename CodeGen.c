#include "CodeGen.h"
#include "CodeGenInternal.h"
#include "Lex.h"
#include "Container.h"
#include "EmitC.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>


#define cg_MAX_LABELS 1000
#define cg_MAX_VARS 1000

struct cg_Context
{
	const mem_Allocator* allocator;
	ct_FixArray(cg_Label, cg_MAX_LABELS) labels;
	ct_FixArray(cg_Var, cg_MAX_VARS) vars;

	/* Id of next allocated variable id */	
	int tempVariableId;

	/* Id of next allocated temp label */	
	int tempLabelId;

	cg_Backend* backend;
};

/******************************************************************************/
static void
useTempVar(cg_Context* c, cg_Var* var)
{
	if (var->kind == cg_TempVariable)
	{
		assert(c->tempVariableId > 0);
		--c->tempVariableId;
	}
}

/******************************************************************************/

cg_Context* 
cg_newContext(mem_Allocator* allocator)
{
	cg_Context* c = (cg_Context*)(allocator->allocMem)(sizeof(cg_Context));
	c->allocator = allocator;
	ct_fixArrayInit(&c->labels);
	ct_fixArrayInit(&c->vars);
	c->tempVariableId = 0;
	c->tempLabelId = 0;
	c->backend = ec_newCCodeBackend(allocator);
	return c;
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
	cg_Label* label;
	assert(ct_fixArraySize(&c->labels) < cg_MAX_LABELS);
	ct_fixArrayPushBackRaw(&c->labels);
	label = ct_fixArrayLast(&c->labels);
	label->id = name;
	label->tempId = name ? 0 : c->tempLabelId++;
	return ct_fixArrayLast(&c->labels);
}

cg_Label*	
cg_newTempLabel(cg_Context* c)
{
	return cg_newLabel(c, 0);
}

cg_Var*	
cg_newVar(cg_Context* c, const char* name, cg_Type type)
{
	cg_Var* v;
	assert(ct_fixArraySize(&c->vars) < cg_MAX_VARS);
	ct_fixArrayPushBackRaw(&c->vars);
	v = ct_fixArrayLast(&c->vars);
	v->type = type;
	if (name) {
		v->kind = cg_Variable;
		v->v.s = name;
	} else {
		v->kind = cg_TempVariable;
		v->v.i = -1; /* Will be initialized when assigned to */
	}

	return v;
}

cg_Var*		
cg_newTempVar(cg_Context* c, cg_Var* inheritType)
{
	return cg_newVar(c, 0, inheritType->type);
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
	c->backend->emitLabel(c->backend, label);
}

void
cg_emitBeginFunc(cg_Context* c, cg_Label* label)
{
	c->backend->emitBeginFunc(c->backend, label);
}

void
cg_emitEndFunc(cg_Context* c)
{
	c->backend->emitEndFunc(c->backend);
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

	if (result->kind == cg_TempVariable)
		result->v.i = c->tempVariableId++;
	
	c->backend->emitAssign(c->backend, result, var);
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
	
	if (result->kind == cg_TempVariable)
		result->v.i = c->tempVariableId++;
	
	
	c->backend->emitBinOp(c->backend, result, var1, op, var2);
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

	if (result->kind == cg_TempVariable)
		result->v.i = c->tempVariableId++;
	
	
	c->backend->emitUnaryOp(c->backend, result, op, var);
}

void
cg_emitIfFalseGoto(cg_Context* c, cg_Var* var, cg_Label* label)
{
	useTempVar(c, var);
	
	assert(var->type != cg_Auto);
	
	c->backend->emitIfFalseGoto(c->backend, var, label);
}

void
cg_emitGoto(cg_Context* c, cg_Label* label)
{
	c->backend->emitGoto(c->backend, label);
}


cg_Type		
cg_varType(cg_Context* c, cg_Var* var)
{
	(void)c;
	return var->type;
}

