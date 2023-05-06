/*------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2015/02/02 18:57:06 $
 * $Header: /home/gssvc/cvsroot/gbs/lib/nx/leaklib.c,v 1.3 2015/02/02 18:57:06 hbray Exp $
 *
 $Log: leaklib.c,v $
 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


#define LEAK_DETECTION 0
#include "util.h"


static inline void*
CreateSignature(void *ptr)
{
  void **sig = (void**) ptr;

  *sig = ptr;
  ptr += sizeof(void *);
  return ptr;
}


static inline void *
VerifySignature(void *ptr)
{
  ptr -= sizeof(void*);
  void  **sig = (void**)ptr;

  if(*sig != ptr) {
    fprintf(stderr, "\nFTL Fatal error: Invalid signature in %p\n", ptr);
    exit(1);    // always exit
  }
  *sig = NULL;    // erase it

  return ptr;
}



#undef malloc
#undef calloc
#undef realloc
#undef free
#undef strdup



static FILE *MemLeakFile = NULL;



void
LeakStop(void)
{
  if (MemLeakFile != NULL)
    fclose(MemLeakFile);
  MemLeakFile = NULL;
}


static void
LeakExit(void)
{
  LeakStop();
}


static inline void
LeakOpenLog()
{
  if(MemLeakFile == NULL) {
    char  tmp[1024];
    sprintf(tmp, "leaklog");
    if((MemLeakFile = fopen(tmp, "w")) != NULL) {
      printf("Memory Leak detection is on\n");
      atexit(LeakExit);
    }
  }
}

int      FuncStackDepth;
int      FuncStackOverflow;
void    *FuncStack[];

static char*
GetFuncStack()
{
  static char    *stack = NULL;
  static int      stackDepth = 0;

  if (FuncStackDepth > stackDepth || stack == NULL) {
    stackDepth = FuncStackDepth;
    if (stack != NULL)
      free(stack);  // release previous area
    if ((stack = calloc(stackDepth + 1, 16)) == NULL)
      Fatal("Unable to alloc %d bytes", stackDepth * 16);
  }

  char  *out = stack;
  *out = '\0';

  for (int i = 0; i < FuncStackDepth; ++i)
    out += sprintf(out, ",%p", FuncStack[i]);
  return stack;
}


static inline void
LogAlloc(char *type, size_t size, void *ptr, char *file, int lno)
{
  static int  depth = 0; // prevent recursion

  if (depth > 0)
    return;     // no log
  ++depth;

  LeakOpenLog();
  if (MemLeakFile != NULL) {
    time_t          tod;
    struct tm      *ts;

    time(&tod);
    ts = localtime(&tod);
    fprintf(MemLeakFile, "%02d%02d%02d %02d:%02d:%02d %s.%d %s(%d)=%p",
      ts->tm_year - 100, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec, file, lno, type,
      (int) size, ptr);
    fprintf(MemLeakFile, "%s\n", GetFuncStack());
    fflush(MemLeakFile);
  }

  --depth;
}


static inline void
LogFree(void *ptr, char *file, int lno)
{
  static int   depth = 0; // prevent recursion

  if (depth > 0)
    return;     // no log
  ++depth;

  LeakOpenLog();
  if(MemLeakFile != NULL) {
    time_t          tod;
    struct tm      *ts;

    time(&tod);
    ts = localtime(&tod);
    fprintf(MemLeakFile, "%02d%02d%02d %02d:%02d:%02d %s.%d free()=%p\n",
      ts->tm_year - 100, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec, file, lno, ptr);
    fflush(MemLeakFile);
  }

  --depth;
}


void*
_LeakMalloc(size_t size, char *file, int lno)
{
  void   *ptr;

  if ((ptr = malloc(size + 16)) == NULL)
    return ptr;    // failed
  LogAlloc("malloc", size, ptr, file, lno);
  ptr = CreateSignature(ptr);
  return ptr;
}


void*
_LeakCalloc(int nelem, size_t size, char *file, int lno)
{
  void   *ptr;

  if ((ptr = calloc(nelem, size + 16)) == NULL)
    return ptr;    // failed
  LogAlloc("calloc", nelem * size, ptr, file, lno);
  ptr = CreateSignature(ptr);
  return ptr;
}

void*
_LeakRealloc(void *old, size_t size, char *file, int lno)
{
  void   *ptr;

  old = VerifySignature(old);
  if ((ptr = realloc(old, size + 16)) == NULL)
    return ptr;    // failed
  LogFree(old, file, lno);
  ptr = CreateSignature(ptr);
  char   tmp[132];

  sprintf(tmp, "realloc_%p", old);
  LogAlloc(tmp, size, ptr, file, lno);
  return ptr;
}


void*
_LeakStrDup(const char *stg, char *file, int lno)
{
  char   *ptr;
  int    len = (int)strlen((char *) stg);

  if ((ptr = _LeakMalloc(len + 1 + 16, file, lno)) == NULL)
    return NULL;   // failed
  memcpy(ptr, stg, len);
  ptr[len] = '\0';
  return ptr;
}


void
_LeakFree(void *ptr, char *file, int lno)
{
  ptr = VerifySignature(ptr);
  LogFree(ptr, file, lno);
  free(ptr);
}
