/*****************************************************************************

Filename:   lib/nx/fullxml.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:56 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/fullxml.c,v 1.2.4.1 2011/10/27 18:33:56 hbray Exp $
 *
 $Log: fullxml.c,v $
 Revision 1.2.4.1  2011/10/27 18:33:56  hbray
 Revision 5.5

 Revision 1.2  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: fullxml.c,v 1.2.4.1 2011/10/27 18:33:56 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"


#include "include/fullxml.h"



static char *WhiteSpace = "\r\n\t ";


// flattens the text by ignoring characters in the ignoreList
// returns a pointer after ignoring concurrent chars which appear in the ignoreList
// note: it could return the same pointer as the input pointer (ie: the first char is not ignorable)
static char *
IgnoreChars(char *src, char *ignoreList)
{
	while (*src &&				// if we're not at the end
		   strchr(ignoreList, *src) != NULL)	// and this is an ignorable ch
	{
		++src;					// next ch
	}

	return src;
}


static char *
CollectString(char **ptr)
{
	char tmp[1024];
	char *tptr = tmp;
	int maxLen = sizeof(tmp) - 1;
	int tlen = 0;
	char *iptr = *ptr;

	if (*iptr)
	{
		char ch = *iptr++;
		char delim = ch;

		do
		{
			if ((ch = *iptr++) == '\0')
				break;			// hit end of input

			if (ch != '\\')		/* just a regular ch */
			{
				if (ch != delim)
				{
					*tptr++ = (char)ch;
					if (++tlen >= maxLen)
					{
						break;
					}
				}
			}
			while (ch == '\\')	/* escape */
			{
				if ((ch = *iptr++) == '\0')
				{
					break;
				}
				*tptr++ = (char)ch;
				if (++tlen >= maxLen)
				{
					break;
				}
				if ((ch = *iptr++) == '\0')
				{
					break;
				}
				*tptr++ = (char)ch;
				if (++tlen >= maxLen)
				{
					break;
				}
			}					/* escaped ch */
		} while (ch != delim);
	}

	*tptr = '\0';				// terminate the string
	*ptr = iptr;				// the final ch
	return strdup(tmp);
}


// src = start of xml data
// tag = tag name (sans <>)
// fptr = receives first char of tag's content
// lptr = receives last char of tag's content
// aptr = receives pointer to attribute content
//        receives a malloc'ed string which contains an extract/copy of the attribute area
//        returns NULL if no attributes are present
//        if not NULL, this memory must be freed

// Returns length of content: -1 if tag not found (or is otherwise mangled), 0 if no content, > 0 if content
static int
GetXmlTagPointers(char *src, char *tag, char **fptr, char **lptr, char **aptr)
{
	int tlen = strlen(tag);
	char *stag = alloca(tlen + 3);
	char *etag = alloca(tlen + 4);
	int len = -1;
	char *tptr;


	sprintf(stag, "<%s", tag);
	sprintf(etag, "</%s>", tag);

	if ((tptr = stristr(src, stag)) != NULL)
	{
		tptr += strlen(stag);	// to start of content (possibly nothing)...
		tptr = IgnoreChars(tptr, WhiteSpace);
		if (aptr != NULL)
			*aptr = tptr;

		while (*tptr != '\0' && *tptr != '>')	// not at closure
		{
			++tptr;
			if (*tptr == '<')
			{
				SysLog(LogError, "Bumped into open tag... probable malformed element: %s", src);
				break;
			}
		}

		if (*tptr == '>')		// found the close
		{
			if (aptr)			// collecting attributes for this tag
			{
				int alen = tptr - *aptr;

				if (alen > 0)
				{
					char *a = malloc(alen + 1);

					memcpy(a, *aptr, alen);
					a[alen] = '\0';	// terminate attributes
					*aptr = a;
				}
				else
				{
					*aptr = NULL;	// no attribute
				}
			}

			if (tptr[-1] == '/')	// this is a 'self' tag termination
			{
				len = 0;		// no content
				++tptr;			// one beyond terminating '>'
				*fptr = *lptr = "";	// nothing
			}
			else
			{
				++tptr;			// point to content
				char *cptr;

				if ((cptr = stristr(tptr, etag)))	// locate the end of the tag...
				{
					*fptr = tptr;
					*lptr = cptr;
					len = (cptr - tptr);
				}
			}
		}
	}

	return len;
}


// src = start of xml data
// tag = start of a tag, i.e. pointing to a '<' char.
// fptr = receives first char of tag's content
// lptr = receives last char of tag's content
// aptr = receives pointer to attribute content
//        receives a malloc'ed string which contains an extract/copy of the attribute area
//        returns NULL if no attributes are present
//        if not NULL, this memory must be freed

// Returns length of content: -1 if tag not found (or is otherwise mangled), 0 if no content, > 0 if content
static int
GetNextXmlTagPointers(char *src, char *tag, int maxTagLen, char **fptr, char **lptr, char **aptr)
{
	char *tptr = IgnoreChars(src, WhiteSpace);	// start after whitespace

// Ok, this is what we're going to do...
// src ought to be pointing to a '<' char... so, collect all the chars which follow
// until you hit a '>' or whitespace... that should be the name of the tag. Once we have
// the name of the tag, we can call our friend
//      getTagPointers
// and let him do the heavy lifting...

	if (*tptr != '<')
		return -1;				// wrong...

	char *ptr = tag;

	++tptr;
	while (maxTagLen > 0)		// while we have room...
	{
		if (*tptr == '\0' || *tptr == '>' || strchr(WhiteSpace, *tptr) != NULL)
		{
			break;				// end of the tag
		}
		*ptr++ = *tptr++;
	}
	*ptr = '\0';				// terminate tag name

	if (tag[0] == '\0')
		return -1;				// did not find a tag

	return GetXmlTagPointers(src, tag, fptr, lptr, aptr);
}


// Pointer to attribute area (may contain no, or multiple attributes)
// attributeList = pointer to an array of pointers where pointers to attributes will be placed.
//   The returned list of pointers (NULL terminated) are allocated from the heap and must be freed when done.
// set to null if you don't want attributes

// Returns number of attributes extracted
static int
GetXmlAttributeList(char *aptr, XmlAttributePair ** attributeList[])
{
	int an = 0;

	if (aptr && attributeList)	// have attibutes...
	{
		static const int MaxXmlAttributes = 1024;
		XmlAttributePair *atList[MaxXmlAttributes];

		char *tptr = IgnoreChars(aptr, WhiteSpace);	// start of first attribute

		for (an = 0; *tptr;)
		{
			static char *stopList = "=\t ";
			char *name = IgnoreChars(tptr, WhiteSpace);	// start of attribute name

			tptr = name;
			while (strchr(stopList, *tptr) == NULL)
				++tptr;			// locate end of name
			int nlen = tptr - name;	// length of name

			if (nlen > 0)
			{
				XmlAttributePair *at = malloc(sizeof(at));	// create an attribute (must be freed)

				at->name = at->value = NULL;
				at->name = malloc(nlen + 1);	// and its name (must be freed)
				memcpy(at->name, name, nlen);
				at->name[nlen] = '\0';	// terminate the name

				while (*tptr && *tptr != '=')	// look for equal
					++tptr;

				// now get the attribute's value
				if (*tptr == '=')	// might have a value
				{
					tptr = IgnoreChars(tptr + 1, WhiteSpace);
					char *value = CollectString(&tptr);

					if (value)
						at->value = value;	// this memory must be freed
				}

				if (at->value == NULL)	// did not get a value, create an empty one
					at->value = strdup("");	// this also must be freed

				if (an < MaxXmlAttributes)
				{
					// add this attribute to the array
					atList[an++] = at;
				}
			}
		}

		// finally copy the fixed attribute list to a dynamic array
		{
			*attributeList = malloc(((an + 1) * sizeof(XmlAttributePair *)));	// (must be freed)
			int a;

			for (a = 0; a < an; ++a)
			{
				(*attributeList)[a] = atList[a];
			}
			(*attributeList)[a] = NULL;	// terminate the array
		}
	}

	return an;
}


void
FreeXmlAttributes(XmlAttributePair ** attributeList[])
{

	if (attributeList != NULL)
	{
		XmlAttributePair **atList = *attributeList;

		if (atList != NULL)
		{
			int an;

			for (an = 0; atList[an] != NULL; ++an)
			{
				XmlAttributePair *at = atList[an];

				free(at->name);
				free(at->value);
				free(at);
				atList[an] = NULL;	// freed
			}

			free(atList);
		}
	}
}


// src = start of xml data
// tag = tag name (sans <>)
// value = where to put the extracted value
// maxValueLen = how much data do you want
// attributeList = pointer to an array of pointers where pointers to attributes will be placed.
//   The returned list of pointers (NULL terminated) are allocated from the heap and must be freed when done.
//   FreeXmlAttributes is a good way to accomplish this.
// set to null if you don't want attributes

// Returns first character beyond the end of tag (or NULL if tag not found)
char *
GetXmlTagValue(char *src, char *tag, char *value, int maxValueLen, XmlAttributePair ** attributeList[])
{
	char *fptr;
	char *lptr;
	char *aptr = NULL;

	int len = GetXmlTagPointers(src, tag, &fptr, &lptr, (attributeList != NULL) ? &aptr : NULL);

	if (len < 0)
	{
		return NULL;			// no tag
	}

	if (attributeList)			// if attributes are requested, set the default return of 'none'.
	{
		*attributeList = malloc(sizeof(XmlAttributePair *));	// must be freed
		(*attributeList)[0] = NULL;	// end...
	}

	if (aptr)					// have attibutes...
	{
		free(*attributeList);	// release the dummy one allocated above
		GetXmlAttributeList(aptr, attributeList);
		free(aptr);
	}

	if (value != NULL)
	{
		len = (len > maxValueLen) ? maxValueLen : len;
		memcpy(value, fptr, len);
		value[len] = '\0';
	}
	return (lptr + strlen(tag) + 3);
}


// src = start of xml data
// tag = tag name (sans <>)
// value = where to put the extracted value
// maxValueLen = how much data do you want
// attributeList = pointer to an array of pointers where pointers to attributes will be placed.
//   The returned list of pointers (NULL terminated) are allocated from the heap and must be freed when done.
//   FreeXmlAttributes is a good way to accomplish this.
// set to null if you don't want attributes

// Returns first character beyond the end of tag (or NULL if tag not found)
char *
GetNextXmlTagValue(char *src, char *tag, int maxTagLen, char *value, int maxValueLen, XmlAttributePair ** attributeList[])
{
	char *tptr;
	char *fptr;
	char *lptr;
	char *aptr = NULL;

	// scan the input looking for a leading tag char '<'

	// we need to skip the potential leading <?xml> tag in the input text... but, this may
	// NOT be the first time in here; so the input pointer might not point to the start of the buffer... we could be
	// just about anywhere... solution: skip ANY <?xml> tag seen... regardless of its location in the text.
	for (tptr = IgnoreChars(src, WhiteSpace); *tptr; tptr = IgnoreChars(tptr, WhiteSpace))
	{
		if (*tptr != '<')		// first non-whitespace is not a tag char... this is not good...
			return NULL;		// no tag

		if (tptr[1] == '!')		// this is an xml comment
		{
			if ((tptr = strstr(tptr, "-->")) == NULL)	// find the tail
			{
				tptr = &src[strlen(src)];	// no tail, point to end... and fail out of here
				break;
			}

			tptr = IgnoreChars(tptr + 3, WhiteSpace);	// skip over "-->"
			continue;			// look again
		}

		if (strncmp(tptr, "<?xml ", 6) != 0)
			break;				// not '<?xml... and we're on a '<' char

		if ((tptr = strstr(tptr, "?>")) == NULL)	// find the tail
		{
			tptr = &src[strlen(src)];	// no tail, point to end... and fail out of here
			break;
		}
		tptr = IgnoreChars(tptr + 2, WhiteSpace);	// skip over "?>"
	}

	if (*tptr != '<')			// did not find a tag
		return NULL;

	int len = GetNextXmlTagPointers(tptr, tag, maxTagLen, &fptr, &lptr, (attributeList != NULL) ? &aptr : NULL);

	if (len < 0)
		return NULL;			// no tag

	if (attributeList)			// if attributes are requested, set the default return of 'none'.
	{
		*attributeList = malloc(sizeof(XmlAttributePair *));	// must be freed
		(*attributeList)[0] = NULL;	// end...
	}

	if (aptr && strlen(aptr) > 0)	// have attibutes...
	{
		free(*attributeList);	// release the dummy one allocated above
		if (len == 0 && aptr[strlen(aptr) - 1] == '/')	// we had an empty value
			aptr[strlen(aptr) - 1] = '\0';	// remove close of tag...
		GetXmlAttributeList(aptr, attributeList);
		free(aptr);
	}

	if (strlen(tag) > 0 && tag[strlen(tag) - 1] == '/')	// we had an empty value
		tag[strlen(tag) - 1] = '\0';	// remove close of tag...

	len = (len > maxValueLen) ? maxValueLen : len;
	memcpy(value, fptr, len);
	value[len] = '\0';
	return (lptr + strlen(tag) + 3);
}


#if 0

#undef printf
#undef puts
#undef putchar

void
TestParseAll(char *src, int recursive)
{
	char tag[16384];
	char value[16384];
	XmlAttributePair **attributeList;

	char *sptr;

	for (sptr = src; (sptr = GetNextXmlTagValue(sptr, tag, sizeof(tag), value, sizeof(value), &attributeList)) != NULL;)
	{
		// printf("%s=%s\n", tag, value);
		printf("%s\n", tag);

		if (recursive && strchr(value, '<') != NULL)
			TestParseAll(value, recursive);

		if (0)
		{
			// Display tags attributes
			if (attributeList != NULL)
			{
				puts("Attributes:");
				int an;

				for (an = 0; attributeList[an] != NULL; ++an)
				{
					XmlAttributePair *at = attributeList[an];

					printf("%s=%s\n", at->name, at->value);
				}
				puts("");

				FreeXmlAttributes(&attributeList);
			}
		}
	}
}


int
main(int argc, char *argv[])
{
	char src[1024 * 1024];
	int len;


	len = read(fileno(stdin), src, sizeof(src));	// do one giant read

	if (len > 0)
	{
		src[len] = '\0';

		if (0)
		{
			char SVCMessage[16384];
			char Track2[16384];
			XmlAttributePair **attributeList;

			if (GetXmlTagValue(src, "SVCMessage", SVCMessage, sizeof(SVCMessage), &attributeList) != NULL)
			{
				printf("SVCMessage=%s\n", SVCMessage);

				// Display tags attributes
				if (attributeList != NULL)
				{
					puts("Attributes:");
					for (int an = 0; attributeList[an] != NULL; ++an)
					{
						XmlAttributePair *at = attributeList[an];

						printf("%s=%s\n", at->name, at->value);
					}
					puts("");
				}
			}
			FreeXmlAttributes(&attributeList);

			if (GetXmlTagValue(SVCMessage, "Track2", Track2, sizeof(Track2), &attributeList) != NULL)
			{
				printf("Track2=%s\n", Track2);

				// Display tags attributes
				if (attributeList != NULL)
				{
					puts("Attributes:");
					for (int an = 0; attributeList[an] != NULL; ++an)
					{
						XmlAttributePair *at = attributeList[an];

						printf("%s=%s\n", at->name, at->value);
					}
					puts("");
				}
				FreeXmlAttributes(&attributeList);
			}
		}

		if (1)
		{
			TestParseAll(src, 1);
		}
	}

	return 0;
}
#endif
