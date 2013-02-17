#pragma once
#include "Memory.h"

typedef struct cg_Context cg_Context;
typedef struct cg_Label cg_Label;

enum cg_BinOp {
	cg_EQ,
	cg_NE,
	cg_LT,
	cg_GT,
	cg_LE,
	cg_GE,
	cg_LOGICALAND,
	cg_LOGICALOR,
	cg_MULT,
	cg_DIV,
	cg_MOD,
	cg_PLUS,
	cg_MINUS
};
typedef enum cg_BinOp cg_BinOp;

enum cg_UnaryOp {
	cg_UNARYMINUS,
	cg_LOGICALNOT
};
typedef enum cg_UnaryOp cg_UnaryOp;

cg_Context* cg_newContext(const mem_Allocator* allocator);
void cg_deleteContext(cg_Context* c);

void cg_assign(cg_Context* c);
void cg_pushVar(cg_Context* c, const char* id);
void cg_pushNumConst(cg_Context* c, int value);
void cg_pushBinOp(cg_Context* c, cg_BinOp op);
void cg_pushUnaryOp(cg_Context* c, cg_UnaryOp op);
cg_Label* cg_ifFalseGoto(cg_Context* c);
cg_Label* cg_goto(cg_Context* c);
void cg_label(cg_Context* c, cg_Label* label);

