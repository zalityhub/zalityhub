/*****************************************************************************

Filename:	lib/visak/visaksendrsp.c

Purpose:	Vital EIS 1051 TCP/IP protocol / packet type
			Accepts requests from ProtoBase SofTrans 192

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The load library transaction response send method.  Called after
			the one of the following events.
				1. A response was received from the processor
				2. A response time out event
				3. The processor service is offline, i.e. down
				4. Prior to connection tear down after normal or abnormal
				processing

			This method is where all communications protocol is processed.

			This method is called just prior to tearing down the TCP/IP link
			so that termination protocol can be sent the the POS.

			Return true upon success, or false upon failure/error condition

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:52 $
 * $Header: /home/hbray/cvsroot/fec/lib/visak/visaksendrsp.c,v 1.2.4.2 2011/09/24 17:49:52 hbray Exp $
 *
 $Log: visaksendrsp.c,v $
 Revision 1.2.4.2  2011/09/24 17:49:52  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:24  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:57  hbray
 Added cvs headers

 *

2009.09.22 joseph dionne		Ported to release v4.2
2008.11.15 joseph dionne		Created release 1.0.  Supports authorizations
								only at this release.
*****************************************************************************/

#ident "@(#) $Id: visaksendrsp.c,v 1.2.4.2 2011/09/24 17:49:52 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
SendResponse(PiSession_t *sess)
{
	// Worker's "file scope" application data stored in sess->pub.appData
	HostRequest_t *rsp;
	int ret;

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	// No response received

	// Receive the switch response
	rsp = (HostRequest_t *)alloca(sizeof(HostRequest_t));
	ret = PiHostRecv(sess, rsp);
	if (ret != rsp->len)
	{
		PiException(LogError, "PiHostRecv", eFailure, "failed");
		VisakSignOff(sess);
		return eFailure;
	}							// if (ret != rsp->len)
	SysLog(LogDebug, "PiHostRecv len = %d", rsp->len);

	// Terminate switch response packet with null byte
	rsp->data[rsp->len] = 0;

	// Count this response
	if ((context->auth.inPkts))
		++context->auth.outPkts;
	else
		++context->edc.outPkts;

	// For API packets, send back the Windows newline if needed
	// NOTE: crlf will ONLY be true if API packet was received
	if ((context->crlf))
	{
		// Create temporary buffer twice the sized of that received
		int len = rsp->len << 1;
		char *tmp = alloca(len);
		char *buf = tmp;
		char *bp = rsp->data;
		int bytes;

		// Parse the API into lines restoring the Windows newlines
		// NOTE: CR added to format below just-in-case
		memset(buf, 0, len);
		for (len = 0, bytes = 0; (*bp); bytes = 0)
		{
			sscanf(bp, "%[^\n\r]%n", tmp, &bytes);
			bp += bytes;

			// Advance past the source (response) packet newline
			if ('\r' == *bp)
				bp++;
			if ('\n' == *bp)
				bp++;

			// Add Windows newline to the POS response packet
			tmp += bytes;
			tmp += sprintf(tmp, "%c%c", '\r', '\n');
			len += 2 + bytes;
		}						// for(len=0,bytes=0;(*bp);bytes=0)

		// Overwrite the switch response packet
		bytes = sprintf(rsp->data, "%.*s", sizeof(rsp->data) - 1, buf);
		rsp->len = len;
		SysLog(LogDebug, "Windows newlines added, length = %d(%d)", len, bytes);
	}							// if ((context->crlf))

	// Send the response to the POS
	return (VisakSendPos(sess, rsp->data, rsp->len));
}								// PiApiResult_t SendResponse(PiSession_t *sess)
