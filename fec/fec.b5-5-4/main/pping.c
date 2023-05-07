/*****************************************************************************

Filename:   main/pping.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

// For Minw32; Use:  gcc -g -std=gnu99 pping.c -o pping -lws2_32


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
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#define SockGetErrno errno
#define SockSetErrno(val) errno=(val)
#define SockError(text) perror(text)
#define max(x,y) ( (x)>(y)? (x) : (y) )
#endif


void panic(char *msg);
#define panic(m)	{perror(m); exit(99);}


/****************************************************************************/
/*** This program opens a connection to a server using either a port or a ***/
/*** service.  Once open, it sends the message from the command line.     ***/
/*** some protocols (like HTTP) require a couple newlines at the end of   ***/
/*** the message.                                                         ***/
/*** Compile and try 'tcpclient lwn.net http "GET / HTTP/1.0" '.          ***/
/****************************************************************************/
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

	struct hostent* host;
	struct sockaddr_in addr;
	int sd, port;

	if ( argc != 3 )
	{
		fprintf(stderr, "usage: %s <servername> <protocol or portnum>\n", argv[0]);
		exit(99);
	}

	/*---Get server's IP and standard service connection--*/
	if ( (host = gethostbyname(argv[1])) == NULL )
	{
		fprintf(stderr, "Unable to resolve host '%s'\n", argv[1]);
		exit(1);
	}

	if ( !isdigit(argv[2][0]) )
	{
		struct servent *srv = getservbyname(argv[2], "tcp");
		if ( srv == NULL )
			panic(argv[2]);
		port = srv->s_port;
	}
	else
	{
		port = htons(atoi(argv[2]));
	}


	{
		struct in_addr in;
		in.s_addr = *(long*)host->h_addr_list[0];
		printf("Server %s %s, Port=%8d: ", argv[1], inet_ntoa(in), ntohs(port));
		fflush(stdout);
	}


	/*---Create socket and connect to server---*/
	sd = socket(PF_INET, SOCK_STREAM, 0);        /* create socket */
	if ( sd < 0 )
		panic("socket");

	memset(&addr, 0, sizeof(addr));       /* create & zero struct */
	addr.sin_family = AF_INET;        /* select internet protocol */
	addr.sin_port = port;                       /* set the port # */
	addr.sin_addr.s_addr = *(long*)(host->h_addr_list[0]);  /* set the addr */

	/*---If connection successful, send the message and read results---*/
	if ( connect(sd, (struct sockaddr*)&addr, sizeof(addr)) == 0)
	{
		printf("Connected\n");
		fflush(stdout);
		return 0;
	}
	else
	{
		printf("FAILED\n");
		fflush(stdout);
		return 1;
	}
}
