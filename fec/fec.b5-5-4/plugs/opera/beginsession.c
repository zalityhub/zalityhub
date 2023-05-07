/*****************************************************************************

Filename:	lib/opera/beginsession.c

Purpose:	Micros Opera HTTP/XML Message Set

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
 * $Date: 2011/09/24 17:49:48 $
 * $Header: /home/hbray/cvsroot/fec/lib/opera/beginsession.c,v 1.2.4.3 2011/09/24 17:49:48 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.2.4.3  2011/09/24 17:49:48  hbray
 Revision 5.5

 Revision 1.2.4.1  2011/08/11 19:47:34  hbray
 Many changes

 Revision 1.2  2011/07/27 20:22:20  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:50  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.2.4.3 2011/09/24 17:49:48 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
BeginSession(PiSession_t *sess)
{
	PiSessionVerify(sess);

	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));
	appData_t *context = (appData_t*)sess->pub.context->data;
	context->xmlHeaderRequired = PiGetPropertyBooleanValue(sess, "XmlHeaderRequired");
	SysLog(LogDebug, "XmlHeaderRequired=%s", BooleanToString(context->xmlHeaderRequired));

	return eOk;
}
