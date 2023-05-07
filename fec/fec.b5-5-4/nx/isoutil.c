/*****************************************************************************

Filename:	lib/nx/isoutil.c

Purpose:	Utility methods for working with ISO data packets

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:18 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/isoutil.c,v 1.2 2011/07/27 20:22:18 hbray Exp $
 *
 $Log: isoutil.c,v $
 Revision 1.2  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:46  hbray
 Added cvs headers

 *
2009.06.26 joseph dionne		Created release 3.4
*****************************************************************************/

#ident "@(#) $Id: isoutil.c,v 1.2 2011/07/27 20:22:18 hbray Exp $ "

#include <include/isoutil.h>

// Local scope variable declaration

// Initialize the isobitmap_t structure from the ISO bitmap data
int
setIsoBitMap(isobitmap_t *isobit, char *bitmap)
{
	if (!(isobit) || !(bitmap))
	{
		errno = EINVAL;
		return (-1);
	}							// if (!(isobit) || !(bitmap))

	isobit->map.hi.dword = ntohl(*(unsigned long *)&bitmap[0]);
	isobit->map.lo.dword = ntohl(*(unsigned long *)&bitmap[4]);

	return (0);
}								// int setIsoBitMap(isobitmap_t *isobit,char *bitmap)
