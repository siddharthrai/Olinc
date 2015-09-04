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
#include "common/intermod-common.h"
#include "common/List.h"
#include "streampin.h"
#include "sap.h"

#define CACHE_SET(cache, set)         (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)         (&((set)->blocks[way]))
#define BYPASS_RRPV                   (-1)
#define PHASE_BITS                    (18)
#define PHASE_SIZE                    (1 << PHASE_BITS)
#define SPEEDUP(s)                    ((s) == streampin_stream_x)
#define CACHE_UPDATE_BLOCK_PSTATE(b)  ((b)->is_block_pinned = TRUE)

#define PSEL_WIDTH                    (10)
#define PSEL_MIN_VAL                  (0x00)  
#define PSEL_MAX_VAL                  (0x3ff)  
#define PSEL_MID_VAL                  (512)

#define CACHE_UPDATE_BLOCK_STATE(block, tag, va, state_in)    \
do                                                            \
{                                                             \
  (block)->tag      = (tag);                                  \
  (block)->vtl_addr = (va);                                   \
  (block)->state    = (state_in);                             \
}while(0)

#define CACHE_STREAMPIN_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)\
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
  if ((block)->stream == CS && strm != CS)                    \
  {                                                           \
    streampin_cs_blocks--;                                    \
  }                                                           \
                                                              \
  if (strm == CS && (block)->stream != CS)                    \
  {                                                           \
    streampin_cs_blocks++;                                    \
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
#define free_list_remove_block(set, blk)                                                    \
do                                                                                          \
{                                                                                           \
        /* Check: free list must be non empty as it contains the current block. */          \
        assert(STREAMPIN_DATA_FREE_HEAD(set) && STREAMPIN_DATA_FREE_HEAD(set));                     \
                                                                                            \
        /* Check: current block must be in invalid state */                                 \
        assert((blk)->state == cache_block_invalid);                                        \
                                                                                            \
        CACHE_REMOVE_FROM_SQUEUE(blk, STREAMPIN_DATA_FREE_HEAD(set), STREAMPIN_DATA_FREE_TAIL(set));    \
                                                                                            \
        (blk)->next = NULL;                                                                 \
        (blk)->prev = NULL;                                                                 \
                                                                                            \
        /* Reset block state */                                                             \
        (blk)->busy = 0;                                                                    \
        (blk)->cached = 0;                                                                  \
        (blk)->prefetch = 0;                                                                \
}while(0);

int streampin_cs_blocks = 0;

void cache_init_streampin(int set_idx, struct cache_params *params, streampin_data *policy_data,
    streampin_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  STREAMPIN_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  STREAMPIN_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC  

  assert(STREAMPIN_DATA_VALID_HEAD(policy_data));
  assert(STREAMPIN_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  STREAMPIN_DATA_MAX_RRPV(policy_data)  = MAX_RRPV;
  STREAMPIN_DATA_SET_INDEX(policy_data) = set_idx;

  /* Ways for the set */
  STREAMPIN_DATA_WAYS(policy_data) = params->ways;

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    STREAMPIN_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    STREAMPIN_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    STREAMPIN_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  STREAMPIN_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

  STREAMPIN_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(STREAMPIN_DATA_FREE_HLST(policy_data));

  STREAMPIN_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(STREAMPIN_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  STREAMPIN_DATA_FREE_HEAD(policy_data) = &(STREAMPIN_DATA_BLOCKS(policy_data)[0]);
  STREAMPIN_DATA_FREE_TAIL(policy_data) = &(STREAMPIN_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&STREAMPIN_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&STREAMPIN_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&STREAMPIN_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&STREAMPIN_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&STREAMPIN_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&STREAMPIN_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }
  
  /* Set current and default fill policy to SRRIP */
  STREAMPIN_DATA_CFPOLICY(policy_data) = cache_policy_streampin;
  STREAMPIN_DATA_DFPOLICY(policy_data) = cache_policy_streampin;
  STREAMPIN_DATA_CAPOLICY(policy_data) = cache_policy_streampin;
  STREAMPIN_DATA_DAPOLICY(policy_data) = cache_policy_streampin;
  STREAMPIN_DATA_CRPOLICY(policy_data) = cache_policy_streampin;
  STREAMPIN_DATA_DRPOLICY(policy_data) = cache_policy_streampin;
  
  assert(STREAMPIN_DATA_MAX_RRPV(policy_data) != 0);
  
  /* Allcate CT block hash table */
  if (!global_data->ct_blocks)
  {
    global_data->ct_blocks = (cs_qnode *)xcalloc(HTBLSIZE, sizeof(cs_qnode));
    assert(global_data->ct_blocks);

    for (int i = 0; i < HTBLSIZE; i++)
    {
      cs_qinit(&(global_data->ct_blocks)[i]);
    }
  }

  /* Allcate BT block hash table */
  if (!global_data->bt_blocks)
  {
    global_data->bt_blocks = (cs_qnode *)xcalloc(HTBLSIZE, sizeof(cs_qnode));
    assert(global_data->bt_blocks);

    for (int i = 0; i < HTBLSIZE; i++)
    {
      cs_qinit(&(global_data->bt_blocks)[i]);
    }
  }

  /* Allcate BT block hash table */
  if (!global_data->zt_blocks)
  {
    global_data->zt_blocks = (cs_qnode *)xcalloc(HTBLSIZE, sizeof(cs_qnode));
    assert(global_data->zt_blocks);

    for (int i = 0; i < HTBLSIZE; i++)
    {
      cs_qinit(&(global_data->zt_blocks)[i]);
    }
  }
  
  if (set_idx == 0)
  {
    global_data->bm_thr           = 32; 
    global_data->bm_ctr           = 0; 
    global_data->ct_predicted     = 0;
    global_data->bt_predicted     = 0;
    global_data->zt_predicted     = 0;
    global_data->ct_evict         = 0;
    global_data->bt_evict         = 0;
    global_data->zt_evict         = 0;
    global_data->ct_used          = 0;
    global_data->bt_used          = 0;
    global_data->zt_used          = 0;
    global_data->ct_correct       = 0;
    global_data->bt_correct       = 0;
    global_data->zt_correct       = 0;
    global_data->ct_use_possible  = 0;
    global_data->bt_use_possible  = 0;
    global_data->zt_use_possible  = 0;
    global_data->ct_block_count   = 0;
    global_data->bt_block_count   = 0;
    global_data->zt_block_count   = 0;
    global_data->speedup_stream   = NN;

    for (ub4 i = 0; i <= TST; i++)
    {
      global_data->per_stream_frrpv[i] = xcalloc(1, sizeof(ub8) * STREAMPIN_DATA_MAX_RRPV(policy_data));
      assert(global_data->per_stream_frrpv[i]);

      SAT_CTR_INI(global_data->streampin_hint[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    }
  }

  /* Initialize per-policy fill counters */
  memset(policy_data->fills, 0, sizeof(ub8) * (TST + 1));
  policy_data->min_fills = 0xffffffff;
  policy_data->max_fills = 0;
  
#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_streampin(streampin_data *policy_data)
{
  /* Free all data blocks */
  free(STREAMPIN_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (STREAMPIN_DATA_VALID_HEAD(policy_data))
  {
    free(STREAMPIN_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (STREAMPIN_DATA_VALID_TAIL(policy_data))
  {
    free(STREAMPIN_DATA_VALID_TAIL(policy_data));
  }
}

static void cache_update_interval_end(streampin_gdata *global_data)
{
  ub8 max_speedup;
  
  max_speedup = 0;
  global_data->speedup_stream = NN;

  for (ub1 s = NN; s <= TST; s++)
  {
    if (SAT_CTR_VAL(global_data->streampin_hint[s]) > max_speedup)
    {
       global_data->speedup_stream = s;
       max_speedup = SAT_CTR_VAL(global_data->streampin_hint[s]);

       SAT_CTR_RST(global_data->streampin_hint[s]);
    }
  }
}

/* Returns STREAMPIN stream corresponding to access stream based on stream 
 * classification algoritm. */
streampin_stream get_streampin_stream(memory_trace *info)
{
  return (streampin_stream)(info->sap_stream);
}

static void cache_update_hint_count(streampin_gdata *global_data, memory_trace *info)
{
  assert(info->stream <= TST);
  
  if (SPEEDUP(get_streampin_stream(info)))
  {
    SAT_CTR_INC(global_data->streampin_hint[info->stream]);
  }
}

struct cache_block_t * cache_find_block_streampin(streampin_data *policy_data, 
    streampin_gdata *global_data, long long tag, memory_trace *info)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = STREAMPIN_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

end:
  switch (info->stream)
  {
    case CS:
      global_data->access_seq[CS][1]     = global_data->access_seq[CS][0];
      global_data->access_seq[CS][0]     = global_data->access_count;
      global_data->per_stream_fill[CS]  += 1;
      global_data->reuse_distance[CS]   += global_data->access_seq[CS][0] - global_data->access_seq[CS][1];
      
      if (node)
      {
        global_data->per_stream_hit[CS]  += 1;
      }

      policy_data->fills[CS]  += 1;

      /* Update min, max fills  */
      if (policy_data->fills[CS] < policy_data->min_fills)
      {
        policy_data->min_fills = policy_data->fills[CS];   
      }

      if (policy_data->fills[CS] > policy_data->max_fills)
      {
        policy_data->max_fills = policy_data->fills[CS];   
      }

      break;

    case ZS:
      global_data->access_seq[ZS][1]     = global_data->access_seq[ZS][0];
      global_data->access_seq[ZS][0]     = global_data->access_count;
      global_data->per_stream_fill[ZS]  += 1;
      global_data->reuse_distance[ZS]   += global_data->access_seq[ZS][0] - global_data->access_seq[ZS][1];

      if (node)
      {
        global_data->per_stream_hit[ZS]  += 1;
      }

      policy_data->fills[ZS]  += 1;

      /* Update min, max fills  */
      if (policy_data->fills[ZS] < policy_data->min_fills)
      {
        policy_data->min_fills = policy_data->fills[ZS];   
      }

      if (policy_data->fills[ZS] > policy_data->max_fills)
      {
        policy_data->max_fills = policy_data->fills[ZS];   
      }

      break;

    case TS:
      global_data->access_seq[TS][1]     = global_data->access_seq[TS][0];
      global_data->access_seq[TS][0]     = global_data->access_count;
      global_data->per_stream_fill[TS]  += 1;
      global_data->reuse_distance[TS]   += global_data->access_seq[TS][0] - global_data->access_seq[TS][1];

      policy_data->fills[TS]  += 1;
      
      if (node)
      {
        global_data->per_stream_hit[TS]  += 1;
      }

      /* Update min, max fills  */
      if (policy_data->fills[TS] < policy_data->min_fills)
      {
        policy_data->min_fills = policy_data->fills[TS];   
      }

      if (policy_data->fills[TS] > policy_data->max_fills)
      {
        policy_data->max_fills = policy_data->fills[TS];   
      }

      if (info->stream == TS)
      {
        /* Lookup CT reuse */
        if (attila_map_lookup(global_data->ct_blocks, info->address, ATTILA_MASTER_KEY))
        {
          attila_map_delete(global_data->ct_blocks, info->address, ATTILA_MASTER_KEY);
          global_data->ct_use_possible += 1;

          assert(global_data->ct_block_count);
          global_data->ct_block_count  -= 1;
        }

        /* Lookup BT reuse */
        if (attila_map_lookup(global_data->bt_blocks, info->address, ATTILA_MASTER_KEY))
        {
          attila_map_delete(global_data->bt_blocks, info->address, ATTILA_MASTER_KEY);
          global_data->bt_use_possible += 1;

          assert(global_data->bt_block_count);
          global_data->bt_block_count  -= 1;
        }

        /* Lookup ZT reuse */
        if (attila_map_lookup(global_data->zt_blocks, info->address, ATTILA_MASTER_KEY))
        {
          attila_map_delete(global_data->zt_blocks, info->address, ATTILA_MASTER_KEY);
          global_data->zt_use_possible += 1;

          assert(global_data->zt_block_count);
          global_data->zt_block_count  -= 1;
        }
      }

      break;
  }

  /* Update speedup hint */
  cache_update_hint_count(global_data, info);
  
  if (++(global_data->access_count) >= PHASE_SIZE)
  {

    printf("CF:%ld ZF:%ld TF: %ld\n", global_data->per_stream_fill[CS], 
      global_data->per_stream_fill[ZS], global_data->per_stream_fill[TS]);

    printf("CH:%ld ZH:%ld TH: %ld\n", global_data->per_stream_hit[CS], 
      global_data->per_stream_hit[ZS], global_data->per_stream_hit[TS]);

    printf("C0:%ld C1:%ld C2:%ld C3: %ld CT %ld BT %ld\n", global_data->per_stream_frrpv[CS][0], 
      global_data->per_stream_frrpv[CS][1], global_data->per_stream_frrpv[CS][2],
      global_data->per_stream_frrpv[CS][3], global_data->ct_predicted, global_data->bt_predicted);

    printf("CB:%ld BB:%ld ZB: %ld\n", global_data->ct_block_count, 
      global_data->bt_block_count, global_data->zt_block_count);

#if 0
    printf("PC: %ld PB: %ld PZ: %ld\n", global_data->ct_predicted, global_data->bt_predicted,
        global_data->zt_predicted);

    printf("CC: %ld CB: %ld CZ: %ld\n", global_data->ct_correct, global_data->bt_correct,
        global_data->zt_correct);

    printf("UC: %ld UB: %ld UZ: %ld\n", global_data->ct_used, global_data->bt_used,
        global_data->zt_used);
#endif
    printf("UPC: %ld UPB %ld UPZ %ld\n", global_data->ct_use_possible, global_data->bt_use_possible, 
        global_data->zt_use_possible);

#if 0
    printf("EC: %ld EB: %ld EZ: %ld\n", global_data->ct_evict, global_data->bt_evict,
        global_data->zt_evict);
#endif

    memset(policy_data->fills, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_fill, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_hit, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->reuse_distance, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->access_seq, 0, sizeof(ub8) * (TST + 1) * 2);
    
    cache_update_interval_end(global_data);

    policy_data->min_fills    = 0xffffff;
    policy_data->max_fills    = 0;
    global_data->access_count = 0;
  }
  
  return node;
}

void cache_fill_block_streampin(streampin_data *policy_data, streampin_gdata *global_data, 
    int way, long long tag, enum cache_block_state_t state, int strm, 
    memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain SRRIP specific data */
  block = &(STREAMPIN_DATA_BLOCKS(policy_data)[way]);
  
  assert(block->stream == 0);

  block->is_ct_block = FALSE;   
  block->is_bt_block = FALSE;   
  block->is_zt_block = FALSE;   

  /* Get RRPV to be assigned to the new block */
  if (info)
  {
    rrpv = cache_get_fill_rrpv_streampin(policy_data, global_data, info, NULL);
  }
  else
  {
    rrpv = STREAMPIN_DATA_MAX_RRPV(policy_data);
  }

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
      
    /* Fill stream of interest at RRPV 0 */
    if (info->stream == global_data->speedup_stream)
    {
      CACHE_UPDATE_BLOCK_PSTATE(block);
    }

    block->dirty  = (info && info->spill) ? 1 : 0;
    block->epoch  = 0;
    block->access = 0;

    assert(rrpv <= STREAMPIN_DATA_MAX_RRPV(policy_data));
    global_data->per_stream_frrpv[info->stream][rrpv] += 1;

    /* Insert block into the corresponding RRPV queue */
    CACHE_APPEND_TO_QUEUE(block, 
        STREAMPIN_DATA_VALID_HEAD(policy_data)[rrpv], 
        STREAMPIN_DATA_VALID_TAIL(policy_data)[rrpv]);
  }

  STREAMPIN_DATA_CFPOLICY(policy_data) = STREAMPIN_DATA_DFPOLICY(policy_data);
}

int cache_replace_block_streampin(streampin_data *policy_data, streampin_gdata *global_data)
{
  struct  cache_block_t *block;
  int     rrpv;
  int     min_rrpv;
  ub4     min_wayid;
  
  assert(policy_data && global_data);
  
  min_wayid = ~(0);
  min_rrpv  = !STREAMPIN_DATA_MAX_RRPV(policy_data);

  /* Try to find an invalid block always from head of the free list. */
  for (block = STREAMPIN_DATA_FREE_HEAD(policy_data); block; block = block->prev)
  {
    if (block->way < min_wayid)
      min_wayid = block->way;
  }
  
  /* Obtain RRPV from where to replace the block */
  rrpv = cache_get_replacement_rrpv_streampin(policy_data);

  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= STREAMPIN_DATA_MAX_RRPV(policy_data));

  if (min_wayid == ~(0))
  {
    /* If there is no block with required RRPV, increment RRPV of all the blocks
     * until we get one with the required RRPV */
    while (STREAMPIN_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
    {
      /* All blocks which are already pinned are promoted to RRPV 0 
       * and are unpinned. So we iterate through the blocks at RRPV 3 
       * and move all the blocks which are pinned to RRPV 0 */
      CACHE_STREAMPIN_INCREMENT_RRPV(STREAMPIN_DATA_VALID_HEAD(policy_data), 
          STREAMPIN_DATA_VALID_TAIL(policy_data), rrpv);

      for (block = STREAMPIN_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
      {
        if (block->is_block_pinned == TRUE)
        {
          block->is_block_pinned = FALSE;

          /* Move block to min RRPV */
          CACHE_REMOVE_FROM_QUEUE(block, STREAMPIN_DATA_VALID_HEAD(policy_data)[rrpv],
              STREAMPIN_DATA_VALID_TAIL(policy_data)[rrpv]);
          CACHE_APPEND_TO_QUEUE(block, STREAMPIN_DATA_VALID_HEAD(policy_data)[min_rrpv], 
              STREAMPIN_DATA_VALID_TAIL(policy_data)[min_rrpv]);
        }
      }
    }

#define XSTRM_BLOCK(b)  ((b)->is_bt_block == TRUE || (b)->is_ct_block == TRUE || (b)->is_zt_block == TRUE)

    switch (STREAMPIN_DATA_CRPOLICY(policy_data))
    {
      case cache_policy_streampin:

        for (block = STREAMPIN_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && !XSTRM_BLOCK(block) && block->way < min_wayid)
            min_wayid = block->way;
        }
        
        if (min_wayid == ~0)
        {
          for (block = STREAMPIN_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
          {
            if (!block->busy && block->way < min_wayid)
              min_wayid = block->way;
          }
        }
        break;

      case cache_policy_cpulast:
        /* First try to find a GPU block */
        for (block = STREAMPIN_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid && block->stream <= TST))
            min_wayid = block->way;
        }

        /* If there so no GPU replacement candidate, replace CPU block */
        if (min_wayid == ~(0))
        {
          for (block = STREAMPIN_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
          {
            if (!block->busy && (block->way < min_wayid))
              min_wayid = block->way;
          }
        }
        break;

      default:
        panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
    }

#undef XSTRM_BLOCK

  }
  
  if (min_wayid != ~(0))
  {
    block = &(STREAMPIN_DATA_BLOCKS(policy_data)[min_wayid]);

    if (block->is_ct_block)
    {
      global_data->ct_evict += 1;
    }

    if (block->is_bt_block)
    {
      global_data->bt_evict += 1;
    }

    if (block->is_zt_block)
    {
      global_data->zt_evict += 1;
    }
  }

  /* If no non busy block can be found, return -1 */
  return (min_wayid != ~(0)) ? min_wayid : -1;
}

void cache_access_block_streampin(streampin_data *policy_data, streampin_gdata *global_data, int way, 
    int strm, memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int old_rrpv;
  int new_rrpv;

  blk  = &(STREAMPIN_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);

  switch (STREAMPIN_DATA_CAPOLICY(policy_data))
  {
    case cache_policy_streampin:
      /* Update RRPV and epoch only for read hits */
      if (info)
      {
        /* Get old RRPV from the block */
        old_rrpv = (((rrip_list *)(blk->data))->rrpv);

        /* Get new RRPV using policy specific function */
        new_rrpv = cache_get_new_rrpv_streampin(policy_data, global_data, old_rrpv, info);

        /* Update block queue if block got new RRPV */
        if (new_rrpv != old_rrpv)
        {
          CACHE_REMOVE_FROM_QUEUE(blk, STREAMPIN_DATA_VALID_HEAD(policy_data)[old_rrpv],
              STREAMPIN_DATA_VALID_TAIL(policy_data)[old_rrpv]);
          CACHE_APPEND_TO_QUEUE(blk, STREAMPIN_DATA_VALID_HEAD(policy_data)[new_rrpv], 
              STREAMPIN_DATA_VALID_TAIL(policy_data)[new_rrpv]);
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
      
      if (info->spill == TRUE && blk->is_block_pinned == FALSE && 
          info->stream == global_data->speedup_stream)
      {
        CACHE_UPDATE_BLOCK_PSTATE(blk);
      }
      else
      {
        blk->is_block_pinned = FALSE;
      }

      blk->dirty = (info && info->dirty) ? 1 : 0;

      if (blk->is_ct_block)
      {
        global_data->ct_used += 1;

        if (info->stream == TS)
        {
          global_data->ct_correct += 1;
          blk->is_ct_block         = FALSE;
        }
      }

      if (blk->is_bt_block)
      {
        global_data->bt_used +=1;

        if (info->stream == TS)
        {
          global_data->bt_correct +=1;
          blk->is_bt_block         = FALSE;
        }
      }

      if (blk->is_zt_block)
      {
        global_data->zt_used +=1;

        if (info->stream == TS)
        {
          global_data->zt_correct +=1;
          blk->is_zt_block         = FALSE;
        }
      }

      break;

    case cache_policy_bypass:
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }
}

int cache_get_fill_rrpv_streampin(streampin_data *policy_data, 
    streampin_gdata *global_data, memory_trace *info, ub1 *is_x_block)
{
  int new_rrpv;
  int tmp_rrpv;
  int x_rrpv;

  if ((global_data->bm_ctr)++ == 0)
  {
    x_rrpv = 0;
  }
  else
  {
    x_rrpv = 0;
  }

  if (global_data->bm_ctr == global_data->bm_thr)
  {
    global_data->bm_ctr = 0;
  }

  new_rrpv = STREAMPIN_DATA_MAX_RRPV(policy_data) - 1;  

  switch (STREAMPIN_DATA_CFPOLICY(policy_data))
  {
    case cache_policy_streampin:
      if (info->stream == global_data->speedup_stream)
      {
        new_rrpv = 0;
      }
      break;

    case cache_policy_bypass:
      /* Not to insert */
      return BYPASS_RRPV;  

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }

  return new_rrpv;
}

int cache_get_replacement_rrpv_streampin(streampin_data *policy_data)
{
  return STREAMPIN_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_streampin(streampin_data *policy_data, streampin_gdata *global_data, int old_rrpv, 
    memory_trace *info)
{
  int new_rrpv;
  int reuse_ratio;
  
  new_rrpv = !STREAMPIN_DATA_MAX_RRPV(policy_data);

  if (info->spill == TRUE)
  {
    if (old_rrpv == STREAMPIN_DATA_MAX_RRPV(policy_data))
    {
      new_rrpv = old_rrpv - 1;
    }
    else
    {
      new_rrpv = old_rrpv;
    }
  }

  return new_rrpv;
}

/* Update state of block. */
void cache_set_block_streampin(streampin_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  block = &(STREAMPIN_DATA_BLOCKS(policy_data))[way];

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
  CACHE_REMOVE_FROM_QUEUE(block, STREAMPIN_DATA_VALID_HEAD(policy_data)[old_rrpv],
      STREAMPIN_DATA_VALID_TAIL(policy_data)[old_rrpv]);
  CACHE_APPEND_TO_SQUEUE(block, STREAMPIN_DATA_FREE_HEAD(policy_data), 
      STREAMPIN_DATA_FREE_TAIL(policy_data));
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_streampin(streampin_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  assert(policy_data);
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (STREAMPIN_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (STREAMPIN_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (STREAMPIN_DATA_BLOCKS(policy_data)[way]).stream);

  return STREAMPIN_DATA_BLOCKS(policy_data)[way];
}

/* Get tag and state of a block. */
int cache_count_block_streampin(streampin_data *policy_data, ub1 strm)
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
    head = STREAMPIN_DATA_VALID_HEAD(policy_data)[rrpv].head;

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
void cache_set_current_fill_policy_streampin(streampin_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  STREAMPIN_DATA_CFPOLICY(policy_data) = policy;
}

/* Set policy for next access */
void cache_set_current_access_policy_streampin(streampin_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  STREAMPIN_DATA_CAPOLICY(policy_data) = policy;
}

/* Set policy for next replacment */
void cache_set_current_replacement_policy_streampin(streampin_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_cpulast);

  STREAMPIN_DATA_CRPOLICY(policy_data) = policy;
}

#undef SPEEDUP
#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
