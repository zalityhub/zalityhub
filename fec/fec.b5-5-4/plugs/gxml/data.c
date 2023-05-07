/*****************************************************************************

Filename:	lib/gxml/data.c

Purpose:	GCS HTTP/XML Message Set

			Compliance with specification: "GCS XML Specification"
			Version 4.1.1 last updated on 8/16/2005, by Theron Crissey

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/08/17 17:58:57 $
 * $Header: /home/hbray/cvsroot/fec/lib/gxml/data.c,v 1.2.4.1 2011/08/17 17:58:57 hbray Exp $
 *
 $Log: data.c,v $
 Revision 1.2.4.1  2011/08/17 17:58:57  hbray
 *** empty log message ***

 Revision 1.2  2011/07/27 20:22:15  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:41  hbray
 Added cvs headers

 *

2010.09.01 joseph dionne		Added SQLite to allow all requests, including
								DepositRequests, to multi thread, i.e. send
								without waiting for response.
2009.09.22 joseph dionne		Ported to new FEC Version 4.3
*****************************************************************************/

#ident "@(#) $Id: data.c,v 1.2.4.1 2011/08/17 17:58:57 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"
#include "include/http.h"

// Application plugin release version, and connect status
PiLibSpecs_t PluginVersion = {

// Application plugin release version, and connect status data
#ifndef PLUGINNAME
	"libgxml.so",
#else
	PLUGINNAME,
#endif

#ifndef VERSION
	"5.9",						// Default version value
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
// This array must match the positional order of the libsym_t definition
char *(PluginMethods[]) =
{								// plugin method textual name
	"Load",						// plugin app object initializer
	"Unload",					// plugin app object destructor
	"BeginSession",				// plugin app called at start of new session
	"EndSession",				// plugin app called at end of session
	"ReadRequest",				// plugin app read POS request
	"SendResponse",				// plugin app write POS response
	0							// Array must be NULLP terminated
};

// Load library scope helper method definition
int (*GxmlFilePrintf) (FILE *, char *, ...) = 0;

PiApiResult_t
GxmlErrorResponse(PiSession_t *sess, int msgCode)
{
	appData_t *ctx = (appData_t*)sess->pub.context->data;
	char buf[4 * 1024] = { 0 };
	char *bp = buf;
	char *msgString = 0;
	char httpTime[512] = { 0 };
	int len;

	PiSessionVerify(sess);

	// Get the current time in HTTP format
	HttpGetTime(time(0), httpTime);

	// Validate the return code, or assign the default return code
	// NOTE: "default:" fall through to "eHttpInternalError:" by design
	switch (msgCode)
	{
	default:
	case eHttpInternalError:
		msgCode = eHttpInternalError;
		msgString = "Internal Error";
		break;

	case eHttpServiceUnavailable:
		msgString = "Service Unavailable";
		break;

	case eHttpNoContent:
		msgString = "No Content";
		break;
	} // switch(msgStringCode)

	// Use the HTTP version from the request
	if (*ctx->httpver)
	{
		bp += sprintf(bp, "%s ", ctx->httpver);
	}
	else
	{
		bp += sprintf(bp, "%s ", "HTTP/1.0");
	}

	// Add the Response Code and text
	bp += sprintf(bp, "%d %s\r\n", msgCode, msgString);

	// Add the rest of the HTTP headers
	bp += sprintf(bp,
		"Content-Type: %s\r\n"
		"Server: %s\r\n"
		"Date: %s\r\n"
		"Last-modified: %s\r\n"
		"Content-length: %d\r\n"
		"\r\n",
		"text/xml", "Fusebox", httpTime, httpTime, 0);
	len = bp - buf;

	// Write invalid HTTP response to POS
	if (len != PiPosSend(sess, buf, len))
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return (eFailure);
	} // if (len != PiPosSend(sess,buf,len))

	return (eDisconnect);
} // PiApiResult_t GxmlErrorResponse(PiSession_t *sess,int msgCode)
