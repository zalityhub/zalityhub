/*****************************************************************************

Filename:	lib/onq/onqreadreq.c

Purpose:	GCS Hilton OnQ Message Set

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The connection read method.  Called after the POS connection
			is read ready, i.e. data, or EOF, present at the STREAM head.

			Return true upon success, or false upon failure/error condition.

			NOTE: the number of bytes available on the STREAM head before
			entering into this method can be found in sess->bytesReady.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/11/16 19:33:02 $
 * $Header: /home/hbray/cvsroot/fec/lib/onq/onqreadreq.c,v 1.3.4.7 2011/11/16 19:33:02 hbray Exp $
 *
 $Log: onqreadreq.c,v $
 Revision 1.3.4.7  2011/11/16 19:33:02  hbray
 Updated

 Revision 1.3.4.6  2011/10/27 18:33:59  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:48  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:47  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/25 18:19:45  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:20  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:49  hbray
 Added cvs headers

 *

2009.03.25 joseph dionne		Release 1.9.  Supports OnQ Heartbeat requests
2009.01.12 joseph dionne		Created release 1.2.  Supports authorizations
								only at this release.
*****************************************************************************/

#ident "@(#) $Id: onqreadreq.c,v 1.3.4.7 2011/11/16 19:33:02 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"


#define IsEdc(ctx, req) ((ctx)->packets[idx].type == 2 || (ctx)->packets[idx].type == 3 || ( (ctx)->packets[idx].type == 1 && ((req)->data[17] == 'B' || (req)->data[17] == 'C')))?true:false


// Local scope method declaration
static int msgReady(PiSession_t *sess, int *idx);

PiApiResult_t
ReadRequest(PiSession_t *sess)
{
	// Worker's "file scope" application context stored in sess->pub.appData
	int idx = -1;

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Preview the waiting buffer for a complete msg
	int len = msgReady(sess, &idx);

	SysLog(LogDebug, "msgReady returned %d", len);
	if (len <= 0)
	{
		PiPeekBfr_t peek;

		if (PiPosPeek(sess, &peek))
			return eDisconnect;
		return (len < 0) ? eDisconnect : eWaitForData;
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

	// Test for Windoze style newline, adjust len as needed
	char *bp;

	if ((bp = strchr(buf, '\n')))
	{
		context->newline = true;
		len--;
		if ('\r' == *--bp)
		{
			context->crlf = true;
			len--;
		}
	}

	// Send all but the OnQ "Who Am I" messages on to the Host(Stratus)
	// NOTE: exclude OnQ "Heartbeat" messages, message type "00"

	if (context->packets[idx].type > 0 && context->packets[idx].type < 5)	// for the host?
	{
		// Allocate memory for the request
		HostRequest_t *request = (HostRequest_t *)alloca(1 + len + sizeof(HostRequest_t));

		if (!(request))
		{
			PiException(LogError, "alloca", eFailure, "failed");
			return (eFailure);
		}						// if (!(request))
		memset(request, 0, len + sizeof(HostRequest_t));

		// Return the request
		request->len = len;
		memcpy(request->data, buf, len);

		// set as an auth or edc
		if ( IsEdc(context, request) )
		{
			++context->edc.inPkts;
			request->hdr.svcType = eSvcEdc;
			request->hdr.more = false;
		}
		else
		{
			++context->auth.inPkts;
			request->hdr.svcType = eSvcAuth;
			request->hdr.more = false;
		}

		if (PiHostSend(sess, ProxyFlowSequential, sess->pub.replyTTL, request) != request->len)
		{
			PiException(LogError, "PiHostSend", eFailure, "failed");
			return eFailure;
		}
	}
	else if ( context->packets[idx].type == 0 || context->packets[idx].type == 5 )
	{
		char *response = alloca(128 + len);

		memset(response, 0, 128 + len);
		memcpy(response, buf, len);

		if ( context->packets[idx].type == 5 )
		{
			if (sess->pub.service.properties.additional)
			{
				// Handle the OnQ "Who Am I" message
				// the OnQ "Heartbeat" message
				char ip[16];
				char port[16];

				// OnQ "additional" data is an "IPAddress:Port" string
				// which will be sent back to the Hilton OnQ POS system
				if ((bp = strrchr(sess->pub.service.properties.additional, ':')))
				{
					// Copy and format the OnQ "Port Number" field
					sprintf(port, "%-9.9s", bp + 1);

					// Copy and format the OnQ "Ip Address" field
					*bp = 0;
					sprintf(ip, "%-15.15s", sess->pub.service.properties.additional);
					*bp = ':';

					// Move the "IP Address" and "Port Number" fields into the
					// OnQ "Who Am I" request message
					memcpy(response + 7, ip, strlen(ip));
					memcpy(response + 22, port, strlen(port));
				}				// if ((bp = strrchr(sess->pub.service.properties.additional,':')))
			
				response[0] = '1';	// indicate a response
			}
			else
			{
				SysLog(LogWarn, "Received a WhoAmI; nothing to say (check the 'additional' field for service %d", sess->pub.service.properties.serviceNumber);
			}
		}

// this returns a ping or WhoAmI request
		if (context->crlf)
			strcat(response, "\r");
		if (context->newline)
			strcat(response, "\n");

		len = strlen(response);
		if (PiPosSend(sess, response, len) != len)
		{
			PiException(LogError, "PiPosSend", eFailure, "failed");
			return eFailure;
		}
	}
	else
	{
		PiException(LogError, __FUNC__, eFailure, "Packet type %d is invalid", context->packets[idx].type);
		return eFailure;
	}

	return (eOk);
}



// Local scope method definition

static int
msgReady(PiSession_t *sess, int *idx)
{

	PiPeekBfr_t peek;

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	if (PiPosPeek(sess, &peek))
		return -1;

	int len;
	int bytesready = 0;

	if ((len = peek.len) >= 2) // Preview the waiting buffer for a complete message
	{
		char msgType[4];

		memset(msgType, 0, sizeof(msgType));
		memcpy(msgType, peek.bfr, 2);

		// Peek at the message type to get the packet length
		{
			int type = atoi(msgType);
			int ii;

			// Is this a supported message type?
			for (ii = 0; ii < MAXONQ; ii++)
				if (type == context->packets[ii].type)
					break;

			// Message type not allowed
			if (ii > MAXONQ)
				return (-1);

			// Return the index into the allowed OnQ message array
			*idx = ii;

			// See if the entire packet has arrived
			// NOTE: although the specification indicates fixed packet
			// lengths this code will terminate the message at Windoze
			// newline, i.e. CR/LF pair.
			if (len >= context->packets[ii].length)
				bytesready = context->packets[ii].length;

			// Check for newline after packet
			if (2 <= (len - bytesready))
			{
				char *bp;

				// Is there a UNIX newline after the packet?
				// NOTE: the Windoze CR preceeds the UNIX LF newline
				// and will be stripped when request is actually read
				if ((bp = strchr(&peek.bfr[bytesready], '\n')))
					bytesready = bp - peek.bfr + 1;
			}					// if (2 <= (len - bytesready))
		}
	}							// if (2 < len)

	return (bytesready);
}
