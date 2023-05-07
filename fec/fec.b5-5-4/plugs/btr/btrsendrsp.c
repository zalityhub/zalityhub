/*****************************************************************************

Filename:	lib/btr/btrsendrsp.c

Purpose:	Marriott BTR PropertyCard Message Set

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
 * $Date: 2011/11/16 19:33:01 $
 * $Header: /home/hbray/cvsroot/fec/lib/btr/btrsendrsp.c,v 1.3.4.3 2011/11/16 19:33:01 hbray Exp $
 *
 $Log: btrsendrsp.c,v $
 Revision 1.3.4.3  2011/11/16 19:33:01  hbray
 Updated

 Revision 1.3.4.2  2011/09/24 17:49:40  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:14  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:39  hbray
 Added cvs headers

 *

2009.09.22 harold bray          Updated to support release 4.2
2009.06.03 joseph dionne		Updated to support release 3.4
2009.05.29 harold bray			Created release 3.1.
*****************************************************************************/

#ident "@(#) $Id: btrsendrsp.c,v 1.3.4.3 2011/11/16 19:33:01 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"


// Local (file) scope helper methods

static char*
TransformData(appData_t *context, char *bfr, int len)
{
	if ( stricmp(context->outputEntityEncoding, "url") == 0 )
		bfr = EncodeUrlCharacters(bfr, len);
	else if ( stricmp(context->outputEntityEncoding, "xml") == 0 )
		bfr = EncodeEntityCharacters(bfr, len);
	return bfr;
}


PiApiResult_t
SendResponse(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	HostRequest_t *response = (HostRequest_t *)alloca(sizeof(HostRequest_t));

	if (response == NULL)
		SysLog(LogFatal, "alloca", eFailure, "failed");

	if (PiHostRecv(sess, response) != response->len)
	{
		PiException(LogError, "PiHostRecv", eFailure, "failed");
		return eFailure;
	}

	// Count this response
	++context->auth.outPkts;

	int len = response->len;

	if (len <= 0)
	{
		PiException(LogError, __FUNC__, eFailure, "Received invalid response length: %d", len);
		return eFailure;
	}

	response->data[len] = 0;

	char *rsp;

	{							// Build a response
		char *cooked = TransformData(context, response->data, len);
		rsp = alloca(strlen(cooked) + 1024);
		len = sprintf(rsp, "auth?result=ok&DEFAULT_KEY&sysid=B&rsp=%s\r\n", cooked);
	}

	if (PiPosSend(sess, rsp, len) != len)
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return eFailure;
	}

	return eOk;
}								// int sendResponse(fecconn_t *sess)
