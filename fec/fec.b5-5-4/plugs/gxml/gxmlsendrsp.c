/*****************************************************************************

Filename:	lib/gxml/gxmlsendrsp.c

Purpose:	GCS HTTP/XML Message Set

			Compliance with specification: "GCS XML Specification"
			Version 4.1.1 last updated on 8/16/2005, by Theron Crissey

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
 * $Date: 2011/09/24 17:49:41 $
 * $Header: /home/hbray/cvsroot/fec/lib/gxml/gxmlsendrsp.c,v 1.3.4.3 2011/09/24 17:49:41 hbray Exp $
 *
 $Log: gxmlsendrsp.c,v $
 Revision 1.3.4.3  2011/09/24 17:49:41  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/17 17:58:57  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:15  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:42  hbray
 Added cvs headers

 *

2010.09.01 joseph dionne		Added SQLite to allow all requests, including
								DepositRequests, to multi thread, i.e. send
								without waiting for response.
2009.09.22 joseph dionne		Ported to release v4.3
2009.08.04 joseph dionne		Created.
*****************************************************************************/

#ident "@(#) $Id: gxmlsendrsp.c,v 1.3.4.3 2011/09/24 17:49:41 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"
#include "include/http.h"


// Local (file) scope helper methods
static int xmlConcat(char *tgt, char *http, char *content);
static PiApiResult_t depositResponse(PiSession_t *, HostRequest_t *);

PiApiResult_t
SendResponse(PiSession_t *sess)
{
	// Worker's "file scope" application data stored in sess->pub.appData
	appData_t *ctx = (appData_t*)sess->pub.context->data;
	HostRequest_t *rsp;
	int ret;

	PiSessionVerify(sess);

	// No response received

	// Receive the switch response
	rsp = (HostRequest_t *)alloca(sizeof(HostRequest_t));
	ret = PiHostRecv(sess, rsp);
	if (ret != rsp->len) {
		PiException(LogError, "PiHostRecv", eFailure, "failed");
		return (eFailure);
	} // if (ret != rsp->len)
	SysLog(LogDebug, "PiHostRecv len = %d", rsp->len);

	// Terminate switch response packet with null byte
	rsp->data[rsp->len] = 0;

	// Invalid HGCS response, should start with "ResponseType,...\n"
	if (!stristr(rsp->data,"ResponseType,"))
		return (GxmlErrorResponse(sess, eHttpInternalError));

	// Cache DepositResponse batch responses to disk, and transmit the
	// cached XML Document to the POS  after all responses are received
	// NOTE: the cache does NOT contain any sensitive data as indicated
	// by the specification, and is required to calculate the HTTP
	// Content-Length value prior to transmission to the POS client
	// NOTE: to support so-called "HTTP pipelining" or multi-threading
	// of HTTP POSTs, use of SQLite has been added
	if (stristr(rsp->data,",depositresponse")) {
		++ctx->edc.outPkts;
		return (depositResponse(sess, rsp));
	} // if (stristr(rsp->data,",depositresponse"))

	// Count this response
	++ctx->auths.outPkts;

	return (GxmlSendPos(sess, rsp));
} // PiApiResult_t SendResponse(PiSession_t *sess)

// Global (plug-in) scope method definition
// NOTE: GxmlSendPos() is called directly from ReadRequest() for
// GCS XML Heartbeat processing.  The GxmlSendPos() method must
// NOT be made static to this source module.
PiApiResult_t
GxmlSendPos(PiSession_t *sess, HostRequest_t *rsp)
{
	// Worker's "file scope" application data stored in sess->pub.appData
	appData_t *ctx = (appData_t*)sess->pub.context->data;
	char httpTime[64] = { 0 };
	char httpHead[512] = { 0 };
	char *buf;
	char *bp;
	char *sp;
	int len;

	PiSessionVerify(sess);

	// Validate arguments
	if (!(ctx) || !(rsp)) {
		PiException(LogError, __FUNC__, eFailure, "no ctx or rsp");
		return (eFailure);
	} // if (!(ctx) || !(rsp))

	// Get the current GMT time for response
	HttpGetTime(time(0), httpTime);

	// Allocate a transmission buffer, packet length plus IP header
	// NOTE: allocate from STACK memory, DO NOT FREE
	// NOTE: allocating addition space for the HTTP and XML "headers"
	len = rsp->len;
	if (!(buf = alloca((16 * len) + 256)))
		return (false);
	memset(buf, 0, (16 * len) + 256);

	// Convert "Field_name,value" pairs into XML container
	sp = rsp->data;
	for (bp = buf; *sp;) {
		char line[8 * 1024] = { 0 };
		int lineLen = -1;
		char name[128] = { 0 };
		char value[4 * 1024] = { 0 };
		int ii;

		// Parse off a line of text up to and including the LF('\n')
		sscanf(sp, "%[^\n]%*[\n]%n", line, &lineLen);
		sp += lineLen;

		// Blank line?
		if (!strlen(line)) continue;

		// Parse the "FieldName,Value" pair
		// TODO: Need to test format below for parsing leading whitespace
		ii = sscanf(line, "%[^\t ,]%*[\t ,]%[^\n]s", name, value);

		// Add each "Field name=Value" pair as XML Field(s)
		if (!stristr(name,"responsetype")) {
			bp += sprintf(bp, "<Field name=\"%s\">%s</Field>", name, value);

		// Build the start of the HTTP message and XML container
		} else {
			// Build the HTTP Response Entity-Header sprintf format string
			sprintf(httpHead, "%s %s \r\n"		// "HTTP/1.0","200 OK",
				"Server: %s\r\n"				// "Fusebox",
				"Date: %s\r\n"					// httpTime,
				"Last-modified: %s\r\n"			// httpTime,
				"Content-length: %s\r\n"		// "%d",
				"Content-type: %s\r\n"			// "text/xml",
				"\r\n"							// -- end-of-http content --
				"%s",							// "%s" for the XML Content
				"HTTP/1.0",                 	// --- start sprintf data ---
				!stristr("error", value)		// if not "ResponseType,Error"
				? "200 OK"						// then send HTTP 200
				: "500 Internal Error",			// else send HTTP 500
				"Fusebox",						// "Server: %s\r\n"
				httpTime,						// "Date: %s\r\n"
				httpTime,						// "Last-modified: %s\r\n"
				"%d",							// "Content-length: %s\r\n"
				"text/xml",						// "Content-type: %s\r\n"
				"%s");							// formater for XML content

			// Start the ResponseType XML Document
			bp += sprintf(bp,"<?xml version=\"1.0\"?><Response Type=\"%s\">",
				value);
		} // if (!stristr(name,"requesttype"))
	} // for(bp = buf;*sp;)

	// Append the XML container terminator
	sprintf(bp, "</Response>");
	len = xmlConcat(buf, httpHead, buf);

	if (len != PiPosSend(sess, buf, len)) {
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return (eFailure);
	} // if (len != PiPosSend(sess,buf,len))

	return (eOk);
} // PiApiResult_t GxmlSendPos(PiSession_t *sess,HostRequest_t *rsp)

// Local scope method definition
static int
xmlConcat(char *tgt, char *http, char *content)
{
	int len = strlen(content);
	char *tmp;

	// Allocate STACK memory to combine HTTP and Content 
	tmp = alloca(1 + strlen(http) + len + 1);

	// Combine HTTP and Content
	len = sprintf(tmp, http, len, content);

	// Copy into caller's target memory
	memcpy(tgt, tmp, len);

	// Return the length of combined HTTP and Content
	return (len);
} // int xmlConcat(char *tgt,char *http,char *content)

static PiApiResult_t
depositResponse(PiSession_t *sess, HostRequest_t *rsp)
{
	// Worker's "file scope" application data stored in sess->pub.appData
	appData_t *ctx = (appData_t*)sess->pub.context->data;
	char		temp[8 * 1024]					= {0};
	long long	dep_id							= 0;
	int			requests						= 0;
	int			responses						= 0;
	boolean		at_eof							= 0;
	char		chain[sizeof(ctx->chain)]		= {0};
	char		location[sizeof(ctx->location)]	= {0};
	char		venue[sizeof(ctx->venue)]		= {0};
	char		control[sizeof(ctx->control)]	= {0};
	char		status[4]						= {0};
	long long	msg_id							= 0LL;
	long		batch_date						= 0;
	long		batch_nbr						= 0;
	long		batch_seq						= 0;
	long		xml_size						= 0;
	long		record_nbr						= 0;
	char		resp_code[4]					= {0};
	char		resp_text[32]					= {0};
	int			sqlRc;
	char *		bp;
	char *		tp;

	PiSessionVerify(sess);

	// Parse off the DepositResponse header fields
	for (bp = rsp->data;(*bp);) {
		char	line[8 * 1024]	= { 0 };
		int		lineLen			= -1;
		char	field[512]		= { 0 };
		char	value[512]		= { 0 };

		// Parse off a line of text up to and including the LF('\n')
		sscanf(bp, "%[^\n]%*[\n]%n", line, &lineLen);
		bp += lineLen;

		// Blank line?
		if (!strlen(line)) continue;

		// Parse the "FieldName,Value" pair, send error if parse fails
		if (1 > sscanf(line, "%[^\t ,]%*[\t ,]%[^\n]s", field, value))
			return (GxmlErrorResponse(sess, eHttpInternalError));

		// Extract the DepositRequest DB field data
		if		(stristr(field,"chain_code"))
			sprintf(chain,"%.*s",(int)(sizeof(chain)-1),value);
		else if	(stristr(field,"location_id"))
			sprintf(location,"%.*s",(int)(sizeof(location)-1),value);
		else if	(stristr(field,"venue_id"))
			sprintf(venue,"%.*s",(int)(sizeof(venue)-1),value);
		else if	(stristr(field,"message_id"))
			sscanf(value,"%llu",&msg_id);
		else if	(stristr(field,"control_code"))
			sprintf(control,"%.*s",(int)(sizeof(control)-1),value);
		else if	(stristr(field,"batch_date"))
			batch_date = atol(value);
		// NOTE: must test for batch_nbr AFTER batch_nbr_seq
		else if	(stristr(field,"batch_nbr_seq"))
			batch_seq = atol(value);
		// NOTE: must test for batch_nbr_seq BEFORE batch_nbr
		else if	(stristr(field,"batch_nbr"))
			batch_nbr = atol(value);
		else if	(stristr(field,"status"))
			sprintf(status,"%.*s",(int)(sizeof(status)-1),value);

		// Extract the DepositRequest detail DB field data
		else if	(stristr(field,"record_nbr"))
			record_nbr = atol(value);
		else if	(stristr(field,"response_code"))
			sprintf(resp_code,"%.*s",(int)(sizeof(resp_code)-1),value);
		else if	(stristr(field,"response_text"))
			sprintf(resp_text,"%.*s",(int)(sizeof(resp_text)-1),value);
	} // for (bp = rsp->data;(*bp);)

	// Find the DB deposit header row
	sqlite3_reset(ctx->findHeader);
	sqlite3_bind_int64(ctx->findHeader,	fhMsg_id,		msg_id);
	sqlite3_bind_int(ctx->findHeader,	fhBatch_date,	batch_date);
	sqlite3_bind_int(ctx->findHeader,	fhBatch_nbr,	batch_nbr);
	sqlite3_bind_int(ctx->findHeader,	fhBatch_seq,	batch_seq);
	sqlite3_bind_text(ctx->findHeader,	fhChain,		chain,
		strlen(chain),SQLITE_TRANSIENT);
	sqlite3_bind_text(ctx->findHeader,	fhLocation,		location,
		strlen(location),SQLITE_TRANSIENT);
	sqlite3_bind_text(ctx->findHeader,	fhVenue,		venue,
		strlen(venue),SQLITE_TRANSIENT);
	sqlRc = sqlite3_step(ctx->findHeader);
	if (SQLITE_ROW == sqlRc) {
		char dbStatus[sizeof(status)] = {0};

		dep_id		= sqlite3_column_int64(ctx->findHeader,fhDep_id);
		requests	= sqlite3_column_int(ctx->findHeader,fhRequests);
		responses	= sqlite3_column_int(ctx->findHeader,fhResponses);
		at_eof		= 0 != sqlite3_column_int(ctx->findHeader,fhAt_eof);
		xml_size	= sqlite3_column_int(ctx->findHeader,fhXml_size);
		sprintf(dbStatus,"%.*s",sizeof(dbStatus)-1,
			(char *)sqlite3_column_text(ctx->findHeader,fhStatus));

		// If deposit record failed, update deposit DB header status
		// NOTE: update header Status only on first failed Record
		if (!(stristr(status,"ok")) && stristr(dbStatus,"ok")) {
			char buf[1 * 1024];

			sprintf(buf,"update header set status = '%s' "
				"where dep_id = %llu",status,dep_id);
			sqlRc = sqlite3_exec(ctx->db,buf,0,0,0);
			if ((sqlRc)) {
				PiException(LogError, __FUNC__, eFailure,
					"failed to update deposit header for %s,%s,%s,%lu\n",
					chain,location,venue,batch_date);
				return (eFailure);
			} // if ((sqlRc))
		} // if (!(stristr(status,"ok")) && stristr(dbStatus,"ok"))

	// Deposit header row not found, fatal failure
	} else {
		PiException(LogError, __FUNC__, eFailure,
			"no deposit header found (%d) for %s,%s,%s,%lu,%lu,%lu\n",sqlRc,
			chain,location,venue,batch_date,batch_nbr,batch_seq);
		return (eFailure);
	} // if (SQLITE_ROW == sqlRc)

	// Update the deposit detail row
	tp = temp;
	tp += sprintf(tp,"<Record>");
	tp += sprintf(tp,"<Field name=\"Record_Nbr\">%lu</Field>",record_nbr);
	if ((*resp_code)) {
		tp += sprintf(tp,"<Field name=\"Response_Code\">%s</Field>",
			resp_code);
	} // if ((*resp_code))
	if ((*resp_text)) {
		tp += sprintf(tp,"<Field name=\"Response_Text\">%s</Field>",
			resp_text);
	} // if ((*resp_text))
	tp += sprintf(tp,"</Record>");
	sqlite3_reset(ctx->updateDetail);
	sqlite3_bind_text(ctx->updateDetail,udXml_resp,
		temp,strlen(temp),SQLITE_TRANSIENT);
	sqlite3_bind_int64(ctx->updateDetail,udDep_id,dep_id);
	sqlite3_bind_int(ctx->updateDetail,udRecord_nbr,record_nbr);
	sqlRc = sqlite3_step(ctx->updateDetail);
	if (SQLITE_DONE == sqlRc) {
		responses++;				// increment this response
		xml_size += strlen(temp);	// add this response length
	} else {
		PiException(LogError, __FUNC__, eFailure,
			"failed to update deposit detail for %s,%s,%s,%lu\n",
			chain,location,venue,batch_date);
		return (eFailure);
	} // if (SQLITE_DONE == sqlRc)

	// Send the completed XML DepositResponse to the POS
	if ((at_eof) && (requests == responses)) {
		char *	nl						= "\r\n";
		char	httpTime[64]			= {0};
		char	xmlDocHead[4 * 1024]	= {0};
		long	contentLength			= 0;
		int		bytes;

		// Get the current GMT time for response
		HttpGetTime(time(0), httpTime);

		// Build the DepositResponse XML Document deposit
		// header, calculating the "Content-Length" bytes
		// NOTE: xml_size is from the deposit header row,
		// it is the sum(detail.xml_resp) for each Record
		contentLength = strlen("</Records></Response>") + xml_size
			+ sprintf(xmlDocHead,"<?xml version=\"1.0\" ?>"
			"<Response type=\"DepositResponse\">"
			"<Field name=\"Chain_Code\">%s</Field>"
			"<Field name=\"Location_ID\">%s</Field>"
			"<Field name=\"Venue_ID\">%s</Field>"
			"<Field name=\"Message_ID\">%llu</Field>"
			"<Field name=\"Control_Code\">%s</Field>"
			"<Field name=\"Batch_Date\">%06lu</Field>"
			"<Field name=\"Batch_Nbr\">%lu</Field>"
			"<Field name=\"Batch_Nbr_Seq\">%lu</Field>"
			"<Field name=\"Status\">%s</Field><Records>",
			chain,location,venue,msg_id,control,
			batch_date,batch_nbr,batch_seq,status);

		// Start with the HTTP Response header
		tp		= temp;
		bytes	= sprintf(tp,
			// HTTP protocol header format
			"%s %s %s"						// "HTTP/1.0","200 OK",nl,
			"Server: Fusebox%s"				// nl,
			"Date: %s%s"					// httpTime,nl,
			"Last-modified: %s%s"			// httpTime,nl,
			"Content-length: %ld%s"			// contentLength,nl,
			"Content-type: text/xml%s%s",	// nl,nl
			// HTTP protocol header data
			"HTTP/1.0", "200 OK", nl,		// "%s %s %s"
			nl,								// "Server: Fusebox%s"
			httpTime, nl,					// "Date: %s%s"
			httpTime, nl,					// "Last-modified: %s%s"
			contentLength, nl,				// "Content-length: %ld%s"
			nl, nl);						// "Content-type: text/xml%s%s"
		tp += strlen(tp);

		// Append the start of the DepositResponse XML Document header
		bytes			+= sprintf(tp,"%s",xmlDocHead);
		tp				+= strlen(tp);
		contentLength	+= bytes;

		// Read the deposit details and append the
		// Record tags to the DepositResponse XML
		sqlite3_reset(ctx->getXmlResp);
		sqlite3_bind_int64(ctx->getXmlResp,gxrDep_id,dep_id);
		for(;SQLITE_ROW == sqlite3_step(ctx->getXmlResp);) {
			bytes	+= sprintf(tp,"%s",(char *)
				sqlite3_column_text(ctx->getXmlResp,gxrXml_resp));
			tp		+= strlen(tp);

			// Count each response "sent"
			responses--;

			// Terminate XML Document on last Record from the DB
			// NOTE: last packet sent after detail read loop
			if (!(responses)) break;

			// Build and send approximately 4kb transmit packets
			if ((4 * 1024) > bytes) continue;

			// Transmit the response packet to the GCS Web XML POS
			if (bytes != PiPosSend(sess, temp, bytes)) {
				PiException(LogError, "PiPosSend", eFailure, "failed");
				return (eFailure);
			} // if (bytes != PiPosSend(sess, temp, bytes))

			SysLog(LogDebug, "Wrote %d of %ld total bytes to POS\n",
				bytes, contentLength);

			// Reset temp buffer
			memset(temp,0,sizeof(temp));
			tp		= temp;
			bytes	= 0;
		} // for(;SQLITE_ROW == sqlite3_step(ctx->getXmlResp);)

		// Transmit the last response packet to the GCS Web XML POS
		if (0 < bytes) {
			bytes += sprintf(tp,"</Records></Response>");
			if (bytes != PiPosSend(sess, temp, bytes)) {
				PiException(LogError, "PiPosSend", eFailure, "failed");
				return (eFailure);
			} // if (bytes != PiPosSend(sess, temp, bytes))

			SysLog(LogDebug, "Wrote %d of %ld total bytes to POS\n",
				bytes, contentLength);
		} // if (0 < bytes)

		// Delete the deposit header and detail rows
		sprintf(temp,"delete from header where dep_id = %llu",dep_id);
		sqlRc = sqlite3_exec(ctx->db,temp,0,0,0);
		if ((sqlRc)) {
			PiException(LogError, __FUNC__ , eFailure,
				"Error removing deposit DB entries" );
			return (eFailure);
		} // if ((sqlRc))
	} // if ((at_eof) && (requests == responses))

	return (eOk);
} // static PiApiResult_t depositResponse(PiSession_t *,HostRequest_t *)
