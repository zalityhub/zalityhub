/*****************************************************************************

Filename:	lib/nx/leaklib.c

Purpose:    This space for rent.

	leaklib.c is a simple memory leak detectory. It is implemented at the
	source code level, not at link time. Therefore each source module
	which uses the service must #include the file leaklib.h
	The .h file contains macros to oeverride malloc, free, calloc and strdup
	and convert these calls into references to the 'leak' versions.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:45 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/leaklib.c,v 1.3.4.2 2011/09/24 17:49:45 hbray Exp $
 *
 $Log: leaklib.c,v $
 Revision 1.3.4.2  2011/09/24 17:49:45  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/23 12:03:14  hbray
 revision 5.5

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:46  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: leaklib.c,v 1.3.4.2 2011/09/24 17:49:45 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/leaklib.h"


static inline void* CreateSignature(void *ptr)
{	// create signature
	void **sig = (void**)ptr;
	*sig = ptr;
	ptr += sizeof(void*);
	return ptr;
}


static inline void* VerifySignature(void *ptr)
{	// verify signature
	ptr -= sizeof(void*);
	void **sig = (void**)ptr;
	if ( *sig != ptr )
	{
		fwritef(1, "\n%s %d FTL %s Fatal error: Invalid signature in %p\n",
			SysLogTimeToString(), getpid(),
			NxCurrentProc?NxCurrentProc->name:"",
			ptr);
		kill(getpid(), SIGABRT);	// try for a core
		exit(1);					// always exit
	}
	*sig = NULL;		// erase it

	return ptr;
}



#undef printf
#undef puts
#undef putchar

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
	if (MemLeakFile == NULL)
	{
		if ( NxGlobal != NULL && (!NxGlobal->dummy) && NxCurrentProc != NULL && strlen(NxCurrentProc->name) > 0 )
		{
			char tmp[1024];
			sprintf(tmp, "%s/%s/alloc.log", NxGlobal->nxDir, NxCurrentProc->name);
			if ( (MemLeakFile = fopen(tmp, "w")) != NULL )
			{
				SysLog(LogWarn, "Memory Leak detection is on");
				atexit(LeakExit);
			}
		}
	}
}

extern int     FuncStackDepth;
extern int     FuncStackOverflow;
extern void    *FuncStack[];

static char*
GetFuncStack()
{
	static char *stack = NULL;
	static int	stackDepth = 0;

	if ( FuncStackDepth > stackDepth || stack == NULL )
	{
		stackDepth = FuncStackDepth;
		if ( stack != NULL )
			free(stack);		// release previous area
		if ( (stack = calloc(stackDepth+1, 16)) == NULL )
			NxCrash("Unable to alloc %d bytes", stackDepth*16);
	}

	char *out = stack;
	*out = '\0';

	for ( int i = 0; i < FuncStackDepth; ++i )
		out += sprintf(out, ",%p", FuncStack[i]);
	return stack;
}


static inline void
LogAlloc(char *type, size_t size, void *ptr, char *file, int lno)
{
	static int depth = 0;		// prevent recursion
	if ( depth > 0 )
		return;					// no log
	++depth;

	LeakOpenLog();
	if (MemLeakFile != NULL)
	{
		time_t tod;
		struct tm *ts;

		time(&tod);
		ts = localtime(&tod);
		fprintf(MemLeakFile, "%02d%02d%02d %02d:%02d:%02d %s.%d %s(%d)=%p",
				ts->tm_year - 100, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec, file, lno, type, (int)size, ptr);
		fprintf(MemLeakFile, "%s\n", GetFuncStack());
		fflush(MemLeakFile);
	}

	--depth;
}


static inline void
LogFree(void *ptr, char *file, int lno)
{
	static int depth = 0;		// prevent recursion
	if ( depth > 0 )
		return;					// no log
	++depth;

	LeakOpenLog();
	if (MemLeakFile != NULL)
	{
		time_t tod;
		struct tm *ts;

		time(&tod);
		ts = localtime(&tod);
		fprintf(MemLeakFile, "%02d%02d%02d %02d:%02d:%02d %s.%d free()=%p\n",
				ts->tm_year - 100, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec, file, lno, ptr);
		fflush(MemLeakFile);
	}

	--depth;
}


void *
_LeakMalloc(size_t size, char *file, int lno)
{
	void *ptr;

	if ((ptr = malloc(size+16)) == NULL)
		return ptr;				// failed
	ptr = CreateSignature(ptr);
	LogAlloc("malloc", size, ptr, file, lno);
	return ptr;
}


void *
_LeakCalloc(size_t nelem, size_t size, char *file, int lno)
{
	void *ptr;

	if ((ptr = calloc(nelem, size+16)) == NULL)
		return ptr;				// failed
	ptr = CreateSignature(ptr);
	LogAlloc("calloc", nelem*size, ptr, file, lno);
	return ptr;
}

void*
_LeakRealloc(void *old, size_t size, char *file, int lno)
{
	void *ptr;

	old = VerifySignature(old);
	if ((ptr = realloc(old, size+16)) == NULL)
		return ptr;				// failed
	ptr = CreateSignature(ptr);
	LogFree(old, file, lno);
	char tmp[132];
	sprintf(tmp, "realloc_%p", old);
	LogAlloc(tmp, size, ptr, file, lno);
	return ptr;
}


void *
_LeakStrDup(const char *stg, char *file, int lno)
{
	char *ptr;
	int len = strlen((char *)stg);

	if ((ptr = _LeakMalloc(len + 1+16, file, lno)) == NULL)
		return NULL;			// failed
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
