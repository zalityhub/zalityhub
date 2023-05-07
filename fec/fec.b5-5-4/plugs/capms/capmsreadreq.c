/*****************************************************************************

Filename:	lib/capms/capmsreadreq.c

Purpose:	Micros CA/EDC PMS Front-End Communications plugin

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The connection read method.  Called after the POS connection
			is read ready, i.e. data, or EOF, present at the STREAM head.

			Return true upon success, or false upon failure/error condition.

			NOTE: the number of bytes available on the STREAM head before
			entering into this method can be found in sess->bytesReady.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/11/16 19:33:02 $
 * $Header: /home/hbray/cvsroot/fec/lib/capms/capmsreadreq.c,v 1.4.4.9 2011/11/16 19:33:02 hbray Exp $
 *
 $Log: capmsreadreq.c,v $
 Revision 1.4.4.9  2011/11/16 19:33:02  hbray
 Updated

 Revision 1.4.4.8  2011/10/27 18:33:55  hbray
 Revision 5.5

 Revision 1.4.4.7  2011/09/24 17:49:40  hbray
 Revision 5.5

 Revision 1.4.4.5  2011/09/02 14:17:02  hbray
 Revision 5.5

 Revision 1.4.4.4  2011/09/01 14:49:44  hbray
 Revision 5.5

 Revision 1.4.4.2  2011/08/25 18:19:44  hbray
 *** empty log message ***

 Revision 1.4.4.1  2011/08/11 19:47:33  hbray
 Many changes

 Revision 1.4  2011/07/27 20:22:14  hbray
 Merge 5.5.2

 Revision 1.3.2.2  2011/07/27 20:19:40  hbray
 Added cvs headers

 *

2009.08.01 harold bray          Ported to FEC version 4.x
2009.05.14 joseph dionne		Created release 2.9.
*****************************************************************************/

#ident "@(#) $Id: capmsreadreq.c,v 1.4.4.9 2011/11/16 19:33:02 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"


// Local scope method declaration
static int msgReady(PiSession_t *sess);	// Insure valid msg has arrived



static EnumToStringMap MessageTypeMap[] =
{
	{MicrosCA_REQ, "CA_REQ"},
	{MicrosCA_RSP, "CA_RSP"},
	{MicrosRV_REQ, "RV_REQ"},
	{MicrosRV_RSP, "RV_RSP"},
	{MicrosBO_REQ, "BO_REQ"},
	{MicrosBO_RSP, "BO_RSP"},
	{MicrosBX_REQ, "BX_REQ"},
	{MicrosBX_RSP, "BX_RSP"},
	{MicrosBC_REQ, "BC_REQ"},
	{MicrosBC_RSP, "BC_RSP"},
	{MicrosBI_REQ, "BI_REQ"},
	{MicrosBI_RSP, "BI_RSP"},
	{MicrosGC_REQ, "GC_REQ"},
	{MicrosGC_RSP, "GC_RSP"},
	{-1, NULL}		// end of list
};


PiApiResult_t
ReadRequest(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	NxClient_t *client = sess->_priv.client;
	NxClientVerify(client);

	// Preview the waiting buffer for a complete msg
	int len = msgReady(sess);

	SysLog(LogDebug, "msgReady returned %d", len);
	if (len <= 0)
	{
		PiPeekBfr_t peek;

		if (PiPosPeek(sess, &peek))
			return eDisconnect;
		return eWaitForData;
	}

	// Allocate STACK buffer into which to read the msg
	char *buf = alloca(8 + len);

	memset(buf, 0, 8 + len);

	// Read message from the buffer
	int rlen = PiPosRecv(sess, buf, len);

	if (rlen != len)
	{
		PiException(LogError, __FUNC__, eFailure, "Expected %d, received %d", len, rlen);
		return eFailure;
	}
	buf[rlen] = '\0';			// terminate it

	microsmsg_t *micros = (microsmsg_t *)buf;

	// "POS Source ID" should be either 18 or 25 bytes in length, if it is
	// a differing value, do not process request and drop the connection
	SysLog(LogDebug, "POS Source ID length %d", context->posSrcLen);
	if (context->posSrcLen != 18 && context->posSrcLen != 25 )
	{
		PiException(LogError, __FUNC__, eFailure, "Invalid POS Source ID: %s", StringDump(NULL, buf, len, 0)->str);
		return eFailure;
	}

	// Response to a Micros "PING" message -- an application heartbeat.
	if (stristr((char *)micros, context->pingMark))
	{
		SysLog(LogDebug, "Micros PING request");
		SysLog(LogDebug, "Checksum%s received", Etx == micros->buf[len - 2] ? " not" : "");

		if (PiPosSend(sess, (char *)micros, len) != len)
		{
			PiException(LogError, "PiPosSend", eFailure, "failed");
			return eFailure;
		}
	
		return eWaitForData;
	}

// Not a ping
	// Strip off SOH, Checksum and EOT
	// NOTE: leaving the FS(0x1C) separators embedded within the
	// "Applications Segment" and the ETX that follows that "field"
	boolean more = false;
	HostSvcType_t svcType = eSvcAuth;

	// Test for the presence of the Checksum field
	int checkSumLen = Etx == micros->buf[len - 2] ? 0 : 4;

	SysLog(LogDebug, "Micros request %d bytes, Checksum len %d", len, checkSumLen);


	// Find the svcType of request received

	int servicePort = client->evf->servicePort;

	{
		char msgid[16];
		memset(msgid, 0, sizeof(msgid));

		// extract the msgid based on posSrcLen and msgtype (Auth/Gift)
		if (context->posSrcLen == 18)
		{
			if ( micros->msg2.data.gift.fs == Fs )
				sprintf(msgid, "%.6s", micros->msg2.data.gift.msgId);
			else
				sprintf(msgid, "%.6s", micros->msg2.data.auth.msgId);
		}
		else
		{
			if ( micros->msg9.data.gift.fs == Fs )
				sprintf(msgid, "%.6s", micros->msg9.data.gift.msgId);
			else
				sprintf(msgid, "%.6s", micros->msg9.data.auth.msgId);
		}
	
		int mt = EnumMapStringToVal(MessageTypeMap, msgid, NULL);

		SysLog(LogDebug, "msgid %s=%d", msgid, mt);
		switch (mt)
		{
			default:
				SysLog(LogWarn, "I don't recognize message id %s", msgid);
				break;

			case MicrosCA_REQ:	// Credit Authorization Request
				more = false;
				svcType = eSvcAuth;
				break;

			case MicrosRV_REQ:	// Reversal Request
				more = false;
				svcType = eSvcAuth;
				break;

			case MicrosBO_REQ:	// Batch Open Request
				more = true;
				svcType = eSvcEdcMulti;
				break;

			case MicrosBX_REQ:	// Batch Transfer Request
				more = true;
				svcType = eSvcEdcMulti;
				break;

			case MicrosBC_REQ:	// Batch Close Request
				more = false;
				svcType = eSvcEdcMulti;
				break;

			case MicrosBI_REQ:	// Batch Inquiry Request
				more = false;
				svcType = eSvcEdcMulti;
				break;

			case MicrosGC_REQ:	// Gift Request
				more = false;
				svcType = eSvcAuth;
				servicePort = context->altPort;
				break;
		}
	}


	// Count this
	++context->auth.inPkts;

	// send to host
	HostRequest_t *request = alloca(sizeof(HostRequest_t) + 1 + len);
	memset(request, 0, sizeof(HostRequest_t) + 1 + len);

	// Calculate request length, removing the <SOH>, <EOT> and
	// <Checksum> if the POS sent it
	len -= 2 + checkSumLen;

	// Update the request buffer
	memcpy(request->data, micros->buf + 1, len);
	request->len = len;
	request->hdr.svcType = svcType;
	request->hdr.more = more;

	if (PiHostSendServicePort(sess, ProxyFlowSequential, sess->pub.replyTTL, request, servicePort) != request->len)
	{
		PiException(LogError, "PiHostSendServicePort", eFailure, "failed");
		return eFailure;
	}

	return eWaitForData;
}



// Local scope method definition

#if 0
static int
checkSum(char *data, int len)
{
	unsigned short cksum = 0;

	for (; len--;)
		cksum += (unsigned)*data++;
	return ((unsigned int)cksum);
}								// static int checkSum(char *data, int len)
#endif

static int
msgReady(PiSession_t *sess)
{
	int bytes = -1;

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Preview the waiting buffer for a complete XML Message
	// NOTE: the 27 bytes ready is the length of a Micros "PING"
	// Heartbeat message with a two (2) byte "POS User Workstation
	// Number" field in <POS Source ID> in the following format;
	// <SOH><POS Source ID><STX><ETX><Checksum><EOT>
	// NOTE: Checksum is optional for TCP/IP links
	PiPeekBfr_t peek;

	if (PiPosPeek(sess, &peek))
		return bytes;
	int len = peek.len;

	if (21 < len)
	{
		// Peek at the message type to get the packet length
		for (;;)
		{
			int ii;
			microsmsg_t *micros = (microsmsg_t *)peek.bfr;

			// Find the POS Source ID terminating STX mark
			for (ii = 0; ii < len && (Stx != micros->buf[ii]); ii++) ;

			// Check for an incomplete or invalid message format
			if (ii >= len || Soh != micros->msg9.soh || Stx != micros->buf[ii])
				break;

			// Using two (2) digit "POS User Workstation Number" length?
			context->posSrcLen = ii - 1;

			// Find the terminating EOT mark
			for (ii = 0; ii < len && (Eot != micros->buf[ii]); ii++) ;

			// Full message not yet received
			if (ii >= len || Eot != micros->buf[ii])
			{
				bytes = 0;
				break;
			}

			// Micros message received
			bytes = 1 + ii;

			break;
		}
	}							// if (21 < len)

	if (bytes > len)
	{
		PiException(LogError, __FUNC__, eFailure, "my calculated bytes %d exceed the available %d", bytes, len);
		bytes = -1;		// fail
	}

	return (bytes);
}
