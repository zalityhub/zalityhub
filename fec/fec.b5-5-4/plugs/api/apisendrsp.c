/*****************************************************************************

Filename:	lib/api/apisendrsp.c

Purpose:	ProtoBase API plugin

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
 * $Date: 2011/09/24 17:49:39 $
 * $Header: /home/hbray/cvsroot/fec/lib/api/apisendrsp.c,v 1.2.4.4 2011/09/24 17:49:39 hbray Exp $
 *
 $Log: apisendrsp.c,v $
 Revision 1.2.4.4  2011/09/24 17:49:39  hbray
 Revision 5.5

 Revision 1.2.4.2  2011/09/01 14:49:44  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:38  hbray
 Added cvs headers

 *
$History: $
2009.08.01 harold bray          Ported to FEC version 4.x
 * 
*****************************************************************************/

#ident "@(#) $Id: apisendrsp.c,v 1.2.4.4 2011/09/24 17:49:39 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"



PiApiResult_t
SendResponse(PiSession_t *sess)
{
	// Worker's "file scope" application data stored in sess->pub.appData
	PiApiResult_t eResult = eOk;
	char *buf;
	int len;

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	HostRequest_t *response = (HostRequest_t *)alloca(sizeof(HostRequest_t));

	if (response == NULL)
	{
		PiException(LogError, "alloca", eFailure, "failed");
		return eFailure;
	}

	if (PiHostRecv(sess, response) != response->len)
	{
		PiException(LogError, "PiHostRecv", eFailure, "failed");
		return eFailure;
	}

	// Count this response
	++context->auth.outPkts;

	// Allocate a transmission buffer, packet length plus IP header
	// NOTE: allocate from STACK memory, DO NOT FREE
	len = response->len;
	if (!(buf = alloca(len + 8)))
		return eFailure;
	memset(buf, 0, len + 8);

	// copy the respose; adding optional \\r for DOS
	{
		char *iptr = response->data;
		char *optr = buf;
		for(int i = 0; i < len; ++i )
		{
			if ( *iptr == '\r' )
			{
				++iptr;
				continue;			// skip any host supplied \r's
			}
			if ( *iptr == '\n' && context->crlf )
				*optr++ = '\r';
			*optr++ = *iptr++;
		}
		len = (optr - buf);			// new output length
	}

	// set the terminator; either RS if more to come, or EOT if final

	if ( (context->auth.inPkts == context->auth.outPkts && context->seenEot) || (!response->hdr.more) )
	{
		buf[len++] = EOT;
		SysLog(LogDebug, "Last API response, drop connection");
		eResult = context->persistent?eOk:eDisconnect;
	}
	else
	{
		buf[len++] = RS;		// multiple responses are due
		if ((context->crlf))
			buf[len++] = '\r';
		buf[len++] = '\n';
		eResult = eOk;			// more responses are due... maintain connection
	}

	if (PiPosSend(sess, buf, len) != len)
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return eFailure;
	}

	return eResult;
}
