/*****************************************************************************

Filename:	lib/svs/svssendrsp.c

Purpose:	Radiant Systems SVS Implementation

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
 * $Date: 2011/09/24 17:49:51 $
 * $Header: /home/hbray/cvsroot/fec/lib/svs/svssendrsp.c,v 1.2.4.2 2011/09/24 17:49:51 hbray Exp $
 *
 $Log: svssendrsp.c,v $
 Revision 1.2.4.2  2011/09/24 17:49:51  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:55  hbray
 Added cvs headers

 *

2009.06.26 joseph dionne		Created release 3.4
 * 
*****************************************************************************/

#ident "@(#) $Id: svssendrsp.c,v 1.2.4.2 2011/09/24 17:49:51 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
SendResponse(PiSession_t *sess)
{
	HostRequest_t *rsp;
	char *buf;
	int len;
	int ret;

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Validate arguments

	// Receive the switch response
	rsp = (HostRequest_t *)alloca(sizeof(HostRequest_t));
	ret = PiHostRecv(sess, rsp);
	if (ret != rsp->len)
	{
		PiException(LogError, "PiHostRecv", eFailure, "failed");
		return (eFailure);
	}							// if (ret != rsp->len)
	SysLog(LogDebug, "PiHostRecv len = %d", rsp->len);

	// Terminate switch response packet with null byte
	rsp->data[rsp->len] = 0;

	// Count this response
	// NOTE: Radiant POS (SVS) requests are only for authorizations
	++context->auth.outPkts;

	// Allocate a transmission buffer, packet length plus IP header
	// NOTE: allocate from STACK memory, DO NOT FREE
	len = rsp->len;
	if (!(buf = alloca(8 + len)))
		return (false);
	memset(buf, 0, 8 + len);

	// Build the response packet
	memcpy(buf + sizeof(short), rsp->data, len);

	// Insert the length of the SVS packet
	// NOTE: add to len the size of the NBO Short length bytes
	*(unsigned short *)buf = htons(len);
	len += sizeof(short);

	if (len != PiPosSend(sess, buf, len))
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return (eFailure);
	}							// if (len != PiPosSend(sess,buf,len))

	return (eOk);
}								// PiApiResult_t SendResponse(PiSession_t *sess)
