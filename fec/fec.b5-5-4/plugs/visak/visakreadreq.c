/*****************************************************************************

Filename:	lib/visak/visakreadreq.c

Purpose:	Vital EIS 1051/1052 protocol and EIS 1080/1081 packet types
			See visak/data.h for specifications and devices supported

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The connection read method.  Called after the POS connection
			is read ready, i.e. data, or EOF, present at the STREAM head.

			This method handles all communications protocol is processed
			including packet encapulation (STX/ETX), Longitudinal Redundancy
			Check (LRC), response receipt acknowledgment (ACK/NAK), POS
			disconnection (DLE EOT).

			Return true upon success, or false upon failure/error condition.

			NOTE: the number of bytes available on the STREAM head before
			entering into this method can be found in globals->bytesReady.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/11/16 19:33:03 $
 * $Header: /home/hbray/cvsroot/fec/lib/visak/visakreadreq.c,v 1.3.4.7 2011/11/16 19:33:03 hbray Exp $
 *
 $Log: visakreadreq.c,v $
 Revision 1.3.4.7  2011/11/16 19:33:03  hbray
 Updated

 Revision 1.3.4.6  2011/10/27 18:34:00  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:52  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:47  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/25 18:19:45  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:24  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:57  hbray
 Added cvs headers

 *

2009.09.22 joseph dionne		Ported to release v4.2
2008.11.15 joseph dionne		Created release 1.0.  Supports authorizations
								only at this release.
*****************************************************************************/

#ident "@(#) $Id: visakreadreq.c,v 1.3.4.7 2011/11/16 19:33:03 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

// Local scope method declaration

PiApiResult_t
ReadRequest(PiSession_t *sess)
{
	// Worker's "file scope" application context stored in sess->pub.appData

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;
	
	char *last = (context) ? context->lastResponse : 0;
	PiPeekBfr_t peek = { 0 };
	int reqLen = 1 + sizeof(HostRequest_t);
	HostRequest_t *req = (HostRequest_t *)alloca(1 + reqLen);
	char *buf = "0";
	char *bp = buf;
	int len = -1;
	char *orgBuf;
	int lastByte;
	int byte;
	int ret;

	// Initialize stack
	memset(req, 0, 1 + reqLen);
	reqLen = sizeof(req->data);

	// Visa K 105x comm packets are in the format of <STX><data><ETX><LRC>
	// No data available, must be "Connection disconnect by peer"
	if ((ret = PiPosPeek(sess, &peek)))
	{
		SysLog(LogDebug, "PiPosPeek returned %d (%p,%d)", ret, peek.bfr, (peek.bfr) ? peek.len : -1);
		return (VisakSignOff(sess));
	}							// if ((ret = PiPosPeek(sess,&peek)))
	buf = peek.bfr;
	len = peek.len;

	// Check for HostRequest_t buffer overrun
	if (sizeof(req->data) < len)
	{
		SysLog(LogWarn, "Buffer overrun detected; disconnecting");
		PiPosSend(sess, nak(context), 1);
		return (VisakSignOff(sess));
	}							// if (reqLen < len)

	// Remove parity bit from the first byte and last byte before LRC
	bp = buf;
	byte = buf[0] & 0x7F;
	lastByte = buf[peek.len - 1] & 0x7F;
	SysLog(LogDebug, "Packet length %d, first byte x%02X", peek.len, byte);

	// Copy the original buffer to perserve parity
	if (STX(context) == byte)
	{
		orgBuf = alloca(1 + len);
		memcpy(orgBuf, buf, len);
		orgBuf[len] = 0;
	}
	else
	{
		orgBuf = 0;
	}							// if (STX(context) == byte)

	// POS may be sending EVEN parity, strip off the parity bit
	context->evenParity = context->evenParity | (byte != buf[0]);
	if ((context->evenParity))
	{
		// Remove parity bit from the remaining bytes,
		// but do not do not alter the LRC
		for (int ii = 1; ii < (len - 1); ii++)
			bp[ii] = bp[ii] & 0x7F;
	}

	// Is this a positive/negative acknowledgement of the last response?
	if (ACK(context) == byte		// response acknowledged
		|| EOT(context) == byte		// might be a DLE byte check
		|| EOT(context) == (buf[1] & 0x7F))
	{							// the second byte for EOT
		// Inform POS the link is disconnecting
		return (VisakSignOff(sess));
	}							// if (ACK(context) == byte               // response acknowledged, or EOT

	// Record time STX arrived, setting wait time for ETX/ETB below
	if (STX(context) == byte)
		context->reqBgTime = time(0);

	// POS negative acknowledge of response, resent it
	if (NAK(context) == byte)
	{
		int len = context->lastLen;

		// No response to resend, force disconnect
		if (!(last))
		{
			SysLog(LogWarn, "No response to resend; disconnecting");
			return (VisakSignOff(sess));
		}

		// Send the last response to POS
		ret = VisakSendPos(sess, last, len);

		PiException(LogError, __FUNC__, eFailure, "Too many resends; disconnecting");
		// Only retransmit the last message once, this is TCP/IP after all
		return (eFailure);
	}							// if (NAK(context) == byte)

	// Validate request packet
	if (STX(context) != byte && (ETX(context) != lastByte || ETB(context) != lastByte))
	{
		// Complete buffer not yet available?
		if (STX(context) == byte)
		{
			// Only wait five seconds for ETX or ETB to arrive
			if (-5 < (context->reqBgTime - time(0)))
			{
				SysLog(LogWarn, "Too much time before packet termination; " "disconnecting");
				return (eWaitForData);
			}
		}						// if (STX(context) == byte)

		// Notify POS of receipt failure and disconnect
		if (1 != PiPosSend(sess, nak(context), 1))
		{
			PiException(LogError, "PiPosSend", eFailure, "failed");
			return (eFailure);
		}
		SysLog(LogWarn, "SignOff; not sure why; disconnecting");
		return (VisakSignOff(sess));
	}							// if (STX(context) !=byte && (ETX(context) !=lastByte || ETB(context) != ...

	// Check the LRC value
	if ((orgBuf))
	{
		ret = CalcLrc(orgBuf + 1, len - 2);
		if (ret != buf[len - 1])
		{
			PiException(LogError, __FUNC__, eFailure, "Expected LRC x%02X, got LRC x%02X", ret, buf[len - 1] & 0xFF);
			VisakSignOff(sess);
			return eFailure;
		}						// if (ret != buf[len-1])
	}							// if ((orgBuf))

	// Allocate memory for the packet
	bp = alloca(1 + len);
	memset(bp, 0, 1 + len);

	// "Receive" the POS packet
	if (len != PiPosRecv(sess, bp, len))
	{
		PiException(LogError, "PiPosRecv", eFailure, "failed");
		VisakSignOff(sess);
		return eFailure;
	}							// if (len != PiPosRecv(sess,bp,len))

	// Advance buf pointer past the STX byte
	buf = bp + 1;

	// Count this Auth or EDC packet
	if ('K' != buf[0])
	{
		SysLog(LogDebug, "Auth packet received");
		++context->auth.inPkts;

		// Count this authorization request
	}
	else
	{
		SysLog(LogDebug, "EDC packet received");
		++context->edc.inPkts;
	}							// if ('K' == buf[0])

	// Acknowledge receipt of the authorization request
	if (!(context->edc.inPkts))
	{
		if (1 != PiPosSend(sess, ack(context), 1))
		{
			PiException(LogError, "PiPosSend", eFailure, "failed");
			return (eFailure);
		}

		// Acknowledge receipt of the EDC packet
	}
	else
	{
		// buf points at "K1.ZH" or "K1.ZP" or "K1.ZD" or "K1.ZT"
		char VisaRecType = buf[4];

		// Alternate the acknowledgement byte, ACK and ETB
		// until the Trailer packet arrives
		if ('T' != VisaRecType)
		{
			if (0x01 & context->edc.inPkts)
			{
				if (1 != PiPosSend(sess, ack(context), 1))
				{
					PiException(LogError, "PiPosSend", eFailure, "failed");
					return (eFailure);
				}
			}
			else
			{
				if (1 != PiPosSend(sess, bel(context), 1))
				{
					PiException(LogError, "PiPosSend", eFailure, "failed");
					return (eFailure);
				}
			}					// if (0x01 & context->edc.in)

			// When the trailer record is received, i.e. the last packet,
			// acknowledge it with DC2 to indicate settlement processing is
			// in progress
		}
		else
		{
			if (1 != PiPosSend(sess, dc2(context), 1))
			{
				PiException(LogError, "PiPosSend", eFailure, "failed");
				return (eFailure);
			}
		}						// if ('T' != VisaRecType)
	}							// if (!(context->edc.in))

	// Visa K message type
	if (strcmp("api", context->msgType))
	{
		SysLog(LogDebug, "VisaK packet received, length %d", len);

		// Initialize switched request
		req->len = (len -= 3);
		req->hdr.svcType = ('K' == buf[0]) ? eSvcEdc : eSvcAuth;
		req->hdr.more = false;
		memcpy(req->data, buf, len);

		// Send request packet to switch
		if (req->len != PiHostSend(sess, ProxyFlowSequential, sess->pub.replyTTL, req))
		{
			return (VisakSignOff(sess));
		}						// if (req->len != PiHostSend(sess,req))

		// ProtoBase Plus (API) request
	}
	else
	{
		char *tmp = req->data;
		int newLen = 0;

		SysLog(LogDebug, "API packet received, length %d", len);

		// Initialize switched request
		memset(req, 0, 1 + reqLen);
		req->hdr.svcType = eSvcAuth;
		req->hdr.more = false;

		// Check for Windows style newline
		context->crlf = 0 != strchr(bp, '\r');

		// Copy the API request into the host request buffer
		// less the STX,ETX,LRC, blank or comment lines
		bp++;					// skip STX byte

		// Remove comments, blank lines and Windows newlines
		for (int bytes = 0; (*bp); bytes = 0)
		{
			// Get the API Field number
			int apiNo = atoi(bp);

			// Comment or blank line
			if (0 >= apiNo)
			{
				// Found API terminator
				if (EOT(context) == *bp || *bp == RS(context))
					break;

				// Count number of bytes in comment
				sscanf(bp, "%*[^\r\n]%n", &bytes);
				bp += bytes;
				if ('\r' == *bp)
					bp++;
				if ('\n' == *bp)
					bp++;
				bytes = 0;
				continue;
			}					// if (0 >= apiNo)

			// Scan off the "API Field,API Data"
			// NOTE: will terminate parsing at RS or EOT, see above
			sscanf(bp, "%[^\r\n]%n", tmp, &bytes);
			if (0 >= bytes)
				break;

			// Advance the target pointer by bytes,
			// keep running total of the new length
			bp += bytes;
			tmp += bytes;
			newLen += bytes;

			// Add UNIX newline to buffer
			tmp += sprintf(tmp, "%c", '\n');
			newLen++;

			// Store the newline character(s), UNIX or Windows style
			bytes = 0;
			sscanf(bp, "%[\r\n]%n", tmp, &bytes);
			bp += bytes;
		}

		// Send parsed API length
		SysLog(LogDebug, "API parsed length %d", newLen);
		req->len = newLen;

		// Send request packet to switch
		if (req->len != PiHostSend(sess, ProxyFlowSequential, sess->pub.replyTTL, req))
		{
			PiException(LogError, "PiHostSend", eFailure, "failed");
			VisakSignOff(sess);
			return eFailure;
		}						// if (req->len != PiHostSend(sess,req))
	}							// if (strcmp("api",globals->inMsgType))

	return (eOk);
}								// PiApiResult_t ReadRequest(PiSession_t *sess)
