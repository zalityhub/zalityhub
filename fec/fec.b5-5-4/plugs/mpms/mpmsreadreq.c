/*****************************************************************************

Filename:	lib/mpms/mpmsreadreq.c

Purpose:	Marriott PMS2Way Message Set

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
 * $Header: /home/hbray/cvsroot/fec/lib/mpms/mpmsreadreq.c,v 1.3.4.7 2011/11/16 19:33:02 hbray Exp $
 *
 $Log: mpmsreadreq.c,v $
 Revision 1.3.4.7  2011/11/16 19:33:02  hbray
 Updated

 Revision 1.3.4.6  2011/10/27 18:33:55  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:42  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:44  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/25 18:19:44  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:43  hbray
 Added cvs headers

 *

2009.12.19 harold bray  		Created release 1.0.
*****************************************************************************/

#ident "@(#) $Id: mpmsreadreq.c,v 1.3.4.7 2011/11/16 19:33:02 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"


// Local scope method declaration
static int msgReady(PiSession_t *sess);

PiApiResult_t
ReadRequest(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Preview the waiting buffer for a complete msg
	int len = msgReady(sess);

	SysLog(LogDebug, "msgReady returned %d", len);

	if (len <= 0)
		return (len < 0) ? eDisconnect : eWaitForData;

	PiPeekBfr_t peek;
	if (PiPosPeek(sess, &peek))
		return eDisconnect;	// EOF

	if ( len > peek.len )
		return eWaitForData;		// not a full pkt, wait longer...

	// Allocate STACK buffer into which to read the msg
	int reqlen = len + sizeof(Pms2WayMsg_t);
	Pms2WayMsg_t *req = alloca(reqlen+128);
	memset(req, 0, reqlen);

	// Read message from the buffer
	int rlen = PiPosRecv(sess, (char*)req, len);

	if (rlen != len)
	{
		PiException(LogError, __FUNC__, eFailure, "Expected %d, received %d", len, rlen);
		return eFailure;
	}

// save the header
	memcpy(&context->receivedHeader, &req->hdr, sizeof(Pms2WayMsgHeader_t));
	len -= sizeof(Pms2WayMsgHeader_t);

// if encrypted, decrypt the payload
	if ( strncmp(context->receivedHeader.versionNbr, "0004", 4) == 0 && len > 0 )
	{
		PMS2WaySDecrypt(sess, req->body, len);
		req->body[len] = '\0';			// insure termination
		SysLog(LogDebug  | SubLogDump| SubLogDump, req->body, len, "decrypted %d", len);
	}

// if this is a REQ msgType, send to host
	if (len > 0 && strncmp(context->receivedHeader.msgType, "REQ", 3) == 0)
	{
		// Allocate memory for the request
		HostRequest_t *request = (HostRequest_t *)alloca(1 + len + sizeof(HostRequest_t));
		if (!(request))
			SysLog(LogFatal, "alloca failed");

		memset(request, 0, len + sizeof(HostRequest_t));

		// Return the request
		request->len = len;
		memcpy(request->data, req->body, len);

		++context->auth.inPkts;

		// set request type based on first 3 chrs
		char reqType[4];

		memset(reqType, 0, sizeof(reqType));
		memcpy(reqType, req->body, sizeof(reqType) - 1);
		if (strcmp(reqType, "CSB") == 0 || strcmp(reqType, "CST") == 0 || strcmp(reqType, "CSE") == 0)
		{
			request->hdr.svcType = eSvcEdc;
		}
		else
		{
			request->hdr.svcType = eSvcAuth;
		}
		request->hdr.more = false;

		if (PiHostSend(sess, ProxyFlowSequential, sess->pub.replyTTL, request) != request->len)
		{
			PiException(LogError, "PiHostSend", eFailure, "failed");
			return eFailure;
		}
		if (PMS2WaySendPos(sess, "ACK", NULL, 0) < 0)
		{
			PiException(LogError, "PMS2WaySendPos", eFailure, "failed");
			return eFailure;
		}
	}
	else if (strncmp(context->receivedHeader.msgType, "ACK", 3) != 0)
	{
		SysLog(LogWarn | SubLogDump, (char*)&context->receivedHeader, sizeof(context->receivedHeader), "Invalid header: %s", &context->receivedHeader);
		return eFailure;
	}

	return (eOk);
}								// int readRequest(sess_t *sess)



// Local scope method definition

static int
msgReady(PiSession_t *sess)
{

	PiSessionVerify(sess);

	PiPeekBfr_t peek;

	if (PiPosPeek(sess, &peek))
		return -1;

	int len;
	int bytesready = 0;

	if ((len = peek.len) >= sizeof(Pms2WayMsgHeader_t))
		// Preview the waiting buffer for a valid message header
	{
		Pms2WayMsgHeader_t *hdr = (Pms2WayMsgHeader_t *)peek.bfr;

		if (strncmp(hdr->msgType, "REQ", 3) != 0 && strncmp(hdr->msgType, "ACK", 3) != 0)
		{
			SysLog(LogWarn, "%.3s is not a valid message type", hdr->msgType);
			return -1;
		}

// Verify all the FS chars...
		if (hdr->fs1 != 0x1C || hdr->fs2 != 0x1C || hdr->fs3 != 0x1C || hdr->fs4 != 0x1C)
		{
			SysLog(LogWarn, "missing 0x1C character");
			return -1;
		}

// get datalength
		char dataLen[32];

		memset(dataLen, 0, sizeof(dataLen));
		memcpy(dataLen, hdr->dataLen, sizeof(hdr->dataLen));
		bytesready = atoi(dataLen);
		HostRequest_t *req;

		if (bytesready < 0 || bytesready > sizeof(req->data))
		{
			SysLog(LogWarn, "%d is an invalid packet length", bytesready);
			return -1;
		}

		bytesready += sizeof(Pms2WayMsgHeader_t);	// add the header size
	}

	return (bytesready);
}
