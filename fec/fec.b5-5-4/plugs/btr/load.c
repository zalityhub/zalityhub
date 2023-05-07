/*****************************************************************************

Filename:	lib/btr/load.c

Purpose:	API

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			Called when the plug-in is first loaded.
			Do any global initializations here

			Return true upon success, or false upon failure/error condition

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:14 $
 * $Header: /home/hbray/cvsroot/fec/lib/btr/load.c,v 1.2 2011/07/27 20:22:14 hbray Exp $
 *
 $Log: load.c,v $
 Revision 1.2  2011/07/27 20:22:14  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:39  hbray
 Added cvs headers

 *
$History: $
2009.09.22 harold bray          Updated to support release 4.2
 * 
*****************************************************************************/

#ident "@(#) $Id: load.c,v 1.2 2011/07/27 20:22:14 hbray Exp $ "

// Application plugin context/method header declarations
#include "data.h"

PiApiResult_t
Load()
{

	return eOk;
}								// int Load()
