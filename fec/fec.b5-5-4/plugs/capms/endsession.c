/*****************************************************************************

Filename:	lib/capms/endsession.c

Purpose:	Micros CA/EDC PMS Front-End Communications plugin

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The load library destructor method.  Called just prior to the
			POS connection tear down.  Release any resources allocated, and
			do any required termination clean up to include notifing POS
			of connection termination.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:14 $
 * $Header: /home/hbray/cvsroot/fec/lib/capms/endsession.c,v 1.2 2011/07/27 20:22:14 hbray Exp $
 *
 $Log: endsession.c,v $
 Revision 1.2  2011/07/27 20:22:14  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:40  hbray
 Added cvs headers

 *

2009.05.14 joseph dionne		Created release 2.9
*****************************************************************************/

#ident "@(#) $Id: endsession.c,v 1.2 2011/07/27 20:22:14 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
EndSession(PiSession_t *sess)
{

	PiSessionVerify(sess);

	PiDeleteContext(sess, sess->pub.context);
	return eOk;
}								// int EndSession
