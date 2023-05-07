/*****************************************************************************

Filename:	lib/sdcxml/beginsession.c

Purpose:	SDC XML Message Set

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
 * $Date: 2011/09/24 17:49:50 $
 * $Header: /home/hbray/cvsroot/fec/lib/sdcxml/beginsession.c,v 1.2.4.2 2011/09/24 17:49:50 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.2.4.2  2011/09/24 17:49:50  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:53  hbray
 Added cvs headers

 *

2009.06.03 joseph dionne		Updated to support release 3.4
*****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.2.4.2 2011/09/24 17:49:50 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
BeginSession(PiSession_t *sess)
{

	PiSessionVerify(sess);

	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));
	appData_t *context = (appData_t*)sess->pub.context->data;

	context->outputFinalEot = PiGetPropertyBooleanValue(sess, "OutputFinalEot");

	// Set EntityEncoding options
	if ( (context->inputEntityEncoding = PiGetPropertyValue(sess, "InputEntityEncoding")) == NULL )
		context->inputEntityEncoding = "";
	if ( (context->outputEntityEncoding = PiGetPropertyValue(sess, "OutputEntityEncoding")) == NULL )
		context->outputEntityEncoding = "";

	SysLog(LogDebug, "InputEntityEncoding=%s", context->inputEntityEncoding);
	SysLog(LogDebug, "OutputEntityEncoding=%s", context->outputEntityEncoding);

	return eOk;
}
