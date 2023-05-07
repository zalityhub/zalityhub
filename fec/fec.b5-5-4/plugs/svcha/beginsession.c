/*****************************************************************************

Filename:	lib/svcha/beginsession.c

Purpose:	MICROS SVCHA XML Message Set

			Compliance with specification: MICROS Standard SVC Interface
			Revision 2.5 last updated on 4/05/2005

            The load library instantiation method.  Called directly after
            the POS connection established, but prior to any communication.

            Allocate working memory storage from heap and any other required
            resources needed to process the specific protocol/packet type.

            Initialize the communications channel, i.e. send ENQ for example.

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:22 $
 * $Header: /home/hbray/cvsroot/fec/lib/svcha/beginsession.c,v 1.2 2011/07/27 20:22:22 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.2  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:54  hbray
 Added cvs headers

 *

2009.10.30 joseph dionne		Created at release 5.9
*****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.2 2011/07/27 20:22:22 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
BeginSession(PiSession_t *sess)
{

	PiSessionVerify(sess);

	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));

	return eOk;
}								// int BeginSession(PiSession_t *sess)
