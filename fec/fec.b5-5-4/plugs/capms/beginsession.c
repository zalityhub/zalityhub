/*****************************************************************************

Filename:	lib/capms/beginsession.c

Purpose:	Micros CA/EDC PMS Front-End Communications plugin

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
 * $Date: 2011/09/24 17:49:40 $
 * $Header: /home/hbray/cvsroot/fec/lib/capms/beginsession.c,v 1.3.4.2 2011/09/24 17:49:40 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.3.4.2  2011/09/24 17:49:40  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:14  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:39  hbray
 Added cvs headers

 *

2009.05.14 joseph dionne		Created release 2.9
 *****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.3.4.2 2011/09/24 17:49:40 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"


PiApiResult_t
BeginSession(PiSession_t *sess)
{

	PiSessionVerify(sess);

	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));
	appData_t *context = (appData_t*)sess->pub.context->data;

	// PING (Appl Heartbeat) has Interface == SPACES, w/o Appl Data
	sprintf(context->pingMark, "%16.16s%c%c", " ", Stx, Etx);

// collect optional additional port translation arg
	context->altPort = sess->pub.service.properties.port;		// default

	// look for fec.service.additional = |sp:5404
	static const char *prefix = "|sp:";
	char *add;
	if ( (add = strstr(sess->pub.service.properties.additional, prefix)) != NULL )
	{
		add += strlen(prefix);	// point to port
		boolean err;
		int port = IntFromString(add, &err);
		if ( err )
			SysLog(LogError, "Invalid additional arg of %s", sess->pub.service.properties.additional);
		else
			context->altPort = port;
	}

	return eOk;
}
