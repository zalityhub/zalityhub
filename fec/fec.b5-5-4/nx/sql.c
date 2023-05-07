/*****************************************************************************

Filename:	lib/nx/sql.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:58 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/sql.c,v 1.3.4.2 2011/10/27 18:33:58 hbray Exp $
 *
 $Log: sql.c,v $
 Revision 1.3.4.2  2011/10/27 18:33:58  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:47  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:48  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: sql.c,v 1.3.4.2 2011/10/27 18:33:58 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/sql.h"

#include "include/sqlite/sqlite3.h"


static int
_NxSqlOpen(NxSql_t *this)
{

	NxSqlVerify(this);

// if open then close
	if ( this->dbh != NULL )		// have one open
	{
		SysLog(LogDebug, "sqlite3_close(%s)", this->dbname);
		sqlite3_close(this->dbh);
		this->dbh = NULL;
	}
// Open
	(void)unlink(this->dbname);		// remove previous db
	SysLog(LogDebug, "sqlite3_open(%s)", this->dbname);

	this->dberr = sqlite3_open(this->dbname, &this->dbh);
	if ( this->dberr != SQLITE_OK )
	{
		this->dberrmsg = (char*)sqlite3_errmsg(this->dbh);
		SysLog(LogError, "sqlite3_open of %s failed: %s", this->dbname, this->dberrmsg);
		return -1;		// failed
	}

	return 0;
}



BtNode_t *NxSqlNodeList = NULL;


NxSql_t*
NxSqlConstructor(NxSql_t *this, char *file, int lno, char *dbname)
{

	// save db name
	this->dbname = strdup(dbname);
	// fetch the SQLite version string
	this->version = strdup(sqlite3_libversion());
	this->dberrmsg = "";
	SysLog(LogDebug, "SQL %s version %s", this->dbname, this->version);
	return this;
}


void
NxSqlDestructor(NxSql_t *this, char *file, int lno)
{
	if ( this->dbh != NULL )		// have one open
	{
		SysLog(LogDebug, "sqlite3_close(%s)", this->dbname);
		sqlite3_close(this->dbh);
		this->dbh = NULL;
	}
	free(this->dbname);
	free(this->version);
}


Json_t*
NxSqlSerialize(NxSql_t *this)
{
	NxSqlVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Version", this->version);
	JsonAddString(root, "Dbname", this->dbname);
	JsonAddBoolean(root, "Open", this->dbh!=NULL);
	if ( this->dberr != SQLITE_OK )
		JsonAddString(root, "LastResult", "err=%d,msg=%s", this->dberr, this->dberrmsg);
	return root;
}


char*
NxSqlToString(NxSql_t *this)
{
	Json_t *root = NxSqlSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


int
_NxSqlExecute(NxSql_t *this, char *stmt, ...)
{

	NxSqlVerify(this);

	va_list ap;
	va_start(ap, stmt);
	StringNewStatic(tmp, 32);
	StringSprintfV(tmp, stmt, ap);

	if ( this->dbh == NULL )			// not open
	{
		if ( _NxSqlOpen(this) != 0 )
		{
			SysLog(LogError, "Error when opening for statement: %s [%s]", tmp->str, this->dberrmsg);
			return -1;		// failed
		}
	}

	this->dberr = sqlite3_exec(this->dbh, tmp->str, 0, 0, &this->dberrmsg);
	if ( this->dberr != SQLITE_OK )
	{
		SysLog(LogError, "Error in statement: %s [%s]", tmp->str, this->dberrmsg);
		return -1;		// failed
	}

	return 0;
}
