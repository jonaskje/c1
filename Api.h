#pragma once

#include "Types.h"

typedef struct api_FunctionDesc api_FunctionDesc;
struct api_FunctionDesc
{
	u8 returnType;
	const char* id;
	u8 argType[8];
};

typedef struct api_Functions api_Functions;
struct api_Functions 
{
	int count;
	api_FunctionDesc* descs;
	const void** functions;
};

api_Functions api_getFunctions();
void api_shutdown(void);
