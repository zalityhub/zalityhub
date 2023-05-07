/*****************************************************************************

Filename:	lib/mpms/mpmssendrsp.c

Purpose:	Marriott PMS2Way Message Set

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
 * $Date: 2011/07/27 20:22:17 $
 * $Header: /home/hbray/cvsroot/fec/lib/mpms/mpmssendrsp.c,v 1.3 2011/07/27 20:22:17 hbray Exp $
 *
 $Log: mpmssendrsp.c,v $
 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:43  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: mpmssendrsp.c,v 1.3 2011/07/27 20:22:17 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
SendResponse(PiSession_t *sess)
{

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

	// Count this response
	++context->auth.outPkts;

	if (PMS2WaySendPos(sess, "RSP", response->data, response->len) < 0)
	{
		PiException(LogError, "PMS2WaySendPos", eFailure, "failed");
		return eFailure;
	}

	return eOk;
}								// int SendResponse(PiSession_t *sess)
