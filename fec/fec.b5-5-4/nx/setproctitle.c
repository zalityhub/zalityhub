/*****************************************************************************

Filename:   lib/nx/setproctitle.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:19 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/setproctitle.c,v 1.2 2011/07/27 20:22:19 hbray Exp $
 *
 $Log: setproctitle.c,v $
 Revision 1.2  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:48  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: setproctitle.c,v 1.2 2011/07/27 20:22:19 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"


int
RelocateEnviron(char *argv[])
{
	extern char **environ;

	int elen = 0;

	if (environ != NULL)
	{
		while (environ[elen])
			++elen;
	}

	unsigned int size;

	if (elen > 0)
		size = environ[elen - 1] + strlen(environ[elen - 1]) - argv[0];
	else
		size = 0;

	if (size > 0)
	{
		char **newe = calloc(++elen, sizeof(char *));

		unsigned int i = -1;

		while (environ[++i])
			newe[i] = strdup(environ[i]);

		environ = newe;
	}

	return size;
}


int
SetProcTitle(char *argv[], int size, char *title)
{
	char *argv0 = argv[0];

	memset(argv0, 0, size);
	memcpy(argv0, title, strlen(title));
	return 0;
}
