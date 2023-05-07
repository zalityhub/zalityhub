#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "include/datagram.h"


int
NxDatagramSend(char *host, int port, char *msg, int mlen)
{
	// connector's address information
	struct sockaddr_in addr;
	struct hostent *he;
	// get the host info
	if ((he = gethostbyname(host)) == NULL)
		return -1;

	static int sockfd = -1;
	if ( sockfd < 0 )
	{
		if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
			return -1;
	}

	// host byte order
	addr.sin_family = AF_INET;
	// short, network byte order
	addr.sin_port = htons(port);
	addr.sin_addr = *((struct in_addr *)he->h_addr);
	// zero the rest of the struct
	memset(&(addr.sin_zero), '\0', 8);
	int numbytes = sendto(sockfd, msg, mlen, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	return numbytes;
}


int
NxDatagramPrintfV(char *host, int port, char *fmt, va_list ap)
{
	static char *tmp = NULL;
	static int	tmpSize = 0;

	if ( tmp == NULL )
	{
		tmpSize = 32;
		tmp = calloc(32+10, sizeof(char));
	}

	va_list aq;
	va_copy(aq, ap);		// save a copy
	int len = vsnprintf(tmp, tmpSize+1, fmt, ap);
	if ( len >= tmpSize )		// need more room
	{
		tmpSize += len;
		tmp = realloc(tmp, tmpSize);
		len = vsnprintf(tmp, tmpSize+1, fmt, aq);
	}
	va_end(aq);
	return NxDatagramSend(host, port, tmp, strlen(tmp));
}


int
NxDatagramPrintf(char *host, int port, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return NxDatagramPrintfV(host, port, fmt, ap);
}
