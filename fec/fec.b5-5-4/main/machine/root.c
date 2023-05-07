/*****************************************************************************

Filename:   main/machine/root.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:34:01 $
 * $Header: /home/hbray/cvsroot/fec/main/machine/root.c,v 1.3.4.17 2011/10/27 18:34:01 hbray Exp $
 *
 $Log: root.c,v $
 Revision 1.3.4.17  2011/10/27 18:34:01  hbray
 Revision 5.5

 Revision 1.3.4.16  2011/09/26 15:52:33  hbray
 Revision 5.5

 Revision 1.3.4.15  2011/09/24 18:30:52  hbray
 *** empty log message ***

 Revision 1.3.4.14  2011/09/24 17:49:53  hbray
 Revision 5.5


 Revision 1.2.2.1  2011/07/27 20:19:57  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: root.c,v 1.3.4.17 2011/10/27 18:34:01 hbray Exp $ "


#include <sys/wait.h>

#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/signatures.h"

#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/hostio.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"
#include "include/logwriter.h"

#include "machine/include/root.h"
#include "machine/include/web.h"
#include "machine/include/t70proxy.h"
#include "machine/include/stratusproxy.h"
#include "machine/include/service.h"
#include "machine/include/worker.h"
#include "machine/include/hostsim.h"
#include "machine/include/t70sim.h"



// Local Data Types


#define Context ((NxCurrentProc)->context)


typedef enum
{
	NullState, ConnectingToAuthState, ConfiguringState, AuthOfflineState, ShutdownState, SteadyState
} FsmState;

typedef enum
{
	SyslogRecycleTimerEvent, TimerEvent, AuthReadReadyEvent, AuthWriteReadyEvent, ExceptionEvent
} FsmEvent;



typedef struct ProcContext_t
{
	Fsm_t				*fsm;

	NxClient_t			*authProxySession;
	NxClient_t			*txWorkQueue;
	NxClient_t			*rxWorkQueue;

	Proc_t				*webProc;
	Proc_t				*authProxyProc;
	Proc_t				*edcProxyProc;
	Proc_t				*t70ProxyProc;
	Proc_t				*authSimProc;
	Proc_t				*edcSimProc;
	Proc_t				*t70SimProc;

	// Hash list of active Service
	HashMap_t			*serviceProcList;	// List of Proc_t*; keyed by name

	// Hash list of active Worker
	int					nextWorkerNbr;
	HashMap_t			*workerProcList;	// List of Proc_t*; keyed by name

	Timer_t				*currentTimer;
	Timer_t				*syslogRecycleTimer;

	// the config being built during init
	FILE				*configFile;
	FecConfig_t			*trialFecConfig;

	boolean				webEnabled;
	boolean				authProxyEnabled;
	boolean				edcProxyEnabled;
	boolean				t70ProxyEnabled;
	boolean				authSimEnabled;
	boolean				edcSimEnabled;
	boolean				t70SimEnabled;

	NxTime_t			shutdownStartTime;
	int					idleRetries;					// nbr times I have tried to quiesce (either reload or shutdown)
	int					minIdleRetries;
	int					maxIdleRetries;

	int					workerPids[MAXWORKERS];
	int					servicePids[MAXSERVICES];
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
static void HandleAuthEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandleTimerEvent(Timer_t *tid);
static void HandleSyslogRecycleTimerEvent(Timer_t *tid);
static void SigTermHandler(NxSignal_t*);

// Fsm Functions
//
static char *FsmEventToString(Fsm_t *this, int evt);
static char *FsmStateToString(Fsm_t *this, int state);
static void FsmConfiguringState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmConnectingToAuthState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmEventHandler(Fsm_t *this, int evt, void * efarg);
static void FsmNullState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmAuthOfflineState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmShutdownState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmSteadyState(Fsm_t *this, FsmEvent evt, void * efarg);

// Helper Functions
//
static void LogRecycle(SysLog_t *this, void **contextp, boolean force);
static void SetLogRecycleTimer();
static int ChildPrelude(Proc_t *rootProc);
static int ProcessConfigLine(char *text);
static int OpenWorkQueue();
static void AuthSimStart();
static void AuthProxyStart();
static int AuthProxyConnect();
static boolean AuthProxyIsConnected();
static int AuthProxyDisconnect();
static int AuthProxyRecv(ProxyRequest_t *proxyReq);
static int AuthProxySend(ProxyRequest_t *proxyReq, NxClient_t *client);
static void EdcProxyStart();
static void T70SimStart();
static void T70ProxyStart();
static void WebServerStart();
static void StartSingletons();
static void StartServices();
static void StartWorkers();
static void StartAllChildren();
static int SendConfigurationRequest();
static void KillServices(int signo);
static void KillWorkers(int signo);
static int ChildTerminated(int pid);
static void CheckChildTerminated();
static void SetTimer(int ms);
static void InitiateShutdown();
static void FinishShutdown();

static CommandResult_t ReloadCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static CommandResult_t CommandHandler(NxClient_t *client, Parser_t *parser, String_t *response);


// Static Global Vars
//
static const int InitialWaitInterval = (10 * 1000);	// allow enough time for AuthProxy spin up
static const int RecoveryWaitInterval = (30 * 1000);
static const int ReloadWaitInterval = (10 * 1000);
static const int ShutdownWaitInterval = (10 * 1000);
static const int MinShutdownWaitTime = (60 * 1000);

// Functions Start Here


static void
SigTermHandler(NxSignal_t *sig)
{
	InitiateShutdown();
}


static void
HandleAuthEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, AuthReadReadyEvent, evf);

	if (pollMask & EventWriteMask)
		_FsmDeclareEvent(fsm, AuthWriteReadyEvent, evf);
}


static void
HandleTimerEvent(Timer_t *tid)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	_FsmDeclareEvent(fsm, TimerEvent, tid);
}


static void
HandleSyslogRecycleTimerEvent(Timer_t *tid)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	_FsmDeclareEvent(fsm, SyslogRecycleTimerEvent, tid);
}


static int
OpenWorkQueue()
{
	if ( NxClientPair(Context->txWorkQueue, Context->rxWorkQueue) < 0)
	{
		SysLog(LogError, "NxClientPair failed");
		return -1;
	}
	SysLog(LogDebug, "Left: %s", NxClientToString(Context->txWorkQueue));
	SysLog(LogDebug, "Right: %s", NxClientToString(Context->rxWorkQueue));

	return 0;
}


static int
AuthProxyConnect()
{

	// Open AuthProxy service port

	SysLog(LogDebug, "Connecting to AuthProxy");

	{
		char tmp[1024];
		sprintf(tmp, "%s/%s", NxGlobal->nxDir, NxGetPropertyValue("AuthProxy.InputQueue"));

		if ( NxClientConnect(Context->authProxySession, AF_UNIX, SOCK_STREAM, tmp, 0, EventFileHighPri, EventReadMask, HandleAuthEvent) < 0 )
		{
			SysLog(LogError, "NxClientConnect failed: %s", tmp);
			AuthProxyDisconnect();
			return -1;
		}
	}
	return 0;
}


static boolean
AuthProxyIsConnected()
{

	if (NxClientIsConnected(Context->authProxySession))
	{
		SysLog(LogDebug, "AuthProxy %s connection complete", NxClientNameToString(Context->authProxySession));
		return true;
	}

	SysLog(LogDebug, "AuthProxy %s connection pending", NxClientNameToString(Context->authProxySession));
	return false;
}


static int
AuthProxyDisconnect()
{
	SysLog(LogDebug, "Disconnecting AuthProxy %s", NxClientNameToString(Context->authProxySession));
	NxClientDisconnect(Context->authProxySession);
	return 0;
}


static int
AuthProxySend(ProxyRequest_t *proxyReq, NxClient_t *client)
{
	NxClientVerify(client);

	SysLog(LogDebug, "Sending %d bytes for %s", proxyReq->hostReq.len, NxClientNameToString(client));

	EventFileVerify(client->evf);
	proxyReq->servicePort = client->evf->servicePort;
	proxyReq->pid = getpid();
	proxyReq->uid = client->evf->uid;

	SysLog(LogDebug, "proxyReq=%s", ProxyRequestToString(proxyReq, DumpOutput));
	int len = ProxyRequestLen(proxyReq);

	if (NxClientSendPkt(Context->authProxySession, (char *)proxyReq, len) != len )
	{
		SysLog(LogError, "NxClientSendPkt failed: %s", NxClientNameToString(Context->authProxySession));
		return -1;
	}

	return 0;
}


static int
AuthProxyRecv(ProxyRequest_t *proxyReq)
{
	int rlen;

	if ((rlen = NxClientRecvPkt(Context->authProxySession, (char *)proxyReq, sizeof(*proxyReq))) < 0)
	{
		SysLog(LogError, "NxClientRecvPkt failed: %s", NxClientNameToString(Context->authProxySession));
		return -1;
	}

	if (rlen < sizeof(*proxyReq) - sizeof(proxyReq->hostReq.data))
	{
		SysLog(LogError, "Short read of %d from %s; failed", rlen, NxClientNameToString(Context->authProxySession));
		return -1;
	}

	SysLog(LogDebug, "proxyReq=%s", ProxyRequestToString(proxyReq, DumpOutput));
	return 0;
}


static void
SetTimer(int ms)
{
	TimerCancel(Context->currentTimer);

	if (ms > 0)
	{
		if (TimerActivate(Context->currentTimer, ms, HandleTimerEvent) != 0)
			SysLog(LogError, "TimerActivate failed");
	}
}


static void
SetLogRecycleTimer()
{
	TimerCancel(Context->syslogRecycleTimer);

	if (TimerActivate(Context->syslogRecycleTimer, (NxGetPropertyIntValue("SysLog.MaxFileTime")-30)*1000, HandleSyslogRecycleTimerEvent) != 0)
		SysLog(LogError, "TimerActivate failed");
}


static void
WebServerStart()
{

	if ( Context->fsm->currentState == ShutdownState )
		return;			// in shutdown mode

	if ( ! Context->webEnabled )
		return;			// not enabled

	if (Context->webProc == NULL)		// not running
	{
		// Start the Web Server
		//
		Proc_t *proc = ProcNew(NxCurrentProc, "Web", ProcTagWeb);
		if ( proc == NULL )
			SysLog(LogFatal, "Creation of Web failed");
		//LATER: ProcStart(proc, ChildPrelude, ProcWebStart);
		Context->webProc = proc;
	}
}


static void
T70SimStart()
{
	if ( Context->fsm->currentState == ShutdownState )
		return;			// in shutdown mode

	if ( ! Context->t70SimEnabled)
		return;

	if (Context->t70SimProc == NULL)		// not running
	{
		Proc_t *proc = ProcNew(NxCurrentProc, "T70Sim", ProcTagT70Sim);
		if ( proc == NULL )
			SysLog(LogFatal, "Creation of T70Sim failed");
		//LATER: ProcStart(proc, ChildPrelude, ProcT70SimStart);
		Context->t70SimProc = proc;
	}
}


static void
T70ProxyStart()
{

	if ( Context->fsm->currentState == ShutdownState )
		return;			// in shutdown mode

	if ( ! Context->t70ProxyEnabled )
		return;			// not enabled

	if (Context->t70ProxyProc == NULL)
	{
		// Start the T70Proxy Server
		//
		Proc_t *proc = ProcNew(NxCurrentProc, "T70Proxy", ProcTagT70Proxy);
		if ( proc == NULL )
			SysLog(LogFatal, "Creation of T70Proxy failed");
		//LATER: ProcStart(proc, ChildPrelude, ProcT70ProxyStart);
		Context->t70ProxyProc = proc;
	}
}


static void
AuthSimStart()
{

	if ( Context->fsm->currentState == ShutdownState )
		return;			// in shutdown mode

	if ( ! Context->authSimEnabled )
		return;			// not enabled

	// Optional Auth Simulator
	//
	if (Context->authSimProc == NULL )		// not running
	{
		Proc_t *proc = ProcNew(NxCurrentProc, "AuthSim", ProcTagAuthSim);
		if ( proc == NULL )
			SysLog(LogFatal, "Creation of AuthSim failed");
		ProcStart(proc, ChildPrelude, ProcHgSimStart);
		Context->authSimProc = proc;
	}
}


static void
EdcSimStart()
{

	if ( Context->fsm->currentState == ShutdownState )
		return;			// in shutdown mode

	if ( ! Context->edcSimEnabled )
		return;			// not enabled

	// Optional Edc Simulator
	//
	if (Context->edcSimProc == NULL )		// not running
	{
		Proc_t *proc = ProcNew(NxCurrentProc, "EdcSim", ProcTagEdcSim);
		if ( proc == NULL )
			SysLog(LogFatal, "Creation of EdcSim failed");
		ProcStart(proc, ChildPrelude, ProcHgSimStart);
		Context->edcSimProc = proc;
	}
}


static void
AuthProxyStart()
{

	if ( Context->fsm->currentState == ShutdownState )
		return;			// in shutdown mode

	if ( ! Context->authProxyEnabled )
		return;			// not enabled

	if (Context->authProxyProc == NULL)
	{
		// Start the AuthProxy
		//
		Proc_t *proc = ProcNew(NxCurrentProc, "AuthProxy", ProcTagAuthProxy);
		if ( proc == NULL )
			SysLog(LogFatal, "Creation of AuthProxy failed");
		ProcStart(proc, ChildPrelude, ProcStratusProxyStart);
		Context->authProxyProc = proc;
	}
}


static void
EdcProxyStart()
{

	if ( Context->fsm->currentState == ShutdownState )
		return;			// in shutdown mode

	if ( ! Context->edcProxyEnabled )
		return;			// not enabled

	if (Context->edcProxyProc == NULL)
	{
		// Start the EdcProxy
		//
		Proc_t *proc = ProcNew(NxCurrentProc, "EdcProxy", ProcTagEdcProxy);
		if ( proc == NULL )
			SysLog(LogFatal, "Creation of EdcProxy failed");
		ProcStart(proc, ChildPrelude, ProcStratusProxyStart);
		Context->edcProxyProc = proc;
	}
}


static void
StartSingletons()
{

	if (Context->fsm->currentState == ShutdownState )
		return;				// we're in shutdown mode

	CheckChildTerminated();
	WebServerStart();
	AuthProxyStart();
	EdcProxyStart();
	T70ProxyStart();
}


static void
InitiateShutdown()
{
	SysLog(LogWarn, "Initiating shutdown");
	KillServices(SIGTERM);
	KillWorkers(SIGTERM);

	Context->shutdownStartTime = GetMsTime();

	// calculate how many idle attempts from the idle interval and max wait time
	Context->maxIdleRetries = ProcGetPropertyIntValue(NxCurrentProc, "ShutdownWaitTime") / (ShutdownWaitInterval/1000);
	Context->maxIdleRetries = min(Context->minIdleRetries, Context->maxIdleRetries);
	Context->idleRetries = 0;
	SetTimer(ShutdownWaitInterval);
	FsmSetNewState(Context->fsm, ShutdownState);
}


static void
FinishShutdown()
{
	SysLog(LogWarn, "All idle");
	SysLog(LogWarn, "Finishing shutdown");
	KillServices(SIGKILL);
	KillWorkers(SIGKILL);
	kill(0, SIGKILL);		// kill the group
	exit(0);
}


static int
SendConfigurationRequest()
{

	FecConfigClear(Context->trialFecConfig); // clear the trial configuration

	{		// open the config shadow file
		char tmp[1024];
		sprintf(tmp, "%s/fec.ini", NxGlobal->nxDir);
		if ((Context->configFile = fopen(tmp, "w")) == NULL)
		{
			SysLog(LogError, "fopen of %s failed; errno=%s", tmp, ErrnoToString(errno));
			return -1;
		}
		if (chmod(tmp, 0666) < 0)
			SysLog(LogWarn, "chmod() error=%s: {%s}", ErrnoToString(errno), tmp);
	}

	static ProxyRequest_t *proxyReq = NULL;
	if ( proxyReq == NULL )
		proxyReq = ProxyRequestNew();
	ProxyRequestClear(proxyReq);

	proxyReq->reqType = ProxyReqHostMsg;
	proxyReq->flowType = ProxyFlowNone;
	proxyReq->replyTTL = ProcGetPropertyIntValue(NxCurrentProc, "ConfigTTL");
	proxyReq->uid = Context->authProxySession->evf->uid;

	memcpy(proxyReq->hostReq.hdr.peerIpAddr, Context->authProxySession->evf->peerIpAddr, sizeof(proxyReq->hostReq.hdr.peerIpAddr));
	proxyReq->hostReq.hdr.peerIpType[0] = '4';		// ip4
	proxyReq->hostReq.hdr.peerUid = NxClientGetUid(Context->authProxySession);
	memcpy(proxyReq->hostReq.hdr.peerName, NxClientNameToString(Context->authProxySession), sizeof(proxyReq->hostReq.hdr.peerName));
	proxyReq->hostReq.hdr.peerIpPort = Context->authProxySession->evf->peerPort;

	proxyReq->hostReq.hdr.svcType = eSvcConfig;
	proxyReq->hostReq.len = 0;

	SysLog(LogDebug, "Sending configuration request");

	if (AuthProxySend(proxyReq, Context->authProxySession) != 0)
	{
		if (Context->configFile != NULL)
			fclose(Context->configFile);
		Context->configFile = NULL;
		SysLog(LogError, "SendConfigRequest failed");
		return -1;
	}

	AuditSendEvent(AuditHostConfigStart, "connid", NxClientNameToString(Context->authProxySession));
	return 0;
}


static void
KillServices(int signo)
{
// stop the services
	for ( HashEntry_t *entry = NULL; (entry = HashGetNextEntry(Context->serviceProcList, entry)); )
	{
		Proc_t *child = (Proc_t *)entry->var;
		ProcSignal(child, signo);
	}
}


static void
KillWorkers(int signo)
{
// stop the workers
	for ( HashEntry_t *entry = NULL; (entry = HashGetNextEntry(Context->workerProcList, entry)); )
	{
		Proc_t *child = (Proc_t *)entry->var;
		ProcSignal(child, signo);
	}
}


static void
StartServices()
{

	if (Context->fsm->currentState == ShutdownState )
		return;				// we're in shutdown mode

	CheckChildTerminated();

	// start the services
	// also, stop any that should not be running due to a config change.
	for (int ii = 0; ii < FecConfigGlobal->numberServices; ++ii)
	{
		FecService_t *service = &FecConfigGlobal->services[ii];

		char name[64];

		sprintf(name, "Service_%s.%d", service->properties.protocol, service->properties.port);

		if (service->status == FecConfigVerified && service->plugin.loaded && (!service->plugin.isVirtual))	// ready to start
		{
			if (Context->servicePids[ii] != 0)
				continue;			// already active, skipping

			SysLog(LogDebug, "Starting %s as service %s", name, FecConfigServiceToString(service));

			NxClient_t *workQueue = NxClientNew();
			if (NxClientDupConnection(Context->txWorkQueue, workQueue) != 0)
			{
				SysLog(LogError, "NxClientDupConnection failed: %s", NxClientNameToString(Context->txWorkQueue));
				NxClientDelete(workQueue);
				return;
			}

			Proc_t *child = ProcNew(NxCurrentProc, name, (ProcTag_t)service);
			if ( ProcStart(child, ChildPrelude, ProcServiceStart, service->properties.port, workQueue) < 0 )
			{
				ProcDelete(child);
				SysLog(LogError, "Startup of %s failed", name);
				continue;
			}

			if (HashAddString(Context->serviceProcList, name, child) == NULL)
				SysLog(LogFatal, "HashAddString of %s failed", name);

			SysLog(LogDebug, "%s active", name);

			Context->servicePids[ii] = child->pid;

			NxClientDelete(workQueue);
		}
		else if ( (! service->plugin.isVirtual) && Context->servicePids[ii] != 0 )	// is active, and should not be
		{
			SysLog(LogWarn, "Service listener for %s is active and is not configured; stopping", FecConfigServiceToString(service));
			Proc_t *child = ProcFindChildByPid(NxCurrentProc, Context->servicePids[ii]);
			if ( child != NULL )
			{
				ProcSignal(child, SIGTERM);
			}
			else
			{
				SysLog(LogWarn, "Service %s shows an active listener; but, cannot find it; killing via SIGKILL", FecConfigServiceToString(service));
				kill(Context->servicePids[ii], SIGKILL);
			}
		}
	}
}


static void
StartWorkers()
{

	if (Context->fsm->currentState == ShutdownState )
		return;				// we're in shutdown mode

	CheckChildTerminated();

	// Start all the workers
	for (int ii = HashMapLength(Context->workerProcList); ii < FecConfigGlobal->properties.minWorkerPool; ++ii)
	{
		if (Context->workerPids[ii] != 0)
			continue;			// already active, skipping

		char name[64];

		sprintf(name, "Worker_%03d", (Context->nextWorkerNbr)++);

		SysLog(LogDebug, "Starting %s", name);

		NxClient_t *workQueue = NxClientNew();
		if (NxClientDupConnection(Context->rxWorkQueue, workQueue) != 0)
		{
			SysLog(LogError, "NxClientDupConnection failed: %s", NxClientNameToString(Context->rxWorkQueue));
			NxClientDelete(workQueue);
			return;
		}

		Proc_t *child = ProcNew(NxCurrentProc, name, ProcTagWorker);
		if ( ProcStart(child, ChildPrelude, ProcWorkerStart, workQueue) < 0 )
		{
			ProcDelete(child);
			SysLog(LogError, "Startup of %s failed", name);
			continue;
		}

		Context->workerPids[ii] = child->pid;

		if (HashAddString(Context->workerProcList, name, child) == NULL)
			SysLog(LogFatal, "HashAddString of %s failed", name);

		SysLog(LogDebug, "Started %s", name);

// need to close this file
		NxClientDelete(workQueue);
	}
}


static void
StartAllChildren()
{

	if (Context->fsm->currentState == ShutdownState )
		return;				// we're in shutdown mode

	StartSingletons();

	StartWorkers();

	StartServices();
}


static int
ProcessConfigLine(char *text)
{

	if (Context->configFile != NULL)
		fputs(text, Context->configFile);

	int rr = FecConfigParseString(Context->trialFecConfig, text);

	if (rr < 0)
	{
		SysLog(LogError, "FecConfigParseString failed");
		return -1;
	}

	if (rr != 0)				// end of config?
	{
		if (Context->configFile != NULL)
			fclose(Context->configFile);
		Context->configFile = NULL;

		{
			SysLog(LogDebug, "End of configuration load; attempting to commit");

			if (FecConfigCommit(FecConfigGlobal, Context->trialFecConfig) != 0)
			{
				SysLog(LogError, "FecConfigCommit failed");
				return -1;
			}

// do any overrides...
			int ii = ProcGetPropertyIntValue(NxCurrentProc, "MinNbrWorkers");
			if (ii > 0)
			{
				SysLog(LogWarn, "Changing properties.minWorkerPool from %d to %d", FecConfigGlobal->properties.minWorkerPool, ii);
				FecConfigGlobal->properties.minWorkerPool = ii;	// Override the minWorkerPool
			}

			SysLog(LogDebug, "End of configuration load; commit complete");
			SysLog(LogAny, "Configuration %s loaded", FecConfigToString(FecConfigGlobal));
			FecConfigSync(FecConfigGlobal);
		}
	}

	return rr;
}


static void
CheckChildTerminated()
{

// Check all my Chitlens
	for( boolean fini = false; ! fini; )
	{
		int status;

		while (waitpid(-1, &status, WNOHANG) > 0) ; // Prevent zombies

		fini = true;			// assume we're done
		for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(NxCurrentProc->childMap, entry)) != NULL;)
		{
			Proc_t *child = (Proc_t *)entry->var;
			if (kill(child->pid, 0) < 0 && errno == ESRCH)
			{
				if (ChildTerminated(child->pid) != 0)
					SysLog(LogError, "ChildTerminated(%d) failed", child->pid);
				fini = false;
				break;		// list is modified, break out of inner for...
			}
		}
	}
}


static int
ChildTerminated(int pid)
{

	SysLog(LogDebug, "Child %d terminated; Determing its name", pid);

	Proc_t *child = ProcFindChildByPid(NxCurrentProc, pid);

	if (child == NULL)
	{
		SysLog(LogDebug, "ProcFindChildByPid of %d failed", pid);
		return -1;
	}

	switch ((int)child->tag)
	{
	case ProcTagWeb:
		SysLog(LogWarn, "Web %s, pid=%d terminated", child->name, child->pid);
		ProcDelete(child);
		Context->webProc = NULL;	// no web
		break;

	case ProcTagAuthProxy:
		SysLog(LogWarn, "AuthProxy %s, pid=%d terminated", child->name, child->pid);
		ProcDelete(child);
		Context->authProxyProc = NULL;	// no AuthProxy
		break;

	case ProcTagEdcProxy:
		SysLog(LogWarn, "EdcProxy %s, pid=%d terminated", child->name, child->pid);
		ProcDelete(child);
		Context->edcProxyProc = NULL;	// no EdcProxy
		break;

	case ProcTagT70Proxy:
		SysLog(LogWarn, "T70Proxy %s, pid=%d terminated", child->name, child->pid);
		ProcDelete(child);
		Context->t70ProxyProc = NULL;	// no t70Proxy
		break;

	default:
		{
			SysLog(LogWarn, "Service %s, pid=%d terminated", child->name, child->pid);

			FecService_t *service = (FecService_t *)child->tag;
			Context->servicePids[service->properties.serviceNumber] = 0;

			if (HashDeleteString(Context->serviceProcList, child->name) != 0)
				SysLog(LogError, "HashDeleteString of %s failed", child->name);
			ProcDelete(child);
			break;
		}

	case ProcTagWorker:
		SysLog(LogWarn, "Worker %s, pid=%d terminated", child->name, child->pid);
		if (HashDeleteString(Context->workerProcList, child->name) != 0)
			SysLog(LogError, "HashDeleteString of %s failed", child->name);
		ProcDelete(child);
		break;

	case ProcTagAuthSim:
		SysLog(LogWarn, "AuthSim %s, pid=%d terminated", child->name, child->pid);
		ProcDelete(child);
		break;

	case ProcTagEdcSim:
		SysLog(LogWarn, "EdcSim %s, pid=%d terminated", child->name, child->pid);
		ProcDelete(child);
		break;

	case ProcTagT70Sim:
		SysLog(LogWarn, "T70Sim %s, pid=%d terminated", child->name, child->pid);
		ProcDelete(child);
		break;

	case ProcTagMax:
		SysLog(LogError, "This should never happen; %s, pid=%d terminated", child->name, child->pid);
		break;
	}

	return 0;
}


static CommandResult_t
ReloadCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	if ( Context->fsm->currentState != SteadyState )
	{
		StringSprintf(response, "Unable to reload config while in state %s; must be in SteadyState", FsmStateToString(Context->fsm, Context->fsm->currentState));
		SysLog(LogWarn, response->str);
		return CommandOk;
	}

	if ( ! AuthProxyIsConnected() )		// not connected to AuthProxy
	{
		AuthProxyDisconnect();
		SetTimer(RecoveryWaitInterval);
		SysLog(LogError, "Not connected to AuthProxy");
		_FsmSetNewState(Context->fsm, ConnectingToAuthState);
		return CommandOk;
	}

	SendConfigurationRequest();
	SetTimer(ReloadWaitInterval);
	_FsmSetNewState(Context->fsm, ConfiguringState);
	StringCpy(response, "Configuration reload has been initiated");
	return CommandOk;
}


static CommandResult_t
CommandHandler(NxClient_t *client, Parser_t *parser, String_t *response)
{
	NxClientVerify(client);

	CommandDef_t cmds[] =
	{
		{"reload", ReloadCmd},
		{NULL, NULL}
	};

	Command_t *cmd = CommandNew(client, cmds);
	CommandResult_t ret = CommandExecute(cmd, parser, "", response);
	CommandDelete(cmd);
	return ret;
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
		EnumToString(ConnectingToAuthState);
		EnumToString(AuthOfflineState);
		EnumToString(ConfiguringState);
		EnumToString(ShutdownState);
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

		EnumToString(SyslogRecycleTimerEvent);
		EnumToString(TimerEvent);
		EnumToString(AuthReadReadyEvent);
		EnumToString(AuthWriteReadyEvent);
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
		SysLog(LogFatal, "%s is not a known event for this state", FsmEventToString(this, evt));

	case SyslogRecycleTimerEvent:
		SysLogRecycle(true);
		SetLogRecycleTimer();
		break;

	case TimerEvent:
		// Setup syslog recycler stuff
		SysLogRegisterWriter(LogRecycle, NULL, NULL);
		SetLogRecycleTimer();
		StartSingletons();

		if (OpenWorkQueue() != 0)
			SysLog(LogFatal, "OpenWorkQueue failed");

		AuthSimStart();
		EdcSimStart();
		T70SimStart();

		if ( Context->authProxyEnabled )
		{
			if ( AuthProxyConnect() < 0)
			{
				SysLog(LogError, "AuthProxyConnect failed");
				SetTimer(RecoveryWaitInterval);
				FsmSetNewState(this, ConnectingToAuthState);
			}

			if ( AuthProxyIsConnected() )
			{
				SetTimer(RecoveryWaitInterval);
				AuditSendEvent(AuditAuthProxyOffline, "connid", NxClientNameToString(Context->authProxySession));
				FsmSetNewState(this, AuthOfflineState);	// connected, wait for an online status
			}

			// wait for connect to complete...
			SetTimer(RecoveryWaitInterval);
			FsmSetNewState(this, ConnectingToAuthState);
		}
		else
		{
			SysLog(LogWarn, "Not starting the AuthProxy");
			FsmSetNewState(this, SteadyState);
		}

		break;
	}
}


static void
FsmConnectingToAuthState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch (evt)
	{
	default:
		SysLog(LogFatal, "%s is not a known event for this state", FsmEventToString(this, evt));
	case ExceptionEvent:
		SysLog(LogError, "Exception. Retrying");
		AuthProxyDisconnect();
		SetTimer(RecoveryWaitInterval);
		FsmSetNewState(this, ConnectingToAuthState);	// Remain in this state
		break;

	case SyslogRecycleTimerEvent:
		SysLogRecycle(true);
		SetLogRecycleTimer();
		break;

	case TimerEvent:
		StartSingletons();

		SysLog(LogWarn, "Timeout waiting for AuthProxy Connect");

		if (AuthProxyDisconnect() != 0)
		{
			SysLog(LogError, "AuthProxyDisconnect failed");
			FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
		}

		if (AuthProxyConnect() < 0)
		{
			SysLog(LogError, "AuthProxyConnect failed");
			FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
		}

		if ( AuthProxyIsConnected() )
		{
			SetTimer(RecoveryWaitInterval);
			AuditSendEvent(AuditAuthProxyOffline, "connid", NxClientNameToString(Context->authProxySession));
			FsmSetNewState(this, AuthOfflineState);
		}

		// wait for connect to complete...
		SetTimer(RecoveryWaitInterval);
		FsmSetNewState(this, ConnectingToAuthState);
		break;

	case AuthWriteReadyEvent:
		if (AuthProxyConnect() < 0)
		{
			SysLog(LogError, "AuthProxyConnect failed");
			FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
		}

		if ( AuthProxyIsConnected() )
		{
			SetTimer(RecoveryWaitInterval);
			AuditSendEvent(AuditAuthProxyOffline, "connid", NxClientNameToString(Context->authProxySession));
			FsmSetNewState(this, AuthOfflineState);
		}

		AuthProxyDisconnect();
		SysLog(LogError, "Connection failed on %s", NxClientNameToString(Context->authProxySession));
		FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
		break;
	}
}


static void
FsmAuthOfflineState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch (evt)
	{
		default:
			SysLog(LogFatal, "%s is not a known event for this state", FsmEventToString(this, evt));
		case ExceptionEvent:
			SysLog(LogError, "Exception. Disconnecting");
			AuthProxyDisconnect();
			SetTimer(RecoveryWaitInterval);
			FsmSetNewState(this, ConnectingToAuthState);
			break;

		case SyslogRecycleTimerEvent:
			SysLogRecycle(true);
			SetLogRecycleTimer();
			break;

		case TimerEvent:
			StartSingletons();
			SysLog(LogWarn, "Timeout waiting for an online AuthProxy");
			SetTimer(RecoveryWaitInterval);	// Keep waiting
			break;

		case AuthReadReadyEvent:
		{
			static ProxyRequest_t *proxyReq = NULL;
			if ( proxyReq == NULL )
				proxyReq = ProxyRequestNew();

			if (AuthProxyRecv(proxyReq) != 0)
			{
				SysLog(LogError, "AuthProxyRecv failed");
				FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception and exit
			}

			switch (proxyReq->reqType)
			{
			default:
				SysLog(LogError, "Received %s request reqType from AuthProxy", ProxyReqTypeToString(proxyReq->reqType));
				SysLog(LogError, "I don't know what to do with this");
				FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception and exit
				break;

			case ProxyReqHostOffline:
				SysLog(LogDebug, "AuthProxy %s reports offline", NxClientNameToString(Context->authProxySession));
				break;

			case ProxyReqHostOnline:
				{
					AuditSendEvent(AuditAuthProxyOnline, "connid", NxClientNameToString(Context->authProxySession));
					SysLog(LogDebug, "AuthProxy %s reports Online", NxClientNameToString(Context->authProxySession));
					int status = SendConfigurationRequest();

					if (status < 0)
					{
						SysLog(LogError, "SendConfigurationRequest failed");
						FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception and exit
					}

					SetTimer(RecoveryWaitInterval);	// Refresh the timer
					FsmSetNewState(this, ConfiguringState); // set configuring state and exit
					break;
				}
			}

			SetTimer(RecoveryWaitInterval);	// Refresh the timer
			AuditSendEvent(AuditAuthProxyOffline, "connid", NxClientNameToString(Context->authProxySession));
			FsmSetNewState(this, AuthOfflineState);
			break;
		}
	}
}


// We have a connection to a AuthProxy
// And we are waiting for the arrival of a configuration
// file (fec.ini).

static void
FsmConfiguringState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch (evt)
	{
		default:
			SysLog(LogFatal, "%s is not a known event for this state", FsmEventToString(this, evt));
		case ExceptionEvent:
			SysLog(LogError, "Exception. Disconnecting");
			AuthProxyDisconnect();
			if (Context->configFile != NULL)
				fclose(Context->configFile);
			Context->configFile = NULL;
			SetTimer(RecoveryWaitInterval);
			FsmSetNewState(this, ConnectingToAuthState);
			break;

		case SyslogRecycleTimerEvent:
			SysLogRecycle(true);
			SetLogRecycleTimer();
			break;

		case TimerEvent:
			StartSingletons();
			SysLog(LogError, "Timeout waiting for configuration from AuthProxy");
			FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
			break;

		case AuthReadReadyEvent:
		{
			static ProxyRequest_t *proxyReq = NULL;
			if ( proxyReq == NULL )
				proxyReq = ProxyRequestNew();

			if (AuthProxyRecv(proxyReq) != 0)
			{
				SysLog(LogError, "AuthProxyRecv failed");
				FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception and exit
			}

			switch (proxyReq->reqType)
			{
				default:
					SysLog(LogError, "Received %s reqType from AuthProxy", ProxyReqTypeToString(proxyReq->reqType));
					SysLog(LogError, "I don't know what to do with this: %s", ProxyRequestToString(proxyReq, DumpOutput));
					FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception and exit
					break;

				case ProxyReqHostMsg:
					if (proxyReq->hostReq.len > 0)
					{
						char text[MAX_HOST_PAYLOAD + 10];

						memcpy(text, proxyReq->hostReq.data, proxyReq->hostReq.len);
						text[proxyReq->hostReq.len] = '\0';

						int fin = ProcessConfigLine(text);

						if (fin < 0)
						{
							SysLog(LogError, "ProcessConfigLine failed");
							FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception and exit
						}
						else if (fin > 0)	// this means we're finished loading the config
						{
							AuditSendEvent(AuditHostConfigComplete, "connid", NxClientNameToString(Context->authProxySession));
							SetTimer(10);
							FsmSetNewState(this, SteadyState);	// we're done... exits
						}
					}
					break;

				case ProxyReqHostOffline:
					SysLog(LogDebug, "AuthProxy %s reports offline", NxClientNameToString(Context->authProxySession));
					AuditSendEvent(AuditAuthProxyOffline, "connid", NxClientNameToString(Context->authProxySession));
					FsmSetNewState(this, AuthOfflineState);
					break;

				case ProxyReqHostOnline:
					SysLog(LogError, "AuthProxy %s reports online", NxClientNameToString(Context->authProxySession));
					break;
			}
			SetTimer(RecoveryWaitInterval);	// Refresh the timer
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
		SysLog(LogFatal, "%s is not a known event for this state", FsmEventToString(this, evt));

	case SyslogRecycleTimerEvent:
		SysLogRecycle(true);
		SetLogRecycleTimer();
		break;

	case TimerEvent:
		CheckChildTerminated();
		if ( HashMapLength(Context->workerProcList) <= 0 ||	// no workers left
			Context->idleRetries > Context->maxIdleRetries )	// or exhausted retries
		{
			FinishShutdown();
		}
		else
		{
			SysLog(LogWarn, "%d workers still active; waiting %d...", HashMapLength(Context->workerProcList), (Context->maxIdleRetries-Context->idleRetries));
			++Context->idleRetries;		// count the retry
			SetTimer(ShutdownWaitInterval);			// workers are still working; wait again...
		}
		break;

	case AuthReadReadyEvent:
		AuthProxyDisconnect();		// disconnect; we're shutting down...
		break;
	}
}


static void
FsmSteadyState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch (evt)
	{
	default:
		SysLog(LogFatal, "%s is not a known event for this state", FsmEventToString(this, evt));
	case ExceptionEvent:
		SysLog(LogError, "Exception. Disconnecting");
		AuthProxyDisconnect();
		SetTimer(RecoveryWaitInterval);
		FsmSetNewState(this, ConnectingToAuthState);
		break;

	case SyslogRecycleTimerEvent:
		SysLogRecycle(true);
		SetLogRecycleTimer();
		break;

	case TimerEvent:
		CheckChildTerminated();
		StartAllChildren();
		SetTimer(RecoveryWaitInterval);
		break;

	case AuthReadReadyEvent:
		{
			static ProxyRequest_t *proxyReq = NULL;
			if ( proxyReq == NULL )
				proxyReq = ProxyRequestNew();

			if (AuthProxyRecv(proxyReq) != 0)
			{
				SysLog(LogError, "AuthProxyRecv failed");
				FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
			}

			switch (proxyReq->reqType)
			{
			default:
				SysLog(LogError, "Received %s request reqType from AuthProxy", ProxyReqTypeToString(proxyReq->reqType));
				SysLog(LogError, "I don't know what to do with this");
				FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception; and exits
				break;

			case ProxyReqHostOffline:
				SysLog(LogError, "AuthProxy %s reports offline", NxClientNameToString(Context->authProxySession));
				AuditSendEvent(AuditAuthProxyOffline, "connid", NxClientNameToString(Context->authProxySession));
				FsmSetNewState(this, AuthOfflineState);
				break;

			case ProxyReqHostOnline:
				break;
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
		SysLog(LogFatal, "Invalid state %s", FsmStateToString(this, this->currentState));
		break;

	case NullState:
		FsmNullState(this, evt, efarg);
		break;

	case ConnectingToAuthState:
		FsmConnectingToAuthState(this, evt, efarg);
		break;

	case AuthOfflineState:
		FsmAuthOfflineState(this, evt, efarg);
		break;

	case ConfiguringState:
		FsmConfiguringState(this, evt, efarg);
		break;

	case ShutdownState:
		FsmShutdownState(this, evt, efarg);
		break;

	case SteadyState:
		FsmSteadyState(this, evt, efarg);
		break;
	}
}


static ProcContext_t*
ProcContextConstructor(ProcContext_t *this, char *file, int lno)
{
	this->authProxySession = NxClientNew();

	this->currentTimer = TimerNew("%sTimer", NxCurrentProc->name);
	this->syslogRecycleTimer = TimerNew("%sSyslogRecycleTimer", NxCurrentProc->name);

	this->minIdleRetries = MinShutdownWaitTime / ShutdownWaitInterval;
	this->maxIdleRetries = ProcGetPropertyIntValue(NxCurrentProc, "ShutdownWaitTime") / (ShutdownWaitInterval/1000);
	this->maxIdleRetries = min(this->minIdleRetries, this->maxIdleRetries);

// Socket pairs for the worker queue
	this->txWorkQueue = NxClientNew();
	this->rxWorkQueue = NxClientNew();

// HashLists to hold server (service/worker) handles
	this->serviceProcList = HashMapNew(MAXSERVICES, "ServiceProcList");
	this->workerProcList = HashMapNew(MAXWORKERS, "WorkerProcList");
	this->nextWorkerNbr = 0;

// The trial configuration structure
	if ( (this->trialFecConfig = FecConfigNew()) == NULL )
		SysLogFull(LogFatal, file, lno, __FUNC__, "Unable to allocate a Trial Config");

	this->webEnabled = NxGetPropertyBooleanValue("Web.Enabled");
	this->authProxyEnabled = NxGetPropertyBooleanValue("AuthProxy.Enabled");
	this->edcProxyEnabled = NxGetPropertyBooleanValue("EdcProxy.Enabled");
	this->t70ProxyEnabled = NxGetPropertyBooleanValue("T70Proxy.Enabled");
	this->authSimEnabled = NxGetPropertyBooleanValue("AuthSim.Enabled");
	this->edcSimEnabled = NxGetPropertyBooleanValue("EdcSim.Enabled");
	this->t70SimEnabled = NxGetPropertyBooleanValue("T70Sim.Enabled");

	this->fsm = FsmNew(FsmEventHandler, FsmStateToString, FsmEventToString, NullState, "%sFsm", NxCurrentProc->name);
	return this;
}


static void
ProcContextDestructor(ProcContext_t *this, char *file, int lno)
{
	// only delete children when we're the parent...
	if ( this->authProxyProc != NULL && NxCurrentProc != this->authProxyProc )
		ProcDelete(this->authProxyProc);

	if ( this->edcProxyProc != NULL && NxCurrentProc != this->edcProxyProc )
		ProcDelete(this->edcProxyProc);

	if ( this->t70ProxyProc != NULL && NxCurrentProc != this->t70ProxyProc )
		ProcDelete(this->t70ProxyProc);

	if ( this->authSimProc != NULL && NxCurrentProc != this->authSimProc )
		ProcDelete(this->authSimProc);

	if ( this->edcSimProc != NULL && NxCurrentProc != this->edcSimProc )
		ProcDelete(this->edcSimProc);

	if ( this->t70SimProc != NULL && NxCurrentProc != this->t70SimProc )
		ProcDelete(this->t70SimProc);

	if ( this->webProc != NULL && NxCurrentProc != this->webProc )
		ProcDelete(this->webProc);

	if ( this->fsm != NULL )
		FsmDelete(this->fsm);

	if ( this->authProxySession != NULL )
		NxClientDelete(this->authProxySession);

	TimerDelete(this->currentTimer);
	TimerDelete(this->syslogRecycleTimer);

	if(this->txWorkQueue != NULL)
		NxClientDelete(this->txWorkQueue);
	if(this->rxWorkQueue != NULL )
		NxClientDelete(this->rxWorkQueue);

	HashClear(this->serviceProcList, false);
	HashMapDelete(this->serviceProcList);
	HashClear(this->workerProcList, false);
	HashMapDelete(this->workerProcList);

	FecConfigDelete(this->trialFecConfig);
}


static Json_t*
ProcContextSerialize(ProcContext_t *this)
{
	ContextVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	JsonAddItem(root, "Platform", NxSerialize(NxGlobal));
	JsonAddItem(root, "Fsm", FsmSerialize(this->fsm));

	if ( this->shutdownStartTime != 0 )
		JsonAddString(root, "ShutdownStartTime", MsTimeToStringShort(this->shutdownStartTime, NULL));

	JsonAddNumber(root, "IdleRetries", this->idleRetries);

	this->maxIdleRetries = ProcGetPropertyIntValue(NxCurrentProc, "ShutdownWaitTime") / (ShutdownWaitInterval/1000);
	this->maxIdleRetries = min(this->minIdleRetries, this->maxIdleRetries);
	JsonAddNumber(root, "MinIdleRetries", this->minIdleRetries);
	JsonAddNumber(root, "MaxIdleRetries", this->maxIdleRetries);

	JsonAddItem(root, "AuthProxyConnection", NxClientSerialize(this->authProxySession));
	JsonAddItem(root, "TxWorkQueue", NxClientSerialize(this->txWorkQueue));
	JsonAddItem(root, "RxWorkQueue", NxClientSerialize(this->rxWorkQueue));

	JsonAddString(root, "WebEnabled", BooleanToString(this->webEnabled));
	JsonAddString(root, "AuthProxyEnabled", BooleanToString(this->authProxyEnabled));
	JsonAddString(root, "EdcProxyEnabled", BooleanToString(this->edcProxyEnabled));
	JsonAddString(root, "T70ProxyEnabled", BooleanToString(this->t70ProxyEnabled));
	JsonAddString(root, "AuthSimEnabled", BooleanToString(this->authSimEnabled));
	JsonAddString(root, "EdcSimEnabled", BooleanToString(this->edcSimEnabled));
	JsonAddString(root, "T70SimEnabled", BooleanToString(this->t70SimEnabled));

	if (this->currentTimer != NULL && this->currentTimer->active)
		JsonAddItem(root, "Timer", TimerSerialize(this->currentTimer));
	if (this->syslogRecycleTimer != NULL && this->syslogRecycleTimer->active)
		JsonAddItem(root, "Timer", TimerSerialize(this->syslogRecycleTimer));

// before building child lists, check for terminations
	CheckChildTerminated();

// The singletons
	{
		Json_t *sub = JsonPushObject(root, "Singletons");
		if ( this->webProc != NULL)
			JsonAddItem(sub, "Proc", ProcSerialize(this->webProc));
		if ( this->authProxyProc != NULL)
			JsonAddItem(sub, "Proc", ProcSerialize(this->authProxyProc));
		if ( this->edcProxyProc != NULL)
			JsonAddItem(sub, "Proc", ProcSerialize(this->edcProxyProc));
		if ( this->t70ProxyProc != NULL)
			JsonAddItem(sub, "Proc", ProcSerialize(this->t70ProxyProc));
		if ( this->authSimProc != NULL)
			JsonAddItem(sub, "Proc", ProcSerialize(this->authSimProc));
		if ( this->edcSimProc != NULL)
			JsonAddItem(sub, "Proc", ProcSerialize(this->edcSimProc));
		if ( this->t70SimProc != NULL)
			JsonAddItem(sub, "Proc", ProcSerialize(this->t70SimProc));
	}

// The workers
	{
		if ( HashMapLength(this->workerProcList) > 0 )
		{
			Json_t *sub = JsonPushObject(root, "Workers");
			ObjectList_t* list = HashGetOrderedList(this->workerProcList, ObjectListStringType);
			for ( HashEntry_t *entry = NULL; (entry = ObjectListRemove(list, ObjectListFirstPosition)); )
			{
				Proc_t *child = (Proc_t *)entry->var;
				JsonAddItem(sub, "Proc", ProcSerialize(child));
				HashEntryDelete(entry);
			}
			ObjectListDelete(list);
		}
	}

// The services
	{
		if ( HashMapLength(this->serviceProcList) > 0 )
		{
			Json_t *sub = JsonPushObject(root, "Services");
			ObjectList_t* list = HashGetOrderedList(this->serviceProcList, ObjectListStringType);
			for ( HashEntry_t *entry = NULL; (entry = ObjectListRemove(list, ObjectListFirstPosition)); )
			{
				Proc_t *child = (Proc_t *)entry->var;
				JsonAddItem(sub, "Proc", ProcSerialize(child));
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


static int
ChildPrelude(Proc_t *rootProc)
{
	SysLogUnRegisterWriter(LogRecycle, NULL, NULL);
	ContextDelete(rootProc->context);
	return 0;
}


static void
LogRecycle(SysLog_t *this, void **contextp, boolean force)
{

	off_t o;
	if ( force || (o = lseek(1, 0, SEEK_CUR)) > NxGlobal->logMaxFileSize ) // size
	{
		if ( NxLogFileBackup(NxGlobal, NxGlobal->name) != 0 )
			SysLog(LogError, "NxLogFileBackup failed");

		ConsoleRecycle(this, contextp, true);

		SysLog(LogAny, "New log file");
	}
}


int
ProcRootStart(Proc_t *this, va_list ap)
{

	NxGlobal->steadyState = true;

	{	// map the shared configuration area
		char tmp[1024];
		sprintf(tmp, "%s/%s", NxGlobal->nxDir, ProcGetPropertyValue(this, "ConfigFileName"));
		if ( (NxGlobal->context = FecConfigNewShared(tmp)) == NULL )
			NxCrash("Unable to map config memory %s", tmp);
	}

	this->context = ContextNew();
	this->commandHandler = CommandHandler;
	this->contextSerialize = ProcContextSerialize;
	this->contextToString = ProcContextToString;

	ProcSetSignalHandler(this, SIGTERM, SigTermHandler);	// Termination signal

	StartSingletons(); // Start these now to allow time to become steady state

	// Start the background clock
	//
	SetTimer(InitialWaitInterval);

	return 0;
}
