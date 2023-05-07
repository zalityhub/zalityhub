/*****************************************************************************

Filename:   include/stdapp.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:13 $
 * $Header: /home/hbray/cvsroot/fec/include/stdapp.h,v 1.3 2011/07/27 20:22:13 hbray Exp $
 *
 $Log: stdapp.h,v $
 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:37  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: stdapp.h,v 1.3 2011/07/27 20:22:13 hbray Exp $ "


#ifndef H_STDAPP
#define H_STDAPP

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include <signal.h>
extern int sigignore(int sig);
typedef void (*sighandler_t) (int);
extern sighandler_t sigset(int sig, sighandler_t disp);

#include <alloca.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>
#include <sys/stat.h>

#include "uuid/uuid.h"


#ifndef __CYGWIN__
// #include <strings.h>
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

// These method(s) are not always defined in the proper headers
pid_t getpgid(pid_t);

#endif // H_STDAPP
