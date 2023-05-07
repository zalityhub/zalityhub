/*****************************************************************************

Filename:	lib/svcha/util.c

Purpose:	MICROS SVCHA XML Message Set

			Compliance with specification: MICROS Standard SVC Interface
			Revision 2.5 last updated on 4/05/2005

				ALL CODE/DATA WITHIN MUST BE RUN-TIME REENTRANT

			Utility methods needed to process MICROS SVCHA requests

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:23 $
 * $Header: /home/hbray/cvsroot/fec/lib/svcha/util.c,v 1.2 2011/07/27 20:22:23 hbray Exp $
 *
 $Log: util.c,v $
 Revision 1.2  2011/07/27 20:22:23  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:55  hbray
 Added cvs headers

 *

2009.10.30 joseph dionne		Created at release 5.9
*****************************************************************************/

#ident "@(#) $Id: util.c,v 1.2 2011/07/27 20:22:23 hbray Exp $ "

#include "data.h"

// Returns NULL when name not found, map->name equal to NULL if
// is name unknown, or pointer to the name-to-value map entry
NameToValueMap *
svchaFindMapByName(char *name, NameToValueMap * map)
{
	NameToValueMap *ret = 0;

	// Set errno and indicate usage error, i.e. return NULL pointer
	if (!(name) || !(*name) || !(map))
	{
		errno = EINVAL;
		return (NULL);
	}							// if (!(name) || !(*name) || !(map))

	ret = map;

	// Map is terminated by a NULL name pointer
	for (; ret->name;)
	{
		// Found name transalation map, return
		if (!strcasecmp(name, ret->name))
			break;

		// Advance to next map entry
		ret++;
	}							// for(;ret->name;)

	return (ret);
}								// NameToValueMap *svchaFindMapByName(char *name,NameToValueMap *map)

NameToValueMap *
svchaFindMapByValue(int value, NameToValueMap * map)
{
	NameToValueMap *ret = 0;

	// Set errno and indicate usage error, i.e. return NULL pointer
	if (0 >= value || !(map))
	{
		errno = EINVAL;
		return (NULL);
	}							// if (0 >= value || !(map))

	ret = map;

	// Map is terminated by a NULL name pointer
	for (; ret->name;)
	{
		// Found translation value map, return
		if (value == ret->value)
			break;

		// Advance to next map entry
		ret++;
	}							// for(;ret->name;)

	return (ret);
}								// NameToValueMap *svchaFindMapByValue(int value,NameToValueMap *map)

int
svchaParse(char *xmlBuf, int xmlLen, svchaCallBack_t cb, svchaXml_t *data)
{
	boolean quoted = false;
	char *ofs = xmlBuf;
	char *endBp = ofs + xmlLen;
	char *token = 0;
	int tokenLen = 0;

	// Validate arguments
	if (0 >= xmlLen || !(ofs) || !(cb) || !(data) || !(data->req) || !(data->extra))
	{
		errno = EINVAL;
		return (-1);
	}							// if (0 >= xmlLen || !(ofs) || !(cb) || !(data) ...

	// Parse the SVCHA packet and XML request document
	for (; ofs < endBp; ofs++)
	{
		char ch = *ofs;

		// Count the length in bytes of the current token
		if ((token))
			tokenLen++;

		// Appending quoted string
		if ((quoted) && '"' != ch)
			continue;

		switch (ch)
		{
			// Start of token character(s), or other non case character 
		case '<':
			{
				// Allow "empty" closing XML tag, i.e. "/>"
				token = ofs;
				tokenLen = 1;
			}
			break;

			// Detect quoted strings, maybe the closing quote
		case '"':
			quoted = !quoted;
			break;

			// Ending character(s), fall thru below is by design
		case ' ':				// White space terminates a token
		case '\t':				// Multiple white space bytes that
		case '\r':				// separate "tokens" are ignored
		case '\n':
			{
				if (!(token))
					break;
				else
					tokenLen--;
			}
		case '>':
			{					// Closing XML tag
				if ((token))
				{
					int len = tokenLen;

					if ('>' == token[len - 1])
						len--;
					if ('?' == token[len - 1])
						len--;

					// Just pass the last XML tag name or attribute
					cb(token, len, data);

					// Reset to pass the XML tag closing token(s)
					tokenLen -= len;
					token += len;

					// Passed white space terminated attribute
					// no need to call call back method again
					if (0 >= tokenLen)
					{
						token = 0;
						tokenLen = 0;
						break;
					}			// if (0 >= tokenLen)

					// Send end-tag-name to the call back method
					len = cb(token, tokenLen, data);

					// Return on call back method error return
					if (0 > len)
						return (len);

					// Advance buffer pointer len bytes
					ofs += len;

					// Reset after call back method
					token = 0;
					tokenLen = 0;
				}				// if ((token))
			}
			break;

			// Collect XML tag attribute(s)
		default:
			{
				if (!(token))
				{
					token = ofs;
					tokenLen = 1;
				}				// if (!(token))
			}
			break;
		}						// switch(ch)
	}							// for(;ofs < endBp;ofs++)

	// Convert any embedded commas to 0xFE
	for (ofs = data->extra + 3; *ofs; ofs++)
		if (',' == *ofs)
			*ofs = 0xFE;

	return (0);
}								// int svchaParse(char *xmlBuf,int xmlLen,svchaCallBack_t cb, ...
