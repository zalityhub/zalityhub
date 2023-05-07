/*****************************************************************************

Filename:	lib/mpms/beginsession.c

Purpose:	Marriott PMS2Way Message Set

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
 * $Date: 2011/07/27 20:22:16 $
 * $Header: /home/hbray/cvsroot/fec/lib/mpms/beginsession.c,v 1.2 2011/07/27 20:22:16 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.2  2011/07/27 20:22:16  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:43  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.2 2011/07/27 20:22:16 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
BeginSession(PiSession_t *sess)
{

	PiSessionVerify(sess);

	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));

	return (eOk);
}								// int instantiate(sess_t *sess)
