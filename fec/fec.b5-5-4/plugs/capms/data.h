/*****************************************************************************

Filename:	lib/capms/data.h

Purpose:	Micros CA/EDC PMS Front-End Communications plugin
			Supports Micros 3700, 8700, 9700 and Simphony POS devices

			Compliance with specification(s): 
			Micros Systems, Inc. 8700 HMS CA/EDC PMS Interface Spec. Man.
				Copyrite 1995, Part Number: 150502-062 and the Simphony
				changes to the 8700 HMS CA/EDC PMS Interface Spec. Man.
			Micros Simphony CA/EDC/PMS Interface Specification Manual
				2nd Edition, August 2008

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:41 $
 * $Header: /home/hbray/cvsroot/fec/lib/capms/data.h,v 1.4.4.3 2011/09/24 17:49:41 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.4.4.3  2011/09/24 17:49:41  hbray
 Revision 5.5

 Revision 1.4.4.2  2011/08/23 19:53:59  hbray
 eliminate fecplugin.h

 Revision 1.4.4.1  2011/08/11 19:47:33  hbray
 Many changes

 Revision 1.4  2011/07/27 20:22:14  hbray
 Merge 5.5.2

 Revision 1.3.2.2  2011/07/27 20:19:40  hbray
 Added cvs headers

 *

2009.05.14 joseph dionne		Created release 2.9
*****************************************************************************/

#ident "@(#) $Id: data.h,v 1.4.4.3 2011/09/24 17:49:41 hbray Exp $ "

#include "include/stdapp.h"
#include <include/libnx.h>
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"

#define VERSION		"4.0"		// Current version
#define PERSISTENT	true		// Boolean value

typedef enum
{								// Index into msgName[] below
	MicrosCA_REQ = 1,			// In  Credit Authorization Request
	MicrosCA_RSP = 2,			// Out Credit Authorization Response
	MicrosRV_REQ = 3,			// In  Reversal Request
	MicrosRV_RSP = 4,			// Out Reversal Response
	MicrosBO_REQ = 5,			// In  Batch Open Request
	MicrosBO_RSP = 6,			// Out Batch Open Response
	MicrosBX_REQ = 7,			// In  Batch Transfer Request
	MicrosBX_RSP = 8,			// Out Batch Transfer Response
	MicrosBC_REQ = 9,			// In  Batch Close Request
	MicrosBC_RSP = 10,			// Out Batch Close Response
	MicrosBI_REQ = 11,			// In  Batch Inquiry Request
	MicrosBI_RSP = 12,			// Out Batch Inquiry Response
	MicrosGC_REQ = 13,			// In  Gift Request
	MicrosGC_RSP = 14,			// Out Gift Response
	MicrosMtLast				// Last message type
} MicrosMsgType_t;				// Micros Message Types


// The following typedef(s) describe the Micros CA/EDC PMS message components
// NOTE: Micros has two "flavors" of "Work Station ID", thus two definitions
#pragma pack(1)
typedef struct
{								// POS Source ID Segment
	char workStation[2];		// Two (2) digit work station id
	char interface[16];
} Pos2SourceId_t;

typedef struct
{								// POS Source ID Segment
	char workStation[9];		// Nine (9) digit work station id
	char interface[16];
} Pos9SourceId_t;

// Micros Gift Data Segment consists of the following;
// <fs><seq><retran><Message ID><FS><Msg Field 1><FS><Msg Field 1><FS><Msg Field n>
typedef struct
{								// Gift Data Segment
	char		fs;
	char		sequence[2];
	char		retranFlag;
	char		msgId[6];
	char		fs2;
	char		msgData[];
} GiftReq_t ;

// Micros Auth Data Segment consists of the following;
// <seq><retran><Message ID><FS><Msg Field 1><FS><Msg Field 1><FS><Msg Field n>
typedef struct
{								// Auth Data Segment
	char		sequence[2];
	char		retranFlag;
	char		msgId[6];
	char		fs;
	char		msgData[];
} AuthReq_t ;

typedef struct
{								// Checksum Field
	char checkSum[4];
} CheckSum_t;

// Micros CA/EDC PMS record/field structures
// NOTE: basic structure of a Micros CA/EDC PMS message is as follows;
// <SOH><POS Source ID><STX><ApplSeq><ApplData><ETX><Checksum><EOT>
// where <Checksum> is four ASCII hex digits of a 16-bit binary sum of
// all message bytes counting after <SOH> through to and including <ETX>
typedef union
{
	char buf[32 * 1024];		// Micros maximum message
	struct
	{								// "Redefine" of the Micros message
		char			soh;		// appData_t->msg.$SOH
		Pos2SourceId_t	srcId;		// appData_t->msg.srcId
		char			stx;		// appData_t->msg.$STX
		union
		{
			AuthReq_t 		auth;
			GiftReq_t		gift;
		} data ;
	} msg2;						// message, which is decoded by readreq()
	struct
	{							// "Redefine" of the Micros message
		char			soh;		// appData_t->msg.$SOH
		Pos9SourceId_t	srcId;		// appData_t->msg.srcId
		char			stx;		// appData_t->msg.$STX
		union
		{
			AuthReq_t 		auth;
			GiftReq_t		gift;
		} data ;
	} msg9;						// message, which is decoded by readreq()
} microsmsg_t;


#pragma pack()		// back to default packing

static const char Ack = (const char) 0x06;
static const char Eot = (const char) 0x04;
static const char Etx = (const char) 0x03;
static const char Fs =  (const char) 0x1c;
static const char Nak = (const char) 0x15;
static const char Soh = (const char) 0x01;
static const char Stx = (const char) 0x02;


// Definition of application plugin specific variables
typedef struct
{
	// Client side statistics
	Counts_t	auth;				// Authorization packets
	Counts_t	edc;				// EDC(settlement) packets
	Counts_t	batch;			// Multiple (batch) API requests
	Counts_t	heartbeats;			// Appl Heartbeat requests

	// Plugin specific variables below here
	// The values below are initialized in
	// lib/capms/data.c

	int				posSrcLen;		// "POS Source Id" length, 18 or 25 bytes
	boolean			isEdc;			// Receiving settlement (EDC)

	char			pingMark[32];	// Space(16) + STX + ETX

	MicrosMsgType_t	msgType;		// Index into msgName below

	int				altPort;
} appData_t;


// Required application plugin method declarations
PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);
