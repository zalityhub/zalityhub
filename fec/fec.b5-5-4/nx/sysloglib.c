/*****************************************************************************

Filename:   lib/nx/sysloglib.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:59 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/sysloglib.c,v 1.3.4.6 2011/10/27 18:33:59 hbray Exp $
 *
 $Log: sysloglib.c,v $
 Revision 1.3.4.6  2011/10/27 18:33:59  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:47  hbray
 Revision 5.5

 Revision 1.3.4.4  2011/09/02 14:17:03  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:47  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/01 16:11:29  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:48  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: sysloglib.c,v 1.3.4.6 2011/10/27 18:33:59 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/parseobj.h"
#include "include/logwriter.h"

#undef printf
#undef puts
#undef putchar



/********************************************************************
	Local/Static definitions
********************************************************************/

static String_t *GetLogBfr(SysLog_t *this);


// Returns the specified log level as text.
static char *AbbrevLevelString[] = { "FTL", "ERR", "WRN", "DBG", "ENT", NULL };
static inline char *
GetAbbrevLevelString(int lvl)
{
	lvl &= 0xffff;
	return (lvl < LogLevelMax && lvl >= 0) ? AbbrevLevelString[lvl] : lvl == LogAny ? "ANY" : "";
};


static EnumToStringMap SysLogLevelStringMap[] =
{
	{LogAny,		"LogAny"},
	{LogFatal,		"LogFatal"},
	{LogError,		"LogError"},
	{LogWarn,		"LogWarn"},
	{LogDebug,		"LogDebug"},
	{-1, NULL}
};

typedef struct SysLogWriter_t
{
	void		*context;
	void		(*logRecycle) (SysLog_t*, void **contextp, boolean force);
	void		(*logWrite) (SysLog_t*, void **contextp, SysLogLevel, char*, int, char*, char*, int);
} SysLogWriter_t ;


#define SysLogWriterNew() ObjectNew(SysLogWriter)
#define SysLogWriterVerify(var) ObjectVerify(SysLogWriter, var)
#define SysLogWriterDelete(var) ObjectDelete(SysLogWriter, var)

static SysLogWriter_t* SysLogWriterConstructor(SysLogWriter_t *this, char *file, int lno);
static void SysLogWriterDestructor(SysLogWriter_t *this, char *file, int lno);
static BtNode_t* SysLogWriterNodeList = NULL;
static Json_t* SysLogWriterSerialize(SysLogWriter_t *this);
static char* SysLogWriterToString(SysLogWriter_t *this, ...);


static SysLogWriter_t	DefaultWriter = {NULL};



BtNode_t *SysLogNodeList = NULL;


static void
SysLogHandleRecycleTimerEvent(Timer_t *this)
{
	SysLog_t *syslog = (SysLog_t*)this->context;
	SysLogVerify(syslog);
	_SysLogRecycle(syslog, false);
}


static void
SysLogActivateRecycleTimer(SysLog_t *this)
{
	SysLogVerify(this);
	this->recycleTimer->context = this;
	int recycleTimer = NxGetPropertyIntValue("SysLog.RecycleTimer");
	if ( recycleTimer <= 0 )
		recycleTimer = 60;		// min of 60 seconds
	if (TimerActivate(this->recycleTimer, recycleTimer*1000, SysLogHandleRecycleTimerEvent) != 0)
		SysLog(LogFatal, "TimerActivate failed");
}


SysLog_t*
SysLogConstructor(SysLog_t *this, char *file, int lno, char *name, char *version)
{

// Save settings

	SysLogGlobal = this;

	strncpy(this->name, name, sizeof(this->name) - 1);
	strncpy(this->version, version, sizeof(this->version) - 1);

// save any current file
	{
		char tmp[BUFSIZ];
		sprintf(tmp, "%s.log", this->name);
		if ( access(tmp, W_OK) == 0 )
		{
			if ( NxLogFileBackup(NxGlobal, this->name) != 0 )
				SysLog(LogError, "NxLogFileBackup failed");
		}
	}

// Then the level stack
	this->loglevelStack = StackNew((void*)-1, NULL);
// and token stack
	this->prefixStack = StackNew((void*)-1, NULL);

	if ((this->logWriterList = ObjectListNew(ObjectListVarType, "LogWriterList")) == NULL)
		NxCrash("ObjectListNew failed");

	memset(&DefaultWriter, 0, sizeof(DefaultWriter));

	this->ready = true;

	this->recycleTimer = TimerNew("SysLogRecycleTimer");
	SysLogActivateRecycleTimer(this);

	return this;
}


void
SysLogDestructor(SysLog_t *this, char *file, int lno)
{
	TimerDelete(this->recycleTimer);
	StackDelete(this->prefixStack);
	StackDelete(this->loglevelStack);
}


Json_t*
SysLogSerialize(SysLog_t *this)
{
	SysLogVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Details", ObjectStringStamp(this));
	return root;
}


char*
SysLogToString(SysLog_t *this)
{
	Json_t *root = SysLogSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


void
_SysLogRegisterWriter(SysLog_t *this, void (*logRecycle) (SysLog_t*, void**, boolean force), void (*logWrite) (SysLog_t*, void**, SysLogLevel, char*, int, char*, char*, int), void *context)
{

	SysLogWriter_t *writer = SysLogWriterNew();
	if ( writer == NULL )
		NxCrash("Unable to create SysLogWriter");
	writer->logRecycle = logRecycle;
	writer->logWrite = logWrite;
	writer->context = context;

	ObjectListAdd(this->logWriterList, writer, ObjectListLastPosition);
}


int
_SysLogUnRegisterWriter(SysLog_t *this, void (*logRecycle) (SysLog_t*, void**, boolean force), void (*logWrite) (SysLog_t*, void**, SysLogLevel, char*, int, char*, char*, int), void *context)
{
	int		removals = 0;
	for(;;)
	{
		ObjectLink_t   *link = NULL;
		SysLogWriter_t *writer = NULL;
		for(link = ObjectListFirst(this->logWriterList); link != NULL; link = ObjectListNext(link))
		{
			writer = (SysLogWriter_t*)link->var;
			if ( writer->logWrite == logWrite && writer->logRecycle == logRecycle )
				break;		// this one
		}

		if ( writer == NULL )
			break;		// done

		ObjectListYank(this->logWriterList, link);
		SysLogWriterDelete(writer);
		++removals;
		// then go another round...
	}

	return removals;
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

int
_SysLogSetLevel(SysLog_t *this, SysLogLevel newLevel)
{
	newLevel &= 0xFFFF;

	if (newLevel < LogWarn)
		newLevel = LogWarn;		// minimum level allowed

	this->logLevelValue &= 0xFFFF0000;	// remove current value
	this->logLevelValue |= newLevel;	// set new one
	return 0;
}


int
SysLogLevelStringToValue(char *level)
{
	int v = EnumMapStringToVal(SysLogLevelStringMap, level, "Log");
	return v;
}


int
_SysLogSetLevelString(SysLog_t *this, char *level)
{

	int v = SysLogLevelStringToValue(level);

	if (v == 0xdeadbeef)
	{
		SysLog(LogError, "%s is not a valid log level", level);
		return -1;
	}

	return _SysLogSetLevel(this, v);
}


char *
SysLogLevelToString(SysLogLevel lvl)
{
	lvl &= 0xFFFF;

	char *s = EnumMapValToString(SysLogLevelStringMap, lvl);

	if (s == NULL)
	{
		static char tmp[32];
		return IntegerToString(lvl, tmp);	// give 'em something to chew on
	}

	return s;
}


static String_t*
GetLogBfr(SysLog_t *this)
{
	StringArrayStatic(sa, MaxSysLogStackDepth, 32);
	String_t *output = StringArrayNext(sa);
	return output;
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

char*
_SysLogFullV(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, va_list ap)
{
	char *msg;

	msg = _SysLogFormatMessageV(this, lvl, file, lno, fnc, fmt, ap);

	_SysLogPuts(this, lvl, file, lno, fnc, msg);

	if ((lvl & 0xFFFF) == LogFatal)
		_NxCrash(NxGlobal, file, lno, fnc, "Terminating");

	return msg;
}


char*
_SysLogFull(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return _SysLogFullV(this, lvl, file, lno, fnc, fmt, ap);
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

char*
_SysLogPrintf(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return _SysLogPrintfV(this, lvl, file, lno, fnc, fmt, ap);
}


char*
_SysLogPrintfV(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, va_list ap)
{
	int sErrno = errno;
	String_t *logBfr = GetLogBfr(this);
	StringSprintfV(logBfr, fmt, ap);
	char *text = _SysLogPuts(this, lvl, file, lno, fnc, logBfr->str);
	errno = sErrno;
	return text;
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

char*
_SysLogPuts(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *str)
{
	return _SysLogWrite(this, lvl, file, lno, fnc, str, strlen(str));
}


char*
_SysLogWrite(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *bfr, int len)
{
	ConsoleWriter(this, &DefaultWriter.context, lvl, file, lno, fnc, bfr, len);

	static int recursionLevel = 0;
	if ( recursionLevel == 0 && this->logWriterList != NULL )
	{	// call each registered writer
		for(ObjectLink_t *link = ObjectListFirst(this->logWriterList); link != NULL; link = ObjectListNext(link))
		{
			SysLogWriter_t *writer = (SysLogWriter_t*)link->var;
			++recursionLevel;
			if ( writer->logWrite != NULL )
				(*writer->logWrite)(this, &writer->context, lvl, file, lno, fnc, bfr, len);
			--recursionLevel;
		}
	}

	if ( (lvl & SubLogStack) )
	{
		StringArray_t *st = GetStackTrace();
		for(int i = 0; i < st->next; ++i )
			ConsoleWriter(this, &DefaultWriter.context, lvl, file, lno, fnc, st->array[i]->str, st->array[i]->len);
	}

	if ( (lvl & SubLogCore) )
		MakeCoreFile();

	return bfr;
}


char*
SysLogTimeToString()
{
	StringArrayStatic(sa, 16, 32);
	char *output = StringArrayNext(sa)->str;
	NxTime_t ms = GetMsTime();
	time_t tod = (time_t) (ms/1000);
	struct tm *tm = localtime(&tod);
	sprintf(output, "%02d:%02d:%02d.%03d", tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(ms % 1000));
	return output;
}


char*
_SysLogFormatMessage(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return _SysLogFormatMessageV(this, lvl, file, lno, fnc, fmt, ap);
}


static char*
_SysLogFormatMessageVRaw(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, va_list ap)
{
	int sErrno = errno;

// format hdr

	String_t *logBfr = GetLogBfr(this);

	{
		static char *hdrfmt = "%s "	// date/time
								"%d "				// Pid
								"%s "				// LogLevel
								"%s "				// ProcName
								"%s:%d "			// FileName and LineNumber
								"%s: ";				// Function

		StringSprintf(logBfr, hdrfmt,
								SysLogTimeToString(),
								getpid(),
								GetAbbrevLevelString(lvl),
								NxCurrentProc->name,
								file,
								lno,
								fnc);
	}

	if ( this->prefixStack != NULL && StackLength(this->prefixStack) > 0 )
		StringSprintfCat(logBfr, "%s: ", (char*)StackTop(this->prefixStack));

	if ((lvl & SubLogDump))
	{ // Memory dump requested
		char *bfr = fmt;
		int dlen = va_arg(ap, int);
		fmt = va_arg(ap, char *);
		StringSprintfCatV(logBfr, fmt, ap);

		if (dlen > 8192)
			dlen = 8192;	// no more than 8k
		StringNewStatic(dbfr, 8192);
		StringSprintfCat(logBfr, "\n%s", StringDump(dbfr, bfr, dlen, 0)->str);
	}
	else
	{
		StringSprintfCatV(logBfr, fmt, ap);
	}
	if ( this->doublespace )
		StringCat(logBfr, "\n\n");		// new line...
	else
		StringCat(logBfr, "\n");		// new line...

	errno = sErrno;
	return logBfr->str;
}


static char*
_SysLogFormatMessageVJson(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, va_list ap)
{
	int sErrno = errno;

	Json_t *root = JsonNew(__FUNC__);

// format hdr

	JsonAddString(root, "Date", SysLogTimeToString());
	JsonAddNumber(root, "Pid", getpid());
	JsonAddString(root, "Level", GetAbbrevLevelString(lvl));
	JsonAddString(root, "Name", NxCurrentProc->name);
	JsonAddString(root, "File", "%s:%d", file, lno);
	JsonAddString(root, "Func", fnc);
	if ( this->prefixStack != NULL && StackLength(this->prefixStack) > 0 )
		JsonAddString(root, "Prefix", (char*)StackTop(this->prefixStack));

	if ((lvl & SubLogDump))
	{
	// Memory dump requested
		char *bfr = fmt;
		int dlen = va_arg(ap, int);
		fmt = va_arg(ap, char *);
		JsonAddStringV(root, "Text", fmt, ap);
		JsonDataOut(root, "Dump", bfr, dlen, DumpOutput);
	}
	else
	{
		JsonAddStringV(root, "Text", fmt, ap);
	}

	String_t *logBfr = GetLogBfr(this);
	StringSprintf(logBfr, "%s\n%s", JsonToString(root), this->doublespace?"\n":"");
	JsonDelete(root);
	errno = sErrno;
	return logBfr->str;
}


void
_SysLogPushLevel(SysLog_t *this)
{
	StackPush(this->loglevelStack, (void*)SysLogGetLevelFull());
}


SysLogLevel
_SysLogPopLevel(SysLog_t *this)
{
	if ( StackLength(SysLogGlobal->loglevelStack) > 0)
	{
		_SysLogSetLevel(this, (SysLogLevel)StackPop(SysLogGlobal->loglevelStack));
		return SysLogGetLevelFull();
	}
	return (SysLogLevel)-1;
}


void
_SysLogPushPrefix(SysLog_t *this, char *token)
{
	if ( this->prefixStack != NULL )
		StackPush(this->prefixStack, strdup(token));
}


int
_SysLogPopPrefix(SysLog_t *this)
{
	if ( this->prefixStack == NULL )
		return 0;

	if ( StackLength(SysLogGlobal->prefixStack) > 0)
	{
		free((char*)StackPop(SysLogGlobal->prefixStack));
		return 0;
	}
	return -1;
}



char*
_SysLogFormatMessageV(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, va_list ap)
{
	if ( this->jsonFormat )
		return _SysLogFormatMessageVJson(this, lvl, file, lno, fnc, fmt, ap);
	else
		return _SysLogFormatMessageVRaw(this, lvl, file, lno, fnc, fmt, ap);
}


static SysLogWriter_t*
SysLogWriterConstructor(SysLogWriter_t *this, char *file, int lno)
{
	return this;
}


static void
SysLogWriterDestructor(SysLogWriter_t *this, char *file, int lno)
{
}


static Json_t*
SysLogWriterSerialize(SysLogWriter_t *this)
{
	SysLogWriterVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Details", ObjectStringStamp(this));
	return root;
}


static char*
SysLogWriterToString(SysLogWriter_t *this, ...)
{
	Json_t *root = SysLogWriterSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


void
_SysLogRecycle(SysLog_t *this, boolean force)
{
	static int recursionLevel = 0;
	if ( recursionLevel == 0 )
	{	// call each registered recycler
		for(ObjectLink_t *link = ObjectListFirst(this->logWriterList); link != NULL; link = ObjectListNext(link))
		{
			SysLogWriter_t *writer = (SysLogWriter_t*)link->var;
			++recursionLevel;
			if ( writer->logRecycle != NULL )
				(*writer->logRecycle)(this, &writer->context, force);
			--recursionLevel;
		}
	}

	ConsoleRecycle(this, &DefaultWriter.context, force);
	SysLogActivateRecycleTimer(this);
}
