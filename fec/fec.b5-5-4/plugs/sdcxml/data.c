/*****************************************************************************

Filename:	lib/sdcxml/data.c

Purpose:	SDC XML Message Set

			Compliance with specification: SDC "XML"
			Version 4.0 last updated on 5/20/2001, "Version Number" 2.02.

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:22 $
 * $Header: /home/hbray/cvsroot/fec/lib/sdcxml/data.c,v 1.2 2011/07/27 20:22:22 hbray Exp $
 *
 $Log: data.c,v $
 Revision 1.2  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:53  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: data.c,v 1.2 2011/07/27 20:22:22 hbray Exp $ "

// Application plugin data/method header declarations
#define C_DATA
#include "data.h"

// Application plugin release version, and connect status
PiLibSpecs_t PluginVersion = {

// Application plugin release version, and connect status data
#ifndef PLUGINNAME
	"libsdcxml.so",
#else
	PLUGINNAME,
#endif

#ifndef VERSION
	"4.0",						// Default version value
#else
	VERSION,					// Value defined in data.h
#endif

#ifndef PERSISTENT
	false,						// Default persistence value
#else
	PERSISTENT,					// Value defined in data.h
#endif
};

// The following is required by the plugin loader.  DO NOT ALTER!
// This array must match the positional ordere of the libsym_t definition
char *(PluginMethods[]) =
{								// plugin method textual name
	"Load",						// plugin app object initializer
		"Unload",				// plugin app object de-initializer
		"BeginSession",			// plugin app called at start of new session
		"EndSession",			// plugin app called at end of session
		"ReadRequest",			// plugin app read POS request
		"SendResponse",			// plugin app write POS response
		"ToString",				// plugin app ToString function
		0						// Array must be NULLP terminated
};
