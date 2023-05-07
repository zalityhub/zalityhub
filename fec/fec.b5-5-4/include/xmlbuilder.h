/*****************************************************************************

Filename:   include/xmlbuilder.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/09/24 17:49:39 $
 * $Header: /home/hbray/cvsroot/fec/include/xmlbuilder.h,v 1.3.4.1 2011/09/24 17:49:39 hbray Exp $
 *
 $Log: xmlbuilder.h,v $
 Revision 1.3.4.1  2011/09/24 17:49:39  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:38  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: xmlbuilder.h,v 1.3.4.1 2011/09/24 17:49:39 hbray Exp $ "


#ifndef _XMLBUILDER_H
#define _XMLBUILDER_H

typedef struct XmlBuilder_t
{
	String_t		*text;
	ObjectList_t	*openTags;
} XmlBuilder_t;


#define XmlBuilderNew() ObjectNew(XmlBuilder)
#define XmlBuilderVerify(var) ObjectVerify(XmlBuilder, var)
#define XmlBuilderDelete(var) ObjectDelete(XmlBuilder, var)


extern XmlBuilder_t* XmlBuilderConstructor(XmlBuilder_t *this, char *file, int lno);
extern void XmlBuilderDestructor(XmlBuilder_t *this, char *file, int lno);
extern char* XmlBuilderToString(XmlBuilder_t *this);
extern struct Json_t* XmlBuilderSerialize(XmlBuilder_t *this);

extern char* XmlBuilderOpenTag(XmlBuilder_t *this, char *tag, char *fmt, ...);
extern char* XmlBuilderOpenTagV(XmlBuilder_t *this, char *tag, char *fmt, va_list ap);
extern int XmlBuilderCloseTag(XmlBuilder_t *this);
extern int XmlBuilderCloseAllTags(XmlBuilder_t *this);
extern char* XmlBuilderAddTag(XmlBuilder_t *this, char *tag, char *fmt, ...);
extern char* XmlBuilderAddTagV(XmlBuilder_t *this, char *tag, char *fmt, va_list ap);
extern char *XmlBuilderAddStringV(XmlBuilder_t *this, char *fmt, va_list ap);
extern char *XmlBuilderAddString(XmlBuilder_t *this, char *fmt, ...);
extern char *XmlBuilderOpenTitledTable(XmlBuilder_t *this, char *border, char *title);

#endif
