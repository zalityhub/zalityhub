/*****************************************************************************

Filename:   include/json.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:53 $
 * $Header: /home/hbray/cvsroot/fec/include/Attic/json.h,v 1.1.2.4 2011/10/27 18:33:53 hbray Exp $
 *
 $Log: json.h,v $
 Revision 1.1.2.4  2011/10/27 18:33:53  hbray
 Revision 5.5

 Revision 1.1.2.3  2011/09/24 17:49:36  hbray
 Revision 5.5

 Revision 1.1.2.1  2011/09/01 14:49:43  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:34  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: json.h,v 1.1.2.4 2011/10/27 18:33:53 hbray Exp $ "


#ifndef _JSON_H
#define _JSON_H


typedef struct Json_t
{
	char		*name;
	cJSON		*root;
} Json_t;


#define JsonNew(name, ...) _JsonNew(cJSON_CreateObject(),  name, ##__VA_ARGS__)
#define _JsonNew(root, name, ...) ObjectNew(Json, root, name, ##__VA_ARGS__)
#define JsonVerify(var) ObjectVerify(Json, var)
#define JsonDelete(var) ObjectDelete(Json, var)


// External Functions
extern Json_t* JsonConstructor(Json_t *this, char *file, int lno, cJSON*, char *name, ...);
extern void JsonDestructor(Json_t *this, char *file, int lno);

// For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when JsonParse() returns 0. 0 when JsonParse() succeeds.
extern const char *JsonGetErrorPtr();

// Render a Json_t entity to text for transfer/storage. Free the char* when finished.
extern char* JsonToDisplay(Json_t *this);
extern BtNode_t* JsonNodeList;
extern Json_t* JsonSerialize(Json_t *this);
// Render a Json_t entity to text for transfer/storage without any formatting. Free the char* when finished.
extern char *JsonToString(Json_t *this);

// Supply a block of JSON, and this returns a Json_t object you can interrogate. Call JsonDelete when finished.
extern Json_t *JsonParse(const char *value);
// Delete a Json_t entity and all subentities.

// Returns the number of items in an array (or object).
extern int JsonGetArraySize(Json_t * array);
// Retrieve item number "item" from array "array". Returns NULL if unsuccessful.
extern Json_t *JsonGetArrayItem(Json_t * array, int item);
// Get item "string" from object. Case insensitive.
extern Json_t *JsonGetObjectItem(Json_t * object, const char *string);

// These calls create a Json_t item of the appropriate type.
extern Json_t *JsonCreateNull();
extern Json_t *JsonCreateTrue();
extern Json_t *JsonCreateFalse();
extern Json_t *JsonCreateBool(int b);
extern Json_t *JsonCreateNumber(double num);
extern Json_t *JsonCreateStringV(char *fmt, va_list ap);
extern Json_t *JsonCreateString(char *fmt, ...);
extern Json_t *JsonCreateArray();

// These utilities create an Array of count items.
extern Json_t *JsonCreateIntArray(int *numbers, int count);
extern Json_t *JsonCreateFloatArray(float *numbers, int count);
extern Json_t *JsonCreateDoubleArray(double *numbers, int count);
extern Json_t *JsonCreateStringArray(const char **strings, int count);

// Append item to the specified array/object.
extern void JsonAddItemToArray(Json_t * array, Json_t * item);
extern void JsonAddItem(Json_t * object, const char *string, Json_t * item);
// Append reference to item to the specified array/object. Use this when you want to add an existing Json_t to a new Json_t, but don't want to corrupt your existing Json_t.
extern void JsonAddItemReferenceToArray(Json_t * array, Json_t * item);
extern void JsonAddItemReferenceToObject(Json_t * object, const char *string, Json_t * item);

// Remove/Detatch items from Arrays/Objects.
extern Json_t *JsonDetachItemFromArray(Json_t * array, int which);
extern void JsonDeleteItemFromArray(Json_t * array, int which);
extern Json_t *JsonDetachItemFromObject(Json_t * object, const char *string);
extern void JsonDeleteItemFromObject(Json_t * object, const char *string);

// Update array items.
extern void JsonReplaceItemInArray(Json_t * array, int which, Json_t * newitem);
extern void JsonReplaceItemInObject(Json_t * object, const char *string, Json_t * newitem);

#define JsonAddNull(object,name)	JsonAddItem(object, name, JsonCreateNull())
#define JsonAddBoolean(object,name,b) (b)?JsonAddTrue(object,name):JsonAddFalse(object,name)
#define JsonAddTrue(object,name)	JsonAddItem(object, name, JsonCreateTrue())
#define JsonAddFalse(object,name)		JsonAddItem(object, name, JsonCreateFalse())
#define JsonAddNumber(object,name,n)	JsonAddItem(object, name, JsonCreateNumber(n))

static inline void JsonAddStringV(Json_t *object, char *name, char *fmt, va_list ap)
{
	JsonAddItem(object, name, JsonCreateStringV(fmt, ap));
}

static inline void JsonAddString(Json_t *object, char *name, char *fmt,...)
{
	va_list ap;
	va_start(ap, fmt);
	JsonAddStringV(object, name, fmt, ap);
}

extern void JsonDataOut(Json_t *this, char *name, char *data, int datalen, OutputType_t outputType);

static inline void JsonAddCookedString(Json_t *object, char *name, char *raw, int rawlen)
{
	JsonAddString(object, name, "%s", EncodeUrlCharacters(raw, rawlen));
}

static inline void JsonAddPointer(Json_t *object, char *name, void *ptr)
{
	JsonAddString(object, name, "%p", ptr);
}


extern Json_t* JsonPushObject(Json_t *root, char *name, ...);

// allocators
extern void JsonInit();
extern void* JsonMalloc(size_t sz);
extern void JsonFree(void *ptr);

#endif
