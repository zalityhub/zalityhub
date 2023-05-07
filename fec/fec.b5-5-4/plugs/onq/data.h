/*****************************************************************************

Filename:	lib/onq/data.h

Purpose:	GCS Hilton OnQ Message Set

			Compliance with specification: HiltonOnQMessageSet-AC.doc,
			last updated on 3/5/2008.

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:48 $
 * $Header: /home/hbray/cvsroot/fec/lib/onq/data.h,v 1.3.4.2 2011/09/24 17:49:48 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.3.4.2  2011/09/24 17:49:48  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:54:00  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:20  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:49  hbray
 Added cvs headers

 *

2009.02.19 joseph dionne		Created release 1.4.  Supports authorizations
								only at this release.
*****************************************************************************/

#ident "@(#) $Id: data.h,v 1.3.4.2 2011/09/24 17:49:48 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"


#define VERSION		"4.2"
#define PERSISTENT	true

// GCS Hilton OnQ Message type descriptor
#define MAXONQ		6			// "00","01","02","03","04","05"
typedef struct
{
	int type;					// Message Type
	int length;					// Packet length
} onqmsg_t;

// Definition of application plugin specific variables
typedef struct
{
	// Client side statistics
	Counts_t auth;				// Authorization packets
	Counts_t edc;				// EDC(settlement) packets
	Counts_t batch;			// Multiple (batch) API requests
	Counts_t heartbeats;		// Appl Heartbeat requests

	// Plugin specific variables below here
	onqmsg_t packets[MAXONQ];	// Request packet identification
	boolean newline;			// Newline follows packet
	boolean crlf;				// Received Windoze style newline
} appData_t;

// Required application plugin method declarations
PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);
