#include "Lex.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>

static void nextChar(lex_Context* c)
{
	if (c->sourceCodeCursor == c->sourceCodeEnd) {
		c->c = -1;
		return;
	}

	c->c = *c->sourceCodeCursor++;

	if (c->c == '\r') {
	} else if (c->c == '\n') {
		++c->line;
		c->col = 1;
	} else {
		++c->col;
	}
}

static void prevChar(lex_Context* c)
{
	assert(c->sourceCodeCursor != c->sourceCodeBegin);
	if (c->sourceCodeCursor != c->sourceCodeEnd) /* Stick to EOF */
		--c->sourceCodeCursor;
}

static int token(lex_Context* c, int tok)
{
	c->tok = tok;
	return c->tok;
}

static int tokenNumConst(lex_Context* c, const char* value)
{
	char tmpvalue[MAX_NUMCONST_LENGTH];
	long v;
	strncpy(tmpvalue, value, c->sourceCodeCursor - value);
	tmpvalue[MAX_NUMCONST_LENGTH - 1] = 0;
	v = strtol(tmpvalue, 0, 10);
	if (errno == EINVAL || errno == ERANGE)
		return token(c, tokERROR_NUMCONST_OVERFLOW);
	if (sizeof(long) > sizeof(int))
		if (v > INT_MAX || v < INT_MIN)
			return token(c, tokERROR_NUMCONST_OVERFLOW);
	c->numConst = (int)v;
	return token(c, tokNUMCONST);
}

static int tokenId(lex_Context* c, const char* idBegin)
{
	const int len = c->sourceCodeCursor - idBegin;
	strncpy(c->id, idBegin, len);
	assert(len < MAX_ID_LENGTH);
	c->id[len] = 0;
	if      (0 == strcmp(c->id, "if"))		return token(c, tokIF);
	else if (0 == strcmp(c->id, "then"))		return token(c, tokTHEN);	
	else if (0 == strcmp(c->id, "else"))		return token(c, tokELSE);	
	else if (0 == strcmp(c->id, "endif"))		return token(c, tokENDIF);	
	else						return token(c, tokID);
}

lex_Context* lex_initContext(lex_Context* c, const char* sourceCode, size_t sourceCodeLength)
{
	memset(c, 0, sizeof(lex_Context));
	c->sourceCodeBegin = c->sourceCodeCursor = sourceCode;
	c->sourceCodeEnd = sourceCode + sourceCodeLength;
	c->tok = tokEOF;
	c->line = 1;
	c->col = 1;
	return c;
}


int lex_nextToken(lex_Context* c)
{
	nextChar(c);
	if (c->c == '_' || isalpha(c->c))
	{
		const char* idBegin = c->sourceCodeCursor - 1;
		while(c->sourceCodeCursor - idBegin < MAX_ID_LENGTH)
		{
			nextChar(c);
			if (!(c->c == '_' || isalnum(c->c)))
			{
				prevChar(c);
				return tokenId(c, idBegin);
			}
		}
		return token(c, tokERROR_IDENTIFIER_TOO_LONG);
	}
	if (isnumber(c->c))
	{
		const char* constBegin = c->sourceCodeCursor - 1;
		while(c->sourceCodeCursor - constBegin < MAX_NUMCONST_LENGTH)
		{
			nextChar(c);
			if (!isnumber(c->c))
			{
				prevChar(c);
				return tokenNumConst(c, constBegin);
			}
		}
		return token(c, tokERROR_NUMCONST_OVERFLOW);
	}
	switch(c->c)
	{
	case -1  : return token(c, tokEOF);
	case '\r': /* Fall through */
	case '\n':
		for(;;) {
			nextChar(c);
			if (c->c != '\r' && c->c != '\n') {
				prevChar(c);
				break;
			}
		}
		return token(c, tokNEWLINE);
	case '+' : return token(c, tokPLUS);
	case '-' : return token(c, tokMINUS);
	case '*' : return token(c, tokMULT);
	case '/' : return token(c, tokDIV);
	case '%' : return token(c, tokMOD);
	case '&' : return token(c, tokAND);
	case '|' : return token(c, tokOR);
	case '=' : return token(c, tokEQ);
	case '<' :
		nextChar(c);
		if (c->c == '>')
			return token(c, tokNE);
		else if (c->c == '=')
			return token(c, tokLE);
		else
			prevChar(c);
		return token(c, tokLT);
	case '>' :
		nextChar(c);
		if (c->c == '=')
			return token(c, tokGE);
		else
			prevChar(c);
		return token(c, tokGT);
	case ' ' : return token(c, tokWHITE);
	case '\t': return token(c, tokWHITE);
	case '(' : return token(c, tokRPAR);
	case ')' : return token(c, tokLPAR);
	default  : return token(c, tokERROR_ILLEGAL_CHARACTER);		   
	}	
}

