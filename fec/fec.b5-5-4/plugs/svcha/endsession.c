/*****************************************************************************

Filename:	lib/svcha/endsession.c

Purpose:	MICROS SVCHA XML Message Set

			Compliance with specification: MICROS Standard SVC Interface
			Revision 2.5 last updated on 4/05/2005

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

            The load library destructor method.  Called just prior to the
            POS connection tear down.  Release any resources allocated, and
            do any required termination clean up to include notifing POS
            of connection termination.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:22 $
 * $Header: /home/hbray/cvsroot/fec/lib/svcha/endsession.c,v 1.2 2011/07/27 20:22:22 hbray Exp $
 *
 $Log: endsession.c,v $
 Revision 1.2  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:54  hbray
 Added cvs headers

 *

2009.10.30 joseph dionne		Created at release 5.9
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
	if ((context))
	{
		// Release XML working storage
		if ((context->xmlData.save))
		{
			memset(context->xmlData.save, 0, (6 * context->xmlData.len));
			free(context->xmlData.save);
		}						// if ((context->xmlData.save))
	}							// if ((context))

	return (eOk);
}								// int EndSession(PiSession_t *sess)
