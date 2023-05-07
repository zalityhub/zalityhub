/*****************************************************************************

Filename:	lib/svs/load.c

Purpose:	Radiant POS SVS

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			Called when the plug-in is first loaded.
			Do any global initializations here

			Return true upon success, or false upon failure/error condition

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:23 $
 * $Header: /home/hbray/cvsroot/fec/lib/svs/load.c,v 1.2 2011/07/27 20:22:23 hbray Exp $
 *
 $Log: load.c,v $
 Revision 1.2  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:55  hbray
 Added cvs headers

 *

2009.10.01 joseph dionne		Ported to release v4.2
*****************************************************************************/

#ident "@(#) $Id: load.c,v 1.2 2011/07/27 20:22:23 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
Load()
{

	return eOk;
}
