#include "EmitC.h"
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
typedef struct ec_StackEntry ec_StackEntry;
struct ec_StackEntry
{
	cg_Var* var;
	i32 offset;	/* Offset relative RSP */
	size_t size;	/* Size of entry */
};

typedef struct ec_Context ec_Context;
struct ec_Context 
{
	cg_Backend backend; /* Must be the first member */
	mem_Allocator* allocator;
	ct_FixArray(ec_StackEntry, 256) stackEntries;
	size_t stackSize;
	u8* stackSizePatch[2];
	mc_MachineCode* machineCode;
	u8* stream;
	u8* streamCursor;
	u8* streamEnd;
};

#define ec_getContext(be) ((ec_Context*)(be))

/******************************************************************************/

#define ec_RAX ((u8)0x0u)
#define ec_RCX ((u8)0x1u)
#define ec_RDX ((u8)0x2u)
#define ec_RBX ((u8)0x3u)
#define ec_RSP ((u8)0x4u)
#define ec_RBP ((u8)0x5u)
#define ec_RSI ((u8)0x6u)
#define ec_RDI ((u8)0x7u)
#define ec_R8  ((u8)0x8u)
#define ec_R9  ((u8)0x9u)
#define ec_R10 ((u8)0xAu)
#define ec_R11 ((u8)0xBu)
#define ec_R12 ((u8)0xCu)
#define ec_R13 ((u8)0xDu)
#define ec_R14 ((u8)0xEu)
#define ec_R15 ((u8)0xFu)

#define ec_REX_W ((u8)0x48u)

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
put(ec_Context* c, u8 b)
{
	assert(c->streamCursor < c->streamEnd);
	*c->streamCursor++ = b;
}

static void
puti32(ec_Context* c, i32 b)
{
	assert(c->streamCursor + sizeof(b) <= c->streamEnd);
	writei32(c->streamCursor, b);
	c->streamCursor += sizeof(b);
}

static void
puti64(ec_Context* c, i64 b)
{
	assert(c->streamCursor + sizeof(b) <= c->streamEnd);
	writei64(c->streamCursor, b);
	c->streamCursor += sizeof(b);
}

static void
put2(ec_Context* c, u8 b0, u8 b1)
{
	put(c, b0);
	put(c, b1);
}

static void
put3(ec_Context* c, u8 b0, u8 b1, u8 b2)
{
	put2(c, b0, b1);
	put(c, b2);
}

static void
put4(ec_Context* c, u8 b0, u8 b1, u8 b2, u8 b3)
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
e_pushReg(ec_Context* c, u8 reg)	
{ 
	put(c, 0x50u+reg); 
}

static void 
e_popReg(ec_Context* c, u8 reg)	
{ 
	put(c, 0x58u+reg); 
}

static void 
e_ret(ec_Context* c)		
{ 
	put(c, 0xc3u); 
}

static void 
e_mov_r64_r64(ec_Context* c, u8 rm64, u8 r64)		
{ 
	put3(c, rexW(r64, rm64), 0x89u, modrmReg(r64, rm64)); 
}

static void 
e_mov_m64_r64(ec_Context* c, u8 rm64, i32 offset, u8 r64)		
{ 
	put3(c, rexW(r64, rm64), 0x89u, modrmDisp32(r64, rm64)); 
	puti32(c, offset);
}

static void 
e_mov_r64_m64(ec_Context* c, u8 r64, u8 rm64, i32 offset)		
{ 
	put3(c, rexW(r64, rm64), 0x8bu, modrmDisp32(r64, rm64)); 
	puti32(c, offset);
}

static void 
e_mov_r64_imm64(ec_Context* c, u8 rm64, i64 imm64)		
{ 
	put2(c, rexW(0, rm64), 0xb8u + (rm64 & 0x7u)); 
	puti64(c, imm64);
}

static void
e_sub_r64_imm32(ec_Context* c, u8 rm64, i32 imm32)
{
	/* REX.W + 81 /5 id */
	put3(c, rexW(0, rm64), 0x81u, modrmReg(5, rm64)); 
	puti32(c, imm32);
}

static void
e_add_r64_imm32(ec_Context* c, u8 rm64, i32 imm32)
{
	/* REX.W + 81 /0 id */
	put3(c, rexW(0, rm64), 0x81u, modrmReg(0, rm64)); 
	puti32(c, imm32);
}

static void
e_sub_r64_r64(ec_Context* c, u8 r1, u8 r2)
{
	/* REX.W + 2B /r */
	put3(c, rexW(r1, r2), 0x2bu, modrmReg(r1, r2)); 
}

static void
e_add_r64_r64(ec_Context* c, u8 r1, u8 r2)
{
	put3(c, rexW(r1, r2), 0x03u, modrmReg(r1, r2)); 
}

static void
e_imul_r64_r64(ec_Context* c, u8 r1, u8 r2)
{
	/* Signed multiplication */
	/* REX.W + 0F AF /r */
	put4(c, rexW(r1, r2), 0x0fu, 0xafu, modrmReg(r1, r2)); 
}

static void
e_idiv_r64(ec_Context* c, u8 r1)
{
	/* REX.W + F7 /7 */
	put3(c, rexW(0, r1), 0xf7u, modrmReg(7, r1)); 
}

static void
e_xor_r64_r64(ec_Context* c, u8 r1, u8 r2)
{
	/* REX.W + 33 /r */
	put3(c, rexW(r1, r2), 0x33u, modrmReg(r1, r2));
}

/******************************************************************************/

static ec_StackEntry*
findStackEntry(ec_Context* c, cg_Var* var)
{
	ec_StackEntry* it;
	ct_fixArrayForEach(&c->stackEntries, it) {
		if (it->var == var)
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
	return ec_R8 + var->v.i;
}

static void
rememberStackSizePatch(ec_Context* c, int index)
{
	c->stackSizePatch[index] = c->streamCursor - sizeof(i32);
}

static void
patchStackSize(ec_Context* c)
{
	if (c->stackSizePatch[0])
	       writei32(c->stackSizePatch[0], (i32)c->stackSize);
	if (c->stackSizePatch[1])
	       writei32(c->stackSizePatch[1], (i32)c->stackSize);
}

static void
moveVarToReg(ec_Context* c, u8 reg, cg_Var* var)
{
	switch(var->kind) {
	case cg_Constant:
		assert(var->type == cg_Int);
		e_mov_r64_imm64(	c, reg, var->v.i);
		break;
	case cg_Variable: {
		ec_StackEntry* varStackEntry = findStackEntry(c, var);
		assert(varStackEntry);
		e_mov_r64_m64(		c, reg, ec_RBP, varStackEntry->offset);
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
moveRegToVar(ec_Context* c, cg_Var* var, u8 reg)
{
	switch(var->kind) {
	case cg_Variable: {
		ec_StackEntry* varStackEntry = findStackEntry(c, var);
		assert(varStackEntry);
		e_mov_m64_r64(		c, ec_RBP, varStackEntry->offset, reg);
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
	e_pushReg(		c, ec_RBP);
	e_mov_r64_r64(		c, ec_RBP, ec_RSP);
	e_sub_r64_imm32(	c, ec_RSP, 0);
	rememberStackSizePatch(c, 0);
}

static void
ec_emitEndFunc(cg_Backend* backend)
{
	ec_Context* c = ec_getContext(backend);	
	/* Temp: Move the first variable to rax */
	if (ct_fixArraySize(&c->stackEntries) > 0) {
		ec_StackEntry* entry = ct_fixArrayItem(&c->stackEntries, 0);
		assert(entry->var->type == cg_Int);
		e_mov_r64_m64(		c, ec_RAX, ec_RBP, entry->offset);
	}
	e_add_r64_imm32(	c, ec_RSP, 0);
	rememberStackSizePatch(c, 1);
	e_popReg(		c, ec_RBP);
	e_ret(c);	
}

static void
ec_emitAssign(cg_Backend* backend, cg_Var* result, cg_Var* var)
{
	ec_Context* c = ec_getContext(backend);	
	assert(var->type == cg_Int);
	assert(result->type == cg_Int);
	assert(result->kind == cg_Variable || result->kind == cg_TempVariable);

	if (result->kind == cg_Variable) {
		ec_StackEntry* stackEntry = findStackEntry(c, result);
		if (!stackEntry) {
			ct_fixArrayPushBackRaw(&c->stackEntries);
			stackEntry = ct_fixArrayLast(&c->stackEntries);
			stackEntry->var = result;
			stackEntry->size = sizeof(i64);
			stackEntry->offset = -(c->stackSize + (int)stackEntry->size);
			c->stackSize += stackEntry->size;
		} 
		if (var->kind == cg_Constant) {
			e_mov_r64_imm64(	c, ec_RAX, var->v.i);
			e_mov_m64_r64(		c, ec_RBP, stackEntry->offset, ec_RAX);
		} else if (var->kind == cg_Variable) {
			ec_StackEntry* varStackEntry = findStackEntry(c, var);
			assert(varStackEntry);
			e_mov_r64_m64(		c, ec_RAX, ec_RBP, varStackEntry->offset);
			e_mov_m64_r64(		c, ec_RBP, stackEntry->offset, ec_RAX);
		} else {
			assert(var->kind == cg_TempVariable);
			e_mov_m64_r64(		c, ec_RBP, stackEntry->offset, getTempReg(var));
		}
	} else {
		if (var->kind == cg_Constant) {
			e_mov_r64_imm64(	c, getTempReg(result), (i64)var->v.i);
		} else if (var->kind == cg_Variable) {
			ec_StackEntry* varStackEntry = findStackEntry(c, var);
			assert(varStackEntry);
			e_mov_r64_m64(		c, getTempReg(result), ec_RBP, varStackEntry->offset);
		} else {
			assert(var->kind == cg_TempVariable);
			e_mov_r64_r64(		c, getTempReg(result), getTempReg(var));
		}
	}
}

static void
ec_emitBinOp(cg_Backend* backend, cg_Var* result, cg_Var* var1, cg_BinOp op, cg_Var* var2)
{
	ec_Context* c = ec_getContext(backend);	

	/*
	mov rax, <var1>
	<binop> rax, <var2>
	mov <result>, rax
	*/

	moveVarToReg(c, ec_RAX, var1);
	moveVarToReg(c, ec_R15, var2);

	/* <binop> rax, <var2> */
	switch(op) {
	case cg_PLUS:		e_add_r64_r64(		c, ec_RAX, ec_R15);	break;
	case cg_MINUS:		e_sub_r64_r64(		c, ec_RAX, ec_R15);	break;
	case cg_MULT:		e_imul_r64_r64(		c, ec_RAX, ec_R15);	break;
	case cg_DIV:
				e_xor_r64_r64(		c, ec_RDX, ec_RDX);
				e_idiv_r64(		c, ec_R15);	
				break;
	case cg_MOD:
				e_xor_r64_r64(		c, ec_RDX, ec_RDX);
				e_idiv_r64(		c, ec_R15);	
				e_mov_r64_r64(		c, ec_RAX, ec_RDX); /* Remainder in RDX */
				break;
	default: assert(0);
	}
	
	moveRegToVar(c, result, ec_RAX);
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

static void
ec_finalize(cg_Backend* backend)
{
	ec_Context* c = ec_getContext(backend);	
	c->machineCode->codeSize = (u32)(c->streamCursor - c->stream);
	patchStackSize(c);
}

/******************************************************************************/


cg_Backend*
ec_newCCodeBackend(mem_Allocator* allocator, mc_MachineCode* mc)
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
	be->finalize = ec_finalize;
	c->allocator = allocator;
	ct_fixArrayInit(&c->stackEntries);
	c->stackSize = 8; /* Caller pushes return address, so need to align to 16 bytes stack */
	c->stackSizePatch[0] = 0;
	c->stackSizePatch[1] = 0;

	c->machineCode = mc;
	mc->codeOffset = (u32)sizeof(mc_MachineCode);
	mc->codeSize = 0;

	c->stream = (u8*)mc + sizeof(mc_MachineCode);
	c->streamCursor = c->stream;
	c->streamEnd = c->stream + mc->capacity - mc->codeOffset;

	return &c->backend;
}


