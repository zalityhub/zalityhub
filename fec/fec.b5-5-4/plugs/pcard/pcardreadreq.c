/*****************************************************************************

Filename:	lib/pcard/pcardreadreq.c

Purpose:	PropertyCard Message Set

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
 * $Header: /home/hbray/cvsroot/fec/lib/pcard/pcardreadreq.c,v 1.3.4.9 2011/11/16 19:33:02 hbray Exp $
 *
 $Log: pcardreadreq.c,v $
 Revision 1.3.4.9  2011/11/16 19:33:02  hbray
 Updated

 Revision 1.3.4.8  2011/10/27 18:34:00  hbray
 Revision 5.5

 Revision 1.3.4.7  2011/09/24 17:49:49  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/02 14:17:03  hbray
 Revision 5.5

 Revision 1.3.4.4  2011/09/01 14:49:47  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/25 18:19:45  hbray
 *** empty log message ***

 Revision 1.3.4.1  2011/08/11 19:47:35  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:21  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:51  hbray
 Added cvs headers

 *

2009.09.22 harold bray          Updated to support release 4.2
2009.05.29 harold bray		Created release 3.1.
*****************************************************************************/

#ident "@(#) $Id: pcardreadreq.c,v 1.3.4.9 2011/11/16 19:33:02 hbray Exp $ "

// Application plugin data/method header declarations

#include <ctype.h>
	
#include "data.h"
#include "include/sdcdes.h"
static const int MaxPktLen = 32767;


// Local scope method definition
static inline boolean
IsHsiMessage(char *data, int len) 
{
	return (len >= 4 && data[0] == '$' && 
			 (memcmp(&data[2], "1B", 2) == 0 || memcmp(&data[2], "1D", 2) == 0 || memcmp(&data[2], "05", 2) == 0 || memcmp(&data[2], "06", 2) == 0
			  || memcmp(&data[2], "08", 2) == 0));
}


static int 
Decrypt(PiSession_t * sess, char *eblock, int esize) 
{
	desecb_t enc;

	PiSessionVerify(sess);

	if (esize <= 0)
	{
		PiException(LogError, __FUNC__, eFailure, "%d are too few bytes to decrypt", esize);
		return -1;
	}
	
	// Prime cipher
	char key[8] = { 0xc0, 0x11, 0x79, 0xf7, 0xee, 0xbf, 0xb9, 0x52 };
	if (desecbStart(&enc, key, 8))
	{
		PiException(LogError, "desecbStart", eFailure, "failed; errno=%d", errno);
		return -1;
	}

	int elen;
	char *ct = desecbDecrypt(enc, eblock, esize, &elen);
	if (ct != NULL)
	{
		memcpy(eblock, ct, elen);
		free(ct);
	}
	desecbEnd(&enc);
	return 0;
}


static int 
msgReady(PiSession_t * sess) 
{

	PiSessionVerify(sess);
	
	// Preview the waiting buffer for a complete API
	PiPeekBfr_t peek;
	if (PiPosPeek(sess, &peek))
		return 0;

	int len;
	char *bfr = peek.bfr;

	len = peek.len;
	bfr = peek.bfr;
	if (len < 5)				// must have at least the transmission header
		return 0;				// wait longer...

	if (bfr[4] != 0x1c)		// Not FS; this is not a good situation
	{
		PiException(LogError, __FUNC__, eFailure, "Transmission header does not contain an FS chr in offset 4: %s", StringDump(NULL, bfr, len, 0)->str);
		return -1;
	}
	char transmissionLen[5];

	memset(transmissionLen, 0, sizeof(transmissionLen));
	memcpy(transmissionLen, bfr, 4);
	if (strlen(transmissionLen) != 4)
	{
		PiException(LogError, __FUNC__, eFailure, "Transmission header does not contain 4 hex digits: %s", StringDump(NULL, bfr, len, 0)->str);
		return -1;
	}
	int plen = 0;

	
// convert the length in the transmission header
// which is 4 digits of hex numbers: 0123 = 291 bytes to follow the header...
	for (char *ptr = transmissionLen; *ptr; ++ptr)
	{
		int digit = toupper(*ptr);

		if (!isxdigit(digit))
		{
			PiException(LogError, __FUNC__, eFailure, "Transmission header contains a non hex digit: %s", StringDump(NULL, bfr, len, 0)->str);
			return -1;
		}

		if ((digit -= '0') > 9)
			digit -= 7;

		plen = (plen << 4) | digit;
	}

	SysLog(LogDebug, "Transmission header says there are %d bytes to follow", plen);
	if (plen > MaxPktLen)
	{
		PiException(LogError, __FUNC__, eFailure, "Transmission header indicates too many characters; header says %d; max is %d: %s", plen, MaxPktLen, StringDump(NULL, bfr, len, 0)->str);
		return -1;
	}

	plen += 5;				// total expected pkt len
	if (peek.len < plen)
		return 0;				// need more

	return plen;				// this amount is ready
} 


// Returns in new memory, the next CGI arg
//   returns NULL if none
// caller must release the memory (if any)
//
static char*
NextArg(char *arglist) 
{
	char *start = arglist;

	if (strlen(start) <= 0)
		return NULL;			// no args are left

	char *end = strchr(start + 1, '&');

	if (end == NULL)
		end = &start[strlen(start)];

	char *arg = malloc((end - start) + 10);

	memcpy(arg, start, (end - start));
	arg[(end - start)] = '\0';
	return arg;
}


// Separates arg into a name/value pair; also removes any leading/trailing spaces from name and value
// Example: name = value
// Becomes:
//  "name"
//  "value"
//
static void 
SplitArg(char *arg, char **nameptr, char **valueptr) 
{
	char *name = *nameptr;
	char *value = *valueptr;

	strcpy(name, arg);
	char *equal = strchr(name, '=');

	if (equal == NULL)
	{
		strcpy(value, "");		// no value
	}
	else
	{
		*equal++ = '\0';		// chop...
		strcpy(value, equal);	// set value
	}
	
	// now remove trailing/leading spaces
	for (char *ptr = &name[strlen(name) - 1]; ptr >= name && isspace(*ptr);)
		*ptr-- = '\0';			// trailing
	for (char *ptr = name; isspace(*ptr);)
		strcpy(ptr, &ptr[1]);
	for (char *ptr = &value[strlen(value) - 1]; ptr >= value && isspace(*ptr);)
		*ptr-- = '\0';			// trailing
	for (char *ptr = value; isspace(*ptr);)
		strcpy(ptr, &ptr[1]);
}


static int
BuildRequest(PiSession_t * sess, char *packet, char *request, HostSvcType_t *svcType, boolean *more) 
{
	char *arglist;

	PiSessionVerify(sess);
	
	appData_t *context = (appData_t*)sess->pub.context->data;

// Preallocate empty buffers; makes buffer management easier...
	char *rqsttype = strdup("");
	char *ackenabled = strdup("");
	char *sysid = strdup("");
	char *propid = strdup("");
	char *hotelchain = strdup("");
	char *defaultkey = strdup("");
	char *cmd = strdup("");
	char *timestamp = strdup("");

	if ((arglist = strchr(packet, '?')) == NULL)
		arglist = strchr(packet, '&');

	if (arglist == NULL)
	{
		PiException(LogError, __FUNC__, eFailure, "No args in input packet: %s", StringDump(NULL, packet, strlen(packet), 0)->str);
		return -1;
	}
	char *arg;

	while ((arg = NextArg(arglist)) != NULL)
	{
		char *name = alloca(strlen(arg) + 10);
		char *value = alloca(strlen(arg) + 10);

		SplitArg(arg, &name, &value);
		Downshift(name);
		SysLog(LogDebug, "Parsing arg '%s', value '%s'", name, value);
		if (strcmp(name, "&rqsttype") == 0)
		{
			free(rqsttype);
			rqsttype = strdup(value);
		}
		else if (strcmp(name, "&ackenabled") == 0)
		{
			free(ackenabled);
			ackenabled = strdup(value);
		}
		else if (strcmp(name, "&sysid") == 0)
		{
			free(sysid);
			sysid = strdup(value);
		}
		else if (strcmp(name, "&propid") == 0)
		{
			free(propid);
			propid = strdup(value);
		}
		else if (strcmp(name, "&hotelchain") == 0)
		{
			free(hotelchain);
			hotelchain = strdup(value);
		}
		else if (strcmp(name, "&cmd") == 0)
		{
			free(cmd);
			cmd = strdup(value);
		}
		else if (strcmp(name, "&default_key") == 0)
		{
			free(defaultkey);
			defaultkey = strdup(value);
		}
		else
		{
			SysLog(LogDebug, "Unhandled arg '%s', value '%s'", name, value);
		}
		arglist += strlen(arg);
		free(arg);
	}
	
// Determine request type; auth or edc
	*svcType = eSvcAuth;	// default type
	*more = false;

	
// If not an HSI, determine the request type
	if (!IsHsiMessage(cmd, strlen(cmd)))
	{
		for (int ii = 1; (*context->msgNa[ii]); ++ii)
		{
			boolean found = strncmp(context->msgNa[ii] + 1, cmd, 6) == 0;
			if ((found))
			{
				SysLog(LogDebug, "%.6s == %.6s", context->msgNa[ii] + 1, cmd);
				*more = '0' == context->msgNa[ii][0];
				*svcType = 'C' == context->msgNa[ii][1] || 'I' == context->msgNa[ii][2] ? eSvcAuth : eSvcEdcMulti;
				break;
			}
		}
	
		NxClient_t *client = sess->_priv.client;
		NxClientVerify(client);
		context->servicePort = client->evf->servicePort;
	}
	else
	{
		*more = false;
		*svcType = eSvcAuth;
		context->servicePort = context->altPort;	// this is an HSI message; use alternate port
	}
	
// format the payload
	char *out = request;

	out += sprintf(out, "%c", (*svcType == eSvcAuth) ? 'A' : 'S');
	out += sprintf(out, "%-10.10s", defaultkey);
	out += sprintf(out, "%-2.2s", hotelchain);
	out += sprintf(out, "%c", ((strncmp(rqsttype, "AUTH", 5) == 0)) ? 'Y' : 'N');
	out += sprintf(out, "%s", cmd);
	
// optionally, send an ack to the terminal
	if (stricmp(ackenabled, "ack") == 0)
	{
		char *ack = "ack";
		int len = strlen(ack);

		if (PiPosSend(sess, ack, len) != len)
			PiException(LogError, "PiPosSend", eFailure, "failed");
	}
	free(defaultkey);
	free(rqsttype);
	free(ackenabled);
	free(sysid);
	free(propid);
	free(hotelchain);
	free(cmd);
	free(timestamp);
	return 0;
}


static int 
SendToHost(PiSession_t * sess, char *data, int len, HostSvcType_t svcType, boolean more)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;
	
		// Allocate memory for the request
	HostRequest_t * request = alloca(sizeof(HostRequest_t));
	if (!(request))
		SysLog(LogFatal, "Unable to allocate a request buffer");

	memset(request, 0, sizeof(HostRequest_t));
	
		// send the host request
	request->hdr.svcType = svcType;
	request->hdr.more = more;
	request->len = len;
	memcpy(request->data, data, len);

	if (PiHostSendServicePort(sess, ProxyFlowSequential, sess->pub.replyTTL, request, context->servicePort) != request->len)
	{
		PiException(LogError, "PiHostSendServicePort", eFailure, "failed");
		return -1;
	}
	return 0;
}


static char *
TransformData(appData_t * context, char *bfr, int *len) 
{
	if (stricmp(context->inputEntityEncoding, "url") == 0)
		bfr = DecodeUrlCharacters(bfr, len);
	else if (stricmp(context->inputEntityEncoding, "xml") == 0)
		bfr = DecodeEntityCharacters(bfr, len);
	return bfr;
}


PiApiResult_t  ReadRequest(PiSession_t * sess) 
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
	char *bfr = alloca(8 + len);

	memset(bfr, 0, 8 + len);
	
	// Read message from the buffer
	int rlen = PiPosRecv(sess, bfr, len);
	if (rlen != len)
	{
		PiException(LogError, __FUNC__, eFailure, "Expected %d, received %d", len, rlen);
		return eFailure;
	}
	bfr += 5;					// Remove
	len -= 5;					// header
	Decrypt(sess, bfr, len);
	bfr[len] = '\0';			// insure termination
	char *decoded = TransformData(context, bfr, &len);
	SysLog(LogDebug | SubLogDump, decoded, len, "decoded %d", len);

	char *payload = alloca(len + 1024);

	HostSvcType_t svcType;
	boolean more;
	if ( BuildRequest(sess, decoded, payload, &svcType, &more) < 0 )
	{
		PiException(LogError, "BuildRequest", eFailure, "failed");
		return eFailure;
	}

	int plen = strlen(payload);

	if (SendToHost(sess, payload, plen, svcType, more))
	{
		PiException(LogError, "SendToHost", eFailure, "failed");
		return eFailure;
	}

	return eOk;
}
