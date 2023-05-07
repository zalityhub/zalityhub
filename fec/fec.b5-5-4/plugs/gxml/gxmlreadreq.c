/*****************************************************************************

Filename:	lib/gxml/gxmlreadreq.c

Purpose:	GCS HTTP/XML Message Set

			Compliance with specification: "GCS XML Specification"
			Version 4.1.1 last updated on 8/16/2005, by Theron Crissey

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The connection read method.  Called after the POS connection
			is read ready, i.e. data, or EOF, present at the STREAM head.

			Return true upon success, or false upon failure/error condition.

			NOTE: the number of bytes available on the STREAM head before
			entering into this method can be found in sess->bytesReady.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:55 $
 * $Header: /home/hbray/cvsroot/fec/lib/gxml/gxmlreadreq.c,v 1.3.4.7 2011/10/27 18:33:55 hbray Exp $
 *
 $Log: gxmlreadreq.c,v $
 Revision 1.3.4.7  2011/10/27 18:33:55  hbray
 Revision 5.5

 Revision 1.3.4.6  2011/09/24 17:49:41  hbray
 Revision 5.5

 Revision 1.3.4.4  2011/09/01 14:49:44  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/25 18:19:44  hbray
 *** empty log message ***

 Revision 1.3.4.1  2011/08/17 17:58:57  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:15  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:42  hbray
 Added cvs headers

 *

2010.06.18 joseph dionne		Allow xmlns and other attributes between the
								XML 'Request' Tag and the "type" attribute
								and XML 'Field' Tag and the "name" attribute
2009.09.22 joseph dionne		Ported to release v4.3
2009.08.04 joseph dionne		Created.
*****************************************************************************/

#ident "@(#) $Id: gxmlreadreq.c,v 1.3.4.7 2011/10/27 18:33:55 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"

// FEC internal event handler control methods
int PiSessionReadDisable(PiSession_t *sess);
int PiSessionReadEnable(PiSession_t *sess);

// Local scope method declaration
static int			msgReady(PiSession_t *);
static int			authorizationRequest(PiSession_t *, char *, char *, int);
static int			depositRequest(PiSession_t *, char *, char *, int);
//static long long	getMessageId(char *);

PiApiResult_t
ReadRequest(PiSession_t *sess)
{
	// Worker's "file scope" application data stored in sess->pub.appData
	appData_t *ctx = (appData_t*)sess->pub.context->data;
	int reqLen = sizeof(HostRequest_t);
	HostRequest_t *req = (HostRequest_t *)alloca(1 + reqLen);
	int len = -1;
	char fmt[256] = { 0 };
	char *buf;
	char *bp;

	PiSessionVerify(sess);

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
	if (len != PiPosRecv(sess, bp, len)) {
		PiException(LogError, "PiPosRecv", eFailure, "failed");
		return (GxmlErrorResponse(sess, eHttpInternalError));
	} // if (len != PiPosRecv(sess,bp,len))

	// Find and store the inbound HTTP version string
	// NOTE: msgReady() validated socket data is a valid HTTP POST
	// whose Content-Type is XML
	// NOTE: DepositRequests require several reads to complete,
	// but the HTTP protocol will only be found on first read
	if (!(*ctx->httpver)) {
		int verLen = 0;

		sscanf(stristr(buf, "HTTP"), "%[htpHTP/.0123456789]%n", ctx->httpver,
			&verLen);
		if (5 >= len) {
			PiException(LogError, __FUNC__, eFailure,
				"HTTP version is missing");
			return (GxmlErrorResponse(sess, eHttpNoContent));
		} // if (5 >= len)

		// Advance past the HTTP header
		// NOTE: after this loop, bp should be pointing the XML
		// Document version string.
		for (bp = buf;;) {
			int len = 0;

			// Scan past the HTTP protocol bytes
			// NOTE: only looking for the length, no data stored
			if (0 <= sscanf(bp, "%*[^\r\n]%n", &len)) {
				bp += len;
				len = 0;

				// Scan off the newline
				if (0 <= sscanf(bp, "%*[\r\n]%n", &len)) {
					bp += len;

					// <CR><LF><CR><LF> marks the end of the HTTP protocol
					if (4 == len) break;
				} // if (0 <= sscanf(bp, "%*[\r\n]%n", &len))

			// Done scanning the HTTP tokens
			} else {
				break;
			} // if (0 <= sscanf(bp,"%*[^\r\n]%n",&len))
		} // for(bp=buf;;)

		// Add the number of HTTP protocol bytes to the request length
		// NOTE: ctx->reqLength initialized in msgReady() method
		ctx->reqLength += bp - buf;

		// Just received the HTTP POST header wait for more data
		if (!(*bp)) return (eWaitForData);
	} // if (!(*ctx->httpver))

	// Find and store the inbound XML version
	// NOTE: DepositRequest requires several reads to complete,
	// but the XML version string will only be found on first read
	if (!(*ctx->xmlver)) {
		sprintf(fmt, "%%*[%s]%%n", ctx->xml[eXmlTag]);
		len = 0;
		sscanf(bp, fmt, &len);

		// XML Version string must start with "<?xml" and must also
		// contain "version" token, among other tokens, i.e. UTF, etc.
		if (0 < len && stristr(bp, " version")) {
			char temp[sizeof(ctx->xmlver)] = {0};

			// Save the XML version tag value
			sprintf(fmt,"%%*[ versionVERSION=]%%%d[^ ?>]",(int)(sizeof(temp) - 1));
			sscanf(stristr(bp," version"),fmt,temp);

			// Skip double quotes
			if ('"' == *temp)
				sscanf(temp,"\"%[^\"]",ctx->xmlver);
			else
				sprintf(ctx->xmlver,"%s",temp);

			// Find the end of the XML version tag
			bp = stristr(bp,"?>");

			// Remove any whitespace after end of the XML version tag
			if ((bp)) {
				len = 0; sscanf((bp += 2),"%*[ \t\r\n]%n",&len);
				bp += len;
			} // if ((bp))

		// Invalid XML Document tag, missing version string
		} else {
			PiException(LogError, __FUNC__, eFailure,
				"Invalid XML Document tag");
			return (eFailure);
		} // if (0 < len && stristr(bp,"version"))

		// Just received the XML Document version tag, wait for more data
		if (!(*bp)) return (eWaitForData);
	} // if (!(*ctx->xmlver))

	// Extract the GCS Web XML request type
	if (!(*ctx->reqType)) {
		char *rp = ctx->reqType;

		sprintf(fmt, "%%*[%s]%%n", ctx->xml[eRequest]);
		len = 0; sscanf(bp, fmt, &len);
		if (0 < len) {
			// Initialize the HGCS "RequestType" tag
			rp += sprintf(rp, "RequestType,");

			// Find the type of request
			if ((bp=stristr(bp," type="))) {
				bp += 6;
			} else {
				PiException(LogError, __FUNC__, eFailure,
					"Invalid XML Request type");
				return (eFailure);
			} // if ((bp=stristr(bp," type=")))

			// Advance past the leading double quote (")
			for (; (*bp);) if ('"' == *bp++) break;

			// Scan off the request type
			len = 0;
			sscanf(bp, "%[^\">]%n", rp, &len);
			rp[len] = 0;
			bp += len;

			// Advance past the trailing bracket (>)
			for (; (*bp);) if ('>' == *bp++) break;

			// Advance past whitespace between XML tokens
			len = 0; sscanf(bp, "%*[ \t\r\n]%n", &len); bp += len;
		} // if (0 < len)
	} // if (!(*ctx->reqType))

	// Parse a Heartbeat or other GCS Web XML authorization request
	if (!stristr(ctx->reqType, "depositrequest")) {
		switch ((len = authorizationRequest(sess, bp, req->data, reqLen))) {
		case -1:
			PiException(LogError, __FUNC__, eFailure, "Invalid request, %d %s",
				errno, strerror(errno));
			return (GxmlErrorResponse(sess, eHttpInternalError));

		case 0:	// GCS XML Heartbeat response returned, send to POS
			req->len		= strlen(req->data);
			req->hdr.svcType	= eSvcAuth;
			req->hdr.more		= false;
			return (GxmlSendPos(sess, req));

		default:				// Request received
			req->len = len;
			req->hdr.svcType = eSvcAuth;
			req->hdr.more = false;
			break;
		} // switch((len = authorizationRequest(sess,bp,req->data,reqLen)))

	// Special handling for DepositRequest uploads
	// NOTE: Context for DepositRequest will be done after response sent
	} else {
		// Send HTTP error response to POS on parse error(s)
		if (0 > (len = depositRequest(sess, bp, req->data, reqLen)))
			return (GxmlErrorResponse(sess, eHttpInternalError));

		// Deposit Request record received, pass its length
		// NOTE: GCS XML deposits are treated as authorizations
		req->len = len;
		req->hdr.svcType = eSvcAuth;
		req->hdr.more = false;
	} // if (!stristr(ctx->reqType,"depositrequest"))

	// Reset Context for next HTTP/POST/XML request
	if ((ctx->atLast)) {
		// Reset the "at last" indicator
		ctx->atLast		= false;

		// Reset the HTTP/XML Document read "state" variables
		ctx->reqLength	= 0;
		memset(ctx->reqType,0,sizeof(ctx->reqType));
		memset(ctx->xmlver,0,sizeof(ctx->xmlver));
		memset(ctx->httpver,0,sizeof(ctx->httpver));

		// Reset the DepositRequest record variables
		ctx->dep_id		= 0;
		ctx->msg_id		= 0LL;
		ctx->batch_date	= 0;
		ctx->batch_nbr	= 0;
		ctx->batch_seq	= 0;
		memset(ctx->chain,0,sizeof(ctx->chain));
		memset(ctx->location,0,sizeof(ctx->location));
		memset(ctx->venue,0,sizeof(ctx->venue));
		memset(ctx->control,0,sizeof(ctx->control));
	} // if ((ctx->atLast))

	// Just got "</Records></Request>" end-of-deposit marker
	if (!(len)) return (eWaitForData);

	// Send Authorization and Deposit record requests to the Switch
	if (!stristr(req->data, "heartbeat")) {
		if (len != PiHostSend(sess, ProxyFlowSequential, sess->pub.replyTTL, req)) {
			PiException(LogError, "PiHostSend", eFailure, "failed");
			return (eFailure);
		} // if (len != PiHostSend(sess, req))

	// Send Heartbeat responses directly to POS
	} else {
		// Count Heartbeat response(s) sent
		++ctx->heartbeats.outPkts;

		return(GxmlSendPos(sess, req));
	} // if (stristr(ctx->reqType,"heartbeat"))

	return (eOk);
} // PiApiResult_t ReadRequest(PiSession_t *sess)

// Local scope method definition
static int
msgReady(PiSession_t *sess)
{
	appData_t *ctx = (appData_t*)sess->pub.context->data;
	PiPeekBfr_t	peek	= { 0 };
	int			bytes	= -1;
	char *		ep		= 0;
	boolean		isEdc	= ctx->isEdc;
	int			len;
	char *		buf;

	PiSessionVerify(sess);

	// Zero bytes, or socket error, return now
	if ((len = PiPosPeek(sess, &peek))) {
		SysLog(LogDebug, "PiPosPeek returned %d (%p,%d)",
			len, peek.bfr, (peek.atEof) ? -1 : peek.len);
		return (-1);
	} // if ((len = PiPosPeek(sess,&peek)))
	len = peek.len;

	// Preview the waiting buffer for a complete XML Message
	// NOTE: A GCS Web XML DepositRequest can be hundreds of thousands
	// of bytes.  For these XML Documents, only insure a full "record"
	// is available, i.e. both <Record>...</Record> tags are available
	// and ONLY read one deposit "record" at a time from the socket
	buf = peek.bfr;

	// Message must start with "POST"
	// insuring we are starting with a proper HTTP request
	// NOTE: unless we are in the middle of a BIG DepositRequest
	if (!(isEdc)) {
		char *bp = 0;

		// Extract the value of the HTTP Content-Length token
		if ((bp = stristr(buf, "content-length:"))) {
			if ((stristr(bp,"\r\n")))
				sscanf(bp, "%*[cont-leghCONT-LEGH: \t]%ld", &ctx->reqLength);
			else
				bp = 0;
		} // if ((bp = stristr(buf, "content-length:")))
		if (!(bp)) return(0); // Wait for more data

		// Must have at least 50 bytes for basic POST HTTP header
		// NOTE: give up after TRYS attempts
		if (50 > len) {
			if (0 < --ctx->trys)
				return(0);
			else
				return(-1);
		} // if (50 > len)

		// Unknown data on socket, fail the session
		if (!(bp = stristr(buf, "POST")) && 4 >= len)
			return(-1);

		// First, return HTTP POST header, terminated by <CR><LF><CR><LF>
		// NOTE: hopefully the XML Document will be on socket next time
		if (!(ctx->httpver)) {
			if (!strstr(buf, "\r\n\r\n")) return(0);
			return (bp - buf + 4);
		} // if (!(ctx->httpver))
	} // if (!(isEdc))

	// Peek at the message type to get the packet length
	if (0 < len) {
		char *bp;

		// Continuing deposit upload packet
		if (!(bp = buf)) return (-1);

		// Find the end-of-record or end-of-request
		if ((ep = stristr(bp, "</Record>"))) {
			bytes = (ep = 1 + strchr(ep,'>')) - buf; // buf peek buf start
			isEdc = true;

		} else if ((ep = stristr(bp,"</Request>"))) {
			bytes = (ep = 1 + strchr(ep,'>')) - buf; // buf peek buf start

		// No data found, wait for more
		} else if (0 < --ctx->trys) {
			return(0);

		} else {
			return(-1);
		} // if ((ep = stristr(bp, "</Record>")))
	} // if (0 < len)

	// No request found, wait for more
	if (!(ep)) {
		bytes = 0;

	// Drain off whitespace after the buffer to read
	} else {
		for(;(ep) && (*ep);ep++) {
			if (strchr(" \t\r\n",*ep))
				bytes++;
			else
				break;
		} // for(;(ep) && (*ep);ep++)

		// Wait for the start of the next DepositRequest Record
		// or the end-of-deposit tags "</Records></Request>" to
		// prevent return just the end-of-deposit tags to caller
		if ((isEdc)) {
			if (strncasecmp(ep,"<Record>",8)) {
				char *pRec = stristr(ep,"</Records>");
				char *pReq = stristr(ep,"</Request>");

				// Did not find "</Records>" followed by "</Request>"
				if (!(pRec) || !(pReq)) bytes = 0;

				// Adjust bytes to include "</Request>"
				bytes = (ep = 1 + strchr(pReq,'>')) - buf;

				// Strip off whitespace after </Request>
				for(;(ep) && (*ep);ep++) {
					if (strchr(" \t\r\n",*ep))
						bytes++;
					else
						break;
				} // for(;(ep) && (*ep);ep++)
			} // if (strncasecmp(ep,"<Record>",8))
		} // if ((isEdc))
	} // if (!(ep))

	// Restore the number of read re-try counter
	if (0 < bytes) ctx->trys = TRYS;

	return (bytes);
} // static int msgReady(PiSession_t *sess)

static int
authorizationRequest(PiSession_t *sess, char *buf, char *req, int reqMax)
{
	appData_t *ctx = (appData_t*)sess->pub.context->data;
	char fmt[512] = { 0 };
	char *rp = req;
	char *bp = buf;
	int len = 0;

	PiSessionVerify(sess);

	// Validate arguments
	if (!(sess) || !(buf) || !(req) || 0 >= reqMax) {
		errno = EINVAL;
		return (-1);
	} // if (!(sess) || !(buf) || !(req) || 0 >= reqMax)

	// Start the HGCS request with the type and length
	rp += sprintf(rp, "%s\n", ctx->reqType);
	if (0 < ctx->reqLength)
		rp += sprintf(rp, "RequestLength,%ld\n", ctx->reqLength);

	// Parse the "Field" tags
	for (; (*bp);) {
		char field[256] = { 0 };
		char *fp = field;

		sprintf(fmt, "%%*[%s]%%n", ctx->xml[eField]);
		len = 0;
		sscanf(bp, fmt, &len);
		if (6 < len) {
			// Find the name of the field
			if ((bp=stristr(bp," name="))) {
				bp += 6;
			} else {
				return(-1);
			} // if ((bp=stristr(bp," name=")))

			// Advance past the leading double quote (")
			for (; (*bp);) if ('"' == *bp++) break;

			// Scan off the field name, add the comman (,) separator
			sscanf(bp, "%[^\">]%n", fp, &len);
			fp[len] = ',';
			fp += 1 + len;
			bp += len;

			// Advance past the trailing bracket (>)
			for (; (*bp);) if ('>' == *bp++) break;

			// Scan off the field value, leaving double qoutes
			len = 0; sscanf(bp, "%[^<]%n", fp, &len);
			fp[len] = 0;
			bp += len;

			// Scan off the end-of-field token, and whitespace
			sprintf(fmt, "%%*[%s]%%*[>\t\r\n]%%n", ctx->xml[eEndOfField]);
			len = 0;
			sscanf(bp, fmt, &len);
			bp += len;

			// Append the transaction fields to the DepositRequest Header(s)
			rp += sprintf(rp, "%s\n", field);

		// Done parsing the <Field></Field>
		} else {
			break;
		} // if (6 < len)
	} // for(;(*bp);)

	// Prepare request for HGCS processing
	if (!stristr(ctx->reqType, "heartbeat")) {
		// Count this request
		++ctx->auths.inPkts;

	// Build a HeartbeatRequest response
	} else {
		char *tmp;

		// Count this Heartbeat
		++ctx->heartbeats.inPkts;

		// Append the Heartbeat status response
		rp += sprintf(rp, "Status,OK\n");

		// Advance past the "RequestType" line
		// NOTE: rp now points to the first field of the Heartbeat request
		rp = strchr(req, '\n');
		rp++;

		// Advance past the "RequestLength" field
		rp = strchr(rp, '\n');
		rp++;

		// Allocate temporary STACK buffer to build a Heartbeat response
		tmp = alloca(128 + strlen(rp));
		bp = tmp;

		// Add the "ResponseType" tag first
		bp += sprintf(bp, "ResponseType,%s\n", "HeartbeatResponse");

		// Append the Heartbeat response message
		bp += sprintf(bp, "%s", rp);

		// Replace the return "request" with a Heartbeat "response"
		sprintf(req, "%.*s", reqMax - 1, tmp);
	} // if (!stristr(ctx->reqType,"heartbeat"))

	// The Context field atLast marks the last "Record" of the request
	// has been sent to the Switch, enabling HTTP/POST multi-threading
	ctx->atLast = true;

	// Return the length of the cooked request
	return (strlen(req));
} // static int authorizationRequest(PiSession_t *,char *,char *,int)

static int
depositRequest(PiSession_t *sess, char *buf, char *req, int reqMax)
{
	appData_t *ctx = (appData_t*)sess->pub.context->data;
	char *			bp				= buf;
	char *			rp				= req;
	char			fmt[512]		= { 0 };
	int				len				= 0;
	int				sqlRc			= 0;
	long			record_nbr;

	PiSessionVerify(sess);

	// Validate arguments
	if (!(sess) || !(buf) || !(req) || 0 >= reqMax) {
		errno = EINVAL;
		return (-1);
	} // if (!(sess) || !(buf) || !(req) || 0 >= reqMax)

	// Start the Host request message
	rp += sprintf(rp,"RequestType,DepositRequest\n");

	// New DepositRequest transaction beginning
	if (!(ctx->dep_id)) {
		// Send the "Content-Length" on first Host request
		rp += sprintf(rp,"RequestLength,%ld\n",ctx->reqLength);

		// Parse off the DepositRequest header XML tags
		// Save selected XML tags in this connection Context memory
		// adding all other header XML tags to the Host request
		for (;(*bp);) {
			char field[256] = {0};
			char value[256] = {0};

			sprintf(fmt, "%%*[%s]%%n", ctx->xml[eField]);
			len = 0; sscanf(bp, fmt, &len);
			if (6 < len) {
				// Find the name of the field
				if ((bp=stristr(bp," name="))) {
					bp += 6;
				} else {
					PiException(LogError, __FUNC__, eFailure,
						"Invalid DepositRequest XML Document\n");
					return(-1);
				} // if ((bp=stristr(bp," name=")))

				// Advance past the leading double quote (")
				for (; (*bp);) if ('"' == *bp++) break;

				// Scan off the field name, add the comma (,) separator
				len = 0; sscanf(bp, "%[^\">]%n", field, &len); bp += len;

				// Advance past the trailing bracket (>)
				for (;(*bp);) if ('>' == *bp++) break;

				// Scan off the field value, leaving double qoutes
				len = 0; sscanf(bp, "%[^<]%n", value, &len); bp += len;

				// Update the Context DepositRequest header fields
				if		(stristr(field,"chain_code"))
					sprintf(ctx->chain,"%.*s",sizeof(ctx->chain)-1,value);
				else if	(stristr(field,"location_id"))
					sprintf(ctx->location,"%.*s",(int)(sizeof(ctx->location)-1),value);
				else if	(stristr(field,"venue_id"))
					sprintf(ctx->venue,"%.*s",(int)(sizeof(ctx->venue)-1),value);
				else if	(stristr(field,"message_id"))
					sscanf(value,"%p",(void**)&ctx->msg_id);
				else if	(stristr(field,"application_id"))
					sprintf(ctx->appl_id,"%.*s",sizeof(ctx->appl_id)-1,value);
				else if	(stristr(field,"control_code"))
					sprintf(ctx->control,"%.*s",(int)(sizeof(ctx->control)-1),value);
				else if	(stristr(field,"batch_date"))
					ctx->batch_date = atol(value);
				// NOTE: must test for batch_nbr AFTER batch_nbr_seq
				else if	(stristr(field,"batch_nbr_seq"))
					ctx->batch_seq = atol(value);
				// NOTE: must test for batch_nbr_seq BEFORE batch_nbr
				else if	(stristr(field,"batch_nbr"))
					ctx->batch_nbr = atol(value);

				// Add XML tag to the request buffer
				rp += sprintf(rp,"%s,%s\n",field,value);

				// Scan off the end-of-field token, and whitespace
				sprintf(fmt, "%%*[%s]%%*[>\t\r\n]%%n", ctx->xml[eEndOfField]);
				len = 0; sscanf(bp, fmt, &len); bp += len;

			// Done parsing the <Field></Field>
			} else {
				break;
			} // if (6 < len)
		} // for(;(*bp);)

		// Seek to the <Records> XML token
		if (!(bp = stristr(buf, "<Records>"))) {
			PiException(LogError, __FUNC__, eFailure,
				"Invalid DepositRequest XML Document\n");
			return (-1);
		} // if (!(bp = stristr(buf, "<Records>")))

		// Parse off the "<Records>" XML Tag
		sprintf(fmt, "%%*[%s]%%*[> \t\r\n]%%n", ctx->xml[eRecords]);
		len = 0; sscanf(bp, fmt, &len);
		if (9 > len) {
			PiException(LogError, __FUNC__, eFailure,
				"Invalid DepositRequest XML Document\n");
			return (-1);
		} // if (9 > len)
		bp += len;

		// Bind the above values to the SQL prepared statement
		sqlite3_reset(ctx->insertHeader);
		sqlite3_bind_int64(ctx->insertHeader,	ihMsg_id,		ctx->msg_id);
		sqlite3_bind_int(ctx->insertHeader,		ihBatch_date,
			ctx->batch_date);
		sqlite3_bind_int(ctx->insertHeader,		ihBatch_date,
			ctx->batch_date);
		sqlite3_bind_int(ctx->insertHeader,		ihBatch_nbr,	ctx->batch_nbr);
		sqlite3_bind_int(ctx->insertHeader,		ihBatch_seq,	ctx->batch_seq);
		sqlite3_bind_text(ctx->insertHeader,	ihChain,		ctx->chain,
			strlen(ctx->chain),SQLITE_TRANSIENT);
		sqlite3_bind_text(ctx->insertHeader,	ihLocation,		ctx->location,
			strlen(ctx->location),SQLITE_TRANSIENT);
		sqlite3_bind_text(ctx->insertHeader,	ihVenue,		ctx->venue,
			strlen(ctx->venue),SQLITE_TRANSIENT);

		// Insert a new DepositRequest SQL header row
		if (SQLITE_DONE != sqlite3_step(ctx->insertHeader)) return(-1);

		// Get the newly inserted header.dep_id value
		sqlite3_reset(ctx->getLastDep_id);
		if (SQLITE_ROW != sqlite3_step(ctx->getLastDep_id)) return(-1);
		ctx->dep_id = sqlite3_column_int64(ctx->getLastDep_id,glDep_id);

	// Another BIG DepositRequest Record read
	} else {
		rp += sprintf(rp,"Chain_Code,%s\n",		ctx->chain);
		rp += sprintf(rp,"Location_ID,%s\n",	ctx->location);
		rp += sprintf(rp,"Venue_ID,%s\n",		ctx->venue);
		rp += sprintf(rp,"Message_ID,%llu\n",	ctx->msg_id);
		rp += sprintf(rp,"Application_ID,%s\n",	ctx->appl_id);
		rp += sprintf(rp,"Control_Code,%s\n",	ctx->control);
		rp += sprintf(rp,"Batch_Date,%06ld\n",	ctx->batch_date);
		rp += sprintf(rp,"Batch_Nbr,%ld\n",		ctx->batch_nbr);
		rp += sprintf(rp,"Batch_Nbr_Seq,%ld\n",	ctx->batch_seq);
	} // if (!(ctx->dep_id))

	// Parse off the "Record" fields of each transaction
	// NOTE: each transaction surrounded by "<Record></Record>" XML
	// tokens.  The msgReady() method will always return a buffer
	// containing at least "<Record></Record>", and possibly the
	// XML Document end tags too, i.e. "</Record></Records></Request>"
	sprintf(fmt, "%%*[%s]%%*[> \t\r\n]%%n", ctx->xml[eRecord]);
	len = 0; sscanf(bp, fmt, &len);
	if (8 > len) {
		PiException(LogError, __FUNC__, eFailure,
			"Invalid DepositRequest XML Document\n");
		return (-1);
	} // if (8 > len)
	bp += len;

	// Parse the next DepositRequest Record
	for (;(*bp);) {
		char field[256] = {0};
		char value[256] = {0};

		sprintf(fmt, "%%*[%s]%%n", ctx->xml[eField]);
		len = 0; sscanf(bp, fmt, &len);
		if (6 < len) {
			// Find the name of the field
			if ((bp=stristr(bp," name="))) {
				bp += 6;
			} else {
				PiException(LogError, __FUNC__, eFailure,
					"Invalid DepositRequest XML Document\n");
				return(-1);
			} // if ((bp=stristr(bp," name=")))

			// Advance past the leading double quote (")
			for (; (*bp);) if ('"' == *bp++) break;

			// Scan off the field name, add the comma (,) separator
			len = 0; sscanf(bp, "%[^\">]%n", field, &len); bp += len;

			// Advance past the trailing bracket (>)
			for (;(*bp);) if ('>' == *bp++) break;

			// Scan off the field value, leaving double qoutes
			len = 0; sscanf(bp, "%[^<]%n", value, &len); bp += len;

			// Add XML tag to the request buffer
			rp += sprintf(rp,"%s,%s\n",field,value);

			// Save the DepositRequest Record number (XML tag Record_Nbr)
			if (stristr(field,"record_nbr")) record_nbr = atol(value);
	
			// Scan off the end-of-field token, and whitespace
			sprintf(fmt, "%%*[%s]%%*[>\t\r\n]%%n", ctx->xml[eEndOfField]);
			len = 0; sscanf(bp, fmt, &len); bp += len;

		// Done parsing the <Field></Field>
		} else {
			break;
		} // if (6 < len)
	} // for(;(*bp);)

	// Seek to the </Record> XML token
	if (0 >= record_nbr || !(bp = stristr(buf, "</Record>"))) {
		PiException(LogError, __FUNC__, eFailure,
			"Invalid DepositRequest XML Document\n");
		return(-1);
	} // if (0 >= record_nbr || !(bp = stristr(buf, "</Record>")))

	// Parse off the "</Record>" XML Tag
	sprintf(fmt, "%%*[%s]%%*[> \t\r\n]%%n", ctx->xml[eEndOfRecord]);
	len = 0; sscanf(bp, fmt, &len);
	if (9 > len) {
		PiException(LogError, __FUNC__, eFailure,
			"Invalid DepositRequest XML Document: no end Record tag");
		return(-1);
	} // if (9 > len)
	bp += len;

	// Insert a new DepositRequest detail row
	sqlite3_reset(ctx->insertDetail);
	sqlite3_bind_int64(ctx->insertDetail,idDep_id,ctx->dep_id);
	sqlite3_bind_int(ctx->insertDetail,idRecord_nbr,record_nbr);
	if (SQLITE_DONE != (sqlRc = sqlite3_step(ctx->insertDetail))) {
		char dbErr[2 * 1024];
		sprintf(dbErr,"%d,%s",sqlRc,sqlite3_errmsg(ctx->db));
		PiException(LogError, __FUNC__, eFailure,
			"DB error inserting deposit detail for (%llu,%d) %s",
			ctx->dep_id,record_nbr,dbErr);
		return(-1);
	} // if (SQLITE_DONE != (sqlRc = sqlite3_step(ctx->insertDetail)))

	// Parse off the "</Records>" XML Tag, and mark end-of-deposit (at_eof)
	// NOTE: the msgReady() method insures both "</Records>" and "</Request>"
	// tags are delivered together indicating DepositRequest termination
	sprintf(fmt, "%%*[%s]%%*[> \t\r\n]%%n", ctx->xml[eEndOfRecords]);
	len = 0; sscanf(bp, fmt, &len);
	if (0 < len) {
		// Mark the DB deposit header row "at eof" too
		sqlite3_reset(ctx->setAtEof);
		sqlite3_bind_int64(ctx->setAtEof,saeDep_id,ctx->dep_id);
		if (SQLITE_DONE != sqlite3_step(ctx->setAtEof)) {
			PiException(LogError, __FUNC__, eFailure,
				"DB error marking DepositRequest at EOF");
			return(-1);
		} // if (SQLITE_DONE != sqlite3_step(ctx->setAtEof))

		// Clear the DepositRequest context memory
   		memset(ctx->chain,			0,	sizeof(ctx->chain));
		memset(ctx->location,		0,	sizeof(ctx->location));
		memset(ctx->venue,			0,	sizeof(ctx->venue));
		memset(ctx->appl_id,		0,	sizeof(ctx->appl_id));
		memset(ctx->control,		0,	sizeof(ctx->control));
		ctx->msg_id		= 0LL;
		ctx->batch_date	= 0;
		ctx->batch_nbr	= 0;
		ctx->batch_seq	= 0;

		// Receipt of DepositRequest complete
		ctx->dep_id = 0;
		ctx->atLast = true;
        ctx->isEdc  = false;

	// Indicate a BIG DepositRequest (EDC) is being received
    } else {
        ctx->isEdc  = true;
	} // if (0 < len)

	// Return the length of the cooked request
	return (strlen(req));
} // static int depositRequest(PiSession_t *,char *,char *,int)

#if 0
static long long
getMessageId(char *msg)
{
	char *bp;

	for(bp=msg;(*bp);) {
		int			len		= 0;
		long long	msgId	= 0LL;

		if (!strncmp(bp,"Message_ID",10)) {
			sscanf(bp,"Message_ID,%llu",&msgId);
			return(msgId);
		} // if (!strncmp(bp,"Message_ID",10))

		sscanf(bp,"%*[^\n]%n",&len);
		if (0 >= len) break;

		bp += 1 + len;
	} // for(bp=msg;(*bp);)

	return(0LL);
} // long long getMessageId(char *msg)
#endif
