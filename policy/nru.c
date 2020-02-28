/*
 * Copyright (C) 2014 mainakc 
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
#include "nru.h"

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
  (block)->stream = (strm);                                   \
}while(0)

/*
 * Public Functions
 */

void cache_init_nru(struct cache_params *params, nru_data *policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  
  int ways = params->ways;

#define MEM_ALLOC(size) (xcalloc(size, sizeof(struct cache_block_t)))

  NRU_DATA_WAYS(policy_data)   = params->ways;
  NRU_DATA_BLOCKS(policy_data) = (struct cache_block_t *)MEM_ALLOC(ways);
  
  /* Initialize all blocks */
  for (int way = 0; way < ways; way++)
  {
    (&NRU_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&NRU_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&NRU_DATA_BLOCKS(policy_data)[way])->nru   = TRUE;
    (&NRU_DATA_BLOCKS(policy_data)[way])->next  = NULL; 
    (&NRU_DATA_BLOCKS(policy_data)[way])->prev  = NULL;
  }

#undef MEM_ALLOC
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_nru(nru_data *policy_data)
{
  /* Ensure valid arguments */
  assert(policy_data);

  /* Free all data blocks */
  free(NRU_DATA_BLOCKS(policy_data));
}


struct cache_block_t * cache_find_block_nru(nru_data *policy_data, long long tag)
{
  struct cache_block_t *blocks; /* Pointer to block array */
  int ways;                     /* # way */

  /* Ensure valid arguments */
  assert(policy_data);
  
  blocks = NRU_DATA_BLOCKS(policy_data);
  ways   = NRU_DATA_WAYS(policy_data);

  for (int way = 0; way < ways; way++)
  {
    assert(blocks[way].way == way);

    if (blocks[way].tag == tag && blocks[way].state != cache_block_invalid)
      return &(blocks[way]);
  }

  return NULL;
}

void cache_fill_block_nru(nru_data *policy_data, int way_in, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info)
{
  struct cache_block_t *block_in;
  struct cache_block_t *block;
  int way;

  /* Check: tag, state and insertion_position are valid */
  assert(policy_data);
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain block from LRU specific data */
  block_in = &(NRU_DATA_BLOCKS(policy_data)[way_in]);

  /* Update new block state and stream */
  CACHE_UPDATE_BLOCK_STATE(block_in, tag, state);
  CACHE_UPDATE_BLOCK_STREAM(block_in, strm);
  block_in->dirty = (info && info->spill) ? 1 : 0;
  block_in->epoch = 0;

  /* Set current block as non NRU  and check for an NRU blocks*/
  block_in->nru = FALSE;

  for (way = 0; way < policy_data->ways; way++)
  {
    block = &(NRU_DATA_BLOCKS(policy_data)[way]);

    if (block->nru == TRUE)
      break;
  }
  
  /* If no NRU block is present make all blocks NRU */
  if (way == policy_data->ways)
  {
    for (way = 0; way < policy_data->ways; way++)
    {
      block = &(NRU_DATA_BLOCKS(policy_data)[way]);
      block->nru = TRUE;
    }
  }
  
  /* Set current block as non NRU */
  block_in->nru = FALSE;
}

int cache_replace_block_nru(nru_data *policy_data)
{
  struct cache_block_t *block;
  
  /* Ensure valid argument */
  assert(policy_data);

  /* Try to find an invalid block. */
  for (int way = 0; way < policy_data->ways; way++)
  {
    block = &(NRU_DATA_BLOCKS(policy_data)[way]);
    
    /* NRU bit is reset */
    if (block->nru == TRUE && !block->busy)
      return way;
  }

  /* If no non busy block can be found, return -1 */
  return -1;
}

void cache_access_block_nru(nru_data *policy_data, int way_in, int strm,
  memory_trace *info)
{
  struct cache_block_t *block_in  = NULL;
  struct cache_block_t *block     = NULL;
  int way;

  block_in  = &(NRU_DATA_BLOCKS(policy_data)[way_in]);
  
  /* Check: block's tag and state are valid */
  assert(policy_data);
  assert(block_in->tag >= 0);
  assert(block_in->state != cache_block_invalid);
  
  /* Update epoch only for demand reads */
  if (info && info->fill == TRUE)
  {
    if (block_in->stream == info->stream)
    {
      block_in->epoch  = (block_in->epoch == 3) ? 3 : block_in->epoch + 1;
    }
    else
    {
      block_in->epoch = 0;
    }
  }

  CACHE_UPDATE_BLOCK_STREAM(block_in, strm);
  block_in->dirty  = (info && info->spill) ? 1 : 0;

  /* Set current block as non NRU  and check for an NRU blocks*/
  block_in->nru = FALSE;

  for (way = 0; way < policy_data->ways; way++)
  {
    block = &(NRU_DATA_BLOCKS(policy_data)[way]);

    if (block->nru == TRUE)
      break;
  }
  
  /* If no NRU block is present make all blocks NRU */
  if (way == policy_data->ways)
  {
    for (way = 0; way < policy_data->ways; way++)
    {
      block = &(NRU_DATA_BLOCKS(policy_data)[way]);
      block->nru = TRUE;
    }
  }
  
  /* Set current block as non NRU */
  block_in->nru = FALSE;
}

/* Update state of block. */
void cache_set_block_nru(nru_data *policy_data, int way_in, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block_in;
  struct cache_block_t *block;
  int way;

  /* Ensure valid arguments */
  assert(policy_data);
  assert(tag >= 0);

  block_in = &(NRU_DATA_BLOCKS(policy_data))[way_in];

  /* Check: tag matches with the block's tag. */
  assert(block_in->tag == tag);

  /* Check: block must be in valid state. It is not possible to set
   * state for an invalid block.*/
  assert(block_in->state != cache_block_invalid);

  /* Assign access stream */
  block_in->stream = stream;
  block_in->epoch  = 0;

  if (state != cache_block_invalid)
  {
    block_in->state = state;
    return;
  }

  assert(block_in->state != cache_block_invalid);

  /* Invalidate block */
  block_in->state = cache_block_invalid;
  block_in->nru   = TRUE;

  for (way = 0; way < policy_data->ways; way++)
  {
    block = &(NRU_DATA_BLOCKS(policy_data)[way]);

    if (block->nru == TRUE)
      break;
  }

  assert(way != policy_data->ways);
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_nru(nru_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  /* Ensure valid arguments */
  assert(policy_data);      
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (NRU_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (NRU_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (NRU_DATA_BLOCKS(policy_data)[way]).stream);

  return NRU_DATA_BLOCKS(policy_data)[way];
}
