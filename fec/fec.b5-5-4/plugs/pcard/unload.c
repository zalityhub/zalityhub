/*****************************************************************************

Filename:	lib/pcard/unload.c

Purpose:	API

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The plug-in termination routine.
			Called prior to the framework shutting down.

			Return true upon success, or false upon failure/error condition

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:21 $
 * $Header: /home/hbray/cvsroot/fec/lib/pcard/unload.c,v 1.2 2011/07/27 20:22:21 hbray Exp $
 *
 $Log: unload.c,v $
 Revision 1.2  2011/07/27 20:22:21  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:52  hbray
 Added cvs headers

 *
$History: $
2009.09.22 harold bray          Updated to support release 4.2
 * 
*****************************************************************************/

#ident "@(#) $Id: unload.c,v 1.2 2011/07/27 20:22:21 hbray Exp $ "

// Application plugin context/method header declarations
#include "data.h"

PiApiResult_t
Unload()
{

	return eOk;
}								// int Load()
