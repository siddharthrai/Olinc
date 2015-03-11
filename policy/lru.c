/*
 *  Copyright (C) 2011  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <assert.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "lru.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))

#define CACHE_UPDATE_BLOCK_STATE(block, tag, state)           \
do                                                            \
{                                                             \
  (block)->tag   = (tag);                                     \
  (block)->state = (state);                                   \
}while(0)

#define CACHE_GET_BLOCK_STREAM(block, strm)                   \
do                                                            \
{                                                             \
  strm = (block)->stream;                                     \
}while(0)

#define CACHE_UPDATE_BLOCK_STREAM(block, strm)                \
do                                                            \
{                                                             \
  (block)->stream = strm;                                     \
}while(0)

/*
 * Public Variables
 */

/*
 * Private Functions
 */
#define free_list_remove_block(set, blk)                                                \
do                                                                                      \
{                                                                                       \
        /* Check: free list must be non empty as it contains the current block. */      \
        assert(LRU_DATA_FREE_HEAD(set) && LRU_DATA_FREE_TAIL(set));                     \
                                                                                        \
        /* Check: current block must be in invalid state */                             \
        assert((blk)->state == cache_block_invalid);                                    \
                                                                                        \
        CACHE_REMOVE_FROM_SQUEUE(blk, LRU_DATA_FREE_HEAD(set), LRU_DATA_FREE_TAIL(set));\
                                                                                        \
        (blk)->next = NULL;                                                             \
        (blk)->prev = NULL;                                                             \
                                                                                        \
        /* Reset block state */                                                         \
        (blk)->busy = 0;                                                                \
        (blk)->cached = 0;                                                              \
        (blk)->prefetch = 0;                                                            \
}while(0);

void cache_init_lru(struct cache_params *params, lru_data *policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

#define MEM_ALLOC(size) (xcalloc(size, sizeof(lru_list)))
  
  /* Allocate list heads */
  LRU_DATA_VALID_HLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(LRU_DATA_VALID_HLST(policy_data));

  LRU_DATA_VALID_TLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(LRU_DATA_VALID_TLST(policy_data));

  LRU_DATA_FREE_HLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(LRU_DATA_FREE_HLST(policy_data));

  LRU_DATA_FREE_TLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(LRU_DATA_FREE_TLST(policy_data));

  /* Initialize valid head and tail pointers */
  LRU_DATA_VALID_HEAD(policy_data) = NULL;
  LRU_DATA_VALID_TAIL(policy_data) = NULL;

  LRU_DATA_BLOCKS(policy_data) =
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));
  assert(LRU_DATA_BLOCKS(policy_data));

  /* Initialize array of blocks */
  LRU_DATA_FREE_HEAD(policy_data) = &(LRU_DATA_BLOCKS(policy_data)[0]);
  LRU_DATA_FREE_TAIL(policy_data) = &(LRU_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&LRU_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&LRU_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&LRU_DATA_BLOCKS(policy_data)[way])->next  = way ? 
      (&LRU_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&LRU_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&LRU_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MEM_ALLOC
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_lru(lru_data *policy_data)
{
  /* Ensure valid arguments */
  assert(policy_data);

  /* Free all data blocks */
  free(LRU_DATA_BLOCKS(policy_data));
}


struct cache_block_t * cache_find_block_lru(lru_data *policy_data, long long tag)
{
  struct cache_block_t *head;
  struct cache_block_t *node;
  
  /* Ensure valid arguments */
  assert(policy_data);

  head = LRU_DATA_VALID_HEAD(policy_data);

  for (node = head; node; node = node->prev)
  {
    assert(node->state != cache_block_invalid);

    if (node->tag == tag)
      break;
  }

  return node;
}

void cache_fill_block_lru(lru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info)
{
  struct cache_block_t *block;

  /* Check: tag, state and insertion_position are valid */
  assert(policy_data);
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain block from LRU specific data */
  block = &(LRU_DATA_BLOCKS(policy_data)[way]);

  /* Remove block from free list */
  free_list_remove_block(policy_data, block);
  
  /* Update new block state and stream */
  CACHE_UPDATE_BLOCK_STATE(block, tag, state);
  CACHE_UPDATE_BLOCK_STREAM(block, strm);
  block->dirty = (info && info->spill) ? 1 : 0;
  block->epoch = 0;

  /* Insert block to the head of the queue */
  CACHE_PREPEND_TO_SQUEUE(block, LRU_DATA_VALID_HEAD(policy_data), 
    LRU_DATA_VALID_TAIL(policy_data));
}

int cache_replace_block_lru(lru_data *policy_data)
{
  struct cache_block_t *block;
  
  /* Ensure valid argument */
  assert(policy_data);

  /* Try to find an invalid block. */
  for (block = LRU_DATA_FREE_HEAD(policy_data); block; block = block->next)
    return block->way;

  /* Remove a nonbusy block from the tail */
  for (block = LRU_DATA_VALID_TAIL(policy_data); block; block = block->prev)
    if (!block->busy)
    {
      return block->way;
    }

  /* If no non busy block can be found, return -1 */
  return -1;
}

void cache_access_block_lru(lru_data *policy_data, int way, int strm,
  memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  blk  = &(LRU_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;
  
  /* Check: block's tag and state are valid */
  assert(policy_data);
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);
  
  /* Update epoch only for demand reads */
  if (info && info->fill == TRUE)
  {
    if (blk->stream == info->stream)
    {
      blk->epoch = (blk->epoch == 3) ? 3 : blk->epoch + 1;
    }
    else
    {
      blk->epoch = 0;
    }
  }

  CACHE_UPDATE_BLOCK_STREAM(blk, strm);
  blk->dirty = (info && info->spill) ? 1 : 0;
  
  /* If block is not at the head of the valid list */
  if (blk->next)
  {
    /* Update block queue if block got new RRPV */
    CACHE_REMOVE_FROM_SQUEUE(blk, LRU_DATA_VALID_HEAD(policy_data),
      LRU_DATA_VALID_TAIL(policy_data));
    CACHE_PREPEND_TO_SQUEUE(blk, LRU_DATA_VALID_HEAD(policy_data), 
      LRU_DATA_VALID_TAIL(policy_data));
  }
}

/* Update state of block. */
void cache_set_block_lru(lru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
        struct cache_block_t *block;
        
        /* Ensure valid arguments */
        assert(policy_data);
        assert(tag >= 0);

        block = &(LRU_DATA_BLOCKS(policy_data))[way];

        /* Check: tag matches with the block's tag. */
        assert(block->tag == tag);

        /* Check: block must be in valid state. It is not possible to set
         * state for an invalid block.*/
        assert(block->state != cache_block_invalid);
        
        /* Assign access stream */
        block->stream = stream;

        if (state != cache_block_invalid)
        {
          block->state = state;
          return;
        }
          
        assert(block->state != cache_block_invalid);

        /* Invalidate block */
        block->state = cache_block_invalid;
        block->epoch = 0;

        /* Remove block from valid list and insert into free list */
        CACHE_REMOVE_FROM_SQUEUE(block, LRU_DATA_VALID_HEAD(policy_data),
          LRU_DATA_VALID_TAIL(policy_data));
        CACHE_APPEND_TO_SQUEUE(block, LRU_DATA_FREE_HEAD(policy_data), 
          LRU_DATA_FREE_TAIL(policy_data));
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_lru(lru_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  /* Ensure valid arguments */
  assert(policy_data);      
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (LRU_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (LRU_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (LRU_DATA_BLOCKS(policy_data)[way]).stream);

  return LRU_DATA_BLOCKS(policy_data)[way];
}
