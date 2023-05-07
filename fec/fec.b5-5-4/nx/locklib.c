/*****************************************************************************

Filename:	lib/nx/locklib.c

Purpose:	Request an advisory lock of a FEC system resource, or unlock a
			previously obtained lock.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:18 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/locklib.c,v 1.2 2011/07/27 20:22:18 hbray Exp $
 *
 $Log: locklib.c,v $
 Revision 1.2  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:46  hbray
 Added cvs headers

 *
2009.12.12 harold bray          Adapted
2009.06.03 joseph dionne		Created release 3.1
*****************************************************************************/

#ident "@(#) $Id: locklib.c,v 1.2 2011/07/27 20:22:18 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/locklib.h"

#include <sys/file.h>



boolean
FileLock(int lockFd, boolean wait)
{


	int cmd = LOCK_EX;

	if (!wait)
		cmd |= LOCK_NB;

	for (;;)
	{
		if (flock(lockFd, cmd) != 0)
		{
			if (errno == EINTR)
				continue;		// interrupted

			if (errno != EWOULDBLOCK)
				SysLog(LogError, "flock failed; fd=%d; errno=%s", lockFd, ErrnoToString(errno));
			return (true);
		}

		break;
	}

	return (false);
}


boolean
FileUnlock(int lockFd)
{

	int cmd = LOCK_UN;

	for (;;)
	{
		if (flock(lockFd, cmd) != 0)
		{
			if (errno == EINTR)
				continue;		// interrupted

			if (errno != EWOULDBLOCK)
				SysLog(LogError, "flock failed; fd=%d; errno=%s", lockFd, ErrnoToString(errno));
			return (true);
		}

		break;
	}

	return (false);
}
