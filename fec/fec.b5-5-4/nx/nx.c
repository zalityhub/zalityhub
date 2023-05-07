/*****************************************************************************

Filename:	lib/nx/nx.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:57 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/nx.c,v 1.3.4.10 2011/10/27 18:33:57 hbray Exp $
 *
 $Log: nx.c,v $
 Revision 1.3.4.10  2011/10/27 18:33:57  hbray
 Revision 5.5

 Revision 1.3.4.9  2011/09/24 17:49:45  hbray
 Revision 5.5

 Revision 1.3.4.7  2011/09/01 14:49:45  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/08/24 13:22:29  hbray
 Renamed

 Revision 1.3.4.4  2011/08/23 12:03:14  hbray
 revision 5.5

 Revision 1.3.4.3  2011/08/17 17:58:58  hbray
 *** empty log message ***

 Revision 1.3.4.2  2011/08/11 19:47:33  hbray
 Many changes

 Revision 1.3.4.1  2011/08/01 16:11:29  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:47  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: nx.c,v 1.3.4.10 2011/10/27 18:33:57 hbray Exp $ "


#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <stddef.h>

#include "libxml/parser.h"
#include "libxml/tree.h"
#include "libxml/xpath.h"
#include "libxml/xpathInternals.h"

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/nx.h"
#include "include/sql.h"
#include "include/hostio.h"
#include "include/random.h"

#include "include/utillib.h"
#include "include/json.h"



#define NxNew(iname, version, llevel, new, objLogPort, propertyMap, argc, argv, argvSize) ObjectNew(Nx, iname, version, llevel, new, objLogPort, propertyMap, argc, argv, argvSize)
#define NxVerify(var) ObjectVerify(Nx, var)
#define NxDelete(var) ObjectDelete(Nx, var)

// Static Functions
static Nx_t* NxConstructor(Nx_t *this, char *file, int lno, char *iname, char *version, SysLogLevel level, boolean new, int objLogPort, PropertyMap_t *propertyMap, int argc, char *argv[], int argvSize);
static void NxDestructor(Nx_t *this, char *file, int lno);

static void NxInitGlobal(Nx_t *this, int objLogPort, SysLogLevel level);
static int ParsePropertyFile(Nx_t *this, char *name);
static void DisplayNodeProperties(Nx_t *this);
static void NormalizeProperty(Nx_t *this, PropertyMap_t *map);
static void CheckRequiredProperties(Nx_t *this, PropertyMap_t *pm);
static void FinalizeProperties(Nx_t *this);
static void _NxSetIgnoreIpList(Nx_t *this, char *ipList);
static void _NxSetTrustedIpList(Nx_t *this, char *ipList);


// Globals; only one...

static SysLog_t				SyslogDummy = {LogWarn};
static Audit_t				AuditDummy;
static AuditCounts_t		AuditCountsDummy;
static EventGlobal_t		EfgDummy;
static Proc_t				ProcDummy = { "ProcMain" };

Nx_t	NxDummy =
{
	true,
	false,
	false,
	NULL,
	&SyslogDummy,
	&AuditDummy,
	&AuditCountsDummy,
	&EfgDummy,
	&ProcDummy,
	NULL,
	NULL,
	NULL
};

Nx_t	*NxGlobal = &NxDummy;


//	char				*name;
//	boolean				required;
//	boolean				dynamic;
//	PropertySuffix_t	suffixes[16];
static PropertyMap_t StdPropertyMap[] =
{
	{"Platform.Name",					true,	false,  {{0}}},
	{"Platform.DebugFatal",				true,	true,   {{0}}},
	{"Platform.MaxFd",					true,	false,  {{0}}},
	{"Platform.DbName",					true,	false,  {{0}}},
	{"Platform.AuditSessions",			true,	true,  {{0}}},
	{"Platform.CmdMaxTime",				true,	true,  {{'M',60}, {'H',60*60}, {0}}},
	{"Platform.CmdTrustedIp",			true,	true,  {{0}}},
	{"Platform.DisplayThisConfig",		true,	false, {{0}}},
	{"SysLog.Level",					true,	false, {{0}}},
	{"SysLog.MaxFileSize",				true,	true,  {{'K',1024}, {'M',1024*1024}, {0}}},
	{"SysLog.MaxFileTime",				true,	true,  {{'M',60}, {'H',60*60}, {0}}},
	{"Platform.IgnoreIp",				true,	true,  {{0}}},
	{"Platform.OclAddr",				true,	false, {{0}}},
	{"Platform.OclPort",				true,	false, {{0}}},
	{"Platform.PidFileName",			true,	false, {{0}}},
	{"Platform.TcpPortOffset",			true,	false, {{0}}},
	{NULL, 0}
};


void
_NxSignalPush(Nx_t *this, NxSignal_t *nxSignal)
{
	NxVerify(this);
	StackPush(this->signalStack, nxSignal);	// push the signal
}


static void
SigPushHandler(int sig, siginfo_t *si, void *obs)
{
	NxSignal_t	*nxSignal = NxSignalNew(si);

	if ( ! NxGlobal->steadyState )
		NxCrash("Signal %s not handled during startup", NxSignalToString(nxSignal));

	_NxSignalPush(NxGlobal, nxSignal);
}


struct sigaction
NxSetSigAction(int signum, void (*sighandler) (int, siginfo_t *, void *))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = sighandler;
	sa.sa_flags = SA_SIGINFO;	// using sa_sigaction

	struct sigaction oldsa;
	sigaction(signum, &sa, &oldsa);
	return oldsa;
}


static void
NxInitGlobal(Nx_t *this, int objLogPort, SysLogLevel level)
{
	this->pagesize = getpagesize();

	this->signalStack = NULL;		// none

// Set singleton dummys

	memset(&SyslogDummy, 0, sizeof(SyslogDummy));
	memset(&AuditDummy, 0, sizeof(AuditDummy));
	memset(&AuditCountsDummy, 0, sizeof(AuditCountsDummy));
	memset(&EfgDummy, 0, sizeof(EfgDummy));
	memset(&ProcDummy, 0, sizeof(ProcDummy));

	this->dummy = true;
	this->objLogPort = objLogPort;
	this->steadyState = false;
	this->syslog = &SyslogDummy;
	this->audit = &AuditDummy;
	this->auditCounts = &AuditCountsDummy;
	this->efg = &EfgDummy;
	this->currentProc = &ProcDummy;
	this->context = NULL;
	this->sysname = NULL;
	this->sysid = NULL;
	this->nodename = NULL;

// Set default values
	strcpy(this->currentProc->name, "ProcMain");
	this->syslog->logLevelValue = level;
}


void
NxInit(char *name, char *version, SysLogLevel level, boolean new, int objLogPort, PropertyMap_t *propertyMap, int argc, char *argv[])
{

// Move the environment variables before they become 'cached'.

	int argvSize = RelocateEnviron(argv);

// Guard against running as root...
//
	if ((getuid() == 0) || (geteuid() == 0))
	{
		fwritef(1, "\nI don't like to run as the root user\n");
		kill(getpid(), SIGABRT);	// try for a core
		exit(1);				// always exit
	}

	JsonInit();

	srand(NxGetTime());			// seed

	NxInitGlobal(NxGlobal, objLogPort, level);

// Create main
//
	static PropertyMap_t emptyMap[] =
	{
		{NULL, 0}
	};

	if ( propertyMap == NULL )
		propertyMap = emptyMap;

	NxGlobal = NxNew(name, version, level, new, objLogPort, propertyMap, argc, argv, argvSize);

	SysLog(LogAny, "%s", ObjectToString(NxGlobal));
}



BtNode_t *NxNodeList = NULL;


static Nx_t*
NxConstructor(Nx_t *this, char *file, int lno, char *name, char *version, SysLogLevel level, boolean new, int objLogPort, PropertyMap_t *propertyMap, int argc, char *argv[], int argvSize)
{

	NxInitGlobal(this, objLogPort, level);			// Set the dummys again
	NxGlobal = this;					// newly allocated Nx_t

	this->objLogPort = objLogPort;

	this->pid = getpid();

// get the homedir; and create the base directory
	{
		struct passwd *passwd;
		passwd = getpwuid(getuid());
		strcpy(this->homeDir, passwd->pw_dir);

	// Create the base directory
		sprintf(this->nxDir, "/tmp/.%s/proc%s", passwd->pw_name, name);
		if ( new )		// remove previous contents
		{
			if ( access(this->nxDir, F_OK) == 0 )
			{
				if ( RmDirPath(this->nxDir) != 0 )
					NxCrash("Unable to remove %s", this->nxDir);
			}
		}
		if (MkDirPath(this->nxDir) != 0)
			NxCrash("Unable to create %s", this->nxDir);
	}

// Make temporary global pointers

	strncpy(this->name, name, sizeof(this->name) - 1);
	strncpy(this->version, version, sizeof(this->version) - 1);
	this->stdPropertyMap = StdPropertyMap;
	this->propertyMap = propertyMap;
	this->argvSize = argvSize;
	this->argc = argc;
	this->argv = argv;

// Create the log backup directory
	{
		char *dir = "logs";
		if (MkDirPath(dir) != 0)
			NxCrash("Unable to create %s", dir);
	}

// Create the event list
	this->efg = EventGlobalNew();

// Open the logger
	this->syslog = SysLogNew(this->name, this->version);

// Set default levels
	SysLogSetLevel(level);

// ok to use logging

	this->dummy = false;

// Continue initializations

// Create signal stack
	this->signalStack = StackNew(NULL, "SignalStack");
	StackGrow(this->signalStack, 2000);

	NxSetSignalHandlers(this);

	this->audit = AuditNew();

	this->trustedIpList = HashMapNew(128, "TrustedIpList");
	this->ignoreIpList = HashMapNew(128, "IgnoreIpList");

	this->natList = HashMapNew(128, "NatList");

	this->propList = HashMapNew(MAXSERVICES + MAXWORKERS, "Properties");

// Parse nx properties file
	{
	// Global file
		if ( ParsePropertyFile(this, "nx") != 0 )
			NxCrash("Unable to parse %s.xml", "nx");

	// Implementation file
		if ( ParsePropertyFile(this, this->name) != 0 )
			NxCrash("Unable to parse %s.xml", this->name);

		FinalizeProperties(this);

		// Make certain each required property is present (and has a value)
		CheckRequiredProperties(this, this->stdPropertyMap);
		CheckRequiredProperties(this, this->propertyMap);
	}

// maybe display the properties
	if (NxGetPropertyBooleanValue("Platform.DisplayThisConfig"))
		DisplayNodeProperties(this);

// Set the log options
	SysLogSetLevelString(NxGetPropertyValue("SysLog.Level"));
	SysLogGlobal->doublespace = NxGetPropertyBooleanValue("SysLog.Doublespace");
	SysLogGlobal->jsonFormat = NxGetPropertyBooleanValue("SysLog.JsonFormat");

// Setup the pidfile pathname; should be in users homedir to survive a reinstall...

	sprintf(this->pidFilePath, "%s/%s", this->homeDir, NxGetPropertyValue("Platform.PidFileName"));

// Start sql
	{
		char *dbname = NxGetPropertyValue("Platform.DbName");
		if ( strlen(dbname) > 0 )
			this->sql = NxSqlNew(dbname);
	}

	if (SockInit() != 0)
		SysLog(LogFatal, "SockInit failed");

	AuditSendEvent(AuditSystemStarted, "name", this->name, "version", this->version);

	AuditSendEvent(AuditNxStarted, "name", this->name);

	return this;
}


static void
NxDestructor(Nx_t *this, char *file, int lno)
{
}


Json_t*
NxSerialize(Nx_t *this)
{
	NxVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Name", this->name);
	JsonAddString(root, "DebugFatal", BooleanToString(this->debugFatal));
	JsonAddNumber(root, "ObjLogPort", this->objLogPort);
	JsonAddString(root, "Version", this->version);
	JsonAddString(root, "SysName", NullToValue(this->sysname, ""));
	JsonAddString(root, "SysId", NullToValue(this->sysid, ""));
	JsonAddString(root, "NodeName", NullToValue(this->nodename, ""));
	JsonAddNumber(root, "PageSize", this->pagesize);
	JsonAddNumber(root, "Pid", this->pid);
	JsonAddString(root, "PidFilePath", this->pidFilePath);
	JsonAddString(root, "HomeDir", this->homeDir);
	JsonAddString(root, "NxDir", this->nxDir);
	JsonAddNumber(root, "MaxFd", this->maxFd);
	JsonAddString(root, "LogLevel", SysLogLevelToString(this->syslog->logLevelValue));
	JsonAddNumber(root, "LogMaxFileSize", this->logMaxFileSize);
	JsonAddNumber(root, "LogMaxFileTime", NxGetPropertyIntValue("SysLog.MaxFileTime"));
	JsonAddString(root, "AuditExceptionFilePath", this->auditExceptionFilePath);
	JsonAddString(root, "AuditSessions", BooleanToString(this->auditSessions));
	return root;
}


char*
NxToString(Nx_t *this)
{
	Json_t *root = NxSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


void
NxSetSignalHandlers(Nx_t *this)
{

	StackClear(this->signalStack);

// Set default signal handlers
	NxSetSigAction(SIGPIPE, SigPushHandler);		// nothing good happens with SIGPIPE; we get a write EPIPE error anyway...
	NxSetSigAction(SIGHUP,  SigPushHandler);		// Hangup detected on controlling terminal or death of controlling process
	NxSetSigAction(SIGINT,  SigPushHandler);		// Interrupt from keyboard
	NxSetSigAction(SIGALRM, SigPushHandler);		// Timer signal from alarm(2)
	NxSetSigAction(SIGTERM, SigPushHandler);		// Termination signal
	NxSetSigAction(SIGUSR1, SigPushHandler);		// User-defined signal 1
	NxSetSigAction(SIGUSR2, SigPushHandler);		// User-defined signal 2
	NxSetSigAction(SIGCHLD, SigPushHandler);		// Child stopped or terminated
	NxSetSigAction(SIGXFSZ, SigPushHandler);		// File size exceeded
}

int
NxLogFileBackup(Nx_t *this, char *name)
{

	time_t tod = time(NULL);
	struct tm *ts = localtime(&tod);
	char backupPath[BUFSIZ];
	sprintf (backupPath,
		"logs/%s"
		"%04d%02d%02d_%02d%02d%02d.log",
		name,
		ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec);

	char curPath[BUFSIZ];
	sprintf (curPath, "%s.log", name);

	SysLog(LogDebug, "rename %s to %s", curPath, backupPath);

	if ( rename(curPath, backupPath) != 0 )
	{
		SysLog(LogError, "Unable to rename %s to %s", curPath, backupPath);
		return -1;
	}

	return 0;
}


static void
CheckRequiredProperties(Nx_t *this, PropertyMap_t *pm)
{

	boolean success = true;		// assumption
	for (; pm->name != NULL; ++pm)
	{
		if ( ! pm->required )
			continue;

		char *prop;
		if ( (prop = _NxGetPropFull(this, false, "%s", pm->name)) == NULL || strlen(prop) <= 0 )
		{
			SysLog(LogError, "Required property %s is missing", pm->name);
			success = false;
		}
	}

	if ( ! success )
		SysLog(LogFatal, "Required properties are missing; terminating");
}


static void
FinalizeProperties(Nx_t *this)
{

// Resolve references
	{
		ObjectList_t* list = HashGetOrderedList(this->propList, ObjectListStringType);
		for ( HashEntry_t *entry = NULL; (entry = (HashEntry_t*)ObjectListRemove(list, ObjectListFirstPosition)); )
		{
			char *name = (char *)entry->string;
			_NxSetProp(this, name, "%s", _NxGetPropFull(this, true, "%s", name));
			HashEntryDelete(entry);
		}
		ObjectListDelete(list);
	}

	{ // Normalize values
		for (PropertyMap_t *pm = this->stdPropertyMap; pm->name != NULL; ++pm)
			NormalizeProperty(this, pm);
		for (PropertyMap_t *pm = this->propertyMap; pm->name != NULL; ++pm)
			NormalizeProperty(this, pm);
	}
}


// Never returns
void
_NxDoIdle(Nx_t *this)
{

	NxVerify(this);

	// The idle loop

	for (;;)
	{
		int ret = EventPoll((60 * 1000));

		if (ret < 0)
			SysLog(LogFatal, "EventPoll failed");
	}
}


static void
_NxSetIgnoreIpList(Nx_t *this, char *ipList)
{

    HashClear(this->ignoreIpList, false);		// dump
    HashMapDelete(this->ignoreIpList);		// old one
    this->ignoreIpList = HashMapNew(128, "IgnoreIpList");
    {
        Parser_t *parser = ParserNew(strlen(ipList) + 10);
        ParserSetInputData(parser, ipList, strlen(ipList));
        ParserNormalizeInput(parser);

        char *word;
        while ( strlen(word = ParserGetNextToken(parser, " ")) > 0 )
        {
			static struct sockaddr_in inetaddr;
			memset(&inetaddr, 0, sizeof(inetaddr));
			inetaddr.sin_addr.s_addr = inet_addr(word);
			if (HashUpdateUINT32(this->ignoreIpList, inetaddr.sin_addr.s_addr, StringNewCpy(word)) == NULL)
                SysLog(LogFatal, "HashUpdateUINT32 of IgnoreIp %s failed", word);
        }
        ParserDelete(parser);
    }
}


int
_NxIgnoreThisIp(Nx_t *this, unsigned char *peerIp)
{
	NxVerify(this);

	if ( HashFindUINT32(this->ignoreIpList, *(long*)peerIp) != NULL )
	{
		// SysLog(LogDebug, "%s\n", IpAddrToString((unsigned char*)peerIp));
		return true;
	}

	return false;
}


static void
_NxSetTrustedIpList(Nx_t *this, char *ipList)
{
    // the trusted IP list

    HashClear(this->trustedIpList, false);		// dump
    HashMapDelete(this->trustedIpList);		// old one
    this->trustedIpList = HashMapNew(128, "TrustedIpList");
    {
        Parser_t *parser = ParserNew(strlen(ipList) + 10);
        ParserSetInputData(parser, ipList, strlen(ipList));
        ParserNormalizeInput(parser);

        char *word;
        while ( strlen(word = ParserGetNextToken(parser, " ")) > 0 )
        {
            if ( HashUpdateString(this->trustedIpList, word, StringNewCpy("true")) == NULL )
                SysLog(LogFatal, "HashUpdatedString of TrustedIp %s failed", word);
        }
        ParserDelete(parser);
    }
}


boolean
_NxTrustThisIp(Nx_t *this, unsigned char *peerIp)
{
	NxVerify(this);
	return (HashFindStringVar(this->trustedIpList, (char *)peerIp) != NULL);
}


void
_NxCrash(Nx_t *this, char *file, int lno, const char *fnc, char *msg, ...)
{
	char tmp[128 * 1024];

	{
		va_list ap;

		va_start(ap, msg);
		vsnprintf(tmp, sizeof(tmp), msg, ap);
	}
	
	fwritef(1, "\n%s %d FTL %s %s:%d %s: Fatal error: %s\n",
		SysLogTimeToString(), getpid(),
		this->currentProc?this->currentProc->name:"",
		file, lno, fnc, tmp);

	if ( this->debugFatal )
	{
		if ( ! DebugConnected() )
			RequestDebug(1000);
		else
			kill(getpid(), SIGTRAP);
	}
	else
	{
		this->currentProc->isActive = false;

		{			// attempt a stack trace
			StringArray_t *st = GetStackTrace();
			for(int i = 0; i < st->next; ++i )
				write(1, st->array[i]->str, st->array[i]->len);
		}
		kill(getpid(), SIGABRT);	// try for a core
	}

	exit(1);					// always exit
}


void
NxShutdown(Nx_t *this)
{

	NxVerify(this);

	SysLog(LogError, "Terminating process %d", getpid());

// if not running direct from command line; and this is the parent
// remove the pid file; we're gone...
	if (this->currentProc->parent == NULL)
	{
		SysLog(LogWarn, "Removing the pid file %s", this->pidFilePath);
		if (unlink(this->pidFilePath) < 0)
			SysLog(LogError, "unlink of %s failed; errno=%s", this->pidFilePath, ErrnoToString(errno));
	}

	AuditSendEvent(AuditSystemStopped, "reason", "Shutdown Called");
	exit(1);
}


#define NxNodePropertiesXpath ""

static void
_ParseNodeProperties(Nx_t *this, char *name, xmlXPathContextPtr xpathCtx, char *section, char *xpath)
{

	char tmp[1024];
	sprintf(tmp, "/%s/%s%s", name, section, xpath);
	tmp[1] = toupper(tmp[1]);		// make name cap

	xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar *)tmp, xpathCtx);

	if (xpathObj == NULL ||
		xpathObj->nodesetval == NULL ||
		xpathObj->nodesetval->nodeNr < 1 ||
		xpathObj->nodesetval->nodeTab[0]->type != XML_ELEMENT_NODE ||
		xpathObj->nodesetval->nodeTab[0]->children == NULL || xpathObj->nodesetval->nodeTab[0]->children->next == NULL)
	{
		if ( xpathObj != NULL )
			xmlXPathFreeObject(xpathObj);
		return;		// nothing here... move along
	}

	StringNewStatic(propertyName, 32);

	for (xmlNodePtr node = xpathObj->nodesetval->nodeTab[0]->children->next; node != NULL; node = node->next)
	{
		if (node->type != XML_ELEMENT_NODE)
			continue;			// skip

		if (node->properties == NULL || node->properties->children == NULL)
		{
			SysLog(LogError, "Unable to find a value in %s", NxNodePropertiesXpath);
			if ( xpathObj != NULL )
				xmlXPathFreeObject(xpathObj);
			return;
		}

// Ok; we're looking at a node name; add its attributes

		boolean isNatNode = (strcmp((char*)node->name, "Nat") == 0);

		xmlAttr *attr = node->properties;
		while (attr != NULL)
		{
			StringSprintf(propertyName, "%s.%s", node->name, (char *)attr->name);

			if (attr->children == NULL || attr->children->content == NULL)
				SysLog(LogFatal, "Unable to find a value in %s.%s", NxNodePropertiesXpath, propertyName->str);

			if ( isNatNode )
			{
				static char *from = NULL;
				if ( strcmp((char*)attr->name, "From") == 0 )
				{
					if ( from )
						SysLog(LogFatal, "Nat/From without a Nat/To");
					from = strdup((char*)attr->children->content);
				}
				else
				{
					if ( ! from )
						SysLog(LogFatal, "Nat/To without a Nat/From");
					// have the from/to pair... insert a nat entry
					Pair_t *nat = PairNew();
					nat->first = from;
					nat->second = strdup((char*)attr->children->content);
					HashUpdateString(this->natList, from, nat);
					from = NULL;
				}
			}
			else
			{
				_NxSetProp(this, propertyName->str, "%s", (char*)attr->children->content);
			}
			attr = attr->next;
		}
	}

	if ( xpathObj != NULL )
		xmlXPathFreeObject(xpathObj);
}


static void
ParseNodeProperties(Nx_t *this, char *name, xmlXPathContextPtr xpathCtx, char *xpath)
{
	_ParseNodeProperties(this, name, xpathCtx, "Common", xpath);
	_ParseNodeProperties(this, name, xpathCtx, this->nodename, xpath);
}


static void
NormalizeProperty(Nx_t *this, PropertyMap_t *pm)
{

// normalize/convert value

	if ( pm != NULL )
	{
		char *value = _NxGetPropFull(this, false, "%s", pm->name);
		if ( value != NULL && strlen(value) > 0 )
		{
			char tmp[1024*1024];
			unsigned val = atol(value);		// convert prefix
			char sufchar = toupper(value[strlen(value)-1]);

			for (PropertySuffix_t *suf = pm->suffixes; suf->suffix != 0; ++suf )
			{
				if ( sufchar == suf->suffix )
				{
					val *= suf->val;
					sprintf(tmp, "%u", val);
					value = tmp;
					break;
				}
			}

			if ( HashUpdateString(this->propList, pm->name, StringNewCpy(value)) == NULL )
				SysLog(LogFatal, "HashUpdateString failed");
		}
	}

// set the dynamic values

	this->debugFatal = NxGetPropertyBooleanValue("Platform.DebugFatal");
	this->maxFd = NxGetPropertyIntValue("Platform.MaxFd");
	this->logMaxFileSize = NxGetPropertyIntValue("SysLog.MaxFileSize");
	if ( this->sysname != NULL )
		free(this->sysname);
	this->sysname = strdup(NxGetPropertyValue("Platform.sysname"));
	if ( this->sysid != NULL )
		free(this->sysid);
	this->sysid = strdup(NxGetPropertyValue("Platform.sysid"));

// AuditSession
	this->auditSessions = NxGetPropertyBooleanValue("Platform.AuditSessions");

	_NxSetTrustedIpList(this, NxGetPropertyValue("Platform.CmdTrustedIp"));

	_NxSetIgnoreIpList(this, NxGetPropertyValue("Platform.IgnoreIp"));
}


static int
ParsePropertyFile(Nx_t *this, char *name)
{

	char fname[1024];
	sprintf(fname, "%s.xml", name);

// first, get my system name
	struct utsname utsname;
	uname(&utsname);
	char *tmp = strdup((char*)utsname.nodename);
	char *nodename = Downshift(tmp);
	char *ptr = strchr(nodename, '.');
	if ( ptr != NULL )
		*ptr = '\0';            // trim at optional dot
	if ( this->nodename )
		free(this->nodename);
	this->nodename = strdup(nodename);
	free(tmp);

	LIBXML_TEST_VERSION xmlDocPtr doc;	// initialize the library

	doc = xmlReadFile(fname, NULL, 0);
	if (doc == NULL)
		SysLog(LogFatal, "Failed to parse %s", fname);

	// Create xpath evaluation context
	xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);

	if (xpathCtx == NULL)
		SysLog(LogFatal, "Unable to create new XPath context");

	ParseNodeProperties(this, name, xpathCtx, NxNodePropertiesXpath); // Load the properties

	// Cleanup
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);

	// Shutdown libxml
	xmlCleanupParser();

	return 0;
}


static void
DisplayNodeProperties(Nx_t *this)
{

	// Display Nat Table
	for(HashEntry_t *e = NULL; (e = HashGetNextEntry(this->natList, e)) != NULL;)
	{
		Pair_t *nat = (Pair_t*)e->var;
		SysLogPrintf(LogAny, "Nat From=%s To=%s\n", nat->first, nat->second);
	}

	ObjectList_t* list = HashGetOrderedList(this->propList, ObjectListStringType);
	for ( HashEntry_t *entry = NULL; (entry = (HashEntry_t*)ObjectListRemove(list, ObjectListFirstPosition)); )
	{
		char *name = (char *)entry->string;
		String_t *value = (String_t*)entry->var;
		SysLogPrintf(LogAny, "%s=%s\n", name, value->str);
		HashEntryDelete(entry);
	}
	ObjectListDelete(list);
}


int
_NxSetProp(Nx_t *this, char *name, char *value, ...)
{

	NxVerify(this);

	if ( value == NULL )
		value = "";

	va_list ap;
	va_start(ap, value);
	StringNewStatic(tmp, 32);
	StringSprintfV(tmp, value, ap);
	HashUpdateString(this->propList, name, StringNewCpy(tmp->str));

	{		// now normalize the value and potentially set global values
		PropertyMap_t *pm;
		for (pm = this->stdPropertyMap; pm->name != NULL; ++pm)
		{
			if ( pm->dynamic && stricmp(name, pm->name) == 0 )
				break;		// this one
		}
		NormalizeProperty(this, pm);
	}

	return 0;
}


char*
_NxGetPropFull(Nx_t *this, boolean resolveref, char *name, ...)
{
	NxVerify(this);

	if ( this->propList == NULL )		// no list
		return NULL;					// no property

	va_list ap;
	va_start(ap, name);
	StringNewStatic(tmp, 32);
	StringSprintfV(tmp, name, ap);
	HashEntry_t *entry = HashFindString(this->propList, tmp->str);
	if ( entry == NULL )
		return NULL;

	if ( ! resolveref )
		return ((String_t*)(entry->var))->str; // not resolving references, return as is...


// this resolves references to other properties in the form:
// ${propname}...
// a particular danger, is the value of the returned property is a
// static string area which *will* be overwritten after 32 subsequent calls
// (see size of StringArrayStatic)...
	static int stack = 0;
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringClear(out);

	char *prop = strdup(((String_t*)(entry->var))->str);
	char *in = prop;
	char *repl;
	while ( stack < 10 && (repl = strstr(in, "${")) )
	{
		*repl++ = '\0';
		StringCat(out, in);		// add prefix
		char *tmp;
		if ( (tmp = strchr(++repl, '}')) == NULL )		// no terminator; use as is...
		{
			repl -= 2;
			*repl = '$';		// replace the '$' scrubbed above
			StringCat(out, repl);			// add remaining part
			break;			// done
		}
		*tmp++ = '\0';		// wack the terminating '}'...
		++stack;			// indicate recursion
		if ( (repl = _NxGetPropFull(this, resolveref, "%s", repl)) != NULL )
			StringCat(out, repl);
		--stack;
		in = tmp;			// next...
	}

	if ( strlen(in) > 0 )
		StringCat(out, in);

	free(prop);
	return out->str;
}


// Caller must delete the returned list
ObjectList_t*
_NxGetPropertyListMatching(Nx_t *this, char *name, ...)
{

	NxVerify(this);

	va_list ap;
	va_start(ap, name);
	StringNewStatic(tmp, 32);
	StringSprintfV(tmp, name, ap);

// build suffix
	char *suf = strdup(tmp->str);
	if ( strstr(suf, "_") != NULL )
		*(strstr(suf, "_")) = '\0';

	ObjectList_t* list = HashGetOrderedList(this->propList, ObjectListStringType);
	for ( ObjectLink_t *link = ObjectListFirst(list); link != NULL; )
	{
		HashEntry_t *entry = (HashEntry_t*)link->var;
		char *nm = (char *)entry->string;
		if ( striprefix(nm, tmp->str) != 0 &&	// full match
			striprefix(nm, suf) != 0 )		// partial match
		{	// not a match, dispose of this entry
			ObjectLink_t *next = ObjectListNext(link);
			ObjectListYank(list, link);
			link = next;
			HashEntryDelete(entry);
		}
		else
		{
			link = ObjectListNext(link);		// this is a match, check next...
		}
	}

	free(suf);
	return list;
}


char*
_NxGetPropertyValue(Nx_t *this, char *name, ...)
{
	va_list ap;
	va_start(ap, name);
	StringNewStatic(tmp, 32);
	StringSprintfV(tmp, name, ap);
	char *prop = NullToValue(_NxGetPropFull(this, false, "%s", tmp->str), "");
	return prop;
}
