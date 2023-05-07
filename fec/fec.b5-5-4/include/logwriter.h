/*****************************************************************************

Filename:   include/logwriter.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/08/01 16:11:28 $
 * $Header: /home/hbray/cvsroot/fec/include/logwriter.h,v 1.2.4.1 2011/08/01 16:11:28 hbray Exp $
 *
 $Log: logwriter.h,v $
 Revision 1.2.4.1  2011/08/01 16:11:28  hbray
 *** empty log message ***

 Revision 1.2  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:35  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: logwriter.h,v 1.2.4.1 2011/08/01 16:11:28 hbray Exp $ "


#ifndef _LOGWRITER_H
#define _LOGWRITER_H

extern void ConsoleRecycle (SysLog_t *this, void **contextp, boolean force);
extern void ConsoleWriter (SysLog_t *this, void **contextp, SysLogLevel lvl, char *file, int lno, char *fnc, char *bfr, int len);

#endif
