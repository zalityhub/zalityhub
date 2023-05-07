/*****************************************************************************

Filename:   include/hashlib.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:52 $
 * $Header: /home/hbray/cvsroot/fec/include/hashlib.h,v 1.3.4.3 2011/10/27 18:33:52 hbray Exp $
 *
 $Log: hashlib.h,v $
 Revision 1.3.4.3  2011/10/27 18:33:52  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/09/24 17:49:36  hbray
 Revision 5.5

 Revision 1.3.4.1  2011/08/18 18:26:15  hbray
 release 5.5

 Revision 1.3  2011/07/27 20:22:11  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:35  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#ident "@(#) $Id: hashlib.h,v 1.3.4.3 2011/10/27 18:33:52 hbray Exp $ "


#ifndef _HASHLIB_H
#define _HASHLIB_H

typedef NxUid_t HashKey_t;

// A hash entry

typedef struct HashEntry_t
{
	struct HashMap_t	*parentHash;
	struct HashEntry_t	*first;		// if a collision, points to first tuple
	struct HashEntry_t	*prev;		// if a collision, points to prev tuple
	struct HashEntry_t	*next;		// if a collision, points to next tuple
	int					slot;		// index in the hash map; if -1, then not in a map
	HashKey_t			key;		// the calculated key value
	void				*var;		// user provided reference
	char				*string;	// Null if not a string key, otherwise points to copy of key
} HashEntry_t;


// A hash structure

typedef struct HashMap_t
{
	int			tblSize;
	char		name[MaxNameLen];
	long		length;
	long		nbrAdds;
	long		nbrCollisions;
	long		totCollisionLength;
	long		maxCollisionLength;
	HashEntry_t	**entries;
} HashMap_t;


// Object Functions

#define HashMapNew(tblSize, name, ...) ObjectNew(HashMap, tblSize, name, ##__VA_ARGS__)
#define HashMapVerify(var) ObjectVerify(HashMap, var)
#define HashMapDelete(var) ObjectDelete(HashMap, var)

#define HashEntryDelete(var) ObjectDelete(HashEntry, var)
extern void HashEntryDestructor(HashEntry_t *this, char *file, int lno);

extern HashMap_t* HashMapConstructor(HashMap_t *this, char *file, int lno, int tblSize, char *name, ...);
extern void HashMapDestructor(HashMap_t *this, char *file, int lno);
extern BtNode_t* HashMapNodeList;
extern struct Json_t* HashMapSerialize(HashMap_t *this);
extern char* HashMapToString(HashMap_t *this);
extern BtNode_t* HashKeyNodeList;
extern struct Json_t* HashKeySerialize(HashKey_t *this);
extern char* HashKeyToString(HashKey_t *this);


// External Functions

#define HashClear(this, logLostEntries) _HashClear(this, true, logLostEntries, __FILE__, __LINE__)
extern void _HashClear(HashMap_t *this, boolean deleteLostVars, boolean logLostEntries, char *file, int lno);
extern HashEntry_t *HashGetNextEntry(HashMap_t *this, HashEntry_t *entry);
static inline Object_t* HashGetNextEntryVar(HashMap_t *this, HashEntry_t *key) {HashEntry_t *e = HashGetNextEntry(this, key); return e?e->var:NULL;}
extern HashEntry_t *HashFindEntry(HashMap_t *this, void *var);
extern Object_t *HashFindEntryVar(HashMap_t *this, void *var);
extern ObjectList_t *HashGetOrderedList(HashMap_t *this, ObjectListType_t type);
extern long HashMapLength(HashMap_t *this);

extern HashEntry_t *HashAddUINT32(HashMap_t *this, UINT32 key, void *var);
extern HashEntry_t * HashUpdateUINT32(HashMap_t *this, UINT32 key, void *var);
extern HashEntry_t *HashFindUINT32(HashMap_t *this, UINT32 key);
static inline Object_t* HashFindUINT32Var(HashMap_t *this, UINT32 key) {HashEntry_t *e = HashFindUINT32(this, key); return e?e->var:NULL;}
extern int HashDeleteUINT32(HashMap_t *this, UINT32 key);

extern HashEntry_t *HashAddUid(HashMap_t *this, NxUid_t key, void *var);
extern HashEntry_t* HashAddRandomUid(HashMap_t *this, void *var);
extern HashEntry_t * HashUpdateUid(HashMap_t *this, NxUid_t key, void *var);
extern HashEntry_t *HashFindUid(HashMap_t *this, NxUid_t key);
static inline Object_t* HashFindUidVar(HashMap_t *this, NxUid_t key) {HashEntry_t *e = HashFindUid(this, key); return e?e->var:NULL;}
extern int HashDeleteUid(HashMap_t *this, NxUid_t key);

extern HashEntry_t *HashAddString(HashMap_t *this, char *string, void *var);
extern HashEntry_t * HashUpdateString(HashMap_t *this, char *string, void *var);
extern HashEntry_t *HashFindString(HashMap_t *this, char *string);
static inline Object_t* HashFindStringVar(HashMap_t *this, char *key) {HashEntry_t *e = HashFindString(this, key); return e?e->var:NULL;}
extern int HashDeleteString(HashMap_t *this, char *string);
extern void HashMapTest(int size);

#endif
