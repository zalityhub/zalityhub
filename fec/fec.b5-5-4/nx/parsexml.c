/*****************************************************************************

Filename:	lib/nx/parsexml.c

Purpose:	Simple XML message container parse methods.  These methods are
			in compliance with the following W3C specification.

				Extensible Markup Language (XML) 1.0 (Third Edition)
						W3C Recommendation 04 February 2004

			Copyright © 2004 W3C® (MIT, ERCIM, Keio), All Rights Reserved.
			W3C liability, trademark, document use and software licensing
			rules apply.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)

YYYY.MM.DD --- developer ---	----------------- Comments -------------------
2011.10.08 joseph dionne		Add support for Empty XML Tags
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>



// Local (file) scope variables
static char *fmt = "%*[\t <fieldnamFIELDNAM]%*[=\"]%[^/> \t\"]%*[ \t\"/]>%[^<]";

int
parseXmlField(char *xml, char *name, char *data, int trim)
{
	char na[512] = { 0 };
	char *np = na;
	char da[2 * 1024] = { 0 };
	char *dp = da;
	int ii;

	// Validate arguments
	if (!(xml) || !(*xml) || !(name) || !(data))
		return (-1);

	// Advance past leading whitespace
	for (; isspace(*xml); xml++) ;

	// Scan off the XML field name and data
	ii = sscanf(xml, fmt, np, dp);
	if ((1 != ii) && (2 != ii) && (!strstr(xml, "><")))
	{
		return (-1);
	}							// if (2 != (ii = sscanf(xml,fmt,np,dp)) && (!strstr(xml,"><")))

	// Remove double quotes from the field name
	if ('"' == *np)
	{
		np++;
		*(strchr(np, '"')) = 0;
	}							// if ('"' == *np)

	// Remove leading whitespace from the data
	if ((trim))
		for (; isspace(*dp); dp++) ;

	// Copy the XML Field name
	sprintf(name, "%s", np);

	// Copy the XML Field data
	sprintf(data, "%s", dp);

	return (0);
}								// int parseXmlField(char *xml,char *name,char *data,int trim)
