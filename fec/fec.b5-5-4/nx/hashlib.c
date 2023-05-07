/*****************************************************************************

Filename:   lib/nx/hashlib.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
 * $Author: hbray $
 * $Date: 2011/10/27 18:33:56 $
 * $Header: /home/hbray/cvsroot/fec/lib/nx/hashlib.c,v 1.3.4.4 2011/10/27 18:33:56 hbray Exp $
 *
 $Log: hashlib.c,v $
 Revision 1.3.4.4  2011/10/27 18:33:56  hbray
 Revision 5.5

 Revision 1.3.4.3  2011/09/24 17:49:44  hbray
 Revision 5.5

 Revision 1.3.4.2  2011/08/18 18:44:29  hbray
 *** empty log message ***

 Revision 1.3.4.1  2011/08/18 18:26:17  hbray
 release 5.5

 Revision 1.3  2011/07/27 20:22:18  hbray
 Merge 5.5.2

 Revision 1.2.2.1  2011/07/27 20:19:45  hbray
 Added cvs headers

 *
2009.07.15 harold bray          Created release 4.0
*****************************************************************************/

#ident "@(#) $Id: hashlib.c,v 1.3.4.4 2011/10/27 18:33:56 hbray Exp $ "


#include "include/stdapp.h"
#include "include/libnx.h"
#include "include/crc.h"
#include "include/signatures.h"
#include "include/random.h"

#include "include/gmp.h"


// Object Functions

#define HashEntryNew(string) ObjectNew(HashEntry, string)
#define HashEntryVerify(var) ObjectVerify(HashEntry, var)

static HashEntry_t* HashEntryConstructor(HashEntry_t *this, char *file, int lno, char *string);
static HashEntry_t* HashEntryDup(HashEntry_t *this);
static BtNode_t* HashEntryNodeList = NULL;
static Json_t* HashEntrySerialize(HashEntry_t *this);
static char* HashEntryToString(HashEntry_t *this);
static Json_t* HashCollisionSerialize(HashEntry_t *this);


// Static Functions

static inline HashKey_t
HashKeyFromUint64(UINT64 u64)
{
	union
	{
		UINT64			u64;
		unsigned char	u[sizeof(u64)];
	} u[2] ;

	u[0].u64 = u64;
 	for (int i = 0; i < sizeof(u[0].u64); ++i)
		u[1].u[sizeof(u[0].u64)-1-i] = u[0].u[i];
	HashKey_t key;
	key.ull[0] = 0;
	key.ull[1] = u[1].u64;
	return key;
}

static inline HashKey_t
HashKeyFromUint32(UINT32 u32)
{
	return HashKeyFromUint64((UINT64)u32);
}


static inline UINT32
HashIndexFromUint32(int tblSize, UINT32 u)
{
	UINT32 hidx = (UINT32)(u % tblSize);	// initial index
	return hidx;
}


static inline UINT32
HashIndexFromKey(int tblSize, HashKey_t key)
{
	static boolean	init = false;

	static mpz_t r;
	static mpz_t d;

	if ( ! init )
	{
		mpz_init2(r, 128);
		mpz_init2(d, 128);
		init = true;
	}

	mpz_t n;
	char *text = NxUidToString(key);
	mpz_init_set_str (n, text, 16);

	mpz_set_ui(d, tblSize);

	mpz_mod (r, n, d);
	mpz_clear(n);

	UINT32 hidx = mpz_get_ui(r);
	return hidx;
}


static int HashIsPrime(int value);
static int HashPrime(int minValue);
static HashEntry_t *_HashEntryAdd(HashMap_t *this, UINT32 hidx, HashKey_t key, void *var, char *string);
static HashEntry_t *_HashEntryFind(HashMap_t *this, HashKey_t key, char *string);
static int _HashEntryDelete(HashMap_t *this, HashKey_t key, char *string);




BtNode_t *HashMapNodeList = NULL;


HashMap_t*
HashMapConstructor(HashMap_t *this, char *file, int lno, int tblSize, char *name, ...)
{
	if (tblSize <= 0)
		SysLogFull(LogError, file, lno, __FUNC__, "tblSize [%d]<= 0; using %d", tblSize, 128);

	{
		va_list ap;
		va_start(ap, name);
		vsnprintf(this->name, sizeof(this->name), name, ap);
	}

	tblSize = HashPrime(tblSize);

	this->tblSize = tblSize;

	int len = sizeof(HashEntry_t *) * tblSize;
	if ((unsigned)len < (UINT32)((long)sizeof(HashEntry_t *) * (long)tblSize))
		SysLogFull(LogFatal, file, lno, __FUNC__, "Unable to allocate memory");

	if ((this->entries = (HashEntry_t **)malloc(len)) == NULL)
		SysLogFull(LogFatal, file, lno, __FUNC__, "Unable to allocate memory");

	memset(this->entries, 0, len);
	return this;
}


/*+******************************************************************
	Name:
		HashMapDestructor(2) - destroy a hash map and its hashed entries.

	Synopsis:
		#include "include/hashlib.h"

		HashMapDestructor (this)
		HashMap_t	*this;

	Description:
		This function will dispose of all entries in a hash map then
		dispose of the hash map.  The function frees all memory associated
		with the hash map.

	Diagnostics:
		Returns zero for success, -1 for failure.
-*******************************************************************/

void
HashMapDestructor(HashMap_t *this, char *file, int lno)
{

	HashMapVerify(this);
	_HashClear(this, false, true, file, lno);
	free((char *)this->entries);
}


Json_t*
HashMapSerialize(HashMap_t *this)
{
	HashMapVerify(this);
	Json_t *root = JsonNew(__FUNC__);

	JsonAddNumber(root, "TblSize", this->tblSize);
	JsonAddString(root, "Name", this->name);
	JsonAddNumber(root, "Length", this->length);
	JsonAddNumber(root, "NbrAdds", this->nbrAdds);
	if ( this->nbrCollisions > 0 )
	{
		JsonAddNumber(root, "NbrCollisions", this->nbrCollisions);
		JsonAddNumber(root, "TotCollisionLength", this->totCollisionLength);
		JsonAddNumber(root, "MaxCollisionLength", this->maxCollisionLength);
	}

	// if (recursive)
	{
		Json_t *sub = JsonPushObject(root, "Entries");
		for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(this, entry)) != NULL;)
		{
			if ( entry->next != NULL || entry->first != NULL )		// collision
				JsonAddItem(sub, "Entry", HashCollisionSerialize(entry));
			else
				JsonAddItem(sub, "Entry", HashEntrySerialize(entry));		// regular
		}
	}

	return root;
}


char*
HashMapToString(HashMap_t *this)
{
	Json_t *root = HashMapSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


/*+******************************************************************
	Name:
		_HashClear(2) - Dispose of all entries in a hash map.

	Synopsis:
		#include "include/hashlib.h"

		int _HashClear (this, logLostEntries)
		HashMap_t	*this;
		boolean		logLostEntries;

	Description:
		This function will remove and dispose of all the entries in the
		given hash map.

	Diagnostics:
		Returns zero for success, -1 for failure.
-*******************************************************************/

void
_HashClear(HashMap_t *this, boolean deleteLostVars, boolean logLostEntries, char *file, int lno)
{

	HashMapVerify(this);

	for (unsigned int i = 0; i < this->tblSize; ++i)
	{
		HashEntry_t *entry;

		while ((entry = this->entries[i]) != NULL)
		{
			HashEntryVerify(entry);

			if ( logLostEntries )
				SysLogFull(LogError, file, lno, __FUNC__, "%s disposing of possible lost entry %s", this->name, ObjectStringStamp(entry->var));
			if ( deleteLostVars )
			{
				Object_t *obj = VarToObject(entry->var);
				if ( ! ObjectSignatureIsValid(obj) )
					_NxCrash(NxGlobal, file, lno, __FUNC__, "Bad object signature");
				_ObjectDelete(&entry->var, obj->destructor, obj->objName, obj->file, obj->lno);
			}
			HashEntryDelete(entry);
		}
	}

	this->nbrAdds = this->nbrCollisions = this->totCollisionLength = this->maxCollisionLength = 0;
}


/*+******************************************************************
	Name:
		HashGetNextEntry(2) - Get next entry in a hash map

	Synopsis:
		#include "include/hashlib.h"

		int HashGetNextEntry (this, entry)
		HashMap_t		*this;
		HashEntry_t		*entry;

	Description:
		This function will locate the next 'physical' entry in a hash map.
		The entries are not returned in any particular sequence. Pass the
		'entry' parameter set to NULL to obtain the first entry.
		Subsequent entries will be returned by forward referencing the
		'entry' parameter.

	Diagnostics:
		Returns zero for success, -1 for failure.
-*******************************************************************/

HashEntry_t *
HashGetNextEntry(HashMap_t *this, HashEntry_t *entry)
{
	HashMapVerify(this);

	if (entry == NULL)			// first time; find first physical
	{
		for (UINT32 i = 0; entry == NULL && i < this->tblSize; ++i)
			entry = this->entries[i];
		return entry;			// return the entry or null
	}

// find next from the provided entry
	HashEntry_t *next;
	HashEntryVerify(entry);

	if ((next = entry->next) != NULL)
		return next;			// return the next collision entry

	if (entry->first != NULL)	// if we chased a collision chain
		entry = entry->first;	// back to the first entry

	if (entry->slot >= this->tblSize)
		return NULL;			// end of list (redundant check?)

// scan for next physical
	int i;
	for (next = NULL, i = entry->slot + 1; next == NULL && i < this->tblSize; ++i)
		next = this->entries[i];

	return next;
}


/*+******************************************************************
	Name:
		HashAddUINT32(2) - Add a UINT32 entry to a hash map.

	Synopsis:
		#include "include/hashlib.h"

		HashEntry_t* HashAddUINT32 (this, key, var)
		HashMap_t		*this;
		UINT32				key;
		void			*var;

	Description:
		Add a new entry to a hash map. Duplicate entries are not allowed.
		The entry is referenced by a UINT32 key.
		This 'key' is reduced to an index in the range of the
		hash map and the key and its associated var is
		entered into the table.

	Diagnostics:
		Returns entry address after success, otherwise returns NULL.
-*******************************************************************/

HashEntry_t*
HashAddUINT32(HashMap_t *this, UINT32 key, void *var)
{
	HashMapVerify(this);
	return _HashEntryAdd(this, HashIndexFromUint32(this->tblSize, key), HashKeyFromUint32(key), var, NULL);
}


HashEntry_t*
HashUpdateUINT32(HashMap_t *this, UINT32 key, void *var)
{
    HashMapVerify(this);
    if ( ! ObjectSignatureIsValid(VarToObject(var)) )
        SysLog(LogFatal, "Invalid signature in passed object");

    HashEntry_t *entry;
    if ( (entry = HashFindUINT32(this, key)) != NULL )
    {
        Object_t *obj = VarToObject(entry->var);
        _ObjectDelete((void**) (&(entry->var)), obj->destructor, obj->objName, __FILE__, __LINE__);
        HashDeleteUINT32(this, key);	// remove previous value
    }
    return HashAddUINT32(this, key, var);
}


/*+******************************************************************
	Name:
		HashFindUINT32(2) - Locate a UINT32 entry in a hash map.

	Synopsis:
		#include "include/hashlib.h"

		HashEntry_t *HashFindUINT32 (this, key)
		HashMap_t		*this;
		UINT32				key;

	Description:
		This function will probe the hash map and locate the entry that
		matches the given UINT32 key value.

	Diagnostics:
		Returns zero for success, -1 for failure.
-*******************************************************************/

HashEntry_t *
HashFindUINT32(HashMap_t *this, UINT32 key)
{
	HashMapVerify(this);
	return _HashEntryFind(this, HashKeyFromUint32(key), NULL);
}


/*+******************************************************************
	Name:
		HashDeleteUINT32(2) - Remove a UINT32 entry from a hash map.

	Synopsis:
		#include "include/hashlib.h"

		int HashDeleteUINT32 (this, key)
		HashMap_t		*this;
		UINT32				key;

	Description:
		This function will probe the hash map and remove the entry that
		matches the given UINT32 key value.

	Diagnostics:
		Returns zero for success, -1 for failure.
-*******************************************************************/

int
HashDeleteUINT32(HashMap_t *this, UINT32 key)
{
	HashMapVerify(this);
	return _HashEntryDelete(this, HashKeyFromUint32(key), NULL);
}


/*+******************************************************************
	Name:
		HashAddUid(2) - Add a uid entry to a hash map.

	Synopsis:
		#include "include/hashlib.h"

		HashEntry_t* HashAddUid(this, key, var)
		HashMap_t		*this;
		uid				key;
		void			*var;

	Description:
		Add a new entry to a hash map. Duplicate entries are not allowed.
		The entry is referenced by a uid key.
		This 'key' is reduced to an index in the range of the
		hash map and the key and its associated var is
		entered into the table.

	Diagnostics:
		Returns entry address after success, otherwise returns NULL.
-*******************************************************************/

HashEntry_t*
HashAddUid(HashMap_t *this, NxUid_t key, void *var)
{
	HashMapVerify(this);
	return _HashEntryAdd(this, HashIndexFromKey(this->tblSize, key), key, var, NULL);
}


HashEntry_t*
HashUpdateUid(HashMap_t *this, NxUid_t key, void *var)
{
    HashMapVerify(this);
    if ( ! ObjectSignatureIsValid(VarToObject(var)) )
        SysLog(LogFatal, "Invalid signature in passed object");

    HashEntry_t *entry;
    if ( (entry = HashFindUid(this, key)) != NULL )
    {
        Object_t *obj = VarToObject(entry->var);
        _ObjectDelete((void**) (&(entry->var)), obj->destructor, obj->objName, __FILE__, __LINE__);
        HashDeleteUid(this, key);	// remove previous value
    }
    return HashAddUid(this, key, var);
}


/*+******************************************************************
	Name:
		HashFindUid(2) - Locate a NxUid_t entry in a hash map.

	Synopsis:
		#include "include/hashlib.h"

		HashEntry_t *HashFindUid (this, key)
		HashMap_t		*this;
		NxUid_t			key;

	Description:
		This function will probe the hash map and locate the entry that
		matches the given NxUid_t key value.

	Diagnostics:
		Returns zero for success, -1 for failure.
-*******************************************************************/

HashEntry_t *
HashFindUid(HashMap_t *this, NxUid_t key)
{
	HashMapVerify(this);
	return _HashEntryFind(this, key, NULL);
}


/*+******************************************************************
	Name:
		HashDeleteUid(2) - Remove a NxUid_t entry from a hash map.

	Synopsis:
		#include "include/hashlib.h"

		int HashDeleteUid (this, key)
		HashMap_t		*this;
		NxUid_t			key;

	Description:
		This function will probe the hash map and remove the entry that
		matches the given NxUid_t key value.

	Diagnostics:
		Returns zero for success, -1 for failure.
-*******************************************************************/

int
HashDeleteUid(HashMap_t *this, NxUid_t key)
{
	HashMapVerify(this);
	return _HashEntryDelete(this, key, NULL);
}


/*+******************************************************************
	Name:
		HashAddString(2) - Add a string entry to a hash map.

	Synopsis:
		#include "include/hashlib.h"

		HashEntry_t* HashAddString (this, string, var)
		HashMap_t		*this;
		char			*string;
		void			*var;

	Description:
		Add a new entry to a hash map. Duplicate entries are not allowed.
		The entry is defined by a character string key.
		This 'string' is reduced to an index in the range of the
		hash map and the string key and its associated var is
		entered into the table.

	Diagnostics:
		Returns entry address after success, otherwise returns NULL.
-*******************************************************************/

HashEntry_t *
HashAddString(HashMap_t *this, char *string, void *var)
{
	HashMapVerify(this);
	SysLog(LogDebug, "%s to %s", string, this->name);
	UINT32 crc32 = Crc32String((unsigned char*)string);
	HashKey_t key = HashKeyFromUint32(crc32);
	UINT32 index = HashIndexFromUint32(this->tblSize, crc32);
	return _HashEntryAdd(this, index, key, var, string);
}


HashEntry_t *
HashUpdateString(HashMap_t *this, char *string, void *var)
{
	HashMapVerify(this);
	if ( ! ObjectSignatureIsValid(VarToObject(var)) )
		SysLog(LogFatal, "Invalid signature in passed object");

	HashEntry_t *entry;
	if ( (entry = HashFindString(this, string)) != NULL)
	{
		Object_t *obj = VarToObject(entry->var);
		_ObjectDelete((void**)(&(entry->var)), obj->destructor, obj->objName, __FILE__, __LINE__);
		HashDeleteString(this, string);	// remove previous value
	}
	return HashAddString(this, string, var);
}


/*+******************************************************************
	Name:
		HashFindString(2) - Locate a string entry in a hash map.

	Synopsis:
		#include "include/hashlib.h"

		HashEntry_t* HashFindString (this, key)
		HashMap_t		*this;
		char			*key;

	Description:
		This function will probe the hash map and locate the entry that
		matches the given character string key value.

	Diagnostics:
		Returns zero for success, -1 for failure.
-*******************************************************************/

HashEntry_t *
HashFindString(HashMap_t *this, char *string)
{
	HashMapVerify(this);
	UINT32 crc32 = Crc32String((unsigned char*)string);
	HashKey_t key = HashKeyFromUint32(crc32);
	return _HashEntryFind(this, key, string);
}


/*+******************************************************************
	Name:
		HashDeleteString(2) - Remove a string entry from a hash map.

	Synopsis:
		#include "include/hashlib.h"

		int HashDeleteString (this, key)
		HashMap_t		*this;
		char			*key;

	Description:
		This function will probe the hash map and remove the entry that
		matches the given character string key value.

	Diagnostics:
		Returns zero for success, -1 for failure.
-*******************************************************************/

int
HashDeleteString(HashMap_t *this, char *string)
{
	HashMapVerify(this);
	SysLog(LogDebug, "%s from %s", string, this->name);
	HashKey_t key = HashKeyFromUint32(Crc32String((unsigned char*)string));
	return _HashEntryDelete(this, key, string);
}


ObjectList_t*
HashGetOrderedList(HashMap_t *this, ObjectListType_t listType)
{

	HashMapVerify(this);

	ObjectList_t *list = ObjectListNew(listType, "%s HashOrderedList", this->name);

	for (HashEntry_t *entry = NULL; (entry = HashGetNextEntry(this, entry)) != NULL;)
	{
		HashEntry_t *dup = HashEntryDup(entry);
		if ( ObjectListAddOrdered(list, dup, &dup->string, 0) == NULL)
			SysLog(LogFatal, "%s ObjectListAddOrdered of %s failed", this->name, ObjectToString(entry->var));
	}

	return list;
}


long
HashMapLength(HashMap_t *this)
{
	HashMapVerify(this);
	return this->length;
}


/*+******************************************************************
	Name:
		HashFindTagEntry(2) - Locate an entry in a hash map.

	Synopsis:
		#include "include/hashlib.h"

		HashEntry_t *HashFindTagEntry (this, var)
		HashMap_t		*this;
		void			*var;

	Description:
		This function will probe the hash map and locate the entry that
		matches the given var value.

	Diagnostics:
		Returns zero for success, -1 for failure.
-*******************************************************************/

HashEntry_t *
HashFindTagEntry(HashMap_t *this, void *var)
{

	HashMapVerify(this);

	for (unsigned int i = 0; i < this->tblSize; ++i)
	{
		HashEntry_t *entry;
		if ((entry = this->entries[i]) != NULL)
		{
			if (entry->var == var)
				return entry;
			for (HashEntry_t *next = entry->next; next;)	// search for siblings
			{
				if (next->var == var)
					return next;
				next = next->next;
			}
		}
	}

	return NULL;				// not found
}


static HashEntry_t *
_HashEntryAdd(HashMap_t *this, UINT32 hidx, HashKey_t key, void *var, char *string)
{
	int chainLength = 0;
	HashEntry_t *pEntry;
	HashEntry_t *aEntry;

	HashMapVerify(this);

	if ( ! ObjectSignatureIsValid(VarToObject(var)) )
		SysLog(LogFatal, "Invalid signature in passed object");

	if ((pEntry = aEntry = this->entries[hidx]) != NULL)	// already have one of these...
	{
		for (chainLength = 1;; ++chainLength)	// search the collision list for possible dup
		{
			if (((NxUidCompare(aEntry->key, key)==0) && (string == NULL || aEntry->string == NULL)) || (string != NULL && strcmp(string, aEntry->string) == 0))
			{
				SysLog(LogError, "Duplicate key [%s.%d%s%s] is not allowed", HashKeyToString(&key), hidx, string ? ":" : "", string ? string : "");
				return NULL;
			}
			pEntry = aEntry;
			if ((aEntry = pEntry->next) == NULL)
				break;			// end of collision list
		}
	}

	if ( (aEntry = HashEntryNew(string)) == NULL )
		SysLog(LogFatal, "Unable to allocate memory");

	aEntry->parentHash = this;
	aEntry->key = key;
	aEntry->var = var;

	if (pEntry)					// collision
	{
		aEntry->first = this->entries[hidx];       // the root
		aEntry->next = NULL;
		aEntry->prev = pEntry;
		pEntry->next = aEntry;
		++(this->nbrCollisions);
		this->totCollisionLength += chainLength;
		if (chainLength > this->maxCollisionLength)
			this->maxCollisionLength = chainLength;
		// SysLog(LogDebug, "%s", HashMapToString(this));
	}
	else
	{
		this->entries[hidx] = aEntry;
		aEntry->slot = hidx;
	}

	++(this->length);
	++(this->nbrAdds);

	return aEntry;
}


static HashEntry_t *
_HashEntryFind(HashMap_t *this, HashKey_t key, char *string)
{

	HashMapVerify(this);

	UINT32 hidx = HashIndexFromKey(this->tblSize, key);

	HashEntry_t *entry;
	if ((entry = this->entries[hidx]) != NULL)	// got something
	{
		for (;;)		// chase possible collision...
		{
			if ( NxUidCompare(entry->key, key) == 0 )
			{
				if (string == NULL || entry->string == NULL)
					return entry;
				if (strcmp(string, entry->string) == 0)
					return entry;
			}
			if ((entry = entry->next) == NULL)
				break;			// end of list
		}
	}

	return NULL;				// not found
}


static int
_HashEntryDelete(HashMap_t *this, HashKey_t key, char *string)
{

	HashMapVerify(this);

	UINT32 hidx = HashIndexFromKey(this->tblSize, key);

    HashEntry_t *entry;
    if ((entry = this->entries[hidx]) == NULL) // nothing here
        return -1;                  // not found

    if ( entry->first || entry->next || entry->prev )      // collision chain
    {
        if ( (entry = _HashEntryFind(this, key, string)) == NULL )
            return -1;      // not found
    }

    HashEntryDelete(entry);
    return 0;       // ok
}


static HashEntry_t*
HashEntryConstructor(HashEntry_t *this, char *file, int lno, char *string)
{

	if (string != NULL)
	{
		if ((this->string = strdup(string)) == NULL)
			SysLog(LogFatal, "Unable to allocate memory");
	}

	return this;
}


void
HashEntryDestructor(HashEntry_t *this, char *file, int lno)
{
    HashMap_t *map = this->parentHash;
	if ( map != NULL )		// not a dangling dup
	{
    	HashMapVerify(map);

		UINT32 hidx = HashIndexFromKey(map->tblSize, this->key);

    	if ( this->first || this->next || this->prev )      // collision chain
    	{
        	if (this->prev != NULL)         // not the first; close the hole being made
        	{
            	if ( (this->prev->next = this->next) )
            		this->next->prev = this->prev;
        	}
        	else if (map->entries[hidx] == this)   // removing the root (first)
        	{
            	HashEntry_t *first = this->next;
            	if ((map->entries[hidx] = first) != NULL)      // set new first; and its a 'live' one
            	{
                	first->first = NULL;    // new first
					first->prev = NULL;
                	first->slot = hidx;

                	// have a new root, change all children
                	for (HashEntry_t *tmp = first; (tmp = tmp->next) != NULL;)
                    	tmp->first = first;
            	}
        	}
    	}
    	else
    	{
        	map->entries[hidx] = NULL; // simple remove
    	}
    	--(map->length);
	}

	if (this->string)
		free(this->string);
}


static HashEntry_t*
HashEntryDup(HashEntry_t *this)
{
	HashEntryVerify(this);
	HashEntry_t *new = HashEntryNew(this->string);
	new->first = this->first;
	new->next = this->next;
	new->slot = this->slot;
	new->key = this->key;
	new->var = this->var;
	return new;
}


char*
HashKeyToString(HashKey_t *this)
{
	return NxUidToString(*this);
}


static Json_t*
HashEntrySerialize(HashEntry_t *this)
{
	HashEntryVerify(this);

	if ( this->string != NULL && strcmp(this->string, "TraceLog.Port") == 0 )
		printf("%s\n", this->string);

	Json_t *root = JsonNew(__FUNC__);
	JsonAddPointer(root, "This", this);
	if ( this->parentHash != NULL )
		JsonAddString(root, "ParentMap", "%s", this->parentHash->name);
	JsonAddPointer(root, "First", this->first);
	JsonAddPointer(root, "Next", this->next);
	JsonAddNumber(root, "Slot", this->slot);
	JsonAddString(root, "Key", "%s", HashKeyToString(&this->key));
	JsonAddPointer(root, "Var", (void *)this->var);
	if ( this->string != NULL )
		JsonAddString(root, "String", "%s", this->string);
	return root;
}


static char*
HashEntryToString(HashEntry_t *this)
{
	Json_t *root = HashEntrySerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}


static Json_t*
HashCollisionSerialize(HashEntry_t *this)
{
	HashEntryVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddItem(root, "This", HashEntrySerialize(this));
	for ( HashEntry_t *entry = this->next; entry != NULL; entry = entry->next)
		JsonAddItem(root, "Entry", HashEntrySerialize(entry));
	return root;
}


/*+******************************************************************
	Name:
		HashIsPrime - Test a number for its primeness

	Synopsis:
		int HashIsPrime (value)
		int		value;

	Description:
		The given value will be tested to determine if it is a prime
		number.

	Diagnostics:
		Returns true if the value is prime or
		false if it is not.
-*******************************************************************/

static int
HashIsPrime(int value)
{
	int v;
	int v2;


	if ((value & 1) == 0)
		return 0;				// even, can't be prime

	v = value;
	v2 = v / 2;					// this should be the square root of value; but, takes too long to get sqrt
	for (int f = 3; f < v2; f += 2)
	{
		if ((v % f) == 0)
			return 0;
	}

	return 1;
}


/*+******************************************************************
	Name:
		HashPrime(2) - Return a prime number.

	Synopsis:
		int HashPrime (minValue)
		int		minValue;

	Description:
		Returns a prime number that is at least as large as the
		provided minValue.  In other words, the first prime number
		that is >= minValue.

		Returns a prime number >= minValue.

	Diagnostics:
		None.
-*******************************************************************/

static int
HashPrime(int minValue)
{
	int c;

	for (c = minValue | 1;		// force the minValue odd
		 !HashIsPrime(c); c += 2)
	{ }

	return c;					// this is it
}


static void
HashMapVerifySize(HashMap_t *map, int size)
{
	HashMapVerify(map);

	if ( map->length != size )
		SysLog(LogFatal, "%s should be %d", HashMapToString(map), size);

	int i, j;
	for(i = j = 0; i < map->tblSize; ++i)
	{
		if ( map->entries[i] != NULL )
		{
			++j;		// count the root entry
			for(HashEntry_t *e = map->entries[i]->next; e != NULL; e = e->next)
				++j;		// then the collision list
		}
	}
	if ( j != size )
		SysLog(LogFatal, "%s should be %d", HashMapToString(map), size);
}


void
HashMapTest(int size)
{
	int nbr = size * 1024;
	int i = 0, j = 0;
	HashMap_t *map = HashMapNew(size, "SessionConnectionList");
	HashMapVerify(map);

	for(i = 0; i < nbr; )
	{
		char s[128];
		sprintf(s, "%d", i);
		HashAddString(map, s, StringNewCpy(s));
		HashMapVerifySize(map, ++i);
	}

	// delete the first entry
	HashEntry_t *first = NULL;
	for (i = 0; i < map->tblSize && (first = map->entries[i]) != NULL; ++i)
		;
	if ( first != NULL )
	{
		if ( HashDeleteString(map, first->string) == 0 )		// delete the first entry
			++j;
	}

	// now delete the last
	HashEntry_t *last = NULL;
	for(HashEntry_t *e = map->entries[0]->next; e != NULL; e = e->next)
		last = e;
	if ( last != NULL )
	{
		if ( HashDeleteString(map, last->string) == 0 )
			++j;
	}

	// delete random entries
	for(i = 0; i < nbr; ++i)
	{
		HashMapVerifySize(map, (nbr-1)-j);
		char s[128];
		sprintf(s, "%d", RandomRange(0, nbr));
		if ( HashDeleteString(map, s) == 0 )
			++j;
	}

	// delete remaining; sequentially
	for(HashEntry_t *e; (e = HashGetNextEntry(map, NULL)) != NULL;)
	{
		HashMapVerifySize(map, (nbr-1)-j);
		if ( HashDeleteString(map, e->string) == 0 )
			++j;
	}

	// verify empty
	if ( j != (nbr-1) || map->length != 0 )
		SysLog(LogFatal, "map %s should be empty", HashMapToString(map));
	HashMapDelete(map);
}
