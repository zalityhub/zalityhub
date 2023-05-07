/*****************************************************************************

Filename:	lib/sdcxml/sdcxmlsendrsp.c

Purpose:	SDC XML Message Set

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
 * $Date: 2011/11/16 19:33:02 $
 * $Header: /home/hbray/cvsroot/fec/lib/sdcxml/sdcxmlsendrsp.c,v 1.3.4.7 2011/11/16 19:33:02 hbray Exp $
 *
 $Log: sdcxmlsendrsp.c,v $
 Revision 1.3.4.7  2011/11/16 19:33:02  hbray
 Updated

 Revision 1.3.4.6  2011/09/24 17:49:50  hbray
 Revision 5.5

 Revision 1.3.4.4  2011/09/01 14:49:47  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/23 12:03:15  hbray
 revision 5.5

 Revision 1.3  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:53  hbray
 Added cvs headers

 *

2009.06.03 joseph dionne		Updated to support release 3.4
2009.05.29 harold bray			Created release 3.1.
*****************************************************************************/

#ident "@(#) $Id: sdcxmlsendrsp.c,v 1.3.4.7 2011/11/16 19:33:02 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"
#include "include/http.h"


// Local (file) scope helper methods
static void ApiConcat(appData_t *context, char *xml, char *name, char *value);


static char*
TransformData(appData_t *context, char *bfr, int len)
{
	if ( stricmp(context->outputEntityEncoding, "url") == 0 )
		bfr = EncodeUrlCharacters(bfr, len);
	else if ( stricmp(context->outputEntityEncoding, "xml") == 0 )
		bfr = EncodeEntityCharacters(bfr, len);
	return bfr;
}


PiApiResult_t
SendResponse(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	HostRequest_t *response = (HostRequest_t *)alloca(sizeof(HostRequest_t));

	if (response == NULL)
	{
		PiException(LogError, "alloca", eFailure, "failed");
		return eFailure;
	}

	if (PiHostRecv(sess, response) != response->len)
	{
		PiException(LogError, "PiHostRecv", eFailure, "failed");
		return eFailure;
	}

	// Count this response
	++context->auth.outPkts;

	NxUid_t batchId = response->hdr.requestEcho;

// Determine a maximum output size based on how many api field lines there are...
	char *xmlResponse;
	int xmlResponseLen = strlen(response->data);	// initial assumption

	{
		char *tagData = "<API_Field>\r\n" "<Field_Number>%s</Field_Number>\r\n" "<Field_Value>%s</Field_Value>\r\n" "</API_Field>\r\n";
		for (char *ptr = response->data; *ptr; ++ptr)
		{
			if (*ptr == '\n')
				xmlResponseLen += strlen(tagData);
		}

		xmlResponseLen *= 4;	// that ought to cover it...
		xmlResponseLen += 4096;
	}

	SysLog(LogDebug, "Allocating %d for the response", xmlResponseLen);
	xmlResponse = alloca(xmlResponseLen);
	memset(xmlResponse, 0, xmlResponseLen);

	// Convert "Field_name=value" pairs into XML container

	if (response->len <= 0)
	{
		SysLog(LogDebug, "No response data to process");
		return eOk;
	}

	char *responseData = alloca(response->len + 1);

	memcpy(responseData, response->data, response->len);
	responseData[response->len] = '\0';

	SysLog(LogDebug, "Converting %d bytes", response->len);

	char *rptr = responseData;

	while (*rptr)
	{
		char *bol = rptr;
		char *eol = bol;

		while (*eol && *eol != '\n')
			++eol;

		int llen = (eol - bol);

		if (llen > 0)
		{
			char *line = alloca(llen + 1);	// room for the line

			memcpy(line, bol, llen);
			line[llen] = '\0';

			// now isolate the field nbr and its value
			char *fn = line;
			char *fv = line;

			while (*fv && *fv != ',')
				++fv;

			if (*fv == ',')
				*fv++ = '\0';	// chop

			while (*fv == ' ')
				++fv;			// remove leading spaces (one's which follow the ',' before the value

			char *ptr = &fv[strlen(fv) - 1];

			while (ptr >= fv && *ptr == '\r')
				*ptr-- = '\0';	// remove any trailing \r

			while (*fn == ' ' || *fn == '\r')
				++fn;			// remove any leading spaces or \r...

			if (strlen(fn) > 0)	// we have a field number
			{
				SysLog(LogDebug, "fn=%s, fv=%s", fn, fv);
				ApiConcat(context, xmlResponse, fn, fv);
			}
		}

		rptr = eol;
		if (*rptr != '\0')
			++rptr;				// next line
	}

// build the response

	if ( PiBatchSize(sess, batchId) == 0 )		// if first response, open ProtoBase_Transaction_Batch/Settlement_Batch tags
	{
		char *tmp = alloca(4096);
		sprintf(tmp, "<ProtoBase_Transaction_Batch><Settlement_Batch>%s</Settlement_Batch>", (response->hdr.more)?"true":"false");
		int len = strlen(tmp);
		if (PiBatchPush(sess, tmp, len, batchId) != len)
		{
			PiException(LogError, "PiBatchPush", eFailure, "of %s failed", PiBatchToString(sess, batchId));
			return eFailure;
		}
	}

	char *responseString = alloca((strlen(xmlResponse) * 2) + 4096);
	sprintf(responseString, "<Transaction>\r\n%s</Transaction>", xmlResponse);
	int len = strlen(responseString);
	if (PiBatchPush(sess, responseString, len, batchId) != len)		// send the response
	{
		PiException(LogError, "PiBatchPush", eFailure, "of %s failed", PiBatchToString(sess, batchId));
		return eFailure;
	}

	if ( response->hdr.more )		// more to come
	{
		SysLog(LogDebug, "%s", PiBatchToString(sess, batchId));
		return eOk;
	}

// response complete; release the batched data
	char *tmp = alloca(4096);

// close ProtoBase_Transaction_Batch
	SysLog(LogDebug, "Release batch %s", NxUidToString(batchId));

	{
		strcpy(tmp, "</ProtoBase_Transaction_Batch>");
		int len = strlen(tmp);
		if (PiBatchPush(sess, tmp, len, batchId) != len)
		{
			PiException(LogError, "PiBatchPush", eFailure, "of %s failed", PiBatchToString(sess, batchId));
			return eFailure;
		}
	}

	if ( context->outputFinalEot )	// I owe an EOT...
	{
		char *eot = "\x004";
		int len = strlen(eot);
		if (PiBatchPush(sess, eot, len, batchId) != len)
		{
			PiException(LogError, "PiBatchPush", eFailure, "EOT of %s failed", PiBatchToString(sess, batchId));
			return eFailure;
		}
	}

	if ( context->postHeader != NULL )
	{
		char httpTime[64];

		HttpGetTime(time(0), httpTime); // Get the current GMT time for response

		// Build the HTTP Response Header
		sprintf(tmp,
						// fmt:
						"%s 200 OK \r\n"	//  context->httpver
						"Server: sdcxml\r\n"	//
						"Date: %s\r\n"	//  httpTime
						"Last-Modified: %s\r\n"	//  httpTime
						"Content-Type: text/xml\r\n"	//
						"Content-Length: %d\r\n"	//
						"\r\n"	//
						,
						// args:
						context->postHeader,	// Response line
						httpTime,	// Date
						httpTime,	// Last-modified
						PiBatchSize(sess, batchId));
		free(context->postHeader);	// release header
		context->postHeader = NULL;		// used...

		int hlen = strlen(tmp);
		if (PiPosSend(sess, tmp, hlen) != hlen)
		{
			PiException(LogError, "PiPosSend", eFailure, "failed: %s", tmp);
			return eFailure;
		}
	}

// Send payload
	if (PiBatchPosSend(sess, batchId) != 0 )		// send the batch
	{
		PiException(LogError, "PiBatchPosSend", eFailure, "");
		return eFailure;
	}
	if (PiBatchClose(sess, batchId) != 0 )		// close the batch
	{
		PiException(LogError, "PiBatchClose", eFailure, "");
		return eFailure;
	}

	return eOk;
}


static void
ApiConcat(appData_t *context, char *xml, char *name, char *value)
{
	char *tmp = alloca((strlen(name) + strlen(value)) + 1024);
	char *tvalue = TransformData(context, value, strlen(value));
	sprintf(tmp, "<API_Field>\r\n" "<Field_Number>%s</Field_Number>\r\n" "<Field_Value>%s</Field_Value>\r\n" "</API_Field>\r\n", name, tvalue);
	strcat(xml, tmp);
}
