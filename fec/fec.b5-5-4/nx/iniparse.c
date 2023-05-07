/*****************************************************************************

Filename:   lib/nx/iniparse.c

Purpose:	Methods for reading and parsing Java property configuration
			file entries.  This will work for most INI file entries as
			well, but does not support the concepts of INI "sections."

			int parseIni(char *entry,char token[],char value[]);

				Parse the property/INI entry in the format of
			'token = "value"' into token and value.  Whitespace and/or
			double quotes are invalud for the entries token component
			but are allowed in the quoted value component.
				Double quotes can be used to include whitespace in the
			value component, but are not required.  Trailing comments
			that follow after the value are removed.

			int readIni(FILE *propFp,char token[],char value[]);

				Read the properties from propFp, ignoring comments and
			blank lines.  Return true if a valid property (INI) entry
			is parsed, or false on EOF and/or file read error.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:18 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/iniparse.c,v 1.2 2011/07/27 20:22:18 hbray Exp $
 *
 $Log: iniparse.c,v $
 Revision 1.2  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:46  hbray
 Added cvs headers

 *

2009.06.03 joseph dionne		Updated to support release 3.4
*****************************************************************************/

#ident "@(#) $Id: iniparse.c,v 1.2 2011/07/27 20:22:18 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"



// Parse the property "token=value" entry into its components
int
parseIni(char *buf, char token[], char value[])
{
	char *bp;
	char *cp;

	// Clear previous token/value
	*token = *value = 0;

	// Strip off leading whitespace
	for (bp = buf; (*bp); bp++)
		if (!isspace(*bp))
			break;

	// Check for comment or "blank" lines
	if (!(*bp) || strchr(";#:/", *bp))
		return (false);

	// Check for in-line comments
	if ((cp = strrchr(bp, '#')))
	{
		// Terminate line at the comment character '#'
		*cp-- = 0;

		// Remove any whitespace preceding the comment character
		for (; bp < cp; cp--)
		{
			if (!(strchr(" \t", *cp)))
				break;			// Non whitespace character
			*cp = 0;
		}
	}

	// Scan off the token and value
	if ((cp = strchr(bp, '"')) && ('"' != *(cp - 1) || *(cp - 1) != '\\'))
		sscanf(bp, "%[^ \t=\"\r\n]%*[ \t=]%*[\"]%[^#\"\t\r\n]s", token, value);
	else
		sscanf(bp, "%[^ \t=\r\n]%*[ \t=]%[^#\t\r\n]s", token, value);

	// Fold token to lowercase
	for (bp = token; (*bp); bp++)
	{
		int ch = *bp;

		ch = tolower(ch);
		*bp = ch;
	}

	return (true);
}

// Read and parse the property file
int
readIni(FILE * iniFp, char token[], char value[])
{
	char buf[4096] = { 0 };

	for (; fgets(buf, sizeof(buf), iniFp);)
	{
		char *bp = buf + strlen(buf);

		// Remove newline byte(s)
		for (; buf < bp--;)
		{
			if ('\r' == *bp || *bp == '\n')
				*bp = 0;
			else
				break;
		}

		// Blank line, or at least way too short
		if (buf == bp)
			continue;

		// Parse into token and value fields
		if (!parseIni(buf, token, value))
			continue;

		return (true);
	}

	return (false);
}								// int readIni(FILE *iniFp,char token[],char value[])
