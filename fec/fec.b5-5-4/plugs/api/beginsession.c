/*****************************************************************************

Filename:	lib/api/beginsession.c

Purpose:	ProtoBase API plugin

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The load library beginsession method.  Called directly after
			the POS connection established, but prior to any communication.

			Allocate working memory storage from heap and any other required
			resources needed to process the specific protocol/packet type.

			Initialize the communications channel, i.e. send ENQ for example.

			Return true upon success, or false upon failure/error condition.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:39 $
 * $Header: /home/hbray/cvsroot/fec/lib/api/beginsession.c,v 1.3.4.3 2011/09/24 17:49:39 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.3.4.3  2011/09/24 17:49:39  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/11 19:47:32  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:38  hbray
 Added cvs headers

 *
$History: $
2009.08.01 harold bray          Ported to FEC version 4.x
*****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.3.4.3 2011/09/24 17:49:39 hbray Exp $ "

// Application plugin data/method header declarations

#include "data.h"


PiApiResult_t
BeginSession(PiSession_t *sess)
{

	PiSessionVerify(sess);

	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));
	appData_t *context = (appData_t*)sess->pub.context->data;

	context->persistent = PiGetPropertyBooleanValue(sess, "Persistent");

	context->t70proxy = NxGetPropertyBooleanValue("T70Proxy.Enabled");

	SysLog(LogDebug, "T70proxy/Enabled=%s", BooleanToString(context->t70proxy));

	return eOk;
}
