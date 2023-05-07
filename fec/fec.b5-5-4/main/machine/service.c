/*****************************************************************************

Filename:   main/machine/service.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:34:01 $
 * $Header: /home/hbray/cvsroot/fec/main/machine/service.c,v 1.3.4.8 2011/10/27 18:34:01 hbray Exp $
 *
 $Log: service.c,v $
 Revision 1.3.4.8  2011/10/27 18:34:01  hbray
 Revision 5.5

 Revision 1.3.4.7  2011/09/26 15:52:33  hbray
 Revision 5.5

 Revision 1.3.4.6  2011/09/24 17:49:53  hbray
 Revision 5.5

 Revision 1.3.4.4  2011/08/23 19:54:01  hbray
 eliminate fecplugin.h

 Revision 1.3.4.3  2011/08/23 12:03:15  hbray
 revision 5.5

 Revision 1.3.4.2  2011/08/15 19:12:32  hbray
 5.5 revisions

 Revision 1.3.4.1  2011/08/11 19:47:36  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:24  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:58  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: service.c,v 1.3.4.8 2011/10/27 18:34:01 hbray Exp $ "


#include <sys/wait.h>

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"

#include "machine/include/service.h"
#include "machine/include/worker.h"
#include "machine/include/possim.h"



// Local Data Types

#define Context ((NxCurrentProc)->context)


typedef enum
{
	NullState, WaitingForListenState, SteadyState
} FsmState;

typedef enum
{
	TimerEvent, ListenReadyEvent, ExceptionEvent
} FsmEvent;

typedef struct ProcContext_t
{
	Fsm_t				*fsm;

	FecService_t		service;

	NxServer_t			*serverListen;

	NxClient_t			*workQueue;

	Timer_t				*currentTimer;

	Proc_t				*simProc;
} ProcContext_t;


#define ContextNew(sn, workQueue) ObjectNew(ProcContext, sn, workQueue)
#define ContextVerify(var) ObjectVerify(ProcContext, var)
#define ContextDelete(var) ObjectDelete(ProcContext, var)

static ProcContext_t* ProcContextConstructor(ProcContext_t *this, char *file, int lno, int sn, NxClient_t *workQueue);
static void ProcContextDestructor(ProcContext_t *this, char *file, int lno);
static BtNode_t* ProcContextNodeList;
static Json_t* ProcContextSerialize(ProcContext_t *this);
static char* ProcContextToString(ProcContext_t *this);


// Static Functions
//
// Event Handlers
//
static void HandleClientListenEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandleTimerEvent(Timer_t *tid);
static void SigTermHandler(NxSignal_t*);

// Fsm Functions
//
static void FsmEventHandler(Fsm_t *this, int evt, void * efarg);
static char *FsmEventToString(Fsm_t *this, int evt);
static void FsmNullState(Fsm_t *this, FsmEvent evt, void * efarg);
static char *FsmStateToString(Fsm_t *this, int state);
static void FsmSteadyState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmWaitingForListenState(Fsm_t *this, FsmEvent evt, void * efarg);

// Helper Functions
//
static int ClientListen();
static NxClient_t* ClientAccept();
static void ClientDisconnect(NxClient_t *client);
static int ClientHandoff(NxClient_t *client);
static void TimerSet(int ms);
static int CommandHandler(NxClient_t *client, Parser_t *parser, String_t *response);
static void Shutdown();
static int ChildPrelude(Proc_t *serviceProc);
static int CheckChildTerminated();
static void StartSimulator();



// Static Global Vars
//
static const int InitialWaitInterval = (10 * 1000);
static const int RecoveryWaitInterval = (60 * 1000);

// Functions Start Here

// Event Handlers
//


static void
SigTermHandler(NxSignal_t *sig)
{
	Shutdown();
}


static void
HandleTimerEvent(Timer_t *tid)
{
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	_FsmDeclareEvent(fsm, TimerEvent, tid);
}


static void
HandleClientListenEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg)
{
	NxServer_t *listen = (NxServer_t*)farg;
	NxServerVerify(listen);
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, ListenReadyEvent, evf);
}


// Helper Functions
//


static int
ClientListen()
{

	SysLog(LogDebug, "Listen for service: {%s}", FecConfigServiceToString(&Context->service));

	// Open my service port

	{
		char addr[1024];
		sprintf(addr, "%s/%s", NxGlobal->nxDir, ProcGetPropertyValue(NxCurrentProc, "HostAddr"));
		int port = Context->service.properties.port;
		if ( NxServerListen(Context->serverListen, AF_INET, SOCK_STREAM, addr, port, EventFileLowPri, EventReadMask, HandleClientListenEvent) != 0 )
		{
			SysLog(LogError, "SockListen failed: %s.%d", addr, port);
			return -1;
		}
	}

	return 0;
}


static NxClient_t*
ClientAccept()
{
	NxClient_t *clientConnection = NxServerAccept(Context->serverListen, EventReadMask, EventFileLowPri, NULL);
	if ( clientConnection == NULL )
	{
		SysLog(LogError, "NxServerAccept failed");
		return NULL;
	}
	return clientConnection;
}


static void
ClientDisconnect(NxClient_t *client)
{
	NxClientDelete(client);
}


static int
ClientHandoff(NxClient_t *client)
{
	NxClientVerify(client);

// assign new name
	char name[1024];
	sprintf(name, "%s_%d.%s", Context->service.properties.protocol, client->evf->servicePort, client->evf->uidString);
	NxClientSetName(client, name);

	SysLog(LogDebug, "Connected to client %s", NxClientToString(client));

	SysLog(LogDebug, "Passing connection %s to %s", name, NxClientNameToString(Context->workQueue));

	if (NxClientSendConnection(client, Context->workQueue) != 0)
	{
		SysLog(LogError, "NxClientSendConnection failed: %s", NxClientToString(client));
		ClientDisconnect(client);
		return -1;
	}

	AuditSendEvent(AuditPosForwardSend, "connid", name);
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


static int
ChildPrelude(Proc_t *serviceProc)
{
	NxCurrentProc = serviceProc;	// set proc context as one being started
	TimerCancelAll();
	ContextDelete(serviceProc->context);
	return 0;
}


static int
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
				if (child != Context->simProc)
				{
					if ( child != NULL )
						ProcDelete(child);
					SysLog(LogError, "ProcFindChildByPid of %d failed", child->pid);
					return -1;
				}
				ProcDelete(child);
				Context->simProc = NULL;		// simulator is done...
				fini = false;
				break;		// list is modified, break out of inner for...
			}
		}
	}

	return 0;
}


static void
StartSimulator()
{
#if LATER
	if ( Context->simProc == NULL &&
		ProcGetPropertyBooleanValue(this, "PosSim") )
	{
		char name[64];
		sprintf(name, "PosSim_%s.%d", Context->service.properties.protocol, Context->service.properties.port);
		Proc_t *proc = ProcNew(this, name, ProcTagPosSim);
		if ( proc == NULL )
			SysLog(LogFatal, "Creation of %s failed", name);
		ProcStart(proc, ProcChildPrelude, ProcPosSimStart, Context->service.properties.serviceNumber);
		Context->simProc = proc;
	}
#endif
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
		EnumToString(WaitingForListenState);
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
		EnumToString(ListenReadyEvent);
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
		TimerSet(1);
		FsmSetNewState(this, WaitingForListenState);
		break;
	}
}


static void
FsmWaitingForListenState(Fsm_t *this, FsmEvent evt, void * efarg)
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
			if (ClientListen() != 0)
			{
				SysLog(LogError, "ClientListen failed");
				TimerSet(RecoveryWaitInterval);
				FsmSetNewState(this, WaitingForListenState);
			}
			else		// we're connected; let 'er fly
			{
				AuditSendEvent(AuditStartService, "connid", NxServerNameToString(Context->serverListen), "service", FecConfigServiceToString(&Context->service));
				TimerSet(InitialWaitInterval);
				FsmSetNewState(this, SteadyState);
			}
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
			SysLog(LogFatal, "Exception. All is lost...");
			break;

		case TimerEvent:
			CheckChildTerminated();
			StartSimulator();
			TimerSet(RecoveryWaitInterval);	// set the timer
			break;

		case ListenReadyEvent:
		{
			NxClient_t *client;

			if ((client = ClientAccept()) == NULL)
			{
				SysLog(LogError, "ClientAccept failed");
				break;
			}

			if ( NxIgnoreThisIp((unsigned char *)client->evf->peerIpAddr))
			{
				ClientDisconnect(client);	// ignoring, no longer need this...
			}
			else
			{
				if (ClientHandoff(client) < 0)
					SysLog(LogError, "ClientHandoff failed");
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

	case WaitingForListenState:
		FsmWaitingForListenState(this, evt, efarg);
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


static CommandResult_t
CommandHandler(NxClient_t *client, Parser_t *parser, String_t *response)
{
	CommandDef_t cmds[] = {
		{NULL, NULL}
	};

	Command_t *cmd = CommandNew(client, cmds);

	CommandResult_t ret = CommandExecute(cmd, parser, "", response);

	CommandDelete(cmd);

	return ret;
}


static ProcContext_t*
ProcContextConstructor(ProcContext_t *this, char *file, int lno, int port, NxClient_t *workQueue)
{

	this->serverListen = NxServerNew();

	// The worker queue port
	this->workQueue = workQueue;

	this->currentTimer = TimerNew("%sTimer", NxCurrentProc->name);

// copy the service details from shared memory (allows the Root process to change it without killing this process)

	FecService_t *service = FecConfigGetServiceContext(FecConfigGlobal, port);
	if ( service == NULL )
		SysLog(LogFatal, "No service for port %d", port);
	memcpy(&this->service, service, sizeof(this->service));	// local config

	return this;
}


static void
ProcContextDestructor(ProcContext_t *this, char *file, int lno)
{
	NxServerDelete(this->serverListen);

	if(this->currentTimer != NULL )
		TimerDelete(this->currentTimer);
}


static Json_t*
ProcContextSerialize(ProcContext_t *this)
{
	ContextVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	JsonAddItem(root, "Fsm", FsmSerialize(this->fsm));

	JsonAddItem(root, "ServiceListen", NxServerSerialize(this->serverListen));
	JsonAddItem(root, "WorkQueue", NxClientSerialize(this->workQueue));
	JsonAddItem(root, "Service", FecConfigServiceSerialize(&this->service));

	if (this->currentTimer != NULL && this->currentTimer->active)
		JsonAddItem(root, "Timer", TimerSerialize(this->currentTimer));

	JsonAddString(root, "CreatePosSim", BooleanToString(ProcGetPropertyBooleanValue(NxCurrentProc, "PosSim")));

// Any children
	{
		if ( HashMapLength(NxCurrentProc->childMap) > 0 )
		{
			Json_t *sub = JsonPushObject(root, "Sims");
			ObjectList_t* list = HashGetOrderedList(NxCurrentProc->childMap, ObjectListStringType);
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


int
ProcServiceStart(Proc_t *this, va_list ap)
{

// arg1 is my service number
// arg2 is a pointer to my workqueue

	int port = va_arg(ap, int);	// service port

	NxClient_t *workQueue = va_arg(ap, NxClient_t*);
	NxClientVerify(workQueue);

	this->context = ContextNew(port, workQueue);
	this->commandHandler = CommandHandler;
	this->contextSerialize = ProcContextSerialize;
	this->contextToString = ProcContextToString;

	Context->fsm = FsmNew(FsmEventHandler, FsmStateToString, FsmEventToString, NullState, "%sFsm", this->name);

	ProcSetSignalHandler(this, SIGTERM, SigTermHandler);	// Termination signal

	// Start the background clock
	TimerSet(InitialWaitInterval);

	return 0;
}
