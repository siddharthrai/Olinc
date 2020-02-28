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
#include "srripbs.h"

#define SRRIPBS_SAMPLED_SET   (0)
#define SRRIPBS_FOLLOWER_SET  (1)

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))
#define BYPASS_RRPV             (-1)

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

#define CACHE_SRRIPBS_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)  \
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
        assert(SRRIPBS_DATA_FREE_HEAD(set) && SRRIPBS_DATA_FREE_HEAD(set));                     \
                                                                                            \
        /* Check: current block must be in invalid state */                                 \
        assert((blk)->state == cache_block_invalid);                                        \
                                                                                            \
        CACHE_REMOVE_FROM_SQUEUE(blk, SRRIPBS_DATA_FREE_HEAD(set), SRRIPBS_DATA_FREE_TAIL(set));\
                                                                                            \
        (blk)->next = NULL;                                                                 \
        (blk)->prev = NULL;                                                                 \
                                                                                            \
        /* Reset block state */                                                             \
        (blk)->busy = 0;                                                                    \
        (blk)->cached = 0;                                                                  \
        (blk)->prefetch = 0;                                                                \
}while(0);


static ub4 get_set_type_srripbs(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x0f;
  msb_bits = (indx >> 6) & 0x0f;
  mid_bits = (indx >> 4) & 0x03;

  if (lsb_bits == msb_bits && mid_bits == 0)
  {
    return SRRIPBS_SAMPLED_SET;
  }

  return SRRIPBS_FOLLOWER_SET;
}

void cache_init_srripbs(ub4 set_indx, struct cache_params *params, srripbs_data *policy_data, 
    srripbs_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  
  policy_data->set_type = get_set_type_srripbs(set_indx);

  if (set_indx == 0)
  {
    for (ub1 i = 0; i < EPOCH_COUNT; i++)
    {
      /* Initialize epoch fill counter */
      SAT_CTR_INI(global_data->tx_epoch_fctr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_RST(global_data->tx_epoch_fctr[i]);

      /* Initialize epoch eviction counter */
      SAT_CTR_INI(global_data->tx_epoch_ectr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_RST(global_data->tx_epoch_ectr[i]);

      /* Initialize epoch fill counter */
      SAT_CTR_INI(global_data->cs_epoch_fctr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_RST(global_data->cs_epoch_fctr[i]);

      /* Initialize epoch eviction counter */
      SAT_CTR_INI(global_data->cs_epoch_ectr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_RST(global_data->cs_epoch_ectr[i]);

      /* Initialize epoch fill counter */
      SAT_CTR_INI(global_data->zs_epoch_fctr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_RST(global_data->zs_epoch_fctr[i]);

      /* Initialize epoch eviction counter */
      SAT_CTR_INI(global_data->zs_epoch_ectr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_RST(global_data->zs_epoch_ectr[i]);

      /* Initialize epoch fill counter */
      SAT_CTR_INI(global_data->bt_epoch_fctr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_RST(global_data->bt_epoch_fctr[i]);

      /* Initialize epoch eviction counter */
      SAT_CTR_INI(global_data->bt_epoch_ectr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_RST(global_data->bt_epoch_ectr[i]);

      /* Initialize epoch fill counter */
      SAT_CTR_INI(global_data->ps_epoch_fctr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_RST(global_data->ps_epoch_fctr[i]);

      /* Initialize epoch eviction counter */
      SAT_CTR_INI(global_data->ps_epoch_ectr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_RST(global_data->ps_epoch_ectr[i]);
    }

    global_data->access_interval = 0;
  }

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  SRRIPBS_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SRRIPBS_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC  

  assert(SRRIPBS_DATA_VALID_HEAD(policy_data));
  assert(SRRIPBS_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIPBS_DATA_MAX_RRPV(policy_data)    = MAX_RRPV;
  SRRIPBS_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  assert(params->spill_rrpv <= MAX_RRPV);

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SRRIPBS_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    SRRIPBS_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    SRRIPBS_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  SRRIPBS_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

  SRRIPBS_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(SRRIPBS_DATA_FREE_HLST(policy_data));

  SRRIPBS_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(SRRIPBS_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  SRRIPBS_DATA_FREE_HEAD(policy_data) = &(SRRIPBS_DATA_BLOCKS(policy_data)[0]);
  SRRIPBS_DATA_FREE_TAIL(policy_data) = &(SRRIPBS_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SRRIPBS_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SRRIPBS_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SRRIPBS_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SRRIPBS_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SRRIPBS_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SRRIPBS_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }
  
  /* Set current and default fill policy to SRRIPBS */
  SRRIPBS_DATA_CFPOLICY(policy_data) = cache_policy_srripbs;
  SRRIPBS_DATA_DFPOLICY(policy_data) = cache_policy_srripbs;
  SRRIPBS_DATA_CAPOLICY(policy_data) = cache_policy_srripbs;
  SRRIPBS_DATA_DAPOLICY(policy_data) = cache_policy_srripbs;
  SRRIPBS_DATA_CRPOLICY(policy_data) = cache_policy_srripbs;
  SRRIPBS_DATA_DRPOLICY(policy_data) = cache_policy_srripbs;
  
  assert(SRRIPBS_DATA_MAX_RRPV(policy_data) != 0);

#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_srripbs(srripbs_data *policy_data)
{
  /* Free all data blocks */
  free(SRRIPBS_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (SRRIPBS_DATA_VALID_HEAD(policy_data))
  {
    free(SRRIPBS_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SRRIPBS_DATA_VALID_TAIL(policy_data))
  {
    free(SRRIPBS_DATA_VALID_TAIL(policy_data));
  }
}

struct cache_block_t * cache_find_block_srripbs(srripbs_data *policy_data, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SRRIPBS_DATA_VALID_HEAD(policy_data)[rrpv].head;

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

void cache_fill_block_srripbs(srripbs_data *policy_data, srripbs_gdata *global_data,
    int way, long long tag, enum cache_block_state_t state, int strm, 
    memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain SRRIPBS specific data */
  block = &(SRRIPBS_DATA_BLOCKS(policy_data)[way]);
  
  assert(block->stream == 0);

  /* Get RRPV to be assigned to the new block */

  rrpv = cache_get_fill_rrpv_srripbs(policy_data, info);

#if 0
  if (info && info->spill == TRUE)
  {
    rrpv = SRRIPBS_DATA_SPILL_RRPV(policy_data);
  }
#endif

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
    block->dirty  = (info && info->spill) ? 1 : 0;
    block->epoch  = 0;
    block->access = 0;

    /* Insert block in to the corresponding RRPV queue */
    CACHE_APPEND_TO_QUEUE(block, 
        SRRIPBS_DATA_VALID_HEAD(policy_data)[rrpv], 
        SRRIPBS_DATA_VALID_TAIL(policy_data)[rrpv]);
  }

  SRRIPBS_DATA_CFPOLICY(policy_data) = SRRIPBS_DATA_DFPOLICY(policy_data);
  
  /* Update epoch counters */
  if (policy_data->set_type == SRRIPBS_SAMPLED_SET)
  {
    switch (info->stream)
    {
      case TS:
        SAT_CTR_INC(global_data->tx_epoch_fctr[MIN_EPOCH]);
        break;

      case CS:
        SAT_CTR_INC(global_data->cs_epoch_fctr[MIN_EPOCH]);
        break;

      case ZS:
        SAT_CTR_INC(global_data->zs_epoch_fctr[MIN_EPOCH]);
        break;

      case BT:
        SAT_CTR_INC(global_data->bt_epoch_fctr[MIN_EPOCH]);
        break;

      case PS:
        SAT_CTR_INC(global_data->ps_epoch_fctr[MIN_EPOCH]);
        break;

      default:
        break;
    }

    if (++(global_data->access_interval) >= INTERVAL_SIZE)
    {
      /* Half epoch counters  */
      for (ub1 i = 0; i < EPOCH_COUNT; i++)
      {
        printf("TSEF%d:%5d TSEE%d:%5d CSEF%d:%5d CSEE%d:%5d ZSEF%d:%5d ZSEE%d:%5d "
            "BTEF%d:%5d BTEE%d:%5d PSEF%d:%5d PSEE%d:%5d\n", 
            i, SAT_CTR_VAL(global_data->tx_epoch_fctr[i]), 
            i, SAT_CTR_VAL(global_data->tx_epoch_ectr[i]),
            i, SAT_CTR_VAL(global_data->cs_epoch_fctr[i]), 
            i, SAT_CTR_VAL(global_data->cs_epoch_ectr[i]),
            i, SAT_CTR_VAL(global_data->zs_epoch_fctr[i]), 
            i, SAT_CTR_VAL(global_data->zs_epoch_ectr[i]),
            i, SAT_CTR_VAL(global_data->bt_epoch_fctr[i]), 
            i, SAT_CTR_VAL(global_data->bt_epoch_ectr[i]),
            i, SAT_CTR_VAL(global_data->ps_epoch_fctr[i]), 
            i, SAT_CTR_VAL(global_data->ps_epoch_ectr[i]));

        SAT_CTR_HLF(global_data->tx_epoch_fctr[i]);
        SAT_CTR_HLF(global_data->tx_epoch_ectr[i]);
        SAT_CTR_HLF(global_data->cs_epoch_fctr[i]);
        SAT_CTR_HLF(global_data->cs_epoch_ectr[i]);
        SAT_CTR_HLF(global_data->zs_epoch_fctr[i]);
        SAT_CTR_HLF(global_data->zs_epoch_ectr[i]);
        SAT_CTR_HLF(global_data->bt_epoch_fctr[i]);
        SAT_CTR_HLF(global_data->bt_epoch_ectr[i]);
        SAT_CTR_HLF(global_data->ps_epoch_fctr[i]);
        SAT_CTR_HLF(global_data->ps_epoch_ectr[i]);
      }

      global_data->access_interval = 0;
    }
  }
}

int cache_replace_block_srripbs(srripbs_data *policy_data, srripbs_gdata *global_data)
{
  struct cache_block_t *block;
  int    rrpv;

  /* Remove a nonbusy block from the tail */
  unsigned int min_wayid = ~(0);

  /* Try to find an invalid block always from head of the free list. */
  for (block = SRRIPBS_DATA_FREE_HEAD(policy_data); block; block = block->prev)
  {
    return block->way;
  }
  
  /* Obtain RRPV from where to replace the block */
  rrpv = cache_get_replacement_rrpv_srripbs(policy_data);

  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= SRRIPBS_DATA_MAX_RRPV(policy_data));

  if (min_wayid == ~(0))
  {
    /* If there is no block with required RRPV, increment RRPV of all the blocks
     * until we get one with the required RRPV */
    if (SRRIPBS_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
    {
      CACHE_SRRIPBS_INCREMENT_RRPV(SRRIPBS_DATA_VALID_HEAD(policy_data), 
          SRRIPBS_DATA_VALID_TAIL(policy_data), rrpv);
    }

    switch (SRRIPBS_DATA_CRPOLICY(policy_data))
    {
      case cache_policy_srripbs:
        for (block = SRRIPBS_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && block->way < min_wayid)
            min_wayid = block->way;
        }
        break;

      case cache_policy_cpulast:
        /* First try to find a GPU block */
        for (block = SRRIPBS_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid && block->stream < TST))
            min_wayid = block->way;
        }

        /* If there so no GPU replacement candidate, replace CPU block */
        if (min_wayid == ~(0))
        {
          for (block = SRRIPBS_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
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
  
  if (min_wayid != ~(0) && policy_data->set_type == SRRIPBS_SAMPLED_SET)
  {
    ub1 epoch;

    epoch = policy_data->blocks[min_wayid].epoch;
    assert(epoch <= MAX_EPOCH);
    
    switch (policy_data->blocks[min_wayid].stream)
    {
      case TS:
        SAT_CTR_INC(global_data->tx_epoch_ectr[epoch]);
        break;

      case CS:
        SAT_CTR_INC(global_data->cs_epoch_ectr[epoch]);
        break;

      case ZS:
        SAT_CTR_INC(global_data->zs_epoch_ectr[epoch]);
        break;

      case BT:
        SAT_CTR_INC(global_data->bt_epoch_ectr[epoch]);
        break;
      
      case PS:
        SAT_CTR_INC(global_data->ps_epoch_ectr[epoch]);
        break;
      
      default:
        break;
    }
  }

  /* If no non busy block can be found, return -1 */
  return (min_wayid != ~(0)) ? min_wayid : -1;
}

void cache_access_block_srripbs(srripbs_data *policy_data, srripbs_gdata *global_data,
    int way, int strm, memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int old_rrpv;
  int new_rrpv;
  
  blk  = &(SRRIPBS_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);
  
  switch (SRRIPBS_DATA_CAPOLICY(policy_data))
  {
    case cache_policy_srripbs:
      /* Get old RRPV from the block */
      old_rrpv = (((rrip_list *)(blk->data))->rrpv);
      new_rrpv = old_rrpv;

      /* Get new RRPV using policy specific function */
      new_rrpv = cache_get_new_rrpv_srripbs(policy_data, global_data, 
          old_rrpv, info, blk->epoch);

      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        CACHE_REMOVE_FROM_QUEUE(blk, SRRIPBS_DATA_VALID_HEAD(policy_data)[old_rrpv],
            SRRIPBS_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, SRRIPBS_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            SRRIPBS_DATA_VALID_TAIL(policy_data)[new_rrpv]);
      }

      if (info && blk->stream == info->stream)
      {
        blk->epoch  = (blk->epoch == MAX_EPOCH) ? MAX_EPOCH : blk->epoch + 1;
      }
      else
      {
        blk->epoch = 0;
      }
      
      if (policy_data->set_type == SRRIPBS_SAMPLED_SET)
      {
        switch (blk->stream)
        {
          case TS:
            assert(blk->epoch <= MAX_EPOCH);
            SAT_CTR_INC(global_data->tx_epoch_fctr[blk->epoch]);
            break;

          case CS:
            assert(blk->epoch <= MAX_EPOCH);
            SAT_CTR_INC(global_data->cs_epoch_fctr[blk->epoch]);
            break;

          case ZS:
            assert(blk->epoch <= MAX_EPOCH);
            SAT_CTR_INC(global_data->zs_epoch_fctr[blk->epoch]);
            break;

          case BT:
            assert(blk->epoch <= MAX_EPOCH);
            SAT_CTR_INC(global_data->bt_epoch_fctr[blk->epoch]);
            break;

          case PS:
            assert(blk->epoch <= MAX_EPOCH);
            SAT_CTR_INC(global_data->ps_epoch_fctr[blk->epoch]);
            break;

          default:
            break;
        }
      }

      if (policy_data->set_type == SRRIPBS_SAMPLED_SET)
      {
        switch (blk->stream)
        {
          case TS:
            assert(blk->epoch <= MAX_EPOCH);
            SAT_CTR_INC(global_data->tx_epoch_fctr[blk->epoch]);
            break;

          case CS:
            assert(blk->epoch <= MAX_EPOCH);
            SAT_CTR_INC(global_data->cs_epoch_fctr[blk->epoch]);
            break;

          case ZS:
            assert(blk->epoch <= MAX_EPOCH);
            SAT_CTR_INC(global_data->zs_epoch_fctr[blk->epoch]);
            break;

          case BT:
            assert(blk->epoch <= MAX_EPOCH);
            SAT_CTR_INC(global_data->bt_epoch_fctr[blk->epoch]);
            break;

          case PS:
            assert(blk->epoch <= MAX_EPOCH);
            SAT_CTR_INC(global_data->ps_epoch_fctr[blk->epoch]);
            break;

          default:
            break;
        }
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

int cache_get_fill_rrpv_srripbs(srripbs_data *policy_data, memory_trace *info)
{
  switch (SRRIPBS_DATA_CFPOLICY(policy_data))
  {
    case cache_policy_lru:
      return 0;

    case cache_policy_lip:
      return SRRIPBS_DATA_MAX_RRPV(policy_data);

    case cache_policy_srripbs:
      if (info->stream == CS || info->stream == TS || 
          info->stream == ZS || info->stream == BS || info->stream == PS)
      {
        return SRRIPBS_DATA_MAX_RRPV(policy_data) - 1;
      }
      else
      {
        return SRRIPBS_DATA_MAX_RRPV(policy_data);
      }
    
    case cache_policy_bypass:
      /* Not to insert */
      return BYPASS_RRPV;  

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }
}

int cache_get_replacement_rrpv_srripbs(srripbs_data *policy_data)
{
  return SRRIPBS_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_srripbs(srripbs_data *policy_data, 
    srripbs_gdata *global_data, int old_rrpv, memory_trace *info,
    ub1 epoch)
{

#define FILL_CTR_VAL(g, e)  (SAT_CTR_VAL((g)->tx_epoch_fctr[e]))
#define EVCT_CTR_VAL(g, e)  (SAT_CTR_VAL((g)->tx_epoch_ectr[e]))
#define NE(e)               ((e) + 1)

  if (info->stream == TS && policy_data->set_type == SRRIPBS_FOLLOWER_SET)
  {
    if (epoch == 0 || epoch == 1 || epoch == 2)  
    {
      if (FILL_CTR_VAL(global_data, NE(epoch)) > 8 * FILL_CTR_VAL(global_data, NE(NE(epoch))))
      {
        return SRRIPBS_DATA_MAX_RRPV(policy_data); 
      }
    }
  }

#undef FILL_CTR_VAL
#undef EVCT_CTR_VAL
#undef NE

#define FILL_CTR_VAL(g, e)  (SAT_CTR_VAL((g)->ps_epoch_fctr[e]))
#define EVCT_CTR_VAL(g, e)  (SAT_CTR_VAL((g)->ps_epoch_ectr[e]))
#define NE(e)               ((e) + 1)

  if (info->stream == PS && policy_data->set_type == SRRIPBS_FOLLOWER_SET)
  {
    if (epoch == 0 || epoch == 1 || epoch == 2)  
    {
      if (FILL_CTR_VAL(global_data, NE(epoch)) > 8 * FILL_CTR_VAL(global_data, NE(NE(epoch))))
      {
        return SRRIPBS_DATA_MAX_RRPV(policy_data); 
      }
    }
  }

#undef FILL_CTR_VAL
#undef EVCT_CTR_VAL
#undef NE

  return 0;
}

/* Update state of block. */
void cache_set_block_srripbs(srripbs_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  block = &(SRRIPBS_DATA_BLOCKS(policy_data))[way];

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
  CACHE_REMOVE_FROM_QUEUE(block, SRRIPBS_DATA_VALID_HEAD(policy_data)[old_rrpv],
      SRRIPBS_DATA_VALID_TAIL(policy_data)[old_rrpv]);
  CACHE_APPEND_TO_SQUEUE(block, SRRIPBS_DATA_FREE_HEAD(policy_data), 
      SRRIPBS_DATA_FREE_TAIL(policy_data));

}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_srripbs(srripbs_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  assert(policy_data);
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (SRRIPBS_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (SRRIPBS_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (SRRIPBS_DATA_BLOCKS(policy_data)[way]).stream);

  return SRRIPBS_DATA_BLOCKS(policy_data)[way];
}

/* Get tag and state of a block. */
int cache_count_block_srripbs(srripbs_data *policy_data, ub1 strm)
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
    head = SRRIPBS_DATA_VALID_HEAD(policy_data)[rrpv].head;

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
void cache_set_current_fill_policy_srripbs(srripbs_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  SRRIPBS_DATA_CFPOLICY(policy_data) = policy;
}

/* Set policy for next access */
void cache_set_current_access_policy_srripbs(srripbs_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  SRRIPBS_DATA_CAPOLICY(policy_data) = policy;
}

/* Set policy for next replacment */
void cache_set_current_replacement_policy_srripbs(srripbs_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_cpulast);

  SRRIPBS_DATA_CRPOLICY(policy_data) = policy;
}

#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
#undef SRRIPBS_SAMPLED_SET
#undef SRRIPBS_FOLLOWER_SET
