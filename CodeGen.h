#pragma once
#include "Memory.h"

typedef struct cg_Context	cg_Context;
typedef struct cg_Label		cg_Label;
typedef struct cg_Var		cg_Var;

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

enum cg_Type {
	cg_Auto,
	cg_Int,
	cg_String,
	cg_Number
};
typedef enum cg_Type cg_Type;

cg_Context*	cg_newContext(		const mem_Allocator* allocator);
void		cg_deleteContext(	cg_Context* c);

cg_Label*	cg_newLabel(		cg_Context* c, const char* name);
cg_Var*		cg_newVar(		cg_Context* c, const char* name, cg_Type type);
cg_Var*		cg_newTempVar(		cg_Context* c, cg_Type type);
cg_Var*		cg_newIntConstant(	cg_Context* c, int value);
cg_Var*		cg_newNumberConstant(	cg_Context* c, double value);
cg_Var*		cg_newStringConstant(	cg_Context* c, const char* value);

void		cg_emitLabel(		cg_Context* c, cg_Label* label);
void		cg_emitBeginFunc(	cg_Context* c);
void		cg_emitEndFunc(		cg_Context* c);
void		cg_emitAssign(		cg_Context* c, cg_Var* result, cg_Var* var);
void		cg_emitBinOp(		cg_Context* c, cg_Var* result, cg_Var* var1, cg_BinOp op, cg_Var* var2);
void		cg_emitUnaryOp(		cg_Context* c, cg_Var* result, cg_UnaryOp op, cg_Var* var);
void		cg_emitIfFalseGoto(	cg_Context* c, cg_Var* var, cg_Label* label);
void		cg_emitGoto(		cg_Context* c, cg_Label* label);

