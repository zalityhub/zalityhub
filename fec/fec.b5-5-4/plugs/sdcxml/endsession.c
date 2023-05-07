/*****************************************************************************

Filename:	lib/sdcxml/endsession.c

Purpose:	SDC XML Message Set

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The load library destructor method.  Called just prior to the
			POS connection tear down.  Release any resources allocated, and
			do any required termination clean up to include notifing POS
			of connection termination.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:22 $
 * $Header: /home/hbray/cvsroot/fec/lib/sdcxml/endsession.c,v 1.2 2011/07/27 20:22:22 hbray Exp $
 *
 $Log: endsession.c,v $
 Revision 1.2  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:53  hbray
 Added cvs headers

 *

2009.06.03 joseph dionne		Updated to support release 3.4
*****************************************************************************/

#ident "@(#) $Id: endsession.c,v 1.2 2011/07/27 20:22:22 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
EndSession(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Free this worker's application data memory
	{
		// Clear and release the entire appl plugin heap memory
		if (context->postHeader != NULL)
			free(context->postHeader);	// release any previous saved header
		context->postHeader = NULL;
	}							// if ((data))

	PiDeleteContext(sess, sess->pub.context);
	return eOk;
}								// int EndSession(PiSession_t *sess)
