/*****************************************************************************

Filename:	lib/nx/writef.c

Purpose:        Formated file descriptor (fd) output.
                        NOTE: formated output is limited to 8kb.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:59 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/writef.c,v 1.3.4.2 2011/10/27 18:33:59 hbray Exp $
 *
 $Log: writef.c,v $
 Revision 1.3.4.2  2011/10/27 18:33:59  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/01 16:11:29  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:49  hbray
 Added cvs headers

 *
$History: $
 *
*****************************************************************************/

#ident "@(#) $Id: writef.c,v 1.3.4.2 2011/10/27 18:33:59 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/sysloglib.h"

static int _fwritef(int fd, char *fmt, va_list ap);


int
_writef(int fd, char *file, int lno, char *fnc, int error, char *errortext, char *fmt, ...)
{

	if ( fd < 3 )		// std output; use syslog
	{
		va_list ap;
		va_start(ap, fmt);
		_SysLogFullV(SysLogGlobal, LogAny, file, lno, fnc, fmt, ap);
		return 0;
	}

// file output
	va_list ap;
	va_start(ap, fmt);
	return _fwritef(fd, fmt, ap);
}


int
fwritef(int fd, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return _fwritef(fd, fmt, ap);
}


static int
_fwritef(int fd, char *fmt, va_list ap)
{
	char	obuf[16384];
	int		bytes = -1;

	if (0 < (bytes = vsnprintf(obuf, sizeof(obuf), fmt, ap)))
		write(fd, obuf, bytes);
	va_end(ap);

	return (bytes);
}
