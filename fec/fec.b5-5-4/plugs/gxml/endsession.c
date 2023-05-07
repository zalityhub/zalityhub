/*****************************************************************************

Filename:	lib/gxml/endsession.c

Purpose:	GCS HTTP/XML Message Set

			Compliance with specification: "GCS XML Specification"
			Version 4.1.1 last updated on 8/16/2005, by Theron Crissey

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT


			The load library destructor method.  Called just prior to the
			POS connection tear down.  Release any resources allocated, and
			do any required termination clean up to include notifing POS
			of connection termination.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/08/17 17:58:57 $
 * $Header: /home/hbray/cvsroot/fec/lib/gxml/endsession.c,v 1.2.4.1 2011/08/17 17:58:57 hbray Exp $
 *
 $Log: endsession.c,v $
 Revision 1.2.4.1  2011/08/17 17:58:57  hbray
 *** empty log message ***

 Revision 1.2  2011/07/27 20:22:15  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:41  hbray
 Added cvs headers

 *

2009.09.22 joseph dionne		Ported to new FEC Version 4.3
*****************************************************************************/

#ident "@(#) $Id: endsession.c,v 1.2.4.1 2011/08/17 17:58:57 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
EndSession(PiSession_t *sess)
{
	PiSessionVerify(sess);

	appData_t *ctx = (appData_t*)sess->pub.context->data;

	// Release the SQLite resources
	if ((ctx->insertDetail))	sqlite3_finalize(ctx->insertDetail);
	if ((ctx->updateDetail))	sqlite3_finalize(ctx->updateDetail);
	if ((ctx->insertHeader))	sqlite3_finalize(ctx->insertHeader);
	if ((ctx->getLastDep_id))	sqlite3_finalize(ctx->getLastDep_id);
	if ((ctx->getXmlLength))	sqlite3_finalize(ctx->getXmlLength);
	if ((ctx->findHeader))		sqlite3_finalize(ctx->findHeader);
	if ((ctx->getRespCount))	sqlite3_finalize(ctx->getRespCount);
	if ((ctx->getXmlResp))		sqlite3_finalize(ctx->getXmlResp);
	if ((ctx->setAtEof))		sqlite3_finalize(ctx->setAtEof);
	if ((ctx->db))				sqlite3_close(ctx->db);
	if (!(ctx->leaveDb))
		if ((*ctx->dbName))		remove(ctx->dbName);
	ctx->insertDetail	= 0;
	ctx->updateDetail	= 0;
	ctx->insertHeader	= 0;
	ctx->getLastDep_id	= 0;
	ctx->getXmlLength	= 0;
	ctx->findHeader		= 0;
	ctx->getRespCount	= 0;
	ctx->getXmlResp		= 0;
	ctx->setAtEof		= 0;
	ctx->db				= 0;

	PiDeleteContext(sess, sess->pub.context);

	return (eOk);
} // PiApiResult_t EndSession(PiSession_t *sess)
