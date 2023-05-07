/*****************************************************************************

Filename:   lib/nx/itoa.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:45 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/itoa.c,v 1.2.4.1 2011/09/24 17:49:45 hbray Exp $
 *
 $Log: itoa.c,v $
 Revision 1.2.4.1  2011/09/24 17:49:45  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:46  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: itoa.c,v 1.2.4.1 2011/09/24 17:49:45 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"


char*
IntegerToString(int value, char *result)
{
	int base = 10;
	StringArrayStatic(sa, 16, 16);

	if ( result == NULL )
		result = StringArrayNext(sa)->str;

	char *ptr = result, *ptr1 = result, tmp_char;
	unsigned tmp_value;

	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0)
		*ptr++ = '-';
	*ptr-- = '\0';

// Reverse result
	while (ptr1 < ptr)
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}

	return result;
}
