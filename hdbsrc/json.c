/*****************************************************************************

Filename:   nx/json.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2018/02/05 16:33:48 $
*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <stdarg.h>
#include <glob.h>

#include "util.h"
#include "json.h"


// init the jSON engine
void
JsonInit()
{
}


Json_t         *
JsonConstructor(Json_t * this, char *file, int lno, cJSON * node, char *name, ...)
{

 if ((this->node = node) == NULL)
  SysLog(LogFatal, "Unable to cJSON_CreateObject");

 this->node->context = (void *) this; // ptr to me
 {
  va_list         ap;

  va_start(ap, name);
  StringNewStatic(tmp, 32);
  StringSprintfV(tmp, name, ap);
  va_end(ap);
  this->name = strdup(tmp->str);
 }
 return this;
}


void
JsonDestructor(Json_t * this, char *file, int lno)
{
 if (this->name)
  free(this->name);

 this->name = NULL;

 if (this->node != NULL) {
  this->node->context = NULL; // indicate self is deleted
  cJSON_Delete(this->node);
  this->node = NULL;
 }
}


Json_t         *
JsonSerialize(Json_t * this)
{
 return this;
}


char           *
JsonToDisplay(Json_t * this)
{
 JsonVerify(this);
 StringArrayStatic(sa, 16, 32);
 String_t       *out = StringArrayNext(sa);
 char           *tmp = cJSON_Print(this->node);

 StringCpy(out, tmp);
 (*Free) (tmp);
 return out->str;
}


char           *
JsonToString(Json_t * this)
{
 JsonVerify(this);
 StringArrayStatic(sa, 16, 32);
 String_t       *out = StringArrayNext(sa);
 char           *tmp = cJSON_PrintUnformatted(this->node);

 StringCpy(out, tmp);
 (*Free) (tmp);
 return out->str;
}


// For analysing failed parses. This returns a pointer to the parse error
const char     *
JsonGetErrorPtr()
{
 return cJSON_GetErrorPtr();
}



// Supply a block of JSON, and this returns a Json_t object you can interrogate. Call JsonDelete when finished.
Json_t         *
JsonParse(const char *value)
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_Parse(value)) != NULL)
  json = _JsonNew(node, __FUNC__);
 return json;
}


// Returns the number of items in an array (or object).
int
JsonGetArraySize(Json_t * this)
{
 JsonVerify(this);
 return cJSON_GetArraySize(this->node);
}


// Retrieve item number "item" from array "array". Returns NULL if unsuccessful.
Json_t         *
JsonGetArrayItem(Json_t * this, int item)
{
 JsonVerify(this);
 cJSON          *node = cJSON_GetArrayItem(this->node, item);

 if (node != NULL)
  return _JsonNew(node, __FUNC__);
 return NULL;
}


// Get item "string" from object. Case insensitive.
Json_t         *
JsonGetObjectItem(Json_t * this, const char *string)
{
 JsonVerify(this);
 cJSON          *node = cJSON_GetObjectItem(this->node, string);

 return _JsonNew(node, __FUNC__);
 return NULL;
}


Json_t         *
JsonPushObject(Json_t * node, char *name, ...)
{
 Json_t         *sub = JsonNew("");
 va_list         ap;

 va_start(ap, name);
 StringNewStatic(tmp, 32);
 StringSprintfV(tmp, name, ap);
 va_end(ap);
 JsonAddItem(node, tmp->str, sub);
 return sub;
}


// These calls create a Json_t item of the appropriate type.
Json_t         *
JsonCreateNull()
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_CreateNull()) == NULL)
  SysLog(LogFatal, "cJSON_CreateNull failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


Json_t         *
JsonCreateTrue()
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_CreateTrue()) == NULL)
  SysLog(LogFatal, "cJSON_CreateTrue failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


Json_t         *
JsonCreateFalse()
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_CreateFalse()) == NULL)
  SysLog(LogFatal, "cJSON_CreateFalse failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


Json_t         *
JsonCreateBool(int b)
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_CreateBool(b)) == NULL)
  SysLog(LogFatal, "cJSON_CreateBool failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


Json_t         *
JsonCreateNumber(double num)
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_CreateNumber(num)) == NULL)
  SysLog(LogFatal, "cJSON_CreateNumber failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


Json_t         *
JsonCreateStringV(char *fmt, va_list ap)
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 StringNewStatic(tmp, 32);
 StringSprintfV(tmp, fmt, ap);
 if ((node = cJSON_CreateString(tmp->str)) == NULL)
  SysLog(LogFatal, "cJSON_CreateString failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


Json_t         *
JsonCreateString(char *fmt, ...)
{
 Json_t         *r;

 va_list         ap;

 va_start(ap, fmt);
 r = JsonCreateStringV(fmt, ap);
 va_end(ap);
 return r;
}


Json_t         *
JsonCreateArray()
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_CreateArray()) == NULL)
  SysLog(LogFatal, "cJSON_CreateArray failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


// These utilities create an Array of count items.
Json_t         *
JsonCreateIntArray(int *numbers, int count)
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_CreateIntArray(numbers, count)) == NULL)
  SysLog(LogFatal, "cJSON_CreateIntArray failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


Json_t         *
JsonCreateFloatArray(float *numbers, int count)
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_CreateFloatArray(numbers, count)) == NULL)
  SysLog(LogFatal, "cJSON_CreateFloatArray failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


Json_t         *
JsonCreateDoubleArray(double *numbers, int count)
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_CreateDoubleArray(numbers, count)) == NULL)
  SysLog(LogFatal, "cJSON_CreateDoubleArray failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


Json_t         *
JsonCreateStringArray(const char **strings, int count)
{
 Json_t         *json = NULL;
 cJSON          *node = NULL;

 if ((node = cJSON_CreateStringArray(strings, count)) == NULL)
  SysLog(LogFatal, "cJSON_CreateStringArray failed");
 json = _JsonNew(node, __FUNC__);
 return json;
}


static void
JsonPutDumpLine(int offset, char *dump, char *text, void *arg)
{
 char            oft[BUFSIZ];

 sprintf(oft, "%x", offset);
 Json_t         *node = (Json_t *) arg;

 JsonAddString(node, oft, "%-48.48s %-16.16s", dump, text);
}


void
JsonDataOut(Json_t * this, char *name, char *data, int datalen, OutputType_t outputType)
{
 Json_t         *sub = JsonPushObject(this, name);

 JsonAddNumber(sub, "Len", datalen);

 switch (outputType) {
 default:
  SysLog(LogError, "Don't know type %d", outputType);
  break;
 case NoOutput:
  break;
 case TextOutput:
  JsonAddString(sub, "Text", EncodeUrlCharacters(data, datalen));
  break;
 case DumpOutput:
  sub = JsonPushObject(sub, "Dump");
  if (datalen > 8192)
   datalen = 8192;
  DumpmemFull(JsonPutDumpLine, sub, data, datalen, 0);
  break;
 }
}


// Append item to the specified array/object.
void
JsonAddItemToArray(Json_t * this, Json_t * item)
{
 JsonVerify(this);
 JsonVerify(item);
 cJSON_AddItemToArray(this->node, item->node);
}


void
JsonAddItem(Json_t * this, const char *string, Json_t * item)
{
 JsonVerify(this);
 JsonVerify(item);
 cJSON_AddItemToObject(this->node, string, item->node);
}


// Append reference to item to the specified array/object. Use this when you want to add an existing Json_t to a new Json_t,
// but don't want to corrupt your existing Json_t.
void
JsonAddItemReferenceToArray(Json_t * this, Json_t * item)
{
 JsonVerify(this);
 JsonVerify(item);
 cJSON_AddItemReferenceToArray(this->node, item->node);
}


void
JsonAddItemReferenceToObject(Json_t * this, const char *string, Json_t * item)
{
 JsonVerify(this);
 JsonVerify(item);
 cJSON_AddItemReferenceToObject(this->node, string, item->node);
}


// Remove/Detatch items from Arrays/Objects.
Json_t         *
JsonDetachItemFromArray(Json_t * this, int which)
{
 JsonVerify(this);
 cJSON          *node = cJSON_DetachItemFromArray(this->node, which);

 return _JsonNew(node, __FUNC__);
 return NULL;
}


void
JsonDeleteItemFromArray(Json_t * this, int which)
{
 JsonVerify(this);
 cJSON_DeleteItemFromArray(this->node, which);
}


Json_t         *
JsonDetachItemFromObject(Json_t * this, const char *string)
{
 JsonVerify(this);
 cJSON          *node = cJSON_DetachItemFromObject(this->node, string);

 return _JsonNew(node, __FUNC__);
 return NULL;
}


void
JsonDeleteItemFromObject(Json_t * this, const char *string)
{
 JsonVerify(this);
 cJSON_DeleteItemFromObject(this->node, string);
}



// Update array items.
void
JsonReplaceItemInArray(Json_t * this, int which, Json_t * newitem)
{
 JsonVerify(this);
 JsonVerify(newitem);
 cJSON_ReplaceItemInArray(this->node, which, newitem->node);
}


void
JsonReplaceItemInObject(Json_t * this, const char *string, Json_t * newitem)
{
 JsonVerify(this);
 JsonVerify(newitem);
 cJSON_ReplaceItemInObject(this->node, string, newitem->node);
}


// Allocators

static void
JsonDeleteNode(cJSON * node)
{
 if (node->context != NULL) {
  Json_t         *obj = (Json_t *) node->context;

  obj->node = NULL;  // indicate node is deleted
  JsonDelete(obj);
 }
 node->context = NULL;
 (*Free) (node);
}
