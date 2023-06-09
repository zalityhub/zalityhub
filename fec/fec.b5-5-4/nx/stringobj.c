/*****************************************************************************

Filename:   lib/nx/stringobj.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:59 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/stringobj.c,v 1.3.4.3 2011/10/27 18:33:59 hbray Exp $
 *
 $Log: stringobj.c,v $
 Revision 1.3.4.3  2011/10/27 18:33:59  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/24 17:49:47  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/09/02 14:17:03  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:19  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:48  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: stringobj.c,v 1.3.4.3 2011/10/27 18:33:59 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/stringobj.h"


static const int TrailingPad = 128;



BtNode_t *StringNodeList = NULL;


String_t*
StringConstructor(String_t *this, char *file, int lno, int size)
{
	this->size = size;
	if ( (this->str = LeakCalloc(1, this->size + TrailingPad, file, lno)) == NULL )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "Unable to calloc %d", this->size);
	return this;
}


void
StringDestructor(String_t *this, char *file, int lno)
{
	StringVerify(this);
	LeakFree(this->str, file, lno);
}


Json_t*
StringSerialize(String_t *this)
{
	StringVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, "Details", ObjectStringStamp(this));
	JsonAddNumber(root, "Len", this->len);
	int slen = this->len;
	if ( slen > 128 )
		slen = 128;
	char sav = this->str[slen];
	this->str[slen] = '\0';		// restrict to slen bytes
	JsonAddString(root, "Value", this->str);
	this->str[slen] = sav;		// restore to full size
	return root;
}


char*
StringToString(String_t *this)
{
	Json_t *root = StringSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


void
StringClear(String_t *this)
{
	StringVerify(this);
	this->len = 0;
	memset(this->str, 0, this->size + TrailingPad);
}


void
StringResize(String_t *this, int sizeIncrement)
{

	StringVerify(this);

	if ( (this->size += sizeIncrement) < 0 )
		this->size = 1;			// keep it at least one
	if ( (this->str = (char *)realloc(this->str, this->size + TrailingPad)) == NULL )
		NxCrash("Unable to realloc %d", this->size);
	memset(&this->str[this->size], 0, 10);	// Add ten terminators...
}


char*
StringCat(String_t *this, char *str)
{

	StringVerify(this);

	int len = strlen(str);
	if ((this->len + len) > this->size)	// need more space
		StringResize(this, len);

	memcpy(&this->str[this->len], str, len);
	this->len += len;
	memset(&this->str[this->len], 0, 10);	// Add ten terminators...
	return this->str;
}


char*
StringCpy(String_t *this, char *str)
{
	StringClear(this);
	StringCat(this, str);
	return this->str;
}


char*
StringDup(String_t *this, String_t *str)
{
	return StringCpy(this, str->str);
}


char*
StringCatChar(String_t *this, char ch)
{

	char c2[2];
	c2[0] = ch;
	c2[1] = '\0';
	return StringCat(this, c2);
}


int
StringSprintfV(String_t *this, char *fmt, va_list ap)
{
	StringVerify(this);

	va_list aq;
	va_copy(aq, ap);		// save a copy
	this->len = vsnprintf(this->str, this->size+1, fmt, ap);
	if ( this->len > this->size )		// need more room
	{
		StringResize(this, this->len - this->size);
		this->len = vsnprintf(this->str, this->size+1, fmt, aq);
	}
	va_end(aq);

	memset(&this->str[this->len], 0, 10);	// Add ten terminators...
	return this->len;
}


int
StringSprintf(String_t *this, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return StringSprintfV(this, fmt, ap);
}


char*
StringStaticSprintfV(char *fmt, va_list ap)
{
	StringArrayStatic(sa, 128, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintfV(out, fmt, ap);
	return out->str;
}


char*
StringStaticSprintf(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return StringStaticSprintfV(fmt, ap);
}


int
StringSprintfCatV(String_t *this, char *fmt, va_list ap)
{

	StringVerify(this);

	va_list aq;
	va_copy(aq, ap);		// save a copy
	char *out = &this->str[this->len];
	int osize = (this->size+1) - this->len;
	int flen = vsnprintf(out, osize, fmt, ap);
	if ( flen >= osize )
	{
		StringResize(this, (this->len + flen) - this->size);
		out = &this->str[this->len];
		osize = (this->size+1) - this->len;
		flen = vsnprintf(out, osize, fmt, aq);
	}
	this->len += flen;
	va_end(aq);

	memset(&this->str[this->len], 0, 10);	// Add ten terminators...
	return flen;
}


int
StringSprintfCat(String_t *this, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return StringSprintfCatV(this, fmt, ap);
}


int
StringSprintfCooked(String_t *this, char *fmt, char *raw, int rawlen)
{
	char *cooked = EncodeUrlCharacters(raw, rawlen);
	int olen = StringSprintf(this, fmt, cooked);
	return olen;
}


int
StringSprintfCookedCat(String_t *this, char *fmt, char *raw, int rawlen)
{
	char *cooked = EncodeUrlCharacters(raw, rawlen);
	int olen = StringSprintfCat(this, fmt, cooked);
	return olen;
}


static void
putStringLine(char *line, void *arg)
{
	String_t *out = (String_t*)arg;
	StringCat(out, line);
}


String_t*
StringDump(String_t *this, void *mem, int len, int *offset)
{
	StringArrayStatic(sa, 16, 32);

	if ( this == NULL )
		this = StringArrayNext(sa);

	StringClear(this);
	Dumpmem(putStringLine, this, mem, len, offset);
	for (char *ptr = &this->str[strlen(this->str)-1]; ptr >= this->str && *ptr == '\n';)
		*ptr-- = '\0';
	this->len = strlen(this->str);
	return this;
}


int
StringSprintfDump(String_t *this, char *fmt, char *raw, int rawlen)
{
	return StringSprintf(this, fmt, StringDump(NULL, raw, rawlen, 0)->str);
}


int
StringSprintfDumpCat(String_t *this, char *fmt, char *raw, int rawlen)
{
	return StringSprintfCat(this, fmt, StringDump(NULL, raw, rawlen, 0)->str);
}


int
StringDataOut(String_t *this, char *fmt, char *raw, int rawlen, OutputType_t outputType)
{
	int len = 0;

	switch(outputType)
	{
		default:
			SysLog(LogError, "Don't know type %d", outputType);
			break;
		case NoOutput:
			break;
		case TextOutput:
			len = StringSprintfCooked(this, fmt, raw, rawlen);
			break;
		case DumpOutput:
			len = StringSprintfDump(this, fmt, raw, rawlen);
			break;
	}
	
	return len;
}


char*
StringReplace(String_t *this, char *from, char *to, boolean once)
{
	StringVerify(this);

	for (char *iptr; (iptr = strstr(this->str, from)) != NULL; )
	{
		char *tmp = calloc(1, strlen(this->str)+strlen(to)+128);
		if ( tmp == NULL )
			NxCrash("Unable to calloc %d", this->size);
		*iptr = '\0';			// cut it
		iptr += strlen(from);			// point to suffix
		char *optr = tmp;
		optr += sprintf(optr, "%s", this->str);		// non matching prefix
		optr += sprintf(optr, "%s", to);		// the replacement
		optr += sprintf(optr, "%s", iptr);	// the suffix
		StringCpy(this, tmp);
		if ( once )
			break;						// only one time (not global replace)...
	}

	return this->str;
}


int
_StringSprintfPairs(String_t *this, void **nv, int npairs)
{
	int len = 0;

	StringVerify(this);

	// format the name, value pairs
	for (int n = 0; n < npairs; n += 2)
	{
		if (nv == NULL || *nv == NULL)
			break;

		if ((n + 1) >= npairs)
		{
			len += StringSprintfCat(this, "%s=\"\" ", nv[n]);
		}
		else
		{
			char *name = nv[n];
			void *val = nv[n + 1];

			if (strncmp(name, "%d.", 3) == 0)	// integer conversion
				len += StringSprintfCat(this, "%s=\"%d\" ", &name[3], *((int*)val));
			else if (strncmp(name, "%lld.", 5) == 0)	// long long conversion
				len += StringSprintfCat(this, "%s=\"%s\" ", &name[5], *((unsigned long long*)val));
			else if (strncmp(name, "%s.", 3) == 0)	// string conversion
				len += StringSprintfCat(this, "%s=\"%s\" ", &name[3], *((char*)val));
			else
				len += StringSprintfCat(this, "%s=\"%s\" ", name, (char*)val);
		}
	}

	return len;
}


// Separates string into a name/value pair; also removes any leading/trailing spaces from name and value
// Example: name = value
// Becomes:
//  "name"
//  "value"
//
void
StringSplitPair(String_t *this, char *name, char *value, boolean trim)
{

	StringVerify(this);

	strcpy(name, this->str);

	char *equal = strchr(name, '=');

	if (equal == NULL)
	{
		strcpy(value, "");		// no value
	}
	else
	{
		*equal++ = '\0';		// chop...
		strcpy(value, equal);	// set value
	}

	if ( trim ) // now remove trailing/leading spaces
	{
		for (char *ptr = &name[strlen(name) - 1]; ptr >= name && isspace(*ptr);)
			*ptr-- = '\0';			// trailing
		for (char *ptr = name; isspace(*ptr);)
			strcpy(ptr, &ptr[1]);
		for (char *ptr = &value[strlen(value) - 1]; ptr >= value && isspace(*ptr);)
			*ptr-- = '\0';			// trailing
		for (char *ptr = value; isspace(*ptr);)
			strcpy(ptr, &ptr[1]);
	}
}


StringArray_t*
StringSplit(String_t *this, char *splitchrs)
{

	StringVerify(this);

	StringArray_t *values = StringArrayNew(0, 32);

	if ( strlen(this->str) <= 0 )
		return values;		// none

	char *str = strdup(this->str);

	int n = 0;
	for (char *ptr = str; *ptr != '\0'; ++n )
	{
		if ( n >= values->nmemb )
			StringArrayGrow(values, 1);

		char *sptr = strpbrk(ptr, splitchrs);
		if ( sptr == NULL )
		{
			StringCpy(values->array[n], ptr);		/// copy string
			break;				// done
		}
		*sptr++ = '\0';
		StringCpy(values->array[n], ptr);		/// copy string
		ptr = sptr;		// next
	}

	free(str);
	return values;
}


BtNode_t *StringArrayNodeList = NULL;


StringArray_t*
StringArrayConstructor(StringArray_t *this, char *file, int lno, int nmemb, int esize)
{

	if ( nmemb < 0 || esize <= 0 )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "Can't create nmemb=%d, esize=%d", nmemb, esize);

	this->nmemb = nmemb;
	this->esize = esize;
	if ( (this->array = (String_t**)LeakCalloc(this->nmemb+1, sizeof(String_t*), file, lno)) == NULL )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "Unable to calloc %d", (this->nmemb+1)*sizeof(String_t*));

	for(int i = 0; i < this->nmemb; ++i)
		this->array[i] = StringNew(this->esize);
	return this;
}


void
StringArrayDestructor(StringArray_t *this, char *file, int lno)
{
	for(int i = 0; i < this->nmemb; ++i)
		StringDelete(this->array[i]);
	LeakFree(this->array, file, lno);
}


Json_t*
StringArraySerialize(StringArray_t *this)
{
	StringArrayVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddNumber(root, "Nmemb", this->nmemb);
	JsonAddNumber(root, "Esize", this->esize);
	for(int i = 0; i < this->nmemb; ++i)
	{
		char tmp[16];
		sprintf(tmp, "%d", i);
		JsonAddItem(root, tmp, StringSerialize(this->array[i]));
	}
	return root;
}


char*
StringArrayToString(StringArray_t *this)
{
	Json_t *root = StringArraySerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


void
StringArrayClear(StringArray_t *this)
{

	StringArrayVerify(this);

	for(int i = 0; i < this->nmemb; ++i)
		StringClear(this->array[i]);
	this->next = 0;
}


void
StringArrayGrow(StringArray_t *this, int nmemb)
{

	StringArrayVerify(this);

	if ( nmemb < 0 )
		NxCrash("Can't grow nmemb=%d", nmemb);

	int n = this->nmemb;		// save current nmemb

// increase size of pointer table
	this->nmemb += nmemb;
	if ( (this->array = (String_t**)realloc(this->array, (this->nmemb+1)*sizeof(String_t*))) == NULL )
		NxCrash("realloc failed; no memory");

// create new members
	for(int i = n; i < this->nmemb; ++i)
		this->array[i] = StringNew(this->esize);
}


String_t*
StringArrayNext(StringArray_t *this)
{
	StringArrayVerify(this);
	String_t *string = this->array[this->next];
	if ( ++this->next >= this->nmemb )
		this->next = 0;				// wrap
	return string;
}
