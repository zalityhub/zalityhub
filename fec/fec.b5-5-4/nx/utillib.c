/*****************************************************************************

Filename:   lib/nx/utillib.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:48 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/utillib.c,v 1.3.4.4 2011/09/24 17:49:48 hbray Exp $
 *
 $Log: utillib.c,v $
 Revision 1.3.4.4  2011/09/24 17:49:48  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/08/23 19:54:00  hbray
 eliminate fecplugin.h

 Revision 1.3.4.2  2011/08/15 19:12:32  hbray
 5.5 revisions

 Revision 1.3.4.1  2011/08/11 19:47:34  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:49  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: utillib.c,v 1.3.4.4 2011/09/24 17:49:48 hbray Exp $ "


#include <libgen.h>

#include "include/stdapp.h"
#include "include/libnx.h"

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/ptrace.h>
#include <sys/wait.h>







static char *wdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };



// Local Functions

static void SigChldIgn(int sig, siginfo_t *si, void *obs) { }

static void _FormatLine(long offset, char *mem, char *obfr, int len);



void*
GetSymbolAddr(char *libname, char *symbol)
{

	// Open the current process memory
	void *handle;
	void *addr = NULL;
	if ( (handle = dlopen(libname, RTLD_NOW)) == NULL )
		SysLog(LogError, "Unable to get address of %s from %s; error %s", symbol, NullToValue(libname, "exec file"), dlerror());
	else if ( (addr = dlsym(handle, symbol)) == NULL )
		SysLog(LogError, "Unable to get address of %s from %s; error %s", symbol, NullToValue(libname, "exec file"), dlerror());
	if ( libname != NULL && handle != NULL )
		dlclose(handle);

	return addr;
}


// Returns a textual representation of error (errno)
char*
ErrnoToString(int error)
{
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s(%d)", strerror(error), error);
	return out->str;
}


// If string not found; returns 0xdeadbeef
int
EnumMapStringToVal(EnumToStringMap * map, char *text, char *prefix)
{

	if (map == NULL || text == NULL)
		return 0xdeadbeef;		// can't map

	EnumToStringMap *mp = NULL;
	for (mp = map; mp->t != NULL; ++mp)
	{
		if (stricmp(text, mp->t) == 0)
			break;
	}

	if (mp->t == NULL && prefix != NULL && strlen(prefix) > 0)
	{
		// Ok, that didn't work; Add prefix and try again
		char *p = alloca(strlen(text) + strlen(prefix) + 10);

		sprintf(p, "%s%s", prefix, text);
		for (mp = map; mp->t != NULL; ++mp)
		{
			if (stricmp(p, mp->t) == 0)
				break;
		}
	}

	if (mp->t == NULL)
		return 0xdeadbeef;

	return mp->e;
}


// If value not found; returns NULL
char *
EnumMapValToString(EnumToStringMap * map, int val)
{

	EnumToStringMap *mp = NULL;
	for (mp = map; mp->t != NULL; ++mp)
	{
		if (mp->e == val)
			break;
	}

	return mp->t;
}


int
MkDirPath(char *path)
{
	char dir[512];
	char *cp;
	char *ep;

	strcpy(dir, path);

	cp = dir;
	while ((ep = strchr(cp, '/')) != NULL)
	{
		*ep = '\0';
		if ( strlen(dir) > 0 )
		{
			(void)mkdir(dir, 0777);
			(void)chmod(dir, 0777);
		}
		*ep = '/';
		cp = ep + 1;
	}

	if (mkdir(dir, 0777) < 0 && (errno != EEXIST && errno != EACCES))
	{
		SysLog(LogError, "mkdir failed; errno=%s", ErrnoToString(errno));
		return -1;
	}
	if (chmod(dir, 0777) < 0)
		SysLog(LogWarn, "chmod() error=%s: {%s}", ErrnoToString(errno), dir);

	return 0;
}


int
RmDirPath(char *path)
{

	if ( WalkDirPath(path, RmDirNode) != 0 )
		SysLog(LogError, "Unable to WalkDirPath %s; error %s", path, ErrnoToString(errno));

	if ( rmdir(path) != 0 )
	{
		SysLog(LogError, "Unable to delete %s; error %s", path, ErrnoToString(errno));
		return -1;
	}

	return 0;
}


int
RmDirNode(char *path, struct stat *st)
{

	if ( st->st_mode & S_IFREG )		// is it a regular file?
	{
		if ( unlink(path) != 0 )
			perror("unlink");
	}
	else if ( st->st_mode & S_IFDIR )		// directory?
	{
		WalkDirPath(path, RmDirNode);
		if ( rmdir(path) != 0 )
			perror("rmdir");
	}
	else if ( st->st_mode & S_IFCHR )		// character device?
	{
		if ( unlink(path) != 0 )
			perror("unlink");
	}
	else if ( st->st_mode & S_IFBLK )		// block device?
	{
		if ( unlink(path) != 0 )
			perror("unlink");
	}
	else if ( st->st_mode & S_IFIFO )		// FIFO (named pipe)?
	{
		if ( unlink(path) != 0 )
			perror("unlink");
	}
	else if ( st->st_mode & S_IFLNK )		// symbolic link? (Not in POSIX.1-1996.)
	{
		if ( unlink(path) != 0 )
			perror("unlink");
	}
	else if ( st->st_mode & S_IFSOCK )		// socket? (Not in POSIX.1-1996.)
	{
		if ( unlink(path) != 0 )
			perror("unlink");
	}

	return 0;
}


int
WalkDirPath(char *dirPath, WalkDirCallBack_t callback)
{

	int ret = 0;

	if ( callback == NULL )
		return ret;		// nothing to do

	DIR *dir = opendir(dirPath);
	if (NULL != dir)
	{
		struct dirent *entry;

		for (; NULL != (entry = readdir(dir));)
		{
			// Skip dots
			if ('.' == entry->d_name[0] && ('\0' == entry->d_name[1] || ('.' == entry->d_name[1] && '\0' == entry->d_name[2])))
				continue;		// do nothing

			char *target = malloc(strlen(dirPath) + strlen(entry->d_name)+10);
			sprintf(target, "%s/%s", dirPath, entry->d_name);

			struct stat st;
			if (0 == lstat(target, &st))
			{
				if ( (*callback) (target, &st) != 0 )
					break;			// returned an error; skip out...
			}
			else
			{
				SysLog(LogError, "lstat of %s failed; error %s", target, ErrnoToString(errno));
				ret = -1;
			}
			free(target);
		}

		closedir(dir);
		dir = NULL;		// done
	}
	else
	{
		SysLog(LogError, "opendir of %s failed; error %s", dirPath, ErrnoToString(errno));
		ret = -1;
	}

	return ret;
}


int
OpenFilePath(const char *pathname, int flags, mode_t mode)
{
	char *dir = dirname(strdup((char*)pathname));
	if ( MkDirPath(dir) != 0 )
		NxCrash("Unable to create %s", dir);
	free(dir);
	return open(pathname, flags, mode);
}


char *
FTrim(char *text, char *trimchr)
{
	return RTrim((LTrim(text, trimchr)), trimchr);
}


char *
LTrim(char *text, char *trimchr)
{

	for (int ii = strspn(text, trimchr); ii > 0; ii = strspn(text, trimchr))
		strcpy(text, &text[ii]);
	return text;
}


char *
RTrim(char *text, char *trimchr)
{

	for (char *eol = &text[strlen(text) - 1]; eol >= text;)
	{
		if (strchr(trimchr, *eol) != NULL)
			*eol-- = '\0';
		else
			break;				// done
	}

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

char *
stristr(const char *s0, const char *s1)
{
	char *str[2];
	const char *cp;
	char *ptr;



	{
		if ((str[0] = (char *)alloca(strlen(s0) + 1)) == NULL)
			return NULL;
		if ((str[1] = (char *)alloca(strlen(s1) + 1)) == NULL)
			return NULL;
	}

	for (ptr = str[0], cp = s0; *cp;)
	{
		*ptr++ = tolower(*cp);
		++cp;
	}
	*ptr = '\0';

	for (ptr = str[1], cp = s1; *cp;)
	{
		*ptr++ = tolower(*cp);
		++cp;
	}
	*ptr = '\0';

	if ((ptr = strstr(str[0], str[1])) != NULL)
		ptr = (char *)&s0[(ptr - str[0])];

	return ptr;
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
strprefix(const char *stg, const char *prefix)
{
	int slen;
	int plen;

	plen = strlen(prefix);
	slen = strlen(stg);

	if (slen < plen)
		return 1;				// mismatch

	return strncmp(stg, prefix, plen);
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
striprefix(const char *stg, const char *pattern)
{
	int slen;
	int plen;

	plen = strlen(pattern);
	slen = strlen(stg);

	if (slen < plen)
		return 1;				// no match

	return strnicmp(stg, pattern, plen);
}


// Make sure the input string has enough 'room' to handle all replacements.
// This function does not check for output overflow
char*
strreplace(char *inp, char *from, char *to, int caseless, int once)
{
	for (char *iptr;;)
	{
		if (caseless)
			iptr = stristr(inp, from);
		else
			iptr = strstr(inp, from);
		if (iptr == NULL)
			break;				// done

		char *tmp = (char *)calloc(1, strlen(inp) + strlen(to) + 128);

		*iptr = '\0';			// cut it
		iptr += strlen(from);	// point to suffix

		char *out = tmp;

		out += sprintf(out, "%s", inp);	// non matching prefix
		out += sprintf(out, "%s", to);	// the replacement
		out += sprintf(out, "%s", iptr);	// the suffix
		strcpy(inp, tmp);
		free(tmp);

		if (once)
			break;				// only one time (not global replace)...
	}

	return inp;
}


// return stack trace
StringArray_t*
GetStackTrace()
{
	static StringArray_t *lines = NULL;

	if ( lines == NULL )
		lines = StringArrayNew(1024, 32);		// 1024 deep
	lines->next = 0;							// start at the top

    char exename[512];
    exename[readlink("/proc/self/exe", exename, 511)]=0;

	char cmd[1024];
	sprintf(cmd, "gdb --batch -n -ex thread -ex bt %s %d 2>&1", exename, getpid());
	struct sigaction oldsa = NxSetSigAction(SIGCHLD, SigChldIgn);		// Ignore Child stopped or terminated
	FILE *f = popen(cmd, "r");
	if ( f != NULL )
	{
		char line[1024];
		int hitme = 0;
		while ( fgets(line, sizeof(line), f) != NULL )
		{
			(void)FTrim(line," \t\r\n\f");
			if ( hitme )
				StringCpy(StringArrayNext(lines), line);
			if (strstr(line, __FUNC__))
				++hitme;
		}
		pclose(f);
	}
	else
	{
		int e = errno;
		StringSprintf(StringArrayNext(lines), "Unable to %s", cmd);
		StringSprintf(StringArrayNext(lines), "Error %s", ErrnoToString(e));
	}

	NxSetSigAction(SIGCHLD, oldsa.sa_sigaction);		// Restore Child stopped or terminated

	return lines;
}


// Create a core file
int
MakeCoreFile()
{
	int		pid;

	switch ((pid = fork()))
	{
		case -1:
			SysLog(LogFatal, "fork failed: %s", ErrnoToString(errno));
			break;

		default:		// Parent
			return 0;
			break;

		case 0:			// Child
			break;
	}

	// make this a daemon
	{
		// new file mode mask
		umask(0);

		// create a new sid for this child
		(void)setsid();

		// Don't close the standard out, err; they are attached by syslog
		close(STDIN_FILENO);
	}

	kill(getpid(), SIGABRT);

	for(;;)
		exit(1);
}


// If we are attached by a debugger, return true
// else false
boolean
DebugConnected()
{
	int		pid;
	boolean	ret = false;

	struct sigaction oldsa = NxSetSigAction(SIGCHLD, SigChldIgn);		// Ignore Child stopped or terminated

	switch ((pid = fork()))
	{
		case -1:
			SysLog(LogFatal, "fork failed: %s", ErrnoToString(errno));
			break;

		default:		// Parent
		{
			siginfo_t infop;
			if ( waitid(P_PID, pid, &infop, WEXITED) < 0 )
			{
				if ( errno != EINTR )
					SysLog(LogError, "waitid for %d failed: %s", pid, ErrnoToString(errno));
				infop.si_status = 0;		// not debugging
			}
			else
			{
				// SysLog(LogDebug, "pid=%d, uid=%d, signo=%d, status=%d, code=%d", infop.si_pid, infop.si_uid, infop.si_signo, infop.si_status, infop.si_code);
			}
			ret = infop.si_status != 0;
			NxSetSigAction(SIGCHLD, oldsa.sa_sigaction);		// Restore Child stopped or terminated
			break;
		}

		case 0:			// Child
		{
			int ppid = getppid();
			int res = 0;

			// Try to trace the parent...
			if (ptrace(PTRACE_ATTACH, ppid, NULL, NULL) == 0)
			{
				// that worked; tell it to continue
				waitpid(ppid, NULL, 0);
				ptrace(PTRACE_CONT, NULL, NULL);

				// Detach parent
				ptrace(PTRACE_DETACH, getppid(), NULL, NULL);

				// We were the tracers, so gdb is not present
				res = 0;
			}
			else
			{
				res = 1;		// the ptrace failed; gdb must have me attached...
			}
			exit(res);			// tell parent what's going on...
			break;
		}
	}

	return ret;
}


/*+******************************************************************
	Name:
		RequestDebug - 

	Synopsis:
		void RequestDebug (int ms)

	Description:
		Logs a message requesting a debugger every 'ms' (millisecond)

        a developer then should do a:
        gdb  <exec>  -p <pid>
        where:
            <exec>  is the name of the executable file
            <pid>    is the process id of the requester.

        then set the stackframe to this function and set the go variable to true
        set go=1

	Diagnostics:
		None.
-*******************************************************************/

void
RequestDebug(int ms)
{

	if (ms < 1000)
		ms = 1000;				// min time

	while (!DebugConnected())
	{
		SysLog(LogAny, "Hello. I need a debugger; my pid is %d", getpid());
		sleep(ms / 1000);
	}
}


/*+******************************************************************
	Name:
		TimeToStringLong - Convert time value into a displayable string

	Synopsis:
		char *TimeToStringLong (tod, output)
		time_t	tod;
		char	*output;

	Description:
		Converts the given time into a displayable string in the format:
			Mon Jan 01, 1990 17:45:17

		If NULL is passed as the 'output' argument, an internal static
		data area will be used to hold the formatted output.

		If 0 is passed as the 'tod' argument, the current time of day
		will be used.

		Returns a pointer to the output string.

	Diagnostics:
		None.
-*******************************************************************/

char *
TimeToStringLong(time_t tod, char *output)
{
	StringArrayStatic(sa, 16, 32);

	if (output == NULL)
		output = StringArrayNext(sa)->str;

	if (tod == 0)				/* no time, get one */
		tod = time(NULL);

	struct tm *tm = localtime(&tod);
	sprintf(output, "%s %s %02d %04d %02d:%02d:%02d", wdays[tm->tm_wday], months[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return output;
}


/*+******************************************************************
	Name:
		TimeToStringShort - Convert time value into a displayable string

	Synopsis:
		char *TimeToStringShort (tod, output)
		time_t	tod;
		char	*output;

	Description:
		Converts the given time into a displayable string in the format:
			01/01/90 17:45:17

		If NULL is passed as the 'output' argument, an internal static
		data area will be used to hold the formatted output.

		If 0 is passed as the 'tod' argument, the current time of day
		will be used.

		Returns a pointer to the output string.

	Diagnostics:
		None.
-*******************************************************************/

char *
TimeToStringShort(time_t tod, char *output)
{
	StringArrayStatic(sa, 16, 32);

	if (output == NULL)
		output = StringArrayNext(sa)->str;

	if (tod == 0)				/* no time, get one */
		tod = time(NULL);

	struct tm *tm = localtime(&tod);
	sprintf(output, "%02d/%02d/%02d %02d:%02d:%02d", tm->tm_mon + 1, tm->tm_mday, tm->tm_year - 100, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return output;
}


char*
MsTimeToStringShort(NxTime_t ms, char *output)
{
	if (ms == 0)				/* no time, get one */
		ms = GetMsTime();

	char *out = TimeToStringShort((time_t) (ms / 1000), output);
	char *tmp = out + strlen(out);

	sprintf(tmp, ".%03d", (int)(ms % 1000));
	return out;
}


char*
UsTimeToStringShort(NxTime_t us, char *output)
{
	if (us == 0)				/* no time, get one */
		us = GetMsTime();

	char *out = TimeToStringShort((time_t) (us / 1000000), output);
	char *tmp = out + strlen(out);

	sprintf(tmp, ".%06d", (int)(us % 1000000));
	return out;
}


/*+******************************************************************
	Name:
		TimeToStringHHMM - Convert a time value into hours and minutes

	Synopsis:
		char *TimeToStringHHMM (tod, output)
		time_t	tod;
		char	*output;

	Description:
		Converts the given time into hours and minutes.
			7:30pm will become:
				1930

			and 7:30am will become:
				0730

		If NULL is passed as the 'output' argument, an internal static
		data area will be used to hold the formatted output.

		If 0 is passed as the 'tod' argument, the current time of day
		will be used.

		Returns a pointer to the output string.

	Diagnostics:
		None.
-*******************************************************************/

char *
TimeToStringHHMM(time_t tod, char *output)
{
	StringArrayStatic(sa, 16, 32);

	if (output == NULL)
		output = StringArrayNext(sa)->str;

	if (tod == 0)				/* no time, get one */
		tod = time(NULL);

	struct tm *tm = localtime(&tod);
	sprintf(output, "%02d%02d", tm->tm_hour, tm->tm_min);
	return output;
}


/*+******************************************************************
	Name:
		TimeToStringHHMMSS - Convert a time value into hours, minutes and seconds

	Synopsis:
		char *TimeToStringHHMMSS (tod, output)
		time_t	tod;
		char	*output;

	Description:
		Converts the given time into hours and minutes.
			45 seconds past 7:30pm will become:
				193045

			and 45 seconds past 7:30am will become:
				073045

		If NULL is passed as the 'output' argument, an internal static
		data area will be used to hold the formatted output.

		If 0 is passed as the 'tod' argument, the current time of day
		will be used.

		Returns a pointer to the output string.

	Diagnostics:
		None.
-*******************************************************************/

char *
TimeToStringHHMMSS(time_t tod, char *output)
{
	StringArrayStatic(sa, 16, 32);

	if (output == NULL)
		output = StringArrayNext(sa)->str;

	if (tod == 0)				/* no time, get one */
		tod = time(NULL);

	struct tm *tm = localtime(&tod);
	sprintf(output, "%02d%02d%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	return output;
}


char*
MsTimeToStringHHMMSS(NxTime_t ms, char *output)
{
	StringArrayStatic(sa, 16, 32);

	if (output == NULL)
		output = StringArrayNext(sa)->str;

	if (ms == 0)				/* no time, get one */
		ms = GetMsTime();

	time_t tod = (time_t) (ms/1000);
	struct tm *tm = localtime(&tod);
	sprintf(output, "%02d%02d%02d.%03d", tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(ms % 1000));
	return output;
}


/*+******************************************************************
	Name:
		TimeToStringYYMMDD - Convert a time value into year/month/day

	Synopsis:
		char *TimeToStringYYMMDD (tod, output)
		time_t	tod;
		char	*output;

	Description:
		Converts the given time into year/month/day.
		Feb 3, 1991 will become:
			910203

		If NULL is passed as the 'output' argument, an internal static
		data area will be used to hold the formatted output.

		If 0 is passed as the 'tod' argument, the current time of day
		will be used.

		Returns a pointer to the output string.

	Diagnostics:
		None.
-*******************************************************************/

char *
TimeToStringYYMMDD(time_t tod, char *output)
{
	StringArrayStatic(sa, 16, 32);

	if (output == NULL)
		output = StringArrayNext(sa)->str;

	if (tod == 0)				/* no time, get one */
		tod = time(NULL);

	struct tm *tm = localtime(&tod);
	sprintf(output, "%02d%02d%02d", tm->tm_year - 100, tm->tm_mon + 1, tm->tm_mday);
	return output;
}


/*+******************************************************************
	Name:
		GetDeltaTime - Return the difference between now and another time

	Synopsis:
		time_t	GetDeltaTime (startTime)
		time_t	startTime;

	Description:
		This function will return the Absolute difference between the
		current time and a given previous or future time.
		The time values are given in units of one second.

		Returns Absolute (currentTime - startTime)

	Diagnostics:
		None.
-*******************************************************************/

time_t
GetDeltaTime(time_t startTime)
{
	time_t t;


	t = time(NULL);
	if (startTime > t)
		return startTime - t;
	else
		return t - startTime;
}


/*+******************************************************************
	Name:
		GetTimeZone - Get the local time zone name

	Synopsis:
		char *GetTimeZone ()

	Description:
		This function will return the name of the local time zone
		as a null terminated 3 character string.  The returned pointer
		points to a local (static) data area.

		Returns pointer to null terminated 3 character time zone string.

	Diagnostics:
		None.
-*******************************************************************/
char *
GetTimeZone()
{
	extern char *tzname[2];

	return tzname[0];
}


/*+******************************************************************
	Name:
		GetSecTime - Get the time in seconds.

	Synopsis:
		unsigned long GetSecTime ()

	Description:

	Diagnostics:
		None.
-*******************************************************************/
unsigned long
GetSecTime()
{
	time_t tod;

	time(&tod);
	return tod;
}


/*+******************************************************************
	Name:
		GetMsTime - Get the time in mseconds.

	Synopsis:
		unsigned long GetMsTime ()

	Description:

	Diagnostics:
		None.
-*******************************************************************/
NxTime_t
GetMsTime()
{
	struct timeval tv;
	NxTime_t t;

	gettimeofday(&tv, NULL);
	t = ((NxTime_t) tv.tv_usec) / 1000;
	t += (((NxTime_t) tv.tv_sec) * 1000);
	return t;
}


/*+******************************************************************
	Name:
		GetUsTime - Get the time in useconds.

	Synopsis:
		NxTime_t GetUsTime ()

	Description:

	Diagnostics:
		None.
-*******************************************************************/
NxTime_t
GetUsTime()
{
	struct timeval tv;
	NxTime_t t;

	gettimeofday(&tv, NULL);
	t = ((NxTime_t) tv.tv_usec);
	t += (((NxTime_t) tv.tv_sec) * 1000000L);
	return t;
}


/*+******************************************************************
	Name:
		Downshift - Convert all characters of a string to lowercase

	Synopsis:
		char *Downshift (char *string)

	Description:
		Converts each character of the given string to its lowercase
		equivalent.  Returns a pointer to the given string.

	Diagnostics:
		None.
-*******************************************************************/
char *
Downshift(char *string)
{
	for (char *s = string; *s; ++s)
		*s = tolower(*s);
	return string;
}


/*+******************************************************************
	Name:
		Upshift - Convert all characters of a string to uppercase

	Synopsis:
		char *Upshift (char *string)

	Description:
		Converts each character of the given string to its uppercase
		equivalent.  Returns a pointer to the given string.

	Diagnostics:
		None.
-*******************************************************************/
char *
Upshift(char *string)
{
	for (char *s = string; *s; ++s)
		*s = toupper(*s);
	return string;
}


char *
GetMnemonicString(unsigned char *text, int slen)	// pass slen=0 to use strlen of text
{
	StringArrayStatic(sa, 16, 32);
	String_t *output = StringArrayNext(sa);

	if (slen <= 0)
		slen = strlen((char *)text);

	for (StringClear(output); slen > 0; ++text, --slen)
		StringCat(output, GetMnemonicCh(*text, NULL));

	return output->str;
}


char*
GetMnemonicCh(unsigned char ch, char *mnem)
{
	static char *specialChs[32] =
	{
		"Nul", "Soh", "Stx", "Etx", "Eot", "Enq", "Ack", "Bel", "Bs",
		"Ht", "Lf", "Vt", "Ff", "Cr", "So", "Si", "Dle", "Dc1",
		"Dc2", "Dc3", "Dc4", "Nak", "Syn", "Etb", "Can", "Em", "Sub",
		"Esc", "Fs", "Gs", "Rs", "Us"
	};
	StringArrayStatic(sa, 16, 4);

	if (mnem == (char *)0)
		mnem = StringArrayNext(sa)->str;

	ch &= 0x7F;					/* strip top bit */

	if (ch < 32)				/* a non printing ch */
		strcpy(mnem, specialChs[ch]);
	else if (ch == 127)
		strcpy(mnem, "del");	/* mnemonic for delete ch */

	else
	{
		mnem[0] = (char)ch;
		mnem[1] = '\0';
	}

	return mnem;
}


#define BYTES_PER_LINE				16

int
HexDump(int (*oFnc) (char *, void *),	/* user function that wants the formatted lines */
		void *oFncArg,			/* the argument to pass to the user function */
		void *mem,				/* data to dump */
		int len,				/* number of bytes */
		long offset				/* starting offset used in output */
	)
{
	int status;
	int nl;
	int dupLine;
	char fLine[128];
	char *ptr = (char *)mem;


	if (len <= 0)
		return 0;				/* nothing to dump */

	status = nl = dupLine = 0;

	do
	{
		if (nl)
		{
			if (memcmp(&ptr[-BYTES_PER_LINE], &ptr[0], BYTES_PER_LINE) == 0)
			{
				++dupLine;
				goto nextLine;
			}
		}

/* Current line is different from previous line */

		if (dupLine)			/* detected one or more dup lines */
		{
			if (--dupLine)		/* More than one */
			{
				sprintf(fLine, "          %u duplicate lines.", dupLine);
				if ((status = (*oFnc) (fLine, oFncArg)) < 0)
					break;		/* reported error */
			}
/* force printing of the duplicate line */
			offset -= BYTES_PER_LINE;
			ptr -= BYTES_PER_LINE;
			len += BYTES_PER_LINE;
			--nl;
			dupLine = 0;
		}

		_FormatLine(offset, ptr, fLine, len);
		if ((status = (*oFnc) (fLine, oFncArg)) < 0)
			break;				/* reported error */

	  nextLine:
		offset += BYTES_PER_LINE;
		ptr += BYTES_PER_LINE;
		len -= BYTES_PER_LINE;
		++nl;					/* count the line */
	} while (len > 0);

/* Finished dumping the data */

	if (status == 0 && dupLine)
	{
		if (--dupLine)
		{
			sprintf(fLine, "          %u duplicate lines.", dupLine);
			if ((status = (*oFnc) (fLine, oFncArg)) < 0)
				return status;
		}
/* force printing of the duplicate line */
		offset -= BYTES_PER_LINE;
		ptr -= BYTES_PER_LINE;
		len += BYTES_PER_LINE;
		--nl;
		dupLine = 0;

		_FormatLine(offset, ptr, fLine, len);
		if ((status = (*oFnc) (fLine, oFncArg)) < 0)
			return status;
	}

	return status;
}								/* of hex memory dump */


static void
_FormatLine(long offset, char *mem, char *obfr, int len)
{
	static char *asciiDigit = { "0123456789ABCDEF" };
	char *op;
	char *ip;
	char ch;
	int nb;
	int value;

/*
						op = &obfr[sprintf (obfr, "%6lX: ", offset)];	*//* offset */

	sprintf(obfr, "%6lX: ", offset);
	op = &obfr[6];

/* format hex part of line */

	ip = &mem[0];

	for (nb = 0; nb < len && nb < BYTES_PER_LINE; ++nb)
	{
		if ((nb % 8) == 0 && nb != 0)
		{
			*op++ = ' ';
			*op++ = ':';		/* put a : every 8 bytes */
		}
		*op++ = ' ';			/* separator */
		value = *ip++ & 0xFF;
		*op++ = asciiDigit[(value >> 4) & 0x0F];
		*op++ = asciiDigit[value & 0x0F];
	}

/* format ascii part of line */

	*op = '\0';
	for (nb = strlen(obfr); nb < 60; ++nb)
		*op++ = ' ';			/* move to 60th column */

	ip = &mem[0];

	for (nb = 0; nb < BYTES_PER_LINE && nb < len; ++nb)
	{
		ch = (*ip++) & 0x7F;	/* lower 7 bits */
		if (!isprint(ch))
			ch = '.';			/* not printable */
		*op++ = ch;
	}

	*op = '\0';					/* terminate the line */
}


/*+
    Name:
    IntFromString(2) - Convert an ascii number to binary

    Synopsis:
    int IntFromString (char *num);

    Description:
    Convert ascii string into a binary int.  Assumes the value is
    in decimal unless prefixed with a 0x which then indicates a
    hex value.  If the value has a leading zero, octal will not be
    assumed.  The ascii value may have a leading sign character
    to express a positive or negative value.  Any leading spaces
    are ignored.

    10      - is a decimal ten
    -10     - is a negative decimal ten
    0x10    - is a hex 10 or decimal 16
    -0x10   - is a negative hex 10
    010     - is a decimal 10
    -010    - is an negative octal 10

    Returns the converted binary value or 0xdeadbeef
-*/

int
IntFromString(const char *num, boolean *err)
{
	boolean errBoolean;

	if ( err == NULL )
		err = &errBoolean;
	*err = false;

	num = &num[strspn(num, " \t")];	/* skip leading whitespace */

	/* check for leading sign character */

	char sign;
	if (*num == '-' || *num == '+')
		sign = *num++;
	else
		sign = '+';

	num = &num[strspn(num, " \t")];	/* skip leading whitespace */

	if (!isdigit(*num))
	{
		*err = true;
		return (0xdeadbeef);	/* not a number */
	}

	/* check for leading radix indicator */

	int radix;
	if (*num == '0' && tolower(num[1]) == 'x')
	{
		radix = 16;
		num += 2;
	}
	else
	{
		radix = 10;
	}

	int result = 0;
	for (int digit; isxdigit(*num); )
	{
		digit = toupper(*num++);
		digit -= '0';
		if (digit > 9)			/* hex range */
		{
			if (radix != 16)	/* not hex radix */
				break;
			digit -= (('A' - '9') - 1);
		}
		else if (digit > 7 && radix == 8)
		{
			break;				/* non octal digit */
		}

		{
			double test = result;

			test *= radix;
			test += digit;

			if (test > UINT_MAX)
			{
				*err = true;
				return (0xdeadbeef);	/* not a number */
			}
		}

		result = (result * radix) + digit;
	}

	if (sign == '-')			/* negative value */
		result = 0 - result;

	return (result);
}


/*+
    Name:
    Int64FromString(2) - Convert an ascii number to binary

    Synopsis:
    UINT64 Int64FromString (char *num);

    Description:
    Convert ascii string into a binary UINT64.  Assumes the value is
    in decimal unless prefixed with a 0x which then indicates a
    hex value.  If the value has a leading zero, octal will not be
    assumed.  
    Any leading spaces are ignored.

    10      - is a decimal ten
    0x10    - is a hex 10 or decimal 16
    010     - is a decimal 10

    Returns the converted binary value or an error
-*/

UINT64
Int64FromString(const char *num, boolean *err)
{
	boolean errBoolean;

	if ( err == NULL )
		err = &errBoolean;
	*err = false;

	num = &num[strspn(num, " \t")];	/* skip leading whitespace */

	num = &num[strspn(num, " \t")];	/* skip leading whitespace */

	if (!isdigit(*num))
	{
		*err = true;
		return (0xdeadbeef);	/* not a number */
	}

	/* check for leading radix indicator */
	int radix;
	if (*num == '0' && tolower(num[1]) == 'x')
	{
		radix = 16;
		num += 2;
	}
	else
	{
		radix = 10;
	}

	UINT64 result = 0;
	int digit;
	while (isxdigit(*num))
	{
		digit = toupper(*num++);
		digit -= '0';
		if (digit > 9)			/* hex range */
		{
			if (radix != 16)	/* not hex radix */
				break;
			digit -= (('A' - '9') - 1);
		}
		else if (digit > 7 && radix == 8)
		{
			break;				/* non octal digit */
		}

		{
			double test = result;

			test *= radix;
			test += digit;

			if (test > UINT64_MAX)
			{
				*err = true;
				return (0xdeadbeef);	/* not a number */
			}
		}

		result = (result * radix) + digit;
	}

	return (result);
}


typedef struct EntityTableStruct
{
	char	ch;
	char	*entityNumber;
	int		entityNumberLen;
	char	*entityString;
	int		entityStringLen;
	char	*entityDesc;
} EntityTableStruct;

static EntityTableStruct EntityTable[] =
{
	{'"',	"&#34;",	0,	"&quot;",	0,	"quotation mark"},
	{'\'',	"&#39;",	0,	"&apos;",	0,	"apostrophe"},
	{'&',	"&#38;",	0,	"&amp;",	0,	"ampersand"},
	{'<',	"&#60;",	0,	"&lt;",		0,	"less-than"},
	{'>',	"&#62;",	0,	"&gt;",		0,	"greater-than"},
	{' ',	"&#160;",	0,	"&nbsp;",	0,	"non-breaking space"},
	{'\0',	NULL,		0, NULL,		0, 	NULL}
};


char*
DecodeEntityCharacter(char *val, char *out)
{

	char *ip = val;

	*out = '\0';

	if (*ip == '\0')
		return ip;

	// Check the entity translation table
	for (EntityTableStruct *e = &EntityTable[0]; e->entityString != NULL; ++e)
	{
	// if needed, initialize the table
		if (e->entityStringLen == 0)
			e->entityStringLen = strlen(e->entityString);
		if (e->entityNumberLen == 0)
			e->entityNumberLen = strlen(e->entityNumber);

		if (strnicmp(ip, e->entityString, e->entityStringLen) == 0)
		{
			SysLog(LogDebug, "Translating %s to %c", e->entityString, e->ch);
			*out = e->ch;
			return ip;
		}
		if (strnicmp(ip, e->entityNumber, e->entityNumberLen) == 0)
		{
			SysLog(LogDebug, "Translating %s to %c", e->entityNumber, e->ch);
			*out = e->ch;
			return ip;
		}
	}

	if (strnicmp(ip, "&#", 2) == 0)
	{
		char *semi = strchr(ip + 2, ';');

		if (semi != NULL)
		{
			int elen = (semi - (ip + 2));

			if (elen > 1)
			{
				char *e = alloca(elen);

				memset(e, '\0', elen);
				memcpy(e, (ip + 2), elen - 1);
				boolean err;
				int v = IntFromString(e, &err);

				if (!err)
				{
					*out = (char)v;
					ip += (elen + 2);
					return ip;
				}
			}
		}
		// failed, fall through and grab the '&'
	}

	*out = (char)*ip++;
	return ip;
}


char*
DecodeEntityCharacters(char *val, int *decodedLen)
{
	StringArrayStatic(sa, 2, 32);
	String_t *out = StringArrayNext(sa);
	StringClear(out);

	for (char *ip = val; *ip;)
	{
		char tmp[32];
		ip = DecodeEntityCharacter(ip, tmp);
		StringCat(out, tmp);
	}

	*decodedLen = out->len;
	return out->str;
}


char *
DecodeUrlCharacter(char *val, char *out)
{
	char *ip = val;

	*out = '\0';

	if (*ip == '\0')
		return ip;

	if (*ip != '%')
	{
		if (*ip == '+')
			*out = ' ';
		else
			*out = *ip;
		++ip;
	}
	else if (ip[1] != '\0' && isxdigit(ip[1]) && ip[2] != '\0' && isxdigit(ip[2]))
	{
		char dig[5];

		dig[0] = '0';
		dig[1] = 'x';
		dig[2] = ip[1];
		dig[3] = ip[2];
		dig[4] = '\0';

		boolean err;
		int c = IntFromString(dig, &err);

		if (err == 0)
		{
			*out = (char)c;
			ip += 3;
		}
		else
		{
			*out = *ip++;
		}
	}
	else
	{
		*out = *ip++;
	}

	return ip;
}


char*
DecodeUrlCharacters(char *val, int *decodedLen)
{
	StringArrayStatic(sa, 2, 32);
	String_t *out = StringArrayNext(sa);
	StringClear(out);

	for (char *ip = val; *ip;)
	{
		char c;
		ip = DecodeUrlCharacter(ip, &c);
		StringCatChar(out, c);
	}

// return the final output length
	if ( decodedLen != NULL )
		*decodedLen = out->len;
	return out->str;
}


char*
EncodeUrlCharacters(char *val, int len)
{
	StringArrayStatic(sa, 2, 32);
	String_t *out = StringArrayNext(sa);
	StringClear(out);

	for (char *ip = val; len > 0; ++ip, --len)
	{
		char ch = *ip;

		if (ch == '\0' || (isspace(ch)) || (!isascii(ch)) || iscntrl(ch) || (!isprint(ch)) || ispunct(ch))
		{
			StringSprintfCat(out, "%%%02X", ch);
		}
		else
		{
			EntityTableStruct *e = NULL;
			for (e = &EntityTable[0]; e->entityString != NULL; ++e)
			{
				if (e->ch == ch)
				{
					StringSprintfCat(out, "%%%02X", ch);
					e = NULL;	// signal done
					break;
				}
			}
			if (e != NULL)		// not converted...
				StringCatChar(out, ch);		// copy the ch
		}
	}

	return out->str;
}


char*
EncodeEntityCharacter(char val, char *out)
{

	*out = '\0';

	if (val == '\0')
		return out;

	// Check the entity translation table
	for (EntityTableStruct *e = &EntityTable[0]; e->entityString != NULL; ++e)
	{
		if (e->ch == val)
		{
			strcpy(out, e->entityString);
			break;		// done
		}
	}

	if ( strlen(out) <= 0 )		// did not translate
	{
		if ( iscntrl(val) )
		{
			sprintf(out, "&#%03d;", val);
		}
		else		// nothing special, just copy it
		{
			out[0] = val;
			out[1] = '\0';
		}
	}

	return out;
}


/**
* Replace entity characters having special meaning inside HTML tags
* with their escaped equivalents, using character entities such as '&amp;'.
*
* The escaped characters are :
*
* <
* >
* "
* '
* \
* &
*
* Use cases for this method include :
*
* render ineffective all HTML present in arbitrary text input
* ensure that arbitrary text appearing inside a tag does not "confuse"
* the tag. For example, HREF='Blah.do?Page=1&Sort=ASC'
* does not comply with strict HTML because of the ampersand, and should be changed to
* HREF='Blah.do?Page=1&amp;Sort=ASC'.
*
*/
char*
EncodeEntityCharacters(char *val, int len)
{
	StringArrayStatic(sa, 2, 32);
	String_t *out = StringArrayNext(sa);
	StringClear(out);

	for (char *ip = val; len > 0; ++ip, --len)
	{
		char tmp[32];
		EncodeEntityCharacter(*ip, tmp);
		StringCat(out, tmp);
	}

	return out->str;
}


char*
EncodeEntityString(char *val)
{
	return EncodeEntityCharacters(val, strlen(val));
}


char*
EncodeCdataCharacters(char *val, int len)
{
	return EncodeUrlCharacters(val, len);
}


#if TEST_WALKDIR
int
main()
{
	char *dir = "procfec";

	WalkDirPath(dir, RmDirNode);
	if ( rmdir(dir) != 0 )
		perror("rmdir");
	return 0;
}
#endif
