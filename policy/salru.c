/*
 * Copyright (C) 2014 Siddharth Rai
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
#include "salru.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))

#define CACHE_UPDATE_BLOCK_STATE(block, tag, state_in)        \
do                                                            \
{                                                             \
  (block)->tag   = (tag);                                     \
  (block)->state = (state_in);                                \
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


void cache_init_salru(struct cache_params *params, salru_data *policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

  /* For SALRU blocks are organized in per stream list */
#define STREAMS         (params->streams)
#define WAYS            (params->ways)
#define MEM_ALLOC(size) ((salru_list *)xcalloc(size, sizeof(salru_list)))

  /* Create per stream buckets */
  SALRU_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(STREAMS);
  SALRU_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(STREAMS);
  
  assert(SALRU_DATA_VALID_HEAD(policy_data));
  assert(SALRU_DATA_VALID_TAIL(policy_data));

  /* Set max streams for the set */
  SALRU_DATA_STREAMS(policy_data) = STREAMS;
  SALRU_DATA_WAYS(policy_data)    = WAYS;

  /* Initialize head nodes */
  for (int i = 0; i < STREAMS; i++)
  {
    SALRU_DATA_VALID_HEAD(policy_data)[i].stream  = i;
    SALRU_DATA_VALID_HEAD(policy_data)[i].head    = NULL;
    SALRU_DATA_VALID_TAIL(policy_data)[i].head    = NULL;
  }

  /* Create array of blocks */
  SALRU_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

  /* Initialize array of blocks */
  SALRU_DATA_FREE_HEAD(policy_data) = &(SALRU_DATA_BLOCKS(policy_data)[0]);
  SALRU_DATA_FREE_TAIL(policy_data) = &(SALRU_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SALRU_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SALRU_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SALRU_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SALRU_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SALRU_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SALRU_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }
  
  SALRU_DATA_ALLOCATED(policy_data) = (ub4 *)xmalloc(sizeof(unsigned int) * params->streams + 1);
  assert(SALRU_DATA_ALLOCATED(policy_data));
  memset(SALRU_DATA_ALLOCATED(policy_data), 0, sizeof(unsigned int) * params->streams + 1);

  SALRU_DATA_DFPOLICY(policy_data) = cache_policy_lru;
  SALRU_DATA_CFPOLICY(policy_data) = cache_policy_lru;
#undef STREAMS
#undef WAYS
#undef CACHE_ALLOC
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_salru(salru_data *policy_data)
{
  /* Free all data blocks */
  free(SALRU_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (SALRU_DATA_VALID_HEAD(policy_data))
  {
    free(SALRU_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SALRU_DATA_VALID_TAIL(policy_data))
  {
    free(SALRU_DATA_VALID_TAIL(policy_data));
  }

  free(SALRU_DATA_ALLOCATED(policy_data));
}

struct cache_block_t * cache_find_block_salru(salru_data *policy_data, long long tag)
{
  int     streams;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  streams   = policy_data->streams;
  node      = NULL;

  for (int stream = 0; stream < streams; stream++)
  {
    head = SALRU_DATA_VALID_HEAD(policy_data)[stream].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

end:
  return node;
}

void cache_update_recency_salru(salru_data *policy_data, ub4 way_in)
{
  for (int way = 0; way < SALRU_DATA_WAYS(policy_data); way++)
  {
    if (way != way_in)
    {
      SALRU_DATA_BLOCKS(policy_data)[way].recency += 1;
    }
  }

  SALRU_DATA_BLOCKS(policy_data)[way_in].recency = 0;
}

void cache_fill_block_current_policy(salru_data *policy_data, int strm, struct cache_block_t *block)
{
  
  switch(SALRU_DATA_CFPOLICY(policy_data))
  {
    case cache_policy_lru:

      /* Insert block at the head of the corresponding stream queue */
      CACHE_PREPEND_TO_QUEUE(block, 
        SALRU_DATA_VALID_HEAD(policy_data)[strm - 1], 
        SALRU_DATA_VALID_TAIL(policy_data)[strm - 1]);

      break;

    case cache_policy_lip:

      /* Insert block at the tail of the corresponding stream queue */
      CACHE_APPEND_TO_QUEUE(block, 
        SALRU_DATA_VALID_HEAD(policy_data)[strm - 1], 
        SALRU_DATA_VALID_TAIL(policy_data)[strm - 1]);

      break;

    case cache_policy_bypass:

      /* Block is not inserted if policy is to bypass the cache */ 
      break;

    default:

      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);

  } 

}

void cache_fill_block_salru(salru_data *policy_data, int way, long long tag, 
  enum cache_block_state_t state, int strm_in, memory_trace *info)
{
  struct cache_block_t *block;
  int    strm;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);
  
  /* If a valid stream is passed use that as accessing stream otherwise
   * obtain stream from the memory trace */
  strm = (info && strm_in == NN) ? info->stream : strm_in;

  /* Ensure stream is with in max streams bound */
  assert(strm > 0 && strm <= SALRU_DATA_STREAMS(policy_data));
  
  /* Obtain SRRIP specific data */
  block = &(SALRU_DATA_BLOCKS(policy_data)[way]);
  assert(block->stream == NN);
  
  /* Remove block from free list */
  free_list_remove_block(policy_data, block);
  
  /* Update new block state and stream */
  CACHE_UPDATE_BLOCK_STATE(block, tag, state);
  CACHE_UPDATE_BLOCK_STREAM(block, strm);

  block->dirty  = (info && info->spill) ? 1 : 0;
  block->epoch  = 0;
  
  assert(SALRU_DATA_VALID_HEAD(policy_data)[strm - 1].head != block);

  SALRU_DATA_ALLOCATED(policy_data)[strm - 1] += 1;
  
  /* Fill block according to current policy */
  cache_fill_block_current_policy(policy_data, strm, block);
  
  assert(SALRU_DATA_ALLOCATED(policy_data)[strm - 1] <= policy_data->ways);

  cache_update_recency_salru(policy_data, way);
  
  /* Reset current fill policy back to default */
  SALRU_DATA_CFPOLICY(policy_data) = SALRU_DATA_DFPOLICY(policy_data);
}

int cache_replace_block_salru(salru_data *policy_data, int stream_in, memory_trace *info)
{
  struct  cache_block_t *block;
  int     stream;

  /* Try to find an invalid block always from head of the free list. */
  for (block = policy_data->free_head; block; block = block->next)
    return block->way;

  /* Obtain stream from which to replace the block */
  stream = cache_get_replacement_stream_salru(policy_data, stream_in, info);
  
  /* Ensure stream is with in max streams bound */
  assert(stream >= 0 && stream < SALRU_DATA_STREAMS(policy_data));
  
  for (block = SALRU_DATA_VALID_TAIL(policy_data)[stream].head; block; 
    block = block->next)
  {
    if (!block->busy)
    {
      assert(SALRU_DATA_ALLOCATED(policy_data)[block->stream - 1] > 0);

      SALRU_DATA_ALLOCATED(policy_data)[block->stream - 1] -= 1;

      return block->way;
    }
  }
  
  /* If no non busy block can be found, return -1 */
  return -1;
}

void cache_access_block_salru(salru_data *policy_data, int way, int strm_in,
  memory_trace *info)
{
  struct  cache_block_t *blk   = NULL;
  struct  cache_block_t *next  = NULL;
  struct  cache_block_t *prev  = NULL;
  int     strm;

  /* If a valid stream is passed use that as accessing stream otherwise
   * obtain stream from the memory trace */
  strm = (info && strm_in == NN) ? info->stream : strm_in;

  /* Ensure stream is with in max streams bound */
  assert(strm > 0 && strm <= SALRU_DATA_STREAMS(policy_data));
  
  blk  = &(SALRU_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);

  /* Update epoch only for demand read */
  if (info && info->fill == TRUE)
  {
    if (info && (blk->stream == info->stream))
    {
      blk->epoch  = (blk->epoch == 3) ? 3 : blk->epoch + 1;
    }
    else
    {
      blk->epoch = 0;
    }
  }

  if (info && (blk->stream != info->stream))
  {
    SALRU_DATA_ALLOCATED(policy_data)[blk->stream - 1]  -= 1;
    SALRU_DATA_ALLOCATED(policy_data)[info->stream - 1] += 1;
    
    assert(info->stream == strm);

    CACHE_REMOVE_FROM_QUEUE(blk, SALRU_DATA_VALID_HEAD(policy_data)[blk->stream - 1],
      SALRU_DATA_VALID_TAIL(policy_data)[blk->stream - 1]);
    CACHE_APPEND_TO_QUEUE(blk, SALRU_DATA_VALID_HEAD(policy_data)[strm - 1], 
      SALRU_DATA_VALID_TAIL(policy_data)[strm - 1]);
  }

  blk->dirty  = (info && info->spill) ? 1 : 0;
  CACHE_UPDATE_BLOCK_STREAM(blk, strm);

  cache_update_recency_salru(policy_data, way);
}

ub4 cache_get_replacement_stream_salru(salru_data *policy_data, int strm_in, memory_trace *info)
{
  ub4 victim_stream;    /* Victim stream */
  ub4 request_stream;   /* Requesting stream */
  ub4 streams;          /* Total streams */
  ub4 recency;          /* Temprary for LRU block */
  ub4 allocation;       /* Assigned allocation */
  ub4 allocated;        /* Allocated space */

  struct  cache_block_t *node;

  /* If a valid stream is passed use that as accessing stream otherwise
   * obtain stream from the memory trace */
  request_stream = (info && strm_in == NN) ? info->stream : strm_in;

  /* Ensure stream is with in max streams bound */
  assert(request_stream > 0 && request_stream <= SALRU_DATA_STREAMS(policy_data));
  
  /* Get allocation and partition for this stream */
  allocation    = SALRU_DATA_PARTITIONS(policy_data)[request_stream - 1];
  allocated     = SALRU_DATA_ALLOCATED(policy_data)[request_stream - 1];
  victim_stream = request_stream - 1; 
  
  assert(allocation > 0 && allocation <= policy_data->ways);

  /* If allocated blocks are equal or above allocation replace from 
   * requesting stream or choose other stream */
  if (allocated >= allocation)
  {
    return victim_stream;
  }
  else
  {
    /* Obtain victim stream */
    streams = policy_data->streams;
    recency = 0;
    for (int stream = 0; stream < streams; stream++)
    {
      node = SALRU_DATA_VALID_TAIL(policy_data)[stream].head;
      
      if (node && (stream != request_stream - 1 )&& node->recency >= recency)
      {
        victim_stream = stream;
        recency       = node->recency;
      }
    }
  }

  return victim_stream;
}

/* Update state of block. */
void cache_set_block_salru(salru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 strm_in, memory_trace *info)
{
  struct  cache_block_t *block;
  int     strm;
  
  /* If a valid stream is passed use that as accessing stream otherwise
   * obtain stream from the memory trace */
  strm = (info && strm_in == NN) ? info->stream :  strm_in;

  /* Ensure stream is with in max streams bound */
  assert(strm > 0 && strm <= SALRU_DATA_STREAMS(policy_data));
  
  block = &(SALRU_DATA_BLOCKS(policy_data))[way];

  /* Check: tag matches with the block's tag. */
  assert(block->tag == tag);

  /* Check: block must be in valid state. It is not possible to set
   * state for an invalid block.*/
  assert(block->state != cache_block_invalid);

  if (state != cache_block_invalid)
  {
    /* Assign access stream */
    CACHE_UPDATE_BLOCK_STATE(block, tag, state);
    CACHE_UPDATE_BLOCK_STREAM(block, strm);
    return;
  }

  assert(block->stream == strm);
  
  block->epoch = 0;

  /* Remove block from valid list and insert into free list */
  CACHE_REMOVE_FROM_QUEUE(block, SALRU_DATA_VALID_HEAD(policy_data)[strm - 1],
      SALRU_DATA_VALID_TAIL(policy_data)[strm - 1]);
  CACHE_APPEND_TO_SQUEUE(block, SALRU_DATA_FREE_HEAD(policy_data), 
      SALRU_DATA_FREE_TAIL(policy_data));

  /* Invalidate block */
  CACHE_UPDATE_BLOCK_STATE(block, tag, cache_block_invalid);
  CACHE_UPDATE_BLOCK_STREAM(block, NN);
}


/* Get tag and state of a block. */
struct cache_block_t  cache_get_block_salru(salru_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  assert(policy_data);
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (SALRU_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (SALRU_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (SALRU_DATA_BLOCKS(policy_data)[way]).stream);

  return SALRU_DATA_BLOCKS(policy_data)[way];
}

/* Set new partitions */
void cache_set_partition(salru_data *policy_data, ub4 *partition)
{
  assert(policy_data);
  assert(partition);

  policy_data->per_stream_partition  = partition;
}

/* Set policy for next fill */
void cache_set_current_fill_policy_salru(salru_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip 
    || policy == cache_policy_bypass);

  SALRU_DATA_CFPOLICY(policy_data) = policy;
}
