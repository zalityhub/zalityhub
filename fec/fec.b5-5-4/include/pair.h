/*****************************************************************************

Filename:   include/pair.h

Purpose:    This space for rent.

Maintenance:
------------------------------------------------------------------------------
History: (top down change order, newest to oldest changes)
YYYY.MM.DD --- developer ---    ----------------- Comments -------------------
2009.07.15 harold bray          Created release 4.0
 *****************************************************************************/


#ifndef _PAIR_H
#define _PAIR_H

typedef struct Pair_t
{
	void		*first;
	void		*second;
} Pair_t ;


#define PairNew() ObjectNew(Pair)
#define PairVerify(var) ObjectVerify(Pair, var)
#define PairDelete(var) ObjectDelete(Pair, var)


// External Functions
extern Pair_t* PairConstructor(Pair_t *this, char *file, int lno);
extern void PairDestructor(Pair_t *this, char *file, int lno);
extern BtNode_t* PairNodeList;
extern struct Json_t* PairSerialize(Pair_t *this);
extern char* PairToString(Pair_t *this);

#endif
