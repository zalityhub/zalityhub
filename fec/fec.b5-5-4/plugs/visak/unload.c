/*****************************************************************************

Filename:	lib/visak/unload.c

Purpose:	API

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The plug-in termination routine.
			Called prior to the framework shutting down.

			Return true upon success, or false upon failure/error condition

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:24 $
 * $Header: /home/hbray/cvsroot/fec/lib/visak/unload.c,v 1.2 2011/07/27 20:22:24 hbray Exp $
 *
 $Log: unload.c,v $
 Revision 1.2  2011/07/27 20:22:24  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:57  hbray
 Added cvs headers

 *

2009.09.22 joseph dionne		Ported to new FEC Version 4.2
*****************************************************************************/

#ident "@(#) $Id: unload.c,v 1.2 2011/07/27 20:22:24 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
Unload()
{

	return eOk;
}								// int Load()
