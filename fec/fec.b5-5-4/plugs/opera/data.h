/*****************************************************************************

Filename:	lib/opera/data.h

Purpose:	Micros Opera HTTP/XML Message Set

			Compliance with specification: Micros "Opera Hotel Edition"
			Version 4.0 last updated on 5/20/2001, "Version Number" 2.02.

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:49 $
 * $Header: /home/hbray/cvsroot/fec/lib/opera/data.h,v 1.3.4.3 2011/09/24 17:49:49 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.3.4.3  2011/09/24 17:49:49  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/23 19:54:00  hbray
 eliminate fecplugin.h

 Revision 1.3.4.1  2011/08/11 19:47:34  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:20  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:50  hbray
 Added cvs headers

 *

2009.03.25 joseph dionne		Created release 2.1.
*****************************************************************************/

#ident "@(#) $Id: data.h,v 1.3.4.3 2011/09/24 17:49:49 hbray Exp $ "

#include "include/stdapp.h"
#include <include/libnx.h>
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"

#define VERSION		"4.0"		// Current version
#define PERSISTENT	true		// Boolean value

// Definition of application plugin specific variables
typedef struct
{
	// Client side statistics
	Counts_t auth;				// Authorization packets
	Counts_t edc;				// EDC(settlement) packets
	Counts_t batch;			// Multiple (batch) API requests
	Counts_t heartbeats;		// Appl Heartbeat requests

	// Plugin specific variables below here
	boolean xmlHeaderRequired;

	boolean isEdc;				// Receiving settlement (EDC)
	boolean httpcrlf;			// Received "CRLF" for HTTP newlines
	char httpver[16];			// Received "HTTP/1.0" version tag
	char xmlver[64];			// Received "<?xml version="1.0" ?>" tag
} appData_t;


PiApiResult_t PiOperaSendData(PiSession_t *sess, char *data, int len);


// Required application plugin method declarations
PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);
