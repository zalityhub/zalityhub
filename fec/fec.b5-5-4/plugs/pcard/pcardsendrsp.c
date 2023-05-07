/*****************************************************************************

Filename:	lib/pcard/pcardsendrsp.c

Purpose:	PropertyCard Message Set

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
 * $Date: 2011/11/16 19:33:02 $
 * $Header: /home/hbray/cvsroot/fec/lib/pcard/pcardsendrsp.c,v 1.3.4.3 2011/11/16 19:33:02 hbray Exp $
 *
 $Log: pcardsendrsp.c,v $
 Revision 1.3.4.3  2011/11/16 19:33:02  hbray
 Updated

 Revision 1.3.4.2  2011/09/24 17:49:49  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:21  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:52  hbray
 Added cvs headers

 *

2009.09.22 harold bray          Updated to support release 4.2
2009.06.03 joseph dionne		Updated to support release 3.4
2009.05.29 harold bray			Created release 3.1.
*****************************************************************************/

#ident "@(#) $Id: pcardsendrsp.c,v 1.3.4.3 2011/11/16 19:33:02 hbray Exp $ "

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

	// Worker's "file scope" application data stored in sess->pub.appData
	appData_t *context = (appData_t*)sess->pub.context->data;

	HostRequest_t *response = (HostRequest_t *)alloca(sizeof(HostRequest_t));

	if (response == NULL)
		SysLog(LogFatal, "alloca for HostResponse_t failed");

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
		PiException(LogError, __FUNC__, eFailure, "Invalid response length: %d", len);
		return eFailure;
	}

	response->data[len] = 0;

	char *rsp = alloca((len*3) + 1024);

	{							// Build a response
		char *out = rsp;

// format using a dummy length (0)
// which will be subsequently replaced

        char *cooked = TransformData(context, &response->data[14], strlen(&response->data[14]));
		out += sprintf(out, "%04d&DEFAULT_KEY=%-10.10s&result=ok&rsp=%s", 0, &response->data[1], cooked);
		len = out - rsp;
		char tmp[16];

		sprintf(tmp, "%04d", len - 4);
		memcpy(rsp, tmp, 4);
	}

	if (PiPosSend(sess, rsp, len) != len)
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return eFailure;
	}

	return eOk;
}
