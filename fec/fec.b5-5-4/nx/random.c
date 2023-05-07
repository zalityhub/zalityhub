/*****************************************************************************

Filename:   lib/nx/random.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:19 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/random.c,v 1.2 2011/07/27 20:22:19 hbray Exp $
 *
 $Log: random.c,v $
 Revision 1.2  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:47  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: random.c,v 1.2 2011/07/27 20:22:19 hbray Exp $ "


#include <stdio.h>
#include <stdlib.h>




int
RandomNbr(void)
{

	return rand();
}


/*+
	Name:
		RandomRange - Generate a random number within a specified range

	Synopsis:
		int RandomRange (min, max)
		int		min;
		int		max;

	Description:
		This function will generate a random number that is within the
		range of >= min <= max.

	Diagnostics:
		None.
-*/

int
RandomRange(int min, int max)
{

	int r = rand();

	if (min == max)
		return r;				/* standard call, no scaling is desired */

	double d = (((double)r) / ((double)RAND_MAX));	// makes it 0 - .999

	r = (int)((d * (double)(max - min) + (double)min) + 0.5);
	return r;
}


/*+
	Name:
		RandomBoolean - Generate a random TRUE/FALSE based upon a percentage

	Synopsis:
		int RandomBoolean (percentage)
		int		percentage;

	Description:
		This routine will return a random TRUE or a FALSE within a specified
		probability.  For example: if you call RandomBoolean (75)
		100 consecutive times, you should get approximately 75 TRUEs
		and 25 FALSEs.

	Diagnostics:
		None.
-*/

int
RandomBoolean(int percentage)
{

	return (RandomRange(1, 100) <= percentage);
}


#if 0
#include <sys/time.h>

int
main()
{

	{
		struct timeval tv;
		unsigned long long t;

		gettimeofday(&tv, NULL);
		t = ((unsigned long long)tv.tv_usec);
		t += (((unsigned long long)tv.tv_sec) * 1000000L);
		srand(t);
	}

	for (int i = 0; i < 100; ++i)
	{
		printf("%d: %s\n", i, RandomBoolean(75) ? "true" : "false");
	}

	return 0;
}
#endif
