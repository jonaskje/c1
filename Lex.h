#pragma once

#include <stdlib.h>

#define MAX_ID_LENGTH 255
#define MAX_NUMCONST_LENGTH 12

enum {
	tokERROR_NUMCONST_OVERFLOW = -4,
	tokERROR_ILLEGAL_CHARACTER = -3,
	tokERROR_IDENTIFIER_TOO_LONG = -2,
	tokEOF = -1,
	tokNEWLINE,
	tokWHITE,
	tokID,
	tokNUMCONST,
	tokLPAR,
	tokRPAR,
	tokPLUS,
	tokMINUS,
	tokMULT,
	tokDIV,
	tokMOD,
	tokAND,
	tokOR,
	tokEQ,
	tokNE,
	tokLT,
	tokGT,
	tokLE,
	tokGE,

	tokIF,
	tokTHEN,
	tokELSE,
	tokENDIF,
};

typedef struct lex_Context lex_Context;
struct lex_Context
{
	const char* sourceCodeBegin;
	const char* sourceCodeEnd;
	const char* sourceCodeCursor;
	int line;				/* current line */
	int col;				/* current column */
	int c;					/* current character */

	int tok;				/* kind of token from enum above */

	/* Values */
	int numConst;				/* tokNUMCONST */
	char id[MAX_ID_LENGTH + 1];		/* tokID */
};

lex_Context* lex_initContext(lex_Context* c, const char* sourceCode, size_t sourceCodeLength);
int lex_nextToken(lex_Context* c);
