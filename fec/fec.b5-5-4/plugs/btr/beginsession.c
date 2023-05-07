/*****************************************************************************

Filename:	lib/btr/beginsession.c

Purpose:	Marriott BTR PropertyCard Message Set

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The load library instantiation method.  Called directly after
			the POS connection established, but prior to any communication.

			Allocate working memory storage from heap and any other required
			resources needed to process the specific protocol/packet type.

			Initialize the communications channel, i.e. send ENQ for example.

			Return true upon success, or false upon failure/error condition.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:39 $
 * $Header: /home/hbray/cvsroot/fec/lib/btr/beginsession.c,v 1.2.4.2 2011/09/24 17:49:39 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.2.4.2  2011/09/24 17:49:39  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:39  hbray
 Added cvs headers

 *

2009.09.22 harold bray          Updated to support release 4.2
2009.06.03 joseph dionne		Updated to support release 3.4
*****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.2.4.2 2011/09/24 17:49:39 hbray Exp $ "

// Application plugin context/method header declarations
#include "data.h"

PiApiResult_t
BeginSession(PiSession_t *sess)
{

	PiSessionVerify(sess);

	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Initialize the Micros Message Type textual name
	// NOTE: a '1' in the first byte indicates the request is the last to
	// be received, a '0' indicates more messages follow, i.e. settlement.
	// NOTE: a 'C' in the second bytes indicates request is an authorization
	// otherwise request is part of an EDC upload
	memcpy(context->msgNa, (char[12][8])
		   {
		   "\0INVALD", "1CA_REQ", "1CA_RSP", "0BO_REQ", "0BO_RSP", "0BX_REQ", "0BX_RSP", "1BC_REQ", "1BC_RSP", "1BI_REQ", "1BI_RSP", "\0INVALD"}, sizeof(context->msgNa));

	// Set EntityEncoding options
	if ( (context->inputEntityEncoding = PiGetPropertyValue(sess, "InputEntityEncoding")) == NULL )
		context->inputEntityEncoding = "";
	if ( (context->outputEntityEncoding = PiGetPropertyValue(sess, "OutputEntityEncoding")) == NULL )
		context->outputEntityEncoding = "";

	SysLog(LogDebug, "InputEntityEncoding=%s", context->inputEntityEncoding);
	SysLog(LogDebug, "OutputEntityEncoding=%s", context->outputEntityEncoding);

	return eOk;
}								// int instantiate(fecconn_t *sess)
