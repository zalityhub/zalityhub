/*****************************************************************************

Filename:   include/fullxml.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/07/27 20:22:11 $
 * $Header: /home/hbray/cvsroot/fec/include/fullxml.h,v 1.2 2011/07/27 20:22:11 hbray Exp $
 *
 $Log: fullxml.h,v $
 Revision 1.2  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.1.1.1.6.1  2011/07/27 20:19:34  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: fullxml.h,v 1.2 2011/07/27 20:22:11 hbray Exp $ "


#ifndef __FULL_XML_H__
#define __FULL_XML_H__

typedef struct XmlAttributePair
{
	char *name;
	char *value;
} XmlAttributePair;


// src = start of xml data
// tag = tag name (sans <>)
// value = where to put the extracted value
// maxValueLen = how much data do you want
// attributeList = pointer to an array of pointers where pointers to attributes will be placed.
//   The returned list of pointers (NULL terminated) are allocated from the heap and must be free'ed when done.
// set to null if you don't want attributes

// Returns first character beyond the end of tag (or NULL if tag not found)
extern char *GetXmlTagValue(char *src, char *tag, char *value, int maxValueLen, XmlAttributePair ** attributeList[]);

extern char *GetNextXmlTagValue(char *src, char *tagptr, int maxTagLen, char *value, int maxValueLen, XmlAttributePair ** attributeList[]);

extern void FreeXmlAttributes(XmlAttributePair ** attributeList[]);

#endif
