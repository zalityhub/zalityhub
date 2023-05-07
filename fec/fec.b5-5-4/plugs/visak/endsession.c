/*****************************************************************************

Filename:	lib/visak/endsession.c

Purpose:	Vital EIS 1051 TCP/IP protocol / packet type
			Accepts requests from ProtoBase SofTrans 192

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The load library destructor method.  Called just prior to the
			POS connection tear down.  Release any resources allocated, and
			do any required termination clean up to include notifing POS
			of connection termination.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:23 $
 * $Header: /home/hbray/cvsroot/fec/lib/visak/endsession.c,v 1.2 2011/07/27 20:22:23 hbray Exp $
 *
 $Log: endsession.c,v $
 Revision 1.2  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:56  hbray
 Added cvs headers

 *

2009.09.22 joseph dionne		Ported to new FEC Version 4.2
*****************************************************************************/

#ident "@(#) $Id: endsession.c,v 1.2 2011/07/27 20:22:23 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
EndSession(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Send disconnect to POS link
	VisakSignOff(sess);

	// Clear and release conn object's application plugin memory
	char *last = context->lastResponse;
	if ((last))
	{
		int len = context->lastLen;

		// PCI clear the response and release if from memory
		// NOTE: write bit patterns 10101010, 01010101, 11111111, 00000000
		memset(last, 0xAA, len);
		memset(last, 0x55, len);
		memset(last, 0xFF, len);
		memset(last, 0x00, len);

		// Release the last response for appl plugin object
		free(context->lastResponse);
		context->lastResponse = 0;
	}							// if (!(last))

	PiDeleteContext(sess, sess->pub.context);
	return (eOk);
}								// PiApiResult_t EndSession(PiSession_t *sess)
