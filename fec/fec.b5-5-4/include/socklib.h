/*****************************************************************************

Filename:   include/socklib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:38 $
 * $Header: /home/hbray/cvsroot/fec/include/socklib.h,v 1.3.4.2 2011/09/24 17:49:38 hbray Exp $
 *
 $Log: socklib.h,v $
 Revision 1.3.4.2  2011/09/24 17:49:38  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/11 19:47:32  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:37  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: socklib.h,v 1.3.4.2 2011/09/24 17:49:38 hbray Exp $ "


#ifndef _SOCKETLIB_H
#define _SOCKETLIB_H

#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>

// External Functions

extern int SockAccept(EventFile_t *this, EventFile_t *socket);
extern int SockBytesReady(EventFile_t *this);
extern int SockCheckSocket(EventFile_t *this);
extern int SockSocketPair(EventFile_t *evf1, EventFile_t *evf2, int type);
extern int SockClose(EventFile_t *this);
extern int SockShutdown(EventFile_t *this);
extern int SockConnect(EventFile_t *this, char *destname, int port, int domain, int type);
extern boolean SockIsConnected(EventFile_t *this);
extern int SockInit(void);
extern int SockListen(EventFile_t *ld, char *hostname, int port, int domain, int type);
extern int SockParseAddress(char *strAddr, struct sockaddr_in *addr);
extern int SockRecvRaw(EventFile_t *this, char *bfr, int len, int flags);
extern int SockRecvRawExact(EventFile_t *this, char *bfr, int len, int flags, int ms);
extern int SockSendRaw(EventFile_t *this, char *bfr, int len, int flags);
extern int SockRecvPkt(EventFile_t *this, char *pkt, int pktLen, int flags);
extern int SockSendPkt(EventFile_t *this, char *pkt, int pktLen, int flags);
extern int SockSetSocketOptions(EventFile_t *this, int maxPktLen);
extern int SockSetWait(EventFile_t *this);
extern int SockSingleWait(EventFile_t *this, EventPollMask_t pollMask, int ms);
extern int SockDupConnection(EventFile_t *this, EventFile_t *socket);
extern int SockSendConnection(EventFile_t *this, EventFile_t *socket);
extern int SockRecvConnection(EventFile_t *this, EventFile_t *socket);
extern char *SockIpAddrToString(unsigned char *addr, int port, char *out);
extern CommandResult_t SockWatchCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);

#endif
