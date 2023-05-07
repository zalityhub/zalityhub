/*****************************************************************************

Filename:	lib/nx/instrument.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


#define __USE_GNU
#include <dlfcn.h>
 
typedef enum { false = 0, off = false, true, on = true } boolean ;
#include "include/bt.h"



static char*
GetName(void *fnc)
{
	static BtNode_t *symRoot = NULL;
	static void *dlHandle = NULL;

	char *name = NULL;

	if ( dlHandle == NULL )
		dlHandle = dlopen(NULL, RTLD_NOW);
	Dl_info info;

	BtRecord_t	*rec = BtFind(symRoot, (unsigned long)fnc, false);
	if ( rec == NULL )
	{
		if ( dladdr(fnc, &info) != 0 )
		{
			name = strdup(info.dli_sname);
			symRoot = BtInsert(symRoot, (unsigned long)fnc, (unsigned long)name);
			rec = BtFind(symRoot, (unsigned long)fnc, false);
		}
	}
	else
	{
		name = (char*)rec->value;
	}

	if ( name == NULL )
	{
		static char tmp[16];
		name = tmp;
		sprintf(name, "%p", fnc);
	}

	return name;
}


#define MaxStackSize 1024
int		FuncStackDepth = 0;
int		FuncStackOverflow = 0;
void	*FuncStack[MaxStackSize+2];


void
__cyg_profile_func_enter(void *fnc, void *call)
{
	if ( FuncStackDepth < MaxStackSize )
	{
		FuncStack[FuncStackDepth++] = GetName(fnc);
		FuncStack[FuncStackDepth] = NULL;
	}
	else
	{
		++FuncStackOverflow;
	}
}


void
__cyg_profile_func_exit(void *fnc, void *call)
{
	if ( FuncStackOverflow > 0 )
		--FuncStackOverflow;
	else
		FuncStack[--FuncStackDepth] = NULL;
}
