/*****************************************************************************

Filename:	lib/svs/svsreadreq.c

Purpose:	Radiant Systems SVS Implementation

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The connection read method.  Called after the POS connection
			is read ready, i.e. data, or EOF, present at the STREAM head.

			Return true upon success, or false upon failure/error condition.

			NOTE: the number of bytes available on the STREAM head before
			entering into this method can be found in sess->bytesReady.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:34:00 $
 * $Header: /home/hbray/cvsroot/fec/lib/svs/svsreadreq.c,v 1.3.4.6 2011/10/27 18:34:00 hbray Exp $
 *
 $Log: svsreadreq.c,v $
 Revision 1.3.4.6  2011/10/27 18:34:00  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:51  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:47  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/25 18:19:45  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:55  hbray
 Added cvs headers

 *

2009.10.30 joseph dionne		BT 25022 fix POS NNM request processing
2009.06.26 joseph dionne		Created release 3.4
*****************************************************************************/

#ident "@(#) $Id: svsreadreq.c,v 1.3.4.6 2011/10/27 18:34:00 hbray Exp $ "

#if 0
SVS ISO8583 - like data packets are preceeded by two length bytes in
	Network Byte Order Short, exclusive-- meaning the the length bytes are NOT included in the value.SVS sample
800(Network Management Heartbeat)
request 0:08 00 00 20 00 01 08 00 00 00 04 00 00 00 00 00 16:00 00 17 13 11 06 06 13 24 2 d 31 39 37 31 33 39 $ - 197139 32:31 20 20 20 20 03 01 1 SVS sample
810(Network Management Heartbeat)
response
	0:08 10 00 20 00 01 08 00 00 00 04 00 00 00 00 00
	16:00 00 17 13 11 06 06 13 24 2 d 31 39 37 31 33 39 $ - 197139
	32:31 20 20 20 20 03 01 08 00 00 20 00 01 08 00 00 1 48:00 04 00 00 00 00 00 00 00 17 13 11 06 06 13 24 $ 64:2 d 31 39 37 31 33 39 31 20 20 20 20 03 01 - 1971391
#endif
// Application plugin data/method header declarations
#include "data.h"
// Local scope method declaration
static int
isoNetManagement(char *dst, char *src, int dstSize);

PiApiResult_t
ReadRequest(PiSession_t *sess)
{
	// Worker's "file scope" application data stored in sess->pub.appData
	PiPeekBfr_t peek = { 0 };
	int msgType = 0;
	int reqLen = sizeof(HostRequest_t);
	HostRequest_t *req = (HostRequest_t *)alloca(1 + reqLen);
	int ret = 0;
	int len = -1;
	char *buf = "0000";
	char *bp = buf;
	int ii;

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Initialize stack
	memset(req, 0, 1 + reqLen);
	reqLen = sizeof(req->data);

	// NOTE: the SVS ISO8583 data packet length is a Network Byte
	// Order Short, excluding the sizeof(short).
	if ((ret = PiPosPeek(sess, &peek)))
	{
		SysLog(LogDebug, "PiPosPeek returned %d (%p,%d)", ret, peek.bfr, (peek.bfr) ? peek.len : -1);
		return (eDisconnect);
	}
	else
	{
		int msgLen = 0;

		// Get the buffer and its length from the peek object
		buf = peek.bfr;
		len = peek.len;

		// Get the NBO message length from the peek buffer
		msgLen = ntohs(*(unsigned short *)buf);

		// Wait for a full ISO packet to arrive
		if ((sizeof(short) + msgLen) > len)
			return (eWaitForData);

		// Read one full ISO packet from the buffer
		len = sizeof(short) + msgLen;
	}							// if ((ret = PiPosPeek(sess,&peek)))

	// Check for HostRequest_t buffer overrun
	if (reqLen < len)
	{
		PiException(LogError, __FUNC__, eFailure, "Radiant POS SVS packet too big, %d bytes", len);
		return (eFailure);
	}							// if (reqLen < len)

	// Insure we have a valid Radiant POS SVS ISO8583 Message
	// NOTE: the ISO8583 Message Type is a NBO short directly
	// following the exclusive packet NBO short length bytes
	bp = buf + sizeof(short);
	msgType = ntohs(*(unsigned short *)bp);
	SysLog(LogDebug, "Radiant POS ISO Message type %04X", msgType);

	// Test for a valid message from the Radiant POS
	for (ii = 0; ii < MAX_ISO_MSG; ii++)
	{
		if (!memcmp(&context->validIsoMsg[ii], bp, sizeof(short)))
			break;
	}							// for(ii=0;ii < MAX_ISO_MSG;ii++)

	// Disconnect for invalid Radiant POS Message Type
	if (MAX_ISO_MSG <= ii)
	{
		PiException(LogError, __FUNC__, eFailure, "Unknown Radiant POS ISO message type, %04X", msgType);
		return (eFailure);
	}							// if (MAX_ISO_MSG <= ii)

	// Allocate STACK buffer into which to read the API
	bp = alloca(1 + len);
	memset(bp, 0, 1 + len);

	// "Receive" the POS packet
	if (len != PiPosRecv(sess, bp, len))
	{
		PiException(LogError, "PiPosRecv", eFailure, "failed");
		return (eFailure);
	}							// if (len != PiPosRecv(sess,bp,len))
	buf = bp;

	// Advance past the Packet length, a Network Byte Order (NBO) short
	buf += sizeof(short);
	len -= sizeof(short);

	// Don't send "Network Management" messages to the HGCS(Stratus)
	if (0x08 != buf[0])
	{
		// Return the request
		req->len = len;
		memcpy(req->data, buf, len);

		// Count this authorization packet
		++context->auth.inPkts;
		req->hdr.svcType = eSvcAuth;
		req->hdr.more = true;

		// Send request packet to switch
		if (req->len != PiHostSend(sess, ProxyFlowSequential, sess->pub.replyTTL, req))
		{
			PiException(LogError, "PiHostSend", eFailure, "failed");
			return (eFailure);
		}

		// Handle the "Network Management" message
	}
	else
	{
		char rsp[512] = { 0 };
		int len = isoNetManagement(rsp + sizeof(short), buf,
								   sizeof(rsp) - sizeof(short));

		// Send POS ISO Network Management message response
		if (0 < len)
		{
			// Set the ISO message NBO Short length bytes
			// Add the sizeof(short) to the transmitted length
			*(unsigned short *)rsp = htons(len);
			len += sizeof(short);

			// Send the response to the POS
			if (len != PiPosSend(sess, rsp, len))
			{
				PiException(LogError, "PiPosSend", eFailure, "failed");
				return (eFailure);
			}					// if (len != PiPosSend(sess,rsp,len))

			// Invalid NMM or error creating response, force disconnect
		}
		else
		{
			PiException(LogError, __FUNC__, eFailure, "Network Management Message error");
			return (eFailure);
		}						// if (0 >= len)
	}							// if (0x08 != *buf)

	return (eOk);
}								// PiApiResult_t ReadRequest(PiSession_t *sess)

// Local scope method definition
static int
isoNetManagement(char *rsp, char *req, int size)
{
	unsigned char *bp = (unsigned char *)req;
	isopribitmap_t isopri = { {0} };
	isosecbitmap_t isosec = { {0} };
	unsigned char *rp = (unsigned char *)rsp;
	isopribitmap_t *rPriBits = 0;
	isosecbitmap_t *rSecBits = 0;
	int len = 0;

	// Validate the arguments
	// NOTE: ISO Network Management echo response actually requires 45
	// bytes in rsp, requiring 64 here for room to grow, so-to-speak
	if (!(rsp) || !(req) || 64 > size)
		return (0);

	// Copy the inbound ISO Message type, set response ISO Message type
	memcpy(rp, bp, 2);
	rp[1] = 0x10;
	rp += 2;
	bp += 2;

	// Initialize the Primary Bitmap field
	setIsoBitMap((isobitmap_t *)&isopri, (char *)bp);

	// Initialize the ISO primary bit map "field"
	rPriBits = (isopribitmap_t *)rp;
	memset(rp, 0, sizeof(rPriBits[0]));
	rp += sizeof(rPriBits[0]);
	bp += sizeof(rPriBits[0]);

	// Bit 1 Secondary bitmap present
	if (isopri.hi.bit.bitmap2)
	{
		setIsoBitMap((isobitmap_t *)&isosec, (char *)bp);

		// Update the response buffer
		rPriBits->hi.bit.bitmap2 = 1;

		// Initialize the secondary bitmap field
		rSecBits = (isosecbitmap_t *)rp;
		memset(rp, 0, sizeof(rSecBits[0]));

		// Advance the response pointer
		rp += sizeof(rSecBits[0]);

		// Advance to next field
		bp += sizeof(rSecBits[0]);
	}							// if (isopri.hi.bit.bitmap2)

	// Bit 11 Trace Audit Number
	if (isopri.hi.bit.auditid)
	{
		// Update the response buffer
		rPriBits->hi.bit.auditid = 1;
		memcpy(rp, bp, 3);
		rp += 3;

		// Advance to next field
		bp += 3;
	}							// if (isopri.hi.bit.auditid)

	// Bit 32 Merchant ID
	if (isopri.hi.bit.mercid)
	{
		int len = 1 + ((*bp & 0xff) >> 1);

		// Update the response buffer
		rPriBits->hi.bit.mercid = 1;
		memcpy(rp, bp, len);
		rp += len;

		// Advance to next field
		bp += len;
	}							// if (isopri.hi.bit.mercid)

	// Bit 37 Retrieval Reference Number
	if (isopri.lo.bit.refnum)
	{
		// Update the response buffer
		rPriBits->lo.bit.refnum = 1;
		memcpy(rp, bp, 12);
		rp += 12;

		// Advance to next field
		bp += 12;
	}							// if (isopri.lo.bit.refnum)

	// Bit 38 Approval Code
	rPriBits->lo.bit.approval = 1;
	memcpy(rp, "000000", 6);
	rp += 6;

	// Bit 39 Response code, a positive response is "01"
	rPriBits->lo.bit.response = 1;
	memcpy(rp, "01", 2);
	rp += 2;

	// Bit 70 Network Mmanagement Info Code, should be 301 for echo
	if (isosec.hi.bit.netmgmcode)
	{
		// Update the response buffer
		rSecBits->hi.bit.netmgmcode = 1;
		memcpy(rp, bp, 2);
		rp += 2;
	}							// if (isosec.hi.bit.netmgmcode)

	// Fix-up the Intel, Little Endian, bitmap(s)
	{
		unsigned long tmp;

		tmp = ntohl(rPriBits->hi.dword);
		rPriBits->hi.dword = ntohl(rPriBits->lo.dword);
		rPriBits->lo.dword = tmp;

		if ((rSecBits))
		{
			tmp = ntohl(rSecBits->hi.dword);
			rSecBits->hi.dword = ntohl(rSecBits->lo.dword);
			rSecBits->lo.dword = tmp;
		}						// if ((rSecBits))
	}

	// Return the response length
	len = (char *)rp - rsp;

	return (len);
}								// static int isoNetManagement(char *rsp,char *req,int size)
