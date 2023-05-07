/*****************************************************************************

Filename:   main/machine/include/web.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:25 $
 * $Header: /home/hbray/cvsroot/fec/main/machine/include/web.h,v 1.2 2011/07/27 20:22:25 hbray Exp $
 *
 $Log: web.h,v $
 Revision 1.2  2011/07/27 20:22:25  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:59  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: web.h,v 1.2 2011/07/27 20:22:25 hbray Exp $ "


#ifndef _PROCWEB_H
#define _PROCWEB_H

#include "include/proclib.h"

extern int ProcWebStart(Proc_t *proc, va_list ap);

#endif
