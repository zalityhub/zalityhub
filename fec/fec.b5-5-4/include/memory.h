/*****************************************************************************

Filename:   include/memory.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:53 $
 * $Header: /home/hbray/cvsroot/fec/include/memory.h,v 1.3.4.2 2011/10/27 18:33:53 hbray Exp $
 *
 $Log: memory.h,v $
 Revision 1.3.4.2  2011/10/27 18:33:53  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:37  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:36  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: memory.h,v 1.3.4.2 2011/10/27 18:33:53 hbray Exp $ "


#ifndef _MEMORY_H
#define _MEMORY_H


typedef struct Memory_t
{
} Memory_t ;


#define MemoryNew(size) ((void*)ObjectNewSized(Memory, size))
#define MemoryNewShared(fname, size) ((void*)ObjectNewSharedSized(fname, Memory, size))
#define MemoryVerify(var) ObjectVerify(Memory, var)
#define MemoryDelete(var) ObjectDelete(Memory, var)

extern Memory_t* MemoryConstructor(Memory_t *this, char *file, int lno, int size);
extern void MemoryDestructor(Memory_t *this, char *file, int lno);
extern BtNode_t* MemoryNodeList;
extern struct Json_t* MemorySerialize(Memory_t *this);
extern char* MemoryToString(Memory_t *this);

extern char* MemoryStringDup(char *str);

#endif
