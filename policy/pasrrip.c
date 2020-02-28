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
#include "pasrrip.h"
#include "sap.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))
#define BYPASS_RRPV             (-1)
#define PHASE_BITS              (12)
#define PHASE_SIZE              (1 << PHASE_BITS)

#define CACHE_UPDATE_BLOCK_STATE(block, tag, va, state_in)    \
do                                                            \
{                                                             \
  (block)->tag      = (tag);                                  \
  (block)->vtl_addr = (va);                                   \
  (block)->state    = (state_in);                             \
}while(0)

#define CACHE_PASRRIP_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)\
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
    pa_cs_blocks--;                                           \
  }                                                           \
                                                              \
  if (strm == CS && (block)->stream != CS)                    \
  {                                                           \
    pa_cs_blocks++;                                           \
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
        assert(PASRRIP_DATA_FREE_HEAD(set) && PASRRIP_DATA_FREE_HEAD(set));                     \
                                                                                            \
        /* Check: current block must be in invalid state */                                 \
        assert((blk)->state == cache_block_invalid);                                        \
                                                                                            \
        CACHE_REMOVE_FROM_SQUEUE(blk, PASRRIP_DATA_FREE_HEAD(set), PASRRIP_DATA_FREE_TAIL(set));\
                                                                                            \
        (blk)->next = NULL;                                                                 \
        (blk)->prev = NULL;                                                                 \
                                                                                            \
        /* Reset block state */                                                             \
        (blk)->busy = 0;                                                                    \
        (blk)->cached = 0;                                                                  \
        (blk)->prefetch = 0;                                                                \
}while(0);

int pa_cs_blocks = 0;

void cache_init_pasrrip(int set_idx, struct cache_params *params, pasrrip_data *policy_data,
    pasrrip_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  PASRRIP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  PASRRIP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC  

  assert(PASRRIP_DATA_VALID_HEAD(policy_data));
  assert(PASRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  PASRRIP_DATA_MAX_RRPV(policy_data)  = MAX_RRPV;
  PASRRIP_DATA_SET_INDEX(policy_data) = set_idx;

  /* Ways for the set */
  PASRRIP_DATA_WAYS(policy_data) = params->ways;

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    PASRRIP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    PASRRIP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    PASRRIP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  PASRRIP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

  PASRRIP_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(PASRRIP_DATA_FREE_HLST(policy_data));

  PASRRIP_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(PASRRIP_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  PASRRIP_DATA_FREE_HEAD(policy_data) = &(PASRRIP_DATA_BLOCKS(policy_data)[0]);
  PASRRIP_DATA_FREE_TAIL(policy_data) = &(PASRRIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&PASRRIP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&PASRRIP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&PASRRIP_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&PASRRIP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&PASRRIP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&PASRRIP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }
  
  /* Set current and default fill policy to SRRIP */
  PASRRIP_DATA_CFPOLICY(policy_data) = cache_policy_pasrrip;
  PASRRIP_DATA_DFPOLICY(policy_data) = cache_policy_pasrrip;
  PASRRIP_DATA_CAPOLICY(policy_data) = cache_policy_pasrrip;
  PASRRIP_DATA_DAPOLICY(policy_data) = cache_policy_pasrrip;
  PASRRIP_DATA_CRPOLICY(policy_data) = cache_policy_pasrrip;
  PASRRIP_DATA_DRPOLICY(policy_data) = cache_policy_pasrrip;
  
  assert(PASRRIP_DATA_MAX_RRPV(policy_data) != 0);
  
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

    for (ub4 i = 0; i < TST; i++)
    {
      global_data->per_stream_frrpv[i] = xcalloc(1, sizeof(ub8) * PASRRIP_DATA_MAX_RRPV(policy_data));
      assert(global_data->per_stream_frrpv[i]);
    }
  }

  /* Initialize per-policy fill counters */
  memset(policy_data->fills, 0, sizeof(ub8) * TST);
  policy_data->min_fills = 0xffffffff;
  policy_data->max_fills = 0;
  
#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_pasrrip(pasrrip_data *policy_data)
{
  /* Free all data blocks */
  free(PASRRIP_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (PASRRIP_DATA_VALID_HEAD(policy_data))
  {
    free(PASRRIP_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (PASRRIP_DATA_VALID_TAIL(policy_data))
  {
    free(PASRRIP_DATA_VALID_TAIL(policy_data));
  }
}

struct cache_block_t * cache_find_block_pasrrip(pasrrip_data *policy_data, 
    pasrrip_gdata *global_data, long long tag, memory_trace *info)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = PASRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head;

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

    printf("PC: %ld PB: %ld PZ: %ld\n", global_data->ct_predicted, global_data->bt_predicted,
        global_data->zt_predicted);

    printf("CC: %ld CB: %ld CZ: %ld\n", global_data->ct_correct, global_data->bt_correct,
        global_data->zt_correct);

    printf("UC: %ld UB: %ld UZ: %ld\n", global_data->ct_used, global_data->bt_used,
        global_data->zt_used);

    printf("UPC: %ld UPB %ld UPZ %ld\n", global_data->ct_use_possible, global_data->bt_use_possible, 
        global_data->zt_use_possible);

    printf("EC: %ld EB: %ld EZ: %ld\n", global_data->ct_evict, global_data->bt_evict,
        global_data->zt_evict);

    memset(policy_data->fills, 0, sizeof(ub8) * TST);
    memset(global_data->per_stream_fill, 0, sizeof(ub8) * TST);
    memset(global_data->per_stream_hit, 0, sizeof(ub8) * TST);
    memset(global_data->reuse_distance, 0, sizeof(ub8) * TST);
    memset(global_data->access_seq, 0, sizeof(ub8) * TST * 2);
    
    policy_data->min_fills    = 0xffffff;
    policy_data->max_fills    = 0;
    global_data->access_count = 0;
#if 0
    global_data->ct_predicted = 0;
    global_data->bt_predicted = 0;
    global_data->zt_predicted = 0;
    global_data->ct_evict     = 0;
    global_data->bt_evict     = 0;
    global_data->zt_evict     = 0;
#endif
  }

  return node;
}

void cache_fill_block_pasrrip(pasrrip_data *policy_data, pasrrip_gdata *global_data, 
    int way, long long tag, enum cache_block_state_t state, int strm, 
    memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain SRRIP specific data */
  block = &(PASRRIP_DATA_BLOCKS(policy_data)[way]);
  
  assert(block->stream == 0);

  block->is_ct_block = FALSE;   
  block->is_bt_block = FALSE;   
  block->is_zt_block = FALSE;   

  /* Get RRPV to be assigned to the new block */
#if 0
  if (info && info->fill == TRUE)
#endif
  if (info)
  {
    rrpv = cache_get_fill_rrpv_pasrrip(policy_data, global_data, info, NULL);

    if (info->stream == CS)
    {
      rrpv = cache_get_fill_rrpv_pasrrip(policy_data, global_data, info, &(block->is_ct_block));

      if (block->is_ct_block)
      {
        global_data->ct_predicted += 1;

        if (!attila_map_lookup(global_data->ct_blocks, tag, ATTILA_MASTER_KEY))
        {
          attila_map_insert(global_data->ct_blocks, tag, ATTILA_MASTER_KEY, block);
          global_data->ct_block_count++;
        }
      }
    }

    if (info->stream == BS)
    {
      rrpv = cache_get_fill_rrpv_pasrrip(policy_data, global_data, info, &(block->is_bt_block));

      if (block->is_bt_block)
      {
        global_data->bt_predicted += 1;

        if (!attila_map_lookup(global_data->bt_blocks, tag, ATTILA_MASTER_KEY))
        {
          attila_map_insert(global_data->bt_blocks, tag, ATTILA_MASTER_KEY, block);
          global_data->bt_block_count++;
        }
      }
    }

    if (info->stream == ZS)
    {
      rrpv = cache_get_fill_rrpv_pasrrip(policy_data, global_data, info, &(block->is_zt_block));

      if (block->is_zt_block)
      {
        global_data->zt_predicted += 1;

        if (!attila_map_lookup(global_data->zt_blocks, tag, ATTILA_MASTER_KEY))
        {
          attila_map_insert(global_data->zt_blocks, tag, ATTILA_MASTER_KEY, block);
          global_data->zt_block_count++;
        }
      }
    }
  }
  else
  {
    rrpv = PASRRIP_DATA_MAX_RRPV(policy_data);
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

    block->dirty  = (info && info->spill) ? 1 : 0;
    block->epoch  = 0;
    block->access = 0;

    assert(rrpv <= PASRRIP_DATA_MAX_RRPV(policy_data));
    global_data->per_stream_frrpv[info->stream][rrpv] += 1;

#if 0
    block->access = (PASRRIP_DATA_MAX_RRPV(policy_data) - rrpv);

    /* Modulate RRPV based on fill count */
#define NUM(p, i)   ((p->max_fills - p->fills[i->stream]) * PASRRIP_DATA_MAX_RRPV(p))
#define DNM(p)      (p->max_fills - p->min_fills)

    if (rrpv != PASRRIP_DATA_MAX_RRPV(policy_data))
    {
      if (policy_data->fills[info->stream] && (policy_data->max_fills - policy_data->min_fills))
      {
        rrpv = NUM(policy_data, info) / DNM(policy_data);
      }
    }

#undef NUM
#undef DNM
#endif    

    /* Insert block into the corresponding RRPV queue */
    CACHE_APPEND_TO_QUEUE(block, 
        PASRRIP_DATA_VALID_HEAD(policy_data)[rrpv], 
        PASRRIP_DATA_VALID_TAIL(policy_data)[rrpv]);
  }

  PASRRIP_DATA_CFPOLICY(policy_data) = PASRRIP_DATA_DFPOLICY(policy_data);
}

int cache_replace_block_pasrrip(pasrrip_data *policy_data, pasrrip_gdata *global_data)
{
  struct cache_block_t *block;
  int    rrpv;

  /* Remove a nonbusy block from the tail */
  unsigned int min_wayid = ~(0);

  /* Try to find an invalid block always from head of the free list. */
  for (block = PASRRIP_DATA_FREE_HEAD(policy_data); block; block = block->prev)
  {
    if (block->way < min_wayid)
      min_wayid = block->way;
  }
  
  /* Obtain RRPV from where to replace the block */
  rrpv = cache_get_replacement_rrpv_pasrrip(policy_data);

  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= PASRRIP_DATA_MAX_RRPV(policy_data));

  if (min_wayid == ~(0))
  {
    /* If there is no block with required RRPV, increment RRPV of all the blocks
     * until we get one with the required RRPV */
#if 0
    if (PASRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
    {
      CACHE_PASRRIP_INCREMENT_RRPV(PASRRIP_DATA_VALID_HEAD(policy_data), 
          PASRRIP_DATA_VALID_TAIL(policy_data), rrpv);
    }
#endif

#define XSTRM_BLOCK(b)  ((b)->is_bt_block == TRUE || (b)->is_ct_block == TRUE || (b)->is_zt_block == TRUE)

    switch (PASRRIP_DATA_CRPOLICY(policy_data))
    {
      case cache_policy_pasrrip:
        for (block = PASRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && !(XSTRM_BLOCK(block)) && block->way < min_wayid)
            min_wayid = block->way;
#if 0
          if (!block->busy && block->access == 0 && block->way < min_wayid)
            min_wayid = block->way;
#endif
        }

        if (min_wayid == ~(0))
        {
          if (PASRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
          {
            CACHE_PASRRIP_INCREMENT_RRPV(PASRRIP_DATA_VALID_HEAD(policy_data), 
                PASRRIP_DATA_VALID_TAIL(policy_data), rrpv);
          }

          for (block = PASRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
          {
            if (!block->busy && !(XSTRM_BLOCK(block)) && block->way < min_wayid)
              min_wayid = block->way;
#if 0
            if (!block->busy && block->access == 0 && block->way < min_wayid)
              min_wayid = block->way;
#endif
          }
        }

        if (min_wayid == ~0)
        {
          for (block = PASRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
          {
            if (!block->busy && block->way < min_wayid)
              min_wayid = block->way;
          }
        }
#if 0
        for (block = PASRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && block->way < min_wayid)
            min_wayid = block->way;
        }
#endif
        break;

      case cache_policy_cpulast:
        /* First try to find a GPU block */
        for (block = PASRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid && block->stream < TST))
            min_wayid = block->way;
        }

        /* If there so no GPU replacement candidate, replace CPU block */
        if (min_wayid == ~(0))
        {
          for (block = PASRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
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
    block = &(PASRRIP_DATA_BLOCKS(policy_data)[min_wayid]);

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

void cache_access_block_pasrrip(pasrrip_data *policy_data, pasrrip_gdata *global_data, int way, 
    int strm, memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int old_rrpv;
  int new_rrpv;

  blk  = &(PASRRIP_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);

  switch (PASRRIP_DATA_CAPOLICY(policy_data))
  {
    case cache_policy_pasrrip:
      /* Update RRPV and epoch only for read hits */
#if 0
      if (info && info->fill == TRUE)
#endif
        if (info)
        {
#if 0
          if (blk->access)
          {
            blk->access--;
          }
#endif
          /* Get old RRPV from the block */
          old_rrpv = (((rrip_list *)(blk->data))->rrpv);

          /* Get new RRPV using policy specific function */
          new_rrpv = cache_get_new_rrpv_pasrrip(policy_data, global_data, old_rrpv, info);
#if 0
          blk->access +=  PASRRIP_DATA_MAX_RRPV(policy_data) - new_rrpv;

          if (blk->access == 0)
          {
            new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data);
          }
#endif

#if 0
          if (blk->is_ct_block == TRUE && info->stream == TS)
          {
            new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data);
          }
#endif

#if 0
          /* Modulate RRPV based on fill count */
#define NUM(p, i)   ((p->max_fills - p->fills[i->stream]) * PASRRIP_DATA_MAX_RRPV(p))
#define DNM(p)      (p->max_fills - p->min_fills)

          if (new_rrpv != PASRRIP_DATA_MAX_RRPV(policy_data))
          {
            if (policy_data->fills[info->stream] && (policy_data->max_fills - policy_data->min_fills))
            {
              new_rrpv = (NUM(policy_data, info) / DNM(policy_data));
            }
          }

#undef NUM
#undef DNM
#endif
          /* Update block queue if block got new RRPV */
          if (new_rrpv != old_rrpv)
          {
            CACHE_REMOVE_FROM_QUEUE(blk, PASRRIP_DATA_VALID_HEAD(policy_data)[old_rrpv],
                PASRRIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
            CACHE_APPEND_TO_QUEUE(blk, PASRRIP_DATA_VALID_HEAD(policy_data)[new_rrpv], 
                PASRRIP_DATA_VALID_TAIL(policy_data)[new_rrpv]);
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

      blk->dirty   = (info && info->dirty) ? 1 : 0;
#if 0
      blk->access += 1;
#endif

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

int cache_get_fill_rrpv_pasrrip(pasrrip_data *policy_data, 
    pasrrip_gdata *global_data, memory_trace *info, ub1 *is_x_block)
{
  int new_rrpv;
  int tmp_rrpv;

#if 0
  if ((global_data->bm_ctr)++ == 0)
  {
    new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data) - 1;  
  }
  else
  {
    new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data);
  }

  if (global_data->bm_ctr == global_data->bm_thr)
  {
    global_data->bm_ctr = 0;
  }
#endif

  new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data) - 1;  

  switch (PASRRIP_DATA_CFPOLICY(policy_data))
  {
    case cache_policy_lru:
      return 0;

    case cache_policy_lip:
      return PASRRIP_DATA_MAX_RRPV(policy_data);

    case cache_policy_pasrrip:
      if (info->stream == CS)
      {
        new_rrpv = add_region_block(CS, info->stream, info->vtl_addr);
        tmp_rrpv = get_xstream_reuse_ratio(CS, TS, info->vtl_addr);

        if (tmp_rrpv != PASRRIP_DATA_MAX_RRPV(policy_data) && tmp_rrpv <= new_rrpv)
        {
          new_rrpv = tmp_rrpv;

          if (is_x_block)
          {
            *is_x_block = TRUE;
          }
        }
      }
      else
      {
        if (info->stream == BS)
        {
          new_rrpv = add_region_block(BS, info->stream, info->vtl_addr);
          tmp_rrpv = get_xstream_reuse_ratio(BS, TS, info->vtl_addr);

          if (tmp_rrpv != PASRRIP_DATA_MAX_RRPV(policy_data) && tmp_rrpv <= new_rrpv)
          {
            new_rrpv = tmp_rrpv;

            if (is_x_block)
            {
              *is_x_block = TRUE;
            }
          }
        }
        else
        {
          if (info->stream == ZS)
          {
            new_rrpv = add_region_block(ZS, info->stream, info->vtl_addr);
            tmp_rrpv = get_xstream_reuse_ratio(ZS, TS, info->vtl_addr);

            if (tmp_rrpv != PASRRIP_DATA_MAX_RRPV(policy_data) && tmp_rrpv <= new_rrpv)
            {
              new_rrpv = tmp_rrpv;

              if (is_x_block)
              {
                *is_x_block = TRUE;
              }
            }
          }
          else
          {
            if (info->stream == TS)
            {
              new_rrpv = add_region_block(CS, info->stream, info->vtl_addr);
              tmp_rrpv = add_region_block(BS, info->stream, info->vtl_addr);
              if (tmp_rrpv < new_rrpv)
              {
                new_rrpv = tmp_rrpv;
              }

              tmp_rrpv = add_region_block(ZS, info->stream, info->vtl_addr);
              if (tmp_rrpv < new_rrpv)
              {
                new_rrpv = tmp_rrpv;
              }

              tmp_rrpv = add_region_block(TS, info->stream, info->vtl_addr);
              if (tmp_rrpv < new_rrpv)
              {
                new_rrpv = tmp_rrpv;
              }
            }
          }
        }
      }

      return new_rrpv;

    case cache_policy_bypass:
      /* Not to insert */
      return BYPASS_RRPV;  

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }
}

int cache_get_replacement_rrpv_pasrrip(pasrrip_data *policy_data)
{
  return PASRRIP_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_pasrrip(pasrrip_data *policy_data, pasrrip_gdata *global_data, int old_rrpv, 
    memory_trace *info)
{
  int new_rrpv;
  int reuse_ratio;

#define ACCESS_TH (4) 
  
  if ((global_data->bm_ctr)++ == 0)
  {
    new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data) - 1;  
  }
  else
  {
    new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data);
  }
  
  if (global_data->bm_ctr == global_data->bm_thr)
  {
    global_data->bm_ctr = 0;
  }

  /* If access is from the texture stream */
  if (info->stream == TS)
  {
    sb4 tmp_rrpv;
    /* Lookup BT region */
#if 0
    reuse_ratio = add_region_block(BS, info->stream, info->vtl_addr);
#endif
    new_rrpv = add_region_block(BS, info->stream, info->vtl_addr);

    tmp_rrpv = add_region_block(CS, info->stream, info->vtl_addr);
    if (tmp_rrpv < new_rrpv)
    {
      new_rrpv = tmp_rrpv;
    }

    tmp_rrpv = add_region_block(TS, info->stream, info->vtl_addr);
    if (tmp_rrpv < new_rrpv)
    {
      new_rrpv = tmp_rrpv;
    }

    tmp_rrpv = add_region_block(ZS, info->stream, info->vtl_addr);
    if (tmp_rrpv < new_rrpv)
    {
      new_rrpv = tmp_rrpv;
    }

#if 0
#if 0
    if (reuse_ratio < ACCESS_TH)
#endif
    if (new_rrpv > 0)
    {
      /* Lookup CT region */
#if 0
      reuse_ratio = add_region_block(CS, info->stream, info->vtl_addr);
#endif
      new_rrpv = add_region_block(CS, info->stream, info->vtl_addr);
#if 0
      if (reuse_ratio < ACCESS_TH)
#endif
      if (new_rrpv > 0)
      {
        new_rrpv = add_region_block(TS, info->stream, info->vtl_addr);
#if 0
        /* Lookup TT region */
        reuse_ratio = add_region_block(TS, info->stream, info->vtl_addr);

        if (reuse_ratio < ACCESS_TH)
        {
          new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data);
        }
        else
        {
          new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data) - 1;
        }
#endif
      }
#if 0
      else
      {
        new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data) - 1;
      }
#endif
    }
#if 0
    else
    {
      new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data)- 1;
    }
#endif
#endif
  }

  if (info->stream == BS)
  {
    new_rrpv = add_region_block(BS, info->stream, info->vtl_addr);
#if 0
    reuse_ratio = add_region_block(BS, info->stream, info->vtl_addr);

    if (reuse_ratio < ACCESS_TH)
    {
      new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data); 
    }
    else
    {
      new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data)- 1;
    }
#endif
  }

  if (info->stream == CS)
  {
    new_rrpv = add_region_block(CS, info->stream, info->vtl_addr);
#if 0
    reuse_ratio = add_region_block(CS, info->stream, info->vtl_addr);

    if (reuse_ratio < ACCESS_TH)
    {
      new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data);
    }
    else
    {
      new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data)- 1;
    }
#endif
  }

  if (info->stream == ZS)
  {
    new_rrpv = add_region_block(ZS, info->stream, info->vtl_addr);
#if 0
    reuse_ratio = add_region_block(ZS, info->stream, info->vtl_addr);

    if (reuse_ratio < ACCESS_TH)
    {
      new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data);
    }
    else
    {
      new_rrpv = PASRRIP_DATA_MAX_RRPV(policy_data) - 1;
    }
#endif
  }
#undef ACCESS_TH 

  return new_rrpv;
}

/* Update state of block. */
void cache_set_block_pasrrip(pasrrip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  block = &(PASRRIP_DATA_BLOCKS(policy_data))[way];

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
  CACHE_REMOVE_FROM_QUEUE(block, PASRRIP_DATA_VALID_HEAD(policy_data)[old_rrpv],
      PASRRIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
  CACHE_APPEND_TO_SQUEUE(block, PASRRIP_DATA_FREE_HEAD(policy_data), 
      PASRRIP_DATA_FREE_TAIL(policy_data));
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_pasrrip(pasrrip_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  assert(policy_data);
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (PASRRIP_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (PASRRIP_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (PASRRIP_DATA_BLOCKS(policy_data)[way]).stream);

  return PASRRIP_DATA_BLOCKS(policy_data)[way];
}

/* Get tag and state of a block. */
int cache_count_block_pasrrip(pasrrip_data *policy_data, ub1 strm)
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
    head = PASRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head;

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
void cache_set_current_fill_policy_pasrrip(pasrrip_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  PASRRIP_DATA_CFPOLICY(policy_data) = policy;
}

/* Set policy for next access */
void cache_set_current_access_policy_pasrrip(pasrrip_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  PASRRIP_DATA_CAPOLICY(policy_data) = policy;
}

/* Set policy for next replacment */
void cache_set_current_replacement_policy_pasrrip(pasrrip_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_cpulast);

  PASRRIP_DATA_CRPOLICY(policy_data) = policy;
}
