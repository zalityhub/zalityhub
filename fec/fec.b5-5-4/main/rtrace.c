#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>



#define HostName "127.0.0.1"
#define HostPort 9500

static char			*MyName = "";


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
	fprintf(stderr, "Reads tracelog from datagram port\n");
	fprintf(stderr, "    -h source_address; default -h %s:%d\n", HostName, HostPort);
	fprintf(stderr, "\n");
	exit(1);
}


int
main(int argc, char *argv[])
{
	char	*hostName = HostName;
	int		hostPort = HostPort;


	MyName = basename(strdup(argv[0]));

	--argc;
	argv += 1;

	while (argc > 0)
	{
		if (strcmp(argv[0], "-h") == 0)
		{
			char *ptr;

			if (argc < 2)
				Usage("address must follow -h");
			hostName = argv[1];
			if ((ptr = strchr(hostName, ':')) != NULL)	// port designation
			{
				*ptr++ = '\0';	// trim address
				hostPort = atoi(ptr);
				if ( (!isdigit(*ptr)) || hostPort < 0)
					Usage("Invalid port number given");
			}
			else
			{
				Usage("port is missing from -h");
			}
			argc -= 2;
			argv += 2;
		}
		else
		{
			Usage("%s is not a valid argument", *argv);
		}
	}

	if ( hostName == NULL || strlen(hostName) <= 0 )
		Usage("At least option -h is required");

// resolve the destination address
	struct hostent *host = NULL;
	if ( (host = gethostbyname(hostName)) == NULL )
	{
		fprintf(stderr, "Unable to resolve host '%s'\n", hostName);
		exit(1);
	}

	struct	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(hostPort);
	sockAddr.sin_addr.s_addr = *(long*)(host->h_addr_list[0]);  /* set the addr */

	//      Create a UNIX datagram socket for server
	int	sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("server: socket");
		exit(1);
	}

	printf("\nReceiving on port %d\n", hostPort);

	if (bind(sock, (const struct sockaddr*)&sockAddr, sizeof(sockAddr)) < 0)
	{
		close(sock);
		perror("server: bind");
		exit(2);
	}        

	for(;;)
	{
    	char bfr[1024*1024];
		struct  sockaddr_un from;
		socklen_t fromlen;

		fromlen = sizeof(from);
		int rlen = recvfrom(sock, bfr, sizeof(bfr), 0, (struct sockaddr*)&from, &fromlen);
		if (rlen == -1)
		{
			perror("recv\n");
			exit(3);
		}
        bfr[rlen] = '\0';
		if ( strlen(bfr) > 0 && bfr[rlen-1] != '\n' )
			strcat(bfr, "\n");

        printf("%s", bfr);
		fflush(stdout);
	}

	return 0;
}
