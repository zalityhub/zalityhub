/*****************************************************************************

Filename:   include/libnx.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:53 $
 * $Header: /home/hbray/cvsroot/fec/include/libnx.h,v 1.3.4.6 2011/10/27 18:33:53 hbray Exp $
 *
 $Log: libnx.h,v $
 Revision 1.3.4.6  2011/10/27 18:33:53  hbray
 Revision 5.5

 Revision 1.3.4.5  2011/09/24 17:49:37  hbray
 Revision 5.5

 Revision 1.3.4.4  2011/09/02 14:17:02  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/01 14:49:43  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/23 19:53:58  hbray
 eliminate fecplugin.h

 Revision 1.3.4.1  2011/08/01 16:11:28  hbray
 *** empty log message ***

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:35  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: libnx.h,v 1.3.4.6 2011/10/27 18:33:53 hbray Exp $ "


#ifndef _LIBNX_H
#define _LIBNX_H


// Common data types

typedef unsigned long long UINT64;
typedef unsigned long UINT32;

typedef enum { false = 0, off = false, true, on = true } boolean ;

typedef enum {DumpRadixOctal = 8, DumpRadixDecimal = 10, DumpRadixHex = 16 } DumpRadix_t ;

typedef enum { NoOutput = 0, TextOutput = 1, DumpOutput = 2 } OutputType_t ;

typedef unsigned long long NxCount_t;

typedef long long NxTime_t;

typedef unsigned long Spin_t;

typedef union
{
	uuid_t				u;
	unsigned long long	ull[2];
} NxUid_t ;

#define NxUidNull _NxUidNull()
static inline NxUid_t _NxUidNull() { static NxUid_t uid; uid.ull[0] = uid.ull[1] = 0; return uid; }


// Helper Macros

// Use this to declare a function that uses SysLog
#if WIN32
#define __FUNC__ ((char*)__FUNCTION__)
#else
#define __FUNC__ ((char*)__func__)
#endif

#define sizeof_array(arr) ((sizeof (arr))/(sizeof (arr)[0]))
#define va_args_toarray(...) (void *[]){ __VA_ARGS__, "" }, sizeof_array(((void *[]){ __VA_ARGS__ }))

#define CnvStringIntValue(val) ((val)?atol(val):0)
static inline boolean CnvStringBooleanValue(char *val)
{
	return (val != NULL) && (
		(stricmp(val,"yes")== 0) ||
		(stricmp(val,"true")== 0) ||
		(CnvStringIntValue(val) != 0) );
}

#ifndef min
#define		min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define		max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef EnumToString
#define EnumToString(e) \
case (e): \
	text = #e; \
	break
#endif

#define NullToValue(n, v) ((n)==NULL)?(v):(n)
#define BooleanToString(b) (b)?"yes":"no"


#define printf(...) writef(fileno(stdout), ##__VA_ARGS__)
#define puts(...) writef(fileno(stdout), ##__VA_ARGS__)
#ifndef __CYGWIN__
#define putchar(ch) writef(fileno(stdout), "%c", ch)
#endif



// Various Global/System-wide Constants
#define     MaxSockPacketLen        (8 * 1024)
#define		MaxNameLen				256
#define		MaxPropertyLen			1024
#define		MaxPathLen				(MaxPropertyLen)

#define		MAXHOSTS				3
#define     MAXSERVICES             2000	// Maximum Service (LISTEN) ports
#define     MAXWORKERS              128		// Maximum generic workers (128)

// Common structures
//
typedef struct Counts_t
{
	unsigned long long inPkts;		// Inbound counts
	unsigned long long outPkts;		// Outbound counts
	
	unsigned long long inChrs;		// Inbound counts
	unsigned long long outChrs;		// Outbound counts
} Counts_t;


// includes
#include "include/leaklib.h"
#include "include/spin.h"
#include "include/bt.h"
#include "include/objlib.h"
#include "include/memory.h"
#include "include/stringobj.h"
#include "include/utillib.h"
#include "include/cJSON.h"
#include "include/json.h"
#include "include/stack.h"
#include "include/sysloglib.h"
#include "include/hashlib.h"
#include "include/nxsignal.h"
#include "include/pair.h"
#include "include/nx.h"
#include "include/eventlib.h"
#include "include/timerlib.h"
#include "include/parseobj.h"
#include "include/proclib.h"
#include "include/fsmlib.h"
#include "include/command.h"
#include "include/socklib.h"
#include "include/server.h"
#include "include/auditlib.h"
#include "include/timerlib.h"
#include "include/locklib.h"


// Utility functions
//
extern int parseIni(char *, char[], char[]);

extern void Dumpmem(void (*put) (char *line, void *arg), void *putarg, void *mem, int len, int *offset);
extern void DumpmemFull(void (*put) (int offset, char *dump, char *text, void *arg), void *putarg, void *mem, int len, int *offset);

extern int parseXmlField(char *xml, char *name, char *data, int trim);

#define writef(fd, fmt, ...) _writef(fd, __FILE__, __LINE__, __FUNC__, 0, "", fmt, ##__VA_ARGS__)
extern int _writef(int fd, char *file, int lno, char *fnc, int error, char *errortext, char *fmt, ...);
extern int fwritef(int fd, char *fmt, ...);

extern int CalcLrc(char *data, int size);
extern int RelocateEnviron(char *argv[]);
extern int SetProcTitle(char *argv[], int size, char *title);

extern char *IntegerToString(int value, char *result);


// formats an 8 byte IP address into dot notation
static inline char*
IpAddrToString(unsigned char *addr)
{
	char *text = StringStaticSprintf("%d.%d.%d.%d", (unsigned int)(addr[0]), (unsigned int)(addr[1]), (unsigned int)(addr[2]), (unsigned int)(addr[3]));
	return text;
}

#include "include/isoutil.h"

#endif
