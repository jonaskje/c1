#pragma once

enum tac_Op
{
	tac_ILoad,
	tac_IStore,
	tac_IAdd,
	tac_ISub,
	tac_IMul,
	tac_IDiv,
	tac_IMod,
	tac_JumpZero,
	tac_Jump
};
typedef enum tac_Op tac_Op;

enum tac_AddressType
{
	tac_None,
	tac_Int
	/*tac_String, */
};
typedef enum tac_AddressType tac_AddressType;

typedef struct tac_Address tac_Address;
struct tac_Address
{
	tac_AddressType adressType;
	union 
	{
		int ival;
		const char* id;
	} value;
};

typedef struct tac_Entry tac_Entry;
struct tac_Entry 
{
	/* result = operand1 op operand2 */
	tac_Op op;
	tac_Address operand1;
	tac_Address operand2;
	tac_Address result;
};


