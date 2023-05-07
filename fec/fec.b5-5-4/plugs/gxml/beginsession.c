/*****************************************************************************

Filename:	lib/gxml/beginsession.c

Purpose:	GCS HTTP/XML Message Set

			Compliance with specification: "GCS XML Specification"
			Version 4.1.1 last updated on 8/16/2005, by Theron Crissey

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			The load library instantiation method.  Called directly after
			the POS connection established, but prior to any communication.

			Allocate working memory storage from heap and any other required
			resources needed to process the specific protocol/packet type.

			Initialize the communications channel, i.e. send ENQ for example.

			Return true upon success, or false upon failure/error condition.
Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:41 $
 * $Header: /home/hbray/cvsroot/fec/lib/gxml/beginsession.c,v 1.2.4.4 2011/09/24 17:49:41 hbray Exp $
 *
 $Log: beginsession.c,v $
 Revision 1.2.4.4  2011/09/24 17:49:41  hbray
 Revision 5.5

 Revision 1.2.4.2  2011/08/18 18:26:16  hbray
 release 5.5

 Revision 1.2.4.1  2011/08/17 17:58:57  hbray
 *** empty log message ***

 Revision 1.2  2011/07/27 20:22:15  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:41  hbray
 Added cvs headers

 *
2010.09.01 joseph dionne		Added SQLite to allow all requests, including
								DepositRequests, to multi thread, i.e. send
								without waiting for response.
2010.06.18 joseph dionne		Allow xmlns and other attributes between the
								XML 'Request' Tag and the "type" attribute
								and XML 'Field' Tag and the "name" attribute
2009.09.22 joseph dionne		Ported to new FEC Version 4.3
2009.06.03 joseph dionne		Created release 3.1
*****************************************************************************/

#ident "@(#) $Id: beginsession.c,v 1.2.4.4 2011/09/24 17:49:41 hbray Exp $ "

// Application plugin data/method header declarations
#include "data.h"
#include <dlfcn.h>

// Local scope variable declaration
static char *xml[] = {			// NOTE: don't add '>' terminator
	"<?xmlXML ",				// 00 -- eXmlTag
	"<Request ",				// 01 -- eRequest
	"<Field ",					// 02 -- eField
	"</fieldFIELD",				// 03 -- eEndOfField
	"<recordsRECORDS",			// 04 -- eRecords
	"<recordRECORD",			// 05 -- eRecord
	"</recordRECORD",			// 06 -- eEndOfRecord
	"</recordsRECORDS",			// 07 -- eEndOfRecords
	"</requestREQUEST",			// 08 -- eEndOfRequest
	NULL						// NULL pointer terminated list
};

// NOTE: The variable definition below generates the following compile time
// error should xml[] (above) be out-of-sync with the appData_t->xml[]
// definition. (gcc compiler assumed)
// gxml/beginsession.c:51 error: size of array ‘xmlSizeCheck’ is negative
static volatile char xmlSizeCheck[(sizeof(xml) == sizeof(((appData_t *)xml)->xml)) - 1];

PiApiResult_t
BeginSession(PiSession_t *sess)
{
	PiApiResult_t	pluginRc		= eOk;
	void *			handle;
	char *			sql				= 0;
	const char *	sqlTail			= 0;
	char *			sqlErr			= 0;
	int				sqlRc			= 0;

	PiSessionVerify(sess);

	// Get the address of the standard IO fprintf() method
	if (!(GxmlFilePrintf))
	{
		// Open the current process memory
		if (!(handle = dlopen(0, RTLD_NOW)))
		{
			SysLog(LogDebug, "Access to fprintf() is required");

			// Get the address for fprintf()
		}
		else if (!(GxmlFilePrintf = dlsym(handle, "fprintf")))
		{
			SysLog(LogDebug, "Access to fprintf() is required");
		}						// if (!(handle = dlopen(0,RTLD_NOW)))
		dlclose(handle);

		// Cannot process without access to the standard fprintf method
		if (!(GxmlFilePrintf)) return (eFailure);
	} // if (!(GxmlFilePrintf))

	// Allocate this worker's application data, appData_t
	// NOTE: xmlSizeCheck test suppresses CC warning only
	sess->pub.context = PiCreateContext(sess, sizeof(appData_t));
	appData_t *ctx = (appData_t*)sess->pub.context->data;

	// Access to the XML Document scan formats
	memcpy(ctx->xml, xml, sizeof(ctx->xml));

	// Uncomment the line below to stop the deletion of the SQL DB
	//ctx->leaveDb = true;

	// Initialize the number of read attempts to "try" before ending session
	ctx->trys = TRYS;

	// Create SQL DB for processing multi-threaded DepositRequest(s)
	for(;eOk == pluginRc;) {
		// Default to plug-in error
		pluginRc = eFailure;

		// Open the SQL DB for DepositRequest storage per session
		// NOTE: a "session" is the lifetime of the connection
		sprintf(ctx->dbName,"%s/.gxml.%llu.db",PiCreateSessionDir(sess),NOW);
		sqlRc = sqlite3_open(ctx->dbName,&ctx->db);
		if ((sqlRc)) break;

		// Set SQLite3 PRAGMAs for performance
		sqlRc = sqlite3_exec(ctx->db,"PRAGMA synchronous=OFF",0,0,&sqlErr);
		if ((sqlRc)) break;
		sqlRc = sqlite3_exec(ctx->db,"PRAGMA journal_mode=OFF",0,0,&sqlErr);
		if ((sqlRc)) break;

		// Build the SQLite prepared statements for DepositRequest
		// Create the DepositRequest header table
		sqlRc = sqlite3_exec(ctx->db,"create table header("
			"dep_id integer primary key autoincrement,"
			"msg_id integer(8),"
			"chain text,"
			"location text,"
			"venue text,"
			"batch_date integer(4),"
			"batch_nbr integer(4),"
			"batch_seq integer(4),"
			"status text default 'OK ',"
			"requests integer(4) default 0,"
			"responses integer(4) default 0,"
			"at_eof integer(1) default 0,"
			"xml_length integer(4) default 0,"
			"unique(msg_id,batch_date,batch_nbr,batch_seq,chain,location,venue))",
			0,0,&sqlErr);
		if ((sqlRc)) break;

		// Create the DepositRequest header unique index using batch uniques
		sqlRc = sqlite3_exec(ctx->db,"create unique index hdr_idx on header("
			"msg_id,batch_date,batch_nbr,batch_seq,chain,location,venue)",0,0,&sqlErr);
		if ((sqlRc)) break;

		// Create the DepositRequest detail table
		sqlRc = sqlite3_exec(ctx->db,"create table detail("
			"dep_id integer,"
			"record_nbr integer(4),"
			"xml_resp text,"
			"unique(dep_id,record_nbr))",0,0,&sqlErr);
		if ((sqlRc)) break;

		// Create the DepositRequest detail unique index
		sqlRc = sqlite3_exec(ctx->db,"create unique index dtl_idx on detail("
			"dep_id,record_nbr)",0,0,&sqlErr);
		if ((sqlRc)) break;

		// Create the DepositRequest detail insert statement
		sql = "insert into detail (dep_id,record_nbr) values(?,?)";
		sqlRc = sqlite3_prepare_v2(ctx->db,sql,strlen(sql),
			&ctx->insertDetail,&sqlTail);
		if ((sqlRc)) break;

		// Create the deposit detail update statement
		sql = "update detail set xml_resp = ? where dep_id = ? "
			"and record_nbr = ?";
		sqlRc = sqlite3_prepare_v2(ctx->db,sql,strlen(sql),
			&ctx->updateDetail,&sqlTail);
		if ((sqlRc)) break;

		// Create the DepositRequest header insert statement
		sql = "insert into header (msg_id,chain,location,venue,"
			"batch_date,batch_nbr,batch_seq) values (?,?,?,?,?,?,?);";
		sqlRc = sqlite3_prepare_v2(ctx->db,sql,strlen(sql),
			&ctx->insertHeader,&sqlTail);
		if ((sqlRc)) break;

		// Create the DepositRequest header query for the last inserted dep_id
		sql = "select dep_id from header where rowid = last_insert_rowid();";
		sqlRc = sqlite3_prepare_v2(ctx->db,sql,strlen(sql),
			&ctx->getLastDep_id,&sqlTail);
		if ((sqlRc)) break;

		// Create the DepositRequest header find (select) statement
		sql = "select dep_id,requests,responses,at_eof,status,xml_length "
			"from header where msg_id = ? and batch_date = ? and batch_nbr = ? "
			"and batch_seq = ? and chain = ? and location = ? and venue = ? "
			"order by batch_date";
		sqlRc = sqlite3_prepare_v2(ctx->db,sql,strlen(sql),
			&ctx->findHeader,&sqlTail);
		if ((sqlRc)) break;

		// Create the DepositRequest header find (select) statement
		sql = "update header set at_eof = 1 where dep_id = ? ";
		sqlRc = sqlite3_prepare_v2(ctx->db,sql,strlen(sql),
			&ctx->setAtEof,&sqlTail);
		if ((sqlRc)) break;

		// Create the DepositRequest header after delete trigger
		sqlRc = sqlite3_exec(ctx->db,"create trigger delete_deposit "
			"after delete on header begin delete from detail "
			"where dep_id = old.dep_id; end",0,0,&sqlErr);
		if ((sqlRc)) break;

		// Create the DepositRequest detail after insert trigger
		sqlRc = sqlite3_exec(ctx->db,"create trigger count_request "
			"after insert on detail begin update header "
			"set requests = 1 + requests "
			"where dep_id = new.dep_id; end",0,0,&sqlErr);
		if ((sqlRc)) break;

		// Create the DepositRequest detail after update trigger
		sqlRc = sqlite3_exec(ctx->db,"create trigger count_response "
			"after update on detail begin update header "
			"set responses = 1 + responses, "
			"xml_length = length(new.xml_resp) + xml_length "
			"where dep_id = old.dep_id; end",0,0,&sqlErr);
		if ((sqlRc)) break;

		// Calculate the length of each deposit Record's XML response
		// NOTE: this statement not currently used, the 'count_response'
		// trigger keeps a running total of the XML bytes for the
		// response to each deposit 'Record'
		sql = "select length(detail.xml_resp) from header "
			"left join detail on header.dep_id = detail.dep_id "
			"where header.dep_id = ?";
		sqlRc = sqlite3_prepare_v2(ctx->db,sql,strlen(sql),
			&ctx->getXmlLength,&sqlTail);
		if ((sqlRc)) break;

		// Retrieve each deposit (batch) detail.xml_resp column
		sql = "select xml_resp from detail where dep_id = ? order by dep_id";
		sqlRc = sqlite3_prepare_v2(ctx->db,sql,strlen(sql),
			&ctx->getXmlResp,&sqlTail);
		if ((sqlRc)) break;

		// SQLite initialized with out error, reset return code
		if (eFailure == pluginRc) {
			pluginRc = eOk;
			break;
		} // if (eFailure == pluginRc)
	} // for(;eOk == pluginRc;)

	// SQLite initialization error, clean up SQLite objects and return error
	if (eFailure == pluginRc) {
		if ((ctx->insertDetail))	sqlite3_finalize(ctx->insertDetail);
		if ((ctx->updateDetail))	sqlite3_finalize(ctx->updateDetail);
		if ((ctx->insertHeader))	sqlite3_finalize(ctx->insertHeader);
		if ((ctx->getLastDep_id))	sqlite3_finalize(ctx->getLastDep_id);
		if ((ctx->getXmlLength))	sqlite3_finalize(ctx->getXmlLength);
		if ((ctx->findHeader))		sqlite3_finalize(ctx->findHeader);
		if ((ctx->getRespCount))	sqlite3_finalize(ctx->getRespCount);
		if ((ctx->getXmlResp))		sqlite3_finalize(ctx->getXmlResp);
		if ((ctx->setAtEof))		sqlite3_finalize(ctx->setAtEof);
		if ((ctx->db))				sqlite3_close(ctx->db);
		ctx->insertDetail	= 0;
		ctx->updateDetail	= 0;
		ctx->insertHeader	= 0;
		ctx->getLastDep_id	= 0;
		ctx->getXmlLength	= 0;
		ctx->findHeader		= 0;
		ctx->getRespCount	= 0;
		ctx->getXmlResp		= 0;
		ctx->setAtEof		= 0;
		ctx->db				= 0;
	} // if (eFailure == pluginRc)


	return (pluginRc);
} // PiApiResult_t BeginSession(PiSession_t *sess)
