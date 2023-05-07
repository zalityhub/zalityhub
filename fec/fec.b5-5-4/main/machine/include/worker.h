/*****************************************************************************

Filename:   main/machine/include/worker.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:25 $
 * $Header: /home/hbray/cvsroot/fec/main/machine/include/worker.h,v 1.2 2011/07/27 20:22:25 hbray Exp $
 *
 $Log: worker.h,v $
 Revision 1.2  2011/07/27 20:22:25  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:59  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: worker.h,v 1.2 2011/07/27 20:22:25 hbray Exp $ "


#ifndef _PROCWORKER_H
#define _PROCWORKER_H

#include "include/proclib.h"

extern int ProcWorkerStart(Proc_t *proc, va_list ap);

#endif
