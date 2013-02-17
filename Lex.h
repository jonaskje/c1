#pragma once

#include <stdlib.h>

#define lex_MAX_ID_LENGTH 255
#define lex_MAX_NUMCONST_LENGTH 12

enum lex_Token {
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
	tokLOGICALAND,
	tokLOGICALOR,
	tokLOGICALNOT,

	tokIF,
	tokTHEN,
	tokELSE,
	tokENDIF,

	tokDONTMATCH = 2000000 /* Lexer will never return this token value */
};
typedef enum lex_Token lex_Token;

typedef struct lex_Context lex_Context;
struct lex_Context
{
	const char* sourceCodeBegin;
	const char* sourceCodeEnd;
	const char* sourceCodeCursor;
	int line;				/* current line */
	int col;				/* current column */
	int c;					/* current character */

	lex_Token tok;				/* kind of token from enum above */

	/* Values */
	int numConst;				/* tokNUMCONST */
	char id[lex_MAX_ID_LENGTH + 1];		/* tokID */
};

lex_Context* lex_initContext(lex_Context* c, const char* sourceCode, size_t sourceCodeLength);
lex_Token lex_nextToken(lex_Context* c);
