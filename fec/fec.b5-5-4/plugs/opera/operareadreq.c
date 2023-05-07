/*****************************************************************************

Filename:	lib/opera/operareadreq.c

Purpose:	Micros Opera HTTP/XML Message Set

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The connection read method.  Called after the POS connection
			is read ready, i.e. data, or EOF, present at the STREAM head.

			Return true upon success, or false upon failure/error condition.

			NOTE: the number of bytes available on the STREAM head before
			entering into this method can be found in sess->bytesReady.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)

YYYY.MM.DD --- developer ---	----------------- Comments -------------------
2011.10.08 joseph dionne		Add support for Empty XML Tags
2009.05.15 joseph dionne		Plugin is using NL, "\n", as the intra-
								XML field delimiter.  The new and improved
								Opera simulator is sending XML requests w/o
								NL in the XML message.
2009.04.03 joseph dionne		Extract and send only the XML field pairs,
								"Field name" and its associated data to the
								Host (Stratus).  The initial release sent
								the entire XML container, less the "xml 
								version" tag.
2009.03.25 joseph dionne		Created release 2.1.
*****************************************************************************/

// Application plugin data/method header declarations
#include "data.h"

// Local scope method declaration
static int msgReady(PiSession_t *sess);	// Insure valid msg has arrived
static int xmlParse(char *);	// Extracts XML content name/data

PiApiResult_t
ReadRequest(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Preview the waiting buffer for a complete msg
	int len = msgReady(sess);

	SysLog(LogDebug, "msgReady returned %d", len);
	if (len <= 0)
	{
		PiPeekBfr_t peek;

		if (PiPosPeek(sess, &peek))
			return eDisconnect;
		return eWaitForData;
	}

	// Allocate STACK buffer into which to read the msg
	char *buf = alloca(8 + len);

	memset(buf, 0, 8 + len);

	// Read message from the buffer
	int rlen = PiPosRecv(sess, buf, len);

	if (rlen != len)
	{
		PiException(LogError, __FUNC__, eFailure, "Expected %d, received %d", len, rlen);
		return eFailure;
	}
	buf[rlen] = '\0';			// terminate it


	// Test for Windoze style newline in HTTP tags
	char *bp;

	if ((bp = strchr(buf, '\n')))
	{
		if ('\r' == *--bp)
		{
			context->httpcrlf = true;
		}						// if ('\r' == *--bp)
	}							// if ((bp = strchr(buf,'\n')))

	// Find and store the inbound HTTP version
	*context->httpver = 0;
	bp = stristr(buf, "HTTP");
	if ((bp))
	{
		// Find the version string
		for (bp += 4; *(bp); bp++)
		{
			if (strchr(" \t/", *bp))
				continue;
			if (strchr("01234567890", *bp))
			{
				sscanf(bp, "%[^ \t\r\n]", context->httpver);
				break;
			}					// if (strchr("01234567890",*bp))
		}						// for(bp += 4;*(bp);bp++)
	}							// if ((bp))

	// Find and store the inbound XML version
	*context->xmlver = 0;
	bp = stristr(buf, "xml version");
	for (; (bp);)
	{
		char *ep = strchr(bp, '>');

		// Back up to find leading "<" brace
		for (bp--; '<' != *bp && bp > buf; bp--) ;

		// Test for buffer overrun
		if (sizeof(context->xmlver) < (ep - bp))
			break;

		// Store the XML version
		sprintf(context->xmlver, "%.*s", (int)(1 + ep - bp), bp);

		break;
	}							// for(;(bp);)

	// Advance to the XML request
	bp = stristr(buf, "Request ");
	if ((bp))
	{
		// Back up to find the leading "<" brace
		for (bp--; '<' != *bp && bp > buf; bp--)
		{
			if (!strchr(" \t", *bp))
			{
				*bp = 0;
				break;
			}					// if (!strchr(" \t",*bp))
		}						// for(bp--;'<' != *bp && bp > buf;bp--)
	}							// if ((bp))

	// Invalid or missing HTTP POST
	if ( (!(*context->httpver)) )
	{
		PiException(LogError, __FUNC__, eFailure, "Invalid or missing HTTP POST");
		return eFailure;
	}							// if (!(*data->httpver))
	if ( context->xmlHeaderRequired && (!(*context->xmlver)) )
	{
		PiException(LogError, __FUNC__, eFailure, "Invalid or missing XML document header");
		return eFailure;
	}							// if (!(*data->httpver))

	// Found the Opera XML request message, recalculate its length
	buf = bp;					// buf points at "<Request type=...>"
	len = xmlParse(buf);

	// An unsupported Opera XML Message received, drop connection
	if (0 >= len)
	{
		PiException(LogError, __FUNC__, eFailure, "An unsupported Opera XML Message received, drop connection");
		return eFailure;
	}

	// Send all but the Opera "Heartbeat" requests to the HGCS
	if (!stristr(buf, "heartbeat"))
	{
		// Allocate memory for the request
		HostRequest_t *request = (HostRequest_t *)alloca(1 + len + sizeof(HostRequest_t));

		if (!(request))
		{
			PiException(LogError, "alloca", eFailure, "failed");
			return eFailure;
		}						// if (!(request))
		memset(request, 0, len + sizeof(HostRequest_t));

		// send the request to host
		request->len = len;
		memcpy(request->data, buf, len);

		// Count this authorization packet
		// NOTE: Micros Opera "Settlement*" Request Type is to be treated
		// as an "*Authorization" Request Type per requirement of the
		// Stratus developer(s).
		if ((true) || !stristr(buf, "Settlement"))
		{
			++context->auth.inPkts;
			request->hdr.svcType = eSvcAuth;
		}
		else
		{
			++context->edc.inPkts;
			context->isEdc = true;
			request->hdr.svcType = eSvcEdc;
		}

		if (PiHostSend(sess, ProxyFlowPersistent, sess->pub.replyTTL, request) != request->len)
		{
			PiException(LogError, "PiHostSend", eFailure, "failed");
			return eFailure;
		}
	}
	else						// Respond to "Heartbeat" requests
	{
		char rsp[MaxSockPacketLen];

		memset(rsp, 0, sizeof(rsp));

		char *rp = rsp;

		// Advance past the "RequestType" line
		bp = strchr(buf, '\n');
		bp++;

		// Advance past the "RequestLength" line
		bp = strchr(bp, '\n');
		bp++;

		// Begin the XML Response message
		rp += sprintf(rp, "ResponseType,Heartbeat\n");

		// Append the full XML Request message
		rp += sprintf(rp, "%s\n", bp);

		// Add the "Status" XML field
		rp += sprintf(rp, "Status,ok\n");

		// Return the Heartbeat response
		if (PiOperaSendData(sess, rsp, strlen(rsp)) != eOk)
		{
			PiException(LogError, "PiOperaSendData", eFailure, "failed");
			return eFailure;
		}
	}							// if (!stristr(buf,"heartbeat"))

	return eWaitForData;
}								// int readRequest(fecconn_t *sess)

// Local scope method definition

static int
msgReady(PiSession_t *sess)
{
	int bytes = -1;


	PiSessionVerify(sess);

	PiPeekBfr_t peek;

	if (PiPosPeek(sess, &peek))
		return bytes;

	if (stristr(peek.bfr, "POST") == NULL)
	{
		SysLog(LogDebug, "Waiting to see POST");
		return 0;
	}

	{
		int slen = (peek.bfr - stristr(peek.bfr, "POST"));
		if ( slen > 0 )
		{
			char *tmp = alloca(slen + 10);

			if (PiPosRecv(sess, tmp, slen) != slen)
			{
				PiException(LogError, "PiPosRecv", eFailure, "failed");
				return -1;
			}
		}

		// now, re-peek
		if (PiPosPeek(sess, &peek))
			return bytes;
	}

	// Preview the waiting buffer for a complete XML Message
	// NOTE: the 200 byte test for bytes ready is based on the length of
	// the sample "Heartbeat" request in the Micros Opera specification,
	// lib/opera/data.h has more information regarding the document
	char *buf = peek.bfr;
	int bufsize = peek.len;

	// Peek at the message type to get the packet length
	for (;;)
	{
		char *bp;

		// Find the end of the XML request
		bp = stristr(buf, "</Request>");
		if ((bp))
			bytes = bp - buf + sizeof("</Request>") - 1;
		else
		{
			bytes = 0;
			break;
		}						// if ((bp))

		// Check for newline after packet
		if (bp != NULL && (1 <= (bufsize - bytes)))
		{
			// Is there a UNIX newline after the packet?
			// NOTE: the Windoze CR preceeds the UNIX LF newline
			// and will be stripped when request is actually read
			if ((bp = strrchr(buf, '\n')))
				bytes++;
			if ('\r' == *--bp)
				bytes++;
		}						// if (1 <= (len - bytes))

		// Partial (HTTP POST) request received, wait for data
		if (bytes < bufsize)
			bytes = 0;

		break;
	}

	return (bytes);
}								// static int msgReady(int peer)


static int
xmlParse(char *src)
{
	int len = strlen(src);
	char *buf = alloca(8 + len);
	char *bp = buf;
	char *sp = src;
	char null = 0;
	int ii;

	// Copy src into buf, appending a final '\n' to the stream data
	sprintf(buf, "%s\n", src);

	// Clear the src buffer
	memset(src, 0, len);

	// Parse off the "Request Type" token
	if ((bp = stristr(buf, "<Request type")))
	{
		char msgType[128];

		memset(msgType, 0, sizeof(msgType));

		ii = sscanf(bp, "%*[\t <requstypREQUSTYP]%*[=\"]%[^\t =\"]", msgType);

		sp += sprintf(sp, "RequestType,%s\n", msgType);
		sp += sprintf(sp, "RequestLength,%d\n", len);

		// Invalid "Request type"
	}
	else
	{
		return (0);
	}							// if ((bp = stristr(buf,"Request type")))

	// Find the first "Field name"
	if (!(bp = stristr(bp, "<Field name")))
		bp = &null;

	// Parse out the XML field name/data pair(s)
	for (; *bp;)
	{
		char line[2 * 1024];

		memset(line, 0, sizeof(line));

		char name[128];

		memset(name, 0, sizeof(name));

		char data[2 * 1024];

		memset(data, 0, sizeof(data));

		char ch = 0;
		char *tp;
		char *ttp;

        // Parse off a "line" of XML text up to and including the LF('\n')
        // NOTE: a "line" is "<Token> ... </Token>" or "<Token .../>"
        // with an optional '\n'
        tp = stristr(bp, "/>");				// look for Empty Tag first
        ttp=stristr(bp, "</");				// find position of Closing Tag
        if ((tp > ttp) || !(tp)) tp = ttp;	// Closing Tag before Empty Tag
		if ((tp))
		{
			// Find the terminating ">" byte
			char *cp = strchr(tp, '>');

			if ((cp))
				tp = cp + 1;
			else
				tp += strlen(tp);

			// Skip past the optional LF ('\n') byte(s)
			for (ch = *tp; '\n' == ch;)
				ch = *++tp;

			// Terminate the buffer and extract the XML "line"
			*tp = 0;
			sprintf(line, "%s", bp);

			// Put back the saved byte, which should be '>'
			*tp = ch;

			// Advance the XML buffer pointer to the "next XML line"
			bp = tp;
			// Missing end token, we're done now
		}
		else
		{
			break;
		}						// if ((lp = stristr(xp,"</")))

		// Blank line?
		if (!strlen(line))
			continue;

		// End-of-Request XML token
		if (stristr(line, "/Request"))
			break;

		*name = 0;
		*data = 0;
		if (!parseXmlField(line, name, data, 1))
		{
			sp += sprintf(sp, "%s,%s\n", name, data);

			// Invalid XML "Field type" entry
		}
		else
		{
			return (-1);
		}						// if (!parseXmlField(line,name,data,1))

	}							// for(;*bp;)

	return (sp - src);
}								// static int xmlParse(char *src)
