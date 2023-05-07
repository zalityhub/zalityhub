/*****************************************************************************

Filename:   lib/nx/stack.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:59 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/stack.c,v 1.3.4.2 2011/10/27 18:33:59 hbray Exp $
 *
 $Log: stack.c,v $
 Revision 1.3.4.2  2011/10/27 18:33:59  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:47  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:48  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: stack.c,v 1.3.4.2 2011/10/27 18:33:59 hbray Exp $ "

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/stack.h"


static const int StackPadSize = 16;



BtNode_t *StackNodeList = NULL;


Stack_t*
StackConstructor(Stack_t *this, char *file, int lno, void *invalidTuple, char *name, ...)
{
	{
		va_list ap;
		va_start(ap, name);
		StringNewStatic(tmp, 32);
		StringSprintfV(tmp, name, ap);
		this->name = strdup(tmp->str);
	}

	this->invalidTuple = invalidTuple;
	this->size = StackPadSize;
	if ( (this->tuples = calloc(StackPadSize, sizeof(*this->tuples))) == NULL )		// initial allocation
		SysLog(LogFatal, "No memory for %s", this->name);
	return this;
}


void
StackDestructor(Stack_t *this, char *file, int lno)
{
	free(this->name);
	free(this->tuples);
}


Json_t*
StackSerialize(Stack_t *this)
{
	StackVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddNumber(root, "Size", this->size);
	JsonAddNumber(root, "Depth", this->depth);
	return root;
}


char*
StackToString(Stack_t *this)
{
	Json_t *root = StackSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


void
StackClear(Stack_t *this)
{
	StackVerify(this);
	this->depth = 0;
}


void
StackGrow(Stack_t *this, int size)
{
	StackVerify(this);
	this->size += size;		// increase
	if ( (this->tuples = (void**)realloc(this->tuples, (this->size*sizeof(*this->tuples)))) == NULL )
		SysLog(LogFatal, "No memory for %s %d", this->name, this->size);
}


void
StackPush(Stack_t *this, void *tuple)
{
	StackVerify(this);

	if ((this->depth + 1) > this->size)	// need more bfr space
		StackGrow(this, StackPadSize);

	this->tuples[this->depth++] = tuple;
}


void*
StackPop(Stack_t *this)
{
	StackVerify(this);

	if ( --this->depth < 0 )
	{
		SysLog(LogError, "Stack underflow in %s", this->name);
		this->depth = 0;
		return this->invalidTuple;
	}

	return this->tuples[this->depth];
}


void*
StackTop(Stack_t *this)
{
	StackVerify(this);

	if ( this->depth <= 0 )
	{
		SysLog(LogError, "Stack underflow in %s", this->name);
		return this->invalidTuple;
	}

	return this->tuples[this->depth-1];
}


void*
StackBottom(Stack_t *this)
{
	StackVerify(this);

	if ( this->depth <= 0 )
	{
		SysLog(LogError, "Stack underflow in %s", this->name);
		return this->invalidTuple;
	}

	return this->tuples[0];
}
