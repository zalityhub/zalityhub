/*****************************************************************************

Filename:	include/leaklib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:37 $
 * $Header: /home/hbray/cvsroot/fec/include/leaklib.h,v 1.3.4.1 2011/09/24 17:49:37 hbray Exp $
 *
 $Log: leaklib.h,v $
 Revision 1.3.4.1  2011/09/24 17:49:37  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:35  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: leaklib.h,v 1.3.4.1 2011/09/24 17:49:37 hbray Exp $ "


#ifndef _LEAKLIB_H
#define	_LEAKLIB_H


// #define LEAK_DETECTION 1


#ifdef LEAK_DETECTION
#include "malloc.h"
#define malloc(size) LeakMalloc((size), __FILE__, __LINE__)
#define calloc(nelem,elsize) LeakCalloc((nelem),(elsize), __FILE__, __LINE__)
#define realloc(old, size) LeakRealloc((old), (size), __FILE__, __LINE__)
#define free(mem) LeakFree((mem), __FILE__, __LINE__)
#define strdup(stg) LeakStrDup((stg), __FILE__, __LINE__)

#define LeakMalloc(size, file, lno) _LeakMalloc((size), (file), (lno))
#define LeakCalloc(nelem,elsize, file, lno) _LeakCalloc((nelem),(elsize), (file), (lno))
#define LeakRealloc(old, size, file, lno) _LeakRealloc((old), (size), (file), (lno))
#define LeakFree(mem, file, lno) _LeakFree((mem), (file), (lno))
#define LeakStrDup(stg, file, lno) _LeakStrDup((stg), (file), (lno))
#else
#define LeakMalloc(size, file, lno) malloc((size))
#define LeakCalloc(nelem,elsize, file, lno) calloc((nelem),(elsize))
#define LeakRealloc(old, size, file, lno) realloc((old), (size))
#define LeakFree(mem, file, lno) free((mem))
#define LeakStrDup(stg, file, lno) strdup((stg))
#endif

extern void *_LeakMalloc(size_t size, char *file, int lno);
extern void *_LeakRealloc(void *old, size_t size, char *file, int lno);
extern void * _LeakCalloc(size_t nelem, size_t size, char *file, int lno);
extern void *_LeakStrDup(const char *stg, char *file, int lno);
extern void _LeakFree(void *ptr, char *file, int lno);
extern void LeakStop(void);

#endif
