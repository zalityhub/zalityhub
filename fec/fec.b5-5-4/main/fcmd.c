/*****************************************************************************

Filename:   main/fcmd.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:24 $
 * $Header: /home/hbray/cvsroot/fec/main/fcmd.c,v 1.3 2011/07/27 20:22:24 hbray Exp $
 *
 $Log: fcmd.c,v $
 Revision 1.3  2011/07/27 20:22:24  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:57  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: fcmd.c,v 1.3 2011/07/27 20:22:24 hbray Exp $ "

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <alloca.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>



typedef void (*sighandler_t) (int);
extern sighandler_t sigset(int sig, sighandler_t disp);


#define SOCKET_DOMAIN		AF_UNIX


static void
Usage(char *pname, char *error)
{
	fprintf(stderr, "\n%s\n", error);
	fprintf(stderr, "Usage: %s channel cmd\n", pname);
	fprintf(stderr, "\n");
	exit(1);
}


static int
SendCmd(int fd, char *cmd)
{
	char *tmp = alloca(strlen(cmd) + 128);

	sprintf(tmp, "#%07d%s", (int)strlen(cmd), cmd);
	return write(fd, tmp, strlen(tmp));
}


static int
RecvRsp(int fd, char *response, int len)
{
	char	*inp = response;
	char	hdr[9];
	int		rlen;

	if ((rlen = read(fd, hdr, sizeof(hdr) - 1)) < 0)
	{
		fprintf(stderr, "\nread error %s\n", strerror(errno));
		return -1;
	}

	if ( rlen == 0 )
		return 0;		// done

	hdr[rlen] = '\0';
	if (hdr[0] != '#' )
	{
		fprintf(stderr, "\nInvalid hdr mark: %s\n", hdr);
		return -1;
	}

	if ( strlen(hdr) > 0 )
		rlen = atoi(&hdr[1]);

	if (rlen < 0 || rlen > len)
	{
		if ( rlen == 0 )
			return 0;
		fprintf(stderr, "\nInvalid length %d in hdr: %s\n", rlen, hdr);
		return -1;
	}

	if ( rlen > 0 )
	{
		for (int pktlen = rlen; pktlen > 0; )
		{
			if ( (rlen = read(fd, inp, pktlen)) <= 0 )
			{
				fprintf(stderr, "\nShort Packet; expected %d, received %d\n", pktlen, rlen);
				return -1;
			}
			pktlen -= rlen;
			inp += rlen;
		}
	}
	else
	{
		inp += sprintf(inp, "No response");
	}

	return inp - response;
}


static int
Connect(char *channel, int domain, int type)
{
	int fd;

	if ((fd = socket(domain, type, 0)) < 0)
	{
		fprintf(stderr, "Socket open error %s\n", strerror(errno));
		return -1;				// driver error
	}

// build server's address
	struct sockaddr *saddr = NULL;
	int saddrlen = 0;

	switch (domain)
	{
	default:
		fprintf(stderr, "%d is not a valid socket domain\n", domain);
		close(fd);
		return -1;
		break;

	case AF_INET:
		{
			static struct sockaddr_in inetaddr;

			memset(&inetaddr, 0, sizeof(inetaddr));
			inetaddr.sin_family = domain;

			struct hostent *hp = gethostbyname(channel);

			if (hp == NULL)
			{
				perror("gethostbyname");
				fprintf(stderr, "%s failed\n", channel);
				return -1;
			}

			// copy the IP number
			memcpy(&inetaddr.sin_addr.s_addr, hp->h_addr, hp->h_length);
			saddr = (struct sockaddr *)&inetaddr;
			saddrlen = sizeof(inetaddr);
			break;
		}

	case AF_UNIX:
		{
			static struct sockaddr_un nameaddr;

			memset(&nameaddr, 0, sizeof(nameaddr));
			nameaddr.sun_family = domain;
			strncpy(nameaddr.sun_path, channel, sizeof(nameaddr.sun_path) - 1);
			saddr = (struct sockaddr *)&nameaddr;
			saddrlen = sizeof(nameaddr);
			break;
		}
	}

// initiate the connect
	if (connect(fd, saddr, saddrlen) < 0)
	{
		fprintf(stderr, "Unable to connect to %s %s\n", channel, strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}


int
main(int argc, char *argv[])
{
	char *pname = argv[0];		// my name
	char *channel = NULL;

	if (argc < 2)
		Usage(pname, "Insufficient parameters");

// Skip process name
	++argv;
	--argc;

	channel = argv[0];
	++argv;
	--argc;

	int fd = Connect(channel, SOCKET_DOMAIN, SOCK_STREAM);

	if (fd < 0)
	{
		fprintf(stderr, "Connect failed\n");
		exit(1);
	}

// build a command from the arg tail
	static char *nl = "\r\n";
	char tmp[32768];
	char *out = tmp;

	memset(tmp, 0, sizeof(tmp));

// Append all args...
	if (argc <= 0)
	{
		out += sprintf(out, "help");
	}
	else
	{
		for (int i = 0; i < argc; ++i)
			out += sprintf(out, "%s ", argv[i]);
	}

	if (SendCmd(fd, tmp) < 0)
		exit(1);

	static char bfr[1024 * 1024 * 10];
	int rlen;

	while ((rlen = RecvRsp(fd, bfr, sizeof(bfr))) > 0)
	{
		write(fileno(stdout), bfr, rlen);
		if (bfr[rlen] != '\n')
			write(fileno(stdout), nl, strlen(nl));
	}

	return 0;
}
