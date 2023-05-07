/*****************************************************************************

Filename:   include/stack.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:54 $
 * $Header: /home/hbray/cvsroot/fec/include/stack.h,v 1.3.4.2 2011/10/27 18:33:54 hbray Exp $
 *
 $Log: stack.h,v $
 Revision 1.3.4.2  2011/10/27 18:33:54  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:38  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:37  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: stack.h,v 1.3.4.2 2011/10/27 18:33:54 hbray Exp $ "


#ifndef _STACK_H
#define _STACK_H


typedef struct Stack_t
{
	char		*name;
	int			size;			// current size
	int			depth;			// current tuple
	void		*invalidTuple;
	void		**tuples;
} Stack_t;


#define StackNew(invalidTuple, name, ...) ObjectNew(Stack, invalidTuple, name, ##__VA_ARGS__)
#define StackVerify(var) ObjectVerify(Stack, var)
#define StackDelete(var) ObjectDelete(Stack, var)


// External Functions
extern Stack_t* StackConstructor(Stack_t *this, char *file, int lno, void *invalidTuple, char *name, ...);
extern void StackDestructor(Stack_t *this, char *file, int lno);
extern BtNode_t* StackNodeList;
extern struct Json_t* StackSerialize(Stack_t *this);
extern char* StackToString(Stack_t *this);

static inline int StackLength(Stack_t *this) { StackVerify(this); return this->depth; }

extern void StackClear(Stack_t *this);
extern void StackPush(Stack_t *this, void *tuple);
extern void* StackPop(Stack_t *this);
extern void* StackTop(Stack_t *this);
extern void* StackBottom(Stack_t *this);
extern void StackGrow(Stack_t *this, int size);

#endif
