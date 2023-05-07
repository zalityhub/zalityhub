/*****************************************************************************

Filename:	lib/hsi/hsisendrsp.c

Purpose:	Micros CA/EDC PMS Front-End Communications plugin
            Merely a stub...

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:16 $
 * $Header: /home/hbray/cvsroot/fec/lib/hsi/hsisendrsp.c,v 1.2 2011/07/27 20:22:16 hbray Exp $
 *
 $Log: hsisendrsp.c,v $
 Revision 1.2  2011/07/27 20:22:16  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:43  hbray
 Added cvs headers

 *

2010.03.27 harold bray  		Created release 1.0
 *****************************************************************************/

#ident "@(#) $Id: hsisendrsp.c,v 1.2 2011/07/27 20:22:16 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
SendResponse(PiSession_t *sess)
{

	PiSessionVerify(sess);
	PiException(LogError, __FUNC__, eVirtual, "not implemented");
	return eVirtual;
}
