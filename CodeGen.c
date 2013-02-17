#include "CodeGen.h"
#include "Lex.h"
#include "Container.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

enum ValueType
{
	NumConst,
	Var,
	TempVar
};
typedef enum ValueType ValueType;

typedef struct Value Value;
struct Value
{
	ValueType type;
	union {
		int numConst;
		const char* id;
		int tempIndex;
	} val;
};

struct cg_Label
{
	int locationIndex;
};

struct cg_Context
{
	const mem_Allocator* allocator;
	ct_FixStack(Value, 32) valueStack;
	ct_FixArray(cg_Label, 1000) labels;
	int tempIndex;
	int locationIndex;
};

static char* valueToString(char* buf, const Value* v)
{
	switch(v->type) {
	case NumConst:	sprintf(buf, "%d", v->val.numConst); break;
	case Var:	sprintf(buf, "%s", v->val.id); break;
	case TempVar:	sprintf(buf, "T%d", v->val.tempIndex); break;
	}
	return buf;
}

static const char* binOpToString(cg_BinOp op)
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

static const char* unaryOpToString(cg_UnaryOp op)
{
	switch(op) {
	case cg_UNARYMINUS:		return "-";
	case cg_LOGICALNOT:		return "not";
	}
}

cg_Context* cg_newContext(const mem_Allocator* allocator)
{
	cg_Context* c = (cg_Context*)(allocator->allocMem)(sizeof(cg_Context));
	c->allocator = allocator;
	ct_fixStackInit(&c->valueStack);
	ct_fixArrayInit(&c->labels);
	c->tempIndex = 0;
	c->locationIndex = 0;
	return c;
}

void cg_deleteContext(cg_Context* c)
{
	if (!c)
		return;

	(c->allocator->freeMem)(c);
}

void cg_assign(cg_Context* c)
{
	Value* v1;
	Value* v2;

       	v2 = ct_fixStackTop(&c->valueStack);
	ct_fixStackPop(&c->valueStack);
       	v1 = ct_fixStackTop(&c->valueStack);
	ct_fixStackPop(&c->valueStack);

	c->tempIndex -= (v2->type == TempVar);

	{
		char buf0[lex_MAX_ID_LENGTH + 1];
		char buf1[lex_MAX_ID_LENGTH + 1];
		printf("%s = %s\n", valueToString(buf0, v1), valueToString(buf1, v2));
	}
}

void cg_pushVar(cg_Context* c, const char* id)
{
	Value v;
	v.type = Var;
	v.val.id = id;
	ct_fixStackPush(&c->valueStack, &v);
}

void cg_pushNumConst(cg_Context* c, int value)
{
	Value v;
	v.type = NumConst;
	v.val.numConst = value;
	ct_fixStackPush(&c->valueStack, &v);
}

void cg_pushBinOp(cg_Context* c, cg_BinOp op)
{
	Value* v1;
	Value* v2;
	Value newValue;

       	v2 = ct_fixStackTop(&c->valueStack);
	ct_fixStackPop(&c->valueStack);
       	v1 = ct_fixStackTop(&c->valueStack);
	ct_fixStackPop(&c->valueStack);

	c->tempIndex -= (v1->type == TempVar) + (v2->type == TempVar);

	newValue.type = TempVar;
	newValue.val.tempIndex = c->tempIndex++;	

	{
		char buf0[lex_MAX_ID_LENGTH + 1];
		char buf1[lex_MAX_ID_LENGTH + 1];
		char buf2[lex_MAX_ID_LENGTH + 1];
		printf("%s = %s %s %s\n", valueToString(buf0, &newValue), valueToString(buf1, v1),
		       binOpToString(op), valueToString(buf2, v2));
	}
	
	ct_fixStackPush(&c->valueStack, &newValue);
}

void cg_pushUnaryOp(cg_Context* c, cg_UnaryOp op)
{
	Value* v1;
	Value newValue;

       	v1 = ct_fixStackTop(&c->valueStack);
	ct_fixStackPop(&c->valueStack);

	c->tempIndex -= (v1->type == TempVar);

	newValue.type = TempVar;
	newValue.val.tempIndex = c->tempIndex++;	

	{
		char buf0[lex_MAX_ID_LENGTH + 1];
		char buf1[lex_MAX_ID_LENGTH + 1];
		char buf2[lex_MAX_ID_LENGTH + 1];
		printf("%s = %s %s\n", valueToString(buf0, &newValue), unaryOpToString(op), valueToString(buf1, v1));
	}
	
	ct_fixStackPush(&c->valueStack, &newValue);
}

cg_Label* cg_ifFalseGoto(cg_Context* c)
{
	Value* v1;
	cg_Label label;
	label.locationIndex = c->locationIndex++;
       	
	v1 = ct_fixStackTop(&c->valueStack);
	ct_fixStackPop(&c->valueStack);
	c->tempIndex -= (v1->type == TempVar);

	ct_fixArrayPushBack(&c->labels, &label);
	{
		char buf0[lex_MAX_ID_LENGTH + 1];
		printf("ifFalse %s goto L%04d\n", valueToString(buf0, v1), ct_fixArrayLast(&c->labels)->locationIndex);
	}
	return ct_fixArrayLast(&c->labels);
}

cg_Label* cg_goto(cg_Context* c)
{
	cg_Label label;
	label.locationIndex = c->locationIndex++;
	ct_fixArrayPushBack(&c->labels, &label);
	{
		printf("goto L%04d\n", ct_fixArrayLast(&c->labels)->locationIndex);
	}
	return ct_fixArrayLast(&c->labels);
}

void cg_label(cg_Context* c, cg_Label* label)
{
	{
		printf("L%04d:\n", label->locationIndex);
	}
}

