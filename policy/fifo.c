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
#include "fifo.h"

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
        assert((set)->free_head && (set)->free_tail);                                   \
                                                                                        \
        /* Check: current block must be in invalid state */                             \
        assert((blk)->state == cache_block_invalid);                                    \
                                                                                        \
        /* Remove block from free list */                                               \
        if (!(blk)->prev && !(blk)->next) /* Block is the only element in free list */  \
        {                                                                               \
                assert((set)->free_head == (blk) && (set)->free_tail == (blk));         \
                (set)->free_head = NULL;                                                \
                (set)->free_tail = NULL;                                                \
        }                                                                               \
        else if (!(blk)->prev) /* Block is at the head of free list */                  \
        {                                                                               \
                assert((set)->free_head == (blk) && (set)->free_tail != (blk));         \
                (set)->free_head = (blk)->next;                                         \
                (blk)->next->prev = NULL;                                               \
        }                                                                               \
        else if (!(blk)->next) /* Block is at the tail of free list */                  \
        {                                                                               \
                assert((set)->free_head != (blk) && (set)->free_tail == (blk));         \
                (set)->free_tail = (blk)->prev;                                         \
                (blk)->prev->next = NULL;                                               \
        }                                                                               \
        else /* Block some where in middle of free list */                              \
        {                                                                               \
                assert((set)->free_head != (blk) && (set)->free_tail != (blk));         \
                (blk)->prev->next = (blk)->next;                                        \
                (blk)->next->prev = (blk)->prev;                                        \
        }                                                                               \
                                                                                        \
        (blk)->next = NULL;                                                             \
        (blk)->prev = NULL;                                                             \
                                                                                        \
        /* Reset block state */                                                         \
        (blk)->busy = 0;                                                                \
        (blk)->cached = 0;                                                              \
        (blk)->prefetch = 0;                                                            \
}while(0);

void cache_init_fifo(struct cache_params *params, fifo_data *policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

#define MEM_ALLOC(size) (xcalloc(size, sizeof(fifo_list)))

  /* Initialize valid head and tail pointers */
  FIFO_DATA_VALID_HEAD(policy_data) = NULL;
  FIFO_DATA_VALID_TAIL(policy_data) = NULL;

  /* Create array of blocks */
  FIFO_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

  /* Initialize array of blocks */
  FIFO_DATA_FREE_HEAD(policy_data) = &(FIFO_DATA_BLOCKS(policy_data)[0]);
  FIFO_DATA_FREE_TAIL(policy_data) = &(FIFO_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&FIFO_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&FIFO_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&FIFO_DATA_BLOCKS(policy_data)[way])->next  = way ? 
      (&FIFO_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&FIFO_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&FIFO_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV
#undef CACHE_ALLOC
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_fifo(fifo_data *policy_data)
{
  /* Ensure valid arguments */
  assert(policy_data);

  /* Free all data blocks */
  free(FIFO_DATA_BLOCKS(policy_data));
}

struct cache_block_t * cache_find_block_fifo(fifo_data *policy_data, long long tag)
{
  struct cache_block_t *head;
  struct cache_block_t *node;

  head = FIFO_DATA_VALID_HEAD(policy_data);

  for (node = head; node; node = node->prev)
  {
    assert(node->state != cache_block_invalid);

    if (node->tag == tag)
      break;
  }

  return node;
}

void cache_fill_block_fifo(fifo_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info)
{
  struct cache_block_t *block;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain block from FIFO specific data */
  block = &(FIFO_DATA_BLOCKS(policy_data)[way]);

  /* Remove block from free list */
  free_list_remove_block(policy_data, block);
  
  /* Update new block state and stream */
  CACHE_UPDATE_BLOCK_STATE(block, tag, state);
  CACHE_UPDATE_BLOCK_STREAM(block, strm);
  block->dirty = (info && info->spill) ? 1 : 0;
  block->epoch = 0;

  /* Insert block to the head of the queue */
  CACHE_APPEND_TO_SQUEUE(block, FIFO_DATA_VALID_HEAD(policy_data), 
    FIFO_DATA_VALID_TAIL(policy_data));
}

int cache_replace_block_fifo(fifo_data *policy_data)
{
  struct cache_block_t *block;

  /* Try to find an invalid block. */
  for (block = policy_data->free_head; block; block = block->next)
    return block->way;

  /* Remove a nonbusy block from the tail */
  for (block = FIFO_DATA_VALID_TAIL(policy_data); block; block = block->prev)
    if (!block->busy)
      return block->way;

  /* If no non busy block can be found, return -1 */
  return -1;
}

void cache_access_block_fifo(fifo_data *policy_data, int way, int strm,
  memory_trace *info)
{
  struct cache_block_t *block;

  /* Obtain block from FIFO specific data */
  block = &(FIFO_DATA_BLOCKS(policy_data)[way]);

  /* Nothing to do for FIFO */
  block->dirty = (info && info->spill) ? 1 : 0;
  
  /* Update epoch only for demand read */
  if (info && info->fill == TRUE)
  {
    if (info && (block->stream == info->stream))
    {
      block->epoch = (block->epoch == 3) ? 3 : block->epoch + 1;
    }
    else
    {
      block->epoch = 0;
    }
  }

  block->stream = strm;
}

/* Update state of block. */
void cache_set_block_fifo(fifo_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
        struct cache_block_t *block;
        
        block = &(FIFO_DATA_BLOCKS(policy_data))[way];

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
        CACHE_REMOVE_FROM_SQUEUE(block, FIFO_DATA_VALID_HEAD(policy_data),
          FIFO_DATA_VALID_TAIL(policy_data));
        CACHE_APPEND_TO_SQUEUE(block, FIFO_DATA_FREE_HEAD(policy_data), 
          FIFO_DATA_FREE_TAIL(policy_data));

}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_fifo(fifo_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
        PTR_ASSIGN(tag_ptr, (FIFO_DATA_BLOCKS(policy_data)[way]).tag);
        PTR_ASSIGN(state_ptr, (FIFO_DATA_BLOCKS(policy_data)[way]).state);
        PTR_ASSIGN(stream_ptr, (FIFO_DATA_BLOCKS(policy_data)[way]).stream);

        return FIFO_DATA_BLOCKS(policy_data)[way];
}
