/*****************************************************************************

Filename:   lib/nx/pair.c

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "include/stdapp.h"
#include "include/libnx.h"

#include "include/pair.h"



BtNode_t *PairNodeList = NULL;


Pair_t*
PairConstructor(Pair_t *this, char *file, int lno)
{
	return this;
}


void
PairDestructor(Pair_t *this, char *file, int lno)
{
}


Json_t*
PairSerialize(Pair_t *this)
{
	PairVerify(this);
	Json_t *root = JsonNew(__FUNC__);
	JsonAddNumber(root, "First", (unsigned long)this->first);
	JsonAddNumber(root, "Second", (unsigned long)this->second);
	return root;
}


char*
PairToString(Pair_t *this)
{
	Json_t *root = PairSerialize(this);
	StringArrayStatic(sa, 16, 32);
	String_t *out = StringArrayNext(sa);
	StringSprintf(out, "%s", JsonToString(root));
	JsonDelete(root);
	return out->str;
}
