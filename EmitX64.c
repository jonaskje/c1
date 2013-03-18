#include "EmitX64.h"
#include "CodeGenInternal.h"
#include "Memory.h"
#include "Container.h"
#include "MachineCode.h"
#include "Types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

/******************************************************************************/

/* An item on a function's stack */
typedef struct x64_StackEntry x64_StackEntry;
struct x64_StackEntry
{
	cg_Var* var;
	i32 offset;	/* Offset relative RSP */
	size_t size;	/* Size of entry */
};

typedef struct x64_LabelRef x64_LabelRef;
struct x64_LabelRef
{
	const cg_Label* label;		/* The referenced label */
	u32 offset;			/* Offset of reference in generated code */
};

#define x64_MAX_LABELS 1000
#define x64_MAX_LABEL_REFS 2000

typedef struct x64_FunCallState x64_FunCallState;
struct x64_FunCallState
{
	int functionIndex;
	int intArgCount;
	int tempVarCount;
};

typedef struct x64_Context x64_Context;
struct x64_Context 
{
	cg_Backend backend; /* Must be the first member */
	mem_Allocator* allocator;
	ct_FixArray(x64_StackEntry, 256) stackEntries;

	ct_FixArray(const cg_Label*, x64_MAX_LABELS) labels;
	ct_FixArray(size_t, x64_MAX_LABELS) labelOffsets;
	ct_FixArray(x64_LabelRef, x64_MAX_LABEL_REFS) labelRefs;

	size_t stackSize;
	u8* stackSizePatch[2];
	mc_MachineCode* machineCode;
	u8* stream;
	u8* streamCursor;
	u8* streamEnd;

	x64_FunCallState funCallState;

	i32 apiOffsetOnStack;
};

#define x64_getContext(be) ((x64_Context*)(be))

/******************************************************************************/

#define x64_RAX ((u8)0x0u)
#define x64_RCX ((u8)0x1u)
#define x64_RDX ((u8)0x2u)
#define x64_RBX ((u8)0x3u)
#define x64_RSP ((u8)0x4u)
#define x64_RBP ((u8)0x5u)
#define x64_RSI ((u8)0x6u)
#define x64_RDI ((u8)0x7u)
#define x64_R8  ((u8)0x8u)
#define x64_R9  ((u8)0x9u)
#define x64_R10 ((u8)0xAu)
#define x64_R11 ((u8)0xBu)
#define x64_R12 ((u8)0xCu)
#define x64_R13 ((u8)0xDu)
#define x64_R14 ((u8)0xEu)
#define x64_R15 ((u8)0xFu)

static void
writei32(u8* s, i32 v)
{
	u8* p = (u8*)&v;
	*s++ = *p++;
	*s++ = *p++;
	*s++ = *p++;
	*s = *p;
}

static void
writei64(u8* s, i64 v)
{
	u8* p = (u8*)&v;
	*s++ = *p++;
	*s++ = *p++;
	*s++ = *p++;
	*s++ = *p++;
	*s++ = *p++;
	*s++ = *p++;
	*s++ = *p++;
	*s++ = *p++;
	*s = *p;
}

static void
put(x64_Context* c, u8 b)
{
	assert(c->streamCursor < c->streamEnd);
	*c->streamCursor++ = b;
}

static void
puti32(x64_Context* c, i32 b)
{
	assert(c->streamCursor + sizeof(b) <= c->streamEnd);
	writei32(c->streamCursor, b);
	c->streamCursor += sizeof(b);
}

static void
puti64(x64_Context* c, i64 b)
{
	assert(c->streamCursor + sizeof(b) <= c->streamEnd);
	writei64(c->streamCursor, b);
	c->streamCursor += sizeof(b);
}

static void
put2(x64_Context* c, u8 b0, u8 b1)
{
	put(c, b0);
	put(c, b1);
}

static void
put3(x64_Context* c, u8 b0, u8 b1, u8 b2)
{
	put2(c, b0, b1);
	put(c, b2);
}

static void
put4(x64_Context* c, u8 b0, u8 b1, u8 b2, u8 b3)
{
	put2(c, b0, b1);
	put2(c, b2, b3);
}

static u8
rex(u8 w, u8 r, u8 rm)
{
	/* REX is 0100WR0B
	   W = 1 : 64-bit
	   R = 1 : ModRM reg is R8-R15
	   B = 1 : ModRM r/m is R8-R15
	   */
	return 0x40u | (w << 3) | ((r >> 1)&0x4u) | ((rm >> 3)&0x1u);
}

static u8
rexW(u8 r, u8 rm)
{
	return rex(1, r, rm);
}

static u8
modrm(u8 mode, u8 r, u8 rm)
{
	/* MODRM is MMRRRBBB
	   MM is the mode (0-3)
	   R is the lower 3 bits of r
	   B is the lower 3 bits of r/m
	   */
	return (mode << 6) | ((r & 0x7u) << 3) | (rm & 0x7u);
}

static u8
modrmReg(u8 r, u8 rm)
{
	return modrm(3, r, rm);
}
	
static u8
modrmDisp32(u8 r, u8 rm)
{
	return modrm(2, r, rm);
}
	
static void 
e_pushReg(x64_Context* c, u8 reg)	
{ 
	if (reg >= 8)
		put2(c, rex(0, 0, reg), 0x50u + (reg & 0x7u)); 
	else
		put(c, 0x50u+reg); 
}

static void 
e_popReg(x64_Context* c, u8 reg)	
{ 
	if (reg >= 8)
		put2(c, rex(0, 0, reg), 0x58u + (reg & 0x7u)); 
	else
		put(c, 0x58u+reg); 
}

static void 
e_ret(x64_Context* c)		
{ 
	put(c, 0xc3u); 
}

static void 
e_mov_r64_r64(x64_Context* c, u8 rm64, u8 r64)		
{ 
	put3(c, rexW(r64, rm64), 0x89u, modrmReg(r64, rm64)); 
}

static void 
e_mov_m64_r64(x64_Context* c, u8 rm64, i32 offset, u8 r64)		
{ 
	put3(c, rexW(r64, rm64), 0x89u, modrmDisp32(r64, rm64)); 
	puti32(c, offset);
}

static void 
e_mov_r64_m64(x64_Context* c, u8 r64, u8 rm64, i32 offset)		
{ 
	put3(c, rexW(r64, rm64), 0x8bu, modrmDisp32(r64, rm64)); 
	puti32(c, offset);
}

static void 
e_mov_r64_imm64(x64_Context* c, u8 rm64, i64 imm64)		
{ 
	put2(c, rexW(0, rm64), 0xb8u + (rm64 & 0x7u)); 
	puti64(c, imm64);
}

static void
e_sub_r64_imm32(x64_Context* c, u8 rm64, i32 imm32)
{
	/* REX.W + 81 /5 id */
	put3(c, rexW(0, rm64), 0x81u, modrmReg(5, rm64)); 
	puti32(c, imm32);
}

static void
e_add_r64_imm32(x64_Context* c, u8 rm64, i32 imm32)
{
	/* REX.W + 81 /0 id */
	put3(c, rexW(0, rm64), 0x81u, modrmReg(0, rm64)); 
	puti32(c, imm32);
}

static void
e_sub_r64_r64(x64_Context* c, u8 r1, u8 r2)
{
	/* REX.W + 2B /r */
	put3(c, rexW(r1, r2), 0x2bu, modrmReg(r1, r2)); 
}

static void
e_add_r64_r64(x64_Context* c, u8 r1, u8 r2)
{
	put3(c, rexW(r1, r2), 0x03u, modrmReg(r1, r2)); 
}

static void
e_imul_r64_r64(x64_Context* c, u8 r1, u8 r2)
{
	/* Signed multiplication */
	/* REX.W + 0F AF /r */
	put4(c, rexW(r1, r2), 0x0fu, 0xafu, modrmReg(r1, r2)); 
}

static void
e_idiv_r64(x64_Context* c, u8 r1)
{
	/* REX.W + F7 /7 */
	put3(c, rexW(0, r1), 0xf7u, modrmReg(7, r1)); 
}

static void
e_cqo(x64_Context* c)
{
	put2(c, rexW(0, 0), 0x99u);
}

static void
e_xor_r64_r64(x64_Context* c, u8 r1, u8 r2)
{
	/* REX.W + 33 /r */
	put3(c, rexW(r1, r2), 0x33u, modrmReg(r1, r2));
}

static void
e_and_r64_r64(x64_Context* c, u8 r1, u8 r2)
{
	/* REX.W + 23 /r */
	put3(c, rexW(r1, r2), 0x23u, modrmReg(r1, r2));
}

static void
e_or_r64_r64(x64_Context* c, u8 r1, u8 r2)
{
	/* REX.W + 0B /r */
	put3(c, rexW(r1, r2), 0x0bu, modrmReg(r1, r2));
}

static void
e_not_r64(x64_Context* c, u8 reg)
{
	/* REX.W + F7 /2 */
	put3(c, rexW(0, reg), 0xf7u, modrmReg(2, reg)); 
}

static void
e_neg_r64(x64_Context* c, u8 reg)
{
	/* REX.W + F7 /3 */
	put3(c, rexW(0, reg), 0xf7u, modrmReg(3, reg)); 
}

static void
e_jz_rel(x64_Context* c, i32 relAddr)
{
	put2(c, 0x0fu, 0x84u);
	puti32(c, relAddr);
}

static void
e_jmp_rel(x64_Context* c, i32 relAddr)
{
	put(c, 0xe9u);
	puti32(c, relAddr);
}

static void
e_logicalCmp_rax_r64(x64_Context* c, u8 reg, cg_BinOp op)
{
	/*
	cmp	%rax, %reg		REX.W + 39 /r
	set*	%al			0F *                 NE, E, L, G, LE, LG
	andb	%al, $1			24 ib
	movzbl	%rax, %al		REX.W + 0F B6 /r
	*/
	put3(c, rexW(reg, x64_RAX), 0x39u, modrmReg(reg, x64_RAX));
	switch (op) {
	case cg_NE: put3(c, 0x0fu, 0x95u, modrmReg(0, x64_RAX)); break;
	case cg_EQ: put3(c, 0x0fu, 0x94u, modrmReg(0, x64_RAX)); break;
	case cg_LE: put3(c, 0x0fu, 0x9eu, modrmReg(0, x64_RAX)); break;
	case cg_GE: put3(c, 0x0fu, 0x9du, modrmReg(0, x64_RAX)); break;
	case cg_LT: put3(c, 0x0fu, 0x9cu, modrmReg(0, x64_RAX)); break;
	case cg_GT: put3(c, 0x0fu, 0x9fu, modrmReg(0, x64_RAX)); break;
	default: assert(0);
	}
	put2(c, 0x24u, 0x01u);
	put4(c, rexW(x64_RAX, 0), 0x0fu, 0xb6u, modrmReg(x64_RAX, 0)); 
}

static void
e_test_rm64_imm32(x64_Context* c, u8 r1, i32 imm32)
{
	/* REX.W + F7 /0 id */
	put3(c, rexW(0, r1), 0xf7u, modrmReg(0, r1)); 
	puti32(c, imm32);
}
	
static void
e_call_r64(x64_Context* c, u8 reg)
{
	/* FF /2 */
	put2(c, 0xffu, modrmReg(2, reg)); 
}

/******************************************************************************/

static x64_StackEntry*
findStackEntry(x64_Context* c, cg_Var* var)
{
	x64_StackEntry* it;
	ct_fixArrayForEach(&c->stackEntries, it) {
		if (it->var == var)
			return it;
	}
	return 0;
}

static x64_StackEntry*
findRetValStackEntry(x64_Context* c)
{
	x64_StackEntry* it;
	ct_fixArrayForEach(&c->stackEntries, it) {
		if (it->var->flags & cg_IsRetVal)
			return it;
	}
	return 0;
}

static u8
getTempReg(cg_Var* var)
{
	assert(var->kind == cg_TempVariable);
	/* Allowing at most 7 temp variables. r15 is reserved for operations. */
	assert(var->v.i >= 0 && var->v.i < 7);
	return x64_R8 + var->v.i;
}

static void
rememberStackSizePatch(x64_Context* c, int index)
{
	c->stackSizePatch[index] = c->streamCursor - sizeof(i32);
}

static void
patchStackSize(x64_Context* c)
{
	if (c->stackSizePatch[0])
	       writei32(c->stackSizePatch[0], (i32)c->stackSize);
	if (c->stackSizePatch[1])
	       writei32(c->stackSizePatch[1], (i32)c->stackSize);
}

static void
moveVarToReg(x64_Context* c, u8 reg, cg_Var* var)
{
	switch(var->kind) {
	case cg_Constant:
		assert(var->type == cg_Int);
		e_mov_r64_imm64(	c, reg, var->v.i);
		break;
	case cg_Variable: {
		x64_StackEntry* varStackEntry = findStackEntry(c, var);
		assert(varStackEntry);
		e_mov_r64_m64(		c, reg, x64_RBP, varStackEntry->offset);
	}
		break;
	case cg_TempVariable:
		e_mov_r64_r64(		c, reg, getTempReg(var));
		break;
	default:
		assert(0);
		break;
	}
}

static void
moveRegToVar(x64_Context* c, cg_Var* var, u8 reg)
{
	switch(var->kind) {
	case cg_Variable: {
		x64_StackEntry* varStackEntry = findStackEntry(c, var);
		assert(varStackEntry);
		e_mov_m64_r64(		c, x64_RBP, varStackEntry->offset, reg);
	}
		break;
	case cg_TempVariable:
		e_mov_r64_r64(		c, getTempReg(var), reg);
		break;
	case cg_Constant: /* Fall through */
	default:
		assert(0);
		break;
	}
}

static void
addLabel(x64_Context* c, const cg_Label* label)
{
	const cg_Label** labelIterator;
	size_t offset;
	
	/* Sanity check unique labels */
	ct_fixArrayForEach(&c->labels, labelIterator) {
		if ((*labelIterator) == label) 
			assert(0 && "Same label added twice");
	}

	ct_fixArrayPushBack(&c->labels, &label);
	offset = c->streamCursor - c->stream;
	ct_fixArrayPushBack(&c->labelOffsets, &offset);
}

static void
addLabelRef(x64_Context* c, const cg_Label* label, size_t referenceOffset)
{
	const x64_LabelRef ref = { label, referenceOffset };
	ct_fixArrayPushBack(&c->labelRefs, &ref);
}

static void
fixupLabelRefs(x64_Context* c)
{
	x64_LabelRef* refIterator;
	i32 relOffs;
	ct_fixArrayForEach(&c->labelRefs, refIterator) {
		const cg_Label** labelIterator;
		const cg_Label* label = 0;
		ct_fixArrayForEach(&c->labels, labelIterator) {
			if ((*labelIterator) == refIterator->label) {
				label = *labelIterator;
				break;
			}
		}
		if (!label)
			assert(0 && "Some labelRef not found"); /* Should be checked by parser */



		relOffs = c->labelOffsets.a[labelIterator - c->labels.a] - (refIterator->offset +
									    sizeof(i32));
		writei32(c->stream + refIterator->offset, relOffs);
	}
}

/******************************************************************************/

static void 
x64_emitLabel(cg_Backend* backend, cg_Label* label)
{
	x64_Context* c = x64_getContext(backend);	
	addLabel(c, label);
}

static void 
x64_emitBeginFunc(cg_Backend* backend, cg_Label* label)
{
	x64_Context* c = x64_getContext(backend);	
	e_pushReg(		c, x64_RBP);
	e_mov_r64_r64(		c, x64_RBP, x64_RSP);

	/* These registers belong to the caller */
	e_pushReg(		c, x64_RBX);
	e_pushReg(		c, x64_R12);
	e_pushReg(		c, x64_R13);
	e_pushReg(		c, x64_R14);
	e_pushReg(		c, x64_R15);

	/* Add stack space for the pushed registers */
	c->stackSize += 5 * sizeof(void*);

	e_sub_r64_imm32(	c, x64_RSP, 0);
	rememberStackSizePatch(c, 0);

	/* Reserve space for api pointer on the stack */
	c->apiOffsetOnStack = -(c->stackSize + sizeof(void*));
	c->stackSize += sizeof(void*);

	/* Save the api pointer to the stack (the first argument is in RDI) */	
	e_mov_m64_r64(c, x64_RBP, c->apiOffsetOnStack, x64_RDI);
}

static void
x64_emitEndFunc(cg_Backend* backend)
{
	x64_Context* c = x64_getContext(backend);	
	x64_StackEntry* retval = findRetValStackEntry(c);
	if (retval) {
		assert(retval->var->type == cg_Int);
		e_mov_r64_m64(		c, x64_RAX, x64_RBP, retval->offset);
	} else {
		e_mov_r64_imm64(	c, x64_RAX, 0);
	}

	e_add_r64_imm32(	c, x64_RSP, 0);
	rememberStackSizePatch(c, 1);

	e_popReg(		c, x64_R15);
	e_popReg(		c, x64_R14);
	e_popReg(		c, x64_R13);
	e_popReg(		c, x64_R12);
	e_popReg(		c, x64_RBX);

	e_popReg(		c, x64_RBP);
	e_ret(c);	
}

static void
x64_emitAssign(cg_Backend* backend, cg_Var* result, cg_Var* var)
{
	x64_Context* c = x64_getContext(backend);	
	assert(var->type == cg_Int);
	assert(result->type == cg_Int);
	assert(result->kind == cg_Variable || result->kind == cg_TempVariable);

	if (result->kind == cg_Variable) {
		x64_StackEntry* stackEntry = findStackEntry(c, result);
		if (!stackEntry) {
			ct_fixArrayPushBackRaw(&c->stackEntries);
			stackEntry = ct_fixArrayLast(&c->stackEntries);
			stackEntry->var = result;
			stackEntry->size = sizeof(i64);
			stackEntry->offset = -(c->stackSize + (int)stackEntry->size);
			c->stackSize += stackEntry->size;
		} 
		if (var->kind == cg_Constant) {
			e_mov_r64_imm64(	c, x64_RAX, var->v.i);
			e_mov_m64_r64(		c, x64_RBP, stackEntry->offset, x64_RAX);
		} else if (var->kind == cg_Variable) {
			x64_StackEntry* varStackEntry = findStackEntry(c, var);
			assert(varStackEntry);
			e_mov_r64_m64(		c, x64_RAX, x64_RBP, varStackEntry->offset);
			e_mov_m64_r64(		c, x64_RBP, stackEntry->offset, x64_RAX);
		} else {
			assert(var->kind == cg_TempVariable);
			e_mov_m64_r64(		c, x64_RBP, stackEntry->offset, getTempReg(var));
		}
	} else {
		if (var->kind == cg_Constant) {
			e_mov_r64_imm64(	c, getTempReg(result), (i64)var->v.i);
		} else if (var->kind == cg_Variable) {
			x64_StackEntry* varStackEntry = findStackEntry(c, var);
			assert(varStackEntry);
			e_mov_r64_m64(		c, getTempReg(result), x64_RBP, varStackEntry->offset);
		} else {
			assert(var->kind == cg_TempVariable);
			e_mov_r64_r64(		c, getTempReg(result), getTempReg(var));
		}
	}
}

static void
x64_emitBinOp(cg_Backend* backend, cg_Var* result, cg_Var* var1, cg_BinOp op, cg_Var* var2)
{
	x64_Context* c = x64_getContext(backend);	

	/*
	mov rax, <var1>
	<binop> rax, <var2>
	mov <result>, rax
	*/

	moveVarToReg(c, x64_RAX, var1);
	moveVarToReg(c, x64_R15, var2);

	/* <binop> rax, <var2> */
	switch(op) {
	case cg_PLUS:		e_add_r64_r64(		c, x64_RAX, x64_R15);	break;
	case cg_MINUS:		e_sub_r64_r64(		c, x64_RAX, x64_R15);	break;
	case cg_MULT:		e_imul_r64_r64(		c, x64_RAX, x64_R15);	break;
	
	case cg_DIV:
				e_cqo(			c); /* sign extend RAX -> RDX:RAX */
				e_idiv_r64(		c, x64_R15);	
				break;
	
	case cg_MOD:
				e_cqo(			c); /* sign extend RAX -> RDX:RAX */
				e_idiv_r64(		c, x64_R15);	
				e_mov_r64_r64(		c, x64_RAX, x64_RDX); /* Remainder in RDX */
				break;

	case cg_EQ:
	case cg_NE:
	case cg_GT:
	case cg_LT:
	case cg_LE:
	case cg_GE:
				e_logicalCmp_rax_r64(	c, x64_R15, op);	
				break;

	case cg_AND:		e_and_r64_r64(		c, x64_RAX, x64_R15);	break;
	case cg_OR:		e_or_r64_r64(		c, x64_RAX, x64_R15);	break;
	default: assert(0);
	}
	
	moveRegToVar(c, result, x64_RAX);
}

static void
x64_emitUnaryOp(cg_Backend* backend, cg_Var* result, cg_UnaryOp op, cg_Var* var)
{
	x64_Context* c = x64_getContext(backend);	
	moveVarToReg(c, x64_RAX, var);
	switch(op) {
	case cg_NOT:		e_not_r64(		c, x64_RAX);	break;
	case cg_UNARYMINUS:	e_neg_r64(		c, x64_RAX);	break;
	default:		assert(0);				break;
	}
	moveRegToVar(c, result, x64_RAX);
}

static void
x64_emitIfFalseGoto(cg_Backend* backend, cg_Var* var, cg_Label* label)
{
	x64_Context* c = x64_getContext(backend);	
	moveVarToReg(c, x64_RAX, var);
	e_test_rm64_imm32(c, x64_RAX, -1);
	e_jz_rel(c, 0);
	addLabelRef(c, label, (c->streamCursor - c->stream) - sizeof(i32));
}

static void
x64_emitGoto(cg_Backend* backend, cg_Label* label)
{
	x64_Context* c = x64_getContext(backend);	
	e_jmp_rel(c, 0);
	addLabelRef(c, label, (c->streamCursor - c->stream) - sizeof(i32));
}
	
static void 
x64_emitBeginFuncCall(cg_Backend* backend, u32 functionIndex, int tempVarCount)
{
	int i;
	int n;
	x64_Context* c = x64_getContext(backend);	
	assert(c->funCallState.functionIndex == -1);

	c->funCallState.functionIndex = functionIndex;
	c->funCallState.intArgCount = 0;
	
	/* AMD64 */
	/* Integer/pointer args: %rdi, %rsi, %rdx, %rcx, %r8 and %r9 */
	/* Regs that belong to caller: %rbp, %rbx, %r12 - %r15 */
	/* Temp regs that need to be saved: %r8 - %r11 (at most 4 of them) */
	n = tempVarCount > 4 ? 4 : tempVarCount;
	
	/* Always save in pairs to keep the stack aligned */
	if (n > 0)
		n = ((n - 1)/2 + 1)*2;

	/* Save temp registers in use */	
	for (i = 0; i < n; ++i)
		e_pushReg(c, x64_R8 + i);
	
	c->funCallState.tempVarCount = n;
}

static void 
x64_emitPushArg(cg_Backend* backend, cg_Var* var)
{
	x64_Context* c = x64_getContext(backend);	
	assert(c->funCallState.functionIndex != -1);
	assert(var->type == cg_Int);

	switch(c->funCallState.intArgCount) {
	case 0:		moveVarToReg(c, x64_RDI, var); break;
	case 1:		moveVarToReg(c, x64_RSI, var); break;
	case 2:		moveVarToReg(c, x64_RDX, var); break;
	case 3:		moveVarToReg(c, x64_RCX, var); break;
	case 4:		moveVarToReg(c, x64_R8, var); break;
	case 5:		moveVarToReg(c, x64_R9, var); break;
	case 6:		assert(0 && "Too many arguments"); break;
	}
	
	c->funCallState.intArgCount++;
}

static void
x64_emitEndFuncCall(cg_Backend* backend, cg_Var* result)
{
	int i;
	x64_Context* c = x64_getContext(backend);	
	assert(c->funCallState.functionIndex != -1);

	/* Load the api pointer */	
	e_mov_r64_m64(	c, x64_RAX, x64_RBP, c->apiOffsetOnStack);

	/* Load the correct function pointer */
	e_mov_r64_m64(	c, x64_RAX, x64_RAX, c->funCallState.functionIndex * sizeof(void*));

	e_call_r64(	c, x64_RAX);

	/* Restore temp registers */	
	for (i = c->funCallState.tempVarCount - 1; i >= 0; --i)
		e_popReg(c, x64_R8 + i);
	
	if (result) {
		result->type = cg_Int;
		moveRegToVar(c, result, x64_RAX);
	}
	
	c->funCallState.functionIndex = -1;
}

static void
x64_finalize(cg_Backend* backend)
{
	x64_Context* c = x64_getContext(backend);	
	c->machineCode->codeSize = (u32)(c->streamCursor - c->stream);

	/* Align stack size to a 16 bytes boundary */
	/* There is already a return address pushed on the stack by the caller, so we need to
	 * include it in the rounding operation but exclude it from the amount of stack space 
	 * we reserve */
	c->stackSize = ROUND_UP(c->stackSize + 8, 16) - 8;

	patchStackSize(c);
	fixupLabelRefs(c);
}

/******************************************************************************/


cg_Backend*
x64_newBackend(mem_Allocator* allocator, mc_MachineCode* mc)
{
	x64_Context* c = (x64_Context*)(allocator->allocMem)(sizeof(x64_Context));
	cg_Backend* be = &c->backend;
	be->emitLabel = x64_emitLabel;
	be->emitBeginFunc = x64_emitBeginFunc;
	be->emitEndFunc = x64_emitEndFunc;
	be->emitAssign = x64_emitAssign;
	be->emitBinOp = x64_emitBinOp;
	be->emitUnaryOp = x64_emitUnaryOp;
	be->emitIfFalseGoto = x64_emitIfFalseGoto;
	be->emitGoto = x64_emitGoto;
	be->emitBeginFuncCall = x64_emitBeginFuncCall;
	be->emitPushArg = x64_emitPushArg;
	be->emitEndFuncCall = x64_emitEndFuncCall;
	be->finalize = x64_finalize;
	c->allocator = allocator;
	ct_fixArrayInit(&c->stackEntries);
	c->stackSize = 0; 
	c->stackSizePatch[0] = 0;
	c->stackSizePatch[1] = 0;
	
	ct_fixArrayInit(&c->labels);
	ct_fixArrayInit(&c->labelOffsets);
	ct_fixArrayInit(&c->labelRefs);

	c->machineCode = mc;
	mc->codeOffset = (u32)sizeof(mc_MachineCode);
	mc->codeSize = 0;

	c->stream = (u8*)mc + sizeof(mc_MachineCode);
	c->streamCursor = c->stream;
	c->streamEnd = c->stream + mc->capacity - mc->codeOffset;

	c->funCallState.functionIndex = -1;

	return &c->backend;
}


