/*****************************************************************************

Filename:	lib/hsi/data.h

Purpose:	Micros CA/EDC PMS Front-End Communications plugin
            Merely a stub...

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:42 $
 * $Header: /home/hbray/cvsroot/fec/lib/hsi/data.h,v 1.3.4.2 2011/09/24 17:49:42 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.3.4.2  2011/09/24 17:49:42  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:53:59  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:15  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:42  hbray
 Added cvs headers

 *

2010.03.27 harold bray  		Created release 1.0
 *****************************************************************************/

#ident "@(#) $Id: data.h,v 1.3.4.2 2011/09/24 17:49:42 hbray Exp $ "

#include "include/stdapp.h"
#include <include/libnx.h>
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"

#define VERSION		"1.0"		// Current version
#define PERSISTENT	true		// Boolean value

// Definition of application plugin specific variables
typedef struct
{
	int		filler;
} appData_t;


// Required application plugin method declarations
PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);
