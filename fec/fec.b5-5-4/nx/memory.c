/*****************************************************************************

Filename:   lib/nx/memory.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:57 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/memory.c,v 1.3.4.2 2011/10/27 18:33:57 hbray Exp $
 *
 $Log: memory.c,v $
 Revision 1.3.4.2  2011/10/27 18:33:57  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:45  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:46  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: memory.c,v 1.3.4.2 2011/10/27 18:33:57 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/memory.h"



BtNode_t *MemoryNodeList = NULL;


Memory_t*
MemoryConstructor(Memory_t *this, char *file, int lno, int size)
{
	return this;
}


void
MemoryDestructor(Memory_t *this, char *file, int lno)
{
}


char*
MemoryStringDup(char *str)
{
	int len = strlen(str);
	char *new = MemoryNew(len+1);
	strcpy(new, str);
	return new;
}


Json_t*
MemorySerialize(Memory_t *this)
{
	MemoryVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Memory", ObjectStringStamp(this));
	return root;
}


char*
MemoryToString(Memory_t *this)
{
	Json_t *root = MemorySerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}
