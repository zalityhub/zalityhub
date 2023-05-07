/*****************************************************************************

Filename:	lib/svcha/xml2arr.c

Purpose:	Parse an XML Document Tags into an array of char pointers of tags

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:23 $
 * $Header: /home/hbray/cvsroot/fec/lib/svcha/xml2arr.c,v 1.2 2011/07/27 20:22:23 hbray Exp $
 *
 $Log: xml2arr.c,v $
 Revision 1.2  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:55  hbray
 Added cvs headers

 *

2009.10.30 joseph dionne		Created at release 5.9
*****************************************************************************/

#ident "@(#) $Id: xml2arr.c,v 1.2 2011/07/27 20:22:23 hbray Exp $ "

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

int
xmlTagCount(char *xml)
{
	int fields = 0;
	char *lp;
	char *bp;

	// Count the number of array entries needed
	for (lp = (bp = xml); (bp = strstr(bp, "><"));)
	{
		int len = bp - lp;

		// Empty tag, count as one field
		if ('/' == bp[2])
		{
			if (0 == strncmp(lp + 1, bp + 3, len - 1))
			{
				bp = strstr(bp + 3, "><");
				continue;
			}					// if (0 == strncmp(lp+1,bp+3,len-1))
		}						// if ('/' == bp[2])

		// Count tag
		fields++;

		// Advance past end tag sentinel (">")
		bp++;
	}							// for(lp=(bp=xml);(bp = strstr(bp,"><"));)
	if ((fields))
		fields++;

	return (fields);
}								// int xmlTagCount(char *xml)

char **
xmlToArray(char *xml, char **arr)
{
	char *aEntry = (char *)arr;
	int xmlLen = (xml) ? strlen(xml) : -1;
	int fields = 0;
	char *lp = 0;
	char *bp;
	int ii;

	// Validate arguments
	if (0 >= xmlLen || !(arr))
	{
		errno = EINVAL;
		return (NULL);
	}							// if (0 >= xmlLen || !(arr))

	// Count the number of array entries needed
	if (0 >= (fields = xmlTagCount(xml)))
		return (NULL);

	// Point past the array of char pointers
	// NOTE: aEntry begins on an even byte boundry
	if ((fields & 1))
		aEntry += sizeof(char *) * (1 + fields);
	else
		aEntry += sizeof(char *) * (2 + fields);

	// Parse the XML document into the array
	arr[0] = 0;
	lp = xml;
	bp = strstr(xml, "><");
	for (ii = 0; (bp); ii++)
	{
		int len = 1 + bp - lp;

		// Empty tag, do not insert <Newline>
		if ((ii) && '/' == bp[2])
		{
			if (0 == strncmp(lp + 1, bp + 3, len - 1))
			{
				bp = strstr(bp + 3, "><");
				continue;
			}					// if (0 == strncmp(lp+1,bp+3,len-1))
		}						// if ((ii) && '/' == bp[2])

		// Parse the XML tags into NULL terminated array
		len = sprintf(aEntry, "%.*s%c", len, lp, 0);
		lp = ++bp;
		arr[ii] = aEntry;
		aEntry += len;
		arr[1 + ii] = 0;

		bp = strstr(bp, "><");
	}							// for(ii=0;(bp);ii++)
	sprintf(aEntry, "%s", lp);
	arr[ii] = aEntry;

	return (arr);
}								// char **xmlToArray(char *xml,char **arr)
