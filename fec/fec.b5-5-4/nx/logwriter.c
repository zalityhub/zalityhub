/*****************************************************************************

Filename:   lib/nx/logwriter.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:57 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/logwriter.c,v 1.3.4.4 2011/10/27 18:33:57 hbray Exp $
 *
 $Log: logwriter.c,v $
 Revision 1.3.4.4  2011/10/27 18:33:57  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/24 17:49:45  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/23 19:53:59  hbray
 eliminate fecplugin.h

 Revision 1.3.4.1  2011/08/01 16:11:29  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:46  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: logwriter.c,v 1.3.4.4 2011/10/27 18:33:57 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/logwriter.h"

#include <sys/file.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#undef printf
#undef puts
#undef putchar


typedef struct LogWriterContext_t
{
// Ocl Stuff
	time_t			oclConnectionExpireTime;
	int				oclFd;
	boolean			oclConnected;
	NxTime_t		oclThrottleTime;

	char			fname[BUFSIZ];
	boolean			usingStdout;
	struct stat		stat;
} LogWriterContext_t;


static int	OclMaxThrottleTime = 500;

static char* GetOclTime(void);
static void LogOclOpen(LogWriterContext_t *context);
static void LogOclClose(LogWriterContext_t *context);
static void LogRedirectStdio(int fd);
static void LogSpoolWrite(SysLog_t *this, LogWriterContext_t *context, char *bfr, int len);
static void LogSpoolClose(LogWriterContext_t *contet);
static void LogSpoolOpen(LogWriterContext_t *contet);
static int _LogWriteWait(LogWriterContext_t *context, int ms);



// The TCP/IP packet transmitted to the OCL Service
// NOTE: the following structure must be on byte bounds.
// NOTE: use this to structure to encode/decode HGCS TCP/IP headers
#pragma pack(1)
typedef struct OclMsg_t
{
	unsigned short	size;			//  1 NBO short value
	char			project[9];		//  2 "LIGHTNING"
	char			time[23];		//  3 "YYYY-MM-DD HH:MM:SS.sss"
	char			source[10];		//  4 "FEC", "STRATUS", "GBS", etc
	char			subcomp[25];	//  5 ???
	char			node[15];		//  6 default primary NIC IP address
	char			user[20];		//  7 spaces Appl generated msgs
	char			number[4];		//  8 Appl assigned error number
	char			action[10];		//  9 "FATAL", "ERROR", "WARNING", etc
	char			type[20];		// 10 "AUDIT", "CONNECT", "DISCONNECT"
	//    NOTE: The following can appear
	//    zero or more times in a single
	//    OCL log packet.
	char			tag[3];			// 11 "MSG" "Additional Detail" msg tag
	char			length[4];		// 11 4 ASCII digits: strlen(data)
	char			detail[255];	// 11 Variable message, tag/len/data
#define		OCLBASELEN	136			// (ocllog_t.type - ocllog_t.project)
#define		OCLMAXLEN	(OCLBASELEN+sizeof(tag)+sizeof(length)+sizeof(detail))	// Maximum OCL Log Service message size
	// OCLBASELEN + "MSG" + "0000" + Detail
} OclMsg_t;



//
// SysLog Writer
//

static char*
GetOclTime(void)
{
	StringArrayStatic(sa, 16, 32);
	char *text = StringArrayNext(sa)->str;
	struct timeval tv = { 0 };

	// Get the current time
	if (!gettimeofday(&tv, 0))
	{
		struct tm ts = { 0 };
		int millis;

		// Convert the microseconds to milliseconds
		millis = tv.tv_usec / 1000;

		// Convert the seconds for formatting
		memcpy(&ts, localtime(&tv.tv_sec), sizeof(ts));

		// Create the following time stamp: YYYY-MM-DD HH:MM:SS.sss
		strftime(text, 23, "%Y-%m-%d %T", &ts);

		// Add the millisconds to the time stamp
		sprintf(text + strlen(text), ".%03d", millis);

		// Clear our previous time stamp
	}
	else
	{
		*text = 0;
	}							// if (!gettimeofday(&tv,0))

	return (text);
}


static int
_LogWriteWait(LogWriterContext_t *context, int ms)
{

	// clear the fd sets
	fd_set exceptionFds;
	FD_ZERO(&exceptionFds);
	FD_SET(context->oclFd, &exceptionFds);

	fd_set writeFds;
	FD_ZERO(&writeFds);
	FD_SET(context->oclFd, &writeFds);

	struct timeval timeout;
	timeout.tv_sec = ms / 1000;
	timeout.tv_usec = 0;
	int n = select(context->oclFd + 1, NULL, &writeFds, &exceptionFds, ms == -1 ? NULL : &timeout);	// timeout or ready
	return n;
}


static void
LogRedirectStdio(int fd)
{
	if ( fd != 1 )
	{
		(void)close(1);
		if (dup(fd) != 1)
			fwritef(1, "\nUnable to dup stdout?\n");
	}
	if ( fd != 2 )
	{
		(void)close(2);
		if (dup(fd) != 2)
			fwritef(1, "\nUnable to dup stderr?\n");
	}

	if ( fd != 1 && fd != 2 )
		(void)close(fd);			// this has been redirected
}


static void
LogSpoolClose(LogWriterContext_t *context)
{
	(void)close(1);
	(void)close(2);
}


static void
LogSpoolOpen(LogWriterContext_t *context)
{

	LogSpoolClose(context);

	// open the file
	sprintf (context->fname, "%s.log", NxGlobal->name);
	int fd;
	if ((fd = open(context->fname, O_CREAT | O_WRONLY | O_APPEND, 0666)) < 0)
	{
		fwritef(1, "\nUnable to open %s\n", context->fname);
		return;
	}
	if (chmod(context->fname, 0666) < 0)
		SysLog(LogWarn, "chmod() error=%s: {%s}", ErrnoToString(errno), context->fname);

	LogRedirectStdio(fd);	// Redirect stdout, stderr

	char *msg = SysLogFormatMessage(LogAny, __FILE__, __LINE__, "LogSpoolOpen", "%s: VERSION=%s\n", TimeToStringLong(0, NULL), SysLogGlobal->version);
	write(1, msg, strlen(msg));

	fstat(fd, &context->stat);		// grab file stats
}


static void
LogSpoolWrite(SysLog_t *this, LogWriterContext_t *context, char *bfr, int len)
{
	if ( ! NxGlobal->dummy )
		ConsoleRecycle(this, (void**)&context, false);
	(void)write(1, bfr, len); // write it
}


static void
LogOclClose(LogWriterContext_t *context)
{
	(void)close(context->oclFd);
	context->oclFd = 0;
	context->oclConnected = false;
	context->oclConnectionExpireTime = 0;
}


static void
LogOclOpen(LogWriterContext_t *context)
{

	if ( ! NxGlobal->steadyState )		// not up to speed...
		return;					// don't open

	if ( NxGetPropertyIntValue("Platform/OclPort") <= 0 )
		return;					// disabled

	if ( context->oclFd <= 0 || (!context->oclConnected) )		// not connected to ocl
	{
		context->oclConnected = false;

		if ( context->oclFd <= 0 )
		{
			context->oclConnectionExpireTime = 0;

			if ( (context->oclFd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
				fwritef(1, "\nsocket failed: %s\n", ErrnoToString(errno));
	
				int flags = fcntl(context->oclFd, F_GETFL);
				flags |= (O_NDELAY | O_NONBLOCK);
				if (fcntl(context->oclFd, F_SETFL, flags) != 0)
				{
					fwritef(1, "fcntl() error: %s", ErrnoToString(errno));
					LogOclClose(context);
				}
		}

		if ( context->oclFd > 0 && strlen(NxGetPropertyValue("Platform/OclAddr")) > 0 )
		{
			if ( context->oclConnectionExpireTime == 0 )		// connect not in progress
			{
				struct sockaddr_in inetaddr;

				memset(&inetaddr, 0, sizeof(inetaddr));
				inetaddr.sin_family = AF_INET;
				inetaddr.sin_port = htons((u_short)(NxGetPropertyIntValue("Platform/OclPort")));
				inetaddr.sin_addr.s_addr = inet_addr(NxGetPropertyValue("Platform/OclAddr"));

				// Initiate the connect
				if ( connect(context->oclFd, (struct sockaddr*)&inetaddr, sizeof(inetaddr)) < 0 )
				{
					if ( errno != EAGAIN && errno != EINPROGRESS )
					{
						// fwritef(1, "\nconnect failed: %s\n", ErrnoToString(errno));
						LogOclClose(context);
					}
			
					time(&context->oclConnectionExpireTime);
					context->oclConnectionExpireTime += 2;			// allow two seconds
				}
				else
				{
					context->oclConnected = true;
					context->oclConnectionExpireTime = 0;
				}
			}

			if ( context->oclConnectionExpireTime != 0 )		// waiting for a connect
			{
				time_t tod;
				time(&tod);
				if ( context->oclConnectionExpireTime <= tod )		// timeout
				{
					LogOclClose(context);
				}
				else
				{
					if ( _LogWriteWait(context, 0) == 1 )		// writeable...
					{
						context->oclConnected = true;
						context->oclConnectionExpireTime = 0;
					}
				}
			}

			if ( ! context->oclConnected )
				LogOclClose(context);
		}
	}
}


// Insure we have a current spool file
void
ConsoleRecycle (SysLog_t *this, void **contextp, boolean force)
{
	LogWriterContext_t *context = (LogWriterContext_t*)*contextp;

	if ( ! force )
	{	// if file has changed, force the issue
		struct stat		st;
		if ( stat(context->fname, &st) == 0 )
		{
			if ( st.st_ino != context->stat.st_ino )
				force = true;
		}
	}

	if (context != NULL &&
		(context->usingStdout || force) )
	{
		if ( strlen(NxGlobal->name) > 0 )
		{
			LogSpoolOpen(context);		// closes and reopens
			context->usingStdout = false;
		}
	}
}


void
ConsoleWriter (SysLog_t *this, void **contextp, SysLogLevel lvl, char *file, int lno, char *fnc, char *bfr, int len)
{

	static int recursionLevel = 0;
	LogWriterContext_t *context = (LogWriterContext_t*)*contextp;

	if (recursionLevel > 0)		// recursion, just write to spool
	{
		(void)write(1, bfr, len);
		return;
	}

	++recursionLevel;

	if ( context == NULL )		// need a context
	{
		if ( (context = calloc(1, sizeof(LogWriterContext_t))) == NULL )
			_NxCrash(NxGlobal, __FILE__, __LINE__, __FUNC__, "calloc of LogWriterContext_t failed");
		*contextp = context;
		context->usingStdout = true;		// first time
	}

	if ( (! NxGlobal->dummy) && ((lvl & 0xFFFF) <= LogWarn) )	// send these to OCL
	{
		{	// throttle OCL logging
			if ( (OclMaxThrottleTime = NxGetPropertyIntValue("Platform/OclThrottleTime")) <= 0 )
				OclMaxThrottleTime = 500;		// keep a reasonable value
			if ( (GetMsTime() - context->oclThrottleTime) < OclMaxThrottleTime )
				return;					// don't open
			context->oclThrottleTime = GetMsTime();			// time of connection attempt
		}

		LogOclOpen(context);

		if (context->oclFd > 0 && context->oclConnected)	// connected
		{
			char tmp[BUFSIZ];
			OclMsg_t msg;

			memset(&msg, ' ', sizeof(msg));

			memcpy(msg.project, "LIGHTNING", 9);
			char *oclTime = GetOclTime();
			memcpy(msg.time, oclTime, min(sizeof(msg.time), strlen(oclTime)));
			memcpy(msg.source, "FEC", 3);
			char *name = "FEC";
			if ( NxCurrentProc != NULL )
				name = NxCurrentProc->name;
			memcpy(msg.subcomp, name, min(sizeof(msg.subcomp), strlen(name)));
			sprintf(tmp, "%04d", lno);
			memcpy(msg.number, tmp, min(sizeof(msg.number), strlen(tmp)));

			switch (lvl)
			{
			default:
				strcpy(tmp, "");
				break;

			case LogFatal:
				strcpy(tmp, "FATAL");
				break;

			case LogError:
				strcpy(tmp, "ERROR");
				break;

			case LogWarn:
				strcpy(tmp, "WARNING");
				break;
			}

			if (strlen(tmp) >= 0 )
			{
				memcpy(msg.action, tmp, min(sizeof(msg.action), strlen(tmp)));
				memcpy(msg.type, fnc, min(sizeof(msg.type), strlen(fnc)));
				memcpy(msg.tag, "MSG", 3);

				int len = min(sizeof(msg.detail), strlen(bfr));

				sprintf(tmp, "%04d", len);
				memcpy(msg.length, tmp, min(sizeof(msg.length), strlen(tmp)));
				memcpy(msg.detail, bfr, len);

				msg.size = sizeof(msg) - sizeof(msg.size);
				msg.size = htons(msg.size);

				char *msgptr = (char*)&msg;
				int msglen = sizeof(msg);
				for(int attempt = 0; msglen > 0 && attempt < 2; ++attempt)
				{
					int slen = send(context->oclFd, msgptr, msglen, 0);
					if ( slen == msglen )
						break;			// done
					
					if ( slen > 0 )
					{
						_LogWriteWait(context, 2*1000);		// wait 2 secs
						msgptr += slen;
						msglen -= slen;
					}
					else
					{
						fwritef(1, "\nsend failed: %s\n", ErrnoToString(errno));
						LogOclClose(context);
						break;
					}
				}
			}
		}
	}

	// and spool it
	LogSpoolWrite(this, context, bfr, len);

	--recursionLevel;
}
