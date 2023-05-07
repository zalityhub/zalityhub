/*****************************************************************************

Filename:	lib/onq/onqsendrsp.c

Purpose:	GCS Hilton OnQ Message Set

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
 * $Date: 2011/07/27 20:22:20 $
 * $Header: /home/hbray/cvsroot/fec/lib/onq/onqsendrsp.c,v 1.3 2011/07/27 20:22:20 hbray Exp $
 *
 $Log: onqsendrsp.c,v $
 Revision 1.3  2011/07/27 20:22:20  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:50  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: onqsendrsp.c,v 1.3 2011/07/27 20:22:20 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
SendResponse(PiSession_t *sess)
{
	// Worker's "file scope" application context stored in sess->pub.appData
	char *buf;
	char *bp;
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
		return (eFailure);
	memset(buf, 0, len + 8);
	bp = buf;

	// Build the response packet
	memcpy(buf, response->data, len);
	if (context->crlf)
		buf[len++] = '\r';
	if (context->newline)
		buf[len++] = '\n';

	if (PiPosSend(sess, buf, len) != len)
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return eFailure;
	}

	return eOk;
}								// int SendResponse(PiSession_t *sess)
