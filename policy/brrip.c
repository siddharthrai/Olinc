/*
 *  Copyright (C) 2014  Siddharth Rai
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <stdlib.h>
#include <assert.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "brrip.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))
#define BYPASS_RRPV             (-1)

#define CACHE_UPDATE_BLOCK_STATE(block, tag, va, state_in)    \
do                                                            \
{                                                             \
  (block)->tag      = (tag);                                  \
  (block)->vtl_addr = (va);                                   \
  (block)->state    = (state_in);                             \
}while(0)

#define CACHE_BRRIP_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)  \
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
  (block)->stream = strm;                                     \
}while(0)

/*
 * Public Variables
 */

/*
 * Private Functions
 */
#define free_list_remove_block(set, blk)                                                    \
do                                                                                          \
{                                                                                           \
        /* Check: free list must be non empty as it contains the current block. */          \
        assert(BRRIP_DATA_FREE_HEAD(set) && BRRIP_DATA_FREE_TAIL(set));                     \
                                                                                            \
        /* Check: current block must be in invalid state */                                 \
        assert((blk)->state == cache_block_invalid);                                        \
                                                                                            \
        CACHE_REMOVE_FROM_SQUEUE(blk, BRRIP_DATA_FREE_HEAD(set), BRRIP_DATA_FREE_TAIL(set));\
                                                                                            \
        (blk)->next = NULL;                                                                 \
        (blk)->prev = NULL;                                                                 \
                                                                                            \
        /* Reset block state */                                                             \
        (blk)->busy = 0;                                                                    \
        (blk)->cached = 0;                                                                  \
        (blk)->prefetch = 0;                                                                \
}while(0);

void cache_init_brrip(struct cache_params *params, brrip_data *policy_data, 
  brrip_gdata *global_data)
{
    /* For BRRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

    /* Create per rrpv buckets */
    BRRIP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
    BRRIP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC

    /* Set max RRPV for the set */
    BRRIP_DATA_MAX_RRPV(policy_data)    = MAX_RRPV;
    BRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;
    BRRIP_DATA_THRESHOLD(policy_data)   = params->threshold;

    /* Initialize head nodes */
    for (int i = 0; i <= MAX_RRPV; i++)
    {
      BRRIP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
      BRRIP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
      BRRIP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
    }

    /* Create array of blocks */
    BRRIP_DATA_BLOCKS(policy_data) = 
      (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

    BRRIP_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
    assert(BRRIP_DATA_FREE_HLST(policy_data));

    BRRIP_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
    assert(BRRIP_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

    /* Initialize array of blocks */
    BRRIP_DATA_FREE_HEAD(policy_data) = &(BRRIP_DATA_BLOCKS(policy_data)[0]);
    BRRIP_DATA_FREE_TAIL(policy_data) = &(BRRIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

    for (int way = 0; way < params->ways; way++)
    {
      (&BRRIP_DATA_BLOCKS(policy_data)[way])->way   = way;
      (&BRRIP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
      (&BRRIP_DATA_BLOCKS(policy_data)[way])->next  = way ? 
        (&BRRIP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
      (&BRRIP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
        (&BRRIP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
    }

    
    /* Set current and default fill policies */
    BRRIP_DATA_CFPOLICY(policy_data) = cache_policy_brrip;
    BRRIP_DATA_DFPOLICY(policy_data) = cache_policy_brrip;
    BRRIP_DATA_CAPOLICY(policy_data) = cache_policy_brrip;
    BRRIP_DATA_DAPOLICY(policy_data) = cache_policy_brrip;
    BRRIP_DATA_CRPOLICY(policy_data) = cache_policy_brrip;
    BRRIP_DATA_DRPOLICY(policy_data) = cache_policy_brrip;
    
    SAT_CTR_INI(global_data->access_ctr, 8, 0, 255);
    global_data->threshold = params->threshold; 

#undef MAX_RRPV
#undef CACHE_ALLOC
}


/* Free all blocks, sets, head and tail buckets */
void cache_free_brrip(brrip_data *policy_data)
{
  /* Free all data blocks */
  free(BRRIP_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (BRRIP_DATA_VALID_HEAD(policy_data))
  {
    free(BRRIP_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (BRRIP_DATA_VALID_TAIL(policy_data))
  {
    free(BRRIP_DATA_VALID_TAIL(policy_data));
  }
}


struct cache_block_t * cache_find_block_brrip(brrip_data *policy_data, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = BRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head;

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


void cache_fill_block_brrip(brrip_data *policy_data, brrip_gdata *global_data,
  int way, long long tag, enum cache_block_state_t state, int strm,
  memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);
  
  /* Get the current block */
  block = &(BRRIP_DATA_BLOCKS(policy_data)[way]);

  /* Remove block from free list */
  free_list_remove_block(policy_data, block);
  
  /* Update new block state and stream */
  CACHE_UPDATE_BLOCK_STATE(block, tag, info->vtl_addr, state);
  CACHE_UPDATE_BLOCK_STREAM(block, strm);
  block->dirty = (info && info->spill) ? 1 : 0;
  block->epoch = 0;

  /* Get RRPV to be assigned to the new block */
  rrpv = cache_get_fill_rrpv_brrip(policy_data, global_data);

#if 0
  if (info && info->fill == TRUE)
  {
    rrpv = cache_get_fill_rrpv_brrip(policy_data, global_data);
  }
  else
  {
    rrpv = BRRIP_DATA_SPILL_RRPV(policy_data);
  }
#endif  

  if (rrpv != BYPASS_RRPV)
  {
    /* Insert block in to the corresponding RRPV queue */
    CACHE_APPEND_TO_QUEUE(block, 
      BRRIP_DATA_VALID_HEAD(policy_data)[rrpv], 
      BRRIP_DATA_VALID_TAIL(policy_data)[rrpv]);
  }

  BRRIP_DATA_CFPOLICY(policy_data) = BRRIP_DATA_DFPOLICY(policy_data);
}


int cache_replace_block_brrip(brrip_data *policy_data)
{
  struct cache_block_t *block;

  /* Try to find an invalid block. */
  for (block = BRRIP_DATA_FREE_HEAD(policy_data); block; block = block->prev)
  {
    return block->way;
  }

  /* Remove a nonbusy block from the tail */
  unsigned int min_wayid = ~(0);

  /* Obtain RRPV from where to replace the block */
  int rrpv = cache_get_replacement_rrpv_brrip(policy_data);

  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= BRRIP_DATA_MAX_RRPV(policy_data));

  /* If there is no block with required RRPV, increment RRPV of all the blocks
   * until we get one with the required RRPV */
  if (BRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
  {
    CACHE_BRRIP_INCREMENT_RRPV(BRRIP_DATA_VALID_HEAD(policy_data), 
        BRRIP_DATA_VALID_TAIL(policy_data), rrpv);
  }

  switch (BRRIP_DATA_CRPOLICY(policy_data))
  {
    case cache_policy_brrip:
      /* Remove a nonbusy block from the tail */
      for (block = BRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        if (!block->busy && block->way < min_wayid)
          min_wayid = block->way;
      break;

    case cache_policy_cpulast:
      /* First try to find a GPU block */
      for (block = BRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        if (!block->busy && (block->way < min_wayid && block->stream < TST))
          min_wayid = block->way;

      /* If there is no GPU replacement candidate choose a CPU block */
      if (min_wayid == ~(0))
      {
        for (block = BRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid))
            min_wayid = block->way;
        }
      }
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

  BRRIP_DATA_CFPOLICY(policy_data) = BRRIP_DATA_DFPOLICY(policy_data);

  /* If no non busy block can be found, return -1 */
  return (min_wayid != ~(0)) ? min_wayid : -1;
}


void cache_access_block_brrip(brrip_data *policy_data, int way, int strm, memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int old_rrpv;
  int new_rrpv;
  
  blk  = &(BRRIP_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);
  
  switch (BRRIP_DATA_CAPOLICY(policy_data))
  {
    case cache_policy_brrip:
      /* Get old RRPV from the block */
      old_rrpv = (((rrip_list *)(blk->data))->rrpv);
      new_rrpv = old_rrpv;

      if (info && info->fill == TRUE)
      {
        /* Get new RRPV using policy specific function */
        new_rrpv = cache_get_new_rrpv_brrip(old_rrpv);
      }
      else
      {
        new_rrpv = BRRIP_DATA_SPILL_RRPV(policy_data);
      }

      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        CACHE_REMOVE_FROM_QUEUE(blk, BRRIP_DATA_VALID_HEAD(policy_data)[old_rrpv],
            BRRIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, BRRIP_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            BRRIP_DATA_VALID_TAIL(policy_data)[new_rrpv]);
      }

      if (info && (blk->stream == info->stream))
      {
        blk->epoch = (blk->epoch == 3) ? 3 : blk->epoch + 1;
      }
      else
      {
        blk->epoch = 0;
      }

      CACHE_UPDATE_BLOCK_STREAM(blk, strm);
      blk->dirty = (info && info->dirty) ? 1 : 0;
      break;

    case cache_policy_bypass:
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

  BRRIP_DATA_CFPOLICY(policy_data) = BRRIP_DATA_DFPOLICY(policy_data);
}

/* Get fill RRPV */
int cache_get_fill_rrpv_brrip(brrip_data *policy_data, brrip_gdata *global_data)
{
  sb4 new_rrpv;

#define CTR_VAL(data)   (SAT_CTR_VAL(data->access_ctr))
#define THRESHOLD(data) (data->threshold)

  assert(BRRIP_DATA_CFPOLICY(policy_data) == cache_policy_lru ||
    BRRIP_DATA_CFPOLICY(policy_data) == cache_policy_lip      ||
    BRRIP_DATA_CFPOLICY(policy_data) == cache_policy_bypass   ||
    BRRIP_DATA_CFPOLICY(policy_data) == cache_policy_brrip);

  switch (BRRIP_DATA_CFPOLICY(policy_data))
  {
    case cache_policy_lru:
      /* Equivalent to fill at MRU */
      return 0;

    case cache_policy_lip:
      return BRRIP_DATA_MAX_RRPV(policy_data);

    case cache_policy_bypass:
      return BYPASS_RRPV;

    case cache_policy_brrip:
      /* Block is inserted with long reuse prediction value only if counter 
       * value is 0. */
      if (CTR_VAL(global_data) == 0)
      {
        new_rrpv = BRRIP_DATA_MAX_RRPV(policy_data) - 1;
      }
      else
      {
        new_rrpv = BRRIP_DATA_MAX_RRPV(policy_data);
      }

      if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
      {
        /* Increment set access count */
        SAT_CTR_INC(global_data->access_ctr);
      }
      else
      {
        /* Reset access count */
        SAT_CTR_RST(global_data->access_ctr);
      }
      
      return new_rrpv;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }

#undef CTR_VAL
#undef THRESHOLD
}

/* Get replacement candidate RRPV */
int cache_get_replacement_rrpv_brrip(brrip_data *policy_data)
{
  return BRRIP_DATA_MAX_RRPV(policy_data);
}

/* Get new RRPV for the block on hit */
int cache_get_new_rrpv_brrip(int old_rrpv)
{
  return 0;
}

/* Update state of block. */
void cache_set_block_brrip(brrip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  block = &(BRRIP_DATA_BLOCKS(policy_data))[way];

  /* Check: tag matches with the block's tag. */
  assert(block->tag == tag);

  /* Check: block must be in valid state. It is not possible to set
   * state for an invalid block.*/
  assert(block->state != cache_block_invalid);

  if (state != cache_block_invalid)
  {
    /* Assign access stream */
    CACHE_UPDATE_BLOCK_STATE(block, tag, info->vtl_addr, state);
    CACHE_UPDATE_BLOCK_STREAM(block, stream);
    return;
  }

  /* Invalidate block */
  CACHE_UPDATE_BLOCK_STATE(block, tag, info->vtl_addr, cache_block_invalid);
  CACHE_UPDATE_BLOCK_STREAM(block, NN);
  block->epoch = 0;

  /* Get old RRPV from the block */
  int old_rrpv = (((rrip_list *)(block->data))->rrpv);


  /* Remove block from valid list and insert into free list */
  CACHE_REMOVE_FROM_QUEUE(block, BRRIP_DATA_VALID_HEAD(policy_data)[old_rrpv], 
      BRRIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
  CACHE_APPEND_TO_SQUEUE(block, BRRIP_DATA_FREE_HEAD(policy_data), 
      BRRIP_DATA_FREE_TAIL(policy_data));
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_brrip(brrip_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  PTR_ASSIGN(tag_ptr, (BRRIP_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (BRRIP_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (BRRIP_DATA_BLOCKS(policy_data)[way]).stream);

  return BRRIP_DATA_BLOCKS(policy_data)[way];
}

/* Set policy for next fill */
void cache_set_current_fill_policy_brrip(brrip_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip 
    || policy == cache_policy_bypass);

  BRRIP_DATA_CFPOLICY(policy_data) = policy;
}

/* Set policy for next access */
void cache_set_current_access_policy_brrip(brrip_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_bypass);

  BRRIP_DATA_CAPOLICY(policy_data) = policy;
}

/* Set policy for next replacement */
void cache_set_current_replacement_policy_brrip(brrip_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_cpulast);

  BRRIP_DATA_CRPOLICY(policy_data) = policy;
}
