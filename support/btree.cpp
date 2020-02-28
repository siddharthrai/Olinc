#include "btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

static ub8 add_accesses = 0;
static ub8 search_accesses = 0;

void cachesim_add_btnode(cachesim_btnode *head, ub8 key, ub1 *data)
{
  cachesim_btnode *node;
  cachesim_btnode *new_node;
  
  struct timeval st;
  struct timeval et;
  int    depth;

  add_accesses++;

  node  = head;
  depth =  0;

  new_node = (cachesim_btnode *)malloc(sizeof(cachesim_btnode));
  assert(new_node);

  new_node->data  = data;
  new_node->key   = key;
  new_node->left  = NULL;
  new_node->right = NULL;

  gettimeofday(&st, NULL);

  /* find the parent */
  while(node)
  {
    /* If key is less than current node's key it will go to left else right */
    if (key < node->key)
    {
      depth++;

      if (node->left)
      {
        node = node->left;
      }
      else
      {
        new_node->parent  = node;
        node->left        = new_node;
        break;
      }
    }
    else
    {
      if (key > node->key)
      {
        depth++;

        if (node->right)
        {
          node = node->right;
        }
        else
        {
          new_node->parent  = node;
          node->right       = new_node;
          break;
        }
      }
      else
      {
        perror("Key already present\n");
        exit(-1);
      }
    }
  }

  gettimeofday(&et, NULL);

  ub8 add_time =  (et.tv_sec - st.tv_sec) * 1000000 + (et.tv_usec - st.tv_usec);

  if (add_accesses % 100000 == 0 && add_time > 50)
  {
    printf("%lld access took %lld usec at depth %d\n", add_accesses, add_time, depth);
  }

  assert(node);
}

ub1* cachesim_delete_btnode(cachesim_btnode *head, ub8 key)
{
  cachesim_btnode *node;
  ub1             *data;

  node = head;

  while (node)
  {
    if (key < node->key)
    {
      node = node->left; 
    }
    else
    {
      if (key > node->key)
      {
        node = node->right;
      }
      else
      {
        /* Delete the node. If node has only one child make replace it for 
         * the current node else replace current node with the right child */
        if ((node->right && !node->left) || (!node->right && node->left))
        {
          if (node->right) 
          {
            if (node->parent->right == node)
            {
              node->parent->right = node->right;
            }
            else
            {
              assert(node->parent->left == node);
              node->parent->left = node->right;
            }

            break;
          }
          else
          {
            if (node->left)
            {
              assert(node->right == NULL);

              if (node->parent->right == node)
              {
                node->parent->right = node->left;
              }
              else
              {
                assert(node->parent->left == node);
                node->parent->left = node->left;
              }

              break;
            }
            else
            {
              perror("Node can't have both left and right child absent!\n");
              exit (-1);
            }
          }
        }
        else
        {
          if (!node->left && !node->right)
          {
            if (node->parent->right == node) 
            {
              node->parent->right = NULL;
              break;
            }
            else
            {
              assert(node->parent->left == node); 

              node->parent->left = NULL;
              break;
            }
          }
          else
          {
            assert(node->right && node->left);

            cachesim_btnode *leftmost_child;
            leftmost_child = node;
            
            /* Fix left child */
            while (leftmost_child->left)
            {
              leftmost_child = leftmost_child->left;
            }

            leftmost_child->left = node->left;
            node->left->parent   = leftmost_child;
            
            /* Fix right child */
            if (node->parent->right == node)
            {
              node->parent->right = node->right;
            }
            else
            {
              assert(node->parent->left == node);
              node->parent->left = node->right;
            }

            node->right->parent = node->parent;
            break;
          }
        }
      }
    }
  }
  
  /* Node must be in the tree */
  assert(node);
  
  /* Store data to be returned */
  data = node->data;
  delete node;

  return data;
}

ub1* cachesim_search_btnode(cachesim_btnode *head, ub8 key)
{
  cachesim_btnode *node;
  struct timeval   st;
  struct timeval   et;
  int    depth;

  search_accesses++;

  depth = 0;
  node  = head;

  gettimeofday(&st, NULL);

  while (node)
  {
    if (key < node->key)
    {
      node = node->left;
      depth++;
    }
    else
    {
      if (key > node->key)
      {
        node = node->right;
        depth++;
      }
      else
      {
        assert(key == node->key);
        break;
      }
    }
  }
  
  gettimeofday(&et, NULL);

  ub8 search_time =  (et.tv_sec - st.tv_sec) * 1000000 + (et.tv_usec - st.tv_usec);

  if (search_accesses % 100000 == 0 && search_time > 50)
  {
    printf("%lld search took %lld usec at depth %d\n", search_accesses, 
      search_time, depth);
  }

  return (node) ? node->data : NULL;
}
