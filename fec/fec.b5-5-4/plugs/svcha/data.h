/*****************************************************************************

Filename:	lib/svcha/data.h

Purpose:	MICROS SVCHA XML Message Set

			Compliance with specification: MICROS Standard SVC Interface
			Revision 2.5 last updated on 4/05/2005

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:51 $
 * $Header: /home/hbray/cvsroot/fec/lib/svcha/data.h,v 1.3.4.2 2011/09/24 17:49:51 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.3.4.2  2011/09/24 17:49:51  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:54:00  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:22  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:54  hbray
 Added cvs headers

 *

2009.10.30 joseph dionne		Created at release 5.9
*****************************************************************************/

#ident "@(#) $Id: data.h,v 1.3.4.2 2011/09/24 17:49:51 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"

#define VERSION		"5.9"		// Current version
#define PERSISTENT	false		// Boolean value

typedef enum
{
	SOH = 0x01,
	STX = 0x02,
	ETX = 0x03,
	EOT = 0x04,
	FS = 0x1C
} protocol_byte_e;

typedef struct
{
	char *name;
	int value;
	int group;
} NameToValueMap;

typedef struct
{
	long len;
	char origin[256];
	char target[256];
	char **save;
	char *extra;
	char *req;
	NameToValueMap *map;
} svchaXml_t;
typedef int (*svchaCallBack_t) (char *, int, svchaXml_t *);

// Definition of application plugin specific variables
typedef struct
{
// Client side statistics
	Counts_t auth;				// Authorization packets
	Counts_t edc;				// EDC(settlement) packets
	Counts_t batch;			// Multiple (batch) API requests

// Plugin specific variables below here
	svchaXml_t xmlData;			// Storage for XML parser and response

} appData_t;

#define XMLDATASIZE(len) (4 * len)

// Required application plugin method declarations
PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);

// MICROS SVCHA XML plug-in utility methods
extern NameToValueMap *svchaFindMapByName(char *, NameToValueMap *);
extern NameToValueMap *svchaFindMapByValue(int, NameToValueMap *);
extern int svchaParse(char *, int, svchaCallBack_t, svchaXml_t *);
extern int xmlTagCount(char *);
extern char **xmlToArray(char *, char **);

// Externals variables declared
extern NameToValueMap SvchaTagToValue[];
