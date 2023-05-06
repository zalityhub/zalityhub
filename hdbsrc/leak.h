
/*****************************************************************************

Filename: include/leak.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ifndef _LEAKLIB_H
#define _LEAKLIB_H


#if LEAK_DETECTION
#include "memory.h"
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

extern void    *_LeakMalloc(size_t size, char *file, int lno);
extern void    *_LeakRealloc(void *old, size_t size, char *file, int lno);
extern void    *_LeakCalloc(int nelem, size_t size, char *file, int lno);
extern void    *_LeakStrDup(const char *stg, char *file, int lno);
extern void     _LeakFree(void *ptr, char *file, int lno);
extern void     LeakStop(void);

#endif
