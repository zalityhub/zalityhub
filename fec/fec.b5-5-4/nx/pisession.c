/*****************************************************************************

Filename:   lib/nx/pisession.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:58 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/pisession.c,v 1.3.4.14 2011/10/27 18:33:58 hbray Exp $
 *
 $Log: pisession.c,v $
 Revision 1.3.4.14  2011/10/27 18:33:58  hbray
 Revision 5.5

 Revision 1.3.4.13  2011/09/24 17:49:46  hbray
 Revision 5.5

 Revision 1.3.4.11  2011/09/02 14:17:03  hbray
 Revision 5.5

 Revision 1.3.4.10  2011/09/01 14:49:46  hbray
 Revision 5.5

 Revision 1.3.4.8  2011/08/25 18:19:44  hbray
 *** empty log message ***

 Revision 1.3.4.7  2011/08/23 19:53:59  hbray
 eliminate fecplugin.h

 Revision 1.3.4.6  2011/08/23 12:03:14  hbray
 revision 5.5

 Revision 1.3.4.4  2011/08/17 17:58:58  hbray
 *** empty log message ***

 Revision 1.3.4.3  2011/08/15 19:12:32  hbray
 5.5 revisions

 Revision 1.3.4.2  2011/08/11 19:47:33  hbray
 Many changes

 Revision 1.3.4.1  2011/08/01 16:11:29  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:47  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: pisession.c,v 1.3.4.14 2011/10/27 18:33:58 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/filefifo.h"
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"
#include "include/sql.h"


static int SessionClear(PiSession_t *this, boolean eof);
static void DeleteSessionDir(PiSession_t *this);
static void WriteExceptions(PiSession_t *this);
static int _PiHostSend(PiSession_t *this, ProxyFlowType_t flowType, int replyTTL, HostRequest_t *hostReq, int servicePort);


#define PiContextNew(contextLen) ObjectNew(PiContext, contextLen)
#define PiContextVerify(var) ObjectVerify(PiContext, var)
#define PiContextDelete(var) ObjectDelete(PiContext, var)
BtNode_t* PiContextNodeList = NULL;
static PiContext_t* PiContextConstructor(PiContext_t *this, char *file, int lno, int contextLen);
static void PiContextDestructor(PiContext_t *this, char *file, int lno);


#if 0
static void
PiSessionInit()
{
	char *stmt = "CREATE TABLE sessionaudit (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, datecreated DATETIME, sid TEXT, event TEXT, disposition TEXT, xml TEXT)";
	if ( NxSqlExecute(stmt) != 0 )
		SysLog(LogWarn, "Unable to create table %s", stmt);		// assume this is ok... table already present
}
#endif


BtNode_t* PiSessionNodeList = NULL;


void
PiSessionSetLogPrefix(PiSession_t *this)
{
	PiSessionVerify(this);
	sprintf(this->_priv.logPrefix, "%s.%d.%s", this->pub.service.properties.protocol, this->pub.service.properties.port, NxClientUidToString(this->_priv.client));
	SysLogPushPrefix(this->_priv.logPrefix);
}


PiSession_t*
PiSessionConstructor(PiSession_t *this, char *file, int lno, FecService_t *service, NxClient_t *client)
{
	{
		static boolean needInit = true;
		if (needInit)
			//PiSessionInit();
		needInit = false;
	}

	NxClientVerify(client);
	this->_priv.client = client;

	this->_priv.connectTime = GetMsTime();

	this->_priv.inputFifo = FifoNew("");

	this->_priv.t70Queue = ProxyQueueNew();

	this->_priv.authQueue = ProxyQueueNew();

	this->_priv.edcQueue = ProxyQueueNew();

	this->_priv.auditList = ObjectListNew(ObjectListVarType, "AuditList");

	this->_priv.plugin = &service->plugin;

// Make a local copy of the service
	memcpy(&this->pub.service, service, sizeof(FecService_t));

	PiSessionSetLogPrefix(this);

	this->_priv.finalDisposition = eOk;

	this->_priv.contextList = HashMapNew(128, "ContextList");
	
	this->_priv.batchList = HashMapNew(128, "BatchList");

// set Reply TTL
	char *replyTTL = PiGetPropertyValue(this, "ReplyTTL");
	if ( replyTTL == NULL || strlen(replyTTL) <= 0 )
		replyTTL = ProcGetPropertyValue(NxCurrentProc, "ReplyTTL");
	this->pub.replyTTL = atoi(replyTTL);

	return this;
}


void
PiSessionDestructor(PiSession_t *this, char *line, int lno)
{
	PiSessionVerify(this);

	if ( SessionClear(this, PiSessionAtEof(this)) != 0 && this->_priv.finalDisposition == eOk )	// clear all buffers
		this->_priv.finalDisposition = eWarn;

	if (this->_priv.inputFifo != NULL)
		FifoDelete(this->_priv.inputFifo);

	if (this->_priv.t70Queue != NULL)
		ProxyQueueDelete(this->_priv.t70Queue);

	if (this->_priv.authQueue != NULL)
		ProxyQueueDelete(this->_priv.authQueue);

	if (this->_priv.edcQueue != NULL)
		ProxyQueueDelete(this->_priv.edcQueue);

	if ( (this->_priv.auditList != NULL && this->_priv.finalDisposition < 0) &&	// need to write exceptions
		(this->_priv.finalDisposition != eDisconnect) )	// and not for disconnects
	{
		PiAuditSendEvent(AuditPiException, this->_priv.finalDisposition, "sessionid", PiSessionGetName(this), "connid", PiSessionGetName(this), "result", PiApiResultToString(this->_priv.finalDisposition));
		WriteExceptions(this);
		SysLogFull(LogError, line, lno, __FUNC__, "final session disposition: %s", PiApiResultToString(this->_priv.finalDisposition));
	}

	if (this->_priv.auditList != NULL)
		ObjectListDelete(this->_priv.auditList);

// delete all context entries

	for (HashEntry_t *entry; (entry = HashGetNextEntry(this->_priv.contextList, NULL)) != NULL;)
	{
		PiContext_t *context = (PiContext_t*)entry->var;
		PiDeleteContext(this, context);
	}

	HashClear(this->_priv.contextList, false);
	HashMapDelete(this->_priv.contextList);

	HashClear(this->_priv.batchList, false);
	HashMapDelete(this->_priv.batchList);

	DeleteSessionDir(this);
}


char*
PiGetPropertyValue(PiSession_t *this, char *propName, ...)
{
	PiSessionVerify(this);
	char *protocol = this->pub.service.properties.protocol;
	char *prop = NxGetPropertyValue("Service_%s.%s", protocol, propName);
	return prop;
}


char*
PiCreateSessionDir(PiSession_t *this)
{
	PiSessionVerify(this);

	DeleteSessionDir(this);

	char tmp[1024];
	sprintf(tmp, "%s/sessions/%s", NxCurrentProc->procDir, PiSessionGetName(this));
// Create proc session directory
	if (MkDirPath(tmp) != 0)
		NxCrash("Unable to create %s", tmp);
	this->pub.sessionDir = strdup(tmp);
	return this->pub.sessionDir;
}


static void
DeleteSessionDir(PiSession_t *this)
{
	PiSessionVerify(this);

	if ( this->pub.sessionDir == NULL )
		return;		// none

	if ( RmDirPath(this->pub.sessionDir) != 0 )
		SysLog(LogError, "Unable to delete %s; error %s", this->pub.sessionDir, ErrnoToString(errno));
	free(this->pub.sessionDir);
	this->pub.sessionDir = NULL;
}


static void
WriteExceptions(PiSession_t *this)
{
	PiSessionVerify(this);

#if 0
	for(ObjectLink_t *link = ObjectListFirst(this->_priv.auditList); link != NULL; link = ObjectListNext(link))
	{
		AuditXml_t *auditXml = (AuditXml_t*)link->var;
		char *xml = AuditXmlToString(auditXml);
		if ( NxSqlExecute("INSERT INTO sessionaudit (datecreated,sid,event,disposition,xml) VALUES('%s','%s','%s','%s','%s')",
			TimeToStringShort(auditXml->dateCreated, NULL), PiSessionGetName(this), AuditEventToString(auditXml->event), PiApiResultToString(auditXml->disposition), xml) != 0 )
		{
			SysLog(LogWarn, "Unable to insert into sessionaudit: %s", stmt);		// probably locked...
			break;
		}
		char *xml = AuditXmlToString(auditXml);
		int xlen = strlen(xml);
		AuditSendEventData(AuditSessionError, xml, xlen, "time", TimeToStringShort(auditXml->dateCreated, NULL), "sid", PiSessionGetName(this));
	}

	// Then create a final AuditSessionError record
	{
		char *stmt = alloca(1024);
		sprintf(stmt,
			"INSERT INTO sessionaudit (datecreated,sid,event,disposition) VALUES('%s','%s','%s','%s')",
			TimeToStringShort(time(NULL), NULL), PiSessionGetName(this), AuditEventToString(AuditSessionError), PiApiResultToString(eFailure));
		// if ( NxSqlExecute(stmt) != 0 )
			SysLog(LogWarn, "Unable to insert into sessionaudit: %s", stmt);		// probably locked...
	}
#endif
}


static int
SessionClear(PiSession_t *this, boolean eof)
{
	int result = 0;

	PiSessionVerify(this);

	SysLog(LogDebug, "Clearing buffers");

	if (this->_priv.inputFifo->len > 0)
	{
		SysLog(LogDebug | SubLogDump, this->_priv.inputFifo->data, this->_priv.inputFifo->len, "discarding %d input characters", this->_priv.inputFifo->len);
		// PiException(LogWarn, (char*)__FUNC__, (this->_priv.finalDisposition != eOk)?this->_priv.finalDisposition:eWarn, "discarding %d input characters", this->_priv.inputFifo->len);
		// result = 1;
	}

	FifoClear(this->_priv.inputFifo);

// close any open batches
	for (HashEntry_t *entry; (entry = HashGetNextEntry(this->_priv.batchList, NULL)) != NULL;)
	{
		FileFifo_t *batch = (FileFifo_t*)entry->var;
		HashDeleteUid(this->_priv.batchList, batch->uid);
		FileFifoDelete(batch);
	}

// clear proxy queues
	ProxyQueueClear(this->_priv.t70Queue);
	ProxyQueueClear(this->_priv.authQueue);
	ProxyQueueClear(this->_priv.edcQueue);

	PiSessionSetEof(this, eof);

	return result;
}


int
PiPosPoke(PiSession_t *this, char *bfr, int len)
{
	PiSessionVerify(this);
	return FifoWrite(this->_priv.inputFifo, bfr, len);
}


/*
	If at EOF (disconnect), return TRUE.
	If error, return -1.
	Otherwise, return FALSE.
*/
boolean
PiPosPeek(PiSession_t *this, PiPeekBfr_t *peekbfr)
{
	PiSessionVerify(this);

	if (peekbfr == NULL)
	{
		SysLog(LogError, "No PiPeekBfr_t given");
		return -1;
	}

	memset(peekbfr, 0, sizeof(PiPeekBfr_t));
	peekbfr->len = this->_priv.inputFifo->len;
	peekbfr->bfr = this->_priv.inputFifo->data;

	if (peekbfr->len <= 0)
		peekbfr->atEof = PiSessionAtEof(this);	// if no data; then copy eof

	SysLog(LogDebug, "PosPeek len=%d, atEof=%s", peekbfr->len, peekbfr->atEof ? "true" : "false");

	return peekbfr->atEof;
}


int
PiPosRecv(PiSession_t *this, char *bfr, int len)
{
	PiSessionVerify(this);

	int rr = FifoRead(this->_priv.inputFifo, bfr, len);

	PiAuditSendEventData(AuditPiPosRecv, rr<0?eFailure:eOk, bfr, rr, "sessionid", PiSessionGetName(this), "connid", PiSessionGetName(this), "%d.len", (char *)&rr);

	if ( rr > 0 )
	{
		++this->_priv.plugin->posCounts.inPkts;		// Inbound pkt count
		this->_priv.plugin->posCounts.inChrs += rr;	// Inbound chr count
	}
	return rr;
}


int
PiPosUrlRecv(PiSession_t *this, char *bfr, int len)
{
	PiSessionVerify(this);

	int rr = FifoUrlRead(this->_priv.inputFifo, bfr, len);

	PiAuditSendEvent(AuditPiPosRecv, rr<0?eFailure:eOk, "sessionid", PiSessionGetName(this), "connid", PiSessionGetName(this), "%d.len", (char *)&rr);

	if ( rr > 0 )
	{
		++this->_priv.plugin->posCounts.inPkts;		// Inbound pkt count
		this->_priv.plugin->posCounts.inChrs += rr;	// Inbound chr count
	}
	return rr;
}


int
PiPosEntityRecv(PiSession_t *this, char *bfr, int len)
{
	PiSessionVerify(this);

	int rr = FifoEntityRead(this->_priv.inputFifo, bfr, len);

	PiAuditSendEvent(AuditPiPosRecv, rr<0?eFailure:eOk, "sessionid", PiSessionGetName(this), "connid", PiSessionGetName(this), "%d.len", (char *)&rr);

	if ( rr > 0 )
	{
		++this->_priv.plugin->posCounts.inPkts;		// Inbound pkt count
		this->_priv.plugin->posCounts.inChrs += rr;	// Inbound chr count
	}
	return rr;
}


int
PiPosSend(PiSession_t *this, char *bfr, int len)
{
	PiSessionVerify(this);

	NxClient_t *client = this->_priv.client;
	NxClientVerify(client);

	int rr = NxClientSendRaw(client, (char *)bfr, len);

	if (rr != len)
		SysLog(LogError, "Attempted to write %d; only %d written", len, rr);

	SysLog(LogDebug | SubLogDump, bfr, rr, "%s: pos=%s; len=%d", PiSessionGetName(this), PiSessionGetName(this), rr);

	PiAuditSendEvent(AuditPiPosSend, (rr != len)?eFailure:eOk, "sessionid", PiSessionGetName(this), "connid", PiSessionGetName(this), "%d.len", (char *)&rr);

	PiAuditSendEventData(AuditPosSend, (rr != len)?eFailure:eOk, bfr, rr, "sessionid", PiSessionGetName(this), "connid", PiSessionGetName(this), "%d.len", (char *)&rr);

	if ( rr > 0 )
	{
		++this->_priv.plugin->posCounts.outPkts;		// Outbound pkt count
		this->_priv.plugin->posCounts.outChrs += rr;	// Outbound chr count
	}
	return rr;
}


int
PiHostRecv(PiSession_t *this, HostRequest_t *hostReq)
{
	PiSessionVerify(this);

	memset(hostReq, 0, sizeof(*hostReq));

	ProxyRequest_t *proxyReq = (ProxyRequest_t *)ObjectListRemove(this->_priv.authQueue->inputQueue, ObjectListFirstPosition);
	if (proxyReq == NULL)		// no request
		proxyReq = (ProxyRequest_t *)ObjectListRemove(this->_priv.edcQueue->inputQueue, ObjectListFirstPosition);
	if (proxyReq == NULL)		// no request
		proxyReq = (ProxyRequest_t *)ObjectListRemove(this->_priv.t70Queue->inputQueue, ObjectListFirstPosition);

	if (proxyReq == NULL)		// no request
	{
		SysLog(LogError, "NULL response");
		return 0;
	}

	SysLog(LogDebug, "Recv HostRequest %s", ProxyRequestToString(proxyReq, DumpOutput));

	memcpy(hostReq, &proxyReq->hostReq, sizeof(proxyReq->hostReq));	// copy it
	ProxyRequestDelete(proxyReq);

	if (hostReq->len > sizeof(hostReq->data))
	{
		SysLog(LogError, "HostRequest_t->hostReq.len of %d exceeds max allowable size %d; Truncating", hostReq->len, sizeof(hostReq->data));
		hostReq->len = sizeof(hostReq->data);
	}

	PiAuditSendEventData(AuditPiHostRecv, eOk, hostReq->data, hostReq->len, "sessionid", PiSessionGetName(this), "connid", PiSessionGetName(this), "req", HostRequestToString(hostReq, NoOutput));

	{
		++this->_priv.plugin->hostCounts.inPkts;		// Inbound pkt count
		this->_priv.plugin->hostCounts.inChrs += hostReq->len;	// Inbound chr count
	}
	return hostReq->len;
}


int
PiHostSendServicePort(PiSession_t *this, ProxyFlowType_t flowType, int replyTTL, HostRequest_t *hostReq, int servicePort)
{
	PiSessionVerify(this);
	return _PiHostSend(this, flowType, replyTTL, hostReq, servicePort);
}


int
PiHostSend(PiSession_t *this, ProxyFlowType_t flowType, int replyTTL, HostRequest_t *hostReq)
{
	PiSessionVerify(this);
	NxClient_t *client = this->_priv.client;
	NxClientVerify(client);
	return _PiHostSend(this, flowType, replyTTL, hostReq, client->evf->servicePort);
}


static int
_PiHostSend(PiSession_t *this, ProxyFlowType_t flowType, int replyTTL, HostRequest_t *hostReq, int servicePort)
{
	PiSessionVerify(this);
	NxClient_t *client = this->_priv.client;
	NxClientVerify(client);

// Verify the request type; and choose the outbound queue

	ProxyQueue_t *proxyQueue;
	switch(hostReq->hdr.svcType)
	{
		default:
			SysLog(LogError, "SvcType=%s is not valid", HostSvcTypeToString(hostReq->hdr.svcType));
			return -1;
			break;

		case eSvcConfig:
			proxyQueue = this->_priv.authQueue;
			break;

		case eSvcAuth:
			proxyQueue = this->_priv.authQueue;
			break;

		case eSvcEdc:
			proxyQueue = this->_priv.edcQueue;
			break;

		case eSvcEdcMulti:
			proxyQueue = this->_priv.edcQueue;
			break;

		case eSvcT70:
			proxyQueue = this->_priv.t70Queue;
			break;
	}

// Verify the length
	if (hostReq->len <= 0 )
	{
		SysLog(LogError, "hostReq->len of %d is not valid", hostReq->len);
		return -1;
	}

	hostReq->hdr.peerUid = NxClientGetUid(client);
	memcpy(hostReq->hdr.peerName, PiSessionGetName(this), sizeof(hostReq->hdr.peerName));
	if (ProxyQueueAddOutputRequest(proxyQueue, flowType, replyTTL, hostReq, client, servicePort) < 0)
	{
		SysLog(LogError, "ProxyQueueAddOutputRequest of ProxyRequest_t failed");
		return -1;
	}

	PiAuditSendEventData(AuditPiHostSend, eOk, hostReq->data, hostReq->len, "sessionid", PiSessionGetName(this), "connid", PiSessionGetName(this), "req", HostRequestToString(hostReq, NoOutput));

	{
		++this->_priv.plugin->hostCounts.outPkts;		// Inbound pkt count
		this->_priv.plugin->hostCounts.outChrs += hostReq->len;	// Inbound chr count
	}
	return hostReq->len;
}


PiContext_t*
PiCreateContext(PiSession_t *this, int contextLen)
{
	PiSessionVerify(this);

	PiContext_t *context = PiContextNew(contextLen);

	context->sess = this;

	if (HashAddUid(this->_priv.contextList, context->uid, context) == NULL)
		SysLog(LogFatal, "HashAddUid of %s failed", NxUidToString(context->uid));

	return context;
}


int
PiDeleteContext(PiSession_t *this, PiContext_t *context)
{
	PiSessionVerify(this);
	PiContextVerify(context);

	HashEntry_t *entry = HashFindUid(this->_priv.contextList, context->uid);
	if ( entry == NULL )
	{
		SysLog(LogError, "Unable to locate context %s", NxUidToString(context->uid));
		return -1;
	}

	if (HashDeleteUid(this->_priv.contextList, context->uid) != 0)
	{
		SysLog(LogError, "HashDeleteUid of %s failed", NxUidToString(context->uid));
		return -1;
	}

	PiContextDelete(context);
	return 0;
}


PiContext_t*
PiGetContext(PiSession_t *this, NxUid_t uid)
{
	PiSessionVerify(this);

	HashEntry_t *entry = HashFindUid(this->_priv.contextList, uid);
	if ( entry == NULL )
	{
		SysLog(LogError, "Unable to locate context %s", NxUidToString(uid));
		return NULL;
	}

	return entry->var;
}


static PiContext_t*
PiContextConstructor(PiContext_t *this, char *file, int lno, int contextLen)
{
	if ( (this->dataLen = contextLen) > 0 )
	{
		if ( (this->data = calloc(1, contextLen)) == NULL )
			SysLogFull(LogFatal, file, lno, __FUNC__, "Unable to allocate context datalen of %d", contextLen);
	}

	this->uid = NxUidNext();
	return this;
}


static void
PiContextDestructor(PiContext_t *this, char *file, int lno)
{
	if ( this->dataLen > 0 && this->data != NULL )
		free(this->data);	// this is the pointer to the batch context pointer
}


Json_t*
PiContextSerialize(PiContext_t *this)
{
	PiContextVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Name", PiSessionGetName(this->sess));
	JsonAddString(root, "Uid", NxUidToString(this->uid));
	return root;
}


char*
PiContextToString(PiContext_t *this)
{
	Json_t *root = PiContextSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


PiApiResult_t
PiBeginSession(PiSession_t *sess)
{
	PiSessionVerify(sess);

	if ( SessionClear(sess, false) != 0 )	// clear all buffers
		sess->_priv.finalDisposition = eWarn;

	PiAuditSendEvent(AuditPiApiCall, eOk, "api", "BeginSession", "sessionid", PiSessionGetName(sess), "connid", PiSessionGetName(sess), "libname", sess->_priv.plugin->libSpecs.libname, "libversion", sess->_priv.plugin->libSpecs.version);
	PiApiResult_t result = PiCallMethod(sess, BeginSession);

	SysLog(LogDebug, "BeginSession returned result %s", PiApiResultToString(result));

	if ( (int)result < 0 )
		PiException(LogWarn, "BeginSession", result, "failed");

	return result;
}


PiApiResult_t
PiEndSession(PiSession_t *sess)
{
	PiSessionVerify(sess);

	if ( SessionClear(sess, PiSessionAtEof(sess)) != 0 && sess->_priv.finalDisposition == eOk )	// clear all buffers
		sess->_priv.finalDisposition = eWarn;

	if (PiGetService(sess) == NULL)
		return eOk;				// no service; that's ok

	PiAuditSendEvent(AuditPiApiCall, eOk, "api", "EndSession", "sessionid", PiSessionGetName(sess), "connid", PiSessionGetName(sess));
	PiApiResult_t result = PiCallMethod(sess, EndSession);

	SysLog(LogDebug, "EndSession returned result %s", PiApiResultToString(result));

	if ( (int)result < 0 )
		PiException(LogWarn, "EndSession", result, "failed");

	return result;
}


PiApiResult_t
PiReadRequest(PiSession_t *sess)
{
	PiSessionVerify(sess);

	PiPeekBfr_t peek;
	PiPosPeek(sess, &peek);
	SysLog(LogDebug, "Calling ReadRequest: fifo len=%d, eof=%s", peek.len, peek.atEof?"true" : "false");

	PiAuditSendEvent(AuditPiApiCall, eOk, "api", "ReadRequest", "sessionid", PiSessionGetName(sess), "connid", PiSessionGetName(sess), "eof", (peek.atEof) ? "true" : "false", "%d.len", (char *)&peek.len);
	PiApiResult_t result = PiCallMethod(sess, ReadRequest);

	SysLog(LogDebug, "ReadRequest returned result %s", PiApiResultToString(result));

	if ((int)result < 0 && result != eDisconnect)
		PiException(LogWarn, "PiReadRequest", result, "failed");

	return result;
}


PiApiResult_t
PiSendResponse(PiSession_t *sess)
{
	PiSessionVerify(sess);

	PiAuditSendEvent(AuditPiApiCall, eOk, "api", "SendResponse", "sessionid", PiSessionGetName(sess), "connid", PiSessionGetName(sess));
	PiApiResult_t result = PiCallMethod(sess, SendResponse);

	SysLog(LogDebug, "SendResponse returned result %s", PiApiResultToString(result));

	if ((int)result < 0 && result != eDisconnect)
		PiException(LogWarn, "PiSendResponse", result, "failed");

	return result;
}


int
_PiAuditSendEvent(PiSession_t *this, Audit_t *audit, AuditEvent_t event, char *file, int lno, char *fnc, PiApiResult_t disposition, void *data, int len, void **nv, int npairs)
{
	AuditVerify(audit);
	PiSessionVerify(this);

	AuditXml_t *auditXml = _AuditFormatEvent(audit, event, file, lno, fnc, data, len, nv, npairs);

	if ( event == AuditPiException )
		SysLogFull(LogError, file, lno, fnc, "AuditPiException: %s", AuditXmlToString(auditXml));

	auditXml->disposition = disposition;

	// remove previous count (used one to get here)...
	//it will be reincremented in the next AuditIsEnabled
	AuditCountDec(event);

	if ( AuditIsEnabled(event) )
		_AuditSendXml(audit, auditXml);

	if ( NxGlobal->auditSessions )
		ObjectListAdd(this->_priv.auditList, auditXml, ObjectListLastPosition);
	else
		AuditXmlDelete(auditXml);

	return 0;
}


NxUid_t
PiBatchOpen(PiSession_t *this, int contextLen)
{
	PiSessionVerify(this);

	FileFifo_t *batch = FileFifoNew(contextLen);		// new batch fifo
	if ( HashAddUid(this->_priv.batchList, batch->uid, batch) == NULL )
		SysLog(LogFatal, "HashAddUid of %s failed", NxUidToString(batch->uid));

	SysLog(LogDebug, "Created batch %s", NxUidToString(batch->uid));
	return batch->uid;
}


static FileFifo_t*
_PiBatchGetFifo(PiSession_t *this, NxUid_t batchId)
{
	PiSessionVerify(this);

	HashEntry_t *entry = HashFindUid(this->_priv.batchList, batchId);
	if ( entry == NULL )
	{
		SysLog(LogError, "Unable to locate batch %s", NxUidToString(batchId));
		return NULL;
	}

	FileFifo_t *batch = (FileFifo_t*)entry->var;
	return batch;
}


void*
PiBatchGetContext(PiSession_t *this, NxUid_t batchId)
{
	FileFifo_t *batch = _PiBatchGetFifo(this, batchId);
	if ( batch == NULL )
		return NULL;
	return batch->context;
}


int
PiBatchPush(PiSession_t *this, char *bfr, int len, NxUid_t batchId)
{
	PiSessionVerify(this);
		
	FileFifo_t *batch = _PiBatchGetFifo(this, batchId);
	if ( batch == NULL )
		return -1;

	SysLog(LogDebug, "Writing %d bytes to batch %s", len, NxUidToString(batchId));
	if ( FileFifoWrite(batch, bfr, len, SubLogDump) != len )
	{
		SysLog(LogError, "Unable to write batch %s", NxUidToString(batchId));
		return -1;
	}
	return len;
}


unsigned int
PiBatchSize(PiSession_t *this, NxUid_t batchId)
{
	PiSessionVerify(this);

	FileFifo_t *batch = _PiBatchGetFifo(this, batchId);
	if ( batch == NULL )
		return -1;

	return FileFifoGetSize(batch);
}


int
PiBatchPosSend(PiSession_t *this, NxUid_t batchId)
{
	PiSessionVerify(this);

	SysLog(LogDebug, "Sending batch %s to %s", NxUidToString(batchId), PiSessionToString(this));

	FileFifo_t *batch = _PiBatchGetFifo(this, batchId);
	if ( batch == NULL )
		return -1;

// send the batched data...
	int ret = 0;
	for(;;)
	{
		char bfr[8192];
		unsigned int len = FileFifoGetSize(batch);
		if ( len == 0 )
			break;		// done

		if(len > sizeof(bfr))
			len = sizeof(bfr);		// max pkt size
		int tlen;
		SysLog(LogDebug, "Reading %d bytes from batch %s", len, NxUidToString(batchId));
		if ( (tlen = FileFifoRead(batch, bfr, len, SubLogDump)) != len )
		{
			SysLog(LogError, "Batch %s failed; expected to read %d; only got %d", NxUidToString(batchId), len, tlen);
			ret = -1;
			break;
		}
		SysLog(LogDebug, "Writing %d bytes to %s for batch %s", len, PiSessionGetName(this), NxUidToString(batchId));
		if( (tlen = PiPosSend(this, bfr, len)) != len )
		{
			SysLog(LogError, "Batch %s failed; expected to send %d; only sent %d", NxUidToString(batchId), len, tlen);
			ret = -1;
			break;
		}
	}

	return ret;
}


int
PiBatchClose(PiSession_t *this, NxUid_t batchId)
{
	PiSessionVerify(this);

	SysLog(LogDebug, "Closing batch %s", NxUidToString(batchId));

	FileFifo_t *batch = _PiBatchGetFifo(this, batchId);
	if ( batch == NULL )
		return -1;

	HashDeleteUid(this->_priv.batchList, batch->uid);
	FileFifoDelete(batch);
	return 0;
}


Json_t*
PiBatchSerialize(PiSession_t *this, NxUid_t batchId)
{
	FileFifo_t *batch = _PiBatchGetFifo(this, batchId);
	if ( batch == NULL )
	{
		Json_t *root = JsonNew(__FUNC__);
		JsonAddString(root, "Batch", "No batch %s in %s", NxUidToString(batchId), PiSessionToString(this));
		return root;
	}

	return FileFifoSerialize(batch);
}


char*
PiBatchToString(PiSession_t *this, NxUid_t batchId)
{
	Json_t *root = PiBatchSerialize(this, batchId);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


void
_PiException(PiSession_t *this, SysLogLevel lvl, char *api, PiApiResult_t disposition, boolean doLog, char *file, int lno, char *fnc, char *fmt, ...)
{
	static int recursion = 0;

	if ( recursion > 0 )
		return;		// done

	++recursion;

	PiSessionVerify(this);

	if ( this->_priv.finalDisposition == eOk )		// only change final disp if it previously "OK"
		this->_priv.finalDisposition = disposition;

	StringArrayStatic(sa, 16, 32);
	String_t *text = StringArrayNext(sa);
	va_list ap;
	va_start(ap, fmt);
	StringSprintfV(text, fmt, ap);

	if ( PiAuditIsEnabled(AuditPiException) )
	{
		AuditCountDec(AuditPiException);	// remove count.. PiAuditSendEventFull below will recount...
		PiAuditSendEventFull(AuditPiException, file, lno, fnc, disposition, NULL, 0, "sessionid", PiSessionGetName(this), "connid", PiSessionGetName(this), "api", api, "error", EncodeUrlCharacters(text->str, strlen(text->str)), "disposition", PiApiResultToString(disposition));
	}
	else
	{
		if ( doLog )
			SysLogFull(lvl, file, lno, fnc, "sessionid=%s, connid=%s, api=%s, disposition=%s, error=%s", PiSessionGetName(this), PiSessionGetName(this), api, PiApiResultToString(disposition), text->str);
	}
	
	--recursion;
}


Json_t*
PiProfileSerialize(PiProfile_t *this)
{
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Name", this->libSpecs.libname);
	JsonAddBoolean(root, "Loaded", this->loaded);
	JsonAddBoolean(root, "Virtual", this->isVirtual);
	if ( this->loaded )
	{
		JsonAddString(root, "Version", this->libSpecs.version);
		JsonAddBoolean(root, "Persistent", this->libSpecs.persistent);
	}
	return root;
}


char*
PiProfileToString(PiProfile_t *this)
{
	Json_t *root = PiProfileSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


char*
PiApiResultToString(PiApiResult_t result)
{
	StringArrayStatic(sa, 16, 32);
	char *text;

	switch (result)
	{
		default:
			text = StringArrayNext(sa)->str;
			sprintf(text, "Result_%d", (int)result);
			break;

		EnumToString(eFailure);	// Abnormal (FATAL) event
		EnumToString(eWarn);
		EnumToString(eDisconnect);
		EnumToString(eOk);
		EnumToString(eWaitForData);
		EnumToString(eVirtual);
	}

	return text;
}


Json_t*
PiSessionSerialize(PiSession_t *this)
{
	PiSessionVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Name", PiSessionGetName(this));
	JsonAddString(root, "Uid", NxClientUidToString(this->_priv.client));
	JsonAddString(root, "LogPrefix", this->_priv.logPrefix);
	JsonAddString(root, "SessionDir", NullToValue(this->pub.sessionDir, ""));
	JsonAddString(root, "ConnectTime", this->_priv.connectTime ? MsTimeToStringShort(this->_priv.connectTime, NULL) : "Not");
	JsonAddString(root, "PlugIn", PiProfileToString(this->_priv.plugin));

	JsonAddString(root, "T70Queue", ProxyQueueToString(this->_priv.t70Queue));
	JsonAddString(root, "AuthQueue", ProxyQueueToString(this->_priv.authQueue));
	JsonAddString(root, "EdcQueue", ProxyQueueToString(this->_priv.edcQueue));

	JsonAddNumber(root, "ContextAreas", HashMapLength(this->_priv.contextList));

	JsonAddNumber(root, "OpenBatches", HashMapLength(this->_priv.batchList));

	JsonAddNumber(root, "AuditQueue", ObjectListNbrEntries(this->_priv.auditList));

#if TODO
	{	// display the audit entries
		for(ObjectLink_t *link = ObjectListFirst(this->_priv.auditList); link != NULL; link = ObjectListNext(link))
		{
			// AuditXml_t *auditXml = (AuditXml_t*)link->var;
			JsonAddString(root, "%s", AuditXmlToString(auditXml));
		}
	}
#endif

	JsonAddNumber(root, "replyTTL", this->pub.replyTTL);
	JsonAddString(root, "LastPosRecv", this->_priv.lastPosRecvTime ? MsTimeToStringShort(this->_priv.lastPosRecvTime, NULL) : "None");
	JsonAddNumber(root, "NbrPosInQueue", this->_priv.inputFifo->len);
	JsonAddBoolean(root, "PosEof", PiSessionAtEof(this));
	JsonAddString(root, "FinalDisposition", PiApiResultToString(this->_priv.finalDisposition));
	return root;
}


char*
PiSessionToString(PiSession_t *this)
{
	Json_t *root = PiSessionSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}
