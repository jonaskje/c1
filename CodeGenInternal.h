#pragma once

#include "CodeGen.h"

struct cg_Label
{
	const char* id;
	int tempId;
};

enum cg_Kind
{
	cg_Variable,
	cg_TempVariable,
	cg_Constant
};
typedef enum cg_Kind cg_Kind;

struct cg_Var
{
	cg_Type type;
	cg_Kind kind;
	u32 flags;			/* One or more of cg_VarFlags */
	union {
		const char* s;
		int i;
		double d;
	} v;
};

typedef struct cg_Backend cg_Backend;
struct cg_Backend
{
	void (*emitLabel)(cg_Backend* c, cg_Label* label);
	void (*emitBeginFunc)(cg_Backend* c, cg_Label* label);
	void (*emitEndFunc)(cg_Backend* c);
	void (*emitAssign)(cg_Backend* c, cg_Var* result, cg_Var* var);
	void (*emitBinOp)(cg_Backend* c, cg_Var* result, cg_Var* var1, cg_BinOp op, cg_Var* var2);
	void (*emitUnaryOp)(cg_Backend* c, cg_Var* result, cg_UnaryOp op, cg_Var* var);
	void (*emitIfFalseGoto)(cg_Backend* c, cg_Var* var, cg_Label* label);
	void (*emitGoto)(cg_Backend* c, cg_Label* label);

	void (*finalize)(cg_Backend* c);
};

