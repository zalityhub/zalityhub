/*****************************************************************************

Filename:   main/machine/stratusproxy.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:34:01 $
 * $Header: /home/hbray/cvsroot/fec/main/machine/stratusproxy.c,v 1.3.4.19 2011/10/27 18:34:01 hbray Exp $
 *
 $Log: stratusproxy.c,v $
 Revision 1.3.4.19  2011/10/27 18:34:01  hbray
 Revision 5.5

 Revision 1.3.4.18  2011/09/26 15:52:33  hbray
 Revision 5.5

 Revision 1.2.2.1  2011/07/27 20:19:58  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: stratusproxy.c,v 1.3.4.19 2011/10/27 18:34:01 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"
#include "include/hostio.h"

#include "machine/include/stratusproxy.h"




// Local Data Types

#define Context ((NxCurrentProc)->context)


typedef enum
{
	NullState, WaitForReceiveListenState, WaitForHostListenState, SteadyState
} FsmState;

typedef enum
{
	TimerEvent, HostListenReadyEvent, ReceiveListenReadyEvent, HostReadReadyEvent, ClientReadReadyEvent, ExceptionEvent
} FsmEvent;



typedef struct ProcContext_t
{
	Fsm_t				*fsm;
	NxServer_t			*hostListen;
	Counts_t			hostCounts;

	NxServer_t			*receiveListen;
	HashMap_t			*hostConnections;	// hash of ObjectList_t*

	Timer_t				*currentTimer;
	ProxyContextMap_t	*inflightContextMap;
	ProxyContextMap_t	*persistentFlowMap;
} ProcContext_t;


#define ContextNew() ObjectNew(ProcContext)
#define ContextVerify(var) ObjectVerify(ProcContext, var)
#define ContextDelete(var) ObjectDelete(ProcContext, var)

static ProcContext_t* ProcContextConstructor(ProcContext_t *this, char *file, int lno);
static void ProcContextDestructor(ProcContext_t *this, char *file, int lno);
static BtNode_t* ProcContextNodeList;
static Json_t* ProcContextSerialize(ProcContext_t *this);
static char* ProcContextToString(ProcContext_t *this);


// Static Functions
//
// Event Handlers
//
static void HandleClientEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandleReceiveListenEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandleHostEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandleHostListenEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandleTimerEvent(Timer_t *tid);
static void SigTermHandler(NxSignal_t*);

// Fsm Functions
//
static void FsmEventHandler(Fsm_t *this, int evt, void * efarg);
static char *FsmEventToString(Fsm_t *this, int evt);
static void FsmNullState(Fsm_t *this, FsmEvent evt, void * efarg);
static char *FsmStateToString(Fsm_t *this, int state);
static void FsmSteadyState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmWaitForReceiveListenState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmWaitForHostListenState(Fsm_t *this, FsmEvent evt, void * efarg);

// Helper Functions
//
static int ReceiveListen();
static NxClient_t* ReceiveAccept();
static void ClientDisconnect(NxClient_t *client);
static ProxyRequest_t* ClientRecv(NxClient_t *client);
static int ClientSend(NxClient_t *client, ProxyRequest_t *proxyReq);
static int ClientAllNotifyHostStatus();
static int ClientNotifyHostStatus(NxClient_t *client, ProxyRequest_t *proxyReq);

static int HostListen();
static NxClient_t* HostAccept();
static void HostDisconnect(NxClient_t *host);
static int HostDisconnectAll();
static int HostSend(NxClient_t *from, ProxyRequest_t *proxyReq);
static int HostRecv(NxClient_t *host);
static NxClient_t* HostSelectByService(ProxyRequest_t *proxyReq);
static long HostNbrOnline();

static void Shutdown();
static void TimerSet(int ms);
static void InflightContextCollector(ProxyContextEntry_t *ce);
static void PersistentFlowContextCollector(ProxyContextEntry_t *ce);

// Command Functions
static CommandResult_t TpsCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static CommandResult_t ShowCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static int CommandHandler(NxClient_t *client, Parser_t *parser, String_t *response);



// Static Global Vars
//
static const int InitialWaitInterval = (2 * 1000);
static const int RecoveryWaitInterval = (60 * 1000);




// Functions Start Here


static void
SigTermHandler(NxSignal_t *sig)
{
	Shutdown();
}


static void
HandleHostListenEvent(EventFile_t *evf, EventPollMask_t pollMask, void *farg)
{
	ProcVerify(NxCurrentProc);
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);
	NxServer_t *listen = (NxServer_t*)farg;
	NxServerVerify(listen);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, HostListenReadyEvent, listen);
}


static void
HandleReceiveListenEvent(EventFile_t *evf, EventPollMask_t pollMask, void *farg)
{
	ProcVerify(NxCurrentProc);
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);
	NxServer_t *listen = (NxServer_t*)farg;
	NxServerVerify(listen);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, ReceiveListenReadyEvent, listen);
}


static void
HandleHostEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg)
{
	ProcVerify(NxCurrentProc);
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);
	NxClient_t *client = (NxClient_t*)farg;
	NxClientVerify(client);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, HostReadReadyEvent, client);
}


static void
HandleClientEvent(EventFile_t *evf, EventPollMask_t pollMask, void *farg)
{
	ProcVerify(NxCurrentProc);
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);
	NxClient_t *client = (NxClient_t*)farg;
	NxClientVerify(client);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, ClientReadReadyEvent, client);
}


static void
HandleTimerEvent(Timer_t *tid)
{
	ProcVerify(NxCurrentProc);
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);
	_FsmDeclareEvent(fsm, TimerEvent, tid);
}


static int
ReceiveListen()
{
	char addr[1024];

	sprintf(addr, "%s/%s", NxGlobal->nxDir, ProcGetPropertyValue(NxCurrentProc, "InputQueue"));
	int port = 0;
	if ( NxServerListen(Context->receiveListen, AF_UNIX, SOCK_STREAM, addr, port, EventFileHighPri, EventReadMask, HandleReceiveListenEvent) != 0 )
	{
		SysLog(LogError, "NxServerListen failed: %s.%d", addr, port);
		return -1;
	}

	return 0;
}


static NxClient_t*
ReceiveAccept()
{
	NxClient_t *client = NxServerAccept(Context->receiveListen, EventFileLowPri, EventReadMask, HandleClientEvent);
	if ( client == NULL )
	{
		SysLog(LogError, "NxServerAccept failed");
		return NULL;
	}

	if (ClientNotifyHostStatus(client, NULL) != 0)
	{
		SysLog(LogError, "ClientNotifyHostStatus failed: %s", NxClientToString(client));
		NxClientDelete(client);
		return NULL;
	}
	return client;
}


static void
ClientDisconnect(NxClient_t *client)
{
// delete any context referencing....
	ProxyContextDeleteByVectorValue(Context->inflightContextMap, 1, client);
	ProxyContextDeleteByVectorValue(Context->persistentFlowMap, 1, client);
	NxClientDelete(client);
}


static ProxyRequest_t*
ClientRecv(NxClient_t *client)
{

	ProxyRequest_t *proxyReq = ProxyRequestNew();

	int rlen = NxClientRecvPkt(client, proxyReq, sizeof(*proxyReq));
	if (rlen <= 0)
	{
		SysLog(LogError, "NxClientRecvPkt failed. Probably a disconnect: %s", NxClientToString(client));
		ProxyRequestDelete(proxyReq);
		return NULL;
	}
	if (rlen < (sizeof(*proxyReq) - sizeof(proxyReq->hostReq.data)))
	{
		SysLog(LogError, "Short read of %d; treating as a disconnect: %s", rlen, NxClientToString(client));
		ProxyRequestDelete(proxyReq);
		return NULL;
	}

	if ( proxyReq->reqType == ProxyReqHostMsg )
		SysLogPushPrefix(proxyReq->hostReq.hdr.peerName);

	SysLog(LogDebug, "HostRequest %s", ProxyRequestToString(proxyReq, NoOutput));

	AuditSendEvent(AuditWorkerRecv, "connid", NxClientUidToString(client), "req", ProxyRequestToString(proxyReq, NoOutput));
	return proxyReq;
}


static int
ClientSend(NxClient_t *client, ProxyRequest_t *proxyReq)
{

	proxyReq->pid = getpid();

	int len = ProxyRequestLen(proxyReq);

	SysLog(LogDebug, "HostRequest %s", ProxyRequestToString(proxyReq, NoOutput));

	if (NxClientSendPkt(client, proxyReq, len) != len )
	{
		SysLog(LogError, "NxClientSendPkt failed: %s", NxClientToString(client));
		return -1;
	}

	AuditSendEvent(AuditWorkerSend, "connid", NxClientUidToString(client), "rsp", ProxyRequestToString(proxyReq, NoOutput));

	return 0;
}


static int
ClientNotifyHostStatus(NxClient_t *client, ProxyRequest_t *proxyReq)
{
	ProxyRequest_t tmpr;

	if (proxyReq == NULL)
	{
		proxyReq = &tmpr;
		memset(proxyReq, 0, sizeof(*proxyReq));
	}

	proxyReq->pid = getpid();

	int nbrOnline = HostNbrOnline();
	SysLog(LogDebug, "%d hosts are online", nbrOnline);

	proxyReq->reqType = (nbrOnline>0)?ProxyReqHostOnline:ProxyReqHostOffline;
	if (NxClientSendPkt(client, proxyReq, sizeof(*proxyReq)) != sizeof(*proxyReq))
	{
		SysLog(LogError, "NxClientSendPkt failed: %s", NxClientToString(client));
		return -1;
	}

	return 0;
}


static int
ClientAllNotifyHostStatus()
{

	for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(Context->receiveListen->connectionList, entry)) != NULL;)
	{
		NxClient_t *client = (NxClient_t *)entry->var;

		if (ClientNotifyHostStatus(client, NULL) != 0)
			SysLog(LogError, "ClientNotifyHostStatus failed: %s", NxClientToString(client));
	}

	return 0;
}


static int
HostListen()
{
	char addr[1024];

	sprintf(addr, "%s", ProcGetPropertyValue(NxCurrentProc, "HostAddr"));
	int port = ProcGetPropertyIntValue(NxCurrentProc, "HostPort");
	if ( NxServerListen(Context->hostListen, AF_INET, SOCK_STREAM, addr, port, EventFileHighPri, EventReadMask, HandleHostListenEvent) != 0 )
	{
		SysLog(LogError, "NxServerListen failed: %s.%d", addr, port);
		return -1;
	}

	return 0;
}


static NxClient_t*
HostAccept()
{
	NxClient_t *host = NxServerAccept(Context->hostListen, EventFileLowPri, EventReadMask, HandleHostEvent);
	if ( host == NULL )
	{
		SysLog(LogError, "NxServerAccept failed");
		return NULL;
	}

	if ( ! NxClientIsConnected(host) )
	{
		SysLog(LogError, "NxServerAccept not connected");
		NxClientDelete(host);
		return NULL;
	}

	ObjectList_t *list = (ObjectList_t*)HashFindStringVar(Context->hostConnections, host->evf->peerIpAddrString);
	if ( list == NULL )			// first connect from this ip
	{
		list = ObjectListNew(ObjectListStringType, "HostConnectionList");
		if (HashAddString(Context->hostConnections, host->evf->peerIpAddrString, list) == NULL)
			SysLog(LogFatal, "HashAddString of %s failed", NxClientNameToString(host));
	}
	if ( ObjectListAdd(list, host, ObjectListLastPosition) == NULL )
		SysLog(LogFatal, "ObjectListAdd of %s failed", NxClientNameToString(host));

	SysLog(LogDebug, "Connected to Host(%s)", NxClientNameToString(host));
	return host;
}


static void
HostDisconnect(NxClient_t *host)
{
	NxClientVerify(host);

	SysLog(LogDebug, "Disconnecting Host(%s)", NxClientNameToString(host));

	ProxyContextDeleteByVectorValue(Context->persistentFlowMap, 0, host);

	ObjectList_t *list = (ObjectList_t*)HashFindStringVar(Context->hostConnections, host->evf->peerIpAddrString);
	if ( list == NULL )			// no connect from this ip
	{
		SysLog(LogError, "HashFindStringVar of %s failed", NxClientNameToString(host));
		NxClientDelete(host);
		return;
	}

// find this specific connection; then ditch it...
	for(ObjectLink_t *link = ObjectListFirst(list); link != NULL; link = ObjectListNext(link) )
	{
		if ( strcmp(NxClientNameToString(link->var), NxClientNameToString(host)) == 0 )		// this is the one
		{
			ObjectListYank(list, link);
			break;
		}
	}
	if ( list->length <= 0 )
	{
		HashDeleteString(Context->hostConnections, host->evf->peerIpAddrString);
		ObjectListDelete(list);
	}

	ProxyContextDeleteByVectorValue(Context->persistentFlowMap, 0, host);
	NxClientDelete(host);
}


static int
HostDisconnectAll()
{
	return NxServerDisconnectAll(Context->hostListen);
}


static NxClient_t*
HostSelectByService(ProxyRequest_t *proxyReq)
{
	static ObjectLink_t *prevLink = NULL;
	NxClient_t *host = NULL;

	char svcType = (char)proxyReq->hostReq.hdr.svcType;

	if ( svcType == eSvcConfig )
	{
		ObjectList_t *list = (ObjectList_t*)HashGetNextEntryVar(Context->hostConnections, NULL);		// get first
		if ( list != NULL )
			host = (NxClient_t*)ObjectListFirstVar(list);
	}
	else
	{ // try to cycle round-robin across all connected servers...
		FecService_t *service = NULL;
		for (int i = 0; i < FecConfigGlobal->numberServices; ++i)
		{
			if (FecConfigGlobal->services[i].status == FecConfigVerified && FecConfigGlobal->services[i].properties.port == proxyReq->servicePort)
			{
				service = &FecConfigGlobal->services[i];
				break;
			}
		}
		if (service == NULL)
			return NULL;			// nothing available

		if ( (!ObjectTestVerify(ObjectLink, prevLink)) ||
				((prevLink = ObjectListNext(prevLink)) == NULL) ||
				(!ObjectTestVerify(ObjectLink, prevLink)) ||
				(((host = (NxClient_t*)prevLink->var)) == NULL) )
		{ // either first time, prev host is invalid, or no 'next' host from prev...
			ObjectList_t *list = NULL;
			list = (ObjectList_t*)HashFindStringVar(Context->hostConnections, service->properties.stratus1);
			if ( list == NULL )			// no connect from this ip
				list = (ObjectList_t*)HashFindStringVar(Context->hostConnections, service->properties.stratus2);
			if ( list == NULL )			// no connect from this ip
				list = (ObjectList_t*)HashFindStringVar(Context->hostConnections, service->properties.stratus3);
			if ( list == NULL )
			{
				SysLog(LogWarn, "Unable to locate a destination for service %s.%d", service->properties.protocol, service->properties.port);
				return NULL;
			}
			// save selected host only if its a primary
			prevLink = (ObjectLink_t*)ObjectListFirst(list);
			host = (NxClient_t*)prevLink->var;
		}
	}

	if ( ! ObjectTestVerify(NxClient, host) )
		host = NULL;		// no host

	return host;
}


static long
HostNbrOnline()
{
	return HashMapLength(Context->hostListen->connectionList);
}


static void
InflightContextCollector(ProxyContextEntry_t *ce)
{
	ProxyContextEntryVerify(ce);
	SysLog(LogDebug, "%s", ProxyContextEntryToString(ce));
	ProxyRequest_t *proxyReq;
	if ( (proxyReq = (ProxyRequest_t*)ce->vectors[0]) != 0 )
	{
		if ( proxyReq->reqType == ProxyReqHostMsg )
			SysLogPushPrefix(proxyReq->hostReq.hdr.peerName);
		ProxyRequestDelete(proxyReq);
	}
}


static void
PersistentFlowContextCollector(ProxyContextEntry_t *ce)
{
	ProxyContextEntryVerify(ce);
	SysLog(LogDebug, "%s", ProxyContextEntryToString(ce));
}


static int
HostSend(NxClient_t *from, ProxyRequest_t *proxyReq)
{
	char *msgType = "";
	char svcType = (char)proxyReq->hostReq.hdr.svcType;

	switch (svcType)
	{
		default:
			SysLog(LogError, "Invalid request svcType: %s", ProxyRequestToString(proxyReq, DumpOutput));
			return -1;
			break;

		case eSvcConfig:
			msgType = HostConfigReqestMsgType;
			break;

		case eSvcAuth:
		case eSvcEdc:
		case eSvcEdcMulti:
			msgType = HostRequestMsgType;
			break;
	}

// if this is marked as a persistent flow msg then use previous destination host by this session

	NxClient_t *host = NULL;
	if ( proxyReq->flowType == ProxyFlowPersistent )
	{
		ProxyContextEntry_t *ce = ProxyContextFind(Context->persistentFlowMap, proxyReq->hostReq.hdr.peerUid);
		if ( ce == NULL )			// first one... save the selected host
		{
			ce = ProxyContextNew(Context->persistentFlowMap, proxyReq->replyTTL, proxyReq->hostReq.hdr.peerUid);
			SysLog(LogDebug, "Create context %s for %s", ProxyContextEntryToString(ce), ProxyRequestToString(proxyReq, NoOutput));
			if ( (host = HostSelectByService(proxyReq)) == NULL )	// Pick the preferred host
			{
				SysLog(LogError, "No Proxy online for %s", ProxyRequestToString(proxyReq, DumpOutput));
				return 1;			// No service error
			}
			ce->vectors[0] = host;
			ce->vectors[1] = from;
		}
		else		// otherwise, find the previously used host
		{
			host = NULL;
			for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(Context->hostListen->connectionList, entry)) != NULL;)
			{
				NxClient_t *client = (NxClient_t*)entry->var;
				if ( client == ce->vectors[0] )
				{
					host = client;			// this is he
					NxClientVerify(host);
					break;
				}
			}
			if ( ! ObjectTestVerify(NxClient, host) )
			{
				SysLog(LogError, "Cannot find previously used host for %s", ProxyRequestToString(proxyReq, DumpOutput));
				return 1;			// No service error
			}
		}
	}
	else		// not a persistent flow
	{
		host = HostSelectByService(proxyReq);	// Pick a preferred host
	}


	if (host == NULL)
	{
		SysLog(LogError, "No Proxy online for %s", ProxyRequestToString(proxyReq, DumpOutput));
		return 1;			// No service error
	}

	NxClientVerify(host);		// insurance...

// We have a valid request svcType and a host

// create an inflight context

// Build and send host message frame
// Determine the destination and send the frame

	HostFrame_t frame;

// if expecting a reply, replyTTL > 0; then save this request in context

	ProxyContextEntry_t *ce = NULL;
	if ( proxyReq->replyTTL > 0 )
	{
		ProxyRequest_t *proxyHdr = ProxyRequestNewSized(0);		// allocate a zero length request to save hdr into
		int len = sizeof(*proxyHdr)-sizeof(proxyHdr->hostReq.data);		// length of hdr
		memcpy(proxyHdr, proxyReq, len);
		ce = ProxyContextNew(Context->inflightContextMap, proxyHdr->replyTTL, NxUidNull);
		SysLog(LogDebug, "Create context %s for %s", ProxyContextEntryToString(ce), ProxyRequestToString(proxyHdr, NoOutput));
		ce->vectors[0] = proxyHdr;
		ce->vectors[1] = from;
	}

	HostFrameBuildHeader(&frame.hdr, msgType, proxyReq->servicePort, svcType,
			NxGlobal->sysid,
			(char *)proxyReq->hostReq.hdr.peerIpAddr, '4',
					proxyReq->hostReq.hdr.peerIpPort,
					ce?ce->uid : NxUidNull);

	HostFrameSetLen(&frame, 0);

	HostFrameSetPayload(&frame, (unsigned char *)proxyReq->hostReq.data, proxyReq->hostReq.len);
	HostFrameSetLen(&frame, proxyReq->hostReq.len);

	if (HostFrameSend(host, &frame) <= 0)
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(host));
		SysLog(LogError, "HostFrameSend failed: %s", NxClientUidToString(host));
		if ( ce != NULL )
			ProxyContextDelete(Context->inflightContextMap, ce);
		return -1;
	}

	++Context->hostCounts.outPkts;
	Context->hostCounts.outChrs += proxyReq->hostReq.len;
	return 0;
}


static int
HostRecv(NxClient_t *host)
{
	NxClientVerify(host);

	HostFrame_t frame;
	int rlen = HostFrameRecv(host, &frame);

	if (rlen < 0 || (rlen > 0 && rlen < (int)sizeof(frame.hdr)))
	{
		AuditSendEvent(AuditSystemError, "connid", NxClientUidToString(host));
		SysLog(LogError, "HostFrameRecv short read; failed: %s", NxClientUidToString(host));
		return -1;
	}

	if ( rlen == 0 )
		return 0;		// nothing...

	// Translate the service type from host to FEC context
	switch (frame.hdr.svcType[0])
	{
		default:
			SysLog(LogError, "Invalid svcType %d; discarding: %s", frame.hdr.svcType[0], NxClientUidToString(host));
			return 1;
			break;

		case eSvcConfig:
		case eSvcAuth:
		case eSvcEdc:
		case eSvcEdcMulti:
		{
			ProxyContextEntry_t *ce = ProxyContextFind(Context->inflightContextMap, frame.hdr.echo);
			if (ce == NULL)
			{
				SysLog(LogError, "I have no destination for %s", NxUidToString(frame.hdr.echo));
				SysLog(LogError | SubLogDump, (char *)frame.payload, HostFramePayloadLen(frame), "bytes %d", HostFramePayloadLen(frame));
				return 1;
			}

			ProxyRequest_t *proxyHdr = (ProxyRequest_t*) ce->vectors[0];
			ProxyRequestVerify(proxyHdr);
			if ( proxyHdr->reqType == ProxyReqHostMsg )
				SysLogPushPrefix(proxyHdr->hostReq.hdr.peerName);
			SysLog(LogDebug, "Found Context %s for %s", ProxyContextEntryToString(ce), proxyHdr->hostReq.hdr.peerName);

			NxClient_t *client = (NxClient_t *)ce->vectors[1];
			NxClientVerify(client);

			ProxyRequest_t *proxyReq = ProxyRequestNew();		// allocate full size request
			int len = sizeof(*proxyHdr)-sizeof(proxyHdr->hostReq.data);		// length of hdr
			memcpy(proxyReq, proxyHdr, len);			// copy the header

			proxyReq->hostReq.len = HostFramePayloadLen(frame);
			memcpy(proxyReq->hostReq.data, frame.payload, min(sizeof(proxyReq->hostReq.data), proxyReq->hostReq.len));

			++Context->hostCounts.inPkts;
			Context->hostCounts.inChrs += proxyReq->hostReq.len;

			if ( ClientSend(client, proxyReq) < 0 )
			{
				SysLog(LogError, "ClientSend failed: %s", ProxyRequestToString(proxyReq, DumpOutput));
				ProxyContextDelete(Context->inflightContextMap, ce);
				ClientDisconnect(client);
				ProxyRequestDelete(proxyReq);
				return 1;
			}
			if ( frame.hdr.svcType[0] != eSvcConfig )		// if not a config record, ditch the context
				ProxyContextDelete(Context->inflightContextMap, ce);	// otherwise, context will hang around for next config response
			ProxyRequestDelete(proxyReq);
			break;
		}
	}
	return 0;
}


static void
TimerSet(int ms)
{

	TimerCancel(Context->currentTimer);

	if (ms > 0)
	{
		if (TimerActivate(Context->currentTimer, ms, HandleTimerEvent) != 0)
			SysLog(LogError, "TimerActivate failed");
	}
}


// State Machine
//


static char*
FsmStateToString(Fsm_t *this, int state)
{
	char *text;

	switch (state)
	{
		default:
			text = StringStaticSprintf("State_%d", (int)state);
			break;

		EnumToString(NullState);
		EnumToString(WaitForReceiveListenState);
		EnumToString(WaitForHostListenState);
		EnumToString(SteadyState);
	}

	return text;
}


static char*
FsmEventToString(Fsm_t *this, int evt)
{
	char *text;

	switch (evt)
	{
		default:
			text = StringStaticSprintf("Event_%d", (int)evt);
			break;

		EnumToString(TimerEvent);
		EnumToString(HostListenReadyEvent);
		EnumToString(ReceiveListenReadyEvent);
		EnumToString(HostReadReadyEvent);
		EnumToString(ClientReadReadyEvent);
		EnumToString(ExceptionEvent);
	}

	return text;
}


// States
//


static void
FsmNullState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch (evt)
	{
	default:
		SysLog(LogError, "%s is not a known event for this state", FsmEventToString(this, evt));
	case ExceptionEvent:
		TimerSet(0);
		SysLog(LogFatal, "Exception. All is lost...");
		break;

	case TimerEvent:
		if ( ReceiveListen() != 0 )
		{
			SysLog(LogError, "ReceiveListen failed");	// Next state will take care of this...
			TimerSet(RecoveryWaitInterval);
			FsmSetNewState(this, WaitForReceiveListenState);
		}

		if (HostListen() != 0)
		{
			SysLog(LogError, "HostListen failed");	// Next state will take care of this...
			TimerSet(RecoveryWaitInterval);
			FsmSetNewState(this, WaitForHostListenState);
		}
		TimerSet(RecoveryWaitInterval);
		FsmSetNewState(this, SteadyState);
		break;
	}
}


static void
FsmWaitForReceiveListenState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch (evt)
	{
	default:
		SysLog(LogError, "%s is not a known event for this state", FsmEventToString(this, evt));
	case ExceptionEvent:
		TimerSet(0);
		SysLog(LogFatal, "Exception. All is lost...");
		break;

	case TimerEvent:
		SysLog(LogWarn, "Timeout waiting to listen");

		if ( ReceiveListen() != 0 )
		{
			SysLog(LogError, "ReceiveListen failed");	// Next state will take care of this...
			TimerSet(RecoveryWaitInterval);
			FsmSetNewState(this, WaitForReceiveListenState);
		}

		if (HostListen() != 0)
		{
			SysLog(LogError, "HostListen failed");	// Next state will take care of this...
			TimerSet(RecoveryWaitInterval);
			FsmSetNewState(this, WaitForHostListenState);
		}

		TimerSet(RecoveryWaitInterval);
		FsmSetNewState(this, SteadyState);
		break;
	}
}


static void
FsmWaitForHostListenState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch (evt)
	{
	default:
		SysLog(LogError, "%s is not a known event for this state", FsmEventToString(this, evt));
	case ExceptionEvent:
		TimerSet(0);
		SysLog(LogFatal, "Exception. All is lost...");
		break;

	case TimerEvent:
		SysLog(LogWarn, "Timeout waiting to listen");

		if (HostListen() != 0)
		{
			SysLog(LogError, "HostListen failed");	// Next state will take care of this...
			TimerSet(RecoveryWaitInterval);
			FsmSetNewState(this, WaitForHostListenState);
		}

		TimerSet(RecoveryWaitInterval);
		FsmSetNewState(this, SteadyState);
		break;

	case ReceiveListenReadyEvent:
		if ( ReceiveAccept() == NULL )
			SysLog(LogError, "ReceiveAccept failed");
		break;

	case ClientReadReadyEvent:
		{
			NxClient_t *client = (NxClient_t*)efarg;

			ProxyRequest_t *proxyReq = ClientRecv(client);
			if ( proxyReq == NULL )
			{
				SysLog(LogError, "Treating error as a disconnect: %s", NxClientToString(client));
				ClientDisconnect(client);
				break;
			}
			SysLog(LogError, "Host not available; discarding %s", ProxyRequestToString(proxyReq, DumpOutput));
			// host not available; return status
			ProxyRequestDelete(proxyReq);
			if (ClientNotifyHostStatus(client, NULL) != 0)
			{
				SysLog(LogError, "ClientNotifyHostStatus failed: %s", NxClientToString(client));
				ClientDisconnect(client);
			}
			break;
		}
	}
}


static void
FsmSteadyState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch (evt)
	{
	default:
		SysLog(LogError, "%s is not a known event for this state", FsmEventToString(this, evt));
	case ExceptionEvent:
		SysLog(LogError, "Exception. Disconnecting");
		HostDisconnectAll();
		if (ClientAllNotifyHostStatus() < 0)
			SysLog(LogError, "ClientAllNotifyHostStatus failed");
		TimerSet(RecoveryWaitInterval);
		break;

	case TimerEvent:
		if (ClientAllNotifyHostStatus() < 0)
			SysLog(LogError, "ClientAllNotifyHostStatus failed");
		TimerSet(RecoveryWaitInterval);
		break;

	case HostListenReadyEvent:
		TimerSet(RecoveryWaitInterval);

		if ( HostAccept() == NULL )
		{
			SysLog(LogError, "HostAccept failed");
			if ( HostNbrOnline() <= 0 )
			{
				SysLog(LogError, "No host connections available");
				FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception: this is a major problem
			}
		}
		if (ClientAllNotifyHostStatus() < 0)
			SysLog(LogError, "ClientAllNotifyHostStatus failed");
		break;

	case HostReadReadyEvent:
	{
		NxClient_t *host = (NxClient_t*)efarg;

		if (HostRecv(host) < 0)
		{
			SysLog(LogError, "HostRecv failed: %s", NxClientToString(host));
			HostDisconnect(host);
			if (HashGetNextEntry(Context->hostListen->connectionList, NULL) == NULL)
			{
				SysLog(LogError, "No host connections available");
				FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception: this is a major problem
			}
		}
		break;
	}

	case ReceiveListenReadyEvent:
		if ( ReceiveAccept() == NULL )
			SysLog(LogError, "ReceiveAccept failed");
		break;

	case ClientReadReadyEvent:
		{
			NxClient_t *client = (NxClient_t*)efarg;

			ProxyRequest_t *proxyReq = ClientRecv(client);
			if ( proxyReq == NULL )
			{
				SysLog(LogError, "Treating error as a disconnect: %s", NxClientToString(client));
				ClientDisconnect(client);
				break;
			}
			// forward to host
			if (proxyReq->reqType != ProxyReqHostMsg)
			{
				SysLog(LogError, "Don't know message reqType %s; discarding %s", ProxyReqTypeToString(proxyReq->reqType), NxClientToString(client));
				ClientDisconnect(client);
				ProxyRequestDelete(proxyReq);
				break;
			}

			{
				int rr = HostSend(client, proxyReq);
				if ( rr != 0 )
				{
					SysLog(LogError, "HostSend failed; discarding %s", ProxyRequestToString(proxyReq, DumpOutput));
					if ( rr < 0 )
					{
						ProxyRequestDelete(proxyReq);
						FsmDeclareEvent(this, ExceptionEvent, client);	// Declare an exception: this is a major problem
					}
				}
				ProxyRequestDelete(proxyReq);
			}
			break;
		}
	}
}


static void
FsmEventHandler(Fsm_t *this, int evt, void * efarg)
{

	switch (this->currentState)
	{
	default:
		SysLog(LogError, "Invalid state %s", FsmStateToString(this, this->currentState));
		SysLog(LogFatal, "Exception. All is lost...");
		break;

	case NullState:
		FsmNullState(this, evt, efarg);
		break;

	case WaitForReceiveListenState:
		FsmWaitForReceiveListenState(this, evt, efarg);
		break;

	case WaitForHostListenState:
		FsmWaitForHostListenState(this, evt, efarg);
		break;

	case SteadyState:
		FsmSteadyState(this, evt, efarg);
		break;
	}
}


static void
Shutdown()
{
	SysLog(LogWarn, "Attempting a shutdown");
	TimerCancelAll();
	ContextDelete(Context);
	ProcStop(NxCurrentProc, 0);			// only returns on error
	SysLog(LogFatal, "ProcStop failed");
}


static void
HandleTpsTimerEvent(Timer_t *tid)
{
	unsigned long inPkts = (unsigned long)Context->hostCounts.inPkts;	// save current
	NxClient_t	*client = (NxClient_t*)tid->context;
	NxClientVerify(client);

	unsigned long prevcount = (unsigned long)client->evf->context;

	inPkts -= prevcount;
	inPkts /= (tid->ms/1000);		// make it tps...

	char tmp[1024];
	sprintf(tmp, "%ld TPS\n", inPkts);

	if (NxClientSendPkt(client, tmp, strlen(tmp)) != strlen(tmp))
		SysLog(LogError, "NxClientSendPkt failed");

	NxClientDelete(client);
	TimerDelete(tid);			// destroy timer
}


static CommandResult_t
TpsCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{

	CommandResult_t ret = CommandBadOption;
	char *word = ParserGetNextToken(parser, " ");

	int t = 30;

	if (word != NULL && strlen(word) > 0)
	{
		do
		{
			boolean err;
			t = IntFromString(word, &err);
			if ( err )
				t = 30;		// default
		} while ((word = ParserGetNextToken(parser, " ")) != NULL && strlen(word) > 0);

		ret = CommandOk;
	}

	if ( t > 0 )
	{
		Timer_t *tpsTimer = TimerNew("%sTpsTimer", NxCurrentProc->name);
		unsigned long inPkts = (unsigned long)Context->hostCounts.inPkts;
		if (TimerActivate(tpsTimer, t*1000, HandleTpsTimerEvent) != 0)
		{
			SysLog(LogError, "TimerActivate failed");
			ret = CommandFailed;
		}
		else
		{
			NxClient_t *client = NxClientSiezeConnection(this->client);
			tpsTimer->context = client;
			client->evf->context = (void*)inPkts;
			ret = CommandSiezedConnection;
			StringSprintf(response, "Measuring TPS for %d seconds", t);
		}
	}

	return ret;
}


static CommandResult_t
ShowCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	CommandResult_t ret = CommandBadOption;
	char *word = ParserGetNextToken(parser, " ");

	if (word != NULL && strlen(word) > 0)
	{
		do
		{
			if ( stricmp(word, "host") == 0 )
			{
				long n = HostNbrOnline(NxCurrentProc);
				if ( n > 0 )
					StringSprintf(response, "%d Host%s online", n, n>1?"s":"");
				else
					StringCpy(response, "All Hosts offline");
				ret = CommandOk;
			}
			else
			{
				StringCpy(response, "try\nshow host");
			}
		} while ((word = ParserGetNextToken(parser, " ")) != NULL && strlen(word) > 0);
	}
	else
	{
		StringCpy(response, "try\nshow host");
	}

	return ret;
}


static int
CommandHandler(NxClient_t *client, Parser_t *parser, String_t *response)
{

	CommandDef_t cmds[] =
	{
		{"tps", TpsCmd},
		{"show", ShowCmd},
		{NULL, NULL}
	};

	Command_t *cmd = CommandNew(client, cmds);

	CommandResult_t ret = CommandExecute(cmd, parser, "", response);

	CommandDelete(cmd);

	return ret;
}


static ProcContext_t*
ProcContextConstructor(ProcContext_t *this, char *file, int lno)
{

	// Create the Host File
	this->hostListen = NxServerNew();

	// Create the client listener (handles worker request/responses to the Host)
	this->receiveListen = NxServerNew();
	this->hostConnections = HashMapNew(8, "HostConnections");

	this->currentTimer = TimerNew("%sTimer", NxCurrentProc->name);

	this->inflightContextMap = ProxyContextMapNew(ProcGetPropertyIntValue(NxCurrentProc, "ContextSize"), ProcGetPropertyIntValue(NxCurrentProc, "ContextAgeInterval"), 2, InflightContextCollector);
	this->persistentFlowMap = ProxyContextMapNew(ProcGetPropertyIntValue(NxCurrentProc, "ContextSize"), ProcGetPropertyIntValue(NxCurrentProc, "ContextAgeInterval"), 2, PersistentFlowContextCollector);
	return this;
}


static void
ProcContextDestructor(ProcContext_t *this, char *file, int lno)
{
	
	HashMapDelete(this->hostConnections);
	NxServerDelete(this->hostListen);
	NxServerDelete(this->receiveListen);

	if (this->currentTimer != NULL )
		TimerDelete(this->currentTimer);

	ProxyContextMapDelete(this->inflightContextMap);
	ProxyContextMapDelete(this->persistentFlowMap);
}


static Json_t*
ProcContextSerialize(ProcContext_t *this)
{
	ContextVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	JsonAddItem(root, "Fsm", FsmSerialize(this->fsm));

	JsonAddItem(root, "HostListen", NxServerSerialize(this->hostListen));
	JsonAddItem(root, "ReceiveQueue", NxServerSerialize(this->receiveListen));

	if (this->currentTimer != NULL && this->currentTimer->active)
		JsonAddItem(root, "Timer", TimerSerialize(this->currentTimer));

	JsonAddNumber(root, "Inflights", this->inflightContextMap->map->length);
	JsonAddNumber(root, "Persistents", this->persistentFlowMap->map->length);

// The host connections
	{
		JsonAddNumber(root, "HostsOnline", HostNbrOnline());
		Json_t *sub = JsonPushObject(root, "Hosts");
		if ( HashMapLength(this->hostListen->connectionList) > 0 )
		{
			ObjectList_t* list = HashGetOrderedList(this->hostListen->connectionList, ObjectListUidType);
			for ( HashEntry_t *entry = NULL; (entry = ObjectListRemove(list, ObjectListFirstPosition)); )
			{
				NxClient_t *host = (NxClient_t*)entry->var;
				JsonAddItem(sub, "Host", NxClientSerialize(host));
				HashEntryDelete(entry);
			}
			ObjectListDelete(list);
		}
	}
	JsonAddItem(root, "HostCounts", CountsSerialize(this->hostCounts));

// The client connections
	{
		if ( HashMapLength(this->receiveListen->connectionList) > 0 )
		{
			Json_t *sub = JsonPushObject(root, "Clients");
			ObjectList_t* list = HashGetOrderedList(this->receiveListen->connectionList, ObjectListUidType);
			for ( HashEntry_t *entry = NULL; (entry = ObjectListRemove(list, ObjectListFirstPosition)); )
			{
				NxClient_t *client = (NxClient_t*)entry->var;
				JsonAddItem(sub, "Client", NxClientSerialize(client));
				HashEntryDelete(entry);
			}
			ObjectListDelete(list);
		}
	}

	return root;
}


static char*
ProcContextToString(ProcContext_t *this)
{
	Json_t *root = ProcContextSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


int
ProcStratusProxyStart(Proc_t *this, va_list ap)
{
	this->context = ContextNew();
	this->commandHandler = CommandHandler;
	this->contextSerialize = ProcContextSerialize;
	this->contextToString = ProcContextToString;

	Context->fsm = FsmNew(FsmEventHandler, FsmStateToString, FsmEventToString, NullState, "%sFsm", this->name);

	ProcSetSignalHandler(this, SIGTERM, SigTermHandler);	// Termination signal

	// Start the background clock
	TimerSet(1);
	return 0;
}
