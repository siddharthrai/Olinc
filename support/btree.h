#ifndef CACHESIM_BTREE_H
#define CACHESIM_BTREE_H

#include "../common/intermod-common.h"

struct cachesim_btnode;

typedef struct cachesim_btnode
{
  ub1*             data;    /* Data */
  ub8              key;     /* Key for search */
  cachesim_btnode *parent;  /* Parent */
  cachesim_btnode *left;    /* Left child */
  cachesim_btnode *right;   /* Right child */
}cachesim_btnode;


void cachesim_add_btnode(cachesim_btnode *head, ub8 key, ub1 *data);
ub1* cachesim_delete_btnode(cachesim_btnode *head, ub8 key);
ub1* cachesim_search_btnode(cachesim_btnode *head, ub8 key);

#endif
