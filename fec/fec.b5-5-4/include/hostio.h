/*****************************************************************************

Filename:   include/hostio.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:36 $
 * $Header: /home/hbray/cvsroot/fec/include/Attic/hostio.h,v 1.1.2.4 2011/09/24 17:49:36 hbray Exp $
 *
 $Log: hostio.h,v $
 Revision 1.1.2.4  2011/09/24 17:49:36  hbray
 Revision 5.5

 Revision 1.1.2.2  2011/09/01 14:49:43  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/08/23 19:53:58  hbray
 eliminate fecplugin.h

 Revision 1.3.4.2  2011/08/23 12:03:14  hbray
 revision 5.5

 Revision 1.3.4.1  2011/08/17 17:58:57  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:35  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: hostio.h,v 1.1.2.4 2011/09/24 17:49:36 hbray Exp $ "


#ifndef _HOSTIO_H
#define	_HOSTIO_H



// The header exchanged between the FEC and the Host
// NOTE: the following structure must be on byte bounds.
// NOTE: use this to structure to encode/decode Host headers
#pragma pack(1)
typedef struct HostFrameHeader_t
{
	unsigned short		len;			// The value of len is not self-inclusive
	char				msgType[2];			// see below
	unsigned short		servicePort;	// the peer (pos) service port
	unsigned char		svcType[1];		// see below
	unsigned char		peerIpAddr[8];
	unsigned char		peerIpType[1];
	unsigned short		peerIpPort;
	char				sysid[3];
	char				unused[1];
	NxUid_t				echo;
	char				filler1[17];
} HostFrameHeader_t;					// Memory size: 53 + sizeof(short)

#pragma pack()


#define HostConfigReqestMsgType     "00"
#define HostRequestMsgType          "01"
#define HostResponseMsgType         "02"

#define MAX_HOST_PACKET MaxSockPacketLen
#define MAX_HOST_PAYLOAD ((MAX_HOST_PACKET)-sizeof(HostFrameHeader_t))	// Maximum Host transmission size is 8kb

typedef struct HostFrame_t
{
	HostFrameHeader_t	hdr;
	unsigned char	payload[MAX_HOST_PAYLOAD];
} HostFrame_t;

#pragma pack()


#define HostFrameHdrLen() (sizeof(HostFrameHeader_t)-sizeof(unsigned short))

#define HostFrameLen(frame) ( sizeof((frame).hdr) + HostFramePayloadLen(frame) )	// length of the entire frame
#define HostFrameHeaderPayloadLen(hdr) ( ((hdr).len) - (HostFrameHdrLen()) )	// length of the payload
#define HostFramePayloadLen(frame) ( HostFrameHeaderPayloadLen((frame).hdr) )


// External Functions

extern void HostFrameBuildHeader(HostFrameHeader_t *hhdr, char *msgType, int servicePort, char svcType, char *sysid, char *peerIpAddr, char peerIpType, unsigned short peerIpPort, NxUid_t echo);
extern char *HostFrameHeaderToString(HostFrameHeader_t *hhdr);
extern struct Json_t *HostFrameHeaderSerialize(HostFrameHeader_t *hhdr);
extern int HostFrameRecv(NxClient_t *client, HostFrame_t *frame);
extern int HostFrameSend(NxClient_t *client, HostFrame_t *frame);
extern void HostFrameSetLen(HostFrame_t *frame, int len);
extern int HostFrameSetPayload(HostFrame_t *frame, unsigned char *payload, int len);

#endif /* _HOSTIO_H */
