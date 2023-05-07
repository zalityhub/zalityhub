/*****************************************************************************

Filename:	lib/visak/data.h

Purpose:	Vital EIS 1051/1052 protocol and EIS 1080/1081 packet types

			Supports POS devices such as ProtoBase SofTrans 192, and the
			Radiant POS, as-well-as any device that conforms to the
			specifications listed below.

			Conforms to the following specifications;

			Vital EIS 1051 Version 3.2 September 2005
			Vital EIS 1052 Version 3.3 September 2005
			Vital EIS 1080 Version 7.7 September 2008
			Vital EIS 1081 Version 7.7 September 2008

			NOTE: this interface will operate in EVEN or NO parity modes.
			This interface WILL NOT operate in "MIX" parity mode, meaning
			receiving EVEN parity and sending NO parity.

			NOTE: Single transaction/batch only implemented at version 4.2

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:52 $
 * $Header: /home/hbray/cvsroot/fec/lib/visak/data.h,v 1.3.4.2 2011/09/24 17:49:52 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.3.4.2  2011/09/24 17:49:52  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:54:00  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:56  hbray
 Added cvs headers

 *

2009.09.22 joseph dionne		Ported to new FEC Version 4.2
2008.11.15 joseph dionne		Created release 1.0.  Supports authorizations
								only at this release.
*****************************************************************************/

#ident "@(#) $Id: data.h,v 1.3.4.2 2011/09/24 17:49:52 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"

#define VERSION		"4.2"		// Current version
#define PERSISTENT	false		// Boolean value

// Definition of application plugin specific variables
typedef struct
{
	// Client side statistics
	Counts_t auth;				// Authorization packets
	Counts_t edc;				// EDC(settlement) packets
	Counts_t batch;			// Multiple (batch) API requests
	Counts_t heartbeats;		// Appl Heartbeat requests

	// Plugin specific variables below here
	char Ack[2];				// ACK byte 0x06
	char Bel[2];				// BEL byte 0x07
	char Dc2[2];				// DC2 byte 0x12
	char Dle[2];				// DLE byte 0x10
	char Enq[2];				// ENQ byte 0x05
	char Eot[2];				// EOT byte 0x04
	char Etb[2];				// ETB byte 0x17
	char Etx[2];				// ETX byte 0x03
	char Nak[2];				// NAK byte 0x15
	char Rs[2];					// RS  byte 0x1E
	char Stx[2];				// STX byte 0x02

	boolean evenParity;			// POS sending EVEN party TCP/IP data
	char applType[2];			// Single'0', Multiple'2', Interleave'4'
	// Single-Batch'1', Multi-Batch'3'

	boolean crlf;				// API terminated with Windows newline
	time_t reqBgTime;			// Time Request read began
	char msgType[16];			// Message content "visak" or "api"
	int lastLen;				// Length of last response sent to POS
	char *lastResponse;			// Last response sent to POS
} appData_t;

#define ACK(mem)		(mem ? (int)mem->Ack[0]:-1)	// Use the uppercase
#define BEL(mem)		(mem ? mem->Bel[0] : -1)	// tokens for byte
#define DC2(mem)		(mem ? mem->Dc2[0] : -1)	// equal testing
#define DLE(mem)		(mem ? mem->Dle[0] : -1)	// i.e. ACK == buf[0]
#define ENQ(mem)		(mem ? mem->Enq[0] : -1)
#define EOT(mem)		(mem ? mem->Eot[0] : -1)	// NOTE: the initial
#define ETB(mem)		(mem ? mem->Etb[0] : -1)	// values for the
#define ETX(mem)		(mem ? mem->Etx[0] : -1)	// control bytes are
#define NAK(mem)		(mem ? mem->Nak[0] : -1)	// set to no partity
#define RS(mem)			(mem ? mem->Rs[0]  : -1)
#define STX(mem)		(mem ? mem->Stx[0] : -1)
#define APPLTYPE(mem)	(mem ? mem->ApplType[0] : -1)

#define ack(mem)		(mem ? mem->Ack : "\xff")	// Use the lowercase
#define bel(mem)		(mem ? mem->Bel : "\xff")	// tokens for writing
#define dc2(mem)		(mem ? mem->Dc2 : "\xff")	// to connection
#define dle(mem)		(mem ? mem->Dle : "\xff")	// i.e. write(...,ack,)
#define enq(mem)		(mem ? mem->Enq : "\xff")
#define eot(mem)		(mem ? mem->Eot : "\xff")	// NOTE: if POS is in
#define etb(mem)		(mem ? mem->Etb : "\xff")	// EVEN parity, the
#define etx(mem)		(mem ? mem->Etx : "\xff")	// initial values
#define nak(mem)		(mem ? mem->Nak : "\xff")	// will be adjusted
#define rs(mem)			(mem ? mem->rs  : "\xff")
#define stx(mem)		(mem ? mem->Stx : "\xff")
#define appltype(mem)	(mem ? mem->ApplType : "\xff")

#define EVENPARITY(mem)	(mem ? mem->evenParity : 0)	// POS sent EVEN parity

// Load library scope helper methods
PiApiResult_t VisakSignOff(PiSession_t *sess);
PiApiResult_t VisakSendPos(PiSession_t *sess, char *, int);

// Required application plugin method declarations
PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);
