/*****************************************************************************

Filename:	lib/pcard/data.h

Purpose:	PropertyCard Message Set

			Compliance with specification: PropertyCard
			Version 4.2 last updated on 5/20/2001, "Version Number" 2.02.

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:49 $
 * $Header: /home/hbray/cvsroot/fec/lib/pcard/data.h,v 1.3.4.2 2011/09/24 17:49:49 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.3.4.2  2011/09/24 17:49:49  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:54:00  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:21  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:51  hbray
 Added cvs headers

 *

2009.06.03 joseph dionne		Updated to support release 3.4
2009.05.29 harold bray		Created release 3.1.
*****************************************************************************/

#ident "@(#) $Id: data.h,v 1.3.4.2 2011/09/24 17:49:49 hbray Exp $ "

#include "include/stdapp.h"
#include <include/libnx.h>
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"
	
#define VERSION         "4.2"	// Current version
#define PERSISTENT      false	// Boolean value


typedef enum { LineTerminationUnix, LineTerminationDos } LineTerminationProtocol; 

// Definition of application plugin specific variables
typedef struct 
{
	
// Client side statistics
	Counts_t auth;				// Authorization packets
	Counts_t edc;				// EDC(settlement) packets
	Counts_t batch;			// Multiple (batch) API reques                                                                                          ts
	 
// Plugin specific variables below here
	 int		servicePort;
	 int		altPort;

	 LineTerminationProtocol lineTerminationProtocol;
	 char		*inputEntityEncoding;
	 char		*outputEntityEncoding;

	// NOTE: a '1' in the first byte indicates the request is the last to
	// be received, a '0' indicates more messages follow, i.e. settlement.
	// NOTE: a 'C' in the second bytes indicates request is an authorization
	// otherwise request is part of an EDC upload
	 char		msgNa[12][8];		// Textual Micro Message Type names
 } appData_t;


// Required application plugin method declarations
	PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);
