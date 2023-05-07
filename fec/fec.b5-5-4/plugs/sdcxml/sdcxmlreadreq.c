/*****************************************************************************

Filename:	lib/sdcxml/sdcxmlreadreq.c

Purpose:	SDC XML Message Set

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
 * $Header: /home/hbray/cvsroot/fec/lib/sdcxml/sdcxmlreadreq.c,v 1.3.4.7 2011/11/16 19:33:02 hbray Exp $
 *
 $Log: sdcxmlreadreq.c,v $
 Revision 1.3.4.7  2011/11/16 19:33:02  hbray
 Updated

 Revision 1.3.4.6  2011/10/27 18:34:00  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:50  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:47  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/25 18:19:45  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:53  hbray
 Added cvs headers

 *

2009.06.03 joseph dionne		Updated to support release 3.4
2009.05.29 harold bray		Created release 3.1.
*****************************************************************************/

#ident "@(#) $Id: sdcxmlreadreq.c,v 1.3.4.7 2011/11/16 19:33:02 hbray Exp $ "

// Application plugin data/method header declarations

#include <ctype.h>


#include "data.h"
#include "include/fullxml.h"



const NxTime_t MaxReadWait = 30LL * (1000LL * 1000LL);	// seconds of us

static const char *BatchHeader = "<ProtoBase_Transaction_Batch";
static const char *BatchTrailer = "</ProtoBase_Transaction_Batch>";
static const char *TransactionHeader = "<Transaction";
static const char *TransactionTrailer = "</Transaction>";



static char*
TransformData(appData_t *context, char *bfr, int *len)
{
	if ( stricmp(context->inputEntityEncoding, "url") == 0 )
		bfr = DecodeUrlCharacters(bfr, len);
	else if ( stricmp(context->inputEntityEncoding, "xml") == 0 )
		bfr = DecodeEntityCharacters(bfr, len);
	return bfr;
}


// Local scope method definition

static int
msgReady(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	PiPeekBfr_t peek;

	if (PiPosPeek(sess, &peek))
		return -1;

	int rlen = peek.len;
	char *bfr = TransformData(context, peek.bfr, &rlen);
	if (rlen <= 0)
	{
		SysLog(LogDebug, "No data");
		return -1;
	}

	char *bp;
	if (stristr(bfr, BatchHeader) != NULL)
	{
		if ((bp = stristr(bfr, BatchTrailer)) != NULL)
		{
			rlen = &bp[strlen(BatchTrailer)] - bfr;	// frame size of this len is ready
			return rlen;
		}

		return 0;				// I see the batch header; but, no trailer... keep waiting
	}

	if (stristr(bfr, TransactionHeader) != NULL && (bp = stristr(bfr, TransactionTrailer)) != NULL)
	{
		rlen = &bp[strlen(TransactionTrailer)] - bfr;	// frame size of this len is ready
		return rlen;
	}

	return 0;					// no frame is ready
}


static int
SendToHost(PiSession_t *sess, char *data, int len, NxUid_t batchId, HostSvcType_t svcType, boolean more)
{

	PiSessionVerify(sess);
	
	appData_t *context = (appData_t*)sess->pub.context->data;

	// Allocate memory for the request
	HostRequest_t *request = alloca(sizeof(HostRequest_t));

	if (!(request))
	{
		PiException(LogError, "alloca", eFailure, "failed");
		return -1;
	}
	memset(request, 0, sizeof(HostRequest_t));

	// send the host request
	request->hdr.svcType = svcType;
	request->hdr.more = more;
	request->len = len;
	memcpy(request->data, data, len);

// send the batchId as the requestEcho; when we receive the response, we'll know to which batch the response applies
	request->hdr.requestEcho = batchId;

	if (PiHostSend(sess, ProxyFlowSequential, sess->pub.replyTTL, request) != request->len)
	{
		PiException(LogError, "PiHostSend", eFailure, "failed");
		return -1;
	}

	++context->auth.inPkts;
	return 0;
}


static int
SendTransactions(PiSession_t *sess, char *xml)
{
	boolean more = true;
	HostSvcType_t svcType = eSvcAuth;
	NxUid_t batchId = PiBatchOpen(sess, 0);

	char isSettlement[1024];

	if (GetXmlTagValue(xml, "Settlement_Batch", isSettlement, sizeof(isSettlement), NULL) != NULL)
	{
		if (strcmp(isSettlement, "true") == 0)
			svcType = eSvcEdcMulti;
	}

	int maxLen = strlen(xml) * 2;
	char *transaction = alloca(maxLen);

	char *requestString = alloca(maxLen);

	strcpy(requestString, "");	// empty

// For each Transaction
	for (char *tptr = xml; (tptr = GetXmlTagValue(tptr, "Transaction", transaction, maxLen, NULL)) != NULL;)
	{
		char apiField[8192];

		// send previous request; if not first loop
		if (strlen(requestString) > 0)
		{
			if (SendToHost(sess, requestString, strlen(requestString), batchId, svcType, more) != 0)
			{
				PiException(LogError, "SendToHost", eFailure, "failed");
				return -1;
			}
		}

		SysLog(LogDebug, "Parsing transaction");
		strcpy(requestString, "");
		char *out = requestString;

		for (char *aptr = transaction; (aptr = GetXmlTagValue(aptr, "API_Field", apiField, sizeof(apiField), NULL)) != NULL;)
		{
			char fn[1024];
			char fv[1024];

			if (GetXmlTagValue(apiField, "Field_Number", fn, sizeof(fn), NULL) != NULL)
			{
				if (GetXmlTagValue(apiField, "Field_Value", fv, sizeof(fv), NULL) != NULL)
				{
					SysLog(LogDebug, "fn=%s, fv=%s", fn, fv);
					out = &out[sprintf(out, "%s,%s\n", fn, fv)];
				}
			}
		}
	}

	// if we have a pending request, send it...
	if (strlen(requestString) > 0)
	{
		if (SendToHost(sess, requestString, strlen(requestString), batchId, svcType, false) != 0)
		{
			PiException(LogError, "SendToHost", eFailure, "failed");
			return -1;
		}
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

		if (PiPosPeek(sess, &peek))		// repeek
			break;
	}

	return eWaitForData;
}


PiApiResult_t
ReadRequest(PiSession_t *sess)
{
	PiSessionVerify(sess);

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
	char *bfr = alloca(1024 + len);
	memset(bfr, 0, 8 + len);

	// Read message from the buffer
	int rlen = 0;

	if ( stricmp(context->inputEntityEncoding, "url") == 0 )
		rlen = PiPosUrlRecv(sess, bfr, len);
	else if ( stricmp(context->inputEntityEncoding, "xml") == 0 )
		rlen = PiPosEntityRecv(sess, bfr, len);
	else
		rlen = PiPosRecv(sess, bfr, len);

	if (rlen != len)
	{
		PiException(LogError, __FUNC__, eFailure, "Expected %d, received %d", len, rlen);
		return eFailure;
	}
	bfr[rlen] = '\0';			// terminate it


	// save http header (if any)

	if (context->postHeader != NULL)
		free(context->postHeader);	// release any previous saved header
	context->postHeader = NULL;

	if (strstr(bfr, "POST /") != NULL &&
		strstr(bfr, "HTTP/") != NULL)
	{
		char *tmp = alloca(8 + rlen);

		strcpy(tmp, bfr);		// make a tmp copy

		tmp = strstr(tmp, "HTTP/");
		if (tmp != NULL)
		{
			char *eol;

			while ((eol = strchr(tmp, '\r')) != NULL)
				*eol = '\0';
			while ((eol = strchr(tmp, '\n')) != NULL)
				*eol = '\0';
			context->postHeader = strdup(tmp);	// save the http version
		}
	}
	else
	{
		PiException(LogError, "HTTP header is missing", eFailure, "failed");
		return eDisconnect;
	}

	int rr = SendTransactions(sess, bfr);

	SysLog(LogDebug, "SendTransactions=%d", rr);
	if (rr != 0)
	{
		PiException(LogError, "SendTransactions", eFailure, "failed");
		return eFailure;
	}

	return EatLineTerminators(sess);
}
