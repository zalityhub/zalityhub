/*****************************************************************************

Filename:   include/stringobj.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:54 $
 * $Header: /home/hbray/cvsroot/fec/include/stringobj.h,v 1.3.4.3 2011/10/27 18:33:54 hbray Exp $
 *
 $Log: stringobj.h,v $
 Revision 1.3.4.3  2011/10/27 18:33:54  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/24 17:49:38  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/02 14:17:02  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:13  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:37  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: stringobj.h,v 1.3.4.3 2011/10/27 18:33:54 hbray Exp $ "


#ifndef _STRINGOBJ_H
#define _STRINGOBJ_H


typedef struct String_t
{
	int		size;			// allocation size
	int		len;			// current length of str
	char	*str;
} String_t;


typedef struct StringArray_t
{
	int			next;
	int			nmemb;
	int			esize;		// element size
	String_t	**array;
} StringArray_t;



#define StringNew(size) ObjectNew(String, size)
#define StringVerify(var) ObjectVerify(String, var)
#define StringDelete(var) ObjectDelete(String, var)

extern String_t* StringConstructor(String_t *this, char *file, int lno, int size);
extern void StringDestructor(String_t *this, char *file, int lno);
extern BtNode_t* StringNodeList;
extern struct Json_t* StringSerialize(String_t *this);
extern char* StringToString(String_t *this);

extern void StringClear(String_t *this);
extern void StringResize(String_t *this, int sizeIncrement);
extern char* StringCpy(String_t *this, char *str);

static inline String_t* StringNewCpy(char *str) {String_t *tmp = StringNew(strlen(str)+10); StringCpy(tmp, str); return tmp;};
#define StringNewStatic(name, size) \
			static String_t *name = NULL; \
			if ( name == NULL ) \
				name = StringNew(size); \

extern char* StringDup(String_t *this, String_t *str);
extern char *StringCat(String_t *this, char *str);
extern char *StringCatChar(String_t *this, char ch);
extern char* StringReplace(String_t *this, char *from, char *to, boolean once);
extern int StringSprintf(String_t *this, char *fmt, ...);
extern char* StringStaticSprintfV(char *fmt, va_list ap);
extern char* StringStaticSprintf(char *fmt, ...);
extern int StringSprintfV(String_t *this, char *fmt, va_list ap);
extern int StringSprintfCat(String_t *this, char *fmt, ...);
extern int StringSprintfCatV(String_t *this, char *fmt, va_list ap);
extern int StringSprintfCooked(String_t *this, char *fmt, char *raw, int rawlen);
extern int StringSprintfCookedCat(String_t *this, char *fmt, char *raw, int rawlen);
extern String_t* StringDump(String_t *this, void *mem, int len, int *offset);
extern int StringSprintfDump(String_t *this, char *fmt, char *raw, int rawlen);
extern int StringSprintfDumpCat(String_t *this, char *fmt, char *raw, int rawlen);
extern int StringDataOut(String_t *this, char *fmt, char *data, int datalen, OutputType_t outputType);
extern void StringSplitPair(String_t *this, char *name, char *value, boolean trim);
extern StringArray_t *StringSplit(String_t *this, char *splitchr);

#define StringSprintfPairs(this, ...) _StringSprintfPairs(this, va_args_toarray(__VA_ARGS__))
extern int _StringSprintfPairs(String_t *this, void **nv, int npairs);


#define StringArrayNew(nmemb, size) ObjectNew(StringArray, nmemb, size)
#define StringArrayVerify(var) ObjectVerify(StringArray, var)
#define StringArrayDelete(var) ObjectDelete(StringArray, var)

extern StringArray_t* StringArrayConstructor(StringArray_t *this, char *file, int lno, int nmemb, int size);
extern void StringArrayDestructor(StringArray_t *this, char *file, int lno);
extern BtNode_t* StringArrayNodeList;
extern char* StringArrayToString(StringArray_t *this);
extern struct Json_t* StringArraySerialize(StringArray_t *this);

extern void StringArrayGrow(StringArray_t *this, int nmemb);
extern void StringArrayClear(StringArray_t *this);
extern String_t* StringArrayNext(StringArray_t *this);

#define StringArrayStatic(name, nmemb, size) static StringArray_t *name = NULL; if ( name == NULL ) name = StringArrayNew(nmemb, size)

#endif
