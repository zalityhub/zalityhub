/*****************************************************************************

Filename:   include/datagram.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:36:29 $
 * $Header: /home/hbray/cvsroot/fec/include/Attic/datagram.h,v 1.1.2.1 2011/10/27 18:36:29 hbray Exp $
 *
 $Log: datagram.h,v $
 Revision 1.1.2.1  2011/10/27 18:36:29  hbray
 Revision 5.5


 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: datagram.h,v 1.1.2.1 2011/10/27 18:36:29 hbray Exp $ "


#ifndef _DATAGRAM_H
#define _DATAGRAM_H

extern int NxDatagramSend(char *host, int port, char *msg, int mlen);
extern int NxDatagramPrintfV(char *host, int port, char *fmt, va_list ap);
extern int NxDatagramPrintf(char *host, int port, char *fmt, ...);

#endif
