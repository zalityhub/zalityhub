/*****************************************************************************

Filename:	lib/visak/beginsession.c

Purpose:	Vital EIS 1051 TCP/IP protocol / packet type
			Accepts requests from ProtoBase SofTrans 192

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The load library instantiation method.  Called directly after
			the POS connection established, but prior to any communication.

			Allocate working memory storage from heap and any other required
			resources needed to process the specific protocol/packet type.

			Initialize the communications channel, i.e. send ENQ for example.

			Return eOk upon success, or eFailure on error condition.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:52 $
 * $Header: /home/hbray/cvsroot/fec/lib/visak/beginsession.c,v 1.2.4.2 2011/09/24 17:49:52 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.2.4.2  2011/09/24 17:49:52  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:56  hbray
 Added cvs headers

 *

2009.09.22 joseph dionne		Ported to new FEC Version 4.2
*****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.2.4.2 2011/09/24 17:49:52 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
BeginSession(PiSession_t *sess)
{
	PiSessionVerify(sess);

	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Assign values to run-time memory
	// NOTE: the initial values below are set to NO parity, and will
	// be adjusted if the POS is operating in EVEN parity.
	context->Ack[0] = 0x06;
	context->Bel[0] = 0x07;
	context->Dc2[0] = 0x12;
	context->Dle[0] = 0x10;
	context->Enq[0] = 0x05;
	context->Eot[0] = 0x04;
	context->Etb[0] = 0x17;
	context->Etx[0] = 0x03;
	context->Nak[0] = 0x15;
	context->Rs[0] = 0x1E;
	context->Stx[0] = 0x02;
	context->crlf = false;

	// Default msgType to VisaK packet is not compiled with proper header
	if (! sess->pub.service.properties.type)
	{
		sprintf(context->msgType, "%.*s", sizeof(context->msgType) - 1, "visak");

		// Extract the FEC Service type from the received FEC INI configuration
	}
	else
	{
		sprintf(context->msgType, "%.*s", sizeof(context->msgType) - 1, sess->pub.service.properties.type);
	}

	// Enable the POS to send its request
	if (1 != PiPosSend(sess, enq(context), 1))
		return (eFailure);

	SysLog(LogDebug, "Packet type: %s", context->msgType);

	// Record the time the POS was enabled
	context->reqBgTime = time(0);

	return (eOk);
}								// PiApiResult_t BeginSession(PiSession_t *sess)
