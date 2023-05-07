/*****************************************************************************

Filename:   lib/nx/hostio.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:56 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/Attic/hostio.c,v 1.1.2.6 2011/10/27 18:33:56 hbray Exp $
 *
 $Log: hostio.c,v $
 Revision 1.1.2.6  2011/10/27 18:33:56  hbray
 Revision 5.5

 Revision 1.1.2.5  2011/09/24 17:49:44  hbray
 Revision 5.5

 Revision 1.1.2.3  2011/09/02 14:17:02  hbray
 Revision 5.5

 Revision 1.1.2.2  2011/09/01 14:49:45  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/23 12:03:14  hbray
 revision 5.5

 Revision 1.3.4.1  2011/08/17 17:58:58  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: hostio.c,v 1.1.2.6 2011/10/27 18:33:56 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/hostrequest.h"
#include "include/hostio.h"



// Static Functions

static void Hosthton(HostFrameHeader_t *nhdr, HostFrameHeader_t *hhdr);
static void Hostntoh(HostFrameHeader_t *hhdr, HostFrameHeader_t *nhdr);


static void
Hostntoh(HostFrameHeader_t *hhdr, HostFrameHeader_t *nhdr)
{
	memcpy(hhdr, nhdr, sizeof(*hhdr));

	hhdr->len = ntohs(nhdr->len);
	hhdr->servicePort = ntohs(nhdr->servicePort);
	hhdr->peerIpPort = ntohs(nhdr->peerIpPort);
}


static void
Hosthton(HostFrameHeader_t *nhdr, HostFrameHeader_t *hhdr)
{
	memcpy(nhdr, hhdr, sizeof(*nhdr));

	nhdr->len = htons(hhdr->len);
	nhdr->servicePort = htons(hhdr->servicePort);
	nhdr->peerIpPort = htons(hhdr->peerIpPort);
}


static char*
HostMsgTypeToString(char msgType[2])
{
	char *text;
	if(strncmp(msgType, HostConfigReqestMsgType, 2) == 0 )
		text = "ConfigMsg";
	else if(strncmp(msgType, HostRequestMsgType, 2) == 0 )
		text = "RequestMsg";
	else if(strncmp(msgType, HostResponseMsgType, 2) == 0 )
		text = "ResponseMsg";
	else
		text = StringStaticSprintf("MsgType_%-2.2s", msgType);
	return text;
}


Json_t *
HostFrameHeaderSerialize(HostFrameHeader_t *hhdr)
{
	// HostFrameHeaderVerify(this);		// do not validate... probably a static
	Json_t *root = JsonNew(__FUNC__);
	JsonAddNumber(root, "Len", hhdr->len);
	JsonAddNumber(root, "PayloadLen", (int)HostFrameHeaderPayloadLen(*hhdr));
	JsonAddString(root, "Sysid", "%*.*s", sizeof(hhdr->sysid), sizeof(hhdr->sysid), hhdr->sysid);
	JsonAddString(root, "MsgType", HostMsgTypeToString(hhdr->msgType));
	JsonAddNumber(root, "ServicePort", hhdr->servicePort);
	JsonAddString(root, "SvcType", HostSvcTypeToString(hhdr->svcType[0]));
	JsonAddString(root, "Echo", NxUidToString(hhdr->echo));
	Json_t *sub = JsonPushObject(root, "PeerInfo");
	JsonAddString(sub, "IpAddr", IpAddrToString(hhdr->peerIpAddr));
	JsonAddNumber(sub, "IpPort", hhdr->peerIpPort);
	JsonAddString(sub, "IpType", GetMnemonicString((unsigned char *)hhdr->peerIpType, sizeof(hhdr->peerIpType)));
	return root;
}


char *
HostFrameHeaderToString(HostFrameHeader_t *this)
{
	Json_t *root = HostFrameHeaderSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


int
HostFrameSetPayload(HostFrame_t *frame, unsigned char *payload, int len)
{

	if (len > sizeof(frame->payload))
	{
		SysLog(LogError, "payload of %d is too large for a frame; discarding", len);
		return -1;
	}
	memcpy(frame->payload, payload, len);
	return 0;
}



// Build Host header

void
HostFrameSetLen(HostFrame_t *frame, int len)
{
	frame->hdr.len = HostFrameHdrLen() + len;
}


void
HostFrameBuildHeader(HostFrameHeader_t *hhdr, char *msgType, int servicePort, char svcType,
				char *sysid, char *peerIpAddr, char peerIpType, unsigned short peerIpPort, NxUid_t echo)
{

	memset(hhdr, 0, sizeof(*hhdr));

	hhdr->len = 0;
	hhdr->msgType[0] = msgType[0];
	hhdr->msgType[1] = msgType[1];
	hhdr->servicePort = servicePort;
	hhdr->svcType[0] = svcType;

	memcpy(hhdr->sysid, sysid, sizeof(hhdr->sysid));
	memcpy(hhdr->peerIpAddr, peerIpAddr, sizeof(hhdr->peerIpAddr));
	hhdr->peerIpType[0] = peerIpType;
	hhdr->peerIpPort = peerIpPort;
	hhdr->echo = echo;
}


int
HostFrameSend(NxClient_t *client, HostFrame_t *frame)
{

	NxClientVerify(client);

	if ( HostFramePayloadLen(*frame) > 0 )
		SysLog(LogDebug | SubLogDump, (char *)frame->payload, HostFramePayloadLen(*frame), "%s, payload len %d", HostFrameHeaderToString(&frame->hdr), HostFramePayloadLen(*frame));
	else
		SysLog(LogDebug, "%s", HostFrameHeaderToString(&frame->hdr));

	int flen = HostFrameLen(*frame);

// make the header network compatible
	HostFrameHeader_t hhdr;
	HostFrameHeader_t nhdr;

	memcpy(&hhdr, &frame->hdr, sizeof(hhdr));	// save current header
	Hosthton(&nhdr, &hhdr);		// convert it to network
	memcpy(&frame->hdr, &nhdr, sizeof(frame->hdr));	// replace the one in the frame

	int slen = NxClientSendRaw(client, (char *)frame, flen);

	memcpy(&frame->hdr, &hhdr, sizeof(frame->hdr));	// restore the frame header

	if (slen != flen)			// did not send a full frame
	{
		SysLog(LogError, "NxClientSendRaw failed: %s", NxClientNameToString(client));
		return -1;
	}

	AuditSendEvent(AuditHostSend, "connid", NxClientUidToString(client), "framehdr", HostFrameHeaderToString(&frame->hdr));

	return flen;
}


/*+******************************************************************
	Name:
		HostFrameRecv(2) - Receives a frame from the Host (Stratus Switch).

	Synopsis:
		#include "include/hostio.h"

		int HostFrameRecv (NxClient_t *client, HostFrame_t *frame)

	Description:
		This function will read an entire frame from the Stratus switch.
 		A frame is defined to be the Host 55 byte header and an optional payload
		following the header.
		The frame format/content is defined in HostFrame_t

	Diagnostics:
		Returns -1 on error or disconnect, zero when nothing is received.
 * Otherwise returns > 0 indicating the size of frame received.
-*******************************************************************/

int
HostFrameRecv(NxClient_t *client, HostFrame_t *frame)
{

	NxClientVerify(client);

	memset(frame, 0, sizeof(*frame));

// Read the header
	{
		HostFrameHeader_t nhdr;
		int len;
		int rlen;

		// Read the hdr.len (2 bytes)
		memset(&nhdr, 0, sizeof(nhdr));
		len = sizeof(nhdr.len);
		if ((rlen = NxClientRecvRawExact(client, (char *)&nhdr.len, len, 0)) != len)
		{
			SysLog(LogError, "NxClientRecvRawExact failed: %s", NxClientNameToString(client));
			return -1;
		}

		int hlen = ntohs(nhdr.len);	// this is how much host wants to send us...

		if (hlen == 0)			// this is a ping packet
		{
			AuditSendEvent(AuditHostPingRecv, "connid", NxClientNameToString(client));
			return 0;			// indicate no data available
		}

		len = sizeof(nhdr) - sizeof(nhdr.len);
		if ((rlen = NxClientRecvRawExact(client, (char *)&nhdr.msgType[0], len, 2000)) != len)
		{
			SysLog(LogError, "NxClientRecvRawExact failed: %s", NxClientNameToString(client));
			return -1;
		}

		Hostntoh(&frame->hdr, &nhdr);	// convert it to host
	}


// Ok, we have a full header; take a quick look around to see if it looks reasonable

	if (frame->hdr.len > sizeof(frame->payload))	// This will not fit...
	{
		SysLog(LogError, "Host %s wants to send me too many bytes: %s; Disconnecting", NxClientNameToString(client), HostFrameHeaderToString(&frame->hdr));
		return -1;
	}


// Read the payload
	{
		int plen = HostFramePayloadLen(*frame);
		if ( plen > 0 )
		{
			int rlen;

			if ((rlen = NxClientRecvRawExact(client, (char *)&frame->payload, plen, 2000)) != plen)
			{
				SysLog(LogError, "NxClientRecvRawExact failed: %s", NxClientNameToString(client));
				SysLog(LogError, "Expected %d bytes; received %d", plen, rlen);
				return -1;
			}
		}
	}

	if ( HostFramePayloadLen(*frame) > 0 )
		SysLog(LogDebug | SubLogDump, (char *)frame->payload, HostFramePayloadLen(*frame), "%s, payload len %d", HostFrameHeaderToString(&frame->hdr), HostFramePayloadLen(*frame));
	else
		SysLog(LogDebug, "%s", HostFrameHeaderToString(&frame->hdr));

	AuditSendEvent(AuditHostRecv, "connid", NxClientUidToString(client), "framehdr", HostFrameHeaderToString(&frame->hdr));
	return HostFrameLen(*frame);
}
