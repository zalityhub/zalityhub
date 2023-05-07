/*****************************************************************************

Filename:	lib/svs/data.c

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
 * $Date: 2011/07/27 20:22:23 $
 * $Header: /home/hbray/cvsroot/fec/lib/svs/data.c,v 1.2 2011/07/27 20:22:23 hbray Exp $
 *
 $Log: data.c,v $
 Revision 1.2  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:55  hbray
 Added cvs headers

 *

2009.10.01 joseph dionne		Ported to new FEC Version 4.2
2009.06.26 joseph dionne		Created release 3.4
*****************************************************************************/

#ident "@(#) $Id: data.c,v 1.2 2011/07/27 20:22:23 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

// Application plugin release version, and connect status
PiLibSpecs_t PluginVersion = {

// Application plugin release version, and connect status data
#ifndef PLUGINNAME
	"libsvs.so",
#else
	PLUGINNAME,
#endif

#ifndef VERSION
	"4.2",						// Default version value
#else
	VERSION,					// Value defined in data.h
#endif

#ifndef PERSISTENT
	true,						// Default persistence value
#else
	PERSISTENT,					// Value defined in data.h
#endif
};								// PiVersion_t PluginVersion

// The following is required by the plugin loader.  DO NOT ALTER!
// This array must match the positional order of the libsym_t definition
char *(PluginMethods[]) =
{								// plugin method textual name
	"Load",						// plugin app object initializer
		"Unload",				// plugin app object destructor
		"BeginSession",			// plugin app called at start of new session
		"EndSession",			// plugin app called at end of session
		"ReadRequest",			// plugin app read POS request
		"SendResponse",			// plugin app write POS response
		"ToString",				// plugin app ToString function
		0						// Array must be NULLP terminated
};
