/*****************************************************************************

Filename:	lib/gxml/data.h

Purpose:	GCS HTTP/XML Message Set

			Compliance with specification: "GCS XML Specification"
			Version 4.1.1 last updated on 8/16/2005, by Theron Crissey

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/11/16 19:33:02 $
 * $Header: /home/hbray/cvsroot/fec/lib/gxml/data.h,v 1.3.4.3 2011/11/16 19:33:02 hbray Exp $
 *
 $Log: data.h,v $
 Revision 1.3.4.3  2011/11/16 19:33:02  hbray
 Updated

 Revision 1.3.4.2  2011/09/24 17:49:41  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 19:53:59  hbray
 eliminate fecplugin.h

 Revision 1.3  2011/07/27 20:22:15  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:41  hbray
 Added cvs headers

 *

2010.09.01 joseph dionne		Added SQLite to allow all requests, including
								DepositRequests, to multi thread, i.e. send
								without waiting for response.
2009.09.22 joseph dionne		Ported to new FEC Version 4.3
2009.04.28 joseph dionne		Created release 2.7.
*****************************************************************************/

#ident "@(#) $Id: data.h,v 1.3.4.3 2011/11/16 19:33:02 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/fifo.h"
#include "include/hostrequest.h"
#include "include/proxy.h"
#include "include/piprofile.h"
#include "include/fecconfig.h"
#include "include/pisession.h"
#include "include/sqlite/sqlite3.h"

#define NOW			NxGetTime()	// UTC microseconds as long long

#define VERSION		"5.9"		// Current version
#define PERSISTENT	false		// Boolean value

typedef enum
{								// Offsets into *xml[] below
	eXmlTag = 0,				// "<?xmlXML "
	eRequest,					// "<Request "
	eField,						// "<Field "
	eEndOfField,				// "</fieldFIELD"
	eRecords,					// "<recordsRECORDS"
	eRecord,					// "<recordRECORD"
	eEndOfRecord,				// "</recordRECORD"
	eEndOfRecords,				// "</recordsRECORDS"
	eEndOfRequest				// "</requestREQUEST"
} xmlFormat_t;

typedef enum
{
	eHttpNoContent = 204,
	eHttpInternalError = 500,
	eHttpServiceUnavailable = 503
} gxmlHttpRetCode_t;

// SQLite objects needed for DepositRequest transactions
// NOTE: see gxml/data.c for prepared SQL statements

// Create the DepositRequest "batch" header row
typedef enum {					// Prepared statement IN argument numbers
	ihMsg_id = 1,				// value of Message_ID
	ihChain,					// value of Chain_Code
	ihLocation,					// value of Location_ID 
	ihVenue,					// value of Venue_ID
	ihBatch_date,				// value of Batch_Date
	ihBatch_nbr,				// value of Batch_Nbr
	ihBatch_seq					// value of Batch_Nbr_Seq
} insertHeaderFields_t;

// Return the auto-incremented dep_id primary key column of newly added header
typedef enum {
	glDep_id = 0				// first, and only, column of result set
} getLastDep_id_t;

// Set the at_eof indicator when the last Record of the DepositRequest arrives
typedef enum {					// Prepared statement IN argument numbers
	saeDep_id = 1				// value of header.dep_id to update
} setAtEof_t;

// Get the length of the XML Record tags from the DB
typedef enum {					// Prepared statement IN argument numbers
	gxlDep_id = 1,				// value of header.dep_id to update
	gxlLength = 0				// result set column one (zero based)
} getXmlLength_t;

// Find (select) the DepositHeader header table row
typedef enum {					// Prepared statement IN argument numbers
	fhMsg_id = 1,				// value of Message_ID
	fhBatch_date,				// value of Batch_Date
	fhBatch_nbr,				// value of Batch_Nbr
	fhBatch_seq,				// value of Batch_Nbr_Seq
	fhChain,					// value of Chain_Code
	fhLocation,					// value of Location_ID
	fhVenue,					// value of Venue_ID
	fhDep_id = 0,				// result set field 1
	fhRequests,					// result set field 2
	fhResponses,				// result set field 3
	fhAt_eof,					// result set field 4
	fhStatus,					// result set field 5
	fhXml_size					// result set field 6
} findHeaderFields_t;

// Create a deposit (batch) detail table row
// NOTE: detail table rows are created from Stratus response
typedef enum {					// Prepared statement IN argument numbers
	idDep_id = 1,				// value of header.dep_id inserted
	idRecord_nbr,				// value of Record_Nbr
} insertDetailFields_t;

// Update a deposit (batch) detail table row
// NOTE: detail table rows are created from Stratus response
typedef enum {					// Prepared statement IN argument numbers
	udXml_resp = 1,				// value of formated XML Record response
	udDep_id,					// value of header.dep_id
	udRecord_nbr,				// value of Record_Nbr
} updateDetailFields_t;

// Retrieve each deposit (batch) detail table row's xml_resp column
typedef enum {					// Prepared statement IN/OUT argument numbers
	gxrDep_id = 1,				// value of header.dep_id
	gxrXml_resp = 0				// contents of detail.xml_resp
} getXmlResp_t;

// Definition of application plugin specific variables
typedef struct
{
	// Client side statistics
	Counts_t auths;				// Authorization packets
	Counts_t edc;				// EDC(settlement) packets
	Counts_t batched;			// Multiple (batched) API requests
	Counts_t heartbeats;		// Appl Heartbeat requests

	// Plugin specific variables below here
	char	httpver[64];		// Received "HTTP/1.0" version tag
	long	reqLength;			// HTTP Content-Length: value
	char	xmlver[512];		// Received "<?xml version="1.0" ?>" tag
	char	reqType[512];		// XML Document "<Request type=" value
	char *	xml[10];			// Scan formats for parsing GCS Web XML

	// The following are use by gxmlreadreq.msgReady()
#ifndef		TRYS
#define		TRYS	15
#endif
	int		trys;

	// The following are the SQLite prepared statement objects

	// Create the DepositRequest "batch" header row
	sqlite3_stmt *insertHeader;

	// Get the newly added auto-incremented header.dep_id column
	sqlite3_stmt *getLastDep_id;

	// Set the at_eof indicator on receipt the last Record of DepositRequest
	// NOTE: header.at_eof is set in gxml/gxmlreadreq.c
	sqlite3_stmt *setAtEof;

	// Find (select) the DepositHeader header table row
	sqlite3_stmt *findHeader;

	// Create a deposit (batch) detail table row
	// NOTE: detail table rows are created from each DepositRequest Record
	// request in gxml/gxmlreadreq.c and MUST include header.dep_id
	sqlite3_stmt *insertDetail;

	// Update a deposit (batch) detail table row with the XML response
	// NOTE: detail table rows are updated on each Stratus response
	// in gxml/gxmlsendrsp.c and MUST include header.dep_id
	sqlite3_stmt *updateDetail;

	// Calculate the length of each Record's XML response tags
	sqlite3_stmt *getXmlLength;

	// Count the number of Records with XML response tags
	sqlite3_stmt *getRespCount;

	// Retrieve each deposit (batch) detail.xml_resp for POS response
	sqlite3_stmt *getXmlResp;

	char		dbName[128];	// DepositRequest Context SQL DB
	boolean		isEdc;			// Receiving DepositRequest message
	boolean		atLast;			// Last deposit record received
	sqlite3 *	db;				// Multi-thread DepositRequest support DB
	long long	dep_id;			// SQLite int64 value for header.dep_id
	char		chain[32];		// value of Chain_Code tag
	char		location[32];	// value of Location_ID tag
	char		venue[32];		// value of Venue_ID tag
	char		appl_id[32];	// value of Application_ID tag
	char		control[32];	// value of Control_Code tag
	long long	msg_id;			// value of Message_ID tag
	long		batch_date;		// value of Batch_Date tag
	long		batch_nbr;		// value of Batch_Nbr tag
	long		batch_seq;		// value of Batch_Nbr_Seq tag

	// The following are for debug usage
	boolean		leaveDb;		// do not remove the db
} appData_t;

// Required application plugin method declarations
PiApiResult_t Load();
PiApiResult_t Unload();
PiApiResult_t BeginSession(PiSession_t *);
PiApiResult_t EndSession(PiSession_t *);
PiApiResult_t ReadRequest(PiSession_t *);
PiApiResult_t SendResponse(PiSession_t *);

// Load library scope helper method declaration
extern PiApiResult_t GxmlErrorResponse(PiSession_t *, int);
extern PiApiResult_t GxmlSendPos(PiSession_t *, HostRequest_t *);
extern int (*GxmlFilePrintf) (FILE *, char *, ...);
