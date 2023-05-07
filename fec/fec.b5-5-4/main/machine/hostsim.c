/*****************************************************************************

Filename:   main/machine/hostsim.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:34:01 $
 * $Header: /home/hbray/cvsroot/fec/main/machine/hostsim.c,v 1.3.4.10 2011/10/27 18:34:01 hbray Exp $
 *
 $Log: hostsim.c,v $
 Revision 1.3.4.10  2011/10/27 18:34:01  hbray
 Revision 5.5

 Revision 1.3.4.9  2011/09/26 15:52:33  hbray
 Revision 5.5

 Revision 1.3.4.8  2011/09/24 17:49:53  hbray
 Revision 5.5

 Revision 1.3.4.6  2011/09/01 14:49:48  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/08/25 18:19:45  hbray
 *** empty log message ***

 Revision 1.3.4.4  2011/08/24 13:22:29  hbray
 Renamed

 Revision 1.3.4.3  2011/08/23 12:03:15  hbray
 revision 5.5

 Revision 1.3.4.2  2011/08/17 17:59:05  hbray
 *** empty log message ***

 Revision 1.3.4.1  2011/08/11 19:47:35  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:24  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:57  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: hostsim.c,v 1.3.4.10 2011/10/27 18:34:01 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/random.h"
#include "include/hostrequest.h"
#include "include/hostio.h"

#include "machine/include/hostsim.h"


#define Context ((NxCurrentProc)->context)


// Local Data Types

typedef enum
{
	NullState, ConnectingState, ConfiguringState, SteadyState
} FsmState;

typedef enum
{
	TimerEvent, ReadReadyEvent, WriteReadyEvent, ExceptionEvent
} FsmEvent;

typedef struct ProcContext_t
{
	Fsm_t			*fsm;

	NxClient_t		*fecConnection;
	Timer_t			*currentTimer;

	HostFrameHeader_t	hostHeader;

	FILE			*configFile;
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
static void SigTermHandler(NxSignal_t *sig);
static void HandleFecConnectionEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void HandleTimerEvent(Timer_t *tid);

// Fsm Functions
//
static void FsmConfiguringState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmConnectingState(Fsm_t *this, FsmEvent evt, void * efarg);
static void FsmEventHandler(Fsm_t *this, int evt, void * efarg);
static char *FsmEventToString(Fsm_t *this, int evt);
static void FsmNullState(Fsm_t *this, FsmEvent evt, void * efarg);
static char *FsmStateToString(Fsm_t *this, int state);
static void FsmSteadyState(Fsm_t *this, FsmEvent evt, void * efarg);

// Helper Functions
//
static void TimerSet(int ms);
static int FecConnect();
static int FecDisconnect();
static int ProcessHostMsg(HostFrame_t *frame);
static int OpenConfigFile(HostFrame_t *frame);
static int SendConfigLine(char *line);
static int CommandHandler(NxClient_t *client, Parser_t *parser, String_t *response);
static void Shutdown();


// Static Global Vars
//
static const int InitialWaitInterval = (10 * 1000);
static const int RecoveryWaitInterval = (60 * 1000);

static const char *HostIniFile = "etc/fec.ini";




// Functions Start Here


static void
SigTermHandler(NxSignal_t *sig)
{
	Shutdown();
}


static void
HandleFecConnectionEvent(EventFile_t *evf, EventPollMask_t pollMask, void * farg)
{
	ProcVerify(NxCurrentProc);
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);
	NxClient_t *client = (NxClient_t*)farg;
	NxClientVerify(client);

	if (pollMask & EventReadMask)
		_FsmDeclareEvent(fsm, ReadReadyEvent, client);

	if (pollMask & EventWriteMask)
		_FsmDeclareEvent(fsm, WriteReadyEvent, client);
}


static void
HandleTimerEvent(Timer_t *tid)
{
	ProcVerify(NxCurrentProc);
	Fsm_t *fsm = Context->fsm;
    FsmVerify(fsm);

	_FsmDeclareEvent(fsm, TimerEvent, tid);
}


// Helper functions
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
FecConnect()
{

	// Open FEC

	SysLog(LogDebug, "Connecting to FEC %s:%d", ProcGetPropertyValue(NxCurrentProc, "HostAddr"), ProcGetPropertyIntValue(NxCurrentProc, "HostPort"));

	if ( NxClientConnect(Context->fecConnection, AF_INET, SOCK_STREAM, ProcGetPropertyValue(NxCurrentProc, "HostAddr"), ProcGetPropertyIntValue(NxCurrentProc, "HostPort"), EventFileHighPri, EventReadMask | EventWriteMask, HandleFecConnectionEvent) < 0 )
	{
		SysLog(LogError, "NxClientConnect failed: %s.%d", ProcGetPropertyValue(NxCurrentProc, "HostAddr"), ProcGetPropertyIntValue(NxCurrentProc, "HostPort"));
		FecDisconnect();
		return -1;
	}

	return 0;
}


static int
FecDisconnect()
{
	SysLog(LogDebug, "Disconnecting StratusProxy %s", NxClientNameToString(Context->fecConnection));
	NxClientDisconnect(Context->fecConnection);
	return 0;
}


static int
SendConfigLine(char *line)
{
	HostFrame_t frame;

	HostFrameBuildHeader(&frame.hdr, HostResponseMsgType, Context->hostHeader.servicePort, eSvcConfig,
						NxGlobal->sysid,
						(char *)Context->hostHeader.peerIpAddr,
						Context->hostHeader.peerIpType[0],
						Context->hostHeader.peerIpPort,
						Context->hostHeader.echo);

	int slen = strlen(line);

	if (slen > sizeof(frame.payload))
	{
		SysLog(LogError, "Config line '%s' is too long; discarding", slen);
		return -1;
	}

	HostFrameSetPayload(&frame, (unsigned char *)line, slen);
	HostFrameSetLen(&frame, slen);

	if (HostFrameSend(Context->fecConnection, &frame) <= 0)
	{
		SysLog(LogError, "HostFrameSend failed: %s", NxClientToString(Context->fecConnection));
		return -1;
	}

	return 0;
}


static int
OpenConfigFile(HostFrame_t *frame)
{

	memcpy(&Context->hostHeader, &frame->hdr, sizeof(Context->hostHeader));

	SysLog(LogDebug, "Sending %s/%s", getcwd(NULL, 0), HostIniFile);
	
	if ( Context->configFile != NULL )	// file is open
		fclose(Context->configFile);	// close it

	Context->configFile = fopen(HostIniFile, "r");
	if (Context->configFile == NULL)
	{
		SysLog(LogError, "Unable to open %s; errno=%s", HostIniFile, ErrnoToString(errno));
		SysLog(LogError, "I looked in %s", getcwd(NULL, 0));
		return -1;
	}

	return 0;
}


// Returns -1 on error; +1 if entering configuration mode; otherwise, returns 0
static int
ProcessHostMsg(HostFrame_t *frame)
{

// if a config request
	if (strncmp((char *)frame->hdr.msgType, HostConfigReqestMsgType, 2) == 0)
	{
		if (OpenConfigFile(frame) < 0)
			return -1;

		char *option = ProcGetPropertyValue(NxCurrentProc, "Deadman");
		if ( option != NULL && (isdigit(*option) && atoi(option) > 0) )
		{
			return 1;
		}
		else
		{
			char line[MaxSockPacketLen];

			while (fgets(line, sizeof(line), Context->configFile) != NULL)
			{
				if (SendConfigLine(line) < 0)
				{
					SysLog(LogError, "SendConfigLine failed");
					return -1;
				}
			}

			fclose(Context->configFile);
			Context->configFile = NULL;
		}
	}
	else		// not a config request; echo content
	{
		unsigned char line[MAX_HOST_PAYLOAD + 10];
		int plen = HostFramePayloadLen(*frame);

		memcpy(line, frame->payload, plen);
		line[plen] = '\0';

		HostFrameSetPayload(frame, (unsigned char *)line, plen);
		HostFrameSetLen(frame, plen);

		if (HostFrameSend(Context->fecConnection, frame) <= 0)
		{
			SysLog(LogError, "HostFrameSend failed: %s", NxClientToString(Context->fecConnection));
			return -1;
		}
	}

	return 0;
}


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
		EnumToString(ConnectingState);
		EnumToString(ConfiguringState);
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
		EnumToString(ReadReadyEvent);
		EnumToString(WriteReadyEvent);
		EnumToString(ExceptionEvent);
		EnumToString(FsmDeadmanEvent);
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
		if (FecConnect() < 0)
		{
			SysLog(LogError, "FecConnect failed");
		}
		else if (NxClientIsConnected(Context->fecConnection))
		{
			TimerSet(RecoveryWaitInterval);
			FsmSetNewState(this, ConfiguringState);
		}

		TimerSet(RecoveryWaitInterval);
		FsmSetNewState(this, ConnectingState);
		break;
	}
}


static void
FsmConnectingState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch (evt)
	{
	default:
		SysLog(LogError, "%s is not a known event for this state", FsmEventToString(this, evt));
	case ExceptionEvent:
		SysLog(LogError, "Exception. Disconnecting");
		FecDisconnect();
		TimerSet(RecoveryWaitInterval);
		break;

	case TimerEvent:
		SysLog(LogError, "Timeout waiting for FEC Connect");

		FecDisconnect();

		if (FecConnect() < 0)
		{
			SysLog(LogError, "FecConnect failed");
		}
		else if ( NxClientIsConnected(Context->fecConnection) )
		{
			TimerSet(RecoveryWaitInterval);
			FsmSetNewState(this, ConfiguringState);
		}

		TimerSet(RecoveryWaitInterval);
		FsmSetNewState(this, ConnectingState);
		break;

	case WriteReadyEvent:
		if (FecConnect() < 0)
		{
			SysLog(LogError, "FecConnect failed");
			FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
		}

		if (!NxClientIsConnected(Context->fecConnection) )
		{
			SysLog(LogError, "Connection pending");
			FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
		}

		SysLog(LogDebug, "Connected to a FEC: %s", NxClientToString(Context->fecConnection));

		TimerSet(RecoveryWaitInterval);
		FsmSetNewState(this, ConfiguringState);
		break;
	}
}


// We have a connection to an FEC
// And we are waiting for the arrival of a configuration
// file (fec.ini).


static void
FsmConfiguringState(Fsm_t *this, FsmEvent evt, void * efarg)
{

	switch ((int)evt)
	{
	default:
		SysLog(LogError, "%s is not a known event for this state", FsmEventToString(this, evt));
	case ExceptionEvent:
		SysLog(LogError, "Exception. Disconnecting");
		FecDisconnect();

		TimerSet(RecoveryWaitInterval);
		FsmSetNewState(this, ConnectingState);
		break;

	case FsmDeadmanEvent:
		SysLog(LogError, "Deadman Timer Elapsed; Disconnecting");
		FsmDeclareEvent(this, ExceptionEvent, efarg);
		break;

	case TimerEvent:
		if (Context->configFile != NULL)
		{
			char line[MaxSockPacketLen];

			if (fgets(line, sizeof(line), Context->configFile) != NULL)
			{
				if (SendConfigLine(line) < 0)
				{
					SysLog(LogError, "SendConfigLine failed");
					FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
				}

				TimerSet(50);
			}
			else
			{
				fclose(Context->configFile);
				Context->configFile = NULL;
				TimerSet(50);
				FsmSetNewState(this, SteadyState);
			}
		}
		TimerSet(RecoveryWaitInterval);
		break;

	case ReadReadyEvent:
	{
		NxClient_t *client = (NxClient_t*)efarg;
		NxClientVerify(client);

		HostFrame_t frame;
		int rlen = HostFrameRecv(client, &frame);

		if (rlen < (int)sizeof(frame.hdr))
		{
			SysLog(LogError, "HostFrameRecv failed: Received %d bytes", rlen);
			FsmDeclareEvent(this, ExceptionEvent, client);	// Declare an exception
		}
		else
		{
			switch(ProcessHostMsg(&frame))
			{
				default:
					SysLog(LogError, "ProcessHostMsg failed");
					FsmDeclareEvent(this, ExceptionEvent, client);	// Declare an exception

				case 1:		// configuring
					TimerSet(50);		// Config delay
					FsmSetNewState(this, ConfiguringState);
					break;
				
				case 0:
					break;
			}
		}
		TimerSet(RecoveryWaitInterval);
		break;
	}

	case WriteReadyEvent:
		SysLog(LogDebug, "Should only get one of these");
		TimerSet(RecoveryWaitInterval);
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
			SysLog(LogError, "Exception. Disconnecting");
			FecDisconnect();
			TimerSet(RecoveryWaitInterval);
			FsmSetNewState(this, ConnectingState);
			break;

		case TimerEvent:
			TimerSet(RecoveryWaitInterval);
			break;

		case ReadReadyEvent:
		{
			NxClient_t *client = (NxClient_t*)efarg;
			NxClientVerify(client);

			HostFrame_t frame;
			int rlen = HostFrameRecv(client, &frame);

			if (rlen < (int)sizeof(frame.hdr))
			{
				SysLog(LogError, "HostFrameRecv failed");
				FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
			}
			if (rlen > 0)
			{
				ProcessHostMsg(&frame);
				switch(ProcessHostMsg(&frame))
				{
					default:
						SysLog(LogError, "ProcessHostMsg failed");
						FsmDeclareEvent(this, ExceptionEvent, efarg);	// Declare an exception
						break;

					case 1:		// configuring
						TimerSet(50);		// Config delay
						FsmSetNewState(this, ConfiguringState);
						break;
					
					case 0:
						break;
				}
			}
			TimerSet(RecoveryWaitInterval);
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

	case ConnectingState:
		FsmConnectingState(this, evt, efarg);
		break;

	case ConfiguringState:
		FsmConfiguringState(this, evt, efarg);
		break;

	case SteadyState:
		FsmSteadyState(this, evt, efarg);
		break;
	}
}


static int
CommandHandler(NxClient_t *client, Parser_t *parser, String_t *response)
{

	return CommandUnknown;
}


static ProcContext_t*
ProcContextConstructor(ProcContext_t *this, char *file, int lno)
{
	this->fecConnection = NxClientNew();
	this->currentTimer = TimerNew("%sTimer", NxCurrentProc->name);
	return this;
}


static void
ProcContextDestructor(ProcContext_t *this, char *file, int lno)
{
	NxClientDelete(this->fecConnection);
	TimerDelete(this->currentTimer);
}


static Json_t*
ProcContextSerialize(ProcContext_t *this)
{
	ContextVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	JsonAddItem(root, "Fsm", FsmSerialize(this->fsm));
	JsonAddItem(root, "HostConnection", NxClientSerialize(this->fecConnection));
	JsonAddItem(root, "LastHostHeader", HostFrameHeaderSerialize(&this->hostHeader));

	if (this->currentTimer != NULL && this->currentTimer->active)
		JsonAddItem(root, "Timer", TimerSerialize(this->currentTimer));

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
ProcHgSimStart(Proc_t *this, va_list ap)
{

	this->context = ContextNew();
	this->commandHandler = CommandHandler;
	this->contextSerialize = ProcContextSerialize;
	this->contextToString = ProcContextToString;

	Context->fsm = FsmNew(FsmEventHandler, FsmStateToString, FsmEventToString, NullState, "%sFsm", this->name);

	ProcSetSignalHandler(this, SIGTERM, SigTermHandler);	// Termination signal

	// Start the background clock
	TimerSet(InitialWaitInterval);

	return 0;
}
