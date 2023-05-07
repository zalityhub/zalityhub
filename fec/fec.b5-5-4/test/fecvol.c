/*****************************************************************************

Filename:   main/fecvol.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:34:02 $
 * $Header: /home/hbray/cvsroot/fec/test/fecvol.c,v 1.2.6.7 2011/10/27 18:34:02 hbray Exp $
 *
 $Log: fecvol.c,v $
 Revision 1.2.6.7  2011/10/27 18:34:02  hbray
 Revision 5.5

 Revision 1.2.6.6  2011/09/24 17:49:54  hbray
 Revision 5.5

 Revision 1.2.6.3  2011/08/15 19:12:33  hbray
 5.5 revisions

 Revision 1.2.6.2  2011/07/29 16:04:42  hbray
 Add CVS header

 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <libgen.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/types.h>


#define WIN32	0
// For Minw32; Use:  gcc -g -std=gnu99 fecvol.c -o fecvol -lws2_32


#if WIN32
#include <winsock2.h>
#define EINPROGRESS 10035
#define SockGetErrno WSAGetLastError()
#define SockSetErrno(val) WSASetLastError(val)
#define SockError(text) fprintf(stderr, "%s %d", text, SockGetErrno)
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#define SockGetErrno errno
#define SockSetErrno(val) errno=(val)
#define SockError(text) perror(text)
#define max(x,y) ( (x)>(y)? (x) : (y) )
#endif





#define Log(level, fmt, ...) \
		( (level == 0) || (((Verbose)>=(level))) ) ? _Log ( \
			level, \
			__FILE__, \
			__LINE__, \
			fmt, ##__VA_ARGS__):(void)0

static void _Log (int lvl, char *logFile, int logLno, char *fmt, ...);
static char *LogSetIndent(char *indent);
static char *LogIndent = NULL;

#define LogError(fmt, ...) Log(-1, fmt, ##__VA_ARGS__)


typedef struct SubValue_t
{
	char	type;
	char	*from;
	char	*to;
} SubValue_t ;


typedef enum
{
	NullState, ConnectedState, SendingState
} FsmState_t;

typedef enum
{
	InitializeEvent, StartEvent, StopEvent, ReadReadyEvent, SendNextEvent, TimeoutEvent
} FsmEvent_t;


typedef unsigned long long Time_t;



#define MaxMsgs			1024
#define MaxSessions		((int)(((double)FD_SETSIZE) * 0.95))
#define Msize			(1024*1024*20)


typedef struct Session_t
{
	int				sessionNbr;
	int				fd;

	int				destinationPort;
	char			*destinationAddress;

	time_t			timeStarted;
	time_t			timeStopped;
	time_t			testTime;

	int 			timedout;

	int				msgNbr;			// current message nbr

	char			*rcvBfr;
	int				rcvSize;		// current size of buffer
	int				rcvLen;			// current space in use

	FsmState_t		state;
	FsmEvent_t		event;

	int				times;

	unsigned long	nbrSent;
	unsigned long	nbrRecv;

	Time_t			timeOfSend;
	double			minRsp;
	double			maxRsp;
	double			avgRsp;

	int				nbrErrors;

	struct	sockaddr_in sockAddr;
} Session_t ;


typedef struct Msg_t
{
	char	*fname;
	char	*content;
	int		len;
} Msg_t ;



#if 0
static SubValue_t	SubValues[] =		// Gxml
{
	{'s', "${Chain_Code}", "ES"},
	{'s', "${Location_ID}", "LODG"},
	{'s', "${Venue_ID}", "RES"},
	{'i', "${Message_ID}", "123456789"},
	{'i', "${Invoice_Nbr}", "09111701"},
	{'\0', NULL, NULL}
} ;
#endif

#if 1
static SubValue_t	SubValues[] =		// API
{
	{'i', "${refnbr}", "3500"},
	{'\0', NULL, NULL}
} ;
#endif


static char			*MyName = "";
static int			NbrSessions = 0;
static int			NbrForks = 0;
static Session_t	*Sessions;
static Session_t	RootSession = {-1, 0};
static int			MinRspSn = 0;
static int			MaxRspSn = 0;
static Session_t	*CurrentSession = &RootSession;
static fd_set		ReadFdSet;
static int			MaxFd = -1;
static char			*Eom = "\x04";

static int			ActiveSessions = 0;

static int			NbrMsgs = 0;
static Msg_t		Msgs[MaxMsgs];

static int			Verbose = 0;
static int			WaitTime = 30;
static int			Loops = 1;
static int			TargetTps = 0;
static int			TargetTestTime = 0;
static int			DestinationPort = 0;
static char			*DestinationAddress = "";
static struct hostent *Host = NULL;



static void SessionEvent(FsmEvent_t event);


// Compatibility stuff

#if WIN32
#define inet_pton mgw_inet_pton
static const int
mgw_inet_pton(int af, const char *src, void *dst)
{
	int len = 0;
	if ( isdigit (src[0]) )
	{
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));		// create & zero struct
		addr.sin_family = af;
		addr.sin_addr.s_addr = inet_addr (src);
		len = sizeof(addr.sin_addr.s_addr);
		memcpy(dst, &addr.sin_addr.s_addr, len);
	}
	else
	{
	struct hostent	*he;

	if ( (he = gethostbyname (src)) == NULL )
		return -1;
		len = he->h_length;
		memcpy(dst, he->h_addr, len);
	}
	return len;
}
#endif


static long
SockSetSocketBufferSize(int fd, long maxPktLen)
{
	unsigned int	rcv = 0;
	unsigned int	snd = 0;
	unsigned int	tos = maxPktLen;
	socklen_t		len;

	len = sizeof(rcv);
	if ( getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&rcv, &len) != 0 )
	{
		int sockErr = errno;
		SockError( "getsockopt failed");
		errno = sockErr;
		return -1;
	}
	if ( setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&tos, sizeof(tos)) != 0 )
	{
		int sockErr = errno;
		SockError( "setsockopt failed");
		errno = sockErr;
		return -1;
	}
	len = sizeof(rcv);
	if ( getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&rcv, &len) != 0 )
	{
		int sockErr = errno;
		SockError( "getsockopt failed");
		errno = sockErr;
		return -1;
	}

	len = sizeof(snd);
	if ( getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&snd, &len) != 0 )
	{
		int sockErr = errno;
		SockError( "getsockopt failed");
		errno = sockErr;
		return -1;
	}
	if ( setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&tos, sizeof(tos)) != 0 )
	{
		int sockErr = errno;
		SockError( "setsockopt failed");
		errno = sockErr;
		return -1;
	}
	len = sizeof(snd);
	if ( getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&snd, &len) != 0 )
	{
		int sockErr = errno;
		SockError( "getsockopt failed");
		errno = sockErr;
		return -1;
	}

	if (rcv < maxPktLen)
		return((long)rcv);

	if (snd < maxPktLen)
		return((long)snd);

	return(maxPktLen);
}


static int
SockSetSocketOptions(int fd, int maxPktLen)
{

	{
		int tos = 1;

		if ( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SockError( "setsockopt SO_REUSEADDR failed");
			errno = sockErr;
			return -1;
		}
	}	/* Allow address bound to socket be reused */

	{	/* Only LINGER half a second a on socket close */
		struct linger tos = {0, 50}; /* centiseconds */

		if ( setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SockError( "setsockopt failed");
			errno = sockErr;
			return -1;
		}
	}	/* Only LINGER half a second a on socket close */

	{	/* we use small packets, set lowest posible network delay */
		int tos = IPTOS_LOWDELAY;

		if ( setsockopt(fd, SOL_IP, IP_TOS, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SockError( "setsockopt failed");
			errno = sockErr;
			return -1;
		}
	}	/* we use small packets, so set lowest posible network delay */

	{	/* we use small packets, set lowest posible network delay */
		int tos = 1;

		if ( setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SockError( "setsockopt failed");
			errno = sockErr;
			return -1;
		}
	}	/* we use small packets, so set lowest posible network delay */

	{	/* we use small packets, set lowest posible network delay */
		struct timeval tos = {0, 25 * 1000000}; /* 250 milliseconds */

		if ( setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SockError( "setsockopt failed");
			errno = sockErr;
			return -1;
		}
	}	/* we use small packets, so set lowest posible network delay */

	{	/* we use small packets, set lowest posible network delay */
		struct timeval tos = {0, 25 * 1000000}; /* 250 milliseconds */

		if ( setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SockError( "setsockopt failed");
			errno = sockErr;
			return -1;
		}
	}	/* we use small packets, so set lowest posible network delay */

	{	/* Receive out-of-band data in line */
		int tos = 1;

		if ( setsockopt(fd, SOL_SOCKET, SO_OOBINLINE, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SockError( "setsockopt failed");
			errno = sockErr;
			return -1;
		}
	}	/* Receive out-of-band data in line */

    if ( SockSetSocketBufferSize(fd, maxPktLen) != maxPktLen )
    {
		int sockErr = errno;
		SockError( "SockSetSocketBufferSize failed");
		errno = sockErr;
		return -1;
    }

#if 1
	{	/* Reset receive / send low water marks */
		int tos = 0;

		if ( setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SockError( "setsockopt failed");
			errno = sockErr;
			return -1;
		}
#if 0		// Does not work on Linux < 2.4
		if ( setsockopt(fd, SOL_SOCKET, SO_SNDLOWAT, (char *)&tos, sizeof(tos)) )
		{
			int sockErr = errno;
			SockError( "setsockopt failed");
			errno = sockErr;
			return -1;
		}
#endif
	}	/* Reset receive / send low water marks */
#endif

	return 0;
}


static int
SockSetNoWait(int fd)
{

// set the socket to nowait
#if WIN32
	{
		u_long one = 1;
		if ( ioctlsocket(CurrentSession->fd, FIONBIO, &one) != 0 )
		{
			SockError("\nioctl");
			fprintf(stderr, "Unable to ioctlsocket %s.%d\n", CurrentSession->destinationAddress, CurrentSession->destinationPort);
			close(CurrentSession->fd);
			return (CurrentSession->fd = -1);
		}
	}
#else
	{
		int flags = fcntl(CurrentSession->fd, F_GETFL);
		flags |= (O_NDELAY | O_NONBLOCK);
		if (fcntl(CurrentSession->fd, F_SETFL, flags) != 0)
		{
			perror("\nfcntl");
			fprintf(stderr, "Unable to fcntl %s.%d\n", CurrentSession->destinationAddress, CurrentSession->destinationPort);
			close(CurrentSession->fd);
			return (CurrentSession->fd = -1);
		}
	}
#endif

	return 0;
}


static fd_set*
SockSelectSet(fd_set fdSet, int ms)
{
	struct timeval timeout;
	timeout.tv_sec = ms / 1000;
	timeout.tv_usec = 0;

	static fd_set selectSet;
	memcpy(&selectSet, &fdSet, sizeof(selectSet));

	int n = select(MaxFd + 1, &selectSet, NULL, NULL, ms == -1 ? NULL : &timeout);	// timeout or ready
	if ( n < 0 )
	{
		int sockErr = SockGetErrno;
		LogError("select(%d) error=%s(%d)", n+1, strerror(sockErr), sockErr);
		SockSetErrno(sockErr);
		return NULL;		// error
	}
	return &selectSet;
}


static char*
GetMnemonicCh(unsigned char ch, char *mnem)
{
	static char *specialChs[32] =
	{
		"Nul", "Soh", "Stx", "Etx", "Eot", "Enq", "Ack", "Bel", "Bs",
		"Ht", "Lf", "Vt", "Ff", "Cr", "So", "Si", "Dle", "Dc1",
		"Dc2", "Dc3", "Dc4", "Nak", "Syn", "Etb", "Can", "Em", "Sub",
		"Esc", "Fs", "Gs", "Rs", "Us"
	};
	static char tmp[32];

	if (mnem == (char *)0)
		mnem = tmp;

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


static char*
GetMnemonicString(char *text, int slen)	// pass slen=0 to use strlen of text
{
	if (slen <= 0)
		slen = strlen((char *)text);

	static char *tmp = NULL;
	if ( tmp )
		free(tmp);		// release prev bfr
	tmp = calloc(slen+16, 3);		// len * 3 chrs

	for (char *output = tmp; slen > 0; ++text, --slen)
		output += sprintf(output, "%s", GetMnemonicCh(*text, NULL));

	return tmp;
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

static int
IntFromString(const char *num, int *err)
{
	int errBoolean;

	if ( err == NULL )
		err = &errBoolean;
	*err = 0;

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
		*err = 1;
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
				*err = 1;
				return (0xdeadbeef);	/* not a number */
			}
		}

		result = (result * radix) + digit;
	}

	if (sign == '-')			/* negative value */
		result = 0 - result;

	return (result);
}


// us time
static Time_t
GetTime()
{
	struct timeval tv;
	Time_t t;

	gettimeofday(&tv, NULL);
	t = ((Time_t) tv.tv_usec);
	t += (((Time_t) tv.tv_sec) * 1000000L);
	return t;
}


static inline int TpsWait()
{
	if ( TargetTps > 0 )
	{
		RootSession.testTime = (GetTime() - RootSession.timeStarted);
		if ( (((double)RootSession.nbrRecv) / (((double)RootSession.testTime) / 1000000.0)) > TargetTps )
		{
			usleep(100);
			return  -1;
		}
	}
	return 0;
}


static char*
LogGetPrefix()
{
	time_t	tod;
	time(&tod);
	struct tm *ts;
	ts = localtime(&tod);
// format text
	static char	text[1024];;
	char tmp[128];
	if ( CurrentSession->sessionNbr < 0 )
		sprintf(tmp, "%s", "Main");
	else
		sprintf(tmp, "Session.%d", CurrentSession->sessionNbr+1);

	sprintf(text, "%02d:%02d:%02d %-14.14s", ts->tm_hour, ts->tm_min, ts->tm_sec, tmp);
	return text;
}


static char*
LogSetIndent(char *indent)
{
	char *saved = strdup(LogIndent);
	free(LogIndent);		// ditch prev one
	LogIndent = strdup(indent);
	return saved;
}


static void
_Log (int lvl, char *logFile, int logLno, char *fmt, ...)
{
	time_t tod;
	time(&tod);
	struct tm *ts;
	ts = localtime(&tod);
	FILE	*fout = stdout;

	if ( lvl < 0 )
	{
		printf("\n");
		fprintf(stderr, "\n");
		fout = stderr;
		fflush(NULL);
	}

// format text
	char *pfx = LogGetPrefix();

	va_list ap;
	va_start(ap, fmt);

	if ( NbrForks > 1 )
		fprintf(fout, "%s%d: %s ", LogIndent, getpid(), pfx);
	else
		fprintf(fout, "%s%s ", LogIndent, pfx);

	fprintf(fout, "%s%s ", LogIndent, pfx);
	vfprintf(fout, fmt, ap);

	if ( lvl < 0 )
		fputs("\n", fout);
	fflush(NULL);
}


static int
RandomNbr(void)
{
	return rand();
}


static int
RandomRange(int minVal, int maxVal)
{

	int r = RandomNbr();

	if (minVal == maxVal)
		return r;				/* standard call, no scaling is desired */

	double d = (((double)r) / ((double)RAND_MAX));	// makes it 0 - .999

	r = (int)((d * (double)(maxVal - minVal) + (double)minVal) + 0.5);
	return r;
}


#if 0
static int
RandomBoolean(int percentage)
{
	return (RandomRange(1, 100) <= percentage);
}
#endif


static char*
SubValueGetFrom(SubValue_t *sub)
{
	char *tmp = sub->from;
	while ( *tmp && *tmp != '{' )
		++tmp;			// find begin
	char *from;
	if ( *tmp == '{' )
		from = strdup(++tmp);
	else
		from = strdup(tmp);
	for ( tmp = from; *tmp && *tmp != '}'; )
		++tmp;
	*tmp = '\0';		// trim it
	return from;
}


static char*
ReplaceContent(char *content, char *from, char *to)
{
	char *iptr;
	while ( (iptr = strstr(content, from)) != NULL )
	{
		char *nmsg = calloc(1, strlen(content)+strlen(to)+128);
		*iptr = '\0';			// cut it
		iptr += strlen(from);			// point to suffix
		char *optr = nmsg;
		optr += sprintf(optr, "%s", content);		// non matching prefix
		optr += sprintf(optr, "%s", to);		// the replacement
		optr += sprintf(optr, "%s", iptr);	// the suffix
		free(content);
		content = nmsg;
	}
	return content;
}


static char*
FixupContent(char *content, SubValue_t *subs)
{
	for(SubValue_t *sub = subs; sub->from; ++sub)
	{
		char *nmsg = ReplaceContent(content, sub->from, sub->to);
		if ( nmsg != content )	// did a replace; modify the template
		{
			content = nmsg;
			switch(sub->type)
			{
				default:
					break;
				case 'i':
				{
					char *to = calloc(1, 128);
					sprintf(to, "%d", atoi(sub->to)+1);
					sub->to = to;		// new to
					break;
				}
				case 'r':
				{
					char *to = calloc(1, 128);
					sprintf(to, "%d", RandomRange(1, 32000));
					sub->to = to;		// new to
					break;
				}
			}
		}
	}
	return content;
}


static void
Usage(char *fmt, ...)
{
	char msg[1024];
	{
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(msg, sizeof(msg)-1, fmt, ap);
	}
	fprintf(stderr, "\n%s\n\n", msg);
	fprintf(stderr, "Usage: %s  -d destination_address:port  [-r rate] [-w wait_seconds]  [-t time]  [-l loop_times] [-s sessions]  [-f forks]  [-e eom] [-h] [-v] < input_file\n", MyName);
	fprintf(stderr, "Sends contents of input_file to destination:\n");
	fprintf(stderr, "  then waits 30 seconds for a response\n");
	fprintf(stderr, "    -d destination_address:port is used as the target of the test; for example -d 10.165.3.229:6000\n");
	fprintf(stderr, "                                                                       or      -d 127.0.0.1:6000\n");
	fprintf(stderr, "    -r rate;  simulator attempts to regulate the transaction flow to achieve this Tp/s rate\n");
	fprintf(stderr, "    -w time; to wait for response in seconds: default is 30 seconds\n");
	fprintf(stderr, "    -t time;  simulator runs for this period of time (seconds); implied looping\n");
	fprintf(stderr, "    -l number; of test cycles to run: default is once\n");
	fprintf(stderr, "    -s number; of sessions; default is one; Maximum is %d\n", MaxSessions);
	fprintf(stderr, "    -f number; of forks; default is one\n");
	fprintf(stderr, "    -e end of message in form 0x00 or text string; default is 0x04\n");
	fprintf(stderr, "    -h input files are hex encoded; applies to all inputs\n");
	fprintf(stderr, "    -v Verbose mode; string multiple vvvv for more\n");
	fprintf(stderr, "\nSubstitute values:\n");
	for(SubValue_t *sub = SubValues; sub->from; ++sub)
		fprintf(stderr, "    -%s\n", SubValueGetFrom(sub));
	fprintf(stderr, "\n");
	fprintf(stderr, "  The default timeout of 30 seconds can be changed to any positive value (including zero) using the -w option\n");
	fprintf(stderr, "  Use the -l option to control the number of 'times' the test is performed (the default is once)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Example:\n");
	fprintf(stderr, "    ./fecvol -d 127.0.0.1:5101 <operaxml.in -vvvvvvv -e '</Response>'\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    -d 127.0.0.1:5101   Local FEC, port 5101\n");
	fprintf(stderr, "    <operaxml.in        Redirect reference to the input test message\n");
	fprintf(stderr, "    -vvvvvvvvv          Be very verbose when running\n");
	fprintf(stderr, "    –e ‘</Response>'    Format of the end of a response message\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    Remaining arguments ‘tune’ fecvol messaging rates\n");
	fprintf(stderr, "    the above example sends a single transaction and waits for the response. If you want to ‘spin’ in a loop (multiple messages), use ‘–l 10’ for ten loops.\n");
	fprintf(stderr, "\n");
	exit(1);
}


static Msg_t*
GetContent(Msg_t *msg, int hex)
{

	if ( strcmp(msg->fname, "stdin") != 0 )		// not stdin; reopen stdin
	{
		if ( freopen(msg->fname, "r", stdin) == NULL )
		{
			fprintf(stderr, "Unable to open %s\n", msg->fname);
			perror("freopen");
			Usage("Invalid file");
		}
	}

	char *content = calloc(1, Msize+10);
	char *out = content;

	if (hex)
	{
		int ch;

		while ((ch = fgetc(stdin)) != EOF)
		{
			while (isspace(ch))
				ch = fgetc(stdin);	// skip whitespace

			if (ch == EOF)
			{
				*out = '\0';
				break;
			}

			int ch1 = ch;
			int ch2 = fgetc(stdin);

			if (ch2 == EOF)
			{
				fprintf(stderr, "Premature EOF reached\n");
				exit(1);
			}

			if (!isxdigit(ch1))
			{
				fprintf(stderr, "Nibble %c is not a hex digit\n", ch1);
				exit(1);
			}
			if (!isxdigit(ch2))
			{
				fprintf(stderr, "Nibble %c is not a hex digit\n", ch2);
				exit(1);
			}

			{					// convert to hex
				int dig1 = toupper(ch1) - '0';

				if (dig1 > 9)
					dig1 -= 7;
				int dig2 = toupper(ch2) - '0';

				if (dig2 > 9)
					dig2 -= 7;
				int hex = (dig1 << 4) | dig2;

				*out++ = (char)hex;
			}
		}
	}
	else
	{
		// gather the message
		int rlen = read(fileno(stdin), out, Msize);	// one big read
		if (rlen < 0)
		{
			fprintf(stderr, "no data!!\n");
			exit(1);
		}
		out += rlen;
	}

	*out = '\0';
	msg->len = (out - content);
	if ( (msg->content = calloc(1, msg->len+10)) == NULL )
	{
		fprintf(stderr, "\nNo memory\n");
		exit(1);
	}

	memcpy(msg->content, content, msg->len+1);		// include the final null chr
	free(content);
	return msg;
}


static void
SessionClear(Session_t *session)
{
	FsmState_t state = session->state;
	FsmEvent_t event = session->event;
	int sn = session->sessionNbr;

	memset(session, 0, sizeof(Session_t));
	session->minRsp = 0.0;
	session->maxRsp = 0.0;
	session->avgRsp = 0.0;

	session->state = state;
	session->event = event;
	session->sessionNbr = sn;
}


static void
SessionSetDestination()
{
	CurrentSession->destinationPort = DestinationPort;
	CurrentSession->destinationAddress = DestinationAddress;
}


static int
SessionConnect()
{

	SessionClear(CurrentSession);
	memset(&CurrentSession->sockAddr, 0, sizeof(CurrentSession->sockAddr));
	SessionSetDestination();

	CurrentSession->sockAddr.sin_family = AF_INET;
	CurrentSession->sockAddr.sin_port = htons(CurrentSession->destinationPort);
	CurrentSession->sockAddr.sin_addr.s_addr = *(long*)(Host->h_addr_list[0]);  /* set the addr */

	CurrentSession->fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	SockSetSocketOptions(CurrentSession->fd, 8192);

	if (CurrentSession->fd < 0)
	{
		SockError("cannot create socket");
		return -1;
	}

	SockSetNoWait(CurrentSession->fd);

	if (connect(CurrentSession->fd, (const void *)&CurrentSession->sockAddr, sizeof(CurrentSession->sockAddr)) < 0)
	{
		if ( SockGetErrno != EINPROGRESS )
		{
			SockError("connect failed");
			close(CurrentSession->fd);
			return (CurrentSession->fd = -1);
		}
	}

	++ActiveSessions;
	CurrentSession->timeStarted = GetTime();
	FD_SET(CurrentSession->fd, &ReadFdSet);
	MaxFd = max(MaxFd, CurrentSession->fd);

	Log(2, "Connected to %s\n", CurrentSession->destinationAddress);
	return 0;
}


static int
SessionDisconnect()
{
	Log(2, "Disconnect from %s\n", CurrentSession->destinationAddress);

	if ( CurrentSession->fd > 0 )
	{
		close(CurrentSession->fd);
		FD_CLR(CurrentSession->fd, &ReadFdSet);
		CurrentSession->fd = -1;
	}

	CurrentSession->timeStopped = GetTime();
	--ActiveSessions;
	return 0;
}


static int
SessionNextMessageNbr()
{
	if ( (CurrentSession->msgNbr+1) <= NbrMsgs )
		return ++CurrentSession->msgNbr;
	return -1;
}


static int
SessionSend()
{
	char	*ptr;
	int		len = 0;
	int		mn = CurrentSession->msgNbr-1;
	Msg_t	*msg = &Msgs[mn];

	char *content = strdup(msg->content);
	content = FixupContent(content, SubValues);

	int		mlen = strlen(content);
	for (ptr = content; mlen > 0 ; )
	{
		int slen;
		// if ((slen = send(CurrentSession->fd, ptr, 1, 0)) <= 0 )
		if ((slen = send(CurrentSession->fd, ptr, mlen, 0)) <= 0 )
		{
			if ( SockGetErrno != EAGAIN )
			{
				SockError("send failed");
				free(content);
				return -1;
			}
			// busy, do a wait

			fd_set selectSet;
			FD_ZERO(&selectSet);
			FD_SET(CurrentSession->fd, &selectSet);
			if ( SockSelectSet(ReadFdSet, 1000) < 0 )
			{
				SockError("select failed");
				free(content);
				return -1;
			}
			continue;		// try again
		}
		Log(2, "Msg %d: Sent %d\n", CurrentSession->msgNbr, slen);
		if ( Verbose > 2 )
		{
			for(int i = 0; i < slen; ++i)
				fputs(GetMnemonicCh(ptr[i], NULL), stdout);
			fputs("\n", stdout);
			fflush(NULL);
		}
		ptr += slen;
		mlen -= slen;
	}

	++CurrentSession->nbrSent;
	++RootSession.nbrSent;
	CurrentSession->timeOfSend = GetTime();
	free(content);
	return len;
}


static int
SessionRecv()
{
	unsigned long ready;
#if WIN32
	ready = 8192;
#else
	if (ioctl(CurrentSession->fd, FIONREAD, &ready) != 0)
	{
		int sockErr = SockGetErrno;
		LogError("ioctl() error=%s(%d)", strerror(sockErr), sockErr);
		SockSetErrno(sockErr);
		return -1;
	}
#endif

	if ( (CurrentSession->rcvSize + ready) > CurrentSession->rcvSize )		// need more space
	{
		CurrentSession->rcvSize = (CurrentSession->rcvSize + ready);
		CurrentSession->rcvBfr = realloc(CurrentSession->rcvBfr, CurrentSession->rcvSize + 10);
	}

	char *bfr = &CurrentSession->rcvBfr[CurrentSession->rcvLen];
	int rlen = recv(CurrentSession->fd, bfr, ready, 0);
	if (rlen <= 0)
	{
		int sockErr = SockGetErrno;
		if ( sockErr == EINPROGRESS )
			sockErr = EAGAIN;
#if ! WIN32
		if ((sockErr == ECONNRESET || sockErr == ENOTCONN || sockErr == EPIPE))
			SockSetErrno ((sockErr = rlen = 0));	// all these are valid disconnects...
#endif
		if ( sockErr != 0 && sockErr != EAGAIN )
			LogError("recv() error=%s(%d)", strerror(sockErr), sockErr);
		else
			return 0;
		SockSetErrno(sockErr);
		return -1;
	}

	Log(2, "Msg %d: Recv %d\n", CurrentSession->msgNbr, rlen);
	if ( Verbose > 2 )
	{
		for(int i = 0; i < rlen; ++i)
			fputs(GetMnemonicCh(bfr[i], NULL), stdout);
		fputs("\n", stdout);
		fflush(NULL);
	}

	bfr[rlen] = '\0';
	CurrentSession->rcvLen += rlen;
	++CurrentSession->nbrRecv;
	++RootSession.nbrRecv;

	Time_t rspTime = GetTime() - CurrentSession->timeOfSend;
	if ( CurrentSession->minRsp == 0 || rspTime < CurrentSession->minRsp )
		CurrentSession->minRsp = rspTime;
	if ( CurrentSession->maxRsp == 0 || rspTime > CurrentSession->maxRsp )
		CurrentSession->maxRsp = rspTime;
	CurrentSession->avgRsp += rspTime;

	if ( RootSession.minRsp == 0 || rspTime < RootSession.minRsp )
	{
		RootSession.minRsp = rspTime;
		MinRspSn = CurrentSession->sessionNbr;
	}
	if ( RootSession.maxRsp == 0 || rspTime > RootSession.maxRsp )
	{
		RootSession.maxRsp = rspTime;
		MaxRspSn = CurrentSession->sessionNbr;
	}
	RootSession.avgRsp += rspTime;
	return rlen;
}


static fd_set*
SessionSelect(int ms)
{
	if ( TpsWait() < 0 )
		return  NULL;
	return SockSelectSet(ReadFdSet, ms);
}


static void
SessionNullState(int event)
{
	switch(event)
	{
		default:
			LogError("%d is an invalid event for state %d\n", event, CurrentSession->state);
			exit(1);
			break;
	
		case InitializeEvent:
			CurrentSession->rcvBfr = calloc(1,1);
			CurrentSession->rcvSize = 0;
			// fall through to StartEvent

		case StartEvent:
			if ( SessionConnect() != 0 )
			{
				LogError("SessionConnect failed\n");
				exit(1);
			}
			CurrentSession->state = ConnectedState;
			break;
		case TimeoutEvent:
			++CurrentSession->timedout;
			++RootSession.timedout;
			LogError("Timed out");
		case StopEvent:
			break;
	}
}


static void
SessionConnectedState(int event)
{
	switch(event)
	{
		default:
			LogError("%d is an invalid event for state %d\n", event, CurrentSession->state);
			exit(1);
			break;
		case StartEvent:
		case SendNextEvent:
			if ( CurrentSession->times < Loops )
			{
				if ( SessionNextMessageNbr() > 0 )
				{
					if ( SessionSend() < 0 )
					{
						LogError("SessionSend failed\n");
						exit(1);
					}
				}
				CurrentSession->state = SendingState;
			}
			else
			{
				SessionDisconnect();
				CurrentSession->state = NullState;
			}
			break;
		case TimeoutEvent:
			++CurrentSession->timedout;
			++RootSession.timedout;
			LogError("Timed out");
		case StopEvent:
			SessionDisconnect();
			CurrentSession->state = NullState;
			break;
	}
}


static void
SessionSendingState(int event)
{
	switch(event)
	{
		default:
			LogError("%d is an invalid event for state %d\n", event, CurrentSession->state);
			exit(1);
			break;

		case ReadReadyEvent:
		{
			int rlen;
			if ( (rlen = SessionRecv()) <= 0 )
			{
				if ( rlen < 0 )
					LogError("SessionRecv failed\n");
				if ( errno == EAGAIN )
					break;
				SessionDisconnect();
				CurrentSession->state = NullState;
				break;
			}
			if ( strstr(CurrentSession->rcvBfr, Eom) == NULL )		// not end of message
				break;
			if ( (CurrentSession->msgNbr+1) <= NbrMsgs )
			{
				SessionEvent(SendNextEvent);
				break;
			}
			if ( ++CurrentSession->times < Loops ||
				(TargetTestTime > 0 && (RootSession.testTime = (GetTime() - RootSession.timeStarted)) < TargetTestTime) )
			{
				CurrentSession->msgNbr = 0;
				SessionEvent(SendNextEvent);
				break;
			}
			// msgs all done
			SessionDisconnect();
			CurrentSession->state = NullState;
			break;
		}
		case StartEvent:
		case SendNextEvent:
			if ( SessionNextMessageNbr() > 0 )
			{
				if ( SessionSend() < 0 )
				{
					LogError("SessionSend failed\n");
					exit(1);
				}
			}
			break;
		case TimeoutEvent:
			++CurrentSession->timedout;
			++RootSession.timedout;
			LogError("Timed out");
		case StopEvent:
			SessionDisconnect();
			CurrentSession->state = NullState;
			break;
	}
}


static void
SessionEvent(FsmEvent_t event)
{

	switch(CurrentSession->state)
	{
		default:
			LogError("%d is an invalid state\n", CurrentSession->state);
			exit(1);
			break;

		case NullState:
			SessionNullState(event);
			break;

		case ConnectedState:
			SessionConnectedState(event);
			break;

		case SendingState:
			SessionSendingState(event);
			break;
	}

}


static void
SessionEventAll(FsmEvent_t event)
{
	for (int sn = 0; sn < NbrSessions; ++sn)
	{
		CurrentSession = &Sessions[sn];
		SessionEvent(event);
	}
	CurrentSession = &RootSession;
}


static void
SessionReport(Session_t *sess, char *indent)
{
	CurrentSession = sess;
	CurrentSession->testTime = (CurrentSession->timeStopped - CurrentSession->timeStarted);
	char *savedIndent = LogSetIndent(indent);
	Log(0, "TestTime:          %-6.4f\n", ((double)CurrentSession->testTime) / 1000000.0);
	Log(0, "NbrSent:           %d\n", CurrentSession->nbrSent);
	Log(0, "NbrRecv:           %d\n", CurrentSession->nbrRecv);
	if ( sess->sessionNbr == -1 )		// reporting root
	{
		Log(0, "MinRsp:            %-6.4f : Scored by Session %d\n", RootSession.minRsp / 1000000.0, MinRspSn+1);
		Log(0, "MaxRsp:            %-6.4f : Scored by Session %d\n", RootSession.maxRsp / 1000000.0, MaxRspSn+1);
	}
	else
	{
		Log(0, "MinRsp:            %-6.4f\n", CurrentSession->minRsp / 1000000.0);
		Log(0, "MaxRsp:            %-6.4f\n", CurrentSession->maxRsp / 1000000.0);
	}
	unsigned long nbrRecv = max(1, CurrentSession->nbrRecv);
	Log(0, "AvgRsp:            %-6.4f\n", CurrentSession->avgRsp / nbrRecv / 1000000.0);
	Log(0, "TPS:               %-6.4f\n", ((double)CurrentSession->nbrRecv) / (((double)CurrentSession->testTime) / 1000000.0));
	if ( CurrentSession->nbrSent != CurrentSession->nbrRecv )
	{
		Log(0, "NbrSent != NbrRecv ***\n");
		++CurrentSession->nbrErrors;
		++RootSession.nbrErrors;
	}
	if ( CurrentSession->timedout )
	{
		Log(0, "Timeouts: ***      %d\n", CurrentSession->timedout);
		++CurrentSession->nbrErrors;
		++RootSession.nbrErrors;
	}

	if ( CurrentSession->nbrErrors > 0 )
		Log(0, "NbrErrors:         %-6d\n", CurrentSession->nbrErrors);

	LogSetIndent(savedIndent);
	free(savedIndent);
}


int
main(int argc, char *argv[])
{

#if WIN32
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD (1, 1);
	if ( WSAStartup (wVersionRequested, &wsaData) != 0 )
	{
		perror("\nWSAStartup");
		return -1;
	}
#endif

	LogIndent = strdup("");
	srand(GetTime());	// seed
	MyName = basename(strdup(argv[0]));

	FD_ZERO(&ReadFdSet);

	Sessions = calloc(MaxSessions, sizeof(Session_t));
	for(int sn = 0; sn < MaxSessions; ++sn)
	{
		memset(&Sessions[sn], 0, sizeof(Session_t));
		Sessions[sn].sessionNbr = sn;
		SessionClear(&Sessions[sn]);
	}

	memset(&RootSession, 0, sizeof(RootSession));
	RootSession.sessionNbr = -1;
	SessionClear(&RootSession);
	RootSession.timeStarted = GetTime();

	memset(Msgs, 0, sizeof(Msgs));

	NbrSessions = 1;
	NbrForks = 1;

	--argc;
	argv += 1;

	int		hex = 0;
	while (argc > 0)
	{
		if (strcmp(argv[0], "-d") == 0)
		{
			char *ptr;

			if (argc < 2)
				Usage("address must follow -d");
			DestinationAddress = argv[1];
			if ((ptr = strchr(DestinationAddress, ':')) != NULL)	// port designation
			{
				*ptr++ = '\0';	// trim address
				DestinationPort = atoi(ptr);
				if ( (!isdigit(*ptr)) || DestinationPort < 0)
					Usage("Invalid port number given");
			}
			else
			{
				Usage("port is missing from -d");
			}
			argc -= 2;
			argv += 2;
		}
		else if (strcmp(argv[0], "-w") == 0)
		{
			if (argc < 2 || (!isdigit(argv[1][0])))
				Usage("seconds must follow -w");
			WaitTime = atoi(argv[1]);
			if (WaitTime < 0)
				Usage("Invalid response wait time");
			argc -= 2;
			argv += 2;
		}
		else if (strcmp(argv[0], "-l") == 0)
		{
			if (argc < 2 || (!isdigit(argv[1][0])))
				Usage("a number must follow -l");
			if ( (Loops = atoi(argv[1])) < 0 )
				Loops = 0;
			argc -= 2;
			argv += 2;
		}
		else if (strcmp(argv[0], "-s") == 0)
		{
			if (argc < 2 || (!isdigit(argv[1][0])))
				Usage("a number must follow -s");
			if ( (NbrSessions = atoi(argv[1])) > MaxSessions || NbrSessions <= 0 )
			{
				fprintf(stderr, "\n%d is too many sessions. Maximum is %d\n", NbrSessions, MaxSessions);
				exit(1);
			}
			argc -= 2;
			argv += 2;
		}
		else if (strcmp(argv[0], "-f") == 0)
		{
			if (argc < 2 || (!isdigit(argv[1][0])))
				Usage("a number must follow -f");
			if ( (NbrForks = atoi(argv[1])) <= 0 )
			{
				fprintf(stderr, "\n%d is invalid nbr forks.\n", NbrForks);
				exit(1);
			}
			argc -= 2;
			argv += 2;
		}
		else if (strcmp(argv[0], "-t") == 0)
		{
			if (argc < 2 || (!isdigit(argv[1][0])))
				Usage("a number must follow -t");
			if ( (TargetTestTime = atoi(argv[1])) < 0 )
				TargetTestTime = 0;
			TargetTestTime *= 1000000;
			Loops = 1;				// no looping... the 'TargetTestTime' handles the looping
			argc -= 2;
			argv += 2;
		}
		else if (strcmp(argv[0], "-r") == 0)
		{
			if (argc < 2 || (!isdigit(argv[1][0])))
				Usage("a number must follow -r");
			if ( (TargetTps = atoi(argv[1])) < 0 )
				TargetTps = 0;
			argc -= 2;
			argv += 2;
		}
		else if (strncmp(argv[0], "-v", 2) == 0)
		{
			for(char *v = argv[0]; *++v == 'v';)
				++Verbose;
			argc -= 1;
			argv += 1;
		}
		else if (strcmp(argv[0], "-h") == 0)
		{
			hex = 1;
			argc -= 1;
			argv += 1;
		}
		else if (strcmp(argv[0], "-e") == 0)
		{
			if (argc < 2 || (isdigit(argv[1][0])))
			{
				Eom = calloc(1, 16);
				int err;
				Eom[0] = IntFromString(argv[1], &err);
				if ( err )
					Usage("%s must be all numeric", argv[1]);
			}
			else
			{
				Eom = strdup(argv[1]);
			}
			argc -= 2;
			argv += 2;
		}
		else if (argv[0][0] == '-')
		{
			int bad = 1;		// assumption
			// check to see if this is a substitute value
			char *arg = &argv[0][1];
			for(SubValue_t *sub = SubValues; sub->from; ++sub)
			{
				char *from = SubValueGetFrom(sub);
				bad = strcmp(arg, from);
				free(from);
				if ( ! bad )
				{
					if (argc < 2)
						Usage("value must follow -%s", arg);
					++argv;
					argc -= 1;
					sub->to = *argv++;		// new value
					argc -= 1;
					break;				// got it...
				}
			}
			if ( bad )
				Usage("Invalid option: %s", argv[0]);
		}
		else
		{
			Msgs[NbrMsgs].fname = *argv++;
			++NbrMsgs;
			argc -= 1;
		}
	}

	if ( DestinationAddress == NULL || strlen(DestinationAddress) <= 0 )
		Usage("At least option -d is required");
// resolve the destination address
	if ( (Host = gethostbyname(DestinationAddress)) == NULL )
	{
		fprintf(stderr, "Unable to resolve host '%s'\n", CurrentSession->destinationAddress);
		exit(1);
	}

	// gather the message content
	if ( NbrMsgs <= 0 )			// no input files, use stdin
	{
		Msgs[NbrMsgs].fname = "stdin";
		++NbrMsgs;
	}
	for(int i = 0; i < NbrMsgs; ++i)
		GetContent(&Msgs[i], hex);

	Log(1, "Using EndOfMessage=%s\n", GetMnemonicString(Eom, 0));

// spin off any requested children
	for(int f = 1; f < NbrForks; ++f)
	{
		int pid;
		switch((pid = fork()))
		{
			case -1:
				perror("fork");		// failed
				exit(1);
				break;

			case 0:					// parent
				break;

			default:				// child
				break;
		}

		if ( pid )					// a child
			break;					// get going...
	}


	// Initialize the test
	SessionEventAll(InitializeEvent);

	Log(1, "%d sessions poised\n", ActiveSessions);

	SessionEventAll(StartEvent);

// Process the events

	while ( ActiveSessions > 0 )
	{
		CurrentSession = &RootSession;
		fd_set *fds = SessionSelect(WaitTime*1000);
		if ( fds != NULL )
		{
			int ns = 0;
			for(int sn = 0; sn < MaxSessions; ++sn)
			{
				if ( Sessions[sn].fd > 0 && FD_ISSET(Sessions[sn].fd, fds) )
				{
					++ns;				// count it
					CurrentSession = &Sessions[sn];
					SessionEvent(ReadReadyEvent);
					if ( TpsWait() < 0 )
						break;
				}
			}
			if ( ns <= 0 )
				SessionEventAll(TimeoutEvent);
		}
	}

	SessionEventAll(StopEvent);

// report times

	RootSession.timeStopped = GetTime();

	CurrentSession = &RootSession;

	if ( Loops > 0 )
	{
		puts("");
		SessionReport(&RootSession, "");
		RootSession.nbrErrors = 0;

		puts("");
		printf("\n%d sessions reporting:\n", NbrSessions);

		for (int sn = 0; sn < NbrSessions; ++sn)
		{
			SessionReport(&Sessions[sn], "    ");
			puts("");
		}

		if ( RootSession.nbrErrors > 0 )
			Log(0, "*** %d: Errors\n", RootSession.nbrErrors);
	}

	return 0;
}
