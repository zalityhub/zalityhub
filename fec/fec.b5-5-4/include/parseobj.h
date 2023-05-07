/*****************************************************************************

Filename:   include/parseobj.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:53 $
 * $Header: /home/hbray/cvsroot/fec/include/parseobj.h,v 1.3.4.2 2011/10/27 18:33:53 hbray Exp $
 *
 $Log: parseobj.h,v $
 Revision 1.3.4.2  2011/10/27 18:33:53  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:37  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:36  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: parseobj.h,v 1.3.4.2 2011/10/27 18:33:53 hbray Exp $ "


#ifndef _PARSEOBJ_H
#define _PARSEOBJ_H

typedef struct Parser_t
{
	int			eof;

	Stack_t		*stack;

	String_t	*bfr;

	int			setLen;				// original amount 'set'
	int			len;					// how much is being used.
	char		*next;				// current output pointer

	char		*token;
} Parser_t;


#define ParserNew(size) ObjectNew(Parser, size)
#define ParserVerify(var) ObjectVerify(Parser, var)
#define ParserDelete(var) ObjectDelete(Parser, var)


extern Parser_t* ParserConstructor(Parser_t *this, char *file, int lno, int size);
extern void ParserDestructor(Parser_t *this, char *file, int lno);
extern BtNode_t* ParserNodeList;
extern struct Json_t* ParserSerialize(Parser_t *this);
extern char* ParserToString(Parser_t *this);

extern int ParserSetInputData(Parser_t *this, char *bfr, int plen);
// TODO: extern int ParserSetInputFile(Parser_t *this, int f);
extern void ParserClear(Parser_t *this);
extern void ParserRewind(Parser_t *this);
extern void ParserNormalizeInput(Parser_t *this);
extern char *ParserGetNextToken(Parser_t *this, char *delimeters);
extern char *ParserUnGetToken(Parser_t *this, char *delimeters);
extern char *ParserGetFullString(Parser_t *this);
extern char *ParserGetString(Parser_t *this);
extern char *ParserDownshift(Parser_t *this, char *string);

#endif
