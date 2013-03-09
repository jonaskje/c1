#pragma once
#include "Types.h"
#include "Memory.h"

typedef struct cg_Context	cg_Context;
typedef struct cg_Label		cg_Label;
typedef struct cg_Var		cg_Var;
struct mc_MachineCode;

enum cg_BinOp {
	cg_EQ,
	cg_NE,
	cg_LT,
	cg_GT,
	cg_LE,
	cg_GE,
	cg_AND,
	cg_OR,
	cg_MULT,
	cg_DIV,
	cg_MOD,
	cg_PLUS,
	cg_MINUS
};
typedef enum cg_BinOp cg_BinOp;

enum cg_UnaryOp {
	cg_UNARYMINUS,
	cg_NOT
};
typedef enum cg_UnaryOp cg_UnaryOp;

enum cg_Type {
	cg_Auto,
	cg_Int,
	cg_String,
	cg_Number
};
typedef enum cg_Type cg_Type;

enum cg_VarFlags
{
	cg_IsRetVal = 1 << 0
};
typedef enum cg_VarFlags cg_VarFlags;

cg_Context*	cg_newContext(		mem_Allocator* allocator, struct mc_MachineCode* mc);
void		cg_deleteContext(	cg_Context* c);
void		cg_finalize(		cg_Context* c);

cg_Label*	cg_newLabel(		cg_Context* c, const char* name);
cg_Label*	cg_newTempLabel(	cg_Context* c);
cg_Var*		cg_newVar(		cg_Context* c, const char* name, cg_Type type, u32 flags);
cg_Var*		cg_newTempVar(		cg_Context* c, cg_Var* inheritType);
cg_Var*		cg_newIntConstant(	cg_Context* c, int value);
cg_Var*		cg_newNumberConstant(	cg_Context* c, double value);
cg_Var*		cg_newStringConstant(	cg_Context* c, const char* value);

void		cg_emitLabel(		cg_Context* c, cg_Label* label);
void		cg_emitBeginFunc(	cg_Context* c, cg_Label* label);
void		cg_emitEndFunc(		cg_Context* c);
void		cg_emitAssign(		cg_Context* c, cg_Var* result, cg_Var* var);
void		cg_emitBinOp(		cg_Context* c, cg_Var* result, cg_Var* var1, cg_BinOp op, cg_Var* var2);
void		cg_emitUnaryOp(		cg_Context* c, cg_Var* result, cg_UnaryOp op, cg_Var* var);
void		cg_emitIfFalseGoto(	cg_Context* c, cg_Var* var, cg_Label* label);
void		cg_emitGoto(		cg_Context* c, cg_Label* label);

cg_Type		cg_varType(		cg_Var* var);
u32		cg_varFlags(		cg_Var* var);	/* One or more of cg_VarFlags */

