/*****************************************************************************

Filename:	lib/api/data.h

Purpose:	ProtoBase API plugin

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:39 $
 * $Header: /home/hbray/cvsroot/fec/lib/api/data.h,v 1.3.4.2 2011/09/24 17:49:39 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.3.4.2  2011/09/24 17:49:39  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:53:58  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:38  hbray
 Added cvs headers

 *

2009.01.12 joseph dionne		Created release 1.2.  Supports authorizations
								only at this release.
*****************************************************************************/

#ident "@(#) $Id: data.h,v 1.3.4.2 2011/09/24 17:49:39 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"




#define VERSION		"4.0"		// Current version
#define PERSISTENT	true		// Boolean value

// Definition of application plugin specific variables
typedef struct appData_t
{
	Counts_t auth;				// Authorization packets

	boolean seenEot;
	boolean	crlf;				// API using Windoze Newline

	boolean	t70proxy;
	boolean	persistent;
} appData_t;

#define EOT			(char)0x04
#define RS			(char)0x1E


// Required application plugin method declarations
PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);
