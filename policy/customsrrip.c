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
#include "customsrrip.h"
#include "sap.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))
#define BYPASS_RRPV             (-1)

#define PSEL_WIDTH              (20)
#define PSEL_MIN_VAL            (0x00)  
#define PSEL_MAX_VAL            (0xfffff)  
#define PSEL_MID_VAL            (1 << 19)

#define CACHE_UPDATE_BLOCK_STATE(block, tag, va, state_in)          \
do                                                                  \
{                                                                   \
  (block)->tag      = (tag);                                        \
  (block)->vtl_addr = (va);                                         \
  (block)->state    = (state_in);                                   \
}while(0)

#define CACHE_CUSTOMSRRIP_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)  \
do                                                                  \
{                                                                   \
  int dif = 0;                                                      \
                                                                    \
  for (int i = rrpv - 1; i >= 0; i--)                               \
  {                                                                 \
    assert(i <= rrpv);                                              \
                                                                    \
    if ((head_ptr)[i].head)                                         \
    {                                                               \
      if (!dif)                                                     \
      {                                                             \
        dif = rrpv - i;                                             \
      }                                                             \
                                                                    \
      assert((tail_ptr)[i].head);                                   \
      (head_ptr)[i + dif].head  = (head_ptr)[i].head;               \
      (tail_ptr)[i + dif].head  = (tail_ptr)[i].head;               \
      (head_ptr)[i].head        = NULL;                             \
      (tail_ptr)[i].head        = NULL;                             \
                                                                    \
      struct cache_block_t *node = (head_ptr)[i + dif].head;        \
                                                                    \
      /* point data to new RRPV head */                             \
      for (; node; node = node->prev)                               \
      {                                                             \
        node->data = &(head_ptr[i + dif]);                          \
      }                                                             \
    }                                                               \
    else                                                            \
    {                                                               \
      assert(!((tail_ptr)[i].head));                                \
    }                                                               \
  }                                                                 \
}while(0)

#define CACHE_CUSTOMSRRIP_SINCREMENT_RRPV(head_ptr, tail_ptr, rrpv, p, g) \
do                                                                  \
{                                                                   \
  int dif = 0;                                                      \
  struct cache_block_t *node;                                       \
                                                                    \
  for (int i = rrpv - 1; i >= 0; i--)                               \
  {                                                                 \
    assert(i <= rrpv);                                              \
                                                                    \
    if ((head_ptr)[i].head)                                         \
    {                                                               \
      if (!dif)                                                     \
      {                                                             \
        dif = rrpv - i;                                             \
      }                                                             \
                                                                    \
      node = (head_ptr)[i].head;                                    \
                                                                    \
      /* Insert block in to the corresponding RRPV queue */         \
      CACHE_REMOVE_FROM_QUEUE(node, head_ptr[i], tail_ptr[i]);      \
      CACHE_APPEND_TO_QUEUE(node, head_ptr[i + dif], tail_ptr[i + dif]);\
                                                                    \
      node->data = &(head_ptr[i + dif]);                            \
    }                                                               \
    else                                                            \
    {                                                               \
      assert(!((tail_ptr)[i].head));                                \
    }                                                               \
  }                                                                 \
}while(0)

#define CACHE_GET_BLOCK_STREAM(block, strm)                         \
do                                                                  \
{                                                                   \
  strm = (block)->stream;                                           \
}while(0)

#define CACHE_UPDATE_BLOCK_STREAM(block, strm)                      \
  do                                                                \
{                                                                   \
  (block)->stream = strm;                                           \
}while(0)

/*
 * Public Variables
 */

/*
 * Private Functions
 */
#define free_list_remove_block(set, blk)                                                    \
  do                                                                                        \
{                                                                                           \
  /* Check: free list must be non empty as it contains the current block. */                \
  assert(CUSTOMSRRIP_DATA_FREE_HEAD(set) && CUSTOMSRRIP_DATA_FREE_HEAD(set));               \
                                                                                            \
  /* Check: current block must be in invalid state */                                       \
  assert((blk)->state == cache_block_invalid);                                              \
                                                                                            \
  CACHE_REMOVE_FROM_SQUEUE(blk, CUSTOMSRRIP_DATA_FREE_HEAD(set), CUSTOMSRRIP_DATA_FREE_TAIL(set));\
                                                                                            \
  (blk)->next = NULL;                                                                       \
  (blk)->prev = NULL;                                                                       \
                                                                                            \
  /* Reset block state */                                                                   \
  (blk)->busy     = 0;                                                                      \
  (blk)->cached   = 0;                                                                      \
  (blk)->prefetch = 0;                                                                      \
}while(0);


int customsrrip_get_min_wayid_from_head(struct cache_block_t *head)
{
  struct cache_block_t *node;
  int    min_wayid = 0xff;

  assert(head);

  node = head;

  while (node)
  {
    if (node->way < min_wayid)
    {
      min_wayid = node->way;
    }

    node = node->prev;
  }

  return min_wayid;
}

struct cache_block_t* customsrrip_get_minwayid_block(rrip_list *head_ptr,
        rrip_list *tail_ptr, sb4 rrpv, customsrrip_data *p, 
        customsrrip_gdata *g)
{
  struct  cache_block_t *ret_node;
  int     min_wayid;

  assert(head_ptr[rrpv].head);

  /* Obtain an minwayid block */
  min_wayid = customsrrip_get_min_wayid_from_head(head_ptr[rrpv].head);
  ret_node  = &((p)->blocks[min_wayid]);

  return ret_node;
}

void cache_init_customsrrip(ub4 set_indx, struct cache_params *params, customsrrip_data *policy_data, 
    customsrrip_gdata *global_data)
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
        global_data->epoch_ctfctr = NULL;
        global_data->epoch_cthctr = NULL;
        global_data->epoch_btfctr = NULL;
        global_data->epoch_bthctr = NULL;
        global_data->epoch_valid  = NULL;
      }

      global_data->fill_rrpv[s] = params->max_rrpv - 1;
    }

    global_data->epoch_count  = EPOCH_COUNT;
    global_data->max_epoch    = MAX_EPOCH;
    global_data->max_rrpv     = params->max_rrpv;
    global_data->use_ct_hint  = FALSE;
    global_data->use_bt_hint  = FALSE;
  }

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  CUSTOMSRRIP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  CUSTOMSRRIP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC  

  assert(CUSTOMSRRIP_DATA_VALID_HEAD(policy_data));
  assert(CUSTOMSRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  CUSTOMSRRIP_DATA_MAX_RRPV(policy_data)    = MAX_RRPV;
  CUSTOMSRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;
  CUSTOMSRRIP_DATA_USE_EPOCH(policy_data)   = FALSE;

  assert(params->spill_rrpv <= MAX_RRPV);

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    CUSTOMSRRIP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    CUSTOMSRRIP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    CUSTOMSRRIP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  CUSTOMSRRIP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

  CUSTOMSRRIP_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(CUSTOMSRRIP_DATA_FREE_HLST(policy_data));

  CUSTOMSRRIP_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(CUSTOMSRRIP_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  CUSTOMSRRIP_DATA_FREE_HEAD(policy_data) = &(CUSTOMSRRIP_DATA_BLOCKS(policy_data)[0]);
  CUSTOMSRRIP_DATA_FREE_TAIL(policy_data) = &(CUSTOMSRRIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

  /* Set current and default fill policy to CUSTOMSRRIP */
  CUSTOMSRRIP_DATA_CFPOLICY(policy_data) = cache_policy_customsrrip;
  CUSTOMSRRIP_DATA_DFPOLICY(policy_data) = cache_policy_customsrrip;
  CUSTOMSRRIP_DATA_CAPOLICY(policy_data) = cache_policy_customsrrip;
  CUSTOMSRRIP_DATA_DAPOLICY(policy_data) = cache_policy_customsrrip;
  CUSTOMSRRIP_DATA_CRPOLICY(policy_data) = cache_policy_customsrrip;
  CUSTOMSRRIP_DATA_DRPOLICY(policy_data) = cache_policy_customsrrip;

  assert(CUSTOMSRRIP_DATA_MAX_RRPV(policy_data) != 0);

#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_customsrrip(customsrrip_data *policy_data)
{
  /* Free all data blocks */
  free(CUSTOMSRRIP_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (CUSTOMSRRIP_DATA_VALID_HEAD(policy_data))
  {
    free(CUSTOMSRRIP_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (CUSTOMSRRIP_DATA_VALID_TAIL(policy_data))
  {
    free(CUSTOMSRRIP_DATA_VALID_TAIL(policy_data));
  }
}

struct cache_block_t* cache_find_block_customsrrip(customsrrip_data *policy_data, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = CUSTOMSRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head;

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

void cache_fill_block_customsrrip(customsrrip_data *policy_data, customsrrip_gdata *global_data,
    int way, long long tag, enum cache_block_state_t state, int strm, 
    memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain CUSTOMSRRIP specific data */
  block = &(CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way]);
  assert(block);

  block->epoch        = 0;
  block->access       = 0;
  block->is_ct_block  = FALSE;
  block->is_bt_block  = FALSE;

  assert(block->stream == 0);

  /* Get RRPV to be assigned to the new block */

  rrpv = cache_get_fill_rrpv_customsrrip(policy_data, global_data, info, block->epoch);

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

    /* Update source stream, this is done only for fill */
    block->src_stream = info->stream;

    block->dirty      = (info && info->spill) ? 1 : 0;
    block->last_rrpv  = rrpv;
    block->fill_rrpv  = rrpv;

    /* Insert block in to the corresponding RRPV queue */
    CACHE_APPEND_TO_QUEUE(block, 
        CUSTOMSRRIP_DATA_VALID_HEAD(policy_data)[rrpv], 
        CUSTOMSRRIP_DATA_VALID_TAIL(policy_data)[rrpv]);
  }

  CUSTOMSRRIP_DATA_CFPOLICY(policy_data) = CUSTOMSRRIP_DATA_DFPOLICY(policy_data);
}

int cache_replace_block_customsrrip(customsrrip_data *policy_data, 
    customsrrip_gdata *global_data, ub1 last_stream)
{
  struct cache_block_t *block;
  int    rrpv;

  /* Remove a nonbusy block from the tail */
  unsigned int min_wayid;
  unsigned int sa_min_wayid;

  min_wayid     = ~(0);
  sa_min_wayid  = ~(0);

  /* Try to find an invalid block always from head of the free list. */
  for (block = CUSTOMSRRIP_DATA_FREE_HEAD(policy_data); block; block = block->prev)
  {
    return block->way;
  }

  /* Obtain RRPV from where to replace the block */
  rrpv = cache_get_replacement_rrpv_customsrrip(policy_data);

  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= CUSTOMSRRIP_DATA_MAX_RRPV(policy_data));

  if (min_wayid == ~(0))
  {
    /* If there is no block with required RRPV, increment RRPV of all the blocks
     * until we get one with the required RRPV */
    if (CUSTOMSRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
    {
      CACHE_CUSTOMSRRIP_INCREMENT_RRPV(CUSTOMSRRIP_DATA_VALID_HEAD(policy_data), 
          CUSTOMSRRIP_DATA_VALID_TAIL(policy_data), rrpv);
#if 0
      CACHE_CUSTOMSRRIP_SINCREMENT_RRPV(CUSTOMSRRIP_DATA_VALID_HEAD(policy_data), 
          CUSTOMSRRIP_DATA_VALID_TAIL(policy_data), rrpv, policy_data, global_data);
#endif
    }

    switch (CUSTOMSRRIP_DATA_CRPOLICY(policy_data))
    {
      case cache_policy_customsrrip:
        for (block = CUSTOMSRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && block->way < min_wayid)
            min_wayid = block->way;

          if (!block->busy && block->way < sa_min_wayid && block->stream != last_stream)
          {
            sa_min_wayid = block->way;
          }
        }

        if (sa_min_wayid != ~(0))
        {
          min_wayid = sa_min_wayid;
        }
        break;

      case cache_policy_cpulast:
        /* First try to find a GPU block */
        for (block = CUSTOMSRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid && block->stream < TST))
            min_wayid = block->way;
        }

        /* If there so no GPU replacement candidate, replace CPU block */
        if (min_wayid == ~(0))
        {
          for (block = CUSTOMSRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
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

void cache_access_block_customsrrip(customsrrip_data *policy_data,
    customsrrip_gdata *global_data, int way, int strm, memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int old_rrpv;
  int new_rrpv;

  blk  = &(CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way]);
  assert(blk);

  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);

  switch (CUSTOMSRRIP_DATA_CAPOLICY(policy_data))
  {
    case cache_policy_customsrrip:
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
        
        /* Set inter-stream bits */
        if (blk->stream == CS && info->stream == TS)
        {
          blk->is_ct_block = TRUE;
        }
        
        if (blk->stream == BS && info->stream == TS)
        {
          blk->is_bt_block = TRUE;
        }
      }
#if 0
      if (global_data->use_ct_hint)
      {
        /* Get new RRPV using policy specific function */
        if (blk->stream == CS && info->stream == TS)
        {
          new_rrpv = CUSTOMSRRIP_DATA_MAX_RRPV(policy_data);
        }
        else
        {
          if (info->stream == TS)
          {
            new_rrpv = CUSTOMSRRIP_DATA_MAX_RRPV(policy_data) - 1;
          }
          else
          {
            new_rrpv = cache_get_new_rrpv_customsrrip(policy_data, global_data, info, 
              blk->epoch, old_rrpv);
          }
        }
      }
      else
#endif
      {
        new_rrpv = cache_get_new_rrpv_customsrrip(blk, policy_data, global_data, info, 
            blk->epoch, old_rrpv);
      }

      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        blk->last_rrpv = old_rrpv;

        CACHE_REMOVE_FROM_QUEUE(blk, CUSTOMSRRIP_DATA_VALID_HEAD(policy_data)[old_rrpv],
            CUSTOMSRRIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, CUSTOMSRRIP_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            CUSTOMSRRIP_DATA_VALID_TAIL(policy_data)[new_rrpv]);
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

int cache_get_fill_rrpv_customsrrip(customsrrip_data *policy_data, 
    customsrrip_gdata *global_data, memory_trace *info, ub4 epoch)
{
  sb4 ret_rrpv;

  switch (CUSTOMSRRIP_DATA_CFPOLICY(policy_data))
  {
    case cache_policy_lru:
      return 0;

    case cache_policy_lip:
      return CUSTOMSRRIP_DATA_MAX_RRPV(policy_data);

    case cache_policy_customsrrip:
#define VALID_EPOCH(g, i)  ((g)->epoch_valid && (g)->epoch_valid[(i)->stream])

      /* If epoch counter are valid use them to decide fill RRPV */
      if (policy_data->use_epoch && VALID_EPOCH(global_data, info))
      {
        assert(global_data->epoch_fctr[info->stream]);
        assert(global_data->epoch_hctr[info->stream]);

#define FILL_VAL(g, i, e)  (SAT_CTR_VAL((g)->epoch_fctr[(i)->stream][e]))
#define HIT_VAL(g, i, e)   (SAT_CTR_VAL((g)->epoch_hctr[(i)->stream][e]))
#define HIT_PROB(g, i, e)  ((float)HIT_VAL(g, i, e) / FILL_VAL(g, i, e))

        if (FILL_VAL(global_data, info, epoch) > 32 * HIT_VAL(global_data, info, epoch))
        {
          ret_rrpv = CUSTOMSRRIP_DATA_MAX_RRPV(policy_data);
          global_data->thrasher_fdemote[info->stream] += 1;
        }
        else
        {
          ret_rrpv = CUSTOMSRRIP_DATA_MAX_RRPV(policy_data) - 1;
        }

#undef FILL_VAL
#undef HIT_VAL
#undef HIT_PROB
      }
      else
      {
        ret_rrpv =  global_data->fill_rrpv[info->stream];
      }
#if 0
      if (global_data->use_ct_hint)
      {
        if (info->stream != CS && info->stream != PS && info->stream != TS)
        {
          ret_rrpv = CUSTOMSRRIP_DATA_MAX_RRPV(policy_data);
        }
      }
#endif
      return ret_rrpv;

#undef VALID_EPOCH

    case cache_policy_bypass:
      /* Not to insert */
      return BYPASS_RRPV;  

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }
}

int cache_get_replacement_rrpv_customsrrip(customsrrip_data *policy_data)
{
  return CUSTOMSRRIP_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_customsrrip(struct cache_block_t *block, 
    customsrrip_data *policy_data, customsrrip_gdata *global_data, 
    memory_trace *info, ub4 epoch, ub4 old_rrpv)
{
  sb4 ret_rrpv;
  
  ret_rrpv = info->spill ? old_rrpv : 0;

#define VALID_EPOCH(g, i) ((g)->epoch_valid && (g)->epoch_valid[(i)->stream])

  /* If epoch counter is valid use epoch based DBP */
  if (policy_data->use_epoch && VALID_EPOCH(global_data, info))
  {
    assert(global_data->epoch_fctr[info->stream]);
    assert(global_data->epoch_hctr[info->stream]);
    assert(global_data->epoch_ctfctr);
    assert(global_data->epoch_cthctr);

#define CT(b)                 ((b)->is_ct_block)
#define CTFILL_VAL(g, e)      (SAT_CTR_VAL((g)->epoch_ctfctr[e]))
#define SFILL_VAL(g, i, e)    (SAT_CTR_VAL((g)->epoch_fctr[(i)->stream][e]))
#define FILL_VAL(b, g, i, e)  (CT(b) ? CTFILL_VAL(g, e) : SFILL_VAL(g, i, e))
#define CTHIT_VAL(g, e)       (SAT_CTR_VAL((g)->epoch_cthctr[e]))
#define SHIT_VAL(g, i, e)     (SAT_CTR_VAL((g)->epoch_hctr[(i)->stream][e]))
#define HIT_VAL(b, g, i, e)   (CT(b) ? CTHIT_VAL(g, e) : SHIT_VAL(g, i, e))
#define HIT_PROB(b, g, i, e)  ((float)HIT_VAL(b, g, i, e) / FILL_VAL(b, g, i, e))
#define STHR(b, g, i, e)      (32 * HIT_VAL(b, g, i, e))
#define CTTHR(b, g, i, e)     (32 * HIT_VAL(b, g, i, e))
#define THR_VAL(b, g, i, e)   (CT(b) ? CTTHR(b, g, i, e) : STHR(b, g, i, e))

#if 0
    if (get_prob_in_range(0.0F, HIT_PROB(block, global_data, info, epoch)) == FALSE)
#endif

    if (FILL_VAL(block, global_data, info, epoch) > THR_VAL(block, global_data, info, epoch))
    {
      ret_rrpv = CUSTOMSRRIP_DATA_MAX_RRPV(policy_data);
      global_data->thrasher_hdemote[info->stream] += 1;
    }

#undef CT
#undef CTFILL_VAL
#undef SFILL_VAL
#undef FILL_VAL
#undef CTHIT_VAL
#undef SHIT_VAL
#undef HIT_VAL
#undef HIT_PROB
#undef STHR
#undef CTTHR
#undef THR_VAL
  }

#undef VALID_EPOCH

  return ret_rrpv;
}

/* Update state of block. */
void cache_set_block_customsrrip(customsrrip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  block = &(CUSTOMSRRIP_DATA_BLOCKS(policy_data))[way];

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
  CACHE_REMOVE_FROM_QUEUE(block, CUSTOMSRRIP_DATA_VALID_HEAD(policy_data)[old_rrpv],
      CUSTOMSRRIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
  CACHE_APPEND_TO_SQUEUE(block, CUSTOMSRRIP_DATA_FREE_HEAD(policy_data), 
      CUSTOMSRRIP_DATA_FREE_TAIL(policy_data));

}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_customsrrip(customsrrip_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  assert(policy_data);
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way]).stream);

  return CUSTOMSRRIP_DATA_BLOCKS(policy_data)[way];
}

/* Get tag and state of a block. */
int cache_count_block_customsrrip(customsrrip_data *policy_data, ub1 strm)
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
    head = CUSTOMSRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head;

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
void cache_set_stream_fill_rrpv_customsrrip(customsrrip_gdata *global_data, 
    ub1 stream, ub4 rrpv)
{
  assert(stream > NN  && stream <= TST);

  global_data->fill_rrpv[stream] = rrpv;
}

#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
