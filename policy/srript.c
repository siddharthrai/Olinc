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
#include "srript.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))

#define CACHE_UPDATE_BLOCK_STATE(block, tag, state_in)        \
do                                                            \
{                                                             \
  (block)->tag   = (tag);                                     \
  (block)->state = (state_in);                                \
}while(0)

#define CACHE_SRRIPT_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv) \
do                                                            \
{                                                             \
    int dif = 0;                                              \
                                                              \
    for (int i = rrpv - 1; i >= 0; i--)                       \
    {                                                         \
      assert(i <= rrpv);                                      \
                                                              \
      if ((head_ptr)[i].head)                                 \
      {                                                       \
        if (!dif)                                             \
        {                                                     \
          dif = rrpv - i;                                     \
        }                                                     \
                                                              \
        assert((tail_ptr)[i].head);                           \
                                                              \
        (head_ptr)[i + dif].head  = (head_ptr)[i].head;       \
        (tail_ptr)[i + dif].head  = (tail_ptr)[i].head;       \
        (head_ptr)[i].head        = NULL;                     \
        (tail_ptr)[i].head        = NULL;                     \
                                                              \
        struct cache_block_t *node = (head_ptr)[i + dif].head;\
                                                              \
        /* point data to new RRPV head */                     \
        for (; node; node = node->prev)                       \
        {                                                     \
          node->data = &(head_ptr[i + dif]);                  \
        }                                                     \
      }                                                       \
      else                                                    \
      {                                                       \
        assert(!((tail_ptr)[i].head));                        \
      }                                                       \
    }                                                         \
}while(0)

#define CACHE_GET_BLOCK_STREAM(block, strm)                   \
do                                                            \
{                                                             \
  strm = (block)->stream;                                     \
}while(0)

#define CACHE_UPDATE_BLOCK_STREAM(block, strm)                \
do                                                            \
{                                                             \
  if ((block)->stream == CS && strm != CS)                    \
  {                                                           \
    cs_blocks_srript--;                                       \
  }                                                           \
                                                              \
  if (strm == CS && (block)->stream != CS)                    \
  {                                                           \
    cs_blocks_srript++;                                       \
  }                                                           \
                                                              \
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


int cs_blocks_srript = 0;

void cache_init_srript(struct cache_params *params, srript_data *policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((srript_list *)xcalloc(size, sizeof(srript_list)))

  /* Create per rrpv buckets */
  SRRIPT_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SRRIPT_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  
  assert(SRRIPT_DATA_VALID_HEAD(policy_data));
  assert(SRRIPT_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIPT_DATA_MAX_RRPV(policy_data) = MAX_RRPV;

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SRRIPT_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    SRRIPT_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    SRRIPT_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  SRRIPT_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

  /* Initialize array of blocks */
  SRRIPT_DATA_FREE_HEAD(policy_data) = &(SRRIPT_DATA_BLOCKS(policy_data)[0]);
  SRRIPT_DATA_FREE_TAIL(policy_data) = &(SRRIPT_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SRRIPT_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SRRIPT_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SRRIPT_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SRRIPT_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SRRIPT_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SRRIPT_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV
#undef CACHE_ALLOC
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_srript(srript_data *policy_data)
{
  /* Free all data blocks */
  free(SRRIPT_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (SRRIPT_DATA_VALID_HEAD(policy_data))
  {
    free(SRRIPT_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SRRIPT_DATA_VALID_TAIL(policy_data))
  {
    free(SRRIPT_DATA_VALID_TAIL(policy_data));
  }
}

struct cache_block_t * cache_find_block_srript(srript_data *policy_data, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SRRIPT_DATA_VALID_HEAD(policy_data)[rrpv].head;

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

void cache_fill_block_srript(srript_data *policy_data, int way, long long tag, 
  enum cache_block_state_t state, int strm, memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain SRRIP specific data */
  block = &(SRRIPT_DATA_BLOCKS(policy_data)[way]);
  
  assert(block->stream == 0);

  /* Remove block from free list */
  free_list_remove_block(policy_data, block);
  
  /* Update new block state and stream */
  CACHE_UPDATE_BLOCK_STATE(block, tag, state);
  CACHE_UPDATE_BLOCK_STREAM(block, strm);
  block->dirty  = (info && info->spill) ? 1 : 0;
  block->epoch  = 0;
  block->access = 0;
  
  /* Get RRPV to be assigned to the new block */
  if (info && info->fill == TRUE)
  {
    rrpv = cache_get_fill_rrpv_srript(policy_data, block->stream);
  }
  else
  {
    rrpv = SRRIPT_DATA_MAX_RRPV(policy_data);
  }

  /* Ensure a valid RRPV */
  assert(rrpv >=0 && rrpv <= policy_data->max_rrpv); 

  /* Insert block in to the corresponding RRPV queue */
  CACHE_APPEND_TO_QUEUE(block, 
    SRRIPT_DATA_VALID_HEAD(policy_data)[rrpv], 
    SRRIPT_DATA_VALID_TAIL(policy_data)[rrpv]);
}

int cache_replace_block_srript(srript_data *policy_data)
{
  struct cache_block_t *block;

  int rrpv;
  int bt_block_way;
  
  bt_block_way = -1;

  /* Try to find an invalid block always from head of the free list. */
  for (block = policy_data->free_head; block; block = block->next)
    return block->way;

  /* Obtain RRPV from where to replace the block */
  rrpv = cache_get_replacement_rrpv_srript(policy_data);
  
  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= SRRIPT_DATA_MAX_RRPV(policy_data));
  
  /* If there is no block with required RRPV, increment RRPV of all the blocks
   * until we get one with the required RRPV */
  if (SRRIPT_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
  {
    CACHE_SRRIPT_INCREMENT_RRPV(SRRIPT_DATA_VALID_HEAD(policy_data), 
      SRRIPT_DATA_VALID_TAIL(policy_data), rrpv);
  }
 
  /* Remove a nonbusy block from the tail */
  unsigned int min_wayid = ~(0);

  for (block = SRRIPT_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
  {
    if (!block->busy && block->way < min_wayid)
      min_wayid = block->way;
  }
  
  /* If no non busy block can be found, return -1 */
  return (min_wayid != ~(0)) ? min_wayid : -1;
}

void cache_access_block_srript(srript_data *policy_data, int way, int strm,
  memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int old_rrpv;
  int new_rrpv;
  
  blk  = &(SRRIPT_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);
  
  /* Update RRPV and epoch only for read hits */
  if (info && info->fill == TRUE)
  {
    /* Get old RRPV from the block */
    old_rrpv = (((srript_list *)(blk->data))->rrpv);

    /* Get new RRPV using policy specific function */
    new_rrpv = cache_get_new_rrpv_srript(old_rrpv);

    /* Update block queue if block got new RRPV */
    if (new_rrpv != old_rrpv)
    {
      CACHE_REMOVE_FROM_QUEUE(blk, SRRIPT_DATA_VALID_HEAD(policy_data)[old_rrpv],
          SRRIPT_DATA_VALID_TAIL(policy_data)[old_rrpv]);
      CACHE_APPEND_TO_QUEUE(blk, SRRIPT_DATA_VALID_HEAD(policy_data)[new_rrpv], 
          SRRIPT_DATA_VALID_TAIL(policy_data)[new_rrpv]);
    }

    if (info && blk->stream == info->stream)
    {
      blk->epoch  = (blk->epoch == 3) ? 3 : blk->epoch + 1;
    }
    else
    {
      blk->epoch = 0;
    }
  }
  
  CACHE_UPDATE_BLOCK_STREAM(blk, strm);
  
  blk->dirty    = (info && info->spill) ? 1 : 0;
  blk->access  += 1;
}

int cache_get_fill_rrpv_srript(srript_data *policy_data, ub1 stream)
{
  return SRRIPT_DATA_MAX_RRPV(policy_data) - 1;
}

int cache_get_replacement_rrpv_srript(srript_data *policy_data)
{
  return SRRIPT_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_srript(int old_rrpv)
{
  return 0;
}

/* Update state of block. */
void cache_set_block_srript(srript_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  block = &(SRRIPT_DATA_BLOCKS(policy_data))[way];

  /* Check: tag matches with the block's tag. */
  assert(block->tag == tag);

  /* Check: block must be in valid state. It is not possible to set
   * state for an invalid block.*/
  assert(block->state != cache_block_invalid);

  if (state != cache_block_invalid)
  {
    /* Assign access stream */
    CACHE_UPDATE_BLOCK_STATE(block, tag, state);
    CACHE_UPDATE_BLOCK_STREAM(block, stream);
    return;
  }

  /* Invalidate block */
  CACHE_UPDATE_BLOCK_STATE(block, tag, cache_block_invalid);
  CACHE_UPDATE_BLOCK_STREAM(block, NN);
  block->epoch        = 0;
  block->access       = 0;
  block->is_bt_block  = FALSE;

  /* Get old RRPV from the block */
  int old_rrpv = (((srript_list *)(block->data))->rrpv);

  /* Remove block from valid list and insert into free list */
  CACHE_REMOVE_FROM_QUEUE(block, SRRIPT_DATA_VALID_HEAD(policy_data)[old_rrpv],
      SRRIPT_DATA_VALID_TAIL(policy_data)[old_rrpv]);
  CACHE_APPEND_TO_SQUEUE(block, SRRIPT_DATA_FREE_HEAD(policy_data), 
      SRRIPT_DATA_FREE_TAIL(policy_data));

}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_srript(srript_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  assert(policy_data);
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (SRRIPT_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (SRRIPT_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (SRRIPT_DATA_BLOCKS(policy_data)[way]).stream);

  return SRRIPT_DATA_BLOCKS(policy_data)[way];
}

/* Get tag and state of a block. */
int cache_count_block_srript(srript_data *policy_data, ub1 strm)
{
  int     max_rrpv;
  int     count;
  struct  cache_block_t *head;
  struct  cache_block_t *node;
  
  assert(policy_data);

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;
  count     = 0;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SRRIPT_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);
      if (node->stream == strm)
        count++;
    }
  }

  return count;
}
