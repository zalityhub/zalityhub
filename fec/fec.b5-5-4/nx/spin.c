/*****************************************************************************

Filename:   lib/nx/spin.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:47 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/spin.c,v 1.3.4.1 2011/09/24 17:49:47 hbray Exp $
 *
 $Log: spin.c,v $
 Revision 1.3.4.1  2011/09/24 17:49:47  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:48  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: spin.c,v 1.3.4.1 2011/09/24 17:49:47 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/spin.h"


struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((struct __xchg_dummy *)(x))


static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
   switch (size) {
	  case 1:
		 __asm__ __volatile__("xchgb %b0,%1"
			:"=q" (x)
			:"m" (*__xg(ptr)), "0" (x)
			:"memory");
		 break;
	  case 2:
		 __asm__ __volatile__("xchgw %w0,%1"
			:"=r" (x)
			:"m" (*__xg(ptr)), "0" (x)
			:"memory");
		 break;
	  case 4:
		 __asm__ __volatile__("xchgl %0,%1"
			:"=r" (x)
			:"m" (*__xg(ptr)), "0" (x)
			:"memory");
		 break;
   }
   return x;
}

/*
* Atomic compare and exchange.  Compare OLD with MEM, if identical,
* store NEW in MEM.  Return the initial value in MEM.  Success is
* indicated by comparing RETURN with OLD.
*/

static inline unsigned long __cmpxchg(volatile void *ptr, unsigned long old,
				  unsigned long new, int size)
{
   unsigned long prev;
   switch (size) {
   case 1:
	  __asm__ __volatile__("lock ; " "cmpxchgb %b1,%2"
				 : "=a"(prev)
				 : "q"(new), "m"(*__xg(ptr)), "0"(old)
				 : "memory");
	  return prev;
   case 2:
	  __asm__ __volatile__("lock ; " "cmpxchgw %w1,%2"
				 : "=a"(prev)
				 : "q"(new), "m"(*__xg(ptr)), "0"(old)
				 : "memory");
	  return prev;
   case 4:
	  __asm__ __volatile__("lock ; " "cmpxchgl %1,%2"
				 : "=a"(prev)
				 : "q"(new), "m"(*__xg(ptr)), "0"(old)
				 : "memory");
	  return prev;
   }
   return old;
}

#define _cmpxchg(ptr,o,n)\
   ((__typeof__(*(ptr)))__cmpxchg((ptr),(unsigned long)(o),\
			   (unsigned long)(n),sizeof(*(ptr))))


void
SpinInit(Spin_t *lock)
{
	*lock = 0;
}


void
SpinLock(Spin_t *lock)
{
	do
	{
	} while(_cmpxchg(lock, 0, 1) != 0);
}


void
SpinUnlock(Spin_t *lock)
{
	*lock = 0;
}
