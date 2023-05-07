/*****************************************************************************

Filename:   include/sysloglib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:54 $
 * $Header: /home/hbray/cvsroot/fec/include/sysloglib.h,v 1.3.4.6 2011/10/27 18:33:54 hbray Exp $
 *
 $Log: sysloglib.h,v $
 Revision 1.3.4.6  2011/10/27 18:33:54  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:38  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:43  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/01 16:11:28  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:37  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: sysloglib.h,v 1.3.4.6 2011/10/27 18:33:54 hbray Exp $ "


#ifndef _SYSLOGLIB_H
#define _SYSLOGLIB_H


// Public use

// LogLevels are hiearchical; that is LogDebug includes all the prior levels, LogWarn, etc...
typedef enum
{
	LogAny			= 0xFFFF,		// always (-1)
	LogLevelMin		= 0,			// just a reference point
	LogFatal		= 0,			// catastrophic failure...
	LogError		= 1,			// something bad; but, probably recoverable; or at least non-critical
	LogWarn			= 2,			// something of strong interest; could be an error
	LogDebug		= 3,			// miscellaneous detail useful for a developer to see
	LogLevelMax						// a reference point
} SysLogLevel;


// These are bitmapped levels, useful to turn on/off different component levels
// TODO: these are not, yet, properly used by the various components.
typedef enum
{
	SubLogNone		= (0),
	SubLogLevelMin	= (1 << 16),	// just a reference point
	SubLogDump		= (1 << 16),
	SubLogCore		= (2 << 16),
	SubLogStack		= (4 << 16),
	SubLogLevelMax,
	SubLogAll		= (0xFFFF0000)		// I use this to suppress recursive PiExceptions; see pisession.c and worker.c
} SysLogSubLevel;


#define MaxSysLogStackDepth 		16


typedef struct SysLog_t
{
// Must be first
	int				logLevelValue;

	boolean			ready;
	boolean			doublespace;
	boolean			jsonFormat;

	char			name[MaxNameLen];
	char			version[MaxNameLen];
	void			*logWriterContext;

	ObjectList_t	*logWriterList;			// a ObjectList_t* of SysLogWriter_t*; see sysloglib.c

	struct Timer_t	*recycleTimer;

	struct Stack_t	*loglevelStack;			// a stack of SysLogLevel
	struct Stack_t	*prefixStack;			// a stack of char*
} SysLog_t;



// External (public) Functions and Macros

#define SysLogGlobal NxGlobal->syslog

#define SysLogGetLevelFull() (SysLogGlobal->logLevelValue)
#define SysLogGetLevel() (SysLogGetLevelFull() & 0xFFFF)
#define SysLogIsLogLevel(level) ((((level)&0xFFFF) == LogAny) || (((level)&0xFFFF) <= SysLogGetLevel()))


// Internal Area; not for public use
extern char* _SysLogFull(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, ...);
extern char* _SysLogFullV(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, va_list ap);

#define SysLogRegisterWriter(logRecycle, writer, context) _SysLogRegisterWriter(SysLogGlobal, logRecycle, writer, context)
extern void _SysLogRegisterWriter(SysLog_t *this, void (*logRecycle) (SysLog_t*, void**, boolean force), void (*logWrite) (SysLog_t*, void**, SysLogLevel, char*, int, char*, char*, int), void *context);

#define SysLogUnRegisterWriter(logRecycle, writer, context) _SysLogUnRegisterWriter(SysLogGlobal, logRecycle, writer, context)
extern int _SysLogUnRegisterWriter(SysLog_t *this, void (*logRecycle) (SysLog_t*, void**, boolean force), void (*logWrite) (SysLog_t*, void**, SysLogLevel, char*, int, char*, char*, int), void *context);


// Checks the current loglevel; returns true/false

// if level is enabled, log a message
#define SysLog(level, fmt, ...) \
			SysLogFull(level, \
					__FILE__, \
					__LINE__, \
					__FUNC__, \
					fmt, ##__VA_ARGS__)

#define SysLogFull(level, file, lno, func, fmt, ...) \
			SysLogIsLogLevel(level)? _SysLogFull(SysLogGlobal, \
					level, \
					file, \
					lno, \
					func, \
					fmt, ##__VA_ARGS__):(void)0

#define SysLogV(level, fmt, ap) \
			SysLogIsLogLevel(level)? _SysLogFullV(SysLogGlobal, \
					level, \
					__FILE__, \
					__LINE__, \
					__FUNC__, \
					fmt, ap):(void)0


// Kinda like SysLog; but, does not display the log prefix (hdr)
#define SysLogPrintf(lvl, fmt, ...) _SysLogPrintf(SysLogGlobal, lvl, __FILE__, __LINE__, __FUNC__, fmt, ##__VA_ARGS__)
extern char *_SysLogPrintf(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, ...);
#define SysLogPrintfV(lvl, fmt, ap) _SysLogPrintfV(SysLogGlobal, lvl, __FILE__, __LINE__, __FUNC__, fmt, ap)
extern char *_SysLogPrintfV(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, va_list ap);

// Kinda like SysLogPrintf; but, does not format anything; displays string only...
#define SysLogPuts(lvl, str) _SysLogPuts(SysLogGlobal, lvl, __FILE__, __LINE__, __FUNC__, str)
extern char *_SysLogPuts(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *str);

// Kinda like SysLogPuts; but, will display a 'raw' buffer for 'len' characters.
#define SysLogWrite(lvl, bfr, len) _SysLogWrite(SysLogGlobal, lvl, __FILE__, __LINE__, __FUNC__, bfr, len)
extern char *_SysLogWrite(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *bfr, int len);

// Creates a log message; used internally; but, can be used to generate a log message for purposes other than logging.
#define SysLogFormatMessage(lvl, file, lno, fnc, fmt, ...)  _SysLogFormatMessage(SysLogGlobal, lvl, file, lno, fnc, fmt, ##__VA_ARGS__)
extern char *_SysLogFormatMessage(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, ...);
#define SysLogFormatMessageV(lvl, file, lno, fnc, fmt, ap) _SysLogFormatMessageV(SysLogGlobal, lvl, file, lno, fnc, fmt, ap)
extern char *_SysLogFormatMessageV(SysLog_t *this, SysLogLevel lvl, char *file, int lno, char *fnc, char *fmt, va_list ap);

// Gets/Sets log levels
// Getters
extern char *SysLogLevelToString(SysLogLevel level);
extern int SysLogLevelStringToValue(char *level);

// Setters
#define SysLogSetLevel(level) _SysLogSetLevel(SysLogGlobal, level)
extern int _SysLogSetLevel(SysLog_t *this, SysLogLevel level);

// Sets using the textual representation
#define SysLogSetLevelString(level) _SysLogSetLevelString(SysLogGlobal, level)
extern int _SysLogSetLevelString(SysLog_t *this, char *level);


#define SysLogPushLevel() _SysLogPushLevel(SysLogGlobal)
#define SysLogPopLevel() _SysLogPopLevel(SysLogGlobal)
#define SysLogPushPrefix(token) _SysLogPushPrefix(SysLogGlobal, token)
#define SysLogPopPrefix() _SysLogPopPrefix(SysLogGlobal)

extern void _SysLogPushLevel(SysLog_t *this);
extern SysLogLevel _SysLogPopLevel(SysLog_t *this);

extern void _SysLogPushPrefix(SysLog_t *this, char *token);
extern int _SysLogPopPrefix(SysLog_t *this);


#define SysLogRecycle(force) _SysLogRecycle(SysLogGlobal, force)
extern void _SysLogRecycle(SysLog_t *this, boolean force);


extern char* SysLogTimeToString();

extern SysLog_t* SysLogConstructor(SysLog_t *this, char *file, int lno, char *name, char *version);
extern void SysLogDestructor(SysLog_t *this, char *file, int lno);
extern struct Json_t* SysLogSerialize(SysLog_t *this);
extern char* SysLogToString(SysLog_t *this);


// External Functions

extern BtNode_t *SysLogNodeList;

#define SysLogNew(name, version) ObjectNew(SysLog, name, version)
#define SysLogVerify(var) ObjectVerify(SysLog, var)
#define SysLogDelete(var) ObjectDelete(SysLog, var)

#endif
