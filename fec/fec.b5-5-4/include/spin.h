/*****************************************************************************

Filename:   include/spin.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:38 $
 * $Header: /home/hbray/cvsroot/fec/include/spin.h,v 1.3.4.1 2011/09/24 17:49:38 hbray Exp $
 *
 $Log: spin.h,v $
 Revision 1.3.4.1  2011/09/24 17:49:38  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:37  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: spin.h,v 1.3.4.1 2011/09/24 17:49:38 hbray Exp $ "


#ifndef _SPIN_H
#define _SPIN_H

extern void SpinInit(Spin_t *lock);
extern void SpinLock(Spin_t *lock);
extern void SpinUnlock(Spin_t *lock);

#endif
