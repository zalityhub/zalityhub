/*****************************************************************************

Filename:   lib/nx/parseobj.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:58 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/parseobj.c,v 1.3.4.2 2011/10/27 18:33:58 hbray Exp $
 *
 $Log: parseobj.c,v $
 Revision 1.3.4.2  2011/10/27 18:33:58  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:46  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:47  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: parseobj.c,v 1.3.4.2 2011/10/27 18:33:58 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/parseobj.h"



#ifndef isblank
#define isblank(_c) ( (_c)==' ' || (_c) == '\t' )
#endif


BtNode_t *ParserNodeList = NULL;



Parser_t*
ParserConstructor(Parser_t *this, char *file, int lno, int size)
{
	this->bfr = StringNew(size);
	this->stack = StackNew((void*)-1, NULL);
	ParserClear(this);
	return this;
}


void
ParserDestructor(Parser_t *this, char *file, int lno)
{

	if (this->token != NULL)
		free(this->token);		// release previous token

	while ( this->stack->depth > 0 )
	{
		StackPop(this->stack);		// pop len
		StackPop(this->stack);		// pop next pointer
		free(StackPop(this->stack));		// the saved token
	}
	StackDelete(this->stack);
	StringDelete(this->bfr);
}


Json_t*
ParserSerialize(Parser_t *this)
{
	ParserVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	JsonAddBoolean(root, "Eof", this->eof);
	JsonAddItem(root, "Stack", StackSerialize(this->stack));
	JsonAddItem(root, "Bfr", StringSerialize(this->bfr));
	JsonAddString(root, "Token", this->token);
	return root;
}


char*
ParserToString(Parser_t *this)
{
	Json_t *root = ParserSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


void
ParserRewind(Parser_t *this)
{
	ParserVerify(this);
	this->next = this->bfr->str;
	this->len = this->setLen;
}


void
ParserClear(Parser_t *this)
{
	ParserVerify(this);
	StringClear(this->bfr);
	this->next = this->bfr->str;
	this->len = 0;
}


int
ParserSetInputData(Parser_t *this, char *bfr, int plen)
{

	ParserVerify(this);

	StringCpy(this->bfr, bfr);
	this->len = plen;
	this->setLen = this->len;
	return plen;
}


#if 0
int
ParserSetInputFile(Parser_t *this, int f)
{
	ParserVerify(this);

	int rlen = this->maxsize - this->len;

	if (rlen <= 0)
		return rlen;			// unable to read

	char *bfr = alloca(rlen + 10);

	this->eof = 0;

	int ii = read(f, bfr, rlen);

	if (ii >= 0)
	{
		bfr[ii] = '\0';			// terminate input string
		ii = ParserSetInputData(this, bfr, ii);
	}

	if (ii <= 0 && errno == 0)
		this->eof = 1;

	return ii;
}
#endif


void
ParserNormalizeInput(Parser_t *this)
{
	ParserVerify(this);

	char *inp = this->next;

	if (this->len > 0)		// some
	{
		// replace control chars with a space
		char *ptr;
		for (ptr = inp; *ptr;)
		{
			if (*ptr < ' ' || *ptr > '~')
			{
				strcpy(ptr, ptr + 1);
				--this->len;
			}
			else
			{
				++ptr;			// keep char
			}
		}
		// strip multi-space (consecutive spaces) down to single space
		while (this->len > 0 && ((ptr = strchr(inp, ' ')) != NULL && isspace(ptr[1])))
		{
			strcpy(ptr, ptr + 1);
			--this->len;
		}
		// strip leading spaces
		while (this->len > 0 && *inp == ' ')
		{
			strcpy(inp, inp + 1);
			--this->len;
		}
		// strip trailing spaces
		for (char *ptr = &inp[this->len - 1]; ptr >= inp && *ptr == ' ';)
		{
			*ptr-- = '\0';
			--this->len;
		}
	}

	this->next = inp;
	this->len = strlen(inp);
	this->setLen = this->len;
}


char*
ParserGetNextToken(Parser_t *this, char *delimeters)
{
	ParserVerify(this);

	StackPush(this->stack, this->token);
	StackPush(this->stack, this->next);
	StackPush(this->stack, (void*)this->len);

	this->token = NULL;

	// eat blanks
	while (isblank(*this->next))
	{
		++this->next;
		--this->len;
	}

	if (this->len <= 0)
		return "";				// none

	int ch = *this->next;
	char quoted = '\0';
	int len = 0;

	if (ch == '\\')				// escape
	{
		if (--this->len <= 0)
			return "";			// none
		ch = *(++this->next);
	}
	else if (ch == '\'' || ch == '"')
	{
		if (--this->len <= 0)	// no more input
		{
			this->token = calloc(1, 10);
			this->token[0] = *this->next++;	// just the open quote, no close
			return this->token;
		}
		quoted = *this->next++;
		char *ptr = strchr(this->next, quoted);

		if (ptr == NULL)		// no terminating end-quote; go to end of bfr
			ptr = &this->next[this->len];
		len = ptr - this->next;
	}
	else						// not a quoted token
	{
		if (ch != '\0' && strchr(delimeters, ch) != NULL)	// the next char is a delimeter
			len = 1;
		else
			len = strcspn(this->next, delimeters);
	}

	if (len <= 0)
		return "";				// none

	this->token = calloc(1, len + 10);
	memcpy(this->token, this->next, len);
	this->next += len;		// skip the token
	if ( (this->len -= len) > 0 && quoted == *this->next )	// need to close the quoted string
	{
		++this->next;
		--this->len;
	}

	if ( this->len > 0 && strchr(delimeters, *this->next ) != NULL )		// hit a delimeter
	{
		++this->next;
		--this->len;
	}

	return this->token;
}


char*
ParserUnGetToken(Parser_t *this, char *delimeters)
{
	if ( this->stack->depth <= 0 )
		return NULL;		// nothing saved

	this->len = (int)StackPop(this->stack);		// pop len
	this->next = StackPop(this->stack);			// pop next pointer
	this->token = StackPop(this->stack);		// the saved token
	return ParserGetNextToken(this, delimeters);
}


char*
ParserGetFullString(Parser_t *this)
{
	ParserVerify(this);
	return this->bfr->str;
}


char*
ParserGetString(Parser_t *this)
{
	ParserVerify(this);
	return this->next;
}


char *
ParserDownshift(Parser_t *this, char *string)
{

	ParserVerify(this);

	for (char *s = string; *s; ++s)
		*s = tolower(*s);

	return string;
}
