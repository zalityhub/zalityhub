/*****************************************************************************

Filename:   lib/nx/json.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:57 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/Attic/json.c,v 1.1.2.4 2011/10/27 18:33:57 hbray Exp $
 *
 $Log: json.c,v $
 Revision 1.1.2.4  2011/10/27 18:33:57  hbray
 Revision 5.5

 Revision 1.1.2.3  2011/09/24 17:49:45  hbray
 Revision 5.5

 Revision 1.1.2.1  2011/09/01 14:49:45  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:17  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: json.c,v 1.1.2.4 2011/10/27 18:33:57 hbray Exp $ "

#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/json.h"

#include <dlfcn.h>

static void* (*Malloc)(size_t sz) = NULL;
static void  (*Free)(void *ptr) = NULL;
static void JsonDeleteNode(cJSON *node);


// Hook the jSON engine
void JsonInit()
{
	void	*handle;


	// get address of real malloc and free

	if (!(handle = dlopen(0, RTLD_NOW)))
		NxCrash("Access to malloc/free is required");
	if (!(Malloc = dlsym(handle, "malloc")))
		NxCrash("Access to malloc is required");
	if (!(Free = dlsym(handle, "free")))
		NxCrash("Access to free is required");
	dlclose(handle);

	static cJSON_Hooks hooks;
	hooks.malloc_fn = NULL;
	hooks.free_fn = NULL;
	hooks.cJSON_newNode = NULL;
	hooks.cJSON_deleteNode = JsonDeleteNode;
	cJSON_InitHooks(&hooks);
}



BtNode_t *JsonNodeList = NULL;


Json_t*
JsonConstructor(Json_t *this, char *file, int lno, cJSON *root, char *name, ...)
{

	if ( (this->root = root) == NULL )
		SysLog(LogFatal, "Unable to cJSON_CreateObject");

	this->root->context = (void*)this;		// ptr to me
	{
		va_list ap;
		va_start(ap, name);
		StringNewStatic(tmp, 32);
		StringSprintfV(tmp, name, ap);
		this->name = strdup(tmp->str);
	}
	return this;
}


void
JsonDestructor(Json_t *this, char *file, int lno)
{
	if ( this->root != NULL )
	{
		this->root->context = NULL;		// indicate self is deleted
		if ( this->name )
			free(this->name);
		cJSON_Delete(this->root);
	}
}


Json_t*
JsonSerialize(Json_t *this)
{
	return this;
}


char*
JsonToDisplay(Json_t *this)
{
	JsonVerify(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	char *tmp = cJSON_Print(this->root);
	StringCpy(out, tmp);
	(*Free)(tmp);
	return out->str;
}


char*
JsonToString(Json_t *this)
{
	JsonVerify(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	char *tmp = cJSON_PrintUnformatted(this->root);
	StringCpy(out, tmp);
	(*Free)(tmp);
	return out->str;
}


// For analysing failed parses. This returns a pointer to the parse error
const char*
JsonGetErrorPtr()
{
	return cJSON_GetErrorPtr();
}



// Supply a block of JSON, and this returns a Json_t object you can interrogate. Call JsonDelete when finished.
Json_t*
JsonParse(const char *value)
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_Parse(value)) != NULL )
		json = _JsonNew(root, __FUNC__);
	return json;
}


// Returns the number of items in an array (or object).
int
JsonGetArraySize(Json_t *this)
{
	JsonVerify(this);
	return cJSON_GetArraySize(this->root);
}


// Retrieve item number "item" from array "array". Returns NULL if unsuccessful.
Json_t*
JsonGetArrayItem(Json_t *this, int item)
{
	JsonVerify(this);
	cJSON *root = cJSON_GetArrayItem(this->root, item);
	if ( root != NULL )
		root = root->context;
	return (Json_t*)root;
}


// Get item "string" from object. Case insensitive.
Json_t*
JsonGetObjectItem(Json_t *this, const char *string)
{
	JsonVerify(this);
	cJSON *root = cJSON_GetObjectItem(this->root, string);
	if ( root != NULL )
		root = root->context;
	return (Json_t*)root;
}


Json_t*
JsonPushObject(Json_t *root, char *name, ...)
{
	Json_t *sub = JsonNew("");
	va_list ap;
	va_start(ap, name);
	StringNewStatic(tmp, 32);
	StringSprintfV(tmp, name, ap);
	JsonAddItem(root, tmp->str, sub);
	return sub;
}


// These calls create a Json_t item of the appropriate type.
Json_t*
JsonCreateNull()
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_CreateNull()) == NULL )
		SysLog(LogFatal, "cJSON_CreateNull failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


Json_t*
JsonCreateTrue()
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_CreateTrue()) == NULL )
		SysLog(LogFatal, "cJSON_CreateTrue failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


Json_t*
JsonCreateFalse()
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_CreateFalse()) == NULL )
		SysLog(LogFatal, "cJSON_CreateFalse failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


Json_t*
JsonCreateBool(int b)
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_CreateBool(b)) == NULL )
		SysLog(LogFatal, "cJSON_CreateBool failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


Json_t*
JsonCreateNumber(double num)
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_CreateNumber(num)) == NULL )
		SysLog(LogFatal, "cJSON_CreateNumber failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


Json_t*
JsonCreateStringV(char *fmt, va_list ap)
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	StringNewStatic(tmp, 32);
	StringSprintfV(tmp, fmt, ap);
	if ( (root = cJSON_CreateString(tmp->str)) == NULL )
		SysLog(LogFatal, "cJSON_CreateString failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


Json_t*
JsonCreateString(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return JsonCreateStringV(fmt, ap);
}


Json_t*
JsonCreateArray()
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_CreateArray()) == NULL )
		SysLog(LogFatal, "cJSON_CreateArray failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


// These utilities create an Array of count items.
Json_t*
JsonCreateIntArray(int *numbers, int count)
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_CreateIntArray(numbers, count)) == NULL )
		SysLog(LogFatal, "cJSON_CreateIntArray failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


Json_t*
JsonCreateFloatArray(float *numbers, int count)
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_CreateFloatArray(numbers, count)) == NULL )
		SysLog(LogFatal, "cJSON_CreateFloatArray failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


Json_t*
JsonCreateDoubleArray(double *numbers, int count)
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_CreateDoubleArray(numbers, count)) == NULL )
		SysLog(LogFatal, "cJSON_CreateDoubleArray failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


Json_t*
JsonCreateStringArray(const char **strings, int count)
{
	Json_t *json = NULL;
	cJSON *root = NULL;
	if ( (root = cJSON_CreateStringArray(strings, count)) == NULL )
		SysLog(LogFatal, "cJSON_CreateStringArray failed");
	json = _JsonNew(root, __FUNC__);
	return json;
}


static void
JsonPutDumpLine(int offset, char *dump, char *text, void *arg)
{
	char oft[BUFSIZ];
	sprintf(oft, "%x", offset);
	Json_t *root = (Json_t*)arg;
	JsonAddString(root, oft, "%-48.48s %-16.16s", dump, text);
}


void
JsonDataOut(Json_t *this, char *name, char *data, int datalen, OutputType_t outputType)
{
	Json_t *sub = JsonPushObject(this, name);
	JsonAddNumber(sub, "Len", datalen);

	switch(outputType)
	{
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
			if ( datalen > 8192 )
				datalen = 8192;
			DumpmemFull(JsonPutDumpLine, sub, data, datalen, 0);
			break;
	}
}


// Append item to the specified array/object.
void
JsonAddItemToArray(Json_t *this, Json_t *item)
{
	JsonVerify(this);
	JsonVerify(item);
	cJSON_AddItemToArray(this->root, item->root);
}


void
JsonAddItem(Json_t *this, const char *string, Json_t *item)
{
	JsonVerify(this);
	JsonVerify(item);
	cJSON_AddItemToObject(this->root, string, item->root);
}


// Append reference to item to the specified array/object. Use this when you want to add an existing Json_t to a new Json_t, but don't want to corrupt your existing Json_t.
void
JsonAddItemReferenceToArray(Json_t *this, Json_t *item)
{
	JsonVerify(this);
	JsonVerify(item);
	cJSON_AddItemReferenceToArray(this->root, item->root);
}


void
JsonAddItemReferenceToObject(Json_t *this, const char *string, Json_t *item)
{
	JsonVerify(this);
	JsonVerify(item);
	cJSON_AddItemReferenceToObject(this->root, string, item->root);
}


// Remove/Detatch items from Arrays/Objects.
Json_t*
JsonDetachItemFromArray(Json_t *this, int which)
{
	JsonVerify(this);
	cJSON *root = cJSON_DetachItemFromArray(this->root, which);
	if ( root != NULL )
		root = root->context;
	return (Json_t*)root;
}


void
JsonDeleteItemFromArray(Json_t *this, int which)
{
	JsonVerify(this);
	cJSON_DeleteItemFromArray(this->root, which);
}


Json_t*
JsonDetachItemFromObject(Json_t *this, const char *string)
{
	JsonVerify(this);
	cJSON *root = cJSON_DetachItemFromObject(this->root, string);
	if ( root != NULL )
		root = root->context;
	return (Json_t*)root;
}


void
JsonDeleteItemFromObject(Json_t *this, const char *string)
{
	JsonVerify(this);
	cJSON_DeleteItemFromObject(this->root, string);
}



// Update array items.
void
JsonReplaceItemInArray(Json_t *this, int which, Json_t *newitem)
{
	JsonVerify(this);
	JsonVerify(newitem);
	cJSON_ReplaceItemInArray(this->root, which, newitem->root);
}


void
JsonReplaceItemInObject(Json_t *this, const char *string, Json_t *newitem)
{
	JsonVerify(this);
	JsonVerify(newitem);
	cJSON_ReplaceItemInObject(this->root, string, newitem->root);
}


// Allocators

static void
JsonDeleteNode(cJSON *node)
{
	if ( node->context != NULL )
	{
		Json_t *obj = (Json_t*)node->context;
		obj->root = NULL;		// indicate root is deleted
		JsonDelete(obj);
	}
	node->context = NULL;
	(*Free)(node);
}
