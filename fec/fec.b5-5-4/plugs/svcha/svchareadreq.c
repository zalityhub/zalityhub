/*****************************************************************************

Filename:	lib/svcha/svchareadreq.c

Purpose:	MICROS SVCHA XML Message Set

			Compliance with specification: MICROS Standard SVC Interface
			Revision 2.5 last updated on 4/05/2005

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The connection read method.  Called after the POS connection
			is read ready, i.e. data, or EOF, present at the STREAM head.

			Return true upon success, or false upon failure/error condition.

			NOTE: the number of bytes available on the STREAM head before
			entering into this method can be found in sess->bytesReady.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:34:00 $
 * $Header: /home/hbray/cvsroot/fec/lib/svcha/svchareadreq.c,v 1.3.4.6 2011/10/27 18:34:00 hbray Exp $
 *
 $Log: svchareadreq.c,v $
 Revision 1.3.4.6  2011/10/27 18:34:00  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:51  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:47  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/25 18:19:45  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:54  hbray
 Added cvs headers

 *

2009.10.30 joseph dionne		Created at release 5.9
*****************************************************************************/

#ident "@(#) $Id: svchareadreq.c,v 1.3.4.6 2011/10/27 18:34:00 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

// Local scope method declaration
static int msgReady(PiSession_t *);
static int callBack(char *, int, svchaXml_t *);

PiApiResult_t
ReadRequest(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;
	
	int reqLen = sizeof(HostRequest_t);
	HostRequest_t *req = (HostRequest_t *)alloca(1 + reqLen);
	int len = -1;
	svchaXml_t *xmlData = (context) ? &context->xmlData : 0;
	NameToValueMap *mp = 0;
	char *buf;
	char *bp;

	// Initialize stack
	memset(req, 0, 1 + reqLen);
	reqLen = sizeof(req->data);

	// Preview the waiting buffer for a complete reqeust
	if (0 >= (len = msgReady(sess)))
		return (0 > len ? eDisconnect : eWaitForData);

	// Allocate STACK buffer into which to read the API
	bp = (buf = alloca(1 + len));
	memset(buf, 0, 1 + len);

	// "Receive" the POS packet
	if (len != PiPosRecv(sess, bp, len))
	{
		PiException(LogError, "PiPosRecv", eFailure, "failed");
		return (eFailure);
	}							// if (len != PiPosRecv(sess,bp,len))

	// Begin at the start-of-packet, SOH character
	if (!(bp = strchr(bp, SOH)))
	{
		PiException(LogError, __FUNC__, eFailure, "Invalid MICROS SVCHA packet, no SOH");
		return (eFailure);
	}							// if (!(bp = strchr(bp,SOH)))

	// Received a "LOAD_CFG" request, just send it back to the POS
	// NOTE: this response is an "empty configuration" response
	if (stristr(bp, "LOAD_CFG"))
	{
		len = strlen(bp);

		if (len != PiPosSend(sess, bp, len))
		{
			PiException(LogError, "PiPosRecv", eFailure, "failed");
			return (eFailure);
		}						// if (len != PiPosSend(sess,bp,len))
		return (eOk);
	}							// if (stristr(bp,"LOAD_CFG"))

	// Allocate HEAP memory for the request parse buffers
	// NOTE: HEAP memory allocated in one contiguous block and
	// then sub-divided into three pointers, save, extra, req
	xmlData->len = 1 + (strchr(bp, ETX) - bp);
	xmlData->save = malloc(6 * len);
	memset(xmlData->save, 0, (6 * len));
	xmlData->extra = (char *)xmlData->save + (3 * len);
	xmlData->req = xmlData->extra + (5 * len);
	xmlData->map = SvchaTagToValue;

	// Initialize the extra XML tag buffer
	sprintf(xmlData->extra, "99,");

	// Parse the MICROS SVCHA packet and XML Document
	if (SOH == *bp)
	{
		char buf[1024] = { 0 };
		char fmt[16] = { 0 };
		int ofs = 0;
		char *tp = bp + 1;
		char *rp = xmlData->req;
		char *ep = xmlData->extra;

		// Scan off the value for "Originator", translated to "01"
		sprintf(fmt, "%%[^%c]%%n", FS);
		ofs = 0;
		sscanf(tp, fmt, buf, &ofs);
		tp += 1 + ofs;

		// Save "Originator" for POS response
		sprintf(xmlData->origin, "%.*s", sizeof(xmlData->origin) - 1, buf);

		// Find the "Originator" name-to-value map entry
		if (0 < ofs)
		{
			mp = svchaFindMapByName("Originator", xmlData->map);

			// Add mapped token to request 
			if ((mp) && (mp->name) && 0 > mp->group)
			{
				rp += sprintf(rp, "%02d,%s\n", mp->value, buf);

				// Add token to "extra" tokens entry, i.e. "99,..."
			}
			else
			{
				ep += sprintf(ep, "%s%c%s\r", "Originator", FS, buf);
			}					// if ((mp) && (mp->pub.name) && 0 < mp->group)
		}						// if (0 < ofs)

		// Scan off the value for "Target", translated to "02"
		sprintf(fmt, "%%[^%c]%%n", STX);
		ofs = 0;
		sscanf(tp, fmt, buf, &ofs);

		// Save "Target" for POS response
		sprintf(xmlData->target, "%.*s", sizeof(xmlData->target) - 1, buf);

		// Find the "Target" name-to-value map entry
		if (0 < ofs)
		{
			mp = svchaFindMapByName("Target", xmlData->map);

			// Add mapped token to request 
			if ((mp) && (mp->name) && 0 > mp->group)
			{
				rp += sprintf(rp, "%02d,%s\n", mp->value, buf);

				// Add token to "extra" tokens entry, i.e. "99,..."
			}
			else
			{
				ep += sprintf(ep, "%s%c%s\r", "Target", FS, buf);
			}					// if ((mp) && (mp->pub.name) && 0 > mp->group)
		}						// if (0 < ofs)

		// Advance to the MICROS SVCHA request XML Document
		bp = strchr(tp, '<');

		// Calculate the length of the XML Document
		ep = strchr(bp, ETX);
		xmlData->len = ep - bp;
		bp[xmlData->len] = 0;
	}							// if (SOH == *bp)

	// Save the XML Document for use in the response
	if ((bp = stristr(bp, "<?xml")))
	{
		// Convert XML Document "string" to an array of tag pointers
		// NOTE: data pointed to follows the array of pointers
		// make sure that enough memory is allocated for both
		xmlToArray(bp, xmlData->save);

		// Not a valid MICROS SVC XML request?
	}
	else
	{
		PiException(LogError, __FUNC__, eFailure, "Could not find start of XML Document");
		return (eFailure);
	}							// if ((bp = stristr(bp,"<?xml")))

	// Parse the MICROS SVCHA request
	if ((bp = stristr(bp, "<SVCMessage")))
	{
		svchaParse(bp, xmlData->len, callBack, xmlData);

	}
	else
	{
		PiException(LogError, __FUNC__, eFailure, "Could not find start of SVC request");
		return (eFailure);
	}							// if ((bp = stristr(bp,"<SVCMessage")))

	// Append the mapped XML fields with any "extra" XML tag(s)
	req->len = sprintf(req->data, "%s%s\n", xmlData->req, 4 < strlen(xmlData->extra) ? xmlData->extra : "");

	// Log any unknown (extra) XML tags
	if (4 < strlen(xmlData->extra))
	{
		SysLog(LogDebug, "Extra XML tag(s) /%s/", xmlData->extra);
	}							// if (4 < strlen(xmlData->extra))

	// Send request to switch
	req->hdr.svcType = eSvcAuth;
	req->hdr.more = false;
	if (req->len != PiHostSend(sess, ProxyFlowSequential, sess->pub.replyTTL, req))
	{
		PiException(LogError, "PiHostSend", eFailure, "failed");
		return (eFailure);
	}

	// Count this MICROS SVCHA authorization request
	++context->auth.inPkts;

	return (eOk);
}								// PiApiResult_t ReadRequest(PiSession_t *sess)

// Local scope method definition
static int
msgReady(PiSession_t *sess)
{
	PiPeekBfr_t peek = { 0 };
	int ret;
	char *bp;
	char *ep;

	PiSessionVerify(sess);

	if ((ret = PiPosPeek(sess, &peek)))
	{
		SysLog(LogDebug, "PiPosPeek returned %d (%p,%d)", ret, peek.bfr, (peek.bfr) ? peek.len : -1);
		return (-1);
	}							// if ((ret = PiPosPeek(sess,&peek)))

	// Find the start-of-packet (SOH) of the MICROS SVCHA packet
	if (!strchr(peek.bfr, SOH))
	{
		return (0);

		// Find the end-of-packet
	}
	else
	{
		bp = peek.bfr;

		if (!(ep = strchr(bp, EOT)))
			return (0);
	}							// if (!strchr(peek.bfr,SOH))

	// Verify this is a MICROS SVC Message
	if (!(stristr(bp, "<SVCMessage")))
		return (-1);

	return (1 + (ep - bp));
}								// static int msgReady(PiSession_t *)

static int
callBack(char *tag, int len, svchaXml_t *data)
{
	static char *lastToken = 0;
	static int lastTokenId = -1;
	static boolean extraToken = false;
	enum
	{
		eNoToken,
		eOpenToken,
		eAttribute,
		eCloseToken
	} state = 0;
	int retLen = 0;
	char *token = alloca(1 + len);
	char *rp = data->req + strlen(data->req);
	char *ep = data->extra + strlen(data->extra);
	NameToValueMap *mp = 0;

	// Initialize working variables
	memset(token, 0, 1 + len);

	// Decode the quality of this token 
	if ('/' == *tag)
		state = eCloseToken;
	else if ('/' == *(tag + 1))
		state = eCloseToken;
	else if ('?' == *tag)
		state = eCloseToken;
	else if ('>' == *tag)
		state = eCloseToken;
	else if ('<' == *tag)
		state = eOpenToken;
	else
		state = eAttribute;

	if (eAttribute != state)
		sscanf(tag + ('?' == *(1 + tag) ? 2 : 1), "%[^ \t\r\n/?<>\002]", token);
	else
		sscanf(tag, "%[^ \t\r\n/?<>\002]", token);

	switch (state)
	{
	default:
		break;

		// Parse open token, translate tag value
	case eOpenToken:
		{
			// Save the "last" XML token
			lastToken = malloc(1 + strlen(token));
			sprintf(lastToken, "%s", token);

			// Find the token map entry
			if ((mp = svchaFindMapByName(lastToken, data->map)))
			{
				lastTokenId = (mp->value) ? mp->value : -1;
				extraToken = !(mp->name) || 0 <= mp->group;
			}
			else
			{
				lastTokenId = -1;
				extraToken = true;
			}					// if ((mp = svchaFindMapByName(lastToken,data->map)))

			// Add mapped token to request 
			if (!(extraToken))
			{
				rp += sprintf(rp, "%02d,", mp->value);

				// Add token to "extra" tokens buffer, i.e. "99,..."
			}
			else
			{
				if (0 > lastTokenId)
					ep += sprintf(ep, "%s", token);

			}					// if (!(extraToken))
		}
		break;

		// Append attribute(s) to extra tags buffer
	case eAttribute:
		{
			// Something ain't right, should not be here without lastToken
			if (!(lastToken) && 0 > lastTokenId)
			{
				// JJD TODO: print error message
				break;
			}					// if (!(lastToken) && 0 > lastTokenId)

			// Only send non mapped MICROS SVCHA XML tags to
			// the Switch, mapped tags not used are not sent
			if (0 > lastTokenId)
			{
				// If this is not the first attribute of a request
				// XML field just append the attribute to extra
				if ('\r' != *(ep - 1))
				{
					ep += sprintf(ep, " %s", token);

					// Add the translated token id and attribute to extra
				}
				else
				{
					ep += sprintf(ep, "%s, %s", lastToken, token);
				}				// if ('\r' == *(ep-1))
			}					// if (0 > lastTokenId)
		}
		break;

		// Done with the "last" token
	case eCloseToken:
		{
			// Return the length of the XML tag data
			retLen = strlen(token);

			// Ignore the XML end-tags, i.e. </TAG>
			if (0 != retLen && '/' != tag[1] && '?' != tag[1])
			{
				int len = -1;
				char *token = "";

				// Parse the XML tag value
				sscanf(tag + 1, "%*[^<]%n", &len);
				if (0 <= len)
				{
					retLen = len;

					if ((len))
					{
						token = alloca(1 + len);
						sscanf(tag + 1, "%[^<]", token);
					}			// if ((len))
				}				// if (0 < len)

				// Append the XML tag value
				if (0 < retLen)
				{
					// Add value of the mapped token to auth request
					if (!(extraToken))
					{
						rp += sprintf(rp, "%s\n", token);

						// Append the data value in the "extra"
						// buffer and terminate the XML tag
					}
					else if (0 > lastTokenId)
					{
						ep += sprintf(ep, "%c%s\r", FS, token);
					}			// if (!(extraToken))
				}				// if (0 < retLen)
			}					// if (0 != retLen && '/' != tag[1] && '?' != tag[1])

			// If added attribute(s) for request XML field(s),
			// then terminate the extra XML field buffer
			if (!strchr("\r,", *(ep - 1)))
				*ep++ = '\r';

			// Default to 'Y' any presence-only XML tags in request
			if ((lastToken))
			{
				if (',' == *(rp - 1))
					rp += sprintf(rp, "Y\n");

				// Release the lastToken memory 
				free(lastToken);
			}					// if ((lastToken))

			// Insure translated request XML field(s) are terminated
			if ('\n' != *(rp - 1))
				*rp++ = '\n';

			lastToken = 0;
			lastTokenId = -1;
			extraToken = false;
		}
		break;
	}							// switch(state)

	return (retLen);
}								// static int callBack(char *tag,int len,svchaXml_t *data)
