/*****************************************************************************

Filename:   include/sql.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:54 $
 * $Header: /home/hbray/cvsroot/fec/include/sql.h,v 1.3.4.2 2011/10/27 18:33:54 hbray Exp $
 *
 $Log: sql.h,v $
 Revision 1.3.4.2  2011/10/27 18:33:54  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:38  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:37  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: sql.h,v 1.3.4.2 2011/10/27 18:33:54 hbray Exp $ "


#ifndef _SQL_H
#define _SQL_H

typedef struct NxSql_t
{
	char				*dbname;
	char				*version;
	struct sqlite3		*dbh;
	int					dberr;
	char				*dberrmsg;
} NxSql_t ;


#define NxSqlNew(dbname) ObjectNew(NxSql, dbname)
#define NxSqlVerify(var) ObjectVerify(NxSql, var)
#define NxSqlDelete(var) ObjectDelete(NxSql, var)

extern NxSql_t* NxSqlConstructor(NxSql_t *this, char *file, int lno, char *dbname);
extern void NxSqlDestructor(NxSql_t *this, char *file, int lno);
extern BtNode_t* NxSqlNodeList;
extern struct Json_t* NxSqlSerialize(NxSql_t *this);
extern char* NxSqlToString(NxSql_t *this);
#define NxSqlExecute(stmt, ...) _NxSqlExecute(NxGlobal->sql, stmt, ##__VA_ARGS__)
extern int _NxSqlExecute(NxSql_t *this, char *stmt, ...);

#endif
