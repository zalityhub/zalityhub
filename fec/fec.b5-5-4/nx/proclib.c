/*****************************************************************************

Filename:   lib/nx/proclib.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:58 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/proclib.c,v 1.3.4.7 2011/10/27 18:33:58 hbray Exp $
 *
 $Log: proclib.c,v $
 Revision 1.3.4.7  2011/10/27 18:33:58  hbray
 Revision 5.5

 Revision 1.3.4.6  2011/09/24 17:49:46  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/01 14:49:46  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/08/23 12:03:15  hbray
 revision 5.5

 Revision 1.3.4.2  2011/08/17 17:58:58  hbray
 *** empty log message ***

 Revision 1.3.4.1  2011/08/11 19:47:34  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:47  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: proclib.c,v 1.3.4.7 2011/10/27 18:33:58 hbray Exp $ "


#include <sys/types.h>
#include <sys/stat.h>

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/random.h"
#include "include/proclib.h"
#include "include/bt.h"


// Static functions
static void WritePidFile(Proc_t *this);
static void DefaultSignalHandler(NxSignal_t *sig);

static void CmdConnectionEventHandler(EventFile_t *evf, EventPollMask_t pollMask, void * farg);
static void CmdListenEventHandler(EventFile_t *evf, EventPollMask_t pollMask, void * farg);

static int StartCommandListen(Proc_t *this);
static NxClient_t *DoCommandAccept(Proc_t *this);
static int DoCommandRecv(Proc_t *this, NxClient_t *client);
static int DoCommandDisconnect(Proc_t *this, NxClient_t *client);

static int DoCommand(Proc_t *this, NxClient_t *client, char *command);
static CommandResult_t DoSetCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static void DoAuditCmdHelp(char *prior, String_t *response);
static CommandResult_t DoAuditCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static void DoSetLogLevelCmdHelp(char *prior, String_t *response);
static CommandResult_t DoSetLogLevelCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static CommandResult_t DoSetPropCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static CommandResult_t DoWatchCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static CommandResult_t DoShowCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static CommandResult_t DoShowObjsCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static CommandResult_t DoShowProcCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static void DoSetPropCmdHelp(char *prior, String_t *response);
static CommandResult_t DoShowPropCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static void DoShowPropCmdHelp(char *prior, String_t *response);

static CommandResult_t DoShowAuditCountsCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);
static CommandResult_t DoShowLogLevelCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response);

static void HandleKilltimeEvent(Timer_t *tid);


BtNode_t *ProcNodeList = NULL;

Proc_t*
ProcConstructor(Proc_t *this, char *file, int lno, Proc_t *parent, char *name, ProcTag_t tag, ...)
{

	this->memoryLeakDetect = ProcGetPropertyBooleanValue(this, "MemoryLeakDetect");
	this->timeCreated = GetMsTime();
	this->parent = parent;
	this->tag = tag;

	{
		va_list ap;
		va_start(ap, tag);
		vsnprintf(this->name, sizeof(this->name), name, ap);
	}

// make proc home directory
	sprintf(this->procDir, "%s/%s", NxGlobal->nxDir, this->name);
	if (MkDirPath(this->procDir) != 0)
		NxCrash("Unable to create %s", this->procDir);

// Create session directory
	sprintf(this->sessionDir, "%s/sessions", this->procDir);
	if (MkDirPath(this->sessionDir) != 0)
		NxCrash("Unable to create %s", this->sessionDir);

	// Child map
	this->childMap = HashMapNew(1024, "ProcChildrenMap");
	return this;
}


void
ProcDestructor(Proc_t *this, char *file, int lno)
{
	if (this->childMap != NULL)
	{
		for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(this->childMap, entry)) != NULL;)
		{
			Proc_t *child = (Proc_t *)entry->var;
			ProcStop(child, 1);
		}
		HashClear(this->childMap, false);
		HashMapDelete(this->childMap);
	}

	if (this->parent != NULL)	// have a parent
		HashDeleteString(this->parent->childMap, this->name);

	// close the command channels
	if (this->commandServer != NULL)
		NxServerDelete(this->commandServer);

	if ( this->killTimer )
		TimerDelete(this->killTimer);

	if ( ObjectIsOwner(this) )		// if self
	{
	// if there's a procDir; get rid of it
		if ( strlen(this->procDir) > 0 )
		{
			if ( RmDirPath(this->procDir) != 0 )
				SysLog(LogError, "Unable to delete %s; error %s", this->procDir, ErrnoToString(errno));
		}
	}
}


// Exits the process at random times
static void
HandleKilltimeEvent(Timer_t *tid)
{

	Proc_t *proc = (Proc_t *)tid->context;
	ProcVerify(proc);

	NxCurrentProc = proc;

	SysLog(LogWarn, "Killtime of %d expired. Terminating", NxCurrentProc->killTime);
	ProcStop(proc, 0);
}


static void
CmdListenEventHandler(EventFile_t *evf, EventPollMask_t pollMask, void * farg)
{
	Proc_t *proc = NxCurrentProc;
	ProcVerify(proc);

	if (pollMask & EventReadMask)
	{
		NxClient_t *client;

		if ((client = DoCommandAccept(proc)) == NULL)
		{
			SysLog(LogError, "CmdAccept failed");
			NxServerUnlisten(proc->commandServer, true);
			return;
		}
		return;
	}
	else
	{
		SysLog(LogError, "Not sure why I was called with event %s", EventPollMaskToString(pollMask));
	}
}


static void
CmdConnectionEventHandler(EventFile_t *evf, EventPollMask_t pollMask, void * farg)
{
	NxClient_t *client = (NxClient_t *)farg;
	NxClientVerify(client);

	if (pollMask & EventReadMask)
	{
		if (DoCommandRecv(evf->proc, client) != 0)
			DoCommandDisconnect(evf->proc, client);
	}
	else
	{
		SysLog(LogError, "Not sure why I was called with event %s", EventPollMaskToString(pollMask));
		DoCommandDisconnect(evf->proc, client);
	}
}


// Commmand Processing
//
static void
DoSetLogLevelCmdHelp(char *prior, String_t *response)
{
	String_t	*help = StringNew(32);

	for (SysLogLevel lvl = LogLevelMin; lvl != LogLevelMax; ++lvl)
		StringSprintfCat(help, " | %s", SysLogLevelToString(lvl));

	char *tmp = help->str;
	if (strncmp(tmp, " | ", 3) == 0)
		strcpy(tmp, &tmp[3]);	// remove initial " | "
	StringSprintfCat(response, "%s %s\n", prior, tmp);
	StringDelete(help);
}


static CommandResult_t
DoSetLogLevelCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	CommandResult_t ret = CommandBadOption;
	char *word = ParserGetNextToken(parser, " ");

	if (word != NULL && strlen(word) > 0)
	{
		StackClear(SysLogGlobal->loglevelStack);	// pop any saved levels
		do
		{
			if ( SysLogLevelStringToValue(word) != 0xdeadbeef )
			{
				if ((int)SysLogSetLevelString(word) == -1)
				{
					StringSprintfCat(response, "Unable to %s to %s\n", prior, word);
					SysLog(LogError, response->str);
					break;
				}
			}
			else
			{
				StringSprintfCat(response, "Unable to %s to %s\n", prior, word);
				SysLog(LogError, "%s", response->str);
				break;
			}
			StringSprintfCat(response, "loglevel set to %s\n", word);
		} while ((word = ParserGetNextToken(parser, " ")) != NULL && strlen(word) > 0);

		ret = CommandOk;
	}

	if (ret != CommandOk)
		DoSetLogLevelCmdHelp(prior, response);

	return CommandOk;
}


static void
DoSetPropCmdHelp(char *prior, String_t *response)
{
	String_t *help = StringNew(32);

// add dynamic global values
	for (PropertyMap_t *pm = NxGlobal->stdPropertyMap; pm->name != NULL; ++pm)
	{
		if ( pm->dynamic )
			StringSprintfCat(help, " | %s", pm->name);
	}

// then dynamic locals
	for (PropertyMap_t *pm = NxGlobal->propertyMap; pm->name != NULL; ++pm)
	{
		if ( pm->dynamic )
			StringSprintfCat(help, " | %s", pm->name);
	}

	char *tmp = help->str;
	if (strncmp(tmp, " | ", 3) == 0)
		strcpy(tmp, &tmp[3]);	// remove initial " | "
	StringSprintfCat(response, "%s %s\n", prior, tmp);
	StringDelete(help);
}


static CommandResult_t
DoSetPropCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{

	CommandResult_t ret = CommandBadOption;

	ParserNormalizeInput(parser);

	char *name = ParserGetNextToken(parser, " =");
	char *value = NULL;

	if (name != NULL && strlen(name) > 0)
	{
		do
		{
			name = strdup(name);	// need a copy (ParserGetNextToken overwrites)
			value = ParserGetNextToken(parser, " ");

			boolean modified = (NxGetProp(name) != NULL);
			if ( NxSetProp(name, value) < 0 )
			{
				StringSprintfCat(response, "I don't know %s\nTry: ", name);
				ret = CommandBadOption;
				break;
			}

			StringSprintfCat(response, "%s %s=%s\n", modified?"modified":"set", name, value);

			free(name);				// let go
			value = ParserGetNextToken(parser, " "); // skip the value
			ret = CommandOk;
		} while ((name = value) != NULL && strlen(name) > 0);
	}

	if (ret != CommandOk)
		DoSetPropCmdHelp(prior, response);

	return ret;
}


static void
DoAuditCmdHelp(char *prior, String_t *response)
{
	char *tmp = strdup("All");

	for (int event = (int)(AuditFirst) + 1; event < AuditLast; ++event)
	{
		char *l = AuditEventToString(event);
		tmp = realloc(tmp, strlen(tmp) + strlen(l) + 10);
		sprintf(&tmp[strlen(tmp)], " | %s", l);
	}
	if (strncmp(tmp, " | ", 3) == 0)
		strcpy(tmp, &tmp[3]);	// remove initial " | "
	StringSprintfCat(response, "%s %s\n", prior, tmp);
	free(tmp);
}


static CommandResult_t
DoAuditCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{

	CommandResult_t ret = CommandBadOption;

	if (AuditEnableEventMonitor(parser, this->client, response) == NULL)
	{
		DoAuditCmdHelp(prior, response);
	}
	else
	{
		ret = CommandSiezedConnection;
	}

	return ret;
}


static CommandResult_t
DoShowAuditCountsCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	return CommandOk;
}


typedef struct FindObjType_t
{
	Object_t	*obj;
	char		*type;
} FindObjType_t ;

static int
_FindObjType(unsigned long key, BtRecord_t *rec, void *arg)
{
	if ( key == 0 )
		return 0;		// a dummy

	Object_t *obj = (Object_t*)key;
	if ( ! ObjectSignatureIsValid(obj) )
		NxCrash("%p has bad object signature %ld", obj, obj->signature);
	FindObjType_t *fot = (FindObjType_t*)arg;
	if ( stricmp(obj->objName, fot->type) == 0 )
		fot->obj = obj;
	return 0;
}


static Object_t*
FindObjType(BtNode_t *root, char *type)
{
	FindObjType_t fot;
	fot.type = type;
	fot.obj = NULL;
	BtNodeWalk(root, _FindObjType, &fot);
	if ( fot.obj != NULL && (! ObjectSignatureIsValid(fot.obj)) )
		NxCrash("%p has bad object signature %ld", fot.obj, fot.obj->signature);
	return fot.obj;
}


static CommandResult_t
DoShowObjsCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	CommandResult_t ret = CommandBadOption;
	char *word = ParserUnGetToken(parser, " ");

	if (word != NULL && strlen(word) > 0)
	{
		Object_t *obj;
		do
		{
			if ( (obj = FindObjType(RootObjectList, word)) == NULL )
			{
				if ( stricmp(word, "objs") != 0 )
				{
					ret = CommandBadOption;
					break;
				}
				if ( ret != CommandOk )
					StringClear(response);
				ObjectTreeToString(RootObjectList, response);
				ret = CommandOk;
			}
			else
			{
				if ( ret != CommandOk )
					StringClear(response);
				ObjectTreeToString(ObjectGetNodeList(ObjectToVar(obj)), response);
				ret = CommandOk;
			}
		} while ((word = ParserGetNextToken(parser, " ")) != NULL && strlen(word) > 0);

		ret = CommandOk;
	}

	// if (ret != CommandOk)
		// DoShowObjsCmdHelp(prior, response);

	return ret;
}


static CommandResult_t
DoShowProcCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	Json_t *root = ProcSerialize(NxCurrentProc);
	StringCat(response, JsonToDisplay(root));
	JsonDelete(root);
	return CommandOk;
}


static CommandResult_t
DoWatchTcpCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	return SockWatchCmd(this, parser, prior, response);
}


static CommandResult_t
DoShowLogLevelCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{

	SysLogLevel level = SysLogGetLevelFull();

	if ( StackLength(SysLogGlobal->loglevelStack) > 0)	// if we have a stack, get level from bottom of stack
		level = (SysLogLevel)StackBottom(SysLogGlobal->loglevelStack);

	StringSprintfCat(response, "Current log level is %s\n", SysLogLevelToString(level));
	return CommandOk;
}


static void
DoShowPropCmdHelp(char *prior, String_t *response)
{
	String_t *help = StringNew(32);

// add global values
	for (PropertyMap_t *pm = NxGlobal->stdPropertyMap; pm->name != NULL; ++pm)
		StringSprintfCat(help, " | %s", pm->name);

// then local
	for (PropertyMap_t *pm = NxGlobal->propertyMap; pm->name != NULL; ++pm)
		StringSprintfCat(help, " | %s", pm->name);

	char *tmp = help->str;
	if (strncmp(tmp, " | ", 3) == 0)
		strcpy(tmp, &tmp[3]);	// remove initial " | "
	StringSprintfCat(response, "%s %s\n", prior, tmp);
	StringDelete(help);
}


static CommandResult_t
DoShowPropCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{

	CommandResult_t ret = CommandBadOption;

	ParserNormalizeInput(parser);

	char *name = ParserGetNextToken(parser, " =");
	char *value = NULL;

	if (name != NULL && strlen(name) > 0)
	{
		do
		{
			name = strdup(name);	// need a copy (ParserGetNextToken overwrites)
			value = ParserGetNextToken(parser, " ");

			char *value = NxGetProp("%s", name);
			if ( value == NULL )
			{
				StringSprintfCat(response, "I don't know %s\nTry: ", name);
				ret = CommandBadOption;
				break;
			}

			StringSprintfCat(response, "%s=%s\n", name, value);
			free(name);				// let go
			ret = CommandOk;
		} while ((name = value) != NULL && strlen(name) > 0);
	}
	else	// no name, show them all
	{
		ret = CommandOk;

		// global values
		for (PropertyMap_t *pm = NxGlobal->stdPropertyMap; pm->name != NULL; ++pm)
			StringSprintfCat(response, "%s=%s\n", pm->name, NxGetProp("%s", pm->name));

		// then local
		for (PropertyMap_t *pm = NxGlobal->propertyMap; pm->name != NULL; ++pm)
			StringSprintfCat(response, "%s=%s\n", pm->name, NxGetProp("%s", pm->name));

		// then service properties
		{
			char *prefix = "Service_";
			ObjectList_t* list = HashGetOrderedList(NxGlobal->propList, ObjectListStringType);
			for ( HashEntry_t *entry = NULL; (entry = ObjectListRemove(list, ObjectListFirstPosition)); )
			{
				char *name = (char *)entry->string;
				char *value = ((String_t*)entry->var)->str;
				if ( strncmp(name, prefix, strlen(prefix)) == 0 )
					StringSprintfCat(response, "%s=%s\n", name, value);
				HashEntryDelete(entry);
			}
			ObjectListDelete(list);
		}
	}

	if (ret != CommandOk)
		DoShowPropCmdHelp(prior, response);

	return ret;
}


static CommandResult_t
DoWatchCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	CommandDef_t cmds[] = {
		{"tcp", DoWatchTcpCmd},
		{NULL, NULL}
	};

	Command_t *cmd = CommandNew(this->client, cmds);
	CommandResult_t ret = CommandExecute(cmd, parser, prior, response);
	CommandDelete(cmd);
	return ret;
}


static CommandResult_t
DoShowCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	CommandDef_t cmds[] = {
		{"auditcounts", DoShowAuditCountsCmd},
		{"loglevel", DoShowLogLevelCmd},
		{"prop", DoShowPropCmd},
		{"proc", DoShowProcCmd},
		{NULL, NULL}
	};

	Command_t *cmd = CommandNew(this->client, cmds);
	CommandResult_t ret = CommandExecute(cmd, parser, prior, response);
	CommandDelete(cmd);

	if ( ret == CommandBadOption )
		ret = DoShowObjsCmd(this, parser, prior, response);

	return ret;
}


static CommandResult_t
DoSetCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	CommandDef_t cmds[] = {
		{"loglevel", DoSetLogLevelCmd},
		{"prop", DoSetPropCmd},
		{NULL, NULL}
	};

	Command_t *cmd = CommandNew(this->client, cmds);
	CommandResult_t ret = CommandExecute(cmd, parser, prior, response);
	CommandDelete(cmd);
	return ret;
}


static int
DoCommand(Proc_t *this, NxClient_t *client, char *command)
{
	CommandDef_t cmds[] = {
		{"set", DoSetCmd},
		{"show", DoShowCmd},
		{"watch", DoWatchCmd},
		{"audit", DoAuditCmd},
		{NULL, NULL}
	};

	NxClientVerify(client);
	this->commandState = CommandExecuting;
	this->commandDepth = 0;		// start

	Parser_t *parser = ParserNew(strlen(command) + 10);
	ParserSetInputData(parser, command, strlen(command));
	ParserNormalizeInput(parser);

	Command_t *cmd = CommandNew(client, cmds);

	String_t *response = StringNew(32);
	CommandResult_t ret = CommandExecute(cmd, parser, "", response);

	CommandDelete(cmd);

	if (ret < 0)				// if command failed, try the handler
	{
		if (this->commandHandler != NULL)
		{
			// Reset the parser
			ParserRewind(parser);
			ParserNormalizeInput(parser);

			CommandState_t prevState = this->commandState;

			this->commandState = CommandInCommandHandler;
			CommandResult_t prevRet = ret;

			this->commandDepth = 0;
			String_t *r2 = StringNew(32);
			ret = (*(this->commandHandler)) (client, parser, r2);

			if (ret == CommandOk || ret == CommandSiezedConnection)	// handler 'handled' it
			{
				StringCpy(response, r2->str);	// use handler's response
				prevRet = ret;	// and the handler's result
			}
			else if (ret == CommandBadOption)	// handler recognized it
			{
				if (prevRet == CommandUnknown)	// and main cmd did not
				{
					StringCpy(response, r2->str);	// use handler's response
					prevRet = ret;	// and the handler's feedback
				}
				else if (prevRet == CommandBadOption)	// handler recog and main also
				{
					StringCpy(response, r2->str);	// use handler's response
				}
			}
			else if ( ret == CommandUnknown )
			{
				StringCat(response, "Or:\n");
				StringCat(response, r2->str);
			}

			StringDelete(r2);
			ret = prevRet;
			this->commandState = prevState;
		}
	}

#if 0	// not needed?
	if ( ret == CommandUnknown && stricmp(command, "help") == 0 )
	{
		for (CommandDef_t *c = cmds; c != NULL && c->word != NULL; ++c)
			StringSprintfCat(response, "%s\n", c->word);
	}
#endif

	ParserDelete(parser);

// Send the response
	if (ret != CommandResponded && ret != CommandSiezedConnection )	// commandHandler did not return a response
	{
		if (ret < 0 && strlen(response->str) > 0)	// command failed
		{
			String_t *tmp = StringNew(strlen(response->str));
			if ( stricmp(command, "help") == 0 )
				StringSprintfCat(tmp, "Try:\n%s\n", response->str);
			else
				StringSprintfCat(tmp, "I don't know how to %s\nTry:\n%s\n", command, response->str);
			StringCpy(response, tmp->str);
			StringDelete(tmp);
		}

		if (ProcCmdSendResponse(this, client, 0, response->str, strlen(response->str)) < 0)
			SysLog(LogError, "ProcCmdSendResponse failed");
	}

	StringDelete(response);

	if ( ret != CommandSiezedConnection )
		DoCommandDisconnect(this, client);

	this->commandState = CommandIdle;
	return 0;
}


static int
StartCommandListen(Proc_t *this)
{

	// Create the cmd server connection handle
	this->commandServer = NxServerNew();

	char path[1024];
	sprintf(path, "%s/command", this->procDir);
	if ( NxServerListen(this->commandServer, AF_UNIX, SOCK_STREAM, path, 0, EventFileLowPri, EventReadMask, CmdListenEventHandler) != 0 )
	{
		SysLog(LogError, "NxServerListen failed");
		return -1;
	}

	return 0;
}


static void
WritePidFile(Proc_t *this)
{

	if ( this->parent != NULL )		// not the root process, change the pid path
		sprintf(NxGlobal->pidFilePath, "%s/my.pid", this->procDir);

	int fd = open(NxGlobal->pidFilePath, O_TRUNC | O_WRONLY | O_CREAT, 0644);

	if (fd < 0)
		SysLog(LogFatal, "creation of %s failed; errno=%s", NxGlobal->pidFilePath, ErrnoToString(errno));

	fwritef(fd, "%d", getpid());
	close(fd);

	if (chmod(NxGlobal->pidFilePath, 0666) < 0)
		SysLog(LogWarn, "chmod() error=%s: {%s}", ErrnoToString(errno), NxGlobal->pidFilePath);
}


static int
DoCommandDisconnect(Proc_t *this, NxClient_t *client)
{
	if (client != NULL)
		NxClientDelete(client);
	return 0;
}


static NxClient_t *
DoCommandAccept(Proc_t *this)
{

	NxClient_t *client = NxServerAccept(this->commandServer, EventFileLowPri, EventReadMask, CmdConnectionEventHandler);
	if ( client == NULL )
	{
		SysLog(LogError, "NxServerAccept failed");
		return NULL;
	}

	if (! NxClientIsConnected(client) )
	{
		SysLog(LogError, "Failed: Not Connected");
		DoCommandDisconnect(this, client);
		return NULL;
	}

	if (!NxTrustThisIp((unsigned char *)client->evf->peerIpAddrString))
	{
		SysLog(LogWarn, "Disconnecting Untrusted IP %s", client->evf->peerIpAddrString);
		DoCommandDisconnect(this, client);
		return NULL;
	}

	return client;
}


static int
DoCommandRecv(Proc_t *this, NxClient_t *client)
{

	char bfr[MaxSockPacketLen];

	int rlen = NxClientRecvPkt(client, bfr, sizeof(bfr) - 1);

	if (rlen <= 0)
	{
		// SysLog(LogDebug, "Probably a disconnect");
		return 1;
	}
	bfr[rlen] = '\0';

	char *cooked = EncodeUrlCharacters(bfr, strlen(bfr));

	AuditSendEvent(AuditCmdInput, "connid", NxClientUidToString(client), "cmd", cooked);

	if (DoCommand(this, client, bfr) != 0)
		SysLog(LogWarn, "command failed: %s", cooked);

	return 0;
}


int
ProcCmdSendResponse(Proc_t *this, NxClient_t *client, int status, char *bfr, int bfrlen)
{

	ProcVerify(this);

	// char *cooked = EncodeUrlCharacters(bfr, bfrlen);
	AuditSendEvent(AuditCmdResponse, "connid", NxClientUidToString(client), "rsp", cooked);

	if ( ObjectTestVerify(NxClient, client) )		// if viable connection
	{
		if (NxClientSendPkt(client, bfr, bfrlen) != bfrlen)
		{
			SysLog(LogError, "NxClientSendPkt failed");
			return -1;
		}
	}

	return 0;
}


char *
ProcTagToString(ProcTag_t type)
{
	char *text;

	switch (type)
	{
		default:
			text = StringStaticSprintf("ProcTag_%d", (int)type);
			break;

		EnumToString(ProcTagRoot);
		EnumToString(ProcTagT70Proxy);
		EnumToString(ProcTagAuthProxy);
		EnumToString(ProcTagEdcProxy);
		EnumToString(ProcTagWorker);
		EnumToString(ProcTagAud);
		EnumToString(ProcTagWeb);
		EnumToString(ProcTagAuthSim);
		EnumToString(ProcTagEdcSim);
		EnumToString(ProcTagT70Sim);
		EnumToString(ProcTagPosSim);
	}

	return text;
}


Json_t*
ProcSerialize(Proc_t *this)
{
	ProcVerify(this);

	Json_t *root = JsonNew(__FUNC__);

	JsonAddString(root, "Name", this->name);
	JsonAddNumber(root, "Pid", this->pid);
	if ( this->parent )
		JsonAddString(root, "Parent", this->parent->name);
	JsonAddString(root, "CreationTime", MsTimeToStringShort(this->timeCreated, NULL));

	// Set booleans
	boolean isActive = (kill(this->pid, 0) == 0 );
	JsonAddBoolean(root, "Active", isActive);
	JsonAddBoolean(root, "MemoryLeakDetect", this->memoryLeakDetect);
	JsonAddBoolean(root, "IsClone", this->isClone);

	// if this is self
	if (getpid() == this->pid)
	{
		JsonAddString(root, "LogLevel", SysLogLevelToString(SysLogGetLevel()));

	// queued signals
		if ( NxGlobal->signalStack->depth > 0 )
		{
			Json_t *sub = JsonPushObject(root, "QueuedSignals");
			for(int i = NxGlobal->signalStack->depth-1; i >= 0; --i)	// show oldest first
				JsonAddItemToArray(sub, JsonCreateString(NxSignalToString((NxSignal_t*)NxGlobal->signalStack->tuples[i])));
		}
		else
		{
			JsonAddString(root, "QueuedSignals", "None");
		}

	// property settings
		ObjectList_t *props = NxGetPropertyListMatching(this->name);
		if ( props->length > 0 )
		{
			Json_t *sub = JsonPushObject(root, "PropertySettings");
			for ( HashEntry_t *entry = NULL; (entry = ObjectListRemove(props, ObjectListFirstPosition)); )
			{
				char *name = (char *)entry->string;
				String_t *value = (String_t*)entry->var;
				JsonAddString(sub, name, value->str);
				HashEntryDelete(entry);
			}
		}
		else
		{
			JsonAddString(root, "PropertySettings", "None");
		}
		ObjectListDelete(props);

		{ // command connections
			ObjectList_t *list = HashGetOrderedList(this->commandServer->connectionList, ObjectListUidType);
			if ( list->length > 0 )
			{
				Json_t *sub = JsonPushObject(root, "CommandConnections");
				for ( HashEntry_t *entry = NULL; (entry = ObjectListRemove(list, ObjectListFirstPosition)); )
				{
					NxClient_t *child = (NxClient_t *)entry->var;
					JsonAddString(sub, child->evf->name, MsTimeToStringShort(child->evf->msTimeConnected, NULL));
					HashEntryDelete(entry);
				}
			}
			ObjectListDelete(list);
		}

		if ( this->context != NULL && this->contextSerialize != NULL )	// and we have context and a serializer
			JsonAddItem(root, "Context", (*(this)->contextSerialize)(this->context));
	}

	return root;
}


char *
ProcToString(Proc_t *this)
{
	Json_t *root = ProcSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


char*
ProcGetProcDir(Proc_t *this)
{
	ProcVerify(this);
	return this->procDir;
}


Proc_t*
ProcFindChildByPid(Proc_t *this, int pid)
{

	ProcVerify(this);

	SysLog(LogDebug, "Locating pid %d", pid);

	Proc_t *child = NULL;

	for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(this->childMap, entry)) != NULL;)
	{
		child = (Proc_t *)entry->var;
		if (child->pid == pid)
			break;				// this is the one
	}

	if (child != NULL && child->pid == pid)
	{
		SysLog(LogError, "Located %s with pid %d", child->name, pid);
	}
	else
	{
		SysLog(LogError, "Unable to locate pid %d", pid);
		child = NULL;
	}

	return child;
}


static void
ProcExit(void)
{
	NxCurrentProc->isActive = false;
}


static int
ProcCreate(Proc_t *this, int (*prelude)(Proc_t*))
{

	ProcVerify(this);

	boolean doFork = true;

	int ppid = 0;

	if (this->parent == NULL)	// I'm the root machine
		doFork = ! DebugConnected();		// only fork when not debugging...

// fork the proc
	if ( doFork )
	{
		ppid = getpid();		// the parent pid

		int pid = 0;
		switch ((pid = fork()))
		{
		default:				// I'm the parent
			this->pid = pid;
			SysLog(LogDebug, "fork of %s created pid %d", this->name, this->pid);
			if ( this->parent == NxCurrentProc )		// this is a child
			{
				ProcVerify(this->parent);
				if ( HashAddString(this->parent->childMap, this->name, this) == NULL )
					SysLog(LogFatal, "HashAddString of Proc '%s' failed", this->name);
			}
			this->isActive = true;
			return pid;			// Bon Voyage
			break;

		case 0:				// I'm the child
			break;

		case -1:				// I'm an error
			SysLog(LogError, "fork of %s failed", this->name);
			return -1;
			break;
		}
	}

	NxCurrentProc = this;	// set current proc context as one being started
	this->isActive = true;
#ifdef LEAK_DETECTION
	LeakStop();
#endif
	{
		char *name = StringStaticSprintf("NxProc%s", this->name); // build proc name
		SetProcTitle(NxGlobal->argv, NxGlobal->argvSize, name); // set proc name
	}

	setenv("GMON_OUT_PREFIX", "gmon.out", 1);

	atexit(ProcExit);
	this->pid = getpid();
	NxGlobal->pid = this->pid;
	this->ppid = ppid;
	srand(NxGetTime());	// seed

// If requested, enter debug

	if ( ProcGetPropertyBooleanValue(this, "Debug") )
		RequestDebug(1000);

// Write my pid
	WritePidFile(this);

// If this is the root, make it a daemon
	if (this->parent == NULL)	// I'm the root machine
	{
		if ( doFork )		// need to make into a daemon
		{
			// new file mode mask
			umask(0);

			// create a new sid for this child
			pid_t sid = setsid();

			if (sid < 0)
				SysLog(LogFatal, "setsid failed; errno=%s", ErrnoToString(errno));

			// Don't close the standard out, err; they are attached by syslog
			close(STDIN_FILENO);
		}
	}
	else		// not the root... close my command listen... then call prelude...
	{
		if (this->parent->commandServer)	// if parent gave me a command channel
			NxServerDelete(this->parent->commandServer);
	
		if ( prelude != NULL )
		{
			if ((*(prelude)) (this->parent) != 0)
				SysLog(LogFatal, "prelude of %s failed", this->name);
			NxCurrentProc = this;	// to be safe, re-set current proc context as one being started
		}
	}

// set default signal handlers
	for(int i = 0; i < _NSIG; ++i )
		this->signalHandlers[i] = DefaultSignalHandler;

	TimerCancelAll();

	{	// Create audit events
		char path[1024];
		sprintf(path, "%s/auditcounts.shm", this->procDir);
		NxGlobal->auditCounts = AuditCountsNew();
	}

	// activate the command channel
	if (StartCommandListen(this) != 0)
		SysLog(LogFatal, "StartCommandListen failed");

// If a Kill timer is requested; start it

	{
		int kt = ProcGetPropertyIntValue(this, "Killtime");
		if ( kt > 0 )
		{
			this->killTime = RandomRange(1, kt);
			SysLog(LogDebug, "Arming Kill timer for %d seconds", this->killTime);
			this->killTimer = TimerNew("%sKillTimer", this->name);
			this->killTimer->context = this;
			TimerActivate(this->killTimer, this->killTime * 1000, HandleKilltimeEvent);
		}
	}

	SysLogRecycle(false);

	return 0;
}


int
ProcStartV(Proc_t *child, int (*prelude) (Proc_t *), int (*startupEntry) (Proc_t *, va_list ap), va_list ap)
{

	ProcVerify(child);

	if (child->parent != NULL)	// not the Root
	{
		Proc_t *parent = child->parent;
	
		ProcVerify(parent);

		// Verify child proc name is not already in use; if so, then fail start request
		if (HashFindString(parent->childMap, child->name) != NULL)
		{
			SysLog(LogError, "This Proc '%s' has already been started", child->name);
			return -1;
		}
	}

	int ret = ProcCreate(child, prelude);
	if ( ret < 0 )
		SysLog(LogFatal, "ProcCreate of %s failed", child->name);

	if ( ret != 0 )
		return ret;		// I'm the parent, I'm done...

	// This is the child process
	// Start the finite state machine
	if ((*startupEntry) (child, ap) != 0)
	{
		SysLog(LogError, "Startup of %s has failed", child->name);
		SysLog(LogDebug, "exit(1)");
		exit(1);
	}

// Start the Idle loop

	NxDoIdle();			// The idle loop

	SysLog(LogFatal, "Should never reach child");
	return 0;
}


int
ProcStart(Proc_t *child, int (*prelude)(Proc_t *), int (*startupEntry) (Proc_t *, va_list ap), ...)
{
 	ProcVerify(child);
	va_list ap;
	va_start(ap, startupEntry);
	return ProcStartV(child, prelude, startupEntry, ap);
}


int
ProcClone(Proc_t *this, int (*prelude)(Proc_t*))
{

	ProcVerify(this);

	String_t *childName = StringNew(strlen(this->name)+32);
	// Create a unique name
	for(int i = HashMapLength(this->childMap); ; ++i )
	{
		StringSprintf(childName, "%s_%d", this->name, i);
		// Exit if this name is in use...
		if (HashFindString(this->childMap, childName->str) == NULL)
			break;		// have a unique value
	}

	Proc_t *child = ProcNew(this, childName->str, this->tag);
	StringDelete(childName);

	int ret = ProcCreate(child, prelude);
	if ( ret < 0 )
		SysLog(LogFatal, "ProcCreate of %s failed", child->name);

	child->isClone = true;	// A clone...
	return ret;		// done
}


int
ProcStop(Proc_t *this, int status)
{

	ProcVerify(this);

	AuditSendEvent(AuditProcStopped, "name", this->name);

	SysLog(LogWarn, "Stopping with exit status %d", status);
	exit(status);
	kill(getpid(), SIGABRT);	// try for a core
}


int
ProcSignal(Proc_t *this, int signo)
{
	ProcVerify(this);

	AuditSendEvent(AuditSignalSent, "name", this->name, "signo", NxSignalNbrToString(signo));

	SysLog(LogDebug, "Sending signal %s to %s", NxSignalNbrToString(signo), this->name);
	kill(this->pid, signo);
	return 0;
}


char*
ProcGetPropertyValue(Proc_t *this, char *propName, ...)
{
 	ProcVerify(this);

	char *tmp = strdup(this->name);	// make a copy of my name
	char *ptr;

	char *prop = _NxGetPropFull(NxGlobal, false, "%s.%s", tmp, propName);

	if (prop == NULL)			// did not find it... remove implementation suffix and try again
	{
		if ((ptr = strchr(tmp, '.')) != NULL)	// if I have an optional port suffix
			*ptr = '\0';		// remove it

		prop = _NxGetPropFull(NxGlobal, false, "%s.%s", tmp, propName);

		if (prop == NULL)		// did not find it... remove implementation suffix and try again
		{
			if ((ptr = strchr(tmp, '_')) != NULL)	// if I have an implementation suffix
			{
				*ptr = '\0';	// remove it
				prop = _NxGetPropFull(NxGlobal, false, "%s.%s", tmp, propName);
				if (prop == NULL)		// did not find it... try platform
					prop = _NxGetPropFull(NxGlobal, false, "Platform.%s", propName);
			}
		}
	}

	free(tmp);
	return prop;
}


void
_ProcHandleSignals(Proc_t *this)
{
 	ProcVerify(this);

	while ( NxGlobal->signalStack->depth > 0 )
	{
		NxSignal_t *sig = StackPop(NxGlobal->signalStack);
		int signo = sig->si.si_signo;
		if ( signo >= 0 && signo < _NSIG )
		{
			AuditSendEvent(AuditSignalRecv, "name", this->name, "signo", NxSignalNbrToString(signo));
			SysLog(LogDebug, "Signal %s", NxSignalToString(sig));

			if ( NxGlobal->steadyState && NxCurrentProc->signalHandlers[signo] != NULL )
			{
				(*(this->signalHandlers[signo]))(sig);
			}
			else
			{
				SysLog(LogWarn, "No signal handler for %s", NxSignalToString(sig));
				NxSignalDelete(sig);			// ignored...
			}
		}
		else
		{
			SysLog(LogWarn, "Signal %s not in range", NxSignalToString(sig));
			NxSignalDelete(sig);			// ignored...
		}
	}
}


static void
DefaultSignalHandler(NxSignal_t *sig)
{
 	NxSignalVerify(sig);

	SysLog(LogWarn, "Ignored: %s", NxSignalToString(sig));
	NxSignalDelete(sig);
}


ProcSignalHandler_t
ProcSetSignalHandler(Proc_t *this, int signo, ProcSignalHandler_t sigHandler)
{
 	ProcVerify(this);
	ProcSignalHandler_t prev = NULL;

	if ( signo >= 0 && signo < _NSIG )
	{
		prev = this->signalHandlers[signo];
		this->signalHandlers[signo] = sigHandler;
	}
	else
	{
		SysLog(LogError, "Signal %d not in range", signo);
	}
	return prev;
}
