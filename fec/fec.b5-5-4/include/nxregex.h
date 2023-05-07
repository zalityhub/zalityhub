/*****************************************************************************

Filename:   include/nxregex.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:53 $
 * $Header: /home/hbray/cvsroot/fec/include/Attic/nxregex.h,v 1.1.2.2 2011/10/27 18:33:53 hbray Exp $
 *
 $Log: nxregex.h,v $
 Revision 1.1.2.2  2011/10/27 18:33:53  hbray
 Revision 5.5

 Revision 1.1.2.1  2011/09/24 17:51:26  hbray
 New

 Revision 1.3.4.3  2011/08/23 19:53:57  hbray
 eliminate fecplugin.h

 Revision 1.3.4.2  2011/08/23 12:03:13  hbray
 revision 5.5

 Revision 1.3.4.1  2011/08/15 19:12:31  hbray
 5.5 revisions

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:33  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: nxregex.h,v 1.1.2.2 2011/10/27 18:33:53 hbray Exp $ "


#ifndef _NXREGEX_H
#define _NXREGEX_H

#include <regex.h>

typedef struct NxRegex_t
{
	void		*context;
	boolean		compiled;
	boolean		ignoreCase;
	String_t	*expression;
	regex_t		regex;
} NxRegex_t ;

#define NxRegexNew(context) ObjectNew(NxRegex, context)
#define NxRegexVerify(var) ObjectVerify(NxRegex, var)
#define NxRegexDelete(var) ObjectDelete(NxRegex, var)

extern NxRegex_t* NxRegexConstructor(NxRegex_t *this, char *file, int lno, void *context);
extern void NxRegexDestructor(NxRegex_t *this, char *file, int lno);
extern BtNode_t* NxRegexNodeList;
extern struct Json_t* NxRegexSerialize(NxRegex_t *this);
extern char* NxRegexToString(NxRegex_t *this);
extern int NxRegexCompile(NxRegex_t *this, char *expression, String_t *response);
extern int NxRegexExecute(NxRegex_t *this, char *target);

#endif
