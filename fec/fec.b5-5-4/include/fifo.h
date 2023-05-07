/*****************************************************************************

Filename:   include/fifo.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:52 $
 * $Header: /home/hbray/cvsroot/fec/include/fifo.h,v 1.3.4.2 2011/10/27 18:33:52 hbray Exp $
 *
 $Log: fifo.h,v $
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

#ident "@(#) $Id: fifo.h,v 1.3.4.2 2011/10/27 18:33:52 hbray Exp $ "


#ifndef _FIFO_H
#define _FIFO_H


typedef struct Fifo_t
{
	char		*name;
	int			currentSize;			// current size of buffer
	int			len;					// current nbr bytes unread
	char		*data;
} Fifo_t;


#define FifoNew(name, ...) ObjectNew(Fifo, name, ##__VA_ARGS__)
#define FifoVerify(var) ObjectVerify(Fifo, var)
#define FifoDelete(var) ObjectDelete(Fifo, var)


// External Functions
extern Fifo_t* FifoConstructor(Fifo_t *this, char *file, int lno, char *name, ...);
extern void FifoDestructor(Fifo_t *this, char *file, int lno);
extern BtNode_t* FifoNodeList;
extern struct Json_t* FifoSerialize(Fifo_t *this);
extern char* FifoToString(Fifo_t *this);

extern void FifoClear(Fifo_t *this);
extern int FifoWrite(Fifo_t *this, char *bfr, int len);
extern int FifoRead(Fifo_t *this, char *bfr, int len);
extern int FifoUrlRead(Fifo_t *this, char *bfr, int len);
extern char* FifoLineRead(Fifo_t *this, char *bfr, int len);
extern int FifoEntityRead(Fifo_t *this, char *bfr, int len);

#endif
