/*****************************************************************************

Filename:   include/t70.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/01 14:49:44 $
 * $Header: /home/hbray/cvsroot/fec/include/t70.h,v 1.3.4.3 2011/09/01 14:49:44 hbray Exp $
 *
 $Log: t70.h,v $
 Revision 1.3.4.3  2011/09/01 14:49:44  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:53:58  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:37  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: t70.h,v 1.3.4.3 2011/09/01 14:49:44 hbray Exp $ "


#ifndef _T70_H
#define _T70_H

#define		T70MaxPacketLen			(128 * 1024)
typedef struct PiPeekBfr_t
{
	unsigned int	pktlen;		// NOT self inclusive... network ordering
	struct
	{
		char			proxyEcho[10];
		char			uid[10];
		unsigned short	servicePort; // network ordering
	} hdr ;
	char			data[T70MaxPacketLen];		// datalen is pktlen-sizeof(hdr)
} T70Packet_t ;

#endif
