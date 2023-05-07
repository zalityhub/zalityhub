/*****************************************************************************

Filename:   include/filefifo.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:52 $
 * $Header: /home/hbray/cvsroot/fec/include/filefifo.h,v 1.3.4.2 2011/10/27 18:33:52 hbray Exp $
 *
 $Log: filefifo.h,v $
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

#ident "@(#) $Id: filefifo.h,v 1.3.4.2 2011/10/27 18:33:52 hbray Exp $ "


#ifndef _FILEFIFO_H
#define _FILEFIFO_H


typedef struct FileFifo_t
{
	NxUid_t			uid;
	char			*filePath;
	unsigned long	inputOffset;
	unsigned long	outputOffset;
	Counts_t		counts;
	void			*context;
} FileFifo_t ;


#define FileFifoNew(contextLen) ObjectNew(FileFifo, contextLen)
#define FileFifoVerify(var) ObjectVerify(FileFifo, var)
#define FileFifoDelete(var) ObjectDelete(FileFifo, var)
extern FileFifo_t* FileFifoConstructor(FileFifo_t *this, char *file, int lno, int contextLen);
extern void FileFifoDestructor(FileFifo_t *this, char *file, int lno);
extern unsigned int FileFifoGetSize(FileFifo_t *this);
extern int FileFifoRead(FileFifo_t *this, char *bfr, int len, boolean dump);
extern int FileFifoWrite(FileFifo_t *this, char *bfr, int len, boolean dump);
extern BtNode_t* FileFifoNodeList;
extern struct Json_t* FileFifoSerialize(FileFifo_t *this);
extern char* FileFifoToString(FileFifo_t *this);

#endif
