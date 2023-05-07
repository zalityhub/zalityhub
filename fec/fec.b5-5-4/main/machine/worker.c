/****************************************************************************

Filename:   main/machine/worker.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:34:01 $
 * $Header: /home/hbray/cvsroot/fec/main/machine/worker.c,v 1.3.4.14 2011/10/27 18:34:01 hbray Exp $
 *
 $Log: worker.c,v $
 Revision 1.3.4.14  2011/10/27 18:34:01  hbray
 Revision 5.5

 Revision 1.3.4.13  2011/09/26 15:52:33  hbray
 Revision 5.5

 Revision 1.3.4.12  2011/09/24 18:30:52  hbray
 *** empty log message ***

 Revision 1.3.4.11  2011/09/24 17:49:54  hbray
 Revision 5.5

 Revision 1.3.4.9  2011/09/01 14:49:48  hbray
 Revision 5.5

 Revision 1.3.4.7  2011/08/25 18:19:46  hbray
 *** empty log message ***

 Revision 1.3.4.6  2011/08/23 19:54:01  hbray
 eliminate fecplugin.h

 Revision 1.3.4.5  2011/08/23 12:03:16  hbray
 revision 5.5

 Revision 1.3.4.4  2011/08/17 17:59:06  hbray
 *** empty log message ***

 Revision 1.3.4.3  2011/08/15 19:12:32  hbray
 5.5 revisions

 Revision 1.3.4.2  2011/08/11 19:47:36  hbray
 Many changes

 Revision 1.3.4.1  2011/07/29 17:31:55  hbray
 Clean up logging

 Revision 1.3  2011/07/27 20:22:25  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:58  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: worker.c,v 1.3.4.14 2011/10/27 18:34:01 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/utillib.h"

#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"
#include "include/fecconfig.h"
#include "include/proxy.h"

#include "machine/include/worker.h"


// Local Data Types

#define Context ((NxCurrentProc)->context)


typedef enum
{
	NullState, SteadyState, ShutdownState
} FsmState;

typedef enum
{
	TimerEvent, ShutdownTimerEvent, WorkReadReadyEvent, PosReadReadyEvent, AuthReadReadyEvent, EdcReadReadyEvent, T70ReadReadyEvent, ExceptionEvent
} FsmEvent;


typedef struct ProcContext_t
{
	Fsm_t			*fsm;

	NxTime_t		shutdownStartTime;

	NxClient_t		*workQueue;
	NxServer_t		*clientConnections;

	NxClient_t		*authProxy;
	NxClient_t		*edcProxy;
	NxClient_t		*t70Proxy;

	Timer_t			*currentTimer;
	Timer_t			*currentShutdownTimer;

	PiSession_t		*currentSession;

	boolean			checkForStalls;
	
	boolean			authProxyEnabled;
	boolean			edcProxyEnabled;
	boolean			t70ProxyEnabled;

	boolean			workQueueSuppressed;
} ProcContext_t;


#define ContextNew(workQueue) ObjectNew(ProcContext, workQueue)
#define ContextVerify(var) ObjectVerify(ProcContext, var)
#define ContextDelete(var) ObjectDelete(ProcContext, var)

static ProcContext_t* ProcContextConstructor(ProcContext_t *this, char *file, int lno, NxClient_t *workQueue);
static void ProcContextDestructor(ProcContext_t *this, char *file, int lno);
static BtNode_t* ProcContextNodeList;
static Json_t* ProcContextSerialize(ProcContext_t *this);
static char* ProcContextToString(ProcContext_t *this);



// Static Functions
//
// Event Handlers
//
static void HandleAuthEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandleEdcEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandleT70Event(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandlePosEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandleTimerEvent(Timer_t *tid);
static void HandleShutdownTimerEvent(Timer_t *tid);
static void HandleWorkQueueEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void SigTermHandler(NxSignal_t*);

// Fsm Functions
//
static void FsmEventHandler(Fsm_t *this, int evt, void * efarg);
static char *FsmEventToString(Fsm_t *this, int evt);
static char *FsmStateToString(Fsm_t *this, int state);
static void FsmNullState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmSteadyState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmShutdownState(Fsm_t *this, FsmEvent evt, void * efarg);

// Auth Functions
//
static int AuthProxyConnect();
static int AuthProxyDisconnect();
static int AuthProxySend(ProxyRequest_t *proxyReq);
static int AuthProxyRecv(ProxyRequest_t *proxyReq);

// Edc Functions
//
static int EdcProxyConnect();
static int EdcProxyDisconnect();
static int EdcProxySend(ProxyRequest_t *proxyReq);
static int EdcProxyRecv(ProxyRequest_t *proxyReq);

// T70 Functions
//
static int T70ProxyConnect();
static int T70ProxyDisconnect();
static int T70ProxySend(ProxyRequest_t *proxyReq);
static int T70ProxyRecv(ProxyRequest_t *proxyReq);

// Pos Functions
//
static PiSession_t *PosAccept();
static int PosDisconnect(PiSession_t *sess, boolean acceptFailure);
static void PosAcceptDisconnect();
static void PosDisconnectAll();
static void PosShutdownAll();
static int PosRecv(PiSession_t *sess);

// Helper Functions
//
static void TimerSet(int ms);
static void ShutdownTimerSet(int ms);
static int ProxyConnect();
static int ProxyPump(PiSession_t *sess);
static int ProxyRecv(ProxyRequest_t *proxyReq);

static int IntegrityCheck();
static int CommandHandler(NxClient_t *client, Parser_t *parser, String_t *response);
static void InitiateShutdown();
static void FinishShutdown();

// Static Global Vars
//
static const int InitialWaitInterval = (2 * 1000);
static const int RecoveryWaitInterval = (60 * 1000);
static const int SteadyWaitInterval = (30 * 1000);

static void LogWriter (SysLog_t *this, void **contextp, SysLogLevel lvl, char *file, int lno, char *fnc, char *bfr, int len);



// Functions Start Here

// Event Handlers
//

static void
SigTermHandler(NxSignal_t *sig)
{
	InitiateShutdown();
}


static void
HandleTimerEvent(Timer_t *tid)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);
	_FsmDeclareEvent(fsm, TimerEvent, tid);
}


static void
HandleShutdownTimerEvent(Timer_t *tid)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);
	_FsmDeclareEvent(fsm, ShutdownTimerEvent, tid);
}


static void
HandleAuthEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, AuthReadReadyEvent, evf);
}


static void
HandleEdcEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, EdcReadReadyEvent, evf);
}


static void
HandleT70Event(EventFile_t *evf, EventPollMask_t pollMask, void * farg)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, T70ReadReadyEvent, evf);
}


static void
HandleWorkQueueEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, WorkReadReadyEvent, evf);
}


static void
HandlePosEvent(EventFile_t *evf, EventPollMask_t pollMask, void *farg)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	NxClient_t *client = (NxClient_t*)farg;
	NxClientVerify(client);
	PiSession_t *sess = PiGetSession(client);

	if ( ! ObjectTestVerify(PiSession, sess) )
	{
		SysLog(LogError, "Invalid or no service context: %p", sess);
		SysLog(LogError, "Discarding %s from %s", EventPollMaskToString(pollMask), NxClientToString(client));
		NxClientDelete(client);
		return;
	}

	PiSessionVerify(sess);
	PiSessionSetLogPrefix(sess);

	Context->currentSession = sess;

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, PosReadReadyEvent, sess);
}


// Auth Functions
//

static int
AuthProxyConnect()
{

	// Open auth service port

	SysLog(LogDebug, "Connecting to AuthProxy");

	{
		char tmp[1024];
		sprintf(tmp, "%s/%s", NxGlobal->nxDir, NxGetPropertyValue("AuthProxy.InputQueue"));

		if ( NxClientConnect(Context->authProxy, AF_UNIX, SOCK_STREAM, tmp, 0, EventFileHighPri, EventReadMask, HandleAuthEvent) < 0 )
		{
			SysLog(LogError, "NxClientConnect failed: %s", tmp);
			AuthProxyDisconnect();
			return -1;
		}
	}

	AuditSendEvent(AuditAuthConnect, "connid", NxClientUidToString(Context->authProxy));
	return 0;
}


static int
AuthProxyDisconnect()
{
	SysLog(LogDebug, "Disconnecting AuthProxy %s", NxClientNameToString(Context->authProxy));
	NxClientDisconnect(Context->authProxy);
	return 0;
}


static int
AuthProxySend(ProxyRequest_t *proxyReq)
{
// If auth connection is down, try to reconnect
	if ( ! NxClientIsConnected(Context->authProxy) )
	{
		SysLog(LogWarn, "No Auth Proxy Connection");
		return -1;
	}

	SysLog(LogDebug, ProxyRequestToString(proxyReq, DumpOutput));
	int len = ProxyRequestLen(proxyReq);

	if (NxClientSendPkt(Context->authProxy, (char *)proxyReq, len) != len )
	{
		SysLog(LogError, "NxClientSendPkt failed: %s", NxClientNameToString(Context->authProxy));
		return -1;
	}

	return 0;
}


static int
AuthProxyRecv(ProxyRequest_t *proxyReq)
{
	int rlen;

	if ((rlen = NxClientRecvPkt(Context->authProxy, (char *)proxyReq, sizeof(*proxyReq))) < 0)
	{
		SysLog(LogError, "NxClientRecvPkt failed: %s", NxClientNameToString(Context->authProxy));
		return -1;
	}

	if (rlen < sizeof(*proxyReq) - sizeof(proxyReq->hostReq.data))
	{
		SysLog(LogError, "Short read of %d from %s; failed", rlen, NxClientNameToString(Context->authProxy));
		return -1;
	}

	SysLog(LogDebug, "%s", ProxyRequestToString(proxyReq, DumpOutput));
	return 0;
}


// Edc Functions
//

static int
EdcProxyConnect()
{

	// Open Edc service port

	SysLog(LogDebug, "Connecting to EdcProxy");

	{
		char tmp[1024];
		sprintf(tmp, "%s/%s", NxGlobal->nxDir, NxGetPropertyValue("EdcProxy.InputQueue"));

		if ( NxClientConnect(Context->edcProxy, AF_UNIX, SOCK_STREAM, tmp, 0, EventFileHighPri, EventReadMask, HandleEdcEvent) < 0 )
		{
			SysLog(LogError, "NxClientConnect failed: %s", tmp);
			EdcProxyDisconnect();
			return -1;
		}
	}

	AuditSendEvent(AuditEdcConnect, "connid", NxClientUidToString(Context->edcProxy));
	return 0;
}


static int
EdcProxyDisconnect()
{
	SysLog(LogDebug, "Disconnecting EdcProxy %s", NxClientNameToString(Context->edcProxy));
	NxClientDisconnect(Context->edcProxy);
	return 0;
}


static int
EdcProxySend(ProxyRequest_t *proxyReq)
{
// If Edc connection is down, try to reconnect
	if ( ! NxClientIsConnected(Context->edcProxy) )
	{
		SysLog(LogWarn, "No Edc Proxy Connection");
		return -1;
	}

	SysLog(LogDebug, ProxyRequestToString(proxyReq, DumpOutput));
	int len = ProxyRequestLen(proxyReq);

	if (NxClientSendPkt(Context->edcProxy, (char *)proxyReq, len) != len )
	{
		SysLog(LogError, "NxClientSendPkt failed: %s", NxClientNameToString(Context->edcProxy));
		return -1;
	}

	return 0;
}


static int
EdcProxyRecv(ProxyRequest_t *proxyReq)
{
	int rlen;

	if ((rlen = NxClientRecvPkt(Context->edcProxy, (char *)proxyReq, sizeof(*proxyReq))) < 0)
	{
		SysLog(LogError, "NxClientRecvPkt failed: %s", NxClientNameToString(Context->edcProxy));
		return -1;
	}

	if (rlen < sizeof(*proxyReq) - sizeof(proxyReq->hostReq.data))
	{
		SysLog(LogError, "Short read of %d from %s; failed", rlen, NxClientNameToString(Context->edcProxy));
		return -1;
	}

	SysLog(LogDebug, "%s", ProxyRequestToString(proxyReq, DumpOutput));
	return 0;
}


// T70 Functions
//

static int
T70ProxyConnect()
{

	// Open T70 service port

	SysLog(LogDebug, "Connecting to T70Proxy");

	{
		char tmp[1024];
		sprintf(tmp, "%s/%s", NxGlobal->nxDir, NxGetPropertyValue("T70Proxy.InputQueue"));

		if ( NxClientConnect(Context->t70Proxy, AF_UNIX, SOCK_STREAM, tmp, 0, EventFileHighPri, EventReadMask, HandleT70Event) < 0 )
		{
			SysLog(LogError, "NxClientConnect failed: %s", tmp);
			T70ProxyDisconnect();
			return -1;
		}
	}
	return 0;
}


static boolean
T70ProxyIsConnected()
{

	if (NxClientIsConnected(Context->t70Proxy))
	{
		SysLog(LogDebug, "T70Proxy %s connection complete", NxClientNameToString(Context->t70Proxy));
		return true;
	}

	SysLog(LogDebug, "T70Proxy %s connection pending", NxClientNameToString(Context->t70Proxy));
	return false;
}


static int
T70ProxyDisconnect()
{
	SysLog(LogDebug, "Disconnecting T70Proxy %s", NxClientNameToString(Context->t70Proxy));
	NxClientDisconnect(Context->t70Proxy);
	return 0;
}


static int
T70ProxySend(ProxyRequest_t *proxyReq)
{
// If T70 connection is down, try to reconnect
	if ( ! T70ProxyIsConnected() )
	{
		if (T70ProxyConnect() != 0)
			SysLog(LogError, "T70ProxyConnect failed");
	}
	if ( ! T70ProxyIsConnected() )
	{
		SysLog(LogWarn, "No T70 Proxy Connection");
		return -1;
	}

	SysLog(LogDebug, ProxyRequestToString(proxyReq, DumpOutput));
	int len = ProxyRequestLen(proxyReq);

	if (NxClientSendPkt(Context->t70Proxy, (char *)proxyReq, len) != len )
	{
		SysLog(LogError, "NxClientSendPkt failed: %s", NxClientNameToString(Context->t70Proxy));
		return -1;
	}

	return 0;
}


static int
T70ProxyRecv(ProxyRequest_t *proxyReq)
{
	int rlen;

	if ((rlen = NxClientRecvPkt(Context->t70Proxy, (char *)proxyReq, sizeof(*proxyReq))) < 0)
	{
		SysLog(LogError, "NxClientRecvPkt failed: %s", NxClientNameToString(Context->t70Proxy));
		return -1;
	}

	if (rlen < sizeof(*proxyReq) - sizeof(proxyReq->hostReq.data))
	{
		SysLog(LogError, "Short read of %d from %s; failed", rlen, NxClientNameToString(Context->t70Proxy));
		return -1;
	}

	SysLog(LogDebug, "%s", ProxyRequestToString(proxyReq, DumpOutput));
	return 0;
}


// Pos Functions
//

static PiSession_t *
PosAccept()
{

	NxClient_t *client = NxClientRecvConnection(Context->workQueue);
	if ( client == NULL )
	{
		SysLogSetLevel(LogWarn);	// suppress further debug logging of this
		return NULL;
	}

	if ( client == (NxClient_t*)-1 )		// a hard error
		SysLog(LogFatal, "NxClientRecvConnection failed on %s", NxClientNameToString(Context->workQueue));

	if ( NxServerAddClient(Context->clientConnections, client) != 0 )
		SysLog(LogFatal, "NxServerAddClient of %s failed", NxClientNameToString(client));

	if ( NxClientSetEventMask(client, EventFileLowPri, EventReadMask, HandlePosEvent, client) != 0 )
		SysLog(LogFatal, "NxClientSetEventMask of %s failed", NxClientNameToString(client));

// See if we're open
	if ( ! NxClientIsConnected(client) )
	{
		SysLog(LogWarn, "NxClientRecvConnection %s returned no connection", NxClientNameToString(Context->workQueue));
		NxClientDelete(client);
		errno = 0;
		return NULL;
	}

// find service context for this port
	FecService_t *service = FecConfigGetServiceContext(FecConfigGlobal, client->evf->servicePort);
	if (service == NULL)
	{
		SysLog(LogError, "FecConfigGetServiceContext failed: %s", NxClientToString(client));
		NxClientDelete(client);
		errno = 0;
		return NULL;
	}
	PiSession_t *sess = PiSessionNew(service, client);
	Context->currentSession = sess;
	client->evf->context = sess;

	PiSessionSetLogPrefix(sess);

	SysLog(LogDebug, "Located service %s for %d", service->plugin.libSpecs.libname, client->evf->servicePort);

	// setup callbacks in the new session
	sess->_priv.t70Queue->send = T70ProxySend;
	sess->_priv.authQueue->send = AuthProxySend;
	sess->_priv.edcQueue->send = EdcProxySend;

	SysLog(LogDebug, "Received forwarded connection %s", NxClientToString(client));
	
	PiAuditSendEvent(AuditPosForwardReceived, eOk, "session", PiSessionGetName(sess));

// Initialize the plugin
	if (PiBeginSession(sess) != eOk)
	{
		PiException(LogError, "PiBeginSession", sess->_priv.finalDisposition, "failed: %s", PiApiResultToString(sess->_priv.finalDisposition));
		PosDisconnect(sess, true);
		errno = 0;
		return NULL;
	}

	// if we have hit MaxFd, then disable future accepts
	if ( EventGlobalJoinedFdCount(NxGlobal->efg) >= NxGlobal->maxFd )
	{
		SysLog(LogWarn, "Hit %d connections; temporarily suppressing future connects", NxGlobal->maxFd);
		if ( NxClientSetEventMask(Context->workQueue, EventFileLowPri, EventNoMask, NULL, Context->workQueue) != 0 )
			SysLog(LogFatal, "NxClientSetEventMask of %s failed", NxClientNameToString(Context->workQueue));
		Context->workQueueSuppressed = true;
	}

	return sess;
}


static int
PosDisconnect(PiSession_t *sess, boolean acceptFailure)
{

	if (sess == NULL)
	{
		SysLog(LogError, "No service context: %p", sess);
		return -1;
	}

	NxClient_t *client = PiGetClient(sess);
	if (client != NULL)
	{
		SysLog(LogDebug, "%s", NxClientNameToString(client));
		if (NxClientIsOpen(client))	// only audit a disconnect if this socket is open
			PiAuditSendEvent(AuditPosDisconnect, eOk, "session", NxClientNameToString(client));
	}

	if (PiEndSession(sess) != eOk)
		PiException(LogWarn, "PiEndSession", eFailure,  "failed");

	PiSessionDelete(sess);
	Context->currentSession = NULL;

	if (client != NULL)
		NxClientDelete(client);

	return 0;
}


// Used to accept, then disconnect a client session
// Typically used when worker is not ready to process (Not in SteadyState)
static void
PosAcceptDisconnect()
{

	NxClient_t *client = NxClientRecvConnection(Context->workQueue);
	if ( client == NULL )
		SysLogSetLevel(LogWarn);	// suppress further debug logging of this
	else if ( client == (NxClient_t*)-1 )		// a hard error
		SysLog(LogFatal, "NxClientRecvConnection failed on %s", NxClientNameToString(Context->workQueue));

	NxClientDelete(client);
}


static int
PosRecv(PiSession_t *sess)
{

	PiSessionVerify(sess);
	Context->currentSession = sess;

	// Read the available data up to current capacity
	{
		char *bfr = alloca(MaxSockPacketLen+10);	// allocate a buffer
		int rlen = NxClientRecvRaw(PiGetClient(sess), (char *)bfr, MaxSockPacketLen);
		if ( rlen >= 0 )
			bfr[rlen] = '\0';

		SysLog(LogDebug | SubLogDump, bfr, rlen, "Recv %s len=%d", PiSessionGetName(sess), rlen);

		PiAuditSendEventData(AuditPosRecv, eOk, bfr, rlen, "sessionid", PiSessionGetName(sess), "%d.len", (char *)&rlen);

		if (rlen > 0)
		{
			sess->_priv.lastPosRecvTime = GetMsTime();
			if (PiPosPoke(sess, bfr, rlen) != 0)	// add to current aggregate
			{
				PiException(LogError, "PiPosPoke", eFailure, "failed");
				return -1;
			}
		}
		else
		{
			PiSessionSetEof(sess, true);	// Set EOF
		}
	}

	for (int ii = 0; sess != NULL; ++ii)
	{
		if (ii > 999)
		{
			PiException(LogError, "Plugin", eFailure, "libname=%s, libversion=%s: is looping", sess->_priv.plugin->libSpecs.libname, sess->_priv.plugin->libSpecs.version);
			return -1;
		}

		int prevFifoSize = sess->_priv.inputFifo->len;

		PiApiResult_t result = PiReadRequest(sess);

		SysLog(LogDebug, "ApiResult %s", PiApiResultToString(result));

		switch (result)
		{
		default:
			PiException(LogError, "PiReadRequest", eFailure, "I don't know what result %s means", PiApiResultToString(result));
			return -1;
			break;

		case eVirtual:			// A virtual plugin
			PiException(LogError, "PiReadRequest", result, "failed: %s", PiApiResultToString(sess->_priv.finalDisposition));
			return -1;
			break;

		case eFailure:			// Abnormal (FATAL) event
			PiException(LogError, "PiReadRequest", result, "failed: %s", PiApiResultToString(sess->_priv.finalDisposition));
			return -1;
			break;

		case eDisconnect:		// begin disconnect processing
			PosDisconnect(sess, false);
			return 0;
			break;

		case eWaitForData:
			// fall through to do the ProxyPump
		case eOk:
			{
				int ret;
				if ( (ret = ProxyPump(sess)) < 0)
				{
					PiException(LogError, "ProxyPump", eFailure, "failed");
					PosDisconnect(sess, false);
					return -1;
				}
				if ( ret > 0 )
					return 0;		// disconnected; all is well...
			}
			break;
		}

		if (sess == NULL)
			break;				// done

		if (sess->_priv.inputFifo->len <= 0)
			break;				// no data avail, done
		if (sess->_priv.inputFifo->len == prevFifoSize)
			break;				// non productive read, done
	}

// see if we need to force a close
	if ( sess != NULL && PiSessionAtEof(sess) )
	{
		PiException(LogDebug, (char*)__FUNC__, eOk, "%s: AtEof; forcing a close", PiSessionToString(sess));
		if (PosDisconnect(sess, false) != 0)
			PiException(LogError, "PosDisconnect", eFailure, "failed");
		return 0;
	}

	if ( sess != NULL && sess->_priv.finalDisposition != eOk )
	{
		PiException(LogError, (char*)__FUNC__, eFailure,  "%s: Exception; forcing a close", PiSessionToString(sess));
		if (PosDisconnect(sess, false) != 0)
			PiException(LogError, "PosDisconnect", eFailure, "failed");
		return 0;
	}

	return 0;
}


static void
PosShutdownAll()
{

	if ( HashMapLength(Context->clientConnections->connectionList) > 0 )
		SysLog(LogDebug, "Shutting down %d active sessions", HashMapLength(Context->clientConnections->connectionList));

	for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(Context->clientConnections->connectionList, entry)) != NULL;)
	{
		PiSession_t *sess = PiGetSession((NxClient_t *)entry->var);
		PiSessionVerify(sess);
		Context->currentSession = sess;

		PiSessionSetLogPrefix(sess);

		NxClient_t *client = PiGetClient(sess);
		NxClientVerify(client);
		if ( NxClientSetEventMask(client, EventFileLowPri, EventNoMask, NULL, client) != 0 )
			SysLog(LogFatal, "NxClientSetEventMask of %s failed", NxClientNameToString(client));
	}
}


static void
PosDisconnectAll()
{

	if ( HashMapLength(Context->clientConnections->connectionList) > 0 )
		SysLog(LogWarn, "Disconnecting %d active sessions", HashMapLength(Context->clientConnections->connectionList));

	for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(Context->clientConnections->connectionList, NULL)) != NULL;)
	{
		PiSession_t *sess = PiGetSession((NxClient_t *)entry->var);
		PiSessionVerify(sess);
		Context->currentSession = sess;

		PiSessionSetLogPrefix(sess);

		PiException(LogWarn, (char*)__FUNC__, eFailure,  "Force disconnect");
		if (PosDisconnect(sess, false) != 0)
		{
			PiException(LogError, "PosDisconnect", eFailure, "failed");
			NxServerDisconnectAll(Context->clientConnections); // something is wrong, just dump the entries...
			break;
		}
	}
}


// Helper Functions
//

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


static void
ShutdownTimerSet(int ms)
{
	TimerCancel(Context->currentShutdownTimer);

	if (ms > 0)
	{
		if (TimerActivate(Context->currentShutdownTimer, ms, HandleShutdownTimerEvent) != 0)
			SysLog(LogError, "TimerActivate failed");
	}
}


// Common Proxy Functions
// 

static int
ProxyConnect()
{

	if ( Context->authProxyEnabled )
	{
		if ( ! NxClientIsConnected(Context->authProxy))
		{
			if (AuthProxyConnect() != 0)
				SysLog(LogError, "AuthProxyConnect failed");
		}
	}

	if ( Context->edcProxyEnabled )
	{
		if ( ! NxClientIsConnected(Context->edcProxy))
		{
			if (EdcProxyConnect() != 0)
				SysLog(LogError, "EdcProxyConnect failed");
		}
	}

	if ( Context->t70ProxyEnabled )
	{
		if ( ! NxClientIsConnected(Context->t70Proxy))
		{
			if (T70ProxyConnect() != 0)
				SysLog(LogError, "T70ProxyConnect failed");
		}
	}

	return 0;
}


static int
ProxyRecv(ProxyRequest_t *proxyReq)
{

// find destination session
	NxClient_t *client = NxServerFindClient(Context->clientConnections, proxyReq->uid);
	if (client == NULL)
	{
		SysLog(LogError, "I have no destination for terminal connection %s", NxUidToString(proxyReq->uid));
		SysLog(LogError, "Probable late response:\n%s", ProxyRequestToString(proxyReq, DumpOutput));
		return 1;
	}

	PiSession_t *sess = (PiSession_t *)client->evf->context;
	if (sess == NULL)
	{
		SysLog(LogError, "No service context: %p", sess);
		return -1;
	}

	PiSessionVerify(sess);
	Context->currentSession = sess;

	PiSessionSetLogPrefix(sess);

// Get proxy queue reference

	ProxyQueue_t *proxyQueue;
	switch(proxyReq->hostReq.hdr.svcType)
	{
		default:
			SysLog(LogError, "SvcType=%s is not valid", HostSvcTypeToString(proxyReq->hostReq.hdr.svcType));
			return -1;
			break;

		case eSvcConfig:
			proxyQueue = sess->_priv.authQueue;
			break;

		case eSvcAuth:
			proxyQueue = sess->_priv.authQueue;
			break;

		case eSvcEdc:
			proxyQueue = sess->_priv.edcQueue;
			break;

		case eSvcEdcMulti:
			proxyQueue = sess->_priv.edcQueue;
			break;

		case eSvcT70:
			proxyQueue = sess->_priv.t70Queue;
			break;
	}

	proxyQueue->lastProxyRecvTime = GetMsTime();
	if (--proxyQueue->nbrInflights < 0)	// remove inflight
		proxyQueue->nbrInflights = 0;	// keep it sane

	if (ProxyQueueAddInputRequest(proxyQueue, proxyReq) != 0)
	{
		PiException(LogError, "ProxyQueueAddInputRequest", eFailure, "failed");
		return sess->_priv.finalDisposition;
	}

	PiApiResult_t result = PiSendResponse(sess);

	SysLog(LogDebug, "ApiResult %s", PiApiResultToString(result));
	switch (result)
	{
	default:
		PiException(LogError, "PiSendResponse", result, "PiSendResponse failed: %s", PiApiResultToString(result));
		PosDisconnect(sess, false);
		return -1;
		break;

	case eVirtual:				// A virtual plugin
		PiException(LogError, "PiSendResponse", result, "PiSendResponse failed: %s", PiApiResultToString(result));
		PosDisconnect(sess, false);
		return 1;
		break;

	case eFailure:				// Abnormal (FATAL) event
		PiException(LogError, "PiSendResponse", result, "PiSendResponse failed: %s", PiApiResultToString(result));
		PosDisconnect(sess, false);
		return 1;
		break;

	case eDisconnect:			// begin disconnect processing
		PosDisconnect(sess, false);
		return 1;
		break;

	case eOk:
	case eWaitForData:
		SysLog(LogDebug, "set clear-to-send to true");
		proxyQueue->cleartosend = true;
		break;		// fall through...
	}

	if ( sess != NULL )
	{
		if (ProxyPump(sess) < 0)	// send next host request, if any
		{
			PiException(LogError, "ProxyPump", eFailure, "failed");
			PosDisconnect(sess, false);
			return -1;
		}
	}

	return 0;	// AOK
}


static int
ProxyPump(PiSession_t *sess)
{
	PiSessionVerify(sess);
	Context->currentSession = sess;

	if ( Context->authProxyEnabled )
	{
		if (ProxyQueuePump(sess->_priv.authQueue) < 0)	// send next Auth request, if any
		{
			PiException(LogError, "ProxyQueuePump", eFailure, "failed");
			return 1;
		}
	}

	if ( Context->edcProxyEnabled )
	{
		if (ProxyQueuePump(sess->_priv.edcQueue) < 0)	// send next Edc request, if any
		{
			PiException(LogError, "ProxyQueuePump", eFailure, "failed");
			return 1;
		}
	}

	if ( Context->t70ProxyEnabled )
	{
		if (ProxyQueuePump(sess->_priv.t70Queue) < 0)	// send next T70 request, if any
		{
			PiException(LogError, "ProxyQueuePump", eFailure, "failed");
			return 1;
		}
	}

	return 0;
}


static int
IntegrityCheck()
{

	if ( ProxyConnect() != 0 )
		SysLog(LogError, "ProxyConnect failed");

	if ( Context->checkForStalls )
	{
		int maxHostWaitTime = (ProcGetPropertyIntValue(NxCurrentProc, "MaxHostWaitTime") * 1000);
		int maxPosWaitTime = (ProcGetPropertyIntValue(NxCurrentProc, "MaxPosWaitTime") * 1000);

	// SysLog(LogDebug, "Checking for stalled sessions");

	// Check each active client session
		NxTime_t ct = GetMsTime();

		for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(Context->clientConnections->connectionList, entry)) != NULL;)
		{
			PiSession_t *sess = PiGetSession((NxClient_t *)entry->var);
			PiSessionVerify(sess);
			Context->currentSession = sess;

			PiSessionSetLogPrefix(sess);

	// Check for stalled proxy requests
		// T70
			unsigned long t = (ct - sess->_priv.t70Queue->lastProxySendTime);	// time interval since last host send
			if (t > maxHostWaitTime && ObjectListNbrEntries(sess->_priv.t70Queue->outputQueue) > 0)
			{
				PiException(LogError, (char*)__FUNC__, eFailure, "session has %d stalled pending host requests", ObjectListNbrEntries(sess->_priv.t70Queue->outputQueue));
				if (PosDisconnect(sess, false) != 0)
					SysLog(LogError, "PosDisconnect failed");
				if ( T70ProxyDisconnect() != 0 )
					SysLog(LogError, "T70ProxyDisconnect failed");
				entry = NULL;
				continue;			// this guy is toast
			}

		// Auth
			t = (ct - sess->_priv.authQueue->lastProxySendTime);	// time interval since last host send
			if (t > maxHostWaitTime && ObjectListNbrEntries(sess->_priv.authQueue->outputQueue) > 0)
			{
				PiException(LogError, (char*)__FUNC__, eFailure, "session has %d stalled pending host requests", ObjectListNbrEntries(sess->_priv.authQueue->outputQueue));
				if (PosDisconnect(sess, false) != 0)
					SysLog(LogError, "PosDisconnect failed");
				if ( AuthProxyDisconnect() != 0 )
					SysLog(LogError, "AuthProxyDisconnect failed");
				entry = NULL;
				continue;			// this guy is toast
			}

		// Edc
			t = (ct - sess->_priv.edcQueue->lastProxySendTime);	// time interval since last host send
			if (t > maxHostWaitTime && ObjectListNbrEntries(sess->_priv.edcQueue->outputQueue) > 0)
			{
				PiException(LogError, (char*)__FUNC__, eFailure, "session has %d stalled pending host requests", ObjectListNbrEntries(sess->_priv.edcQueue->outputQueue));
				if (PosDisconnect(sess, false) != 0)
					SysLog(LogError, "PosDisconnect failed");
				if ( EdcProxyDisconnect() != 0 )
					SysLog(LogError, "EdcProxyDisconnect failed");
				entry = NULL;
				continue;			// this guy is toast
			}

	// Check for stalled pos data
			t = (ct - sess->_priv.lastPosRecvTime);	// time interval since last pos recv
			if (t > maxPosWaitTime && sess->_priv.inputFifo->len > 0)
			{
				PiException(LogError, (char*)__FUNC__, eFailure, "has %d stalled pos data chars", sess->_priv.inputFifo->len);
				if (PosDisconnect(sess, false) != 0)
					SysLog(LogError, "PosDisconnect failed");
				entry = NULL;
				continue;			// this guy is toast
			}
		}
	}

	return 0;
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
		EnumToString(SteadyState);
		EnumToString(ShutdownState);
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
		EnumToString(ShutdownTimerEvent);
		EnumToString(PosReadReadyEvent);
		EnumToString(WorkReadReadyEvent);
		EnumToString(AuthReadReadyEvent);
		EnumToString(EdcReadReadyEvent);
		EnumToString(T70ReadReadyEvent);
		EnumToString(ExceptionEvent);
	}

	return text;
}


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
		if (ProxyConnect() != 0)
			SysLog(LogError, "ProxyConnect failed");

		TimerSet(RecoveryWaitInterval);
		FsmSetNewState(this, SteadyState);
		break;

	case WorkReadReadyEvent:
		PosAcceptDisconnect();		// not ready
		break;
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
		TimerSet(0);
		PosDisconnectAll();
		SysLog(LogFatal, "Exception. All is lost...");
		break;

	case TimerEvent:
		IntegrityCheck();

	// if we previously suppressed the work queue and we have fallen below 10% of high
	// renable the queue
		if ( Context->workQueueSuppressed )
		{
			if ( EventGlobalJoinedFdCount(NxGlobal->efg) < ((NxGlobal->maxFd*9)/10) )
			{
				SysLog(LogWarn, "Receeded %d connections; renabling connects", NxGlobal->maxFd);
				if ( NxClientSetEventMask(Context->workQueue, EventFileLowPri, EventReadMask, HandleWorkQueueEvent, Context->workQueue) != 0 )
					SysLog(LogFatal, "NxClientSetEventMask of %s failed", NxClientNameToString(Context->workQueue));
				Context->workQueueSuppressed = false;
			}
		}

		TimerSet(SteadyWaitInterval);	// next timer
		break;

	case WorkReadReadyEvent:
		{
			PiSession_t *sess = PosAccept();
			if (sess == NULL)
			{
				if (errno != EAGAIN)
					SysLog(LogError, "PosAccept failed");
				break;
			}
			SysLog(LogDebug, "Connected");
		
			if ( ProxyConnect() != 0 )
				SysLog(LogError, "ProxyConnect failed");
			break;
		}

	case PosReadReadyEvent:
		{
			PiSession_t *sess = (PiSession_t*)efarg;
			PiSessionVerify(sess);
			Context->currentSession = sess;

			if (PosRecv(sess) < 0)
			{
				PiException(LogError, "PosRecv", eFailure, "failed");
				if (PosDisconnect(sess, false) != 0)
					SysLog(LogError, "PosDisconnect failed");
			}
			break;
		}

	case T70ReadReadyEvent:
		{
			static ProxyRequest_t *proxyReq = NULL;
			if ( proxyReq == NULL )
				proxyReq = ProxyRequestNew();

			if (T70ProxyRecv(proxyReq) != 0)
			{
				SysLog(LogError, "T70ProxyRecv failed: %s", NxClientNameToString(Context->t70Proxy));
				T70ProxyDisconnect();
				PosDisconnectAll();
			}
			else
			{
				switch (proxyReq->reqType)
				{
				default:
					if ( ProxyRecv(proxyReq) < 0 )
						SysLog(LogError, "ProxyRecv failed");
					break;

				case ProxyReqHostOffline:
					SysLog(LogError, "%s: host reports offline", NxClientNameToString(Context->t70Proxy));
					break;

				case ProxyReqHostOnline:
					break;
				}
			}
			break;
		}

	case AuthReadReadyEvent:
		{
			static ProxyRequest_t *proxyReq = NULL;
			if ( proxyReq == NULL )
				proxyReq = ProxyRequestNew();

			if (AuthProxyRecv(proxyReq) != 0)
			{
				SysLog(LogError, "AuthProxyRecv failed: %s", NxClientNameToString(Context->authProxy));
				AuthProxyDisconnect();
				PosDisconnectAll();
			}
			else
			{
				switch (proxyReq->reqType)
				{
				default:
					if ( ProxyRecv(proxyReq) < 0 )
						SysLog(LogError, "ProxyRecv failed");
					break;

				case ProxyReqHostOffline:
					SysLog(LogError, "%s: host reports offline", NxClientNameToString(Context->authProxy));
					break;

				case ProxyReqHostOnline:
					break;
				}
			}
			break;
		}

	case EdcReadReadyEvent:
		{
			static ProxyRequest_t *proxyReq = NULL;
			if ( proxyReq == NULL )
				proxyReq = ProxyRequestNew();

			if (EdcProxyRecv(proxyReq) != 0)
			{
				SysLog(LogError, "EdcProxyRecv failed: %s", NxClientNameToString(Context->edcProxy));
				EdcProxyDisconnect();
				PosDisconnectAll();
			}
			else
			{
				switch (proxyReq->reqType)
				{
				default:
					if ( ProxyRecv(proxyReq) < 0 )
						SysLog(LogError, "ProxyRecv failed");
					break;

				case ProxyReqHostOffline:
					SysLog(LogError, "%s: host reports offline", NxClientNameToString(Context->edcProxy));
					break;

				case ProxyReqHostOnline:
					break;
				}
			}
			break;
		}
	}
}


static void
FsmShutdownState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch (evt)
	{
	default:
		SysLog(LogError, "%s is not a known event for this state", FsmEventToString(this, evt));
	case ExceptionEvent:
		TimerSet(0);
		PosDisconnectAll();
		SysLog(LogFatal, "Exception. All is lost...");
		break;

	case TimerEvent:
		IntegrityCheck();

		SysLog(LogDebug, "Checking for idle");
		if (HashMapLength(Context->clientConnections->connectionList) > 0)
			SysLog(LogAny, "%d sessions active. Shutdown deferred", HashMapLength(Context->clientConnections->connectionList));
		else
			FinishShutdown();
		TimerSet(SteadyWaitInterval);	// next timer
		break;

	case ShutdownTimerEvent:
		SysLog(LogError, "Shutdown timer expired; forcing shutdown");
		FinishShutdown();		// forced
		break;

	case T70ReadReadyEvent:
	case AuthReadReadyEvent:
	case EdcReadReadyEvent:
		FsmSteadyState(this, evt, efarg);		// do steady state processing
		break;
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

	case SteadyState:
		FsmSteadyState(this, evt, efarg);
		break;

	case ShutdownState:
		FsmShutdownState(this, evt, efarg);
		break;
	}
}


static void
InitiateShutdown()
{
	SysLog(LogAny, "Initiating shutdown");

	SysLogUnRegisterWriter(NULL, LogWriter, NULL);

// Close the work queue
	if ( Context->workQueue != NULL )
		NxClientDelete(Context->workQueue);

	PosShutdownAll();

	Context->shutdownStartTime = GetMsTime();
	ShutdownTimerSet(ProcGetPropertyIntValue(NxCurrentProc, "ShutdownWaitTime")*1000);
	FsmSetNewState(Context->fsm, ShutdownState);
}


static void
FinishShutdown()
{
	SysLog(LogAny, "Continuing shutdown");

	TimerCancelAll();

	PosDisconnectAll();

	NxClientDisconnect(Context->authProxy);
	NxClientDelete(Context->authProxy);

	NxClientDisconnect(Context->edcProxy);
	NxClientDelete(Context->edcProxy);

	NxClientDisconnect(Context->t70Proxy);
	NxClientDelete(Context->t70Proxy);

	ContextDelete(Context);

	ProcStop(NxCurrentProc, 0);			// only returns on error

	SysLog(LogError, "ProcStop failed");

	kill(getpid(), SIGABRT);	// try for a core
}


static CommandResult_t
CommandHandler(NxClient_t *client, Parser_t *parser, String_t *response)
{
	CommandDef_t cmds[] = {
		{NULL, NULL}
	};

	Command_t *cmd = CommandNew(client, cmds);

	int ret = CommandExecute(cmd, parser, "", response);

	CommandDelete(cmd);

	return ret;
}


static ProcContext_t*
ProcContextConstructor(ProcContext_t *this, char *file, int lno, NxClient_t *workQueue)
{
	NxClientVerify(workQueue);

	this->workQueue = workQueue;
	NxClientSetEventMask(workQueue, EventFileLowPri, EventReadMask, HandleWorkQueueEvent, workQueue);

	this->currentTimer = TimerNew("%sTimer", NxCurrentProc->name);

	this->currentShutdownTimer = TimerNew("%sShutdownTimer", NxCurrentProc->name);

	this->authProxy = NxClientNew();
	
	this->edcProxy = NxClientNew();

	this->t70Proxy = NxClientNew();

	this->checkForStalls = ProcGetPropertyBooleanValue(NxCurrentProc, "CheckForStalls");
	this->authProxyEnabled = NxGetPropertyBooleanValue("AuthProxy.Enabled");
	this->edcProxyEnabled = NxGetPropertyBooleanValue("EdcProxy.Enabled");
	this->t70ProxyEnabled = NxGetPropertyBooleanValue("T70Proxy.Enabled");

	// Create the client listener (handles worker sessions)
	this->clientConnections = NxServerNew();

	this->workQueueSuppressed = false;
	return this;
}


static void
ProcContextDestructor(ProcContext_t *this, char *file, int lno)
{
	if ( this->workQueue != NULL )
		NxClientDelete(this->workQueue);

	if ( this->authProxy != NULL )
		NxClientDelete(this->authProxy);

	if ( this->edcProxy != NULL )
		NxClientDelete(this->edcProxy);

	if ( this->t70Proxy!= NULL )
		NxClientDelete(this->t70Proxy);

	if ( this->currentTimer != NULL )
		TimerDelete(this->currentTimer);

	if ( this->currentShutdownTimer != NULL )
		TimerDelete(this->currentShutdownTimer);

	NxServerDelete(this->clientConnections);
}


static Json_t*
ProcContextSerialize(ProcContext_t *this)
{
	ContextVerify(this);

	Json_t *root = JsonNew(__FUNC__);

	JsonAddItem(root, "Fsm", FsmSerialize(this->fsm));
	JsonAddBoolean(root, "CheckForStalls", BooleanToString(this->checkForStalls));
	JsonAddBoolean(root, "AuthProxyEnabled", BooleanToString(this->authProxyEnabled));
	JsonAddBoolean(root, "EdcProxyEnabled", BooleanToString(this->edcProxyEnabled));
	JsonAddBoolean(root, "T70ProxyEnabled", BooleanToString(this->t70ProxyEnabled));
	JsonAddBoolean(root, "WorkqueueEnabled", BooleanToString(!this->workQueueSuppressed));
	JsonAddNumber(root, "MaxHostWaitTime", ProcGetPropertyIntValue(NxCurrentProc, "MaxHostWaitTime"));
	JsonAddNumber(root, "MaxPosWaitTime", ProcGetPropertyIntValue(NxCurrentProc, "MaxPosWaitTime"));
	JsonAddNumber(root, "ShutdownWaitTime", ProcGetPropertyIntValue(NxCurrentProc, "ShutdownWaitTime"));
	if (this->shutdownStartTime != 0 )
		JsonAddString(root, "ShutdownTime", MsTimeToStringShort(this->shutdownStartTime, NULL));

	if (this->currentTimer != NULL && this->currentTimer->active)
		JsonAddItem(root, "Timer", TimerSerialize(this->currentTimer));

	if (this->currentShutdownTimer != NULL && this->currentShutdownTimer->active)
		JsonAddItem(root, "Timer", TimerSerialize(this->currentShutdownTimer));

	JsonAddItem(root, "AuthConnection", NxClientSerialize(this->authProxy));
	JsonAddItem(root, "EdcConnection", NxClientSerialize(this->edcProxy));
	JsonAddItem(root, "T70Connection", NxClientSerialize(this->t70Proxy));
	JsonAddItem(root, "WorkQueue", NxClientSerialize(this->workQueue));

#if 0
	{ // The services I can handle
		ObjectLock(FecConfigGlobal);				// lock it

		// first, count them...
		int n = 0;
		for (int i = 0; i < FecConfigGlobal->numberServices; ++i )
		{
			FecService_t *service = &FecConfigGlobal->services[i];
			if ( service->status == FecConfigVerified )
				++n;
		}

		Json_t *sub = JsonPushObject(root, "%d services", n);

		// then show detail of services
		for (int i = 0; i < FecConfigGlobal->numberServices; ++i )
		{
			FecService_t *service = &FecConfigGlobal->services[i];
			if ( service->status == FecConfigVerified )
			{
				JsonAddItem(sub, service->properties.protocol, FecConfigServiceSerialize(service));
				JsonAddString(sub, "PlugIn", PiProfileToString(&service->plugin));

// Host counts
				Json_t *cnts = JsonPushObject(sub, "HostCounts");
				JsonAddNumber(cnts, "InChrs", (double)service->plugin.hostCounts.inChrs);
				JsonAddNumber(cnts, "InPkts", (double)service->plugin.hostCounts.inPkts);
				JsonAddNumber(cnts, "OutChrs", (double)service->plugin.hostCounts.outChrs);
				JsonAddNumber(cnts, "OutPkts", (double)service->plugin.hostCounts.outPkts);

// Terminal counts
				cnts = JsonPushObject(sub, "TerminalCounts");
				JsonAddNumber(cnts, "InChrs", (double)service->plugin.posCounts.inChrs);
				JsonAddNumber(cnts, "InPkts", (double)service->plugin.posCounts.inPkts);
				JsonAddNumber(cnts, "OutChrs", (double)service->plugin.posCounts.outChrs);
				JsonAddNumber(cnts, "OutPkts", (double)service->plugin.posCounts.outPkts);
			}
		}

		ObjectUnlock(FecConfigGlobal);				// unlock it
	}
#endif

	{ // The client connections
		if ( this->clientConnections->connectionList->length > 0 )
		{
			Json_t *sub = JsonPushObject(root, "ClientConnection%s", this->clientConnections->connectionList->length?"s":"");
			ObjectList_t* list = HashGetOrderedList(this->clientConnections->connectionList, ObjectListUidType);
			for ( HashEntry_t *entry = NULL; (entry = ObjectListRemove(list, ObjectListFirstPosition)); )
			{
				PiSession_t *sess = PiGetSession((NxClient_t *)entry->var);
				PiSessionVerify(sess);
				NxClient_t *client = PiGetClient(sess);
				JsonAddItem(sub, NxClientNameToString(client), NxClientSerialize(client));
				HashEntryDelete(entry);
			}
			ObjectListDelete(list);
		}
	}

	return root;
}


static char *
ProcContextToString(ProcContext_t *this)
{
	Json_t *root = ProcContextSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


static void
LogWriter(SysLog_t *log, void **contextp, SysLogLevel lvl, char *file, int lno, char *fnc, char *bfr, int len)
{
	PiSession_t *sess = Context->currentSession;

// create an exception if this is a error, and it's an active session
	if ( (lvl & 0xFFFF) < LogWarn && ObjectTestVerify(PiSession, sess) )
	{
		PiSessionSetLogPrefix(sess);
		_PiException(sess, lvl, fnc, eFailure, false, file, lno, fnc, bfr, PiSessionGetName(sess));
	}
}


int
ProcWorkerStart(Proc_t *this, va_list ap)
{

	NxClient_t *workQueue = va_arg(ap, NxClient_t*);
	NxClientVerify(workQueue);

	this->context = ContextNew(workQueue);
	this->commandHandler = CommandHandler;
	this->contextSerialize = ProcContextSerialize;
	this->contextToString = ProcContextToString;

	SysLogRegisterWriter(NULL, LogWriter, this);

	Context->fsm = FsmNew(FsmEventHandler, FsmStateToString, FsmEventToString, NullState, "%sFsm", this->name);

	ProcSetSignalHandler(this, SIGTERM, SigTermHandler);	// Termination signal

	// Start the background clock
	TimerSet(InitialWaitInterval);

	return 0;
}
