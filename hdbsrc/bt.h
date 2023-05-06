#ifndef _BT_H
#define _BT_H

/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010  Amittai Aviram  http://www.amittai.com
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  Author:  Amittai Aviram
 *    http://www.amittai.com
 *    amittai.aviram@yale.edu or afa13@columbia.edu
 *    Department of Computer Science
 *    Yale University
 *    P. O. Box 208285
 *    New Haven, CT 06520-8285
 *  Date:  26 June 2010
 *  Last modified: 28 February 2011
 *
 *  This implementation demonstrates the B+ tree data structure
 *  for educational purposes, includin insertion, deletion, search, and display
 *  of the search path, the leaves, or the whole tree.
 *
 *  Must be compiled with a C99-compliant C compiler such as the latest GCC.
 *
 */


// Data Structures

/*
 * Type representing the BtRecord_t
     to which a given key refers. In a real B+ tree system, the BtRecord_t would hold data (in a
 * database) or a file (in an operating system) or some other information. Users can rewrite this part of the code to change
 * the type and content of the value field.
 */
typedef struct BtRecord_t
{
 unsigned long   value;
} BtRecord_t;

/*
 * Type representing a BtNode_t in the B+ tree. This type is general enough to serve for both the leaf and the internal
 * BtNode_t. The heart of the BtNode_t is the array of keys and the array of corresponding pointers.  The relation between keys
 * and pointers differs between leaves and internal BtNodes. In a leaf, the index of each key equals the index of its
 * corresponding pointer, with a maximum of order - 1 key-pointer pairs.  The last pointer points to the leaf to the right (or
 * NULL in the case of the rightmost leaf). In an internal BtNode_t, the first pointer refers to lower BtNodes with keys less
 * than the smallest key in the keys
     array.  Then, with indices i starting at 0, the pointer at i + 1 points to the subtree with
 * keys greater than or equal to
     the key in this BtNode_t at index i. The num_keys field is used to keep track of the number of
 * valid keys. In an internal BtNode_t, the number of valid pointers is always num_keys + 1. In a leaf, the number of valid
 * pointers to data is always num_keys.  The last leaf pointer points to the next leaf.
 */
typedef struct BtNode_t
{
 void          **pointers;
 unsigned long  *keys;
 struct BtNode_t *parent;
 int             is_leaf;
 int             num_keys;
 struct BtNode_t *next;  // Used for queue.
} BtNode_t;





// External Functions
// Output and utility.

extern void     BtEnqueue(BtNode_t * new_node);
extern BtNode_t *BtDequeue(void);
extern void     BtPrintLeaves(BtNode_t * root, FILE * out);
extern int      BtHeight(BtNode_t * root);
extern int      BtPathToRoot(BtNode_t * root, BtNode_t * child);
extern void     BtPrintTree(BtNode_t * root, FILE * out);
extern BtNode_t *BtFindLeaf(BtNode_t * root, unsigned long key, int verbose);
extern BtRecord_t *BtFind(BtNode_t * root, unsigned long key, int verbose);
extern int      BtCut(int length);
extern BtRecord_t *BtMakeRecord(unsigned long value);
extern BtNode_t *BtMakeNode(void);
extern BtNode_t *BtMakeLeaf(void);
extern int      BtGetLeftIndex(BtNode_t * parent, BtNode_t * left);
extern BtNode_t *BtInsertIntoLeaf(BtNode_t * leaf, unsigned long key, BtRecord_t * pointer);
extern BtNode_t *BtInsertIntoLeafAfterSplitting(BtNode_t * root, BtNode_t * leaf, unsigned long key, BtRecord_t * pointer);
extern BtNode_t *BtInsertIntoNode(BtNode_t * root, BtNode_t * n, int left_index, unsigned long key, BtNode_t * right);
extern BtNode_t *BtInsertIntoNodeAfterSplitting(BtNode_t * root, BtNode_t * old_node, int left_index, unsigned long key,
            BtNode_t * right);
extern BtNode_t *BtInsertIntoParent(BtNode_t * root, BtNode_t * left, unsigned long key, BtNode_t * right);
extern BtNode_t *BtInsertIntoNewRoot(BtNode_t * left, unsigned long key, BtNode_t * right);
extern BtNode_t *BtStartNewTree(unsigned long key, BtRecord_t * pointer);
extern BtNode_t *BtInsert(BtNode_t * root, unsigned long key, unsigned long value);
extern int      BtGetNeighborIndex(BtNode_t * n);
extern BtNode_t *BtRemoveEntryFronNode(BtNode_t * n, unsigned long key, BtNode_t * pointer);
extern BtNode_t *BtAdjustRoot(BtNode_t * root);
extern BtNode_t *BtCoalesceNodes(BtNode_t * root, BtNode_t * n, BtNode_t * neighbor, int neighbor_index, int k_prime);
extern BtNode_t *BtRedistributeNodes(BtNode_t * root, BtNode_t * n, BtNode_t * neighbor, int neighbor_index, int k_prime_index,
          int k_prime);
extern BtNode_t *BtDeleteEntry(BtNode_t * root, BtNode_t * n, unsigned long key, void *pointer);
extern BtNode_t *BtDelete(BtNode_t * root, unsigned long key);
extern void     BtDestoryTreeNodes(BtNode_t * root);
extern BtNode_t *BtDestoryTree(BtNode_t * root);
extern void     BtMain(BtNode_t * root);

typedef int     (*BtNodeIterater_t)(unsigned long key, BtRecord_t * rec, void *arg);
extern int      BtNodeIterate(BtNode_t * node, BtNodeIterater_t iterater, void *arg);

#endif
