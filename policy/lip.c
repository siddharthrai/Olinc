/*
 * Copyright (C) 2014  Siddharth Rai
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <stdlib.h>
#include <assert.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "lip.h"

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
 * Private Functions
 */
#define free_list_remove_block(set, blk)                                                \
do                                                                                      \
{                                                                                       \
        /* Check: free list must be non empty as it contains the current block. */      \
        assert((set)->free_head && (set)->free_tail);                                   \
                                                                                        \
        /* Check: current block must be in invalid state */                             \
        assert((blk)->state == cache_block_invalid);                                    \
                                                                                        \
        CACHE_REMOVE_FROM_SQUEUE(blk, set->free_head, set->free_tail);                  \
                                                                                        \
        (blk)->next = NULL;                                                             \
        (blk)->prev = NULL;                                                             \
                                                                                        \
        /* Reset block state */                                                         \
        (blk)->busy = 0;                                                                \
        (blk)->cached = 0;                                                              \
        (blk)->prefetch = 0;                                                            \
}while(0);

/* Initialize policy specific structures */
void cache_init_lip(struct cache_params *params, lip_data *policy_data)
{
  assert(params);
  assert(policy_data);

  /* Create per rrpv buckets */
  LIP_DATA_VALID_HEAD(policy_data) = NULL;
  LIP_DATA_VALID_TAIL(policy_data) = NULL;

  /* Create array of blocks */
  LIP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

  /* Initialize array of blocks */
  LIP_DATA_FREE_HEAD(policy_data) = &(LIP_DATA_BLOCKS(policy_data)[0]);
  LIP_DATA_FREE_TAIL(policy_data) = &(LIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&LIP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&LIP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&LIP_DATA_BLOCKS(policy_data)[way])->next  = way ? 
      (&LIP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&LIP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&LIP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_lip(lip_data *policy_data)
{
  assert(policy_data);

  /* Free all data blocks */
  free(LIP_DATA_BLOCKS(policy_data));
}

/* Find the block with the tag */
struct cache_block_t * cache_find_block_lip(lip_data *policy_data, long long tag)
{
  struct cache_block_t *head;
  struct cache_block_t *node;
  
  assert(policy_data);

  head = LIP_DATA_VALID_HEAD(policy_data);

  for (node = head; node; node = node->prev)
  {
    assert(node->state != cache_block_invalid);

    if (node->tag == tag)
      break;
  }

  return node;
}

/* Fill a new block */
void cache_fill_block_lip(lip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info)
{
  struct cache_block_t *block;
  
  /* Insrure valid arguments */
  assert(policy_data);
  assert(way >= 0);
  assert(tag >= 0);
  assert(state != cache_block_invalid);
  assert(strm <= TST);

  block = &(LIP_DATA_BLOCKS(policy_data)[way]);

  /* Remove block from free list */
  free_list_remove_block(policy_data, block);
  
  /* Update new block state and stream */
  CACHE_UPDATE_BLOCK_STATE(block, tag, state);
  CACHE_UPDATE_BLOCK_STREAM(block, strm);
  block->dirty = (info && info->spill) ? 1 : 0;
  block->epoch = 0;
  
  /* Insert block to the tail of the valid queue  */
  CACHE_APPEND_TO_SQUEUE(block, LIP_DATA_VALID_HEAD(policy_data), 
    LIP_DATA_VALID_TAIL(policy_data));
}

/* Get a replacement candidate  */
int cache_replace_block_lip(lip_data *policy_data)
{
  struct cache_block_t *block;

  /* Ensure valid argument  */
  assert(policy_data);

  /* Try to find an invalid block. */
  for (block = policy_data->free_head; block; block = block->next)
    return block->way;

  /* Remove a nonbusy block from the tail */
  for (block = LIP_DATA_VALID_TAIL(policy_data); block; block = block->prev)
    if (!block->busy)
      return block->way;

  /* If no non busy block can be found, return -1 */
  return -1;
}

/* Age the given block according to the policy */
void cache_access_block_lip(lip_data *policy_data, int way, int strm,
  memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;
  
  /* Ensure valid arguments */
  assert(policy_data);
  assert(way >= 0);
  assert(strm <= TST);
  
  /* Get the cache block */
  blk  = &(LIP_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Validate cache block info */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);
  
  /* If block is not at the head update its position */
  if (blk->next)
  {
    /* Move the block from current position to the head of the list */
    CACHE_REMOVE_FROM_SQUEUE(blk, LIP_DATA_VALID_HEAD(policy_data),
      LIP_DATA_VALID_TAIL(policy_data));
    CACHE_PREPEND_TO_SQUEUE(blk, LIP_DATA_VALID_HEAD(policy_data), 
      LIP_DATA_VALID_TAIL(policy_data));
  }
  
  /* Update epoch only for demand reads */
  if (info && info->fill == TRUE)
  {
    if (info && (blk->stream == info->stream))
    {
      blk->epoch = (blk->epoch == 3) ? 3 : blk->epoch + 1;
    }
    else
    {
      blk->epoch = 0;
    }
  }

  /* Update block steam */
  blk->dirty = (info && info->spill) ? 1 : 0;
  CACHE_UPDATE_BLOCK_STREAM(blk, strm);
}


/* Update state of the block. */
void cache_set_block_lip(lip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  block = &(LIP_DATA_BLOCKS(policy_data))[way];

  /* Check: tag matches with the block's tag. */
  assert(block->tag == tag);

  /* Check: block must be in valid state. It is not possible to set
   * state for an invalid block.*/
  assert(block->state != cache_block_invalid);
  
  /* Set access stream */
  block->stream = stream;

  if (state != cache_block_invalid)
  {
    block->state = state;
    return;
  }

  /* Invalidate block */
  block->state = cache_block_invalid;
  block->epoch = 0;

  /* Remove block from valid list and insert into free list */
  CACHE_REMOVE_FROM_SQUEUE(block, LIP_DATA_VALID_HEAD(policy_data),
    LIP_DATA_VALID_TAIL(policy_data));
  CACHE_APPEND_TO_SQUEUE(block, LIP_DATA_FREE_HEAD(policy_data), 
    LIP_DATA_FREE_TAIL(policy_data));
}


/* Get the tag and the state of the block. */
struct cache_block_t cache_get_block_lip(lip_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  PTR_ASSIGN(tag_ptr, (LIP_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (LIP_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (LIP_DATA_BLOCKS(policy_data)[way]).stream);

  return LIP_DATA_BLOCKS(policy_data)[way];
}
