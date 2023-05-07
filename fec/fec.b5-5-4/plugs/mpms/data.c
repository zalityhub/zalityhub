/*****************************************************************************

Filename:	lib/mpms/data.c

Purpose:	Marriott PMS2Way Message Set
			See lib/mpms/instance.c for supported Marriott PMS2Way Messages

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:16 $
 * $Header: /home/hbray/cvsroot/fec/lib/mpms/data.c,v 1.2 2011/07/27 20:22:16 hbray Exp $
 *
 $Log: data.c,v $
 Revision 1.2  2011/07/27 20:22:16  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:43  hbray
 Added cvs headers

 *
$History: $
 * 
*****************************************************************************/

#ident "@(#) $Id: data.c,v 1.2 2011/07/27 20:22:16 hbray Exp $ "

// Application plugin data/method header declarations
#define C_DATA
#include "data.h"
#include "include/sdcdes.h"

// Application plugin release version, and connect status
PiLibSpecs_t PluginVersion = {

// Application plugin release version, and connect status data
#ifndef PLUGINNAME
	"libmpms.so",
#else
	PLUGINNAME,
#endif

#ifndef VERSION
	"1.4",						// Default version value
#else
	VERSION,					// Value defined in data.h
#endif

#ifndef PERSISTENT
	false,						// Default persistence value
#else
	PERSISTENT					// Value defined in data.h
#endif
};

// The following is required by the plugin loader.  DO NOT ALTER!
// This array must match the positional ordere of the libsym_t definition
char *(PluginMethods[]) =
{								// plugin method textual name
	"Load",						// plugin app object initializer
		"Unload",				// plugin app object de-initializer
		"BeginSession",			// plugin app called at start of new session
		"EndSession",			// plugin app called at end of session
		"ReadRequest",			// plugin app read POS request
		"SendResponse",			// plugin app write POS response
		"ToString",				// plugin app ToString function
		0						// Array must be NULLP terminated
};


int
PMS2WaySendPos(PiSession_t *sess, char *msgType, char *bfr, int len)
{

	PiSessionVerify(sess);
	appData_t *context = (appData_t*)sess->pub.context->data;

	int rsplen = len + sizeof(Pms2WayMsgHeader_t);
	Pms2WayMsg_t *rsp = alloca(rsplen+128);
	if (rsp == NULL)
	{
		PiException(LogError, "alloca", eFailure, "failed");
		return -1;
	}
	memset(rsp, 0, rsplen+128);


	{		// build hdr
		Pms2WayMsgHeader_t hdr;
		memcpy(&hdr, &context->receivedHeader, sizeof(hdr));
		memcpy(hdr.msgType, msgType, sizeof(hdr.msgType));	// overlay the message type
		memcpy(&rsp->hdr, &hdr, sizeof(hdr));
	}

	{
		if (bfr != NULL && len > 0 )
		{
			memcpy(rsp->body, bfr, len);
			if ( strncmp(rsp->hdr.versionNbr, "0004", 4) == 0 )
			{
				if ( (len % 8) != 0 )
					len += (8-(len %8));
				PMS2WaySEncrypt(sess, rsp->body, len);
				rsplen = len + sizeof(Pms2WayMsgHeader_t);
			}
		}

		char dataLen[32];
		sprintf(dataLen, "%04d", len);
		memcpy(rsp->hdr.dataLen, dataLen, sizeof(rsp->hdr.dataLen));	// overlay the datalen
	}

	if (PiPosSend(sess, (char*)rsp, rsplen) != rsplen )
	{
		PiException(LogError, "PiPosSend", eFailure, "failed");
		return eFailure;
	}

	return len;
}


int 
PMS2WaySDecrypt(PiSession_t * sess, char *eblock, int esize) 
{
	desecb_t enc;

	PiSessionVerify(sess);

	if (esize <= 0)
	{
		PiException(LogError, __FUNC__, eFailure, "%d are too few bytes to decrypt", esize);
		return -1;
	}
	
	// Prime cipher
	char key[22] = { 0x2c, 0x7b, 0xf2, 0xce, 0x7b, 0x59, 0x19, 0xdf, 0x52, 0xe0, 0xff, 0x0c, 0xab, 0xb6, 0x20, 0x8e, 0x83, 0x1d, 0x59, 0xfc, 0xce, 0xf2 };
	if (desecbStart(&enc, key, 22))
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


int 
PMS2WaySEncrypt(PiSession_t * sess, char *eblock, int esize) 
{
	desecb_t enc;

	PiSessionVerify(sess);

	if (esize <= 0)
	{
		PiException(LogError, __FUNC__, eFailure, "%d are too few bytes to decrypt", esize);
		return -1;
	}
	
	// Prime cipher
	char key[22] = { 0x2c, 0x7b, 0xf2, 0xce, 0x7b, 0x59, 0x19, 0xdf, 0x52, 0xe0, 0xff, 0x0c, 0xab, 0xb6, 0x20, 0x8e, 0x83, 0x1d, 0x59, 0xfc, 0xce, 0xf2 };
	if (desecbStart(&enc, key, 22))
	{
		PiException(LogError, "desecbStart", eFailure, "failed; errno=%d", errno);
		return -1;
	}

	int elen;
	char *ct = desecbEncrypt(enc, eblock, esize, &elen);
	if (ct != NULL)
	{
		memcpy(eblock, ct, elen);
		free(ct);
	}
	desecbEnd(&enc);
	return 0;
}


