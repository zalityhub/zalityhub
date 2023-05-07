/*****************************************************************************

Filename:   include/file.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:52 $
 * $Header: /home/hbray/cvsroot/fec/include/file.h,v 1.3.4.2 2011/10/27 18:33:52 hbray Exp $
 *
 $Log: file.h,v $
 Revision 1.3.4.2  2011/10/27 18:33:52  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/24 17:49:36  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:34  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: file.h,v 1.3.4.2 2011/10/27 18:33:52 hbray Exp $ "


#ifndef _FILE_H
#define _FILE_H


typedef struct File_t
{
	char			*name;
	char			*mode;
	FILE			*handle;
	struct Fifo_t	*bfr;
} File_t ;


// Object Functions

#define FileNew() ObjectNew(File)
#define FileVerify(var) ObjectVerify(File, var)
#define FileDelete(var) ObjectDelete(File, var)

extern File_t* FileConstructor(File_t *this, char *file, int lno);
extern void FileDestructor(File_t *this, char *file, int lno);
extern BtNode_t* FileNodeList;
extern struct Json_t* FileSerialize(File_t *this);
extern char* FileToString(File_t *this);
extern int FileOpen(File_t *this, char *name, char *mode);
extern int FileClose(File_t *this);
extern struct Fifo_t* FileReadContents(char *name, ...);
extern struct Fifo_t* FileFdReadContents(int fd);

#endif
