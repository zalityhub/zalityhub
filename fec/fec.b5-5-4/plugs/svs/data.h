/*****************************************************************************

Filename:	lib/svs/data.h

Purpose:	Radiant Systems SVS Implementation

			Compliance with specification: Radiant Systems, Inc.
					"SVS Implementation Description.doc"
							Version 1: First Draft,
								08/13/2004
							William H. Posey

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:51 $
 * $Header: /home/hbray/cvsroot/fec/lib/svs/data.h,v 1.3.4.2 2011/09/24 17:49:51 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.3.4.2  2011/09/24 17:49:51  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:54:00  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:55  hbray
 Added cvs headers

 *

2009.10.01 joseph dionne		Ported to new FEC Version 4.2
2009.06.26 joseph dionne		Created release 3.4
*****************************************************************************/

#ident "@(#) $Id: data.h,v 1.3.4.2 2011/09/24 17:49:51 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"

#define VERSION		"4.2"		// Current version
#define PERSISTENT	true		// Boolean value

#if 0
typedef struct
{
} svsiso_t;
#endif

#ifndef MAX_HGCS_PACKET
#define MAX_HGCS_PACKET (8 * 1024)
#endif

// Definition of application plugin specific variables
typedef struct
{
	// Client side statistics
	Counts_t auth;				// Authorization packets
	Counts_t edc;				// EDC(settlement) packets
	Counts_t batch;			// Multiple (batch) API requests
	Counts_t heartbeats;		// Appl Heartbeat requests

	// Plugin specific variables below here
#	define			MAX_ISO_MSG 6	// Allowed ISO8583 message types
	unsigned short validIsoMsg[1 + MAX_ISO_MSG];
} appData_t;

// Required application plugin method declarations
PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);
