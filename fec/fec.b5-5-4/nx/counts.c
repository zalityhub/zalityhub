/*****************************************************************************

Filename:   lib/nx/counts.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:43 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/counts.c,v 1.2.4.1 2011/09/24 17:49:43 hbray Exp $
 *
 $Log: counts.c,v $
 Revision 1.2.4.1  2011/09/24 17:49:43  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:44  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: counts.c,v 1.2.4.1 2011/09/24 17:49:43 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"


Json_t*
CountsSerialize(Counts_t counts)
{
	Json_t *root = JsonNew(__FUNC__);
	JsonAddNumber(root, "InPkts", counts.inPkts);
	JsonAddNumber(root, "OutPkts", counts.outPkts);
	JsonAddNumber(root, "InChrs", counts.inChrs);
	JsonAddNumber(root, "OutChrs", counts.outChrs);
	return root;
}


char*
CountsToString(Counts_t this)
{
	Json_t *root = CountsSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}
