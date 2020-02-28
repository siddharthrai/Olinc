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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <assert.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "srripbypass.h"
#include "sap.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))
#define BYPASS_RRPV             (-1)
#define SPEEDUP(n)              (n == srripbypass_stream_x)
#define VALID_BYPASS(g, i)      (SAT_CTR_VAL((g)->srripbypass_hint[(i)->stream]) == 0)
#define INTERVAL_SIZE           (256 * 1024)

#define PSEL_WIDTH              (20)
#define PSEL_MIN_VAL            (0x00)  
#define PSEL_MAX_VAL            (0xfffff)  
#define PSEL_MID_VAL            (1 << 19)

#define CACHE_UPDATE_BLOCK_STATE(block, tag, va, state_in)    \
do                                                            \
{                                                             \
  (block)->tag      = (tag);                                  \
  (block)->vtl_addr = (va);                                   \
  (block)->state    = (state_in);                             \
}while(0)

#define CACHE_SRRIPBYPASS_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)  \
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
        assert(SRRIPBYPASS_DATA_FREE_HEAD(set) && SRRIPBYPASS_DATA_FREE_HEAD(set));         \
                                                                                            \
        /* Check: current block must be in invalid state */                                 \
        assert((blk)->state == cache_block_invalid);                                        \
                                                                                            \
        CACHE_REMOVE_FROM_SQUEUE(blk, SRRIPBYPASS_DATA_FREE_HEAD(set), SRRIPBYPASS_DATA_FREE_TAIL(set));\
                                                                                            \
        (blk)->next = NULL;                                                                 \
        (blk)->prev = NULL;                                                                 \
                                                                                            \
        /* Reset block state */                                                             \
        (blk)->busy = 0;                                                                    \
        (blk)->cached = 0;                                                                  \
        (blk)->prefetch = 0;                                                                \
}while(0);


/* Returns TRUE if random number falls in range [lo, hi] */
static ub1 get_prob_in_range(uf8 lo, uf8 hi)
{
  uf8 val;
  ub1 ret;

  val = (uf8)random() / RAND_MAX;
  
  if (lo == 1.0F && hi == 1.0F)
  {
    ret = TRUE;
  }
  else
  {
    if (lo == 0.0F && hi == 0.0F)
    {
      ret = FALSE;
    }
    else
    {
      if (val >= lo && val < hi)
      {
        ret = TRUE;
      }
      else
      {
        ret = FALSE;
      }
    }
  }

  return ret;
}

/* Returns SAPPRIORITY stream corresponding to access stream based on stream 
 * classification algoritm. */
srripbypass_stream get_srripbypass_stream(memory_trace *info)
{
  return (srripbypass_stream)(info->sap_stream);
}

/* Public interface to initialize SAP statistics */
void cache_init_srripbypass(ub4 set_indx, struct cache_params *params, srripbypass_data *policy_data, 
    srripbypass_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  
  if (set_indx == 0)
  {
    for (ub1 s = NN; s <= TST; s++)
    {
      for (ub1 i = 0; i < EPOCH_COUNT; i++)
      {
        /* Initialize epoch fill counter */
        global_data->epoch_fctr   = NULL;
        global_data->epoch_hctr   = NULL;
        global_data->epoch_valid  = NULL;
      }

      SAT_CTR_INI(global_data->srripbypass_hint[s], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    }

    global_data->epoch_count    = EPOCH_COUNT;
    global_data->max_epoch      = MAX_EPOCH;
    global_data->access_count   = 0;
    global_data->speedup_stream = NN;
  }

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  SRRIPBYPASS_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SRRIPBYPASS_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC  

  assert(SRRIPBYPASS_DATA_VALID_HEAD(policy_data));
  assert(SRRIPBYPASS_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIPBYPASS_DATA_MAX_RRPV(policy_data)    = MAX_RRPV;
  SRRIPBYPASS_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;
  SRRIPBYPASS_DATA_USE_EPOCH(policy_data)   = FALSE;

  assert(params->spill_rrpv <= MAX_RRPV);

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SRRIPBYPASS_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    SRRIPBYPASS_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    SRRIPBYPASS_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  SRRIPBYPASS_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

  SRRIPBYPASS_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(SRRIPBYPASS_DATA_FREE_HLST(policy_data));

  SRRIPBYPASS_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(SRRIPBYPASS_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  SRRIPBYPASS_DATA_FREE_HEAD(policy_data) = &(SRRIPBYPASS_DATA_BLOCKS(policy_data)[0]);
  SRRIPBYPASS_DATA_FREE_TAIL(policy_data) = &(SRRIPBYPASS_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SRRIPBYPASS_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SRRIPBYPASS_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SRRIPBYPASS_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SRRIPBYPASS_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SRRIPBYPASS_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SRRIPBYPASS_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }
  
  /* Set current and default fill policy to SRRIPBYPASS */
  SRRIPBYPASS_DATA_CFPOLICY(policy_data) = cache_policy_srripbypass;
  SRRIPBYPASS_DATA_DFPOLICY(policy_data) = cache_policy_srripbypass;
  SRRIPBYPASS_DATA_CAPOLICY(policy_data) = cache_policy_srripbypass;
  SRRIPBYPASS_DATA_DAPOLICY(policy_data) = cache_policy_srripbypass;
  SRRIPBYPASS_DATA_CRPOLICY(policy_data) = cache_policy_srripbypass;
  SRRIPBYPASS_DATA_DRPOLICY(policy_data) = cache_policy_srripbypass;
  
  assert(SRRIPBYPASS_DATA_MAX_RRPV(policy_data) != 0);

#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_srripbypass(srripbypass_data *policy_data)
{
  /* Free all data blocks */
  free(SRRIPBYPASS_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (SRRIPBYPASS_DATA_VALID_HEAD(policy_data))
  {
    free(SRRIPBYPASS_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SRRIPBYPASS_DATA_VALID_TAIL(policy_data))
  {
    free(SRRIPBYPASS_DATA_VALID_TAIL(policy_data));
  }
}

static void cache_update_interval_end(srripbypass_gdata *global_data)
{
  ub8 max_speedup;
  
  max_speedup = 0;
  global_data->speedup_stream = NN;

  for (ub1 s = NN; s <= TST; s++)
  {
    if (SAT_CTR_VAL(global_data->srripbypass_hint[s]) > max_speedup)
    {
       global_data->speedup_stream = s;
       max_speedup = SAT_CTR_VAL(global_data->srripbypass_hint[s]);

       SAT_CTR_RST(global_data->srripbypass_hint[s]);
    }
  }
}

static void cache_update_hint_count(srripbypass_gdata *global_data, memory_trace *info)
{
  assert(info->stream <= TST);
  
  if (SPEEDUP(get_srripbypass_stream(info)))
  {
    SAT_CTR_INC(global_data->srripbypass_hint[info->stream]);
  }
}

struct cache_block_t* cache_find_block_srripbypass(srripbypass_data *policy_data, 
    srripbypass_gdata *global_data, memory_trace *info, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SRRIPBYPASS_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

end:
  /* Update speedup hint */
  cache_update_hint_count(global_data, info);

  if (++(global_data->access_count) >= INTERVAL_SIZE)
  {
    cache_update_interval_end(global_data);
  }

  return node;
}

void cache_fill_block_srripbypass(srripbypass_data *policy_data, srripbypass_gdata *global_data,
    int way, long long tag, enum cache_block_state_t state, int strm, 
    memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain SRRIPBYPASS specific data */
  block = &(SRRIPBYPASS_DATA_BLOCKS(policy_data)[way]);
  block->epoch  = 0;
  block->access = 0;
  
  assert(block->stream == 0);

  /* Get RRPV to be assigned to the new block */

  rrpv = cache_get_fill_rrpv_srripbypass(policy_data, global_data, info, block->epoch);

  /* If block is not bypassed */
  if (rrpv != BYPASS_RRPV)
  {
    /* Ensure a valid RRPV */
    assert(rrpv >= 0 && rrpv <= policy_data->max_rrpv); 

    /* Remove block from free list */
    free_list_remove_block(policy_data, block);

    /* Update new block state and stream */
    CACHE_UPDATE_BLOCK_STATE(block, tag, info->vtl_addr, state);
    CACHE_UPDATE_BLOCK_STREAM(block, strm);
    block->dirty      = (info && info->spill) ? 1 : 0;
    block->last_rrpv  = rrpv;

    /* Insert block in to the corresponding RRPV queue */
    CACHE_APPEND_TO_QUEUE(block, 
        SRRIPBYPASS_DATA_VALID_HEAD(policy_data)[rrpv], 
        SRRIPBYPASS_DATA_VALID_TAIL(policy_data)[rrpv]);
  }

  SRRIPBYPASS_DATA_CFPOLICY(policy_data) = SRRIPBYPASS_DATA_DFPOLICY(policy_data);
}

int cache_replace_block_srripbypass(srripbypass_data *policy_data, srripbypass_gdata *global_data)
{
  struct cache_block_t *block;
  int    rrpv;

  /* Remove a nonbusy block from the tail */
  unsigned int min_wayid = ~(0);

  /* Try to find an invalid block always from head of the free list. */
  for (block = SRRIPBYPASS_DATA_FREE_HEAD(policy_data); block; block = block->prev)
  {
    return block->way;
  }
  
  /* Obtain RRPV from where to replace the block */
  rrpv = cache_get_replacement_rrpv_srripbypass(policy_data);

  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= SRRIPBYPASS_DATA_MAX_RRPV(policy_data));

  if (min_wayid == ~(0))
  {
    /* If there is no block with required RRPV, increment RRPV of all the blocks
     * until we get one with the required RRPV */
    if (SRRIPBYPASS_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
    {
      CACHE_SRRIPBYPASS_INCREMENT_RRPV(SRRIPBYPASS_DATA_VALID_HEAD(policy_data), 
          SRRIPBYPASS_DATA_VALID_TAIL(policy_data), rrpv);
    }

    switch (SRRIPBYPASS_DATA_CRPOLICY(policy_data))
    {
      case cache_policy_srripbypass:
        for (block = SRRIPBYPASS_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && block->way < min_wayid)
            min_wayid = block->way;
        }
        break;

      case cache_policy_cpulast:
        /* First try to find a GPU block */
        for (block = SRRIPBYPASS_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid && block->stream < TST))
            min_wayid = block->way;
        }

        /* If there so no GPU replacement candidate, replace CPU block */
        if (min_wayid == ~(0))
        {
          for (block = SRRIPBYPASS_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
          {
            if (!block->busy && (block->way < min_wayid))
              min_wayid = block->way;
          }
        }
        break;

      default:
        panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
    }
  }
  
  /* If no non busy block can be found, return -1 */
  return (min_wayid != ~(0)) ? min_wayid : -1;
}

void cache_access_block_srripbypass(srripbypass_data *policy_data, srripbypass_gdata *global_data,
    int way, int strm, memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int old_rrpv;
  int new_rrpv;
  
  blk  = &(SRRIPBYPASS_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);
  
  switch (SRRIPBYPASS_DATA_CAPOLICY(policy_data))
  {
    case cache_policy_srripbypass:
      /* Get old RRPV from the block */
      old_rrpv = (((rrip_list *)(blk->data))->rrpv);
      new_rrpv = old_rrpv;

      if (info && blk->stream == info->stream)
      {
#define MX_EP(g)  ((g)->max_epoch)

        blk->epoch  = (blk->epoch == MX_EP(global_data)) ? MX_EP(global_data) : blk->epoch + 1;

#undef MX_EP
      }
      else
      {
        blk->epoch = 0;
      }

      /* Get new RRPV using policy specific function */
      new_rrpv = cache_get_new_rrpv_srripbypass(policy_data, global_data, info, 
          old_rrpv, blk->epoch);
    
      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        blk->last_rrpv = new_rrpv;

        CACHE_REMOVE_FROM_QUEUE(blk, SRRIPBYPASS_DATA_VALID_HEAD(policy_data)[old_rrpv],
            SRRIPBYPASS_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, SRRIPBYPASS_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            SRRIPBYPASS_DATA_VALID_TAIL(policy_data)[new_rrpv]);
      }

      CACHE_UPDATE_BLOCK_STREAM(blk, strm);

      blk->dirty   = (info && info->dirty) ? 1 : 0;
      blk->access += 1;
      break;

    case cache_policy_bypass:
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }
}

int cache_get_fill_rrpv_srripbypass(srripbypass_data *policy_data, 
    srripbypass_gdata *global_data, memory_trace *info, ub4 epoch)
{
  sb4 ret_rrpv;

  switch (SRRIPBYPASS_DATA_CFPOLICY(policy_data))
  {
    case cache_policy_lru:
      return 0;

    case cache_policy_lip:
      return SRRIPBYPASS_DATA_MAX_RRPV(policy_data);

    case cache_policy_srripbypass:
      /* Based on speedup decide bypass */
#if 0
      if (info->stream != PS && info->stream != BS && info->stream != global_data->speedup_stream)
      if (info->stream != CS && VALID_BYPASS(global_data, info))
      if (info->stream == PS)
#endif
      if (info->stream != PS)
      {
        if (get_prob_in_range(0.0, 0.90))
        {
          return BYPASS_RRPV; 
        }
        else
        {
          return SRRIPBYPASS_DATA_MAX_RRPV(policy_data) - 1;
        }
      }
      else
      {
        return SRRIPBYPASS_DATA_MAX_RRPV(policy_data) - 1;
      }

    case cache_policy_bypass:
      /* Not to insert */
      return BYPASS_RRPV;  

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }
}

int cache_get_replacement_rrpv_srripbypass(srripbypass_data *policy_data)
{
  return SRRIPBYPASS_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_srripbypass(srripbypass_data *policy_data, srripbypass_gdata *global_data, 
    memory_trace *info, sb1 old_rrpv, ub4 epoch)
{
  sb4 ret_rrpv;

  ret_rrpv = 0;

#define VALID_EPOCH(g, i) ((g)->epoch_valid && (g)->epoch_valid[(i)->stream])

  /* If epoch counter is valid use epoch based DBP */
  if (policy_data->use_epoch && VALID_EPOCH(global_data, info))
  {
    assert(global_data->epoch_fctr[info->stream]);
    assert(global_data->epoch_hctr[info->stream]);

#define FILL_VAL(g, i, e)  (SAT_CTR_VAL((g)->epoch_fctr[(i)->stream][e]))
#define HIT_VAL(g, i, e)   (SAT_CTR_VAL((g)->epoch_hctr[(i)->stream][e]))

    if (FILL_VAL(global_data, info, epoch) > 32 * HIT_VAL(global_data, info, epoch))
    {
      ret_rrpv = SRRIPBYPASS_DATA_MAX_RRPV(policy_data);
    }

#undef FILL_VAL
#undef HIT_VAL
  }
  
  /* For spill move only block at RRPV 3 to 2 */
  if (info && info->spill)
  {
    if (old_rrpv == SRRIPBYPASS_DATA_MAX_RRPV(policy_data))
    {
      ret_rrpv = old_rrpv - 1;
    }
    else
    {
      ret_rrpv = old_rrpv;
    }
  }

#undef VALID_EPOCH

  return ret_rrpv;
}

/* Update state of block. */
void cache_set_block_srripbypass(srripbypass_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  block = &(SRRIPBYPASS_DATA_BLOCKS(policy_data))[way];

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
  block->epoch  = 0;

  /* Get old RRPV from the block */
  int old_rrpv = (((rrip_list *)(block->data))->rrpv);

  /* Remove block from valid list and insert into free list */
  CACHE_REMOVE_FROM_QUEUE(block, SRRIPBYPASS_DATA_VALID_HEAD(policy_data)[old_rrpv],
      SRRIPBYPASS_DATA_VALID_TAIL(policy_data)[old_rrpv]);
  CACHE_APPEND_TO_SQUEUE(block, SRRIPBYPASS_DATA_FREE_HEAD(policy_data), 
      SRRIPBYPASS_DATA_FREE_TAIL(policy_data));

}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_srripbypass(srripbypass_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  assert(policy_data);
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (SRRIPBYPASS_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (SRRIPBYPASS_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (SRRIPBYPASS_DATA_BLOCKS(policy_data)[way]).stream);

  return SRRIPBYPASS_DATA_BLOCKS(policy_data)[way];
}

/* Get tag and state of a block. */
int cache_count_block_srripbypass(srripbypass_data *policy_data, ub1 strm)
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
    head = SRRIPBYPASS_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);
      if (node->stream == strm)
        count++;
    }
  }

  return count;
}

/* Set policy for next fill */
void cache_set_current_fill_policy_srripbypass(srripbypass_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  SRRIPBYPASS_DATA_CFPOLICY(policy_data) = policy;
}

/* Set policy for next access */
void cache_set_current_access_policy_srripbypass(srripbypass_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  SRRIPBYPASS_DATA_CAPOLICY(policy_data) = policy;
}

/* Set policy for next replacment */
void cache_set_current_replacement_policy_srripbypass(srripbypass_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_cpulast);

  SRRIPBYPASS_DATA_CRPOLICY(policy_data) = policy;
}

#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
#undef VALID_BYPASS
