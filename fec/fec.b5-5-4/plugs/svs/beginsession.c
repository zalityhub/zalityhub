/*****************************************************************************

Filename:	lib/svs/beginsession.c

Purpose:	Radiant Systems SVS Implementation

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
 * $Date: 2011/07/27 20:22:23 $
 * $Header: /home/hbray/cvsroot/fec/lib/svs/beginsession.c,v 1.2 2011/07/27 20:22:23 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.2  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:55  hbray
 Added cvs headers

 *

2009.10.01 joseph dionne		Ported to new FEC Version 4.2
2009.07.14 joseph dionne		Removed appDatalen, FEC clears appData.
2009.06.26 joseph dionne		Created release 3.4
*****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.2 2011/07/27 20:22:23 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

// 
PiApiResult_t
BeginSession(PiSession_t *sess)
{

	PiSessionVerify(sess);

	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Initialize the allowd ISO8583 Message Types we will receive
	// NOTE: values are in Network Byte Short Order for an Intel CPU
	// i.e. Big Endian order rather than normal Little Endian order
	memcpy(context->validIsoMsg, (short[])
		   {
		   0x0001, 0x0002, 0x2002, 0x0003, 0x2004, 0x0008, 0x0000}, sizeof(context->validIsoMsg));

	return (eOk);
}								// PiApiResult_t BeginSession(PiSession_t *sess)
