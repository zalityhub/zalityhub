/*****************************************************************************

Filename:   lib/nx/objlib.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:58 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/objlib.c,v 1.3.4.4 2011/10/27 18:33:58 hbray Exp $
 *
 $Log: objlib.c,v $
 Revision 1.3.4.4  2011/10/27 18:33:58  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/24 17:49:46  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/01 14:49:45  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:47  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: objlib.c,v 1.3.4.4 2011/10/27 18:33:58 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/bt.h"
#include "include/signatures.h"

#include "include/datagram.h"


BtNode_t *RootObjectList = NULL;

static int _ObjectTreeRecordVerify(unsigned long key, BtRecord_t *rec, void *arg);
static int _ObjectTreeRecordToString(unsigned long key, BtRecord_t *rec, void *arg);




static int
_ObjectTreeRecordVerify(unsigned long key, BtRecord_t *rec, void *arg)
{
	if ( key == 0 )
		return 0;		// a dummy

	Object_t *obj = (Object_t*)key;
	if ( ! ObjectSignatureIsValid(obj) )
		NxCrash("%p has bad object signature %ld", obj, obj->signature);
	if ( key != rec->value )
		NxCrash("key %p not same as rec %p", key, rec->value);
	return 0;
}


void
ObjectTreeVerify(BtNode_t *root)
{
	BtNodeWalk(root, _ObjectTreeRecordVerify, NULL);
}


static inline BtNode_t* ObjectInsert(BtNode_t *root, Object_t *obj, char *objName, char *file, int lno)
{
	root = BtInsert(root, (unsigned long)obj, (unsigned long)obj);
	if ( root == RootObjectList )
		NxDatagramPrintf("127.0.0.1", NxGlobal->objLogPort, "%d: %s: %s.%d: %s: %p\n", getpid(), __FUNC__, file, lno, objName, obj);
	ObjectTreeVerify(root);

	*(obj->nodeList) = BtInsert(*(obj->nodeList), (unsigned long)obj, (unsigned long)obj);
	ObjectTreeVerify(*(obj->nodeList));
	return root;
}


static inline BtNode_t* ObjectRemove(BtNode_t *root, Object_t *obj, char *objName, char *file, int lno)
{
	if ( root == RootObjectList )
		NxDatagramPrintf("127.0.0.1", NxGlobal->objLogPort, "%d: %s: %s.%d: %s: %p\n", getpid(), __FUNC__, file, lno, objName, obj);
	root = BtDelete(root, (unsigned long)obj);
	ObjectTreeVerify(root);

	*(obj->nodeList) = BtDelete(*(obj->nodeList), (unsigned long)obj);
	ObjectTreeVerify(*(obj->nodeList));
	return root;
}



#define ObjectLinkNew(list, var) ObjectNew(ObjectLink, list, var)
#define ObjectLinkVerify(var) ObjectVerify(ObjectLink, var)
#define ObjectLinkDelete(var) ObjectDelete(ObjectLink, var)
static ObjectLink_t* ObjectLinkConstructor(ObjectLink_t *this, char *file, int lno, ObjectList_t *list, void *var);
static BtNode_t* ObjectLinkNodeList = NULL;
static Json_t* ObjectLinkSerialize(ObjectLink_t *this);
static char* ObjectLinkToString(ObjectLink_t *this);


static void*
_ObjectInit(Object_t *obj, int size, void*(*constructor)(void*, char*, int, ...), void(*destructor)(void*,char*,int), BtNode_t**nodeList, struct Json_t*(*serialize)(void*), char*(*toString)(void*,...), char *objName, char *file, int lno)
{
	if ( obj == NULL )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "_ObjectInit", "No Memory for %s", objName);

	obj->signature = ObjectSignature;
	obj->time = NxGetTime();
	obj->ownerPid = getpid();
	obj->size = size;
	obj->objName = objName;
	obj->file = file;
	obj->lno = lno;
	obj->constructor = constructor;
	obj->destructor = destructor;
	obj->toString = toString;
	obj->serialize = serialize;
	obj->nodeList = nodeList;

	void *var = ObjectToVar(obj);
	return var;
}


void*
__ObjectNew(int size, void*(*constructor)(void*, char*, int, ...), void(*destructor)(void*,char*,int), BtNode_t**nodeList, struct Json_t*(*serialize)(void*), char*(*toString)(void*,...), char *objName, char *file, int lno)
{
	Object_t *obj = (Object_t*)LeakCalloc(1, sizeof(Object_t)+size, file, lno);
	if ( obj == NULL )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "__ObjectNew", "No Memory for %s", objName);

	void *var = _ObjectInit(obj, size, constructor, destructor, nodeList, serialize, toString, objName, file, lno);

	if ( NxGlobal->objLogPort != 0 )
		RootObjectList = ObjectInsert(RootObjectList, obj, objName, file, lno);

	return var;
}


void*
__ObjectNewShared(char *fname, int size, boolean new, void*(*constructor)(void*, char*, int, ...), void(*destructor)(void*,char*,int), BtNode_t**nodeList, struct Json_t*(*serialize)(void*),char*(*toString)(void*,...), char *objName, char *file, int lno)
{

	Object_t *obj = NULL;

	{							// Create the memory and initialize it
		// Calculate required length of the memory needed
		unsigned long length = size;

		length += (getpagesize()-(length % getpagesize()));	// round up to page boundry

		int mode = O_RDWR;
		if ( new )
		{
			unlink(fname); // start with a fresh file
			mode |= O_CREAT;
		}

		// open the memory file
		int fd;
		if ((fd = open(fname, mode, 0666)) < 0)
			_NxCrash(NxGlobal, file, lno, __FUNC__, "Error %s while creating %s; this is not good...", ErrnoToString(errno), fname);

		if (chmod(fname, 0666) < 0)
			SysLog(LogWarn, "chmod() error=%s: {%s}", ErrnoToString(errno), fname);

		if ( new )
		{
			// write zeros to this file
			void *zeros = calloc(1, length);
			if (write(fd, zeros, length) != length)
				_NxCrash(NxGlobal, file, lno, __FUNC__, "Error %s while writing to %s; this is not good...", ErrnoToString(errno), fname);
			free(zeros);
		}

		obj = mmap(0, (size_t)length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		if (obj == (Object_t*)MAP_FAILED)
			_NxCrash(NxGlobal, file, lno, __FUNC__, "Error %s when mapping to %s; this is not good...", ErrnoToString(errno), fname);

		if ( new )
			memset(obj, 0, length);

		// Close the file backing the memory, its no longer needed
		close(fd);
	
		obj->shared = true;
		obj->sharedFname = strdup(fname);		// save the file name
		obj->shmlen = (size_t)length;
		SpinInit(&obj->lock);
		msync((void*)obj, obj->shmlen, MS_SYNC);
	}

	void *var;
	if ( new )
	{
		var = _ObjectInit(obj, size, constructor, destructor, nodeList, serialize, toString, objName, file, lno);
		if ( NxGlobal->objLogPort != 0 )
			RootObjectList = ObjectInsert(RootObjectList, obj, objName, file, lno);
	}
	else
	{
		var = ObjectToVar(obj);
	}

	return var;
}


void
_ObjectDelete(void **var, void(*destructor)(void*,char*,int), char *objName, char *file, int lno)
{

	Object_t *obj = _ObjectVerify(*var, destructor, objName, file, lno);

	if ( NxGlobal->objLogPort != 0 )
		RootObjectList = ObjectRemove(RootObjectList, obj, objName, file, lno);

	Object_t oo;
	memcpy(&oo, obj, sizeof(oo));		// save a copy of original hdr

	(*obj->destructor)(*var, file, lno);

	if ( oo.shared )
	{
		 free(oo.sharedFname);
		 munmap(obj, oo.shmlen);
	}
	else
	{
		memset(*var, 0, oo.size);
		memset(obj, 0, sizeof(Object_t));
		LeakFree(obj, file, lno);
	}

	*var = NULL;
}


char*
_ObjectToName(void *var, char *file, int lno)
{

	Object_t *obj = VarToObject(var);
	if ( ! ObjectSignatureIsValid(obj) )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "%p has bad object signature %ld", obj, obj->signature);

	return obj->objName;
}


void
ObjectLock(void *var)
{
	Object_t *obj = _ObjectVerify(var, NULL, NULL, __FILE__, __LINE__);
	SpinLock(&obj->lock);
}


void
ObjectUnlock(void *var)
{
	Object_t *obj = _ObjectVerify(var, NULL, NULL, __FILE__, __LINE__);
	SpinUnlock(&obj->lock);
}


static int
_ObjectTreeRecordToString(unsigned long key, BtRecord_t *rec, void *arg)
{
	if ( key == 0 )
		return 0;		// a dummy

	String_t *out = (String_t*)arg;

	Object_t *obj = (Object_t*)key;
	if ( ! ObjectSignatureIsValid(obj) )
		NxCrash("%p has bad object signature %ld", obj, obj->signature);
	char *str = ObjectToString(ObjectToVar(obj));
	StringSprintfCat(out, "\n" "%s created from %s.%d %s\n", obj->objName, obj->file, obj->lno, str);
	return 0;
}


String_t*
ObjectTreeToString(BtNode_t *node, String_t *out)
{
	if ( out == NULL )
		out = StringNew(32);
	BtNodeWalk(node, _ObjectTreeRecordToString, out);
	return out;
}


BtNode_t*
_ObjectGetNodeList(void *var, char *file, int lno)
{
	Object_t *obj = VarToObject(var);
	if ( ! ObjectSignatureIsValid(obj) )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "Bad object signature");
	return *(obj->nodeList);
}


Json_t*
_ObjectSerialize(void *var, char *file, int lno)
{
	Object_t *obj = VarToObject(var);
	if ( ! ObjectSignatureIsValid(obj) )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "Bad object signature");
	return (*obj->serialize)(var);
}


char*
_ObjectToString(void *var, char *file, int lno)
{
	Object_t *obj = VarToObject(var);
	if ( ! ObjectSignatureIsValid(obj) )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "Bad object signature");
	return (*obj->toString)(var);
}


char*
_ObjectToStringRaw(void *var, char *file, int lno)
{

	Object_t *obj = VarToObject(var);
	if ( ! ObjectSignatureIsValid(obj) )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "Bad object signature");

	return StringStaticSprintf("%s created from %s.%d @ %s", obj->objName, obj->file, obj->lno, NxTimeToString(obj->time, NULL));
}


void
_ObjectSync(void *var, void(*destructor)(void*,char*,int), char *objName, char *file, int lno)
{
	Object_t *obj = _ObjectVerify(var, destructor, objName, file, lno);
	if ( obj->shared )
		msync((void*)obj, obj->shmlen, MS_SYNC);
}


boolean
_ObjectTestVerify(void *var, void(*destructor)(void*,char*,int))
{
	Object_t *obj = VarToObject(var);
	return ! ( var == NULL || obj->destructor != destructor );
}


Object_t*
_ObjectVerify(void *var, void(*destructor)(void*,char*,int), char *objName, char *file, int lno)
{

	if ( var == NULL )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "Expected an object %s; was given NULL", objName?objName:"");
	
	Object_t *obj = VarToObject(var);
	if ( ! ObjectSignatureIsValid(obj) )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "Expected an object %s; was given a non object", objName?objName:"");

	if ( destructor != NULL && obj->destructor != destructor )
		_NxCrash(NxGlobal, file, lno, __FUNC__, "Expected an object %s; was given object %s:{%s}", objName?objName:"", ObjectStringStamp(var), _ObjectToString(var, file,lno));
	return VarToObject(var);
}




/********************************************************************
	Local/Static definitions
********************************************************************/
// Static Functions

static ObjectLink_t* ObjectListInsert(ObjectList_t *this, void *var, ObjectLink_t *at);
static ObjectLink_t* ObjectListAddOrderedChar(ObjectList_t *this, void *var, char *key, long offset);
static ObjectLink_t* ObjectListAddOrderedInt(ObjectList_t *this, void *var, int *key, long offset);
static ObjectLink_t* ObjectListAddOrderedLong(ObjectList_t *this, void *var, long *key, long offset);
static ObjectLink_t* ObjectListAddOrderedUid(ObjectList_t *this, void *var, NxUid_t *key, long offset);
static ObjectLink_t* ObjectListAddOrderedTime(ObjectList_t *this, void *var, NxTime_t *key, long offset);
static ObjectLink_t* ObjectListAddOrderedULong(ObjectList_t *this, void *var, unsigned long *key, long offset);
static ObjectLink_t* ObjectListAddOrderedMem(ObjectList_t *this, void *var, char *key, long offset, short int len);
static ObjectLink_t* ObjectListAddOrderedString(ObjectList_t *this, void *var, char **key, long offset);




BtNode_t *ObjectListNodeList = NULL;


ObjectList_t*
ObjectListConstructor(ObjectList_t *this, char *file, int lno, ObjectListType_t listType, char *name, ...)
{
	{
		va_list ap;
		va_start(ap, name);
		vsnprintf(this->name, sizeof(this->name), name, ap);
	}
	this->listType = listType;
	return this;
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

void
ObjectListDestructor(ObjectList_t *this, char *file, int lno)
{
	ObjectListVerify(this);
	_ObjectListClear(this, false, true, file, lno);
}


void
_ObjectListClear(ObjectList_t *this, boolean deleteLostVars, boolean logLostEntries, char *file, int lno)
{

	ObjectListVerify(this);

// dispose of all entries

	for (void *var; (var = ObjectListRemove(this, ObjectListFirstPosition)) != NULL; )
	{
		Object_t *obj = VarToObject(var);
		if ( ! ObjectSignatureIsValid(obj) )
			_NxCrash(NxGlobal, file, lno, __FUNC__, "Bad object signature");
		if ( logLostEntries )
			SysLog(LogError, "%s disposing of possible lost entry %s", this->name, _ObjectToStringRaw(var, obj->file, obj->lno));
		if ( deleteLostVars )
			_ObjectDelete(&var, obj->destructor, obj->objName, obj->file, obj->lno);
	}
}



/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

ObjectLink_t*
ObjectListAdd(ObjectList_t *this, void *var, ObjectListPosition_t position, ...)
{

	ObjectListVerify(this);

	Object_t *obj = VarToObject(var);
	if ( ! ObjectSignatureIsValid(obj) )
		_NxCrash(NxGlobal, VarToObject(this)->file, VarToObject(this)->lno, __FUNC__, "Bad object signature");

	if (position == ObjectListFirstPosition)
	{
		ObjectLink_t *link = ObjectLinkNew(this, var);

		link->next = this->first;	// link.next points to current first
		if (this->first == NULL)	// list empty
		{
			this->last = link;	// set new last
			link->prev = NULL;	// no backward link
		}
		else
		{
			link->prev = this->first->prev;	// link.prev is current first prev
			this->first->prev = link;	// current first.prev is new item
		}

		this->first = link;	// new first
		++(this->length);
		return link;
	}

	if (position == ObjectListLastPosition)
	{
		ObjectLink_t *link = ObjectLinkNew(this, var);

		if (this->last == NULL)	// list empty
		{
			this->first = link;	// list empty, set first
			link->prev = NULL;	// no backward link
		}
		else
		{
			this->last->next = link;	// link to last
			link->prev = this->last;	// back link
		}

		this->last = link;		// new last
		link->next = NULL;	// no next

		++(this->length);
		return link;
	}

	if (position == ObjectListCursorPosition)	// prior to cursor
	{
		va_list ap;
		va_start(ap, position);
		return ObjectListInsert(this, var, va_arg(ap, ObjectLink_t*));
	}

	SysLog(LogError, "ListPosition %d for %s is not valid", position, this->name);
	return NULL;
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

ObjectLink_t*
ObjectListAddOrdered(ObjectList_t *this, void *var, void *key, short len)
{
	ObjectListVerify(this);

	Object_t *obj = VarToObject(var);
	if ( ! ObjectSignatureIsValid(obj) )
		_NxCrash(NxGlobal, VarToObject(this)->file, VarToObject(this)->lno, __FUNC__, "Bad object signature");

	long offset = (long)((long)key - (long)var);
	ObjectLink_t *result;
	switch (this->listType)
	{
	default:
		SysLog(LogError, "listCriteria %d for %s is invalid", this->listType, this->name);
		return NULL;

	case ObjectListCharType:
		result = ObjectListAddOrderedChar(this, var, (char*)key, offset);
		break;

	case ObjectListIntType:
		result = ObjectListAddOrderedInt(this, var, (int*)key, offset);
		break;

	case ObjectListLongType:
		result = ObjectListAddOrderedLong(this, var, (long*)key, offset);
		break;

	case ObjectListULongType:
		result = ObjectListAddOrderedULong(this, var, (unsigned long*)key, offset);
		break;

	case ObjectListUidType:
		result = ObjectListAddOrderedUid(this, var, (NxUid_t*)key, offset);
		break;

	case ObjectListTimeType:
		result = ObjectListAddOrderedTime(this, var, (NxTime_t*)key, offset);
		break;

	case ObjectListStringType:
		result = ObjectListAddOrderedString(this, var, (char**)key, offset);
		break;

	case ObjectListMemType:
		result = ObjectListAddOrderedMem(this, var, (char*)key, offset, len);
		break;
	}

	return result;
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

static ObjectLink_t*
ObjectListAddOrderedChar(ObjectList_t *this, void *var, char *key, long offset)
{
	ObjectLink_t *current = this->first;
	while (current != NULL)
	{
		if (*((char *)((long)current->var + offset)) >= *key)
			break;
		current = current->next;
	}

	return ObjectListInsert(this, var, current);
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

static ObjectLink_t*
ObjectListAddOrderedInt(ObjectList_t *this, void *var, int *key, long offset)
{
	ObjectLink_t *current = this->first;
	while (current != NULL)
	{
		if (*((int *)((long)current->var + offset)) >= *key)
			break;
		current = current->next;
	}

	return ObjectListInsert(this, var, current);
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

static ObjectLink_t*
ObjectListAddOrderedLong(ObjectList_t *this, void *var, long *key, long offset)
{
	ObjectLink_t *current = this->first;
	while (current != NULL)
	{
		if (*((long *)((long)current->var + offset)) >= *key)
			break;
		current = current->next;
	}

	return ObjectListInsert(this, var, current);
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

static ObjectLink_t*
ObjectListAddOrderedULong(ObjectList_t *this, void *var, unsigned long *key, long offset)
{
	ObjectLink_t *current = this->first;
	while (current != NULL)
	{
		if (*((unsigned long *)((unsigned long)current->var + offset)) >= *key)
			break;
		current = current->next;
	}

	return ObjectListInsert(this, var, current);
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

static ObjectLink_t*
ObjectListAddOrderedUid(ObjectList_t *this, void *var, NxUid_t *key, long offset)
{
	ObjectLink_t *current = this->first;
	while (current != NULL)
	{
		if ( NxUidCompare(*((NxUid_t*)((long)current->var + offset)), *key) >= 0)
			break;
		current = current->next;
	}

	return ObjectListInsert(this, var, current);
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

static ObjectLink_t*
ObjectListAddOrderedTime(ObjectList_t *this, void *var, NxTime_t *key, long offset)
{
	ObjectLink_t *current = this->first;
	while (current != NULL)
	{
		if (*((NxTime_t*)((long)current->var + offset)) >= *key)
			break;
		current = current->next;
	}

	return ObjectListInsert(this, var, current);
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

static ObjectLink_t*
ObjectListAddOrderedString(ObjectList_t *this, void *var, char **key, long offset)
{
	ObjectLink_t *current = this->first;
	while (current != NULL)
	{
		char **ckey = (char**)((unsigned long)(current->var) + offset);
		if (strcmp(*ckey, *key) >= 0)
			break;
		current = current->next;
	}

	return ObjectListInsert(this, var, current);
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

static ObjectLink_t*
ObjectListAddOrderedMem(ObjectList_t *this, void *var, char *key, long offset, short len)
{
	ObjectLink_t *current = this->first;
	while (current != NULL)
	{
		char *cap = (char*)((long)current->var + offset);
		if (memcmp(cap, key, len) >= 0)
			break;
		current = current->next;
	}

	return ObjectListInsert(this, var, current);
}


/*+******************************************************************
	Name:

	Synopsis:
		link item before 'at'

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

static ObjectLink_t*
ObjectListInsert(ObjectList_t *this, void *var, ObjectLink_t *at)
{
	ObjectListVerify(this);

	Object_t *obj = VarToObject(var);
	if ( ! ObjectSignatureIsValid(obj) )
		_NxCrash(NxGlobal, VarToObject(this)->file, VarToObject(this)->lno, __FUNC__, "Bad object signature");

	if ( at != NULL )
	{
		ObjectLinkVerify(at);
		if ( at->listHead != this )
			_NxCrash(NxGlobal, VarToObject(this)->file, VarToObject(this)->lno, __FUNC__, "link %s is not associated with list %s", ObjectLinkToString(at), ObjectListToString(this));
	}

	ObjectLink_t *link = ObjectLinkNew(this, var);
	++(this->length);

	if (this->last == NULL)		// an empty list
	{
		this->first = link;	// list empty, set first
		link->prev = NULL;	// no backward link
		this->last = link;		// new last
		link->next = NULL;	// no next
		return link;
	}

	if (at == NULL)				// add to tail of list
	{
		at = this->last;
		at->next = link;
		link->next = NULL;
		link->prev = at;
		this->last = link;
		return link;
	}

	if (this->first == at)		// add to head of list
	{
		link->next = at;
		link->prev = NULL;
		at->prev = link;
		this->first = link;
		return link;
	}

// link item before 'at'

	ObjectLink_t *prev = at->prev;

	link->next = at;
	link->prev = prev;
	at->prev = link;
	prev->next = link;
	return link;
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

void*
ObjectListRemove(ObjectList_t *this, ObjectListPosition_t position, ...)
{

	ObjectListVerify(this);

	if (position == ObjectListFirstPosition)
	{
		ObjectLink_t *link;
		if ((link = this->first) == NULL)	// empty, return NULL
			return NULL;

		if ((this->first = link->next) == NULL)	// removed last
			this->last = NULL;	// reset tail pointer
		else
			this->first->prev = NULL;	// no previous to first

		link->listHead = NULL;
		link->next = link->prev = NULL;
		--(this->length);
		Object_t *obj = VarToObject(link->var);
		if ( ! ObjectSignatureIsValid(obj) )
			NxCrash("Bad object signature");
		ObjectLinkDelete(link);
		return ObjectToVar(obj);
	}

	if (position == ObjectListLastPosition)
	{
		ObjectLink_t *link;
		if ((link = this->last) == NULL)	// list is empty, return NULL  
			return NULL;

		if (this->first == this->last)
		{
			this->first = this->last = NULL;
		}
		else
		{
			this->last = link->prev;
			this->last->next = NULL;
		}

		link->listHead = NULL;
		link->next = link->prev = NULL;
		--(this->length);
		Object_t *obj = VarToObject(link->var);
		if ( ! ObjectSignatureIsValid(obj) )
			NxCrash("Bad object signature");
		ObjectLinkDelete(link);
		return ObjectToVar(obj);
	}

	if (position == ObjectListCursorPosition)
	{
		va_list ap;
		va_start(ap, position);
		return ObjectListYank(this, va_arg(ap, ObjectLink_t*));
	}

	SysLog(LogError, "listPosition %d for %s is not valid", position, this->name);
	return NULL;
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns NULL
-*******************************************************************/

void*
ObjectListYank(ObjectList_t *this, ObjectLink_t *link)
{

	ObjectListVerify(this);
	ObjectLinkVerify(link);

	if (link->next != NULL)	// not last on list
		link->next->prev = link->prev;	// close foreward hole
	else
		this->last = link->prev;	// last obj, change list

	if (link->prev != NULL)	// not first on list
		link->prev->next = link->next;	// close up aft hole
	else
		this->first = link->next;	// first obj, change list

	link->listHead = NULL;
	link->next = link->prev = NULL;
	--(this->length);
	Object_t *obj = VarToObject(link->var);
	if ( ! ObjectSignatureIsValid(obj) )
		NxCrash("Bad object signature");
	ObjectLinkDelete(link);
	return ObjectToVar(obj);
}


/*+******************************************************************
	Name:

	Synopsis:

	Description:
		This function

	Diagnostics:
		Returns 0 on success, else returns -1
-*******************************************************************/

ObjectLink_t*
ObjectListGetLink(ObjectList_t *this, void *var)
{

	ObjectListVerify(this);

// search all entries

	for (ObjectLink_t *link = ObjectListFirst(this); link != NULL; link = ObjectListNext(link))
	{
		if ( link->var == var )
			return link;		// in the list
	}

	return NULL;
}


long
ObjectListNbrEntries(ObjectList_t *this)
{
	ObjectListVerify(this);
	return this->length;
}


ObjectLink_t*
ObjectListFirst(ObjectList_t *this)
{
	ObjectListVerify(this);
	return this->first;
}


ObjectLink_t*
ObjectListLast(ObjectList_t *this)
{
	ObjectListVerify(this);
	return this->last;
}


ObjectLink_t*
ObjectListPrev(ObjectLink_t *link)
{
	ObjectLinkVerify(link);
	return link->prev;
}


ObjectLink_t*
ObjectListNext(ObjectLink_t *link)
{
	ObjectLinkVerify(link);
	return link->next;
}


static ObjectLink_t*
ObjectLinkConstructor(ObjectLink_t *this, char *file, int lno, ObjectList_t *list, void *var)
{
	this->listHead = list;
	this->var = var;
	return this;
}


void
ObjectLinkDestructor(ObjectLink_t *this, char *file, int lno)
{
}


Json_t*
ObjectListSerialize(ObjectList_t *this)
{
	ObjectListVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, ObjectTypeToString(this), ObjectStringStamp(this));
	return root;
}


char*
ObjectListToString(ObjectList_t *this)
{
	Json_t *root = ObjectListSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


Json_t*
ObjectLinkSerialize(ObjectLink_t *this)
{
	ObjectLinkVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddString(root, ObjectTypeToString(this), ObjectStringStamp(this));
	return root;
}


char*
ObjectLinkToString(ObjectLink_t *this)
{
	Json_t *root = ObjectLinkSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


char*
ObjectPositionToString(ObjectListPosition_t position)
{
	char *text;

	switch (position)
	{
		default:
			text = StringStaticSprintf("ObjectListPosition_%d", (int)position);
			break;

		EnumToString(ObjectListFirstPosition);
		EnumToString(ObjectListLastPosition);
		EnumToString(ObjectListCursorPosition);
	}

	return text;
}


char*
ObjectListTypeToString(ObjectListType_t listType)
{
	char *text;

	switch (listType)
	{
		default:
			text = StringStaticSprintf("ObjectListType %d", (int)listType);
			break;

		EnumToString(ObjectListVarType);
		EnumToString(ObjectListCharType);
		EnumToString(ObjectListIntType);
		EnumToString(ObjectListLongType);
		EnumToString(ObjectListULongType);
		EnumToString(ObjectListUidType);
		EnumToString(ObjectListTimeType);
		EnumToString(ObjectListStringType);
		EnumToString(ObjectListMemType);
	}

	return text;
}
