/*****************************************************************************

Filename:	include/locklib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:12 $
 * $Header: /home/hbray/cvsroot/fec/include/locklib.h,v 1.2 2011/07/27 20:22:12 hbray Exp $
 *
 $Log: locklib.h,v $
 Revision 1.2  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:35  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: locklib.h,v 1.2 2011/07/27 20:22:12 hbray Exp $ "


#ifndef _LOCKLIB_H
#define	_LOCKLIB_H

extern boolean FileLock(int lockFd, boolean wait);
extern boolean FileUnlock(int lockFd);

#endif
