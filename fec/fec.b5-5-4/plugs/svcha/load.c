/*****************************************************************************

Filename:	lib/svcha/load.c

Purpose:	MICROS SVCHA XML Message Set

			Compliance with specification: MICROS Standard SVC Interface
			Revision 2.5 last updated on 4/05/2005

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

            Called when the plug-in is first loaded.
            Do any global initializations here

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:22 $
 * $Header: /home/hbray/cvsroot/fec/lib/svcha/load.c,v 1.2 2011/07/27 20:22:22 hbray Exp $
 *
 $Log: load.c,v $
 Revision 1.2  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:54  hbray
 Added cvs headers

 *

2009.10.30 joseph dionne		Created at release 5.9
*****************************************************************************/

#ident "@(#) $Id: load.c,v 1.2 2011/07/27 20:22:22 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

PiApiResult_t
Load()
{

	return eOk;
}								// int Load()
