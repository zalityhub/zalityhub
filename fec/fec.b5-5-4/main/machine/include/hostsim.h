/*****************************************************************************

Filename:   main/machine/include/hostsim.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:25 $
 * $Header: /home/hbray/cvsroot/fec/main/machine/include/hostsim.h,v 1.2 2011/07/27 20:22:25 hbray Exp $
 *
 $Log: hostsim.h,v $
 Revision 1.2  2011/07/27 20:22:25  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:58  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: hostsim.h,v 1.2 2011/07/27 20:22:25 hbray Exp $ "


#ifndef _PROCHGSIM_H
#define _PROCHGSIM_H

#include "include/proclib.h"

extern int ProcHgSimStart(Proc_t *proc, va_list ap);

#endif
