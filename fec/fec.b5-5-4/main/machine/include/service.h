/*****************************************************************************

Filename:   main/machine/include/service.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:25 $
 * $Header: /home/hbray/cvsroot/fec/main/machine/include/service.h,v 1.2 2011/07/27 20:22:25 hbray Exp $
 *
 $Log: service.h,v $
 Revision 1.2  2011/07/27 20:22:25  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:59  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: service.h,v 1.2 2011/07/27 20:22:25 hbray Exp $ "


#ifndef _PROCSERVICE_H
#define _PROCSERVICE_H

#include "include/proclib.h"

extern int ProcServiceStart(Proc_t *proc, va_list ap);

#endif
