#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bt.h"

#define Version "1.8"
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
 *  Usage:  bpt [order]
 *  where order is an optional argument (integer 3 <= order <= 20)
 *  defined as the maximal number of pointers in any BtNode_t.
 *
 */

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>

#define bool char
#define false 0
#define true 1


// Default BtOrder is 4.
#define DEFAULT_ORDER 4

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

// GLOBALS.

/*
 * The BtOrder determines the maximum
     and minimum number of entries (keys and pointers) in any BtNode_t.  Every BtNode_t has at
 * most BtOrder - 1 keys and at least
     (roughly speaking) half that number. Every leaf has as many pointers to data as keys, and
 * every internal BtNode_t has one more pointer to a subtree than the number of keys. This global variable is initialized to
 * the default value.
 */
int             BtOrder = DEFAULT_ORDER;

/*
 * The BtQueue is used to print the
     tree in level BtOrder, starting from the root printing each entire rank on a separate line,
 * finishing with the leaves.
 */
BtNode_t       *BtQueue = NULL;

/*
 * The user can toggle on and off the "verbose" property, which causes the pointer addresses to be printed out in hexadecimal
 * notation next to their corresponding keys.
 */
static int      VerboseOutput = false;
static int      IterateDepth = 0;


// FUNCTION PROTOTYPES.


// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES

/*
 * Helper function for printing the tree out.  See BtPrintTree.
 */

void
BtEnqueue(BtNode_t * new_node)
{
 BtNode_t       *c;

 if (BtQueue == NULL) {
  BtQueue = new_node;
  BtQueue->next = NULL;
 } else {
  c = BtQueue;
  while (c->next != NULL) {
   c = c->next;
  }
  c->next = new_node;
  new_node->next = NULL;
 }
}


/*
 * Helper function for printing the tree out.  See BtPrintTree.
 */
BtNode_t       *
BtDequeue(void)
{
 BtNode_t       *n = BtQueue;

 BtQueue = BtQueue->next;
 n->next = NULL;
 return n;
}


/*
 * Prints the bottom row of keys of the tree (with their respective pointers, if the VerboseOutput flag is set.
 */
void
BtPrintLeaves(BtNode_t * root, FILE * out)
{
 int             i;
 BtNode_t       *c = root;

 if (root == NULL) {
  fprintf(out, "Empty tree.\n");
  return;
 }
 while (!c->is_leaf)
  c = c->pointers[0];
 while (true) {
  for (i = 0; i < c->num_keys; i++) {
   if (VerboseOutput)
    fprintf(out, "%lx ", (unsigned long) c->pointers[i]);
   fprintf(out, "%ld ", c->keys[i]);
  }
  if (VerboseOutput)
   fprintf(out, "%lx ", (unsigned long) c->pointers[BtOrder - 1]);
  if (c->pointers[BtOrder - 1] != NULL) {
   fprintf(out, " | ");
   c = c->pointers[BtOrder - 1];
  } else
   break;
 }
 fprintf(out, "\n");
}


/*
 * Utility function to give the height of the tree, which length in number of edges of the path from the root to any leaf.
 */
int
BtHeight(BtNode_t * root)
{
 int             h = 0;
 BtNode_t       *c = root;

 while (!c->is_leaf) {
  c = c->pointers[0];
  h++;
 }
 return h;
}


/*
 * Utility function to give the length in edges of the path from any BtNode_t to the root.
 */
int
BtPathToRoot(BtNode_t * root, BtNode_t * child)
{
 int             length = 0;
 BtNode_t       *c = child;

 while (c != root) {
  c = c->parent;
  length++;
 }
 return length;
}


/*
 * Prints the B+ tree in the command line in level (rank) BtOrder, with the keys in each BtNode_t and the '|' symbol to
 * separate nodes. With the VerboseOutput flag set. the values of the pointers corresponding to the keys also appear next to
 * their respective keys, in hexadecimal notation.
 */
void
BtPrintTree(BtNode_t * root, FILE * out)
{

 BtNode_t       *n = NULL;
 int             i = 0;
 int             rank = 0;
 int             new_rank = 0;

 if (root == NULL) {
  fprintf(out, "Empty tree.\n");
  return;
 }
 BtQueue = NULL;
 BtEnqueue(root);
 while (BtQueue != NULL) {
  n = BtDequeue();
  if (n->parent != NULL && n == n->parent->pointers[0]) {
   new_rank = BtPathToRoot(root, n);
   if (new_rank != rank) {
    rank = new_rank;
    fprintf(out, "\n");
   }
  }
  if (VerboseOutput)
   fprintf(out, "(%lx)", (unsigned long) n);
  for (i = 0; i < n->num_keys; i++) {
   if (VerboseOutput)
    fprintf(out, "%lx ", (unsigned long) n->pointers[i]);
   fprintf(out, "%ld ", n->keys[i]);
  }
  if (!n->is_leaf)
   for (i = 0; i <= n->num_keys; i++)
    BtEnqueue(n->pointers[i]);
  if (VerboseOutput) {
   if (n->is_leaf)
    fprintf(out, "%lx ", (unsigned long) n->pointers[BtOrder - 1]);
   else
    fprintf(out, "%lx ", (unsigned long) n->pointers[n->num_keys]);
  }
  fprintf(out, "| ");
 }
 fprintf(out, "\n");
}


/*
 * Traces the path from the root to a leaf, searching by key.  Displays information about the path if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
BtNode_t       *
BtFindLeaf(BtNode_t * root, unsigned long key, int verbose)
{
 int             i = 0;
 BtNode_t       *c = root;

 if (c == NULL) {
  if (verbose)
   printf("Empty tree.\n");
  return c;
 }
 while (!c->is_leaf) {
  if (verbose) {
   printf("[");
   for (i = 0; i < c->num_keys - 1; i++)
    printf("%ld ", c->keys[i]);
   printf("%ld] ", c->keys[i]);
  }
  i = 0;
  while (i < c->num_keys) {
   if (key >= c->keys[i])
    i++;
   else
    break;
  }
  if (verbose)
   printf("%d ->\n", i);
  c = (BtNode_t *) c->pointers[i];
 }
 if (verbose) {
  printf("Leaf [");
  for (i = 0; i < c->num_keys - 1; i++)
   printf("%ld ", c->keys[i]);
  printf("%ld] ->\n", c->keys[i]);
 }
 return c;
}

/*
 * Finds and returns the BtRecord_t to which a key refers.
 */
BtRecord_t     *
BtFind(BtNode_t * root, unsigned long key, int verbose)
{
 int             i = 0;
 BtNode_t       *c = BtFindLeaf(root, key, verbose);

 if (c == NULL)
  return NULL;
 for (i = 0; i < c->num_keys; i++)
  if (c->keys[i] == key)
   break;
 if (i == c->num_keys)
  return NULL;
 else
  return (BtRecord_t *) c->pointers[i];
}

/*
 * Finds the appropriate place to split a BtNode_t that is too big into two.
 */
int
BtCut(int length)
{
 if (length % 2 == 0)
  return length / 2;
 else
  return length / 2 + 1;
}













// INSERTION

/*
 * Creates a new BtRecord_t to hold the value to which a key refers.
 */
BtRecord_t     *
BtMakeRecord(unsigned long value)
{
 BtRecord_t     *new_record = (BtRecord_t *) calloc(1, sizeof(BtRecord_t));

 if (new_record == NULL) {
  perror("Record creation.");
  exit(EXIT_FAILURE);
 } else {
  new_record->value = value;
 }
 return new_record;
}


/*
 * Creates a new general BtNode_t, which can be adapted to serve as either a leaf or an internal BtNode_t.
 */
BtNode_t       *
BtMakeNode(void)
{
 BtNode_t       *new_node;

 new_node = calloc(1, sizeof(BtNode_t));
 if (new_node == NULL) {
  perror("Node creation.");
  exit(EXIT_FAILURE);
 }
 new_node->keys = calloc(1, (BtOrder - 1) * sizeof(new_node->keys[0]));
 if (new_node->keys == NULL) {
  perror("New BtNode keys array.");
  exit(EXIT_FAILURE);
 }
 new_node->pointers = calloc(1, BtOrder * sizeof(void *));
 if (new_node->pointers == NULL) {
  perror("New BtNode pointers array.");
  exit(EXIT_FAILURE);
 }
 new_node->is_leaf = false;
 new_node->num_keys = 0;
 new_node->parent = NULL;
 new_node->next = NULL;
 return new_node;
}

/*
 * Creates a new leaf by creating a BtNode_t and then adapting it appropriately.
 */
BtNode_t       *
BtMakeLeaf(void)
{
 BtNode_t       *leaf = BtMakeNode();

 leaf->is_leaf = true;
 return leaf;
}


/*
 * Helper function used in BtInsertIntoParent to find the index of the parent's pointer to the BtNode_t to the left of the key
 * to be inserted.
 */
int
BtGetLeftIndex(BtNode_t * parent, BtNode_t * left)
{

 int             left_index = 0;

 while (left_index <= parent->num_keys && parent->pointers[left_index] != left)
  left_index++;
 return left_index;
}

/*
 * Inserts a new pointer to a BtRecord_t and its corresponding key into a leaf. Returns the altered leaf.
 */
BtNode_t       *
BtInsertIntoLeaf(BtNode_t * leaf, unsigned long key, BtRecord_t * pointer)
{

 int             i, insertion_point;

 insertion_point = 0;
 while (insertion_point < leaf->num_keys && leaf->keys[insertion_point] < key)
  insertion_point++;

 for (i = leaf->num_keys; i > insertion_point; i--) {
  leaf->keys[i] = leaf->keys[i - 1];
  leaf->pointers[i] = leaf->pointers[i - 1];
 }
 leaf->keys[insertion_point] = key;
 leaf->pointers[insertion_point] = pointer;
 leaf->num_keys++;
 return leaf;
}


/*
 * Inserts a new key and pointer to a new BtRecord_t into a leaf so as to exceed the tree's BtOrder, causing the leaf to be
 * split in half.
 */
BtNode_t       *
BtInsertIntoLeafAfterSplitting(BtNode_t * root, BtNode_t * leaf, unsigned long key, BtRecord_t * pointer)
{

 BtNode_t       *new_leaf;
 unsigned long  *temp_keys;
 void          **temp_pointers;
 int             insertion_index, split, new_key, i, j;

 new_leaf = BtMakeLeaf();

 temp_keys = calloc(1, BtOrder * sizeof(temp_keys[0]));
 if (temp_keys == NULL) {
  perror("Temporary keys array.");
  exit(EXIT_FAILURE);
 }

 temp_pointers = calloc(1, BtOrder * sizeof(void *));
 if (temp_pointers == NULL) {
  perror("Temporary pointers array.");
  exit(EXIT_FAILURE);
 }

 insertion_index = 0;
 while (insertion_index < BtOrder - 1 && leaf->keys[insertion_index] < key)
  insertion_index++;

 for (i = 0, j = 0; i < leaf->num_keys; i++, j++) {
  if (j == insertion_index)
   j++;
  temp_keys[j] = leaf->keys[i];
  temp_pointers[j] = leaf->pointers[i];
 }

 temp_keys[insertion_index] = key;
 temp_pointers[insertion_index] = pointer;

 leaf->num_keys = 0;

 split = BtCut(BtOrder - 1);

 for (i = 0; i < split; i++) {
  leaf->pointers[i] = temp_pointers[i];
  leaf->keys[i] = temp_keys[i];
  leaf->num_keys++;
 }

 for (i = split, j = 0; i < BtOrder; i++, j++) {
  new_leaf->pointers[j] = temp_pointers[i];
  new_leaf->keys[j] = temp_keys[i];
  new_leaf->num_keys++;
 }

 free(temp_pointers);
 free(temp_keys);

 new_leaf->pointers[BtOrder - 1] = leaf->pointers[BtOrder - 1];
 leaf->pointers[BtOrder - 1] = new_leaf;

 for (i = leaf->num_keys; i < BtOrder - 1; i++)
  leaf->pointers[i] = NULL;
 for (i = new_leaf->num_keys; i < BtOrder - 1; i++)
  new_leaf->pointers[i] = NULL;

 new_leaf->parent = leaf->parent;
 new_key = new_leaf->keys[0];

 return BtInsertIntoParent(root, leaf, new_key, new_leaf);
}


/*
 * Inserts a new key and pointer to a BtNode_t into a BtNode_t into which these can fit without violating the B+ tree
 * properties.
 */
BtNode_t       *
BtInsertIntoNode(BtNode_t * root, BtNode_t * n, int left_index, unsigned long key, BtNode_t * right)
{
 int             i;

 for (i = n->num_keys; i > left_index; i--) {
  n->pointers[i + 1] = n->pointers[i];
  n->keys[i] = n->keys[i - 1];
 }
 n->pointers[left_index + 1] = right;
 n->keys[left_index] = key;
 n->num_keys++;
 return root;
}


/*
 * Inserts a new key and pointer to a BtNode_t into a BtNode_t, causing the BtNode_t's size to exceed the BtOrder, and causing
 * the BtNode_t to split into two.
 */
BtNode_t       *
BtInsertIntoNodeAfterSplitting(BtNode_t * root, BtNode_t * old_node, int left_index, unsigned long key, BtNode_t * right)
{

 int             i, j, split, k_prime;
 BtNode_t       *new_node, *child;
 unsigned long  *temp_keys;
 BtNode_t      **temp_pointers;

 /*
  * First create a temporary set of keys and pointers to hold everything in BtOrder, including the new key and pointer,
  * inserted in their correct places. Then create a new BtNode_t and copy half of the keys and pointers to the old
  * BtNode_t and the other half to the new.
  */

 temp_pointers = calloc(1, (BtOrder + 1) * sizeof(BtNode_t *));
 if (temp_pointers == NULL) {
  perror("Temporary pointers array for splitting nodes.");
  exit(EXIT_FAILURE);
 }
 temp_keys = calloc(1, BtOrder * sizeof(temp_keys[0]));
 if (temp_keys == NULL) {
  perror("Temporary keys array for splitting nodes.");
  exit(EXIT_FAILURE);
 }

 for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++) {
  if (j == left_index + 1)
   j++;
  temp_pointers[j] = old_node->pointers[i];
 }

 for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
  if (j == left_index)
   j++;
  temp_keys[j] = old_node->keys[i];
 }

 temp_pointers[left_index + 1] = right;
 temp_keys[left_index] = key;

 /*
  * Create the new BtNode_t and copy half the keys and pointers to the old and half to the new.
  */
 split = BtCut(BtOrder);
 new_node = BtMakeNode();
 old_node->num_keys = 0;
 for (i = 0; i < split - 1; i++) {
  old_node->pointers[i] = temp_pointers[i];
  old_node->keys[i] = temp_keys[i];
  old_node->num_keys++;
 }
 old_node->pointers[i] = temp_pointers[i];
 k_prime = temp_keys[split - 1];
 for (++i, j = 0; i < BtOrder; i++, j++) {
  new_node->pointers[j] = temp_pointers[i];
  new_node->keys[j] = temp_keys[i];
  new_node->num_keys++;
 }
 new_node->pointers[j] = temp_pointers[i];
 free(temp_pointers);
 free(temp_keys);
 new_node->parent = old_node->parent;
 for (i = 0; i <= new_node->num_keys; i++) {
  child = new_node->pointers[i];
  child->parent = new_node;
 }

 /*
  * Insert a new key into the parent of the two nodes resulting from the split, with the old BtNode_t to the left and the
  * new to the right.
  */

 return BtInsertIntoParent(root, old_node, k_prime, new_node);
}



/*
 * Inserts a new BtNode_t (leaf or internal BtNode_t) into the B+ tree. Returns the root of the tree after insertion.
 */
BtNode_t       *
BtInsertIntoParent(BtNode_t * root, BtNode_t * left, unsigned long key, BtNode_t * right)
{

 int             left_index;
 BtNode_t       *parent;

 parent = left->parent;

 /*
  * Case: new root.
  */

 if (parent == NULL)
  return BtInsertIntoNewRoot(left, key, right);

 /*
  * Case: leaf or BtNode_t. (Remainder of function body.)
  */

 /*
  * Find the parent's pointer to the left BtNode_t.
  */

 left_index = BtGetLeftIndex(parent, left);


 /*
  * Simple case: the new key fits into the BtNode_t.
  */

 if (parent->num_keys < BtOrder - 1)
  return BtInsertIntoNode(root, parent, left_index, key, right);

 /*
  * Harder case: split a BtNode_t in BtOrder to preserve the B+ tree properties.
  */

 return BtInsertIntoNodeAfterSplitting(root, parent, left_index, key, right);
}


/*
 * Creates a new root for two subtrees and inserts the appropriate key into the new root.
 */
BtNode_t       *
BtInsertIntoNewRoot(BtNode_t * left, unsigned long key, BtNode_t * right)
{

 BtNode_t       *root = BtMakeNode();

 root->keys[0] = key;
 root->pointers[0] = left;
 root->pointers[1] = right;
 root->num_keys++;
 root->parent = NULL;
 left->parent = root;
 right->parent = root;
 return root;
}



/*
 * First insertion: start a new tree.
 */
BtNode_t       *
BtStartNewTree(unsigned long key, BtRecord_t * pointer)
{

 BtNode_t       *root = BtMakeLeaf();

 root->keys[0] = key;
 root->pointers[0] = pointer;
 root->pointers[BtOrder - 1] = NULL;
 root->parent = NULL;
 root->num_keys++;
 return root;
}



/*
 * Master insertion function. Inserts a key and an associated value into the B+ tree, causing the tree to be adjusted however
 * necessary to maintain the B+ tree properties.
 */
BtNode_t       *
BtInsert(BtNode_t * root, unsigned long key, unsigned long value)
{

 if (IterateDepth > 0)
  return root;   // walking the tree; don't insert...

 BtRecord_t     *pointer;
 BtNode_t       *leaf;

 /*
  * The current implementation ignores duplicates.
  */

 if (BtFind(root, key, false) != NULL)
  return root;

 /*
  * Create a new BtRecord_t for the value.
  */
 pointer = BtMakeRecord(value);


 /*
  * Case: the tree does not exist yet. Start a new tree.
  */

 if (root == NULL)
  return BtStartNewTree(key, pointer);


 /*
  * Case: the tree already exists. (Rest of function body.)
  */

 leaf = BtFindLeaf(root, key, false);

 /*
  * Case: leaf has room for key and pointer.
  */

 if (leaf->num_keys < BtOrder - 1) {
  leaf = BtInsertIntoLeaf(leaf, key, pointer);
  return root;
 }


 /*
  * Case: leaf must be split.
  */

 return BtInsertIntoLeafAfterSplitting(root, leaf, key, pointer);
}




// DELETION.

/*
 * Utility function for deletion.  Retrieves the index of a BtNode_t's nearest neighbor (sibling) to the left if one exists.
 * If not (the BtNode_t is the leftmost child), returns -1 to signify this special case.
 */
int
BtGetNeighborIndex(BtNode_t * n)
{

 int             i;

 /*
  * Return the index of the key to the left of the pointer in the parent pointing to n. If n is the leftmost child, this
  * means return -1.
  */
 for (i = 0; i <= n->parent->num_keys; i++)
  if (n->parent->pointers[i] == n)
   return i - 1;

 // Error state.
 printf("Search for nonexistent pointer to BtNode in parent.\n");
 printf("Node:  %#lx\n", (unsigned long) n);
 exit(EXIT_FAILURE);
 return -1;
}


BtNode_t       *
BtRemoveEntryFronNode(BtNode_t * n, unsigned long key, BtNode_t * pointer)
{

 int             i, num_pointers;

 // Remove the key and shift other keys accordingly.
 i = 0;
 while (n->keys[i] != key)
  i++;
 for (++i; i < n->num_keys; i++)
  n->keys[i - 1] = n->keys[i];

 // Remove the pointer and shift other pointers accordingly.
 // First determine number of pointers.
 num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
 i = 0;
 while (n->pointers[i] != pointer)
  i++;
 for (++i; i < num_pointers; i++)
  n->pointers[i - 1] = n->pointers[i];


 // One key fewer.
 n->num_keys--;

 // Set the other pointers to NULL for tidiness.
 // A leaf uses the last pointer to point to the next leaf.
 if (n->is_leaf)
  for (i = n->num_keys; i < BtOrder - 1; i++)
   n->pointers[i] = NULL;
 else
  for (i = n->num_keys + 1; i < BtOrder; i++)
   n->pointers[i] = NULL;

 return n;
}


BtNode_t       *
BtAdjustRoot(BtNode_t * root)
{

 BtNode_t       *new_root;

 /*
  * Case: nonempty root. Key and pointer have already been deleted, so nothing to be done.
  */

 if (root->num_keys > 0)
  return root;

 /*
  * Case: empty root.
  */

 // If it has a child, promote
 // the first (only) child
 // as the new root.

 if (!root->is_leaf) {
  new_root = root->pointers[0];
  new_root->parent = NULL;
 }
 // If it is a leaf (has no children),
 // then the whole tree is empty.

 else
  new_root = NULL;

 free(root->keys);
 free(root->pointers);
 free(root);

 return new_root;
}


/*
 * Coalesces a BtNode_t that has become too small after deletion with a neighboring BtNode_t that can accept the additional
 * entries without exceeding the maximum.
 */
BtNode_t       *
BtCoalesceNodes(BtNode_t * root, BtNode_t * n, BtNode_t * neighbor, int neighbor_index, int k_prime)
{

 int             i, j, neighbor_insertion_index, n_start, n_end, new_k_prime;
 BtNode_t       *tmp;
 int             split;

 /*
  * Swap neighbor with BtNode_t if BtNode_t is on the extreme left and neighbor is to its right.
  */

 if (neighbor_index == -1) {
  tmp = n;
  n = neighbor;
  neighbor = tmp;
 }

 /*
  * Starting point in the neighbor for copying keys and pointers from n. Recall that n and neighbor have swapped places in
  * the special case of n being a leftmost child.
  */

 neighbor_insertion_index = neighbor->num_keys;

 /*
  * Nonleaf nodes may sometimes need to remain split,
  * if the insertion of k_prime would cause the resulting
  * single coalesced BtNode_t to exceed the limit BtOrder - 1.
  * The variable split is always false for leaf nodes
  * and only sometimes set to true for nonleaf nodes.
  */

 split = false;

 /*
  * Case: nonleaf BtNode_t. Append k_prime and the following pointer. If there is room in the neighbor, append all pointers
  * and keys from the neighbor. Otherwise, append only cut(BtOrder) - 2 keys and cut(BtOrder) - 1 pointers.
  */

 if (!n->is_leaf) {

  /*
   * Append k_prime.
   */

  neighbor->keys[neighbor_insertion_index] = k_prime;
  neighbor->num_keys++;


  /*
   * Case (default): there is room for all of n's keys and pointers in the neighbor after appending k_prime.
   */

  n_end = n->num_keys;

  /*
   * Case (special): k cannot fit with all the other keys and pointers into one coalesced BtNode_t.
   */
  n_start = 0;   // Only used in this special case.
  if (n->num_keys + neighbor->num_keys >= BtOrder) {
   split = true;
   n_end = BtCut(BtOrder) - 2;
  }

  for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
   neighbor->keys[i] = n->keys[j];
   neighbor->pointers[i] = n->pointers[j];
   neighbor->num_keys++;
   n->num_keys--;
   n_start++;
  }

  /*
   * The number of pointers is always one more than the number of keys.
   */

  neighbor->pointers[i] = n->pointers[j];

  /*
   * If the nodes are still split, remove the first key from n.
   */
  if (split) {
   new_k_prime = n->keys[n_start];
   for (i = 0, j = n_start + 1; i < n->num_keys; i++, j++) {
    n->keys[i] = n->keys[j];
    n->pointers[i] = n->pointers[j];
   }
   n->pointers[i] = n->pointers[j];
   n->num_keys--;
  }

  /*
   * All children must now point up to the same parent.
   */

  for (i = 0; i < neighbor->num_keys + 1; i++) {
   tmp = (BtNode_t *) neighbor->pointers[i];
   tmp->parent = neighbor;
  }
 }

 /*
  * In a leaf, append the keys and pointers of n to the neighbor. Set the neighbor's last pointer to point to what had been
  * n's right neighbor.
  */

 else {
  for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
   neighbor->keys[i] = n->keys[j];
   neighbor->pointers[i] = n->pointers[j];
   neighbor->num_keys++;
  }
  neighbor->pointers[BtOrder - 1] = n->pointers[BtOrder - 1];
 }

 if (!split) {
  root = BtDeleteEntry(root, n->parent, k_prime, n);
  free(n->keys);
  free(n->pointers);
  free(n);
 } else
  for (i = 0; i < n->parent->num_keys; i++)
   if (n->parent->pointers[i + 1] == n) {
    n->parent->keys[i] = new_k_prime;
    break;
   }

 return root;

}


/*
 * Redistributes entries between two nodes when one has become too small after deletion but its neighbor is too big to append
 * the small BtNode_t's entries without exceeding the maximum
 */
BtNode_t       *
BtRedistributeNodes(BtNode_t * root, BtNode_t * n, BtNode_t * neighbor, int neighbor_index, int k_prime_index, int k_prime)
{

 int             i;
 BtNode_t       *tmp;

 /*
  * Case: n has a neighbor to the left. Pull the neighbor's last key-pointer pair over from the neighbor's right end to n's
  * left end.
  */

 if (neighbor_index != -1) {
  if (!n->is_leaf)
   n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
  for (i = n->num_keys; i > 0; i--) {
   n->keys[i] = n->keys[i - 1];
   n->pointers[i] = n->pointers[i - 1];
  }
  if (!n->is_leaf) {
   n->pointers[0] = neighbor->pointers[neighbor->num_keys];
   tmp = (BtNode_t *) n->pointers[0];
   tmp->parent = n;
   neighbor->pointers[neighbor->num_keys] = NULL;
   n->keys[0] = k_prime;
   n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
  } else {
   n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
   neighbor->pointers[neighbor->num_keys - 1] = NULL;
   n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
   n->parent->keys[k_prime_index] = n->keys[0];
  }
 }

 /*
  * Case: n is the leftmost child. Take a key-pointer pair from the neighbor to the right. Move the neighbor's leftmost
  * key-pointer pair to n's rightmost position.
  */

 else {
  if (n->is_leaf) {
   n->keys[n->num_keys] = neighbor->keys[0];
   n->pointers[n->num_keys] = neighbor->pointers[0];
   n->parent->keys[k_prime_index] = neighbor->keys[1];
  } else {
   n->keys[n->num_keys] = k_prime;
   n->pointers[n->num_keys + 1] = neighbor->pointers[0];
   tmp = (BtNode_t *) n->pointers[n->num_keys + 1];
   tmp->parent = n;
   n->parent->keys[k_prime_index] = neighbor->keys[0];
  }
  for (i = 0; i < neighbor->num_keys; i++) {
   neighbor->keys[i] = neighbor->keys[i + 1];
   neighbor->pointers[i] = neighbor->pointers[i + 1];
  }
  if (!n->is_leaf)
   neighbor->pointers[i] = neighbor->pointers[i + 1];
 }

 /*
  * n now has one more key and one more pointer; the neighbor has one fewer of each.
  */

 n->num_keys++;
 neighbor->num_keys--;

 return root;
}


/*
 * Deletes an entry from the B+ tree. Removes the BtRecord_t and its key and pointer from the leaf, and then makes all
 * appropriate changes to preserve the B+ tree properties.
 */
BtNode_t       *
BtDeleteEntry(BtNode_t * root, BtNode_t * n, unsigned long key, void *pointer)
{
 int             min_keys;
 BtNode_t       *neighbor;
 int             neighbor_index;
 int             k_prime_index, k_prime;
 int             capacity;

 // Remove key and pointer from BtNode_t.

 n = BtRemoveEntryFronNode(n, key, pointer);

 /*
  * Case: deletion from the root.
  */

 if (n == root)
  return BtAdjustRoot(root);


 /*
  * Case: deletion from a BtNode_t below the root. (Rest of function body.)
  */

 /*
  * Determine minimum allowable size of BtNode_t, to be preserved after deletion.
  */

 min_keys = n->is_leaf ? BtCut(BtOrder - 1) : BtCut(BtOrder) - 1;

 /*
  * Case: BtNode_t stays at or above minimum. (The simple case.)
  */

 if (n->num_keys >= min_keys)
  return root;

 /*
  * Case: BtNode_t falls below minimum. Either coalescence or redistribution is needed.
  */

 /*
  * Find the appropriate neighbor BtNode_t with which to coalesce. Also find the key (k_prime) in the parent between the
  * pointer to BtNode_t n and the pointer to the neighbor.
  */

 neighbor_index = BtGetNeighborIndex(n);
 k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
 k_prime = n->parent->keys[k_prime_index];
 neighbor = neighbor_index == -1 ? n->parent->pointers[1] : n->parent->pointers[neighbor_index];

 capacity = n->is_leaf ? BtOrder : BtOrder - 1;

 /*
  * Coalescence.
  */

 if (neighbor->num_keys + n->num_keys < capacity)
  return BtCoalesceNodes(root, n, neighbor, neighbor_index, k_prime);

 /*
  * Redistribution.
  */

 else
  return BtRedistributeNodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}



/*
 * Master deletion function.
 */
BtNode_t       *
BtDelete(BtNode_t * root, unsigned long key)
{

 if (IterateDepth > 0)
  return root;   // walking the tree; don't delete...

 BtNode_t       *key_leaf;
 BtRecord_t     *key_record;

 key_record = BtFind(root, key, false);
 key_leaf = BtFindLeaf(root, key, false);
 if (key_record != NULL && key_leaf != NULL) {
  root = BtDeleteEntry(root, key_leaf, key, key_record);
  free(key_record);
 }
 return root;
}


void
BtDestoryTreeNodes(BtNode_t * root)
{
 int             i;

 if (root->is_leaf)
  for (i = 0; i < root->num_keys; i++)
   free(root->pointers[i]);
 else
  for (i = 0; i < root->num_keys + 1; i++)
   BtDestoryTreeNodes(root->pointers[i]);
 free(root->pointers);
 free(root->keys);
 free(root);
}


BtNode_t       *
BtDestoryTree(BtNode_t * root)
{
 BtDestoryTreeNodes(root);
 return NULL;
}


int
BtNodeIterate(BtNode_t * node, BtNodeIterater_t iterater, void *arg)
{
 int             ret = 0;

 if (node == NULL)
  return 0;

 ++IterateDepth;

 if (node->is_leaf) {
  for (int i = 0; i < node->num_keys; ++i) {
   int             k = (*iterater) (node->keys[i], node->pointers[i], arg);

   if (k < 0)
    break;   // abort
   ret += k;
  }
 } else {
  for (int i = 0; i <= node->num_keys; ++i) {
   int             k = BtNodeIterate(node->pointers[i], iterater, arg);

   if (k < 0)
    break;   // abort
   ret += k;
  }
 }

 --IterateDepth;

 return ret;
}





#if 0
/*
 * Copyright and license notice to user at startup.
 */
static void
license_notice(void)
{
 printf("bpt version %s -- Copyright (C) 2010  Amittai Aviram " "http://www.amittai.com\n", Version);
 printf("This program comes with ABSOLUTELY NO WARRANTY; for details "
     "type `show w'.\n" "This is free software, and you are welcome to redistribute it\n"
     "under certain conditions; type `show c' for details.\n\n");
}
#endif


/*
 * Routine to print portion of GPL license to stdout.
 */
static void
print_license(int license_part)
{
 int             start, end, line;
 FILE           *fp;
 char            buffer[0x100];

 switch (license_part) {
 case LICENSE_WARRANTEE:
  start = LICENSE_WARRANTEE_START;
  end = LICENSE_WARRANTEE_END;
  break;
 case LICENSE_CONDITIONS:
  start = LICENSE_CONDITIONS_START;
  end = LICENSE_CONDITIONS_END;
  break;
 default:
  return;
 }

 fp = fopen(LICENSE_FILE, "r");
 if (fp == NULL) {
  perror("print_license: fopen");
  exit(EXIT_FAILURE);
 }
 for (line = 0; line < start; line++)
  fgets(buffer, sizeof(buffer), fp);
 for (; line < end; line++) {
  fgets(buffer, sizeof(buffer), fp);
  printf("%s", buffer);
 }
 fclose(fp);
}


/*
 * Second message to the user.
 */
static void
usage_2(void)
{
 printf("Enter any of the following commands after the prompt > :\n");
 printf("\ti <k>  -- Insert <k> (an integer) as both key and value).\n");
 printf("\tf <k>  -- Find the value under key <k>.\n");
 printf("\tp <k> -- Print the path from the root to key k and its associated value.\n");
 printf("\td <k>  -- Delete key <k> and its associated value.\n");
 printf("\tx -- Destroy the whole tree.  Start again with an empty tree of the same order.\n");
 printf("\tt -- Print the B+ tree.\n");
 printf("\tl -- Print the keys of the leaves (bottom row of the tree).\n");
 printf("\tv -- Toggle output of pointer addresses (\"verbose\") in tree and leaves.\n");
 printf("\tq -- Quit. (Or use Ctl-D.)\n");
 printf("\t? -- Print this help message.\n");
}


void
BtMain(BtNode_t * root)
{
 BtRecord_t     *r;
 int             input;
 char            instruction;
 char            license_part;

 printf("> ");
 while (scanf("%c", &instruction) != EOF) {
  switch (instruction) {
  case 'd':
   scanf("%d", &input);
   // while (getchar() != (int)'\n');
   root = BtDelete(root, input);
   BtPrintTree(root, stdout);
   break;
  case 'i':
   scanf("%d", &input);
   // while (getchar() != (int)'\n');
   root = BtInsert(root, input, input);
   BtPrintTree(root, stdout);
   break;
  case 'f':
  case 'p':
   scanf("%d", &input);
   // while (getchar() != (int)'\n');
   r = BtFind(root, input, instruction == 'p');
   if (r == NULL)
    printf("Record not found under key %d.\n", input);
   else
    printf("Record at %lx -- key %d, value %ld.\n", (unsigned long) r, input, r->value);
   break;
  case 'l':
   // while (getchar() != (int)'\n');
   BtPrintLeaves(root, stdout);
   break;
  case 'q':
   while (getchar() != (int) '\n');
   return;
  case 's':
   if (scanf("how %c", &license_part) == 0) {
    usage_2();
    break;
   }
   switch (license_part) {
   case 'w':
    print_license(LICENSE_WARRANTEE);
    break;
   case 'c':
    print_license(LICENSE_CONDITIONS);
    break;
   default:
    usage_2();
    break;
   }
   break;
  case 't':
   // while (getchar() != (int)'\n');
   BtPrintTree(root, stdout);
   // BtNodeIterate(root, NodePrinter, NULL);
   break;
  case 'v':
   // while (getchar() != (int)'\n');
   VerboseOutput = !VerboseOutput;
   break;
  case 'x':
   // while (getchar() != (int)'\n');
   root = BtDestoryTree(root);
   BtPrintTree(root, stdout);
   break;
  default:
   usage_2();
   break;
  }
  while (getchar() != (int) '\n');
  printf("> ");
 }
}


#if 0
// MAIN

static int
NodePrinter(unsigned long key, BtRecord_t * rec, void *arg)
{
 printf("%d=%d\n", (int) key, (int) rec->value);
 return 0;
}


int
main(int argc, char **argv)
{
 char           *input_file;
 FILE           *fp;
 int             input;

 BtNode_t       *root = NULL;

 VerboseOutput = false;

 if (argc > 1) {
  BtOrder = atoi(argv[1]);
  if (BtOrder < 3 || BtOrder > 20) {
   fprintf(stderr, "BtOrder of %d is invalid; using %d\n", BtOrder, DEFAULT_ORDER);
   BtOrder = DEFAULT_ORDER;
  } else {
   --argc;
   ++argv;
  }
 }
 // usage_2();

 if (argc > 1) {
  input_file = argv[1];
  fp = fopen(input_file, "r");
  if (fp == NULL) {
   perror("Failure to open input file.");
   exit(EXIT_FAILURE);
  }
  while (!feof(fp)) {
   fscanf(fp, "%d\n", &input);
   root = BtInsert(root, input, input);
  }
  fclose(fp);
  BtPrintTree(root, stdout);
 }

 BtMain(root);
 printf("\n");

 return EXIT_SUCCESS;
}

#endif
