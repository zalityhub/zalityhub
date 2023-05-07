/*****************************************************************************

Filename:	lib/gcreq/unload.c

Purpose:	API

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The plug-in termination routine.
			Called prior to the framework shutting down.

			Return true upon success, or false upon failure/error condition

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:15 $
 * $Header: /home/hbray/cvsroot/fec/lib/gcreq/unload.c,v 1.2 2011/07/27 20:22:15 hbray Exp $
 *
 $Log: unload.c,v $
 Revision 1.2  2011/07/27 20:22:15  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:41  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: unload.c,v 1.2 2011/07/27 20:22:15 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
Unload()
{

	return eVirtual;
}								// int Load()
