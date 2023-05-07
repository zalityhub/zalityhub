/*****************************************************************************

Filename:   lib/nx/socklib.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:58 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/socklib.c,v 1.3.4.7 2011/10/27 18:33:58 hbray Exp $
 *
 $Log: socklib.c,v $
 Revision 1.3.4.7  2011/10/27 18:33:58  hbray
 Revision 5.5

 Revision 1.3.4.6  2011/09/24 17:49:47  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/01 14:49:46  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/08/18 18:26:17  hbray
 release 5.5

 Revision 1.3.4.2  2011/08/15 19:12:32  hbray
 5.5 revisions

 Revision 1.3.4.1  2011/08/11 19:47:34  hbray
 Many changes

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:48  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: socklib.c,v 1.3.4.7 2011/10/27 18:33:58 hbray Exp $ "


#include "include/stdapp.h"
#include "include/datagram.h"
#include "include/libnx.h"
#include "include/fifo.h"
#include "include/nxregex.h"

#include <libgen.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>



/********************************************************************
	Local/Static definitions
********************************************************************/

#define TCP_HDR_SIZE			8	// #LLLLLLL
#define MaxListenBacklog		1024
#define HAVE_MSGHDR_MSG_CONTROL	1

static const int	MaxPktDelay		= 5000;

static int			TcpPortOffset	= 0;

static HashMap_t	*WatchAddrMap = NULL;
static HashMap_t	*WatchPortMap = NULL;
static HashMap_t	*WatchTextMap = NULL;


// TCP/IP Support Functions

static int SockSendRawStream(EventFile_t *this, char *bfr, int len, int flags);
static int SockSendRawGram(EventFile_t *this, char *bfr, int len, int flags);
static int _SockDgramConnect(EventFile_t *this, char *destname, int port, int domain, int type);
static long SockSetSocketBufferSize(EventFile_t *this, long maxPktLen);

static inline char* GetNatName(char *name, boolean *nated) { Pair_t *nat; if ( (nat = (Pair_t*)HashFindStringVar(NxGlobal->natList, name)) != NULL ) {name = (char*)nat->second; if (nated) *nated = true;} else if (nated) *nated = false;  return name; }

static boolean SockTrace(EventFile_t *this, char *caption, boolean verbose, char *bfr, int len);
static CommandResult_t _SockWatchCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response, HashMap_t *map, char *target);



/*
	*	connection names are:
		<ty> :u: <uid> :f: <fd> :s: <serviceIpAddr> :p: <peerIpAddr>

		types are:
			l	-	a Listen
			L	-	a Datagram Listen
			s	-	an Accepted socket (server side)
			S	-	an Accepted datagram socket (server side)
			c	-	a Connected socket (client side)
			C	-	a Connected datagram socket (client side)
			p	-	one side of a pipe
			p	-	the other side of a pipe
*/



/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

int
SockInit()
{

	TcpPortOffset = NxGetPropertyIntValue("Platform.TcpPortOffset");

	if (TcpPortOffset != 0)
		SysLog(LogWarn, "Using TcpPortOffset of %d", TcpPortOffset);

	return 0;
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
SockListen(EventFile_t *this, char *hostname, int port, int domain, int type)
{

	EventFileVerify(this);

	if (this->isOpen)
	{
		if (SockClose(this) != 0)
			SysLog(LogError, "SockClose failed: {%s}", EventFileToString(this));
	}

	hostname = GetNatName(hostname, NULL);
	this->domain = domain;
	this->type = type;

	if ((this->fd = socket(this->domain, type, 0)) < 0)
	{
		int sockErr = errno;
		SysLog(LogError, "Socket open error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;				// driver error
	}
	NxGlobal->efg->evfSet[this->fd] = this;		// in use

	this->isOpen = true;
	this->counts.outPkts = this->counts.outChrs = this->counts.inPkts = this->counts.inChrs = 0;

	this->peerPort = 0;
	sprintf(this->peerPortString, "%d", this->peerPort);
	memset(this->peerIpAddr, 0, sizeof(this->peerIpAddr));
	strcpy(this->peerIpAddrString, IpAddrToString(this->peerIpAddr));

	this->servicePort = port;
	sprintf(this->servicePortString, "%d", this->servicePort);
	memset(this->serviceIpAddr, 0, sizeof(this->serviceIpAddr));
	strcpy(this->serviceIpAddrString, IpAddrToString(this->serviceIpAddr));

	struct sockaddr *saddr = NULL;
	socklen_t saddrlen = 0;

	switch (this->domain)
	{
	default:
		SysLog(LogError, "%d is not a valid socket domain: {%s}", this->domain, EventFileToString(this));
		SockClose(this);
		return -1;
		break;

	case AF_INET:
		{
			static struct sockaddr_in inetaddr;

			memset(&inetaddr, 0, sizeof(inetaddr));
			inetaddr.sin_family = this->domain;
			inetaddr.sin_port = htons((u_short) (this->servicePort + TcpPortOffset));
			inet_aton(hostname, &inetaddr.sin_addr);
			saddr = (struct sockaddr *)&inetaddr;
			saddrlen = sizeof(inetaddr);

			memcpy(this->serviceIpAddr, &inetaddr.sin_addr, sizeof(this->serviceIpAddr));
			strcpy(this->serviceIpAddrString, IpAddrToString(this->serviceIpAddr));
			sprintf(this->name, "Listen(%s.%s)", EventFileUidString(this), this->servicePortString);

#if 1
			if ( SockSetSocketOptions(this, 8192) != 0 )
			{
					int sockErr = errno;
					SysLog(LogError, "SockSetSocketOptions failed");
					errno = sockErr;
			}
#else
			{ // Enable address reuse
				int on = 1;
				if (setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0)
				{
					int sockErr = errno;
					SysLog(LogError, "setsockopt SO_REUSEADDR failed; error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
					errno = sockErr;
				}
			}
#endif
			break;
		}

	case AF_UNIX:
		{
#ifndef __CYGWIN__
			{					// first thing; if the file entry for this socket is not locked; remove the file
				if (access(hostname, F_OK) == 0 && lockf(this->fd, F_TEST, 0) == 0)
				{
					if (unlink(hostname) != 0)
					{
						SysLog(LogError, "Unable to remove previous AF_UNIX entry %s; error=%s: {%s}", hostname, ErrnoToString(errno), EventFileToString(this));
						SockClose(this);
						return -1;
					}
				}
			}
#else
			if (unlink(hostname) != 0)
			{
				SysLog(LogError, "Unable to remove previous AF_UNIX entry %s; error=%s: {%s}", hostname, ErrnoToString(errno), EventFileToString(this));
				SockClose(this);
				return -1;
			}
#endif
			static struct sockaddr_un nameaddr;

			memset(&nameaddr, 0, sizeof(nameaddr));
			nameaddr.sun_family = this->domain;
			strncpy(nameaddr.sun_path, hostname, sizeof(nameaddr.sun_path) - 1);
			saddr = (struct sockaddr *)&nameaddr;
			saddrlen = sizeof(nameaddr);

			{	// build name
				char *tmp = strrchr(hostname, '/');
				if ( tmp == NULL )
					tmp = hostname;	// no final slash; use as is
				if ( tmp[0] == '/' )
					++tmp;		// skip leading slash
				strncpy(this->serviceIpAddrString, tmp, sizeof(this->serviceIpAddrString) - 1);
				sprintf(this->name, "Listen(%s.%s)", EventFileUidString(this), this->serviceIpAddrString);
				char *bn = strdup(tmp);
				strcpy(this->servicePortString, basename(bn));
				free(bn);
			}
			break;
		}
	}

	if (bind(this->fd, saddr, saddrlen) < 0)
	{
		int sockErr = errno;
		SysLog(LogError, "listen() error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
		SockClose(this);
		errno = sockErr;
		return -1;
	}

// If unix domain; change file permissions
	if ( this->domain == AF_UNIX )
	{
		struct sockaddr_un *nameaddr = (struct sockaddr_un*)saddr;
		if (chmod(nameaddr->sun_path, 0666) < 0)
			SysLog(LogWarn, "chmod() error=%s: {%s}", ErrnoToString(errno), EventFileToString(this));
	}

	if (type != SOCK_DGRAM)		// don't listen for datagrams
	{
		if (listen(this->fd, MaxListenBacklog) != 0)
		{
			int sockErr = errno;
			SysLog(LogError, "listen error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
			SockClose(this);
			errno = sockErr;
			return -1;
		}
	}

	SockTrace(this, "Listen", true, NULL, 0);
	this->msTimeConnected = GetMsTime();
	SysLog(LogDebug, "Listen active: %s", EventFileToString(this));
	return 0;
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
SockAccept(EventFile_t *this, EventFile_t *socket)
{

	EventFileVerify(this);
	EventFileVerify(socket);

	if (!this->isOpen)
	{
		SysLog(LogDebug, "Listen is not active");
		return -1;
	}

	if (socket->isOpen)
	{
		if (SockClose(socket) != 0)
			SysLog(LogError, "SockClose failed: {%s}", EventFileToString(socket));
	}

	struct sockaddr_in saddr;

	memset(&saddr, 0, sizeof(saddr));
	socklen_t saddrlen = sizeof(saddr);

	if ((socket->fd = accept(this->fd, (struct sockaddr *)&saddr, &saddrlen)) <= 0)
	{
		int sockErr = errno;
		if (sockErr == EAGAIN || sockErr == EINPROGRESS)
		{
			errno = sockErr;
			return 0;		// in progress
		}

		SysLog(LogError, "accept error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;				// hard error
	}
	NxGlobal->efg->evfSet[socket->fd] = socket;		// in use

// socket is open
	socket->isOpen = true;
	socket->isConnected = true;
	socket->msTimeConnected = GetMsTime();
	socket->domain = this->domain;
	socket->type = this->type;
	socket->counts.outPkts = socket->counts.outChrs = socket->counts.inPkts = socket->counts.inChrs = 0;

	socket->peerPort = ntohs(saddr.sin_port);
	sprintf(socket->peerPortString, "%d", socket->peerPort);
	memcpy(socket->peerIpAddr, &saddr.sin_addr, sizeof(socket->peerIpAddr));
	strcpy(socket->peerIpAddrString, IpAddrToString(socket->peerIpAddr));
	boolean nated = false;
	strcpy(socket->peerIpAddrString, GetNatName(IpAddrToString(socket->peerIpAddr), &nated));
	// now get the nat'ed, if any...
	if ( nated )
	{
		inet_aton(socket->peerIpAddrString, &saddr.sin_addr);
		memcpy(socket->peerIpAddr, &saddr.sin_addr, sizeof(socket->peerIpAddr));
		strcpy(socket->peerIpAddrString, IpAddrToString(socket->peerIpAddr));
	}

	socket->servicePort = this->servicePort;
	strcpy(socket->servicePortString, this->servicePortString);
	memcpy(socket->serviceIpAddr, this->serviceIpAddr, sizeof(socket->serviceIpAddr));
	if (socket->domain == AF_UNIX)
		memcpy(socket->serviceIpAddrString, this->serviceIpAddrString, sizeof(socket->serviceIpAddrString));
	else
		strcpy(socket->serviceIpAddrString, IpAddrToString(socket->serviceIpAddr));

	sprintf(socket->name, "Server(%s.%s)", EventFileUidString(socket), socket->servicePortString);

// Check for Log suppression
	if (NxIgnoreThisIp(socket->peerIpAddr))
		SysLogSetLevel(LogWarn);

	SockTrace(this, "Accept", false, NULL, 0);
	SockTrace(socket, "Connected", true, NULL, 0);
	SysLog(LogDebug, "accepted socket %s", EventFileToString(socket));
	return 0;
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on pending, 0 for connected, else returns -1 on error
-*******************************************************************/

int
SockConnect(EventFile_t *this, char *destname, int port, int domain, int type)
{
	EventFileVerify(this);

	if (this->isConnected )
	{
		SysLog(LogError, "already connected: {%s}", EventFileToString(this));
		return -1;
	}

	destname = GetNatName(destname, NULL);

	if (!this->isOpen)			// not open
	{
		if (type == SOCK_DGRAM)
			return _SockDgramConnect(this, destname, port, domain, type);

		if (type != SOCK_STREAM)
		{
			SysLog(LogError, "%d is not a valid socket type: {%s}", type, EventFileToString(this));
			SockClose(this);
			return -1;
		}

		this->domain = domain;
		this->type = type;

		if ((this->fd = socket(this->domain, type, 0)) < 0)
		{
			int sockErr = errno;
			SysLog(LogError, "Socket open error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;			// driver error
		}
		NxGlobal->efg->evfSet[this->fd] = this;		// in use

		this->isOpen = true;
		this->counts.outPkts = this->counts.outChrs = this->counts.inPkts = this->counts.inChrs = 0;

		this->peerPort = port;
		sprintf(this->peerPortString, "%d", this->peerPort);
		memset(this->peerIpAddr, 0, sizeof(this->peerIpAddr));
		strcpy(this->peerIpAddrString, destname);

		this->servicePort = 0;	// don't know this yet
		sprintf(this->servicePortString, "%d", this->servicePort);
		memset(this->serviceIpAddr, 0, sizeof(this->serviceIpAddr));	// nor this
		strcpy(this->serviceIpAddrString, IpAddrToString(this->serviceIpAddr));

		sprintf(this->name, "Client(%s.%s.%s)", EventFileUidString(this), this->peerIpAddrString, this->peerPortString);

	// build server's address
		struct sockaddr *saddr = NULL;
		int saddrlen = 0;

		switch (this->domain)
		{
		default:
			SysLog(LogError, "%d is not a valid socket domain: {%s}", this->domain, EventFileToString(this));
			SockClose(this);
			return -1;
			break;

		case AF_INET:
			{
				static struct sockaddr_in inetaddr;

				memset(&inetaddr, 0, sizeof(inetaddr));
				inetaddr.sin_family = this->domain;
				inetaddr.sin_port = htons((u_short) (this->peerPort + TcpPortOffset));
				inetaddr.sin_addr.s_addr = inet_addr(destname);
				saddr = (struct sockaddr *)&inetaddr;
				saddrlen = sizeof(inetaddr);

				memcpy(this->peerIpAddr, &inetaddr.sin_addr, sizeof(this->peerIpAddr));
				strcpy(this->peerIpAddrString, IpAddrToString(this->peerIpAddr));
				sprintf(this->name, "Client(%s.%s.%s)", EventFileUidString(this), this->peerIpAddrString, this->peerPortString);
				SysLog(LogDebug, "connecting %s", EventFileToString(this));
				break;
			}

		case AF_UNIX:
			{
				static struct sockaddr_un nameaddr;

				memset(&nameaddr, 0, sizeof(nameaddr));
				nameaddr.sun_family = this->domain;
				strncpy(nameaddr.sun_path, destname, sizeof(nameaddr.sun_path) - 1);
				saddr = (struct sockaddr *)&nameaddr;
				saddrlen = sizeof(nameaddr);

				strncpy(this->peerIpAddrString, destname, sizeof(this->peerIpAddrString) - 1);

				{	// build name
					char *tmp = strrchr(destname, '/');
					if ( tmp == NULL )
						tmp = destname;	// no final slash; use as is
					if ( tmp[0] == '/' )
						++tmp;		// skip leading slash
					sprintf(this->name, "Client(%s.%s)", EventFileUidString(this), tmp);
				}
				strncpy(this->serviceIpAddrString, destname, sizeof(this->serviceIpAddrString) - 1);
				SysLog(LogDebug, "connecting %s", EventFileToString(this));
				break;
			}
		}

	// initiate the connect
		if (connect(this->fd, saddr, saddrlen) < 0)
		{
			int sockErr = errno;
			if (sockErr != EAGAIN && sockErr != EINPROGRESS)
			{
				SysLog(LogError, "connect error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
				SockClose(this);
				errno = sockErr;
				return -1;
			}
		}

		if ( this->domain == AF_INET )
		{ // rebuild the name
			struct sockaddr_in inetaddr;
			socklen_t inetaddrlen = sizeof(inetaddr);
			int ii = getsockname(this->fd, (struct sockaddr *)&inetaddr, &inetaddrlen);

			if (ii == 0)
			{
				this->servicePort = ntohs(inetaddr.sin_port);
				sprintf(this->servicePortString, "%d", this->servicePort);
				memcpy(this->serviceIpAddr, &inetaddr.sin_addr, sizeof(this->serviceIpAddr));
				strcpy(this->serviceIpAddrString, IpAddrToString(this->serviceIpAddr));

				boolean nated = false;
				strcpy(this->serviceIpAddrString, GetNatName(IpAddrToString(this->serviceIpAddr), &nated));
				// now get the nat'ed, if any...
				if ( nated )
				{
					inet_aton(this->serviceIpAddrString, &inetaddr.sin_addr);
					memcpy(this->serviceIpAddr, &inetaddr.sin_addr, sizeof(this->serviceIpAddr));
					strcpy(this->serviceIpAddrString, IpAddrToString(this->serviceIpAddr));
				}
			}
		}
	}

// check if socket is writeable

	int sw = SockSingleWait(this, EventWriteMask, 0);
	int sockErr = errno;
	switch (sw)
	{
		default:
			SysLog(LogError, "_SockSingleWait error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
			break;
		case 0:
			SysLog(LogDebug, "connect pending %s", EventFileToString(this));
			return 0;		// pending
			break;
		case 1:
			if ( SockIsConnected(this) )
				break;		// fall through
			SysLog(LogError, "Not Connected; error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
			break;
	}

	SockTrace(this, "Connected", true, NULL, 0);
	SysLog(LogDebug, "connect complete %s", EventFileToString(this));
	this->isConnected = true;
	this->msTimeConnected = GetMsTime();
	return 1;
}


boolean
SockIsConnected(EventFile_t *this)
{
	EventFileVerify(this);

	if ( ! this->isOpen )		// not open; can't be connected
		return (this->isConnected = false);

	if ( SockCheckSocket(this) != 0 )
	{
		int sockErr = errno;
		SysLog(LogError, "Not Connected; error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return (this->isConnected = false);
	}
	return (this->isConnected = true);
}



/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on pending, 0 for connected, else returns -1 on error
-*******************************************************************/

static int
_SockDgramConnect(EventFile_t *this, char *destname, int port, int domain, int type)
{
	EventFileVerify(this);

	if (this->isOpen)			// open; must not be...
	{
		SysLog(LogError, "Socket is open. Datagram sockets should not be open: {%s}", EventFileToString(this));
		return -1;
	}

	this->domain = domain;
	this->type = type;

	if ((this->fd = socket(this->domain, type, 0)) < 0)
	{
		int sockErr = errno;
		SysLog(LogError, "Socket open error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;				// driver error
	}
	NxGlobal->efg->evfSet[this->fd] = this;		// in use

	this->isOpen = true;
	this->counts.outPkts = this->counts.outChrs = this->counts.inPkts = this->counts.inChrs = 0;

	this->peerPort = port;
	sprintf(this->peerPortString, "%d", this->peerPort);
	memset(this->peerIpAddr, 0, sizeof(this->peerIpAddr));
	strcpy(this->peerIpAddrString, destname);

	this->servicePort = 0;		// don't know this yet
	sprintf(this->servicePortString, "%d", this->servicePort);
	memset(this->serviceIpAddr, 0, sizeof(this->serviceIpAddr));	// nor this
	strcpy(this->serviceIpAddrString, IpAddrToString(this->serviceIpAddr));

	sprintf(this->name, "Client(%s.%s.%s)", EventFileUidString(this), this->peerIpAddrString, this->peerPortString);

// build server's address
	struct sockaddr *saddr = NULL;
	int saddrlen = 0;

	switch (this->domain)
	{
	default:
		SysLog(LogError, "%d is not a valid socket domain: {%s}", this->domain, EventFileToString(this));
		SockClose(this);
		return -1;
		break;

	case AF_INET:
		{
			static struct sockaddr_in inetaddr;

			memset(&inetaddr, 0, sizeof(inetaddr));
			inetaddr.sin_family = this->domain;
			inetaddr.sin_port = htons((u_short) (this->peerPort + TcpPortOffset));
			inetaddr.sin_addr.s_addr = inet_addr(destname);
			saddr = (struct sockaddr *)&inetaddr;
			saddrlen = sizeof(inetaddr);

// build the peer connection details
			memcpy(this->peerIpAddr, &inetaddr.sin_addr, sizeof(this->peerIpAddr));
			strcpy(this->peerIpAddrString, IpAddrToString(this->peerIpAddr));
			sprintf(this->name, "Client(%s.%s.%s)", EventFileUidString(this), this->peerIpAddrString, this->peerPortString);
			SysLog(LogDebug, "connecting %s", EventFileToString(this));
			break;
		}

	case AF_UNIX:
		{
// build the peer connection details
			memcpy(this->peerIpAddr, destname, sizeof(this->peerIpAddr));
			strcpy(this->peerIpAddrString, IpAddrToString(this->peerIpAddr));
			strncpy(this->peerIpAddrString, destname, sizeof(this->peerIpAddrString) - 1);
			{	// build name
				char *tmp = strrchr(destname, '/');
				if ( tmp == NULL )
					tmp = destname;	// no final slash; use as is
				if ( tmp[0] == '/' )
					++tmp;		// skip leading slash
				sprintf(this->name, "Client(%s.%s)", EventFileUidString(this), tmp);
			}
			SysLog(LogDebug, "connecting %s", EventFileToString(this));
			break;
		}
	}

	SockTrace(this, "Connected", true, NULL, 0);
	return 0;					// success
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on pending, 0 for connected, else returns -1 on error
-*******************************************************************/

int
SockSocketPair(EventFile_t *left, EventFile_t *right, int type)
{
	int pair[2];

	EventFileVerify(left);
	EventFileVerify(right);

	if (socketpair(AF_UNIX, type, 0, pair) != 0)
	{
		int sockErr = errno;
		SysLog(LogError, "socketpair error=%d", sockErr);
		errno = sockErr;
		return -1;
	}

	left->fd = pair[0];
	NxGlobal->efg->evfSet[left->fd] = left;		// in use
	right->fd = pair[1];
	NxGlobal->efg->evfSet[right->fd] = right;		// in use
	left->domain = AF_UNIX;
	left->type = type;
	right->domain = AF_UNIX;
	right->type = type;

// Prepare socket 1
	sprintf(left->name, "Pipe(%s.%s)", EventFileUidString(left), EventFileUidString(right));
	left->isPipe = EventPipeTypeLeft;
	SysLog(LogDebug, "pair[0] opensocket %d", left->fd);
	left->isOpen = true;
	left->isConnected = true;
	left->msTimeConnected = GetMsTime();
	left->counts.outPkts = left->counts.outChrs = left->counts.inPkts = left->counts.inChrs = 0;

// Prepare socket 2
	sprintf(right->name, "Pipe(%s.%s)", EventFileUidString(right), EventFileUidString(left));
	right->isPipe = EventPipeTypeRight;
	SysLog(LogDebug, "pair[1] opensocket %d", right->fd);
	right->isOpen = true;
	right->isConnected = true;
	right->msTimeConnected = GetMsTime();
	right->counts.outPkts = right->counts.outChrs = right->counts.inPkts = right->counts.inChrs = 0;

	return 0;
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
SockDupConnection(EventFile_t *this, EventFile_t *socket)
{

	EventFileVerify(this);
	EventFileVerify(socket);

	if (!this->isOpen)
	{
		SysLog(LogError, "Not open; this is not permitted: {%s}", EventFileToString(this));
		return -1;
	}

	if (socket->isOpen)
	{
		SysLog(LogError, "Is open; this is not permitted: {%s}", EventFileToString(socket));
		return -1;
	}

	SysLog(LogDebug, "Duping %s", EventFileToString(this));

	// Dup the file
	// Pass back the received file descriptor
	if ((socket->fd = dup(this->fd)) < 0)
	{
		int sockErr = errno;
		SysLog(LogError, "dup failed; sockErr=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;
	}
	NxGlobal->efg->evfSet[socket->fd] = socket;		// in use

	socket->isOpen = true;
	socket->type = this->type;
	socket->domain = this->domain;
	socket->isConnected = this->isConnected;
	socket->msTimeConnected = this->msTimeConnected;
	socket->isPipe = this->isPipe;
	socket->peerPort = this->peerPort;
	sprintf(socket->peerPortString, "%d", this->peerPort);
	socket->servicePort = this->servicePort;
	sprintf(socket->servicePortString, "%d", this->servicePort);
	memcpy(socket->peerIpAddr, this->peerIpAddr, sizeof(this->peerIpAddr));
	memcpy(socket->peerIpAddrString, this->peerIpAddrString, sizeof(this->peerIpAddrString));
	memcpy(socket->serviceIpAddr, this->serviceIpAddr, sizeof(this->serviceIpAddr));
	memcpy(socket->serviceIpAddrString, this->serviceIpAddrString, sizeof(this->serviceIpAddrString));

	// Rebuild name (replace the uid and first fd
	{
		char *right;
		if (this->isPipe && (right = strrchr(this->name, '.')) != NULL )
		{
			sprintf(socket->name, "Pipe(%s.%s)", socket->uidString, right+1);
		}
		else
		{
			SysLog(LogError, "%s is an unknown name format", this->name);
			strcpy(socket->name, this->name);	// don't know this format, use as is
		}
	}

	socket->counts.outPkts = socket->counts.outChrs = socket->counts.inPkts = socket->counts.inChrs = 0;

	SysLog(LogDebug, "duped to %s", EventFileToString(socket));

	return 0;
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
SockClose(EventFile_t *this)
{

	EventFileVerify(this);

	if (this->isOpen)			// if is open
	{
		SockTrace(this, "Closed", false, NULL, 0);
		SysLog(LogDebug, "closing %s", EventFileToString(this));

		this->isOpen = false;
		this->isConnected = false;

		if (EventFileDisable(this) != 0)
			SysLog(LogError, "EventFileDisable failed %s", EventFileToString(this));

		this->isConnected = false;
		this->msTimeConnected = 0L;
		if (close(this->fd) < 0)
		{
			int sockErr = errno;
			SysLog(LogError, "close error=%s", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
		}
		if (this->fd >= 0 && this->fd < FD_SETSIZE)
			NxGlobal->efg->evfSet[this->fd] = NULL;			// no longer in use
	}

	this->isOpen = false;
	this->isConnected = false;
	strcpy(this->name, "");

	SockTrace(this, "Closed", false, NULL, 0);
	return 0;
}


// Disables read interrupts; essentially causing input from client to stall..
int
SockShutdown(EventFile_t *this)
{

	EventFileVerify(this);

	SysLog(LogDebug, "shutdown %s", EventFileToString(this));

	this->isShutdown = true;

// turn off the event mask
	if (EventFileDisable(this) != 0)
	{
		SysLog(LogError, "EventFileDisable failed: {%s}", EventFileToString(this));
		SockClose(this);		// slam it...
	}

	SockTrace(this, "Shutdown", false, NULL, 0);
	return 0;
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
SockSendRaw(EventFile_t *this, char *bfr, int len, int flags)
{

	EventFileVerify(this);

// This is for SOCK_DGRAM
	if (this->type == SOCK_DGRAM)
		return SockSendRawGram(this, bfr, len, flags);

	return SockSendRawStream(this, bfr, len, flags);
}


static int
SockSendRawStream(EventFile_t *this, char *bfr, int len, int flags)
{

	EventFileVerify(this);

	int tlen;
	char *obfr = bfr;
	int olen = len;
	int attempts = 0;

	for (tlen = 0; len > 0;)
	{
		int wlen = send(this->fd, bfr, len, flags);

		// if ( attempts > 0 )
			// SysLog(LogWarn, "Retry %d of writing %d; wrote %d: %s", attempts, len, wlen, EventFileToString(this));

		if (wlen <= 0)
		{
			int sockErr = errno;
			if (sockErr != EAGAIN && sockErr != EINPROGRESS)
			{
				SysLog(LogError, "send error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
				errno = sockErr;
				return -1;		// error
			}
			SysLog((sockErr==EAGAIN || sockErr==EINPROGRESS)?LogDebug:LogWarn, "send error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			wlen = 0;			// nothing written
		}

		bfr += wlen;
		len -= wlen;
		tlen += wlen;

		if (len > 0)			// more data to send
		{
			// SysLog(LogWarn, "Waiting %d: %s", MaxPktDelay, EventFileToString(this));
			int n = SockSingleWait(this, EventWriteMask, MaxPktDelay);

			if (n <= 0)
			{
				SysLog(LogError, "Short write. Expected %d, sent %d: {%s}", olen, tlen, EventFileToString(this));
				return -1;
			}
			++attempts;
		}
	}

	SockTrace(this, "Send", false, obfr, tlen);
	this->counts.outChrs += tlen;
	++this->counts.outPkts;
	return tlen;
}


static int
SockSendRawGram(EventFile_t *this, char *bfr, int len, int flags)
{
	EventFileVerify(this);

	struct sockaddr *saddr = NULL;
	socklen_t saddrlen = 0;

	if (this->domain == AF_UNIX)
	{
		static struct sockaddr_un nameaddr;

		memset(&nameaddr, 0, sizeof(nameaddr));
		nameaddr.sun_family = this->domain;
		strncpy(nameaddr.sun_path, this->peerIpAddrString, sizeof(nameaddr.sun_path) - 1);
		saddr = (struct sockaddr *)&nameaddr;
		saddrlen = sizeof(nameaddr);
	}
	else
	{
		static struct sockaddr_in inetaddr;

		memset(&inetaddr, 0, sizeof(inetaddr));
		inetaddr.sin_family = this->domain;
		memcpy(&inetaddr.sin_addr, this->peerIpAddr, sizeof(this->peerIpAddr));
		inetaddr.sin_port = htons((u_short) (this->peerPort + TcpPortOffset));
		saddr = (struct sockaddr *)&inetaddr;
		saddrlen = sizeof(inetaddr);
	}
	int wlen = sendto(this->fd, bfr, len, flags, saddr, saddrlen);

	if (wlen != len)
	{
		int sockErr = errno;
		SysLog(LogError, "sendto error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;			// error
	}

	this->counts.outChrs += len;
	return len;				// send was ok
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
SockRecvRawExact(EventFile_t *this, char *bfr, int len, int flags, int ms)
{

	EventFileVerify(this);

	int tlen;

	time_t xtime = time(NULL);
	xtime += ((ms+500)/1000);     // no more than timeout (ms)

	for (tlen = 0; (len - tlen) > 0;)
	{
		int rlen;
		if ((rlen = SockRecvRaw(this, bfr + tlen, len - tlen, flags)) <= 0)
		{
			int sockErr = errno;
			if (sockErr != EAGAIN && sockErr != EINPROGRESS)
			{
				SysLog(LogError, "SockRecvRaw error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
				errno = sockErr;
				return -1;		// error
			}
			errno = sockErr;
			rlen = 0;			// nothing read
		}

		tlen += rlen;

		if ( ms > 0 && (len - tlen) > 0)	// more data required
		{
			int n = SockSingleWait(this, EventReadMask, ms);

			if (n <= 0)
			{
				SysLog(LogError, "Short read. Expected %d, received %d: {%s}", len, tlen, EventFileToString(this));
				return -1;
			}
		}

		if ( (len - tlen) > 0 && (ms == 0 || time(NULL) > xtime) )
		{
			SysLog(LogError, "Exceeded timeout %d", ms);
			break;
		}
	}

	return tlen;
}


int
SockRecvRaw(EventFile_t *this, char *bfr, int len, int flags)
{
	EventFileVerify(this);

	if (len <= 0)
		return 0;				// false alarm, does not want any data!

	int rlen;

	if ((rlen = recv(this->fd, bfr, len, flags)) <= 0)
	{
		int sockErr = errno;

		SysLog(LogDebug, "recv %s error %s", EventFileToString(this), ErrnoToString(sockErr));
		if (rlen <= 0 && (sockErr == ECONNRESET || sockErr == ENOTCONN || sockErr == ENOENT || sockErr == EPIPE))
		{
			SysLog(LogDebug, "translating error %d to disconnect for %s", sockErr, EventFileToString(this));
			sockErr = errno = rlen = 0;	// all these are valid disconnects...
		}

		if (rlen == 0 && sockErr == 0)
		{
			SysLog(LogDebug, "disconnected %s", EventFileToString(this));
			errno = sockErr;
			return 0;			// disconnect
		}
		if (sockErr != EAGAIN && sockErr != EINPROGRESS)
		{
			SysLog(LogError, "recv error %d(%s): {%s}", sockErr, ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;			// error
		}
		rlen = 0;				// nothing read
	}

	SockTrace(this, "Recv", false, bfr, rlen);
	this->counts.inChrs += rlen;
	++this->counts.inPkts;
	return rlen;
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
SockSendPkt(EventFile_t *this, char *pkt, int pktLen, int flags)
{

	EventFileVerify(this);

	int wlen = (pktLen + TCP_HDR_SIZE);
	char *tmpBfr = alloca(wlen + 10);

// format the header, then append the data

	sprintf(tmpBfr, "#%0*d", TCP_HDR_SIZE - 1, pktLen);
	memcpy(&tmpBfr[TCP_HDR_SIZE], pkt, pktLen);

// send the packet

	int len;
	if ((len = SockSendRaw(this, tmpBfr, wlen, flags)) != wlen)
	{
		SysLog(LogError, "SockSendRaw failure; expected to send %d; actual sent is %d: {%s}", wlen, len, EventFileToString(this));
		return -1;
	}

	return pktLen;
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
SockRecvPkt(EventFile_t *this, char *pkt, int pktLen, int flags)
{

	EventFileVerify(this);

	if (this->pollMask != EventNoMask)
	{
		unsigned long nBytes;

		if (ioctl(this->fd, FIONREAD, &nBytes) != 0)	// get nbr bytes in bfr
		{
			int sockErr = errno;
			SysLog(LogError, "ioctl error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
		}
		if (nBytes < TCP_HDR_SIZE)
			return 0;			// not enough for hdr, no error/no data
	}

// read header
	char hdr[TCP_HDR_SIZE + 1];

	memset(hdr, 0, sizeof(hdr));
	int rlen = SockRecvRawExact(this, hdr, TCP_HDR_SIZE, flags, MaxPktDelay);

	if (rlen <= 0)
		return -1;				// already logged error in SockRecvRaw

	int plen = atoi(&hdr[1]);	// the pkt length

// verify hdr chr is present and pkt is not too long (or short)
	if (hdr[0] != '#' || plen < 0 || plen > pktLen || rlen < TCP_HDR_SIZE)
	{
		SysLog(LogError, "Detected an invalid header [%s]: {%s}", hdr, EventFileToString(this));
		return -1;				// skip
	}

	rlen = SockRecvRawExact(this, pkt, plen, flags, MaxPktDelay);
	if (rlen != plen)
	{
		SysLog(LogError, "Truncated read. Expected %d, received %d: {%s}", plen, rlen, EventFileToString(this));
		return -1;				// skip
	}

	return rlen;
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
SockBytesReady(EventFile_t *this)
{
	unsigned long rlen;

	EventFileVerify(this);

	if (ioctl(this->fd, FIONREAD, &rlen) != 0)
	{
		int sockErr = errno;
		SysLog(LogDebug, "ioctl %s error %s", EventFileToString(this), ErrnoToString(sockErr));
		errno = sockErr;
		return -1;
	}

	return rlen;
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
SockSingleWait(EventFile_t *this, EventPollMask_t pollMask, int ms)
{
	EventFileVerify(this);

	struct timeval timeout;
	fd_set readFds;
	fd_set writeFds;
	fd_set exceptionFds;
	int n = 0;


	// clear the fd sets
	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_ZERO(&exceptionFds);

	if (pollMask & EventReadMask)
	{
		FD_SET(this->fd, &readFds);
		n = max(n, this->fd);
	}
	if (pollMask & EventWriteMask)
	{
		FD_SET(this->fd, &writeFds);
		n = max(n, this->fd);
	}

	if (n == 0)
	{
		SysLog(LogError, "%s is not a valid event mask: {%s}", EventPollMaskToString(pollMask), EventFileToString(this));
		return -1;
	}

	timeout.tv_sec = ms / 1000;
	timeout.tv_usec = 0;
	n = select(n + 1, &readFds, &writeFds, &exceptionFds, ms == -1 ? NULL : &timeout);	// timeout or ready
	return n;
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
SockSetWait(EventFile_t *this)
{
	EventFileVerify(this);

	// set non-blocking
	static u_long off = 0;
	if (ioctl(this->fd, FIONBIO, &off) != 0)
	{
		int sockErr = errno;
		SysLog(LogError, "ioctl error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;
	}

	return 0;
}


static ssize_t
write_fd(int fd, void *ptr, size_t nbytes, int sendfd)
{
    struct msghdr   msg;
    struct iovec    iov[1];

#ifdef  HAVE_MSGHDR_MSG_CONTROL
    union {
      struct cmsghdr    cm;
      char              control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr  *cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((int *) CMSG_DATA(cmptr)) = sendfd;
#else
    msg.msg_accrights = (caddr_t) &sendfd;
    msg.msg_accrightslen = sizeof(int);
#endif

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    return(sendmsg(fd, &msg, 0));
}
/* end write_fd */


int
SockSendConnection(EventFile_t *this, EventFile_t *socket)
{
	EventFileVerify(this);
	EventFileVerify(socket);

	if (!socket->isOpen)
	{
		SysLog(LogError, "Connection does not appear to be open; this is not permitted: {%s}", EventFileToString(socket));
		return -1;
	}

	SysLog(LogDebug, "Sending connection %s via %s", EventFileToString(socket), EventFileToString(this));

	if ( write_fd(this->fd, socket, sizeof(*socket), socket->fd) != sizeof(*socket) )
	{
		int sockErr = errno;
		SysLog(LogFatal, "sendmsg failed errno=%s: {%s} -> {%s}", ErrnoToString(sockErr), EventFileToString(socket), EventFileToString(this));
		errno = sockErr;
		return -1;
	}

	if (SockClose(socket) != 0)
	{
		SysLog(LogError, "SockClose failed: {%s}", EventFileToString(socket));
		return -1;
	}

	SockTrace(socket, "Forwarded", false, NULL, 0);
	return 0;
}


static ssize_t
read_fd(int fd, void *ptr, size_t nbytes, int *recvfd)
{
    struct msghdr   msg;
    struct iovec    iov[1];
    ssize_t         n;

#ifdef  HAVE_MSGHDR_MSG_CONTROL
    union {
      struct cmsghdr    cm;
      char              control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr  *cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);
#else
    int             newfd;
    msg.msg_accrights = (caddr_t) &newfd;
    msg.msg_accrightslen = sizeof(int);
#endif

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    if ( (n = recvmsg(fd, &msg, 0)) <= 0)
        return(n);

#ifdef  HAVE_MSGHDR_MSG_CONTROL
    if ( (cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
        cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
        if (cmptr->cmsg_level != SOL_SOCKET)
		{
			SysLog(LogFatal, "control level != SOL_SOCKET; %d != %d", cmptr->cmsg_level, SOL_SOCKET);
			return -1;
		}
        if (cmptr->cmsg_type != SCM_RIGHTS)
		{
			SysLog(LogFatal, "control type != SCM_RIGHTS; %d != %d", cmptr->cmsg_type, SCM_RIGHTS);
			return -1;
		}
        *recvfd = *((int *) CMSG_DATA(cmptr));
    } else
        *recvfd = -1;       /* descriptor was not passed */
#else
/* *INDENT-OFF* */
    if (msg.msg_accrightslen == sizeof(int))
        *recvfd = newfd;
    else
        *recvfd = -1;       /* descriptor was not passed */
/* *INDENT-ON* */
#endif

    return(n);
}
/* end read_fd */


int
SockRecvConnection(EventFile_t *this, EventFile_t *socket)
{

	EventFileVerify(this);
	EventFileVerify(socket);

	if (socket->isOpen)
	{
		SysLog(LogError, "Not open; this is not permitted: {%s}", EventFileToString(socket));
		return -1;
	}

	EventFile_t rcvdEvf;
	int			recvfd;
	int			rlen;

	if ( (rlen = read_fd(this->fd, &rcvdEvf, sizeof(rcvdEvf), &recvfd)) != sizeof(rcvdEvf) )
	{
		int sockErr = errno;
		if ( (rlen == 0 && sockErr == 0) || sockErr == EAGAIN )
		{
			SysLog(LogDebug, "nothing available %s", EventFileToString(this));
			return 1;		// nothing available...
		}
		SysLog(LogFatal, "read_fd failed errno=%s: from {%s}", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;
	}

	if ( recvfd < 0 || recvfd > FD_SETSIZE )
	{
		SysLog(LogFatal, "fd %d is out of range: {%s}", recvfd, EventFileToString(this));
		errno = EAGAIN;
		return -1;
	}

	rcvdEvf.fd = recvfd;	// set local fd...

	// Rebuild the file

	socket->createdByPid = rcvdEvf.createdByPid;
	socket->isOpen = true;
	socket->isConnected = rcvdEvf.isConnected;
	socket->msTimeConnected = rcvdEvf.msTimeConnected;
	socket->fd = rcvdEvf.fd;
	NxGlobal->efg->evfSet[socket->fd] = socket;		// in use
	socket->uid = rcvdEvf.uid;
	memcpy(socket->uidString, rcvdEvf.uidString, sizeof(socket->uidString));
	socket->domain = rcvdEvf.domain;
	socket->type = rcvdEvf.type;
	socket->isPipe = rcvdEvf.isPipe;

	socket->servicePort = rcvdEvf.servicePort;
	memcpy(socket->servicePortString, rcvdEvf.servicePortString, sizeof(socket->servicePortString));
	memcpy(socket->serviceIpAddr, rcvdEvf.serviceIpAddr, sizeof(socket->serviceIpAddr));
	memcpy(socket->serviceIpAddrString, rcvdEvf.serviceIpAddrString, sizeof(socket->serviceIpAddrString));

	socket->peerPort = rcvdEvf.peerPort;
	memcpy(socket->peerPortString, rcvdEvf.peerPortString, sizeof(socket->peerPortString));
	memcpy(socket->peerIpAddr, rcvdEvf.peerIpAddr, sizeof(socket->peerIpAddr));
	memcpy(socket->peerIpAddrString, rcvdEvf.peerIpAddrString, sizeof(socket->peerIpAddrString));

	strcpy(socket->name, rcvdEvf.name);	// TODO: Probably need to work on this

	socket->counts.outPkts = socket->counts.outChrs = socket->counts.inPkts = socket->counts.inChrs = 0;

	// Check for Log suppression
	if (NxIgnoreThisIp(socket->peerIpAddr))
		SysLogSetLevel(LogWarn);

	SysLog(LogDebug, "received opensocket %s", EventFileToString(socket));

	SockTrace(socket, "Connected", true, NULL, 0);
	return (0);
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function validates the socket status

	Diagnostics:
		Returns 0 on success, else returns errno
-*******************************************************************/

int
SockCheckSocket(EventFile_t *this)
{
	EventFileVerify(this);

	int sOpt = 0;
	unsigned int sOptLen = sizeof(sOpt);
	errno = 0;
	if ( getsockopt(this->fd, SOL_SOCKET, SO_ERROR, (void*)&sOpt, &sOptLen) == 0 )
	{
		// Socket fd is ok, check for error
		if (sOpt != 0)
			errno = sOpt;
	}

	if ( errno != 0 )
		SysLog(LogWarn, "SO_ERROR=%s: {%s}", ErrnoToString(errno), EventFileToString(this));
	return errno;
}


#if 1
/* Set common socket options */
int
SockSetSocketOptions(EventFile_t *this, int maxPktLen)
{

	EventFileVerify(this);

	{
		int tos = 1;

		if ( setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SysLog(LogError, "setsockopt SO_REUSEADDR failed; error=%s: {%s}", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
		}
	}	/* Allow address bound to socket be reused */

	{	/* Only LINGER half a second a on socket close */
		struct linger tos = {0, 50}; /* centiseconds */

		if ( setsockopt(this->fd, SOL_SOCKET, SO_LINGER, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SysLog(LogError, "setsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
		}
	}	/* Only LINGER half a second a on socket close */

	{	/* we use small packets, set lowest posible network delay */
		int tos = IPTOS_LOWDELAY;

		if ( setsockopt(this->fd, SOL_IP, IP_TOS, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SysLog(LogError, "setsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
		}
	}	/* we use small packets, so set lowest posible network delay */

	{	/* we use small packets, set lowest posible network delay */
		int tos = 1;

		if ( setsockopt(this->fd, IPPROTO_TCP, TCP_NODELAY, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SysLog(LogError, "setsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
		}
	}	/* we use small packets, so set lowest posible network delay */

	{	/* we use small packets, set lowest posible network delay */
		struct timeval tos = {0, 25 * 1000000}; /* 250 milliseconds */

		if ( setsockopt(this->fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SysLog(LogError, "setsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
		}
	}	/* we use small packets, so set lowest posible network delay */

	{	/* we use small packets, set lowest posible network delay */
		struct timeval tos = {0, 25 * 1000000}; /* 250 milliseconds */

		if ( setsockopt(this->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SysLog(LogError, "setsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
		}
	}	/* we use small packets, so set lowest posible network delay */

	{	/* Receive out-of-band data in line */
		int tos = 1;

		if ( setsockopt(this->fd, SOL_SOCKET, SO_OOBINLINE, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SysLog(LogError, "setsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
		}
	}	/* Receive out-of-band data in line */

    if ( SockSetSocketBufferSize(this, maxPktLen) != maxPktLen )
    {
		int sockErr = errno;
		SysLog(LogError, "SockSetSocketBufferSize failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;
    }

#if 1
	{	/* Reset receive / send low water marks */
		int tos = 0;

		if ( setsockopt(this->fd, SOL_SOCKET, SO_RCVLOWAT, (char *)&tos, sizeof(tos)) != 0 )
		{
			int sockErr = errno;
			SysLog(LogError, "setsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
		}
#if 0		// Does not work on Linux < 2.4
		if ( setsockopt(this->fd, SOL_SOCKET, SO_SNDLOWAT, (char *)&tos, sizeof(tos)) )
		{
			int sockErr = errno;
			SysLog(LogError, "setsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
			errno = sockErr;
			return -1;
		}
#endif
	}	/* Reset receive / send low water marks */
#endif

	return 0;
}


static long
SockSetSocketBufferSize(EventFile_t *this, long maxPktLen)
{
	unsigned int	rcv = 0;
	unsigned int	snd = 0;
	unsigned int	tos = maxPktLen;
	socklen_t		len;

	EventFileVerify(this);
	if ( setsockopt(this->fd, SOL_SOCKET, SO_RCVBUF, (char *)&tos, sizeof(tos)) != 0 )
	{
		int sockErr = errno;
		SysLog(LogError, "setsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;
	}
	len = sizeof(rcv);
	if ( getsockopt(this->fd, SOL_SOCKET, SO_RCVBUF, (char *)&rcv, &len) != 0 )
	{
		int sockErr = errno;
		SysLog(LogError, "getsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;
	}

	if ( setsockopt(this->fd, SOL_SOCKET, SO_SNDBUF, (char *)&tos, sizeof(tos)) != 0 )
	{
		int sockErr = errno;
		SysLog(LogError, "setsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;
	}
	len = sizeof(snd);
	if ( getsockopt(this->fd, SOL_SOCKET, SO_SNDBUF, (char *)&snd, &len) != 0 )
	{
		int sockErr = errno;
		SysLog(LogError, "getsockopt failed; error=%s (%s)", ErrnoToString(sockErr), EventFileToString(this));
		errno = sockErr;
		return -1;
	}

	if (rcv < maxPktLen)
		return((long)rcv);

	if (snd < maxPktLen)
		return((long)snd);

	return(maxPktLen);
}
#endif


static boolean
SockExecuteTrace(HashMap_t *this, EventFile_t *evf, char *target, char *caption, boolean verbose, char *bfr, int len)
{
	static int recursion = 0;

	if ( recursion > 0 )
		return true;
	++recursion;

	EventFileVerify(evf);

	for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(this, entry)) != NULL;)
	{
		NxRegex_t *regex = (NxRegex_t*)entry->var;
		if ( NxRegexExecute(regex, target) == 0 )
		{
			StringNewStatic(hdr, 512);

			// build message header
			{ // format hdr
				static char *hdrfmt =	"%s "				// date/time
										"%d "				// Pid
										"WATCH "			// 'Psuedo' LogLevel
										"%s "				// ProcName
										"%s:%d ";			// FileName and LineNumber

				StringSprintf(hdr, hdrfmt,
										SysLogTimeToString(),
										getpid(),
										NxCurrentProc->name,
										__FILE__,
										__LINE__);
			}

			String_t *dmp = NULL;
			if ( bfr != NULL && len > 0 )
			{
				if ( len > 8192)
					len = 8192;	// no more than 8k
				dmp = StringDump(NULL, bfr, len, 0);
				StringCat(dmp, "\n");
			}
			StringNewStatic(output, 10240);
			if ( verbose )
				StringSprintf(output, "%s %s %s\n%s%s", hdr->str, caption, EventFileToString(evf), (dmp!=NULL)?dmp->str:"", SysLogGlobal->doublespace?"\n":"");
			else
				StringSprintf(output, "%s %s %s\n%s%s", hdr->str, caption, evf->name, (dmp!=NULL)?dmp->str:"", SysLogGlobal->doublespace?"\n":"");

			char *traceLogAddr = NxGetPropertyValue("TraceLog.Addr");
			int traceLogPort = NxGetPropertyIntValue("TraceLog.Port");
			if ( traceLogAddr != NULL && traceLogPort > 0 )
				NxDatagramSend(traceLogAddr, traceLogPort, output->str, strlen(output->str));

			SysLogPrintf(LogAny, "%s", output->str);
			--recursion;
			return true;
		}
	}

	--recursion;
	return false;
}


static boolean
SockTrace(EventFile_t *this, char *caption, boolean verbose, char *bfr, int len)
{
	EventFileVerify(this);
	boolean done = false;

	if ( WatchAddrMap != NULL && WatchAddrMap->length > 0 )
		done = SockExecuteTrace(WatchAddrMap, this, this->peerIpAddrString, caption, verbose, bfr, len);
	else if ( (!done) && WatchPortMap != NULL && WatchPortMap->length > 0 )
		done = SockExecuteTrace(WatchPortMap, this, this->servicePortString, caption, verbose, bfr, len);
	else if ( (!done) && WatchTextMap != NULL && WatchTextMap->length > 0 && bfr != NULL && len > 0 )
	{
		char *tmp = alloca(len+16);
		memcpy(tmp, bfr, len);
		tmp[len] = '\0';
		done = SockExecuteTrace(WatchTextMap, this, tmp, caption, verbose, bfr, len);
	}
	return done;
}


static CommandResult_t
_SockWatchCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response, HashMap_t *map, char *target)
{
	CommandVerify(this);

	CommandResult_t result = CommandOk;

	char *word = ParserGetNextToken(parser, " ");
	if ( strlen(word) <= 0 )		// no option given, show current list
	{
		ObjectList_t* list = HashGetOrderedList(map, ObjectListStringType);
		if ( list->length > 0 )
		{
			StringSprintfCat(response, "Watch for %s:\n", target);
			for ( HashEntry_t *entry = NULL; (entry = ObjectListRemove(list, ObjectListFirstPosition)); )
			{
				StringSprintfCat(response, "\t%s\n", entry->string);
				HashEntryDelete(entry);
			}
			ObjectListDelete(list);
		}
		else
		{
			StringSprintfCat(response, "No trace enabled");
		}
	}
	else
	{
		StringSprintfCat(response, "Watch for %s:\n", target);
		do
		{
			if ( word[0] == '-' )
			{
				++word;		// skip '-'
				NxRegex_t *regex = (NxRegex_t*)HashFindStringVar(map, word);
				if ( regex == NULL )
				{
					StringSprintfCat(response, "\t%s not found; did not disable", word);
				}
				else
				{
					NxRegexDelete(regex);
					HashDeleteString(map, word);
					StringSprintfCat(response, "\t%s watch disabled", word);
				}
			}
			else
			{
				NxRegex_t *regex = NxRegexNew(NULL);
				if(NxRegexCompile(regex, word, response) != 0 )
				{
					SysLog(LogError, "Unable to compile expression %s; error %s", word, response->str);
					return CommandBadOption;
				}
				HashAddString(map, word, regex);
				StringSprintfCat(response, "\t%s", word);
			}
			word = ParserGetNextToken(parser, " ");
		} while (word != NULL && strlen(word) > 0);
	}

	return result;
}


CommandResult_t
SockWatchCmd(Command_t *this, Parser_t *parser, char *prior, String_t *response)
{
	CommandVerify(this);

	// allocate filter maps if not present

	if ( WatchAddrMap == NULL )
		WatchAddrMap = HashMapNew(128, "WatchAddrMap");
	if ( WatchPortMap == NULL )
		WatchPortMap = HashMapNew(128, "WatchPortMap");
	if ( WatchTextMap == NULL )
		WatchTextMap = HashMapNew(128, "WatchTextMap");

	ParserNormalizeInput(parser);

	CommandResult_t result = CommandOk;
	char *word = ParserGetNextToken(parser, " ");

	if (word == NULL || strlen(word) <= 0)
	{
		char tmp[1024];
		sprintf(tmp, "No command options");
		SysLog(LogError, tmp);
		StringSprintfCat(response, "%s\nTry:\n\taddr ...\n\tport ...\n\ttext ...\n", tmp);
		return CommandBadOption;			// no input?
	}

	char *nextWord = NULL;
	do
	{
		word = strdup(word);	// need a copy (ParserGetNextToken overwrites)

		if ( stricmp(word, "addr") == 0 )
			result = _SockWatchCmd(this, parser, prior, response, WatchAddrMap, "address");
		else if ( stricmp(word, "port") == 0 )
			result = _SockWatchCmd(this, parser, prior, response, WatchPortMap, "port");
		else if ( stricmp(word, "text") == 0 )
			result = _SockWatchCmd(this, parser, prior, response, WatchTextMap, "content");
		else
		{
			StringSprintfCat(response, "Try:\n\taddr ...\n\tport ...\n\ttext ...\n");
			return CommandBadOption;
		}

		free(word);				// let go
	} while ((word = nextWord) != NULL && strlen(word) > 0);

	return CommandOk;
}
