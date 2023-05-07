/*****************************************************************************

Filename:	lib/capms/capmssendrsp.c

Purpose:	Micros CA/EDC PMS Front-End Communications plugin

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
 * $Date: 2011/09/24 17:49:40 $
 * $Header: /home/hbray/cvsroot/fec/lib/capms/capmssendrsp.c,v 1.3.4.2 2011/09/24 17:49:40 hbray Exp $
 *
 $Log: capmssendrsp.c,v $
 Revision 1.3.4.2  2011/09/24 17:49:40  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:14  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:40  hbray
 Added cvs headers

 *

2009.05.14 joseph dionne		Created release 2.9.
*****************************************************************************/

#ident "@(#) $Id: capmssendrsp.c,v 1.3.4.2 2011/09/24 17:49:40 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

// Local (file) scope helper methods
static int
checkSum(char *data, int len)
{
	unsigned short cksum = 0;

	for (; len--;)
		cksum += (unsigned)*data++;
	return ((unsigned int)cksum);
}								// static int checkSum(char *data,int len)

PiApiResult_t
SendResponse(PiSession_t *sess)
{
	char *buf;
	int len;

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	HostRequest_t *response = (HostRequest_t *)alloca(sizeof(HostRequest_t));
	if (response == NULL)
		SysLog(LogFatal, "alloca failed");

	if (PiHostRecv(sess, response) != response->len)
	{
		PiException(LogError, "PiHostRecv", eFailure, "failed");
		return eFailure;
	}

	// Allocate a transmission buffer, packet length plus IP header
	// NOTE: allocate from STACK memory, DO NOT FREE
	// NOTE: allocating addition space for the HTTP and XML "headers"
	len = response->len;
	if (!(buf = alloca(len + 128)))
		SysLog(LogFatal, "alloca failed");

	memset(buf, 0, len + 128);

	// Micros "PING" application heartbeat response
	if (stristr(response->data, context->pingMark))
	{
		SysLog(LogDebug, "PING response");
		// Send the Heartbeat packet received
		memcpy(buf, response->data, len);

		// Micros Message response
	}
	else
	{
		int idx = 1 + len;
		char cksum[8];

		// Count this response
		++context->auth.outPkts;

		memcpy(buf + 1, response->data, len);
		buf[0] = Soh;

		// Calculate the response checksum
		sprintf(cksum, "%4.4X", checkSum(buf + 1, 1 + idx));
		// SysLog(LogDebug,"Checksum [%s]",cksum);

		// Add the Checksum and EOT bytes
		buf[idx++] = cksum[0];
		buf[idx++] = cksum[1];
		buf[idx++] = cksum[2];
		buf[idx++] = cksum[3];
		buf[idx] = Eot;
		len = 1 + idx;
	}							// if (stristr(response->data,context->pingMark))

	// Write the response to POS
	if (PiPosSend(sess, buf, len) != len)
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return eFailure;
	}

	return eOk;
}								// int sendResponse(fecconnt_t *sess)
