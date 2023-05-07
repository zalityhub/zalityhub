/*****************************************************************************

Filename:	lib/mpms/data.h

Purpose:	Marriott PMS2Way Message Set

			Compliance with specification: HiltonPMS2WayMessageSet-AC.doc,
			last updated on 3/5/2008.

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:42 $
 * $Header: /home/hbray/cvsroot/fec/lib/mpms/data.h,v 1.3.4.2 2011/09/24 17:49:42 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.3.4.2  2011/09/24 17:49:42  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:53:59  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:16  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:43  hbray
 Added cvs headers

 *

2009.02.19 harold bray		Created release 1.4.  Supports authorizations
								only at this release.
*****************************************************************************/

#ident "@(#) $Id: data.h,v 1.3.4.2 2011/09/24 17:49:42 hbray Exp $ "

#include "include/stdapp.h"
#include <include/libnx.h>
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"

#define VERSION		"4.2"
#define PERSISTENT	true


#pragma pack(1)					// pack structure
typedef struct Pms2WayMsgHeader_t
{
	char msgType[3];
	char fs1;
	char seqNbr[10];
	char fs2;
	char versionNbr[4];
	char fs3;
	char dataLen[4];
	char fs4;
} Pms2WayMsgHeader_t;


typedef struct Pms2WayMsg_t
{
	Pms2WayMsgHeader_t	hdr;
	char				body[1];
} Pms2WayMsg_t ;
#pragma pack()					// back to default


// Definition of application plugin specific variables
typedef struct
{
	// Client side statistics
	Counts_t auth;				// Authorization packets

	Pms2WayMsgHeader_t receivedHeader;
} appData_t;

// Required application plugin method declarations
PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);

int PMS2WaySendPos(PiSession_t *sess, char *msgType, char *bfr, int len);
int PMS2WaySDecrypt(PiSession_t * sess, char *eblock, int esize);
int PMS2WaySEncrypt(PiSession_t * sess, char *eblock, int esize);
