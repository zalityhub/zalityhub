/*****************************************************************************

Filename:	include/hostrequest.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:52 $
 * $Header: /home/hbray/cvsroot/fec/include/hostrequest.h,v 1.3.4.4 2011/10/27 18:33:52 hbray Exp $
 *
 $Log: hostrequest.h,v $
 Revision 1.3.4.4  2011/10/27 18:33:52  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/24 17:49:36  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/01 14:49:43  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:35  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: hostrequest.h,v 1.3.4.4 2011/10/27 18:33:52 hbray Exp $ "


#ifndef _HOSTREQUEST_H
#define _HOSTREQUEST_H

// Host Service Types:
//
typedef enum
{
	eSvcConfig			= '0',
	eSvcAuth			= '1',
	eSvcEdc				= '2',
	eSvcEdcMulti		= '3',
	eSvcEdcCapms		= '4',
	eSvcT70				= '9'
} HostSvcType_t;



typedef struct HostRequestHeader_t
{
	HostSvcType_t		svcType;
	boolean				more;					// true for additional packets in this sequence
	NxUid_t				peerUid;
	char				peerName[MaxNameLen];
	char				peerIpAddr[8];
	char				peerIpType[1];
	unsigned short		peerIpPort;
	NxUid_t				requestEcho;
} HostRequestHeader_t ;


#define MaxHostRequestDataLen MaxSockPacketLen	// Max request data packet

typedef struct HostRequest_t
{
	HostRequestHeader_t	hdr;
	int					len;					// length of data[] in bytes
	char				data[MaxHostRequestDataLen];	// request data packet
} HostRequest_t;

#define HostRequestLen(rq) ((sizeof(HostRequest_t)-sizeof((rq)->data)) + (rq)->len)


extern char *HostSvcTypeToString(HostSvcType_t type);
extern char *HostRequestHeaderToString(HostRequestHeader_t *req);
extern struct Json_t *HostRequestHeaderSerialize(HostRequestHeader_t *req);
extern struct Json_t *HostRequestSerialize(HostRequest_t *req, OutputType_t outputType);
extern char *HostRequestToString(HostRequest_t *req, OutputType_t outputType);

#endif
