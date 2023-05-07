/*****************************************************************************

Filename:	lib/opera/operasendrsp.c

Purpose:	Micros Opera HTTP/XML Message Set

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The load library transaction response send method.  Called after
			the one of the following events.
				1. A response was received from the processor
				2. A response time out event
				3. The processor service is offline, i.e. down
				4. Prior to connection tear down after normal or abnormal
				processing

			This method is where all communications protocol is processed.

			This method is called just prior to tearing down the TCP/IP link
			so that termination protocol can be sent the the POS.

			Return true upon success, or false upon failure/error condition

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:49 $
 * $Header: /home/hbray/cvsroot/fec/lib/opera/operasendrsp.c,v 1.3.4.2 2011/09/24 17:49:49 hbray Exp $
 *
 $Log: operasendrsp.c,v $
 Revision 1.3.4.2  2011/09/24 17:49:49  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:20  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:51  hbray
 Added cvs headers

 *

2009.05.18 joseph dionne		Send multiple "word" values back in XML, scanf
                                needed to be "told" to scan value up to LF.
								Also allow quotes to be returned in XML value.
2009.04.03 joseph dionne		The initial release sent the entire XML
								container, less the "xml version" tag, and
								received an XML container back.  This new
								version only received "Field name" and value
								responses back.  The first pair is used to
								determine the type of HTTP response.
2009.03.25 joseph dionne		Created release 2.1.
*****************************************************************************/

#ident "@(#) $Id: operasendrsp.c,v 1.3.4.2 2011/09/24 17:49:49 hbray Exp $ "

// Application plugin data/method header declarations

#include "data.h"
#include "include/http.h"


// Local (file) scope helper methods
static int xmlConcat(char *tgt, char *http, char *content);

PiApiResult_t
SendResponse(PiSession_t *sess)
{

	PiSessionVerify(sess);

	HostRequest_t *response = (HostRequest_t *)alloca(sizeof(HostRequest_t));
	if (response == NULL)
		SysLog(LogFatal, "alloca failed");

	if (PiHostRecv(sess, response) != response->len)
	{
		PiException(LogError, "PiHostRecv", eFailure, "failed");
		return eFailure;
	}

	return PiOperaSendData(sess, response->data, response->len);
}								// int sendResponse(fecconn_t *sess)



PiApiResult_t
PiOperaSendData(PiSession_t *sess, char *data, int len)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;
	
	char *nl = "\r\n";
	char httpTime[64];

	memset(httpTime, 0, sizeof(httpTime));
	char httpHead[512];

	memset(httpHead, 0, sizeof(httpHead));
	char *buf;
	char *bp;

	// Count this response
	++context->auth.outPkts;

	// Get the current GMT time for response
	HttpGetTime(time(0), httpTime);

	// Allocate a transmission buffer, packet length plus IP header
	// NOTE: allocate from STACK memory, DO NOT FREE
	// NOTE: allocating addition space for the HTTP and XML "headers"
	if (!(buf = alloca((2 * len) + 16384)))
		SysLog(LogFatal, "alloca failed");

	memset(buf, 0, (2 * len) + 16384);

	// Convert "Field_name=value" pairs into XML container
	if (!(context->httpcrlf))
		nl++;					// sending UNIX-style newlines

	if (len <= 0)
	{
		PiException(LogError, __FUNC__, eFailure, "response->len == %d; ignoring", len);
		return eOk;
	}

	char *sp = alloca(len + 10);

	memset(sp, 0, len + 10);
	memcpy(sp, data, len);

	if (sp[strlen(sp) - 1] != '\n')
		strcat(sp, "\n");		// insure lf termination

	for (bp = buf; *sp;)
	{
		char line[2 * 2048] = { 0 };
		int lineLen = -1;
		char name[128] = { 0 };
		char value[2 * 2048] = { 0 };
		int ii;

		// Parse off a line of text up to and including the LF('\n')
		sscanf(sp, "%[^\n]%*[\n]%n", line, &lineLen);
		if (lineLen < 0)
		{
			break;				// TODO: done?
		}
		sp += lineLen;

		// Blank line?
		if (!strlen(line))
			continue;

		// Invalid HGCS response
		if ((bp == buf) && !stristr(line, "ResponseType"))
		{
			// Build the HTTP Error Entity-Header sprintf format string
			sprintf(httpHead, "HTTP/%s 500 internal error%s"	//  context->httpver,nl,
					"Server: PMS%s"	//  nl,
					"Date: %s%s"	//  httpTime,nl,
					"Last-modified: %s%s"	//  httpTime,nl,
					"Content-length: %s%s"	//  "%d",nl,
					"Content-type: text/xml%s"	//  nl,
					"%s"		//  nl,
					"%s",		//  "%s" for the XML Content
					context->httpver, nl, nl, httpTime, nl, httpTime, nl, "%d", nl, nl, nl, "%s");

			// Build the XML Error Response Entity-Body
			bp += sprintf(bp, "%s<Response type=\"Error\">"
						  "<Field name=\"ErrorCode\">%d</Field>" "<Field name=\"ErrorDescription\">%s</Field>", context->xmlver, 9999, "Internal Error");

			break;
		}						// if ((bp == buf) && !stristr(line,"ResponseType"))

		// Parse the "FieldName,Value" pair
		// JJD TODO: Need to test format below for parsing leading whitespace
		ii = sscanf(line, "%[^\t ,]%*[\t ,]%[^\n]s", name, value);

		// Add each "Field name=Value" pair as XML Field(s)
		if ((bp != buf))
		{
			bp += sprintf(bp, "<Field name=\"%s\">%s</Field>", name, value);
		}
		else					// Build the start of the HTTP message and XML container
		{
			// Build the HTTP Response Entity-Header sprintf format string
			sprintf(httpHead, "HTTP/%s %s %s"	//  context->httpver,nl,
					"Server: PMS%s"	//  nl,
					"Date: %s%s"	//  httpTime,nl,
					"Last-modified: %s%s"	//  httpTime,nl,
					"Content-length: %s%s"	//  "%d",nl,
					"Content-type: text/xml%s"	//  nl,
					"%s"		//  nl,
					"%s",		//  "%s" for the XML Content
					context->httpver, !stristr("error", value)	//  if "ResponseType,Error"
					? "200 OK"	//  then send HTTP 200
					: "500 internal error",	//  else send HTTP 500
					nl, nl, httpTime, nl, httpTime, nl, "%d", nl, nl, nl, "%s");

			// Start the response packet
			bp += sprintf(bp, "%s<Response type=\"%s\">", context->xmlver, value);

		}						// if ((bp == buf))
	}							// for(bp = buf;*sp;)

	// Append the XML container terminator
	sprintf(bp, "</Response>");
	len = xmlConcat(buf, httpHead, buf);

	if (PiPosSend(sess, buf, len) != len)
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return eFailure;
	}

	return eOk;
}								// int sendResponse(fecconn_t *sess)


static int
xmlConcat(char *tgt, char *http, char *content)
{
	int len = strlen(content);
	char lenBytes[16];
	char *tmp;

	// Get the length of Content-length digits
	sprintf(lenBytes, "%d", len);

	// Allocate STACK memory to combine HTTP and Content
	tmp = alloca(1 + strlen(http) + len + strlen(content) + 16384);

	// Combine HTTP and Content
	len = sprintf(tmp, http, len, content);

	// Copy into caller's target memory
	strcpy(tgt, tmp);

	// Return the length of combined HTTP and Content
	return (len);
}								// int xmlConcat(char *tgt,char *http,char *content)
