/*****************************************************************************

Filename:	lib/svcha/svchasendrsp.c

Purpose:	MICROS SVCHA XML Message Set

			Compliance with specification: MICROS Standard SVC Interface
			Revision 2.5 last updated on 4/05/2005

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

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:51 $
 * $Header: /home/hbray/cvsroot/fec/lib/svcha/svchasendrsp.c,v 1.4.4.2 2011/09/24 17:49:51 hbray Exp $
 *
 $Log: svchasendrsp.c,v $
 Revision 1.4.4.2  2011/09/24 17:49:51  hbray
 Revision 5.5

 Revision 1.4  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.3.2.2  2011/07/27 20:19:54  hbray
 Added cvs headers

 *

2009.10.30 joseph dionne		Created at release 5.9
*****************************************************************************/

#ident "@(#) $Id: svchasendrsp.c,v 1.4.4.2 2011/09/24 17:49:51 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"


// Local (file) scope helper methods

PiApiResult_t
SendResponse(PiSession_t *sess)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;
	
	svchaXml_t *xmlData = (context) ? &context->xmlData : 0;
	HostRequest_t rsp;
	char *xmlRsp = 0;
	char *xp = 0;
	int len;
	int ret;
	int ii;
	int jj;
	char *bp;

	// No response received

	// Receive the switch response
	ret = PiHostRecv(sess, &rsp);
	if (ret != rsp.len)
	{
		PiException(LogError, "PiHostRecv", eFailure, "failed");
		return (eFailure);
	}							// if (ret != rsp.len)
	SysLog(LogDebug, "PiHostRecv len = %d", rsp.len);

	// Terminate switch response packet with null byte
	rsp.data[rsp.len] = 0;

	// Allocate space for POS response
	xmlRsp = alloca(xmlData->len + (3 * rsp.len));
	xp = xmlRsp;
	memset(xp, 0, xmlData->len + (3 * rsp.len));

	// Parse the mapped response fields, convert to SVCHA XML tags
	bp = rsp.data;
	for (; (*bp);)
	{
		char line[2048] = { 0 };
		int mapId = -1;
		char value[1024] = { 0 };
		int len = 0;

		// Scan off one line of the response, and advance buffer pointer
		// Switch response format: MappedNumber,Value<LF>
		// NOTE: <CR> is embedded in the "99" MappedNumber field which
		// should NOT be returned, so <CR> excluded from formats below
		sscanf(bp, "%[^\n]%n", line, &len);
		if (0 >= len)
			break;
		bp += len;
		sscanf(bp, "%*[\n]%n", &len);
		bp += len;

		// Scan off the mapped token, its value and their length 
		// NOTE: <LF> was already removed above, scan stops at NULL
		sscanf(line, "%d,%[^\n]", &mapId, value);

		// Build the response XML tag
		switch (mapId)
		{
		default:
		case -1:
			PiException(LogError, __FUNC__, eFailure, "Don't know about mapped field /%s/\n", line);
			break;

		case 11:
			xp += sprintf(xp, "<SVAN>%s</SVAN>", value);
			break;

		case 12:
			xp += sprintf(xp, "<AuthorizationCode>%s</AuthorizationCode>", value);
			break;

		case 13:
			xp += sprintf(xp, "<Amount>%s</Amount>", value);
			break;

		case 14:
			xp += sprintf(xp, "<AccountCurrency>%s</AccountCurrency>", value);
			break;

		case 15:
			xp += sprintf(xp, "<ExchangeRate>%s</ExchangeRate>", value);
			break;

		case 16:
			xp += sprintf(xp, "<ItemType>%s</ItemType>", value);
			break;

		case 17:
			xp += sprintf(xp, "<ItemNumber>%s</ItemNumber>", value);
			break;

		case 18:
			{
				char tppCode[256] = { 0 };
				char svcCode[256] = { 0 };
				char fmt[32] = { 0 };

				// Build parse format string
				sprintf(fmt, "%%[^%c]%c%%s", FS, FS);

				// Parse the TPP and SVC response codes
				sscanf(value, fmt, tppCode, svcCode);

				// Return the host (TPP) and SVC response codes
				if ((*tppCode))
				{
					xp += sprintf(xp, "<ResponseCode hostCode=\"%s\">%s</ResponseCode>", tppCode, svcCode);

					// Return just the SVC response code
				}
				else
				{
					xp += sprintf(xp, "<ResponseCode>%s</ResponseCode>", value);
				}				// if ((*tppCode))
			}
			break;

		case 19:
			xp += sprintf(xp, "<DisplayMessage>%s</DisplayMessage>", value);
			break;

		case 20:
			xp += sprintf(xp, "<AccountBalance>%s</AccountBalance>", value);
			break;

		case 29:
			xp += sprintf(xp, "<LocalBalance>%s</LocalBalance>", value);
			break;
		}						// switch(mapId)
	}							// for(;(*bp);)

	// Begin the POS response packet
	// NOTE: xmlData->extra was allocated by ReadRequest()
	xp = xmlData->extra;
	jj = sprintf(xp, "%c%s%c%s%c", SOH, xmlData->origin, FS, xmlData->target, STX);
	len = jj;
	xp += jj;

	// Build the response POS XML Document
	for (ii = 0; (xmlData->save[ii]); ii++)
	{
		char aTag[512] = { 0 };
		char *tp = xmlData->save[ii];

		// Exit when closing XML Document tag found
		if (stristr(tp, "</SVCMessage"))
			break;

		// Send back tags received NOT found in the response
		sscanf(tp, "%[^ \t>]s", aTag);
		if (!stristr(xmlRsp, aTag))
			jj = sprintf(xp, "%s", tp);
		else
			continue;

		// Keep running length
		len += jj;

		// Advance the response buffer pointer
		xp += jj;
	}							// for(ii=0;(xmlData->save[ii]);ii++)

	// Append the response XML tags
	jj = sprintf(xp, "%s", xmlRsp);
	len += jj;
	xp += jj;

	// Append the closing XML Document tag
	jj = sprintf(xp, "%s", xmlData->save[ii]);
	len += jj;
	xp += jj;

	// Terminate the MICROS SVC packet
	len += sprintf(xp, "%c%c", ETX, EOT);

	// Send POS Response packet
	if (len != PiPosSend(sess, xmlData->extra, len))
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return (eFailure);
	}							// if (len != PiPosSend(sess,xmlData->extra,len))

	// Clear and release the HEAP memory
	memset(xmlData->save, 0, (6 * xmlData->len));
	free(xmlData->save);
	xmlData->save = 0;

	// Count this response
	if ((context->auth.inPkts))
		++context->auth.outPkts;
	else
		++context->edc.outPkts;

	return (eDisconnect);
}								// int sendResponse(fecconn_t *sess)
