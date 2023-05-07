/*****************************************************************************

Filename:	lib/api/apireadreq.c

Purpose:	ProtoBase API plugin

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The connection read method.  Called after the POS connection
			is read ready, i.e. data, or EOF, present at the STREAM head.

			Return true upon success, or false upon failure/error condition.

			NOTE: the number of bytes available on the STREAM head before
			entering into this method can be found in sess->bytesReady.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:54 $
 * $Header: /home/hbray/cvsroot/fec/lib/api/apireadreq.c,v 1.3.4.6 2011/10/27 18:33:54 hbray Exp $
 *
 $Log: apireadreq.c,v $
 Revision 1.3.4.6  2011/10/27 18:33:54  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:39  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:44  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/25 18:19:44  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:38  hbray
 Added cvs headers

 *

2009.08.01 harold bray          Ported to FEC version 4.x
2009.03.02 joseph dionne		Release 1.4 allow for partial packets from
								the Host(Stratus) which sends 576 bytes max.
								Strip out blank lines and comments as well.
2009.01.12 joseph dionne		Created release 1.2.  Supports authorizations
								only at this release.
*****************************************************************************/

#ident "@(#) $Id: apireadreq.c,v 1.3.4.6 2011/10/27 18:33:54 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

// Local scope method declaration
static PiApiResult_t PiApiSendSignOff(PiSession_t *sess);
static PiApiResult_t EatLineTerminators(PiSession_t *sess);
static int msgReady(PiSession_t *sess);


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
		return EatLineTerminators(sess);
	}

	// Allocate STACK buffer into which to read the msg
	char *buf = alloca(8 + len);
	memset(buf, 0, 8 + len);

	// Read message from the buffer
	int rlen = PiPosRecv(sess, buf, len);

	if (rlen != len)
	{
		PiException(LogError, __FUNC__, eFailure, "Expected %d, received %d", len, rlen);
		PiApiSendSignOff(sess);
		return eFailure;
	}
	buf[rlen] = '\0';			// terminate it


	boolean more = false;

// remove any \r chrs
	for(char *ptr; (ptr = strchr(buf, '\r')) != NULL; )
	{
		context->crlf = true;
		strcpy(ptr, ptr+1);
	}

// remove blank or comment lines
	for(char *ptr = buf; ptr != NULL && *ptr; )
	{
		while (*ptr && strchr("\n ", *ptr) != NULL)	// white...
			strcpy(ptr, ptr+1);

		if ( ! isdigit(*ptr) )		// does not start with a digit...
		{
			if ( *ptr == RS || *ptr == EOT )
			{
				context->seenEot = (*ptr == EOT);
				more = (*ptr == RS);
				*ptr = '\0';		// then remove final chr
				break;		// we're done
			}
			else if ( *ptr == '*' )	// comment
			{
				char *eol = strchr(ptr, '\n');
				if ( eol != NULL )
					strcpy(ptr, eol+1);	// skip over comment data
			}
			else		// this is unknown input; just eat to end of line.
			{
				if ( (ptr = strchr(ptr, '\n')) != NULL )
					++ptr;	// next line...
			}
		}
		else	// this is an api field, eat till eol...
		{
			if ( (ptr = strchr(ptr, '\n')) != NULL )
				++ptr;	// next line...
		}
	}

	len = strlen(buf);

	if (len <= 0)				// nothing to send
	{
		// If nothing to send
		// And all responses for all request have been received
		// And the EOT has been received
		// Send the terminal an EOT, indicating EOF
		if (context->auth.inPkts == context->auth.outPkts && context->seenEot)
		{
			SysLog(LogDebug, "Sending final EOT");
			if (PiApiSendSignOff(sess) != 0)
			{
				PiException(LogError, "PiApiSendSignOff", eFailure, "failed");
				return eFailure;
			}
			return context->persistent?eWaitForData:eDisconnect;
		}
		return eWaitForData;
	}

	HostRequest_t *request = (HostRequest_t *)alloca(1 + len + sizeof(HostRequest_t));

	if (!(request))
	{
		PiException(LogError, "alloca", eFailure, "failed");
		return eFailure;
	}							// if (!(request))
	memset(request, 0, len + sizeof(HostRequest_t));

	// build the request
	request->hdr.svcType = eSvcAuth;
	request->len = len;
	request->hdr.more = more;
	memcpy(request->data, buf, len);

	/*
		if t70proxy is enabled
			send 1,70 (TranCode == 70) to T70 host (requests settlement report)
		else
			send to Stratus
	*/

// its ok to mangle the input; its already been copied to the host buffer

	if ( context->t70proxy )	// trancode 70 logic is enabled...
	{
		char *t;
		if ( (t = strchr(buf, '\n')) != NULL )
			*t = '\0';		// terminate input at eol
		if ( (t = strchr(buf, ',')) != NULL )		// have a field separator
			*t++ = '\0';

		if ( atoi(buf) == 1 && t != NULL && atoi(t) == 70 )		// this is a trancode 70 message...
			request->hdr.svcType = eSvcT70;
	}

	// send request to host
	if (PiHostSend(sess, ProxyFlowPersistent, sess->pub.replyTTL, request) != request->len)
	{
		PiException(LogError, "PiHostSend", eFailure, "failed");
		return eFailure;
	}

	// Count this request
	++context->auth.inPkts;

	return EatLineTerminators(sess);
}


// Local scope method definition



static PiApiResult_t
PiApiSendSignOff(PiSession_t *sess)
{
	char signoff[2] = { EOT };

	PiSessionVerify(sess);

	if (PiPosSend(sess, signoff, 1) != 1)
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return -1;
	}

	return 0;
}


static PiApiResult_t
EatLineTerminators(PiSession_t *sess)
{

	PiSessionVerify(sess);

	PiPeekBfr_t peek;
	if (PiPosPeek(sess, &peek))
		return eWaitForData;

	while ( peek.len > 0 )
	{
		if ( peek.bfr[0] != '\r' && peek.bfr[0] != '\n' )
			break;

		char bfr[1];
		int rlen = PiPosRecv(sess, bfr, 1);
		if (rlen != 1)
		{
			PiException(LogError, __FUNC__, eFailure, "Expected %d, received %d", 1, rlen);
			return eFailure;
		}

		if (PiPosPeek(sess, &peek))
			break;
	}

	return eWaitForData;
}


static int
msgReady(PiSession_t *sess)
{

	PiSessionVerify(sess);

	// Preview the waiting buffer for a complete API
	PiPeekBfr_t peek;

	if (PiPosPeek(sess, &peek))
		return -1;

	if (peek.len > 0)
	{
		char *bp;

		// Find the first RS or EOT byte; packet termination
		if ( (bp = strchr(peek.bfr, RS)) != NULL ||
			(bp = strchr(peek.bfr, EOT)) != NULL )
		{
			int bytes = 1 + (bp - peek.bfr);
			return bytes;
		}
	}

	SysLog(LogDebug, "Did not find RS or EOT");
	return 0;		// no complete packet
}
