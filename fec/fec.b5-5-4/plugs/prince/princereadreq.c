/*****************************************************************************

Filename:	lib/prince/princereadreq.c

Purpose:	Princess Message Set

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The connection read method.  Called after the POS connection
			is read ready, i.e. data, or EOF, present at the STREAM head.

			Return true upon success, or false upon failure/error condition.

			NOTE: the number of bytes available on the STREAM head before
			entering into this method can be found in sess->bytesReady.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:34:00 $
 * $Header: /home/hbray/cvsroot/fec/lib/prince/princereadreq.c,v 1.3.4.6 2011/10/27 18:34:00 hbray Exp $
 *
 $Log: princereadreq.c,v $
 Revision 1.3.4.6  2011/10/27 18:34:00  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:50  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:47  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/25 18:19:45  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:21  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:52  hbray
 Added cvs headers

 *

2009.09.22 harold bray          Updated to support release 4.2
2009.05.29 harold bray		Created release 3.1.
*****************************************************************************/

#ident "@(#) $Id: princereadreq.c,v 1.3.4.6 2011/10/27 18:34:00 hbray Exp $ "

// Application plugin data/method header declarations

#include <ctype.h>


#include "data.h"
#include "include/sdcdes.h"


// Local scope method definition

static int
msgReady(PiSession_t *sess)
{

	PiSessionVerify(sess);

	// Preview the waiting buffer for a complete API
	PiPeekBfr_t peek;

	if (PiPosPeek(sess, &peek))
		return 0;

	if ( peek.len < 265 )
		peek.len = 0;

	return peek.len;				// this amount is ready
}


PiApiResult_t
ReadRequest(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

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

	HostRequest_t *request = (HostRequest_t *)alloca(1 + len + sizeof(HostRequest_t));

	if (!(request))
	{
		PiException(LogError, "alloca", eFailure, "failed");
		return eFailure;
	}							// if (!(request))
	memset(request, 0, len + sizeof(HostRequest_t));

	// build the request
	request->hdr.svcType = eSvcAuth;
	request->hdr.more = false;
	request->len = len;
	memcpy(request->data, buf, len);

	// send request to host

	if (PiHostSend(sess, ProxyFlowSequential, sess->pub.replyTTL, request) != request->len)
	{
		PiException(LogError, "PiHostSend", eFailure, "failed");
		return eFailure;
	}

	// Count this request
	++context->auth.inPkts;
	return eOk;
}
