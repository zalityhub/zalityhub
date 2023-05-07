/*****************************************************************************

Filename:   include/objlib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:53 $
 * $Header: /home/hbray/cvsroot/fec/include/objlib.h,v 1.3.4.4 2011/10/27 18:33:53 hbray Exp $
 *
 $Log: objlib.h,v $
 Revision 1.3.4.4  2011/10/27 18:33:53  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/24 17:49:37  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/01 14:49:43  hbray
 Revision 5.5

 Revision 1.3  2011/07/27 20:22:12  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:36  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: objlib.h,v 1.3.4.4 2011/10/27 18:33:53 hbray Exp $ "

#ifndef _OBJLIB_H
#define _OBJLIB_H


#include <sys/mman.h>
#include "include/signatures.h"


#define ObjectSignature DefineSignature(0xbeef)
#define ObjectSignatureIsValid(obj) (SignatureIsValid(obj, ObjectSignature))


typedef enum ObjectListPosition_t
{
	ObjectListFirstPosition,
	ObjectListLastPosition,
	ObjectListCursorPosition
} ObjectListPosition_t ;		// where to put/remove entries

extern char *ObjectPositionToString(ObjectListPosition_t position);


typedef enum ObjectListType_t
{
	ObjectListVarType,
	ObjectListCharType,
	ObjectListIntType,
	ObjectListLongType,
	ObjectListULongType,
	ObjectListUidType,
	ObjectListTimeType,
	ObjectListStringType,
	ObjectListMemType
} ObjectListType_t ;

extern char *ObjectListTypeToString(ObjectListType_t type);


typedef struct ObjectList_t
{
	char				name[MaxNameLen];
	ObjectListType_t	listType;
	struct ObjectLink_t	*first;		// pointer to first entry
	struct ObjectLink_t	*last;		// pointer to last entry
	long				length;
} ObjectList_t;


typedef struct ObjectLink_t
{
	ObjectList_t		*listHead;	// pointer to list head
	void				*var;		// pointer to entry
	struct ObjectLink_t	*next;		// forward link
	struct ObjectLink_t	*prev;		// backward link
} ObjectLink_t ;


#define ObjectListNew(listType, name, ...) ObjectNew(ObjectList, listType, name, ##__VA_ARGS__)
#define ObjectListVerify(var) ObjectVerify(ObjectList, var)
#define ObjectListDelete(var) ObjectDelete(ObjectList, var)
extern ObjectList_t* ObjectListConstructor(ObjectList_t *this, char *file, int lno, ObjectListType_t listType, char *name, ...);
extern void ObjectListDestructor(ObjectList_t *this, char *file, int lno);
extern BtNode_t *ObjectListNodeList;
extern struct Json_t* ObjectListSerialize(ObjectList_t *this);
extern char* ObjectListToString(ObjectList_t *this);

#define ObjectListClear(this, logLostEntries) _ObjectListClear(this, true, logLostEntries, __FILE__, __LINE__)
extern void _ObjectListClear(ObjectList_t *this, boolean deleteLostVars, boolean logLostEntries, char *file, int lno);
extern void* ObjectListRemove(ObjectList_t *this, ObjectListPosition_t position, ...);
extern void* ObjectListYank(ObjectList_t *this, ObjectLink_t *link);
extern long ObjectListNbrEntries(ObjectList_t *this);


extern void ObjectLinkDestructor(ObjectLink_t *this, char *file, int lno);
extern ObjectLink_t* ObjectListAdd(ObjectList_t *this, void *var, ObjectListPosition_t position, ...);
extern ObjectLink_t* ObjectListAddOrdered(ObjectList_t *this, void *var, void *key, short int len);
extern ObjectLink_t* ObjectListGetLink(ObjectList_t *this, void *var);

extern ObjectLink_t* ObjectListFirst(ObjectList_t *this);
static inline void* ObjectListFirstVar(ObjectList_t *this) {ObjectLink_t *l = ObjectListFirst(this); return l?l->var:NULL;}

extern ObjectLink_t* ObjectListLast(ObjectList_t *this);
static inline void* ObjectListLastVar(ObjectList_t *this) {ObjectLink_t *l = ObjectListLast(this); return l?l->var:NULL;}

extern ObjectLink_t* ObjectListPrev(ObjectLink_t *this);
static inline void* ObjectListPrevVar(ObjectLink_t *this) {ObjectLink_t *l = ObjectListPrev(this); return l?l->var:NULL;}

extern ObjectLink_t* ObjectListNext(ObjectLink_t *this);
static inline void* ObjectListNextVar(ObjectLink_t *this) {ObjectLink_t *l = ObjectListNext(this); return l?l->var:NULL;}


typedef struct Object_t
{
	long			signature;

	int				ownerPid;
	NxTime_t		time;
	boolean			shared;
	char			*sharedFname;
	Spin_t			lock;
	size_t			shmlen;
	int				size;
	char			*objName;
	char			*file;
	int				lno;
	void*			(*constructor)(void*,char*,int,...);
	void			(*destructor)(void*,char*,int);
	struct Json_t*	(*serialize)(void*);
	char*			(*toString)(void*,...);
	BtNode_t		**nodeList;
} Object_t;						// Object's instance immediately follows this struct


#define VarToObject(var) (var?((Object_t*)(((void*)(var))-sizeof(Object_t))):((Object_t*)NULL))
#define ObjectToVar(obj) (obj?((void*)(((void*)(obj))+sizeof(Object_t))):((void*)NULL))

#define ObjectNew(_objName, ...) _ObjectNew(__FILE__,__LINE__,__FUNC__,_objName, ##__VA_ARGS__)
#define _ObjectNew(file,lno,func,_objName, ...) (_objName##_t*)(_objName##Constructor((_objName##_t*)(__ObjectNew(sizeof(_objName##_t),(void*(*)(void*,char*,int,...))(_objName##Constructor),(void(*)(void*,char*,int))(_objName##Destructor),&(_objName##NodeList), (struct Json_t*(*)(void*))(_objName##Serialize),(char*(*)(void*,...))(_objName##ToString), #_objName "_t", file,lno)),file,lno,##__VA_ARGS__))

#define ObjectNewSized(_objName, size, ...) _ObjectNewSized(__FILE__,__LINE__,__FUNC__,_objName, size, ##__VA_ARGS__)
#define _ObjectNewSized(file,lno,func,_objName, size, ...) (_objName##_t*)(_objName##Constructor((_objName##_t*)(__ObjectNew((size)+sizeof(_objName##_t),(void*(*)(void*,char*,int,...))(_objName##Constructor),(void(*)(void*,char*,int))(_objName##Destructor),&(_objName##NodeList), (struct Json_t*(*)(void*))(_objName##Serialize),(char*(*)(void*,...))(_objName##ToString), #_objName "_t", file,lno)),file,lno,size,##__VA_ARGS__))

#define ObjectNewShared(fname, _objName, ...) _ObjectNewShared(__FILE__,__LINE__,__FUNC__,fname, _objName,##__VA_ARGS__)
#define _ObjectNewShared(file,lno,func,fname, _objName, ...) (_objName##_t*)(_objName##Constructor((_objName##_t*)(__ObjectNewShared(fname, sizeof(_objName##_t), true, (void*(*)(void*,char*,int,...))(_objName##Constructor),(void(*)(void*,char*,int))(_objName##Destructor),&(_objName##NodeList), (struct Json_t*(*)(void*))(_objName##Serialize),(char*(*)(void*,...))(_objName##ToString), #_objName "_t", file,lno)),file,lno,##__VA_ARGS__))

#define ObjectNewSharedSized(fname, _objName, size, ...) _ObjectNewSharedSized(__FILE__,__LINE__,__FUNC__,fname,_objName, size, ##__VA_ARGS__)
#define _ObjectNewSharedSized(file,lno,func,fname, _objName, size, ...) (_objName##_t*)(_objName##Constructor((_objName##_t*)(__ObjectNewShared(fname, (size)+sizeof(_objName##_t), true, (void*(*)(void*,char*,int,...))(_objName##Constructor),(void(*)(void*,char*,int))(_objName##Destructor),&(_objName##NodeList), (struct Json_t*(*)(void*))(_objName##Serialize),(char*(*)(void*,...))(_objName##ToString), #_objName "_t", file,lno)),file,lno,size,##__VA_ARGS__))

#define ObjectMapShared(fname, _objName, ...) (_objName##_t*)(__ObjectNewShared(fname, sizeof(_objName##_t), false, (void*(*)(void*,char*,int,...))(_objName##Constructor),(void(*)(void*,char*,int))(_objName##Destructor),&(_objName##NodeList), (struct Json_t*(*)(void*))(_objName##Serialize),(char*(*)(void*,...))(_objName##ToString), #_objName "_t", __FILE__,__LINE__))

#define ObjectSync(_objName, _this) _ObjectSync((void*)(_this), (void(*)(void*,char*,int))_objName##Destructor,#_objName "_t",__FILE__,__LINE__)

#define ObjectVerify(_objName, _this) _ObjectVerify((void*)(_this), (void(*)(void*,char*,int))_objName##Destructor,#_objName "_t", __FILE__,__LINE__)
#define ObjectTestVerify(_objName, _this) _ObjectTestVerify((void*)(_this), (void(*)(void*,char*,int))_objName##Destructor)

#define ObjectDelete(_objName, _this) _ObjectDelete((void**)(&(_this)), (void(*)(void*,char*,int))_objName##Destructor,#_objName "_t",__FILE__,__LINE__)
#define ObjectStringStamp(_this) _ObjectToStringRaw((void*)(_this), __FILE__,__LINE__)
#define ObjectToName(_this) _ObjectToName((void*)(_this), __FILE__,__LINE__)
#define ObjectGetNodeList(_this) _ObjectGetNodeList((void*)(_this), __FILE__, __LINE__)
#define ObjectSerialize(_this) _ObjectSerialize((void*)(_this), __FILE__, __LINE__)
#define ObjectToString(_this) _ObjectToString((void*)(_this), __FILE__,__LINE__)

#define ObjectTypeToString(var) (_ObjectVerify(var, VarToObject(var)->destructor, VarToObject(var)->objName, __FILE__, __LINE__)->objName)
#define ObjectIsShared(var) (_ObjectVerify(var, VarToObject(var)->destructor, VarToObject(var)->objName, __FILE__, __LINE__)->shared)
#define ObjectIsOwner(var) (_ObjectVerify(var, VarToObject(var)->destructor, VarToObject(var)->objName, __FILE__, __LINE__)->ownerPid==getpid())
#define ObjectMakeOwner(var) (_ObjectVerify(var, VarToObject(var)->destructor, VarToObject(var)->objName, __FILE__, __LINE__)->ownerPid=getpid())


// Object Functions

extern void* __ObjectNew(int size, void*(*constructor)(void*, char*, int, ...), void(*destructor)(void*,char*,int), BtNode_t**nodeList, struct Json_t*(*serialize)(void*),char*(*toString)(void*,...), char *_objName, char *file, int lno);
extern void* __ObjectNewShared(char *fname, int size, boolean new, void*(*constructor)(void*, char*, int, ...), void(*destructor)(void*,char*,int), BtNode_t**nodeList, struct Json_t*(*serialize)(void*),char*(*toString)(void*,...), char *_objName, char *file, int lno);
extern char* _ObjectToStringRaw(void *var, char *file, int lno);
extern char* _ObjectToName(void *var, char *file, int lno);
extern BtNode_t* _ObjectGetNodeList(void *var, char *file, int lno);
extern struct Json_t* _ObjectSerialize(void *var, char *file, int lno);
extern char* _ObjectToString(void *var, char *file, int lno);
extern void _ObjectDelete(void **var, void(*destructor)(void*,char*,int), char *_objName, char *file, int lno);
extern void _ObjectSync(void *var, void(*destructor)(void*,char*,int), char *_objName, char *file, int lno);
extern boolean _ObjectTestVerify(void *var, void(*destructor)(void*,char*,int));
extern Object_t* _ObjectVerify(void *var, void(*destructor)(void*,char*,int), char *objName, char *file, int lno);

extern void ObjectLock(void *var);
extern void ObjectUnlock(void *var);

extern struct String_t* ObjectTreeToString(BtNode_t *node, struct String_t *out);
extern void ObjectTreeVerify();

extern BtNode_t	*RootObjectList;

#endif
