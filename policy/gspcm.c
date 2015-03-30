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
#include <math.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "gspcm.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))
#define SAMPLED_SET             (0)
#define FOLLOWER_SET            (1)

#define CACHE_UPDATE_BLOCK_STATE(block, tag, state)           \
do                                                            \
{                                                             \
  (block)->tag   = (tag);                                     \
  (block)->state = (state);                                   \
}while(0)

#define CACHE_GSPCM_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)  \
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
          dif   = rrpv - i;                                   \
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
  (block)->stream = strm;                                     \
}while(0)

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

static int get_set_type_gspcm(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x0f;
  msb_bits = (indx >> 6) & 0x0f;
  mid_bits = (indx >> 4) & 0x03;

  if (lsb_bits == msb_bits && mid_bits == 0)
  {
    return SAMPLED_SET;
  }

  return FOLLOWER_SET;
}

static void print_counters(gspcm_gdata *global_data)
{
  /* Texture epoch 0 fill */
  printf("Text E0 Fill : %ld \n", SAT_CTR_VAL(global_data->tex_e0_fill_ctr));

  /* Texture epoch 0 hits */
  printf("Tex E0 Hit : %ld\n", SAT_CTR_VAL(global_data->tex_e0_hit_ctr));

  /* Texture epoch 1 fill */
  printf("Tex E1 Fill : %ld\n", SAT_CTR_VAL(global_data->tex_e1_fill_ctr));

  /* Texture epoch 1 hits */
  printf("Tex E1 Hit : %ld\n", SAT_CTR_VAL(global_data->tex_e1_hit_ctr));

  /* Depth fill */
  printf("Z E0 Fill : %ld\n", SAT_CTR_VAL(global_data->z_e0_fill_ctr));

  /* Depth hits */
  printf("Z E0 Hit : %ld\n", SAT_CTR_VAL(global_data->z_e0_hit_ctr));

  /* Render target produced */
  printf("RT Prod : %ld\n", SAT_CTR_VAL(global_data->rt_prod_ctr));

  /* Render target consumed */
  printf("RT Cons : %ld\n", SAT_CTR_VAL(global_data->rt_cons_ctr));

  /* Render target produced */
  printf("BT Prod : %ld\n", SAT_CTR_VAL(global_data->bt_prod_ctr));

  /* Render target consumed */
  printf("BT Cons : %ld\n", SAT_CTR_VAL(global_data->bt_cons_ctr));

  /* Total accesses */
  printf("Total Access : %ld", SAT_CTR_VAL(global_data->acc_all_ctr)); 
}

void cache_init_gspcm(long long int set_indx, struct cache_params *params, 
  gspcm_data *policy_data, gspcm_gdata *global_data)
{
  /* For GSPC blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define THRESHOLD       (params->threshold)
#define MEM_ALLOC(size) ((gspcm_list *)xcalloc(size, sizeof(gspcm_list)))

  /* Create per rrpv buckets */
  GSPCM_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  GSPCM_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

  /* Set max RRPV for the set */
  GSPCM_DATA_MAX_RRPV(policy_data)  = MAX_RRPV;
  GSPCM_DATA_THRESHOLD(policy_data) = THRESHOLD;

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    GSPCM_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    GSPCM_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    GSPCM_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  GSPCM_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

  /* Initialize array of blocks */
  GSPCM_DATA_FREE_HEAD(policy_data) = &(GSPCM_DATA_BLOCKS(policy_data)[0]);
  GSPCM_DATA_FREE_TAIL(policy_data) = &(GSPCM_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&GSPCM_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&GSPCM_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&GSPCM_DATA_BLOCKS(policy_data)[way])->next  = way ? 
      (&GSPCM_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&GSPCM_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&GSPCM_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV
#undef THRESHOLD
#undef CACHE_ALLOC
  
  if (get_set_type_gspcm(set_indx) == SAMPLED_SET)
  {
    /* Initialize srrip policy for the set */
    cache_init_srripm(params, &(policy_data->srrip_policy_data));
    policy_data->following = cache_policy_srrip;
  }
  else
  {
    policy_data->following = cache_policy_gspcm;
  }


  /* Fill and hit counter to get reuse probability */
  SAT_CTR_INI(global_data->tex_e0_fill_ctr, 8, 0, 255);  /* Texture epoch 0 fill */
  SAT_CTR_INI(global_data->tex_e0_hit_ctr, 8, 0, 255);   /* Texture epoch 0 hits */

  SAT_CTR_INI(global_data->tex_e1_fill_ctr, 8, 0, 255);  /* Texture epoch 1 fill */
  SAT_CTR_INI(global_data->tex_e1_hit_ctr, 8, 0, 255);   /* Texture epoch 1 hits */

  SAT_CTR_INI(global_data->z_e0_fill_ctr, 8, 0, 255);    /* Depth fill */
  SAT_CTR_INI(global_data->z_e0_hit_ctr, 8, 0, 255);     /* Depth hits */

  SAT_CTR_INI(global_data->rt_prod_ctr, 8, 0, 255);      /* Render target produced */
  SAT_CTR_INI(global_data->rt_cons_ctr, 8, 0, 255);      /* Render target consumed */

  SAT_CTR_INI(global_data->bt_prod_ctr, 8, 0, 255);      /* Render target produced */
  SAT_CTR_INI(global_data->bt_cons_ctr, 8, 0, 255);      /* Render target consumed */
  
  for (ub1 strm = 0; strm < TST + MAX_CORES; strm++)
  {
    SAT_CTR_INI(global_data->blk_fill[strm], 8, 0, 255);          /* BT blocks filled */
    SAT_CTR_INI(global_data->blk_reuse[strm], 8, 0, 255);         /* BT blocks evicted after reuse */
    SAT_CTR_INI(global_data->blk_no_reuse[strm], 8, 0, 255);      /* BT blocks evicted without reuse */
    
    global_data->total_blocks[strm] = 0;
    global_data->total_access[strm] = 0;
  }

  SAT_CTR_INI(global_data->acc_all_ctr, 7, 0, 127);      /* Total accesses */
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data)
{
  /* Free all data blocks */
  free(GSPCM_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (GSPCM_DATA_VALID_HEAD(policy_data))
  {
    free(GSPCM_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (GSPCM_DATA_VALID_TAIL(policy_data))
  {
    free(GSPCM_DATA_VALID_TAIL(policy_data));
  }
  
  /* Free SRRIP specific information */
  if (policy_data->following == cache_policy_srrip)
  {
    cache_free_srripm(&(policy_data->srrip_policy_data));
  }
}

ub8 get_reuse_ratio(gspcm_gdata *global_data, ub1 strm)
{
  ub8 reuse_ratio;

#define R_BLK(dat, strm)   (SAT_CTR_VAL(dat->blk_reuse[strm]))
#define N_BLK(dat, strm)   (SAT_CTR_VAL(dat->blk_no_reuse[strm]))
#define T_BLK(dat, strm)   (R_BLK(dat, strm) + N_BLK(dat, strm))
#define RR_BLK(dat, strm)  ((float)T_BLK(dat, strm) / R_BLK(dat, strm))

  /* Measure on average how many blocks are evicted after reuse */
  if (R_BLK(global_data, strm))
  {
    reuse_ratio = (ub8)ceil(RR_BLK(global_data, strm));
    assert(reuse_ratio);
  }
  else
  {
    reuse_ratio = 1;
  }

#undef R_BLK
#undef N_BLK
#undef T_BLK
#undef RR_BLK

  return reuse_ratio;
}

/* Block lookup */
struct cache_block_t * cache_find_block_gspcm(gspcm_data *policy_data, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;
  
  node = NULL;

  if (policy_data->following == cache_policy_srrip)
  {
    return cache_find_block_srripm(&(policy_data->srrip_policy_data), tag);
  }
  else
  {
    max_rrpv = policy_data->max_rrpv;

    for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
    {
      head = GSPCM_DATA_VALID_HEAD(policy_data)[rrpv].head;

      for (node = head; node; node = node->prev)
      {
        assert(node->state != cache_block_invalid);

        if (node->tag == tag)
          goto end;
      }
    }
  }

end:
  return node;
}

/* Block insertion */
void cache_fill_block_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int strm, 
  memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  assert(policy_data);
  assert(global_data);
  assert(way >= 0);
  assert(tag >= 0);
  assert(strm <= TST + MAX_CORES - 1);
  assert(state != cache_block_invalid);
  
  /* Get the cache block */
  block = &(GSPCM_DATA_BLOCKS(policy_data)[way]);
  assert(block);
  
  if (policy_data->following == cache_policy_srrip)
  {
    /* Follow RRIP policy */
    cache_fill_block_srripm(&(policy_data->srrip_policy_data), way, tag, 
      state, strm, info);

    /* Update global counters */
    cache_update_fill_counter_gspcm(&(policy_data->srrip_policy_data), 
      global_data, way, strm);

    global_data->total_blocks[strm] += 1;
  }
  else
  {
    /* Follow GSPC */
    assert(policy_data->following == cache_policy_gspcm);

    /* Remove block from free list */
    CACHE_REMOVE_FROM_SQUEUE(block, GSPCM_DATA_FREE_HEAD(policy_data),
      GSPCM_DATA_FREE_TAIL(policy_data));

    /* Update new block state and stream */
    CACHE_UPDATE_BLOCK_STATE(block, tag, state);
    CACHE_UPDATE_BLOCK_STREAM(block, strm);

    block->dirty  = (info && info->spill) ? 1 : 0;
    block->epoch  = 0;
    
    if (info && info->spill == TRUE)
    {
      if (strm == BS)
      {
        block->is_bt_block = TRUE;
      }
      else
      {
        block->is_bt_block = FALSE;
      }
    }

    /* Get RRPV to be assigned to the new block */
    rrpv = cache_get_fill_rrpv_gspcm(policy_data, global_data, strm);

    SAT_CTR_INC(global_data->blk_fill[strm]);

    /* Insert block in to the corresponding RRPV queue */
    CACHE_APPEND_TO_QUEUE(block, 
        GSPCM_DATA_VALID_HEAD(policy_data)[rrpv], 
        GSPCM_DATA_VALID_TAIL(policy_data)[rrpv]);
  }
}

/* Replace a block */
int cache_replace_block_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data,
    memory_trace *info)
{
  struct  cache_block_t *block;
  int     rrpv;
  ub8     avg_reuse;
  ub8     reuse_ratio;
  ub4     min_wayid;

  min_wayid = ~(0);

  if (policy_data->following == cache_policy_srrip)
  {
    int ret_way = cache_replace_block_srripm(&(policy_data->srrip_policy_data));

    /* If to be replaced block is BT block update bt_block counter */
    block = &(SRRIPM_DATA_BLOCKS(&(policy_data->srrip_policy_data))[ret_way]);

    /* If block is replaced */
#if 0
    if (block->state != cache_block_invalid && block->is_bt_block == TRUE)
#endif
      if (block->state != cache_block_invalid)
      {
        ub8 blk_access;
        ub1 strm;

        /* Get the stream of replacement candicate */
        strm = block->stream;

        reuse_ratio = get_reuse_ratio(global_data, strm);

        /* To avoid including blocks with no reuse in average reuse computation, 
         * we divide total blocks by reuse ratio */
        if (global_data->total_blocks[strm])
        {
#define T_ACC(data, strm)     (data->total_access[strm])
#define U_ACC(data, strm, rr) (T_ACC(data, strm) * rr)
#define T_BLK(data, strm)     (data->total_blocks[strm])
#define A_REU(dat, strm, rr)  ((float)U_ACC(dat, strm, rr) / T_BLK(dat, strm))

          avg_reuse = (int)(ceil(A_REU(global_data, strm, reuse_ratio)));

          if (reuse_ratio > 1)
          {
            printf("Stream %d : Average reuse %ld reuse ratio %ld\n", 
                strm, avg_reuse, reuse_ratio);
          }

#undef T_ACC
#undef U_ACC
#undef T_BLK
#undef A_REU
        }

        /* Reset block accesses */
        blk_access    = block->access;

        assert(global_data->total_access[strm] >= blk_access);
        assert(global_data->total_blocks[strm] > 0);

        global_data->total_blocks[strm] -= 1;
        global_data->total_access[strm] -= blk_access;

#if 0
        if (blk_access < avg_reuse)
#endif
        {
          if (blk_access == 0)
          {
            SAT_CTR_INC(global_data->blk_no_reuse[strm]);
          }
          else
          {
            SAT_CTR_INC(global_data->blk_reuse[strm]);

            if (blk_access < avg_reuse)
            {
              printf("Replaced %d block before avg reuse \n", strm);
            }

            if (blk_access < (int)ceil((float)avg_reuse / 4))
            {
              printf("Replaced %d block before reuse / 4 access\n", strm);
            }
            else
            {
              if (blk_access < (int)ceil((float)avg_reuse / 2))
              {
                printf("Replaced %d block before reuse / 2 access\n", strm);
              }
              else
              {
                if (blk_access > (int)ceil(3 * (float)avg_reuse / 4))
                {
                  printf("Replaced %d block after reuse * 3 / 4  access\n", strm);
                }
                else
                {
                  printf("Replaced %d block before reuse * 3 / 4 access\n", strm);
                }
              }
            }
          }
        }
      }

    return ret_way;
  }
  else
  {
    /* Try to find an invalid block. */
    for (block = policy_data->free_head; block; block = block->next)
      return block->way;

    /* Obtain RRPV from where to replace the block */
    rrpv = cache_get_replacement_rrpv_gspcm(policy_data);

    /* Ensure rrpv is with in max_rrpv bound */
    assert(rrpv >= 0 && rrpv <= GSPCM_DATA_MAX_RRPV(policy_data));

    /* If there is no block with required RRPV, increment RRPV of all the 
     * blocks until we get one with the required RRPV */
    if (GSPCM_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
    {
      CACHE_GSPCM_INCREMENT_RRPV(GSPCM_DATA_VALID_HEAD(policy_data),
          GSPCM_DATA_VALID_TAIL(policy_data), rrpv);
    }

    /* Remove a nonbusy block from the tail */
    reuse_ratio = get_reuse_ratio(global_data, info->stream);

    /* To avoid including blocks with no reuse in average reuse computation, 
     * we divide total blocks by reuse ratio */
    if (global_data->total_blocks[info->stream])
    {
#define T_ACC(data, strm)     (data->total_access[strm])
#define U_ACC(data, strm, rr) (T_ACC(data, strm) * rr)
#define T_BLK(data, strm)     (data->total_blocks[strm])
#define A_REU(dat, strm, rr)  ((float)U_ACC(dat, strm, rr) / T_BLK(dat, strm))

      avg_reuse = (int)(ceil(A_REU(global_data, info->stream, reuse_ratio)));

#undef T_ACC
#undef U_ACC
#undef T_BLK
#undef A_REU
    }

#define BLK_LIST_H(data, rrpv) (GSPCM_DATA_VALID_TAIL(data)[rrpv].head)

    /* Ties are broken by choosing block with smallest physical way id */
    for (block = BLK_LIST_H(policy_data, rrpv); block; block = block->next)
    {
      if (!block->busy && block->way < min_wayid && 
          block->access >= avg_reuse / 2)
      {
        min_wayid = block->way;
      }
    }

#undef BLK_LIST_H

    for (block = GSPCM_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
    {
      if (!block->busy && block->way < min_wayid)
        min_wayid = block->way;
    }

    /* If no non busy block can be found, return -1 */
    return (min_wayid != ~(0)) ? min_wayid : -1;
  }
}

/* Aging policy implementation */
void cache_access_block_gspcm(gspcm_data *policy_data, 
  gspcm_gdata *global_data, int way, int strm, memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int old_rrpv;
  int new_rrpv;
  ub8 reuse_ratio;

  assert(policy_data);
  assert(global_data);
  assert(way >= 0);
  assert(strm <= TST + MAX_CORES - 1);
  
  if (policy_data->following == cache_policy_srrip)
  {
    blk = &(SRRIPM_DATA_BLOCKS(&(policy_data->srrip_policy_data))[way]);
    assert(blk);
    
    /* Update global counters */
    if (info && info->fill == TRUE)
    {
      cache_update_hit_counter_gspcm(&(policy_data->srrip_policy_data), 
        global_data, way, strm); 
    }

    if (blk->stream != strm)
    {
      global_data->total_blocks[strm]         += 1;
      global_data->total_blocks[blk->stream]  -= 1;

      assert(global_data->total_access[blk->stream] >= blk->access);
      
      global_data->total_access[blk->stream]  -= blk->access; 
      
      /* First access of new  stream*/
      blk->access = 0;
    }

    global_data->total_access[strm] += 1; 

    /* Follow SRRIP policy */
    cache_access_block_srripm(&(policy_data->srrip_policy_data), way, strm, 
      info);
  }
  else
  { 
    blk  = &(GSPCM_DATA_BLOCKS(policy_data)[way]);
    prev = blk->prev;
    next = blk->next;

    /* Check: block's tag and state are valid */
    assert(blk->tag >= 0);
    assert(blk->state != cache_block_invalid);

    /* Follow GSPC policy */
    assert(policy_data->following == cache_policy_gspcm);

    blk->dirty   = (info && info->spill) ? 1 : 0;

    if (info && info->spill == TRUE)
    {
      if (strm == BS && info->spill == TRUE)
      {
        blk->is_bt_block = TRUE;
      }
    }
    
    /* Obtain fraction of blocks reused (measure in sampled sets) */
    reuse_ratio = get_reuse_ratio(global_data, strm);

#if 0
    if (info->fill == TRUE)
#endif
    {
      /* Get old RRPV from the block */
      old_rrpv = (((gspcm_list *)(blk->data))->rrpv);

      /* Total accesses are normalized using reuse ratio to avoid 
       * counting blocks with no reuse in the measurement of 
       * average reuse */
#define T_ACC(data, rr, st) ((data)->total_access[st] * rr)
#define T_BLK(data, st)     ((data)->total_blocks[st])
#define A_ACC(data, rr, st) ((float)T_ACC(data, rr, st) / T_BLK(data, st))

      /* If blocks use has exceeded average reuse, set its RRPV to distant */
      if (T_BLK(global_data, strm) && 
        blk->access > (int)ceil(A_ACC(global_data, reuse_ratio, strm)))
      {
        new_rrpv = GSPCM_DATA_MAX_RRPV(policy_data);
      }
      else
      {
        new_rrpv = 0;
      }

#undef T_ACC
#undef T_BLK
#undef A_ACC

      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        CACHE_REMOVE_FROM_QUEUE(blk, GSPCM_DATA_VALID_HEAD(policy_data)[old_rrpv],
            GSPCM_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, GSPCM_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            GSPCM_DATA_VALID_TAIL(policy_data)[new_rrpv]);
      }

      if (info && (blk->stream == info->stream))
      {
        blk->epoch = (blk->epoch == 3) ? 3 : blk->epoch + 1;
      }
      else
      {
        blk->epoch = 0;
      }
    }
#if 0
    else
    {
      /* Get old RRPV from the block */
      old_rrpv = (((gspcm_list *)(blk->data))->rrpv);
      
      /* To avoid thrashing, we use reuse ratio to decide, if, block should be 
       * inserted with RRPV 0 */
      if (SAT_CTR_VAL(global_data->blk_fill[strm]) > reuse_ratio)
      {
        new_rrpv = 0;
        SAT_CTR_SET(global_data->blk_fill[strm], 0);
      }
      else
      { 
        new_rrpv = GSPCM_DATA_MAX_RRPV(policy_data) - 1; 
      }

      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        CACHE_REMOVE_FROM_QUEUE(blk, GSPCM_DATA_VALID_HEAD(policy_data)[old_rrpv],
            GSPCM_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, GSPCM_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            GSPCM_DATA_VALID_TAIL(policy_data)[new_rrpv]);
      }
    }
#endif
    CACHE_UPDATE_BLOCK_STREAM(blk, strm);
  }
}

/* Get replacement RRPV */
int cache_get_replacement_rrpv_gspcm(gspcm_data *policy_data)
{
  return GSPCM_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data, int way, int strm)
{
  /* Current GSPC specific block state */
  unsigned int state;
   
  state = GSPCM_DATA_BLOCKS(policy_data)[way].block_state;

  switch (strm)    
  {
    /* Color stream */
    case CS:
    
      /* Promote RT block to RRPV 0 */
      return 0;

      break;

    /* Texture stream */
    case TS:

#define HI_CT0(data) ((float)SAT_CTR_VAL(data->tex_e0_hit_ctr))
#define FI_CT0(data) ((float)SAT_CTR_VAL(data->tex_e0_fill_ctr))
#define HI_CT1(data) ((float)SAT_CTR_VAL(data->tex_e1_hit_ctr))
#define FI_CT1(data) ((float)SAT_CTR_VAL(data->tex_e1_fill_ctr))
#define TX_BLK (0)
#define RT_BLK (3)

      if (state == TX_BLK)
      {
        /* If reuse probability of epoch 0 is below the threshold */
        if ((FI_CT1(global_data) / HI_CT1(global_data)) > policy_data->threshold)
        {
          /* Insert block with max RRPV */
          return GSPCM_DATA_MAX_RRPV(policy_data); 
        }
        else
        {
          /* Insert block with 0 RRPV */
          return !(GSPCM_DATA_MAX_RRPV(policy_data));
        }
      }
      else
      {
        if (state == RT_BLK)
        {
          /* If reuse probability of epoch 0 is below the threshold */
          if ((FI_CT0(global_data) / HI_CT0(global_data)) > policy_data->threshold)
          {
            /* Insert block with max RRPV */
            return GSPCM_DATA_MAX_RRPV(policy_data); 
          }
          else
          {
            /* Insert block with 0 RRPV */
            return !(GSPCM_DATA_MAX_RRPV(policy_data));
          }
        }
        else
        {
          return 0;
        }
      }

#undef HI_CT0
#undef FI_CT0
#undef HI_CT1
#undef FI_CT1
#undef TX_BLK
#undef RT_BLK

      break;

    case ZS:
      
      /* Promote block to RRPV 0 */
      return 0;

      break;
  }

  return GSPCM_DATA_MAX_RRPV(policy_data) - 1;
}

/* Update fill counter */
void cache_update_fill_counter_gspcm(srripm_data *policy_data, gspcm_gdata *global_data, int way, int strm)
{
  /* GSPC counter updates 
   *
   * if (access is miss)
   * - If stream == TS state = 00 TS E0 FILL++ ACC++
   * - If stream == ZS state = 00 ZS E0 FILL++ ACC++
   * - If stream == CS state = 11 ACC++ PROD++
   * - else ACC++
   *
   */

  if (strm == TS)
  {
    SAT_CTR_INC(global_data->tex_e0_fill_ctr);
    SRRIPM_DATA_BLOCKS(policy_data)[way].block_state = 0;
  }
  else
  {
    if (strm == ZS)
    {
      SAT_CTR_INC(global_data->z_e0_fill_ctr);
      SRRIPM_DATA_BLOCKS(policy_data)[way].block_state = 0;
    }
    else
    {
      if (strm == CS)
      {
        SRRIPM_DATA_BLOCKS(policy_data)[way].block_state = 3;
        SAT_CTR_INC(global_data->rt_prod_ctr);
      }
      else
      {
        if (strm == BS)
        {
          SRRIPM_DATA_BLOCKS(policy_data)[way].block_state = 3;
          SAT_CTR_INC(global_data->bt_prod_ctr);
        }
        else
        {
          SRRIPM_DATA_BLOCKS(policy_data)[way].block_state = 0;
        }
      }
    }
  }

  SAT_CTR_INC(global_data->acc_all_ctr);

  /* If access counter is saturated, half all all other counters */
  if (IS_CTR_SAT(global_data->acc_all_ctr))
  {
#if 0
    print_counters(global_data);
#endif
    SAT_CTR_HLF(global_data->tex_e0_fill_ctr);
    SAT_CTR_HLF(global_data->tex_e0_hit_ctr);
    SAT_CTR_HLF(global_data->tex_e1_fill_ctr);
    SAT_CTR_HLF(global_data->tex_e1_hit_ctr);
    SAT_CTR_HLF(global_data->z_e0_fill_ctr);
    SAT_CTR_HLF(global_data->z_e0_hit_ctr);
    SAT_CTR_HLF(global_data->rt_cons_ctr);
    SAT_CTR_HLF(global_data->rt_prod_ctr);
    
    for (ub1 strm = 0; strm <= TST; strm++)
    {
      if (SAT_CTR_VAL(global_data->blk_reuse[strm]) > 128)
      {
        SAT_CTR_HLF(global_data->blk_reuse[strm]);
      }

      if (SAT_CTR_VAL(global_data->blk_no_reuse[strm]) > 128)
      {
        SAT_CTR_HLF(global_data->blk_no_reuse[strm]);
      }
    }

    if (SAT_CTR_VAL(global_data->bt_cons_ctr) >=  global_data->prev_bt_cons)
    {
      SAT_CTR_HLF(global_data->bt_cons_ctr);
      global_data->prev_bt_cons = SAT_CTR_VAL(global_data->bt_cons_ctr);
    }

    if (SAT_CTR_VAL(global_data->bt_prod_ctr) >= global_data->prev_bt_prod)
    {
      SAT_CTR_HLF(global_data->bt_prod_ctr);
      global_data->prev_bt_prod = SAT_CTR_VAL(global_data->bt_prod_ctr);
    }

    SAT_CTR_SET(global_data->acc_all_ctr, 0);
  }
}

/* Update hit counter */
void cache_update_hit_counter_gspcm(srripm_data *policy_data, gspcm_gdata *global_data, int way, int strm)
{
  /* GSPC counter updates 
   *
   * if (access is hit)
   * - If stream == ZS state == 00 ZS E0 HIT++ state 01 ACC++
   * - If stream == ZS state == 01 state 10 ACC++
   * - If stream == ZS state == 10 state 10 ACC++
   * - If stream == TS state == 11 state 00 TS E0 FILL++ ACC++ CONS++
   * - If stream == TS state == 00 state 01 TS E0 HIT++ ACC++
   * - If stream == TS state == 01 state 10 TS E1 HIT++ ACC++
   * - else state = 00 ACC++
   *
   *
   */
  struct cache_block_t *block;
  
  block = &(SRRIPM_DATA_BLOCKS(policy_data)[way]);
  switch (strm)
  {
    case ZS:

      if (block->block_state == 0) 
      {
        block->block_state = 1;
        SAT_CTR_INC(global_data->z_e0_hit_ctr);
      }
      else
      {
        if (block->block_state == 1 || block->block_state == 2)
        {
          block->block_state = 1;
        }
      }
      break;

    case TS:

      if (block->block_state == 0)
      {
        SAT_CTR_INC(global_data->tex_e0_hit_ctr);
        SAT_CTR_INC(global_data->tex_e1_fill_ctr);
        block->block_state = 1;
      }
      else
      {
        if (block->block_state == 1)
        {
          SAT_CTR_INC(global_data->tex_e1_hit_ctr);
          block->block_state = 2;
        }
        else
        {
          if (block->block_state == 3)
          {
            SAT_CTR_INC(global_data->tex_e0_fill_ctr);

            if (block->stream == BS)
            {
              SAT_CTR_INC(global_data->bt_cons_ctr);
            }
            else
            {
              SAT_CTR_INC(global_data->rt_cons_ctr);
            }

            block->block_state = 0;
          }
        }
      }

      break;
  }

  SAT_CTR_INC(global_data->acc_all_ctr);

  /* If access counter is saturated, half all all other counters */
  if (IS_CTR_SAT(global_data->acc_all_ctr))
  {
    SAT_CTR_HLF(global_data->tex_e0_fill_ctr);
    SAT_CTR_HLF(global_data->tex_e0_hit_ctr);
    SAT_CTR_HLF(global_data->tex_e1_fill_ctr);
    SAT_CTR_HLF(global_data->tex_e1_hit_ctr);
    SAT_CTR_HLF(global_data->z_e0_fill_ctr);
    SAT_CTR_HLF(global_data->z_e0_hit_ctr);
    SAT_CTR_HLF(global_data->rt_cons_ctr);
    SAT_CTR_HLF(global_data->rt_prod_ctr);

    SAT_CTR_SET(global_data->acc_all_ctr, 0);
  }
}

/* New RRPV */
int cache_get_fill_rrpv_gspcm(
  gspcm_data  *policy_data, 
  gspcm_gdata *global_data, 
  int          strm)
{
  ub8 reuse_ratio;

  /*
   * CS block is filled with RRPV 0
   * TS block if reuse probability of epoc 0 < threshold 3 or 0
   * ZS block if reuse probability of epoc 0 < threshold 3 else 2
   */

  assert(strm <= TST + MAX_CORES);

  reuse_ratio = get_reuse_ratio(global_data, strm);

  if (SAT_CTR_VAL(global_data->blk_fill[strm]) > reuse_ratio)
  {
    /* Reset fill counter */
    SAT_CTR_SET(global_data->blk_fill[strm], 0);

    /* Fill block with min RRPV */
    return 0;
  }
  else
  { 
    /* Fill block with max RRPV - 1 */
    return GSPCM_DATA_MAX_RRPV(policy_data) - 1; 
  }

  switch (strm)    
  {
    case CS:

#define PROD_CTR(data) ((float)SAT_CTR_VAL(data->rt_prod_ctr))
#define CONS_CTR(data) ((float)SAT_CTR_VAL(data->rt_cons_ctr))

      //printf("Prod %d Cons %d\n", global_data->rt_prod_ctr, global_data->rt_cons_ctr);
      /* If production, consumption ratio is below threshold */
      if ((ub4)(PROD_CTR(global_data) / CONS_CTR(global_data)) > policy_data->threshold)
      {
        /* Insert block with max RRPV */
        return GSPCM_DATA_MAX_RRPV(policy_data);
      }
      else
      {
        /* If prodution, cinsmption ratio is below half of the threshold */
        if ((ub4)(PROD_CTR(global_data) / CONS_CTR(global_data)) > (policy_data->threshold / 2))
        {
          /* Insert block with distant RRPV */
          return GSPCM_DATA_MAX_RRPV(policy_data) - 1;
        }
        else
        {
          /* Insert block with RROV 0 */
          return 0;
        }
      }

#undef PROD_CTR
#undef CONS_CTR

      break;

    case BS:

#define PROD_CTR(data) ((float)SAT_CTR_VAL(data->bt_prod_ctr))
#define CONS_CTR(data) ((float)SAT_CTR_VAL(data->bt_cons_ctr))

      /* If production, consumption ratio is below threshold */
      if (!CONS_CTR(global_data) || 
          (CONS_CTR(global_data) && ((ub4)(PROD_CTR(global_data) / CONS_CTR(global_data))) > policy_data->threshold))
      {
        /* Insert block with max RRPV */
        return GSPCM_DATA_MAX_RRPV(policy_data);
      }
      else
      {
        /* If prodution, cinsmption ratio is below half of the threshold */
        if ((ub4)(PROD_CTR(global_data) / CONS_CTR(global_data)) > (policy_data->threshold / 2))
        {
          /* Insert block with distant RRPV */
          return GSPCM_DATA_MAX_RRPV(policy_data) - 1;
        }
        else
        {
          /* Insert block with RROV 0 */
          return 0;
        }
      }

#undef PROD_CTR
#undef CONS_CTR

      break;

    case TS:

#define HI_CT0(data) ((float)SAT_CTR_VAL(data->tex_e0_hit_ctr))
#define FI_CT0(data) ((float)SAT_CTR_VAL(data->tex_e0_fill_ctr))

      /* If reuse probability of epoch 0 is below the threshold */
      if ((FI_CT0(global_data) / HI_CT0(global_data)) > policy_data->threshold)
      {
        /* Insert block with max RRPV */
        return GSPCM_DATA_MAX_RRPV(policy_data); 
      }
      else
      {
        /* Insert block with 0 RRPV */
        return !(GSPCM_DATA_MAX_RRPV(policy_data));
      }

#undef HI_CT0
#undef FI_CT0

      break;

    case ZS:

#define HI_CT(data) ((float)SAT_CTR_VAL(data->z_e0_hit_ctr))
#define FI_CT(data) ((float)SAT_CTR_VAL(data->z_e0_fill_ctr))

      /* If reuse probability is below the threshold */
      if ((FI_CT(global_data) / HI_CT(global_data)) < policy_data->threshold)
      {
        /* Insetrt block with maximum RRPV */
        return GSPCM_DATA_MAX_RRPV(policy_data); 
      }
      else
      {
        /* Insert block with distant RRPV */
        return GSPCM_DATA_MAX_RRPV(policy_data) - 1;
      }

#undef HI_CT
#undef FI_CT

      break;
  }

  return GSPCM_DATA_MAX_RRPV(policy_data) - 1;
}


/* Update state of the block. */
void cache_set_block_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream,
  memory_trace *info)
{
  struct cache_block_t *block;
  
  if (policy_data->following == cache_policy_srrip)
  {
    cache_set_block_srripm(&(policy_data->srrip_policy_data), way, tag, state,
      stream, info);
  }
  else
  {
    block = &(GSPCM_DATA_BLOCKS(policy_data))[way];

    /* Check: tag matches with the block's tag. */
    assert(block->tag == tag);

    /* Check: block must be in valid state. It is not possible to set
     * state for an invalid block.*/
    assert(block->state != cache_block_invalid);
    
    /* Assign access stream */
    block->stream = stream;

    if (state != cache_block_invalid)
    {
      block->state = state;
      return;
    }

    /* Invalidate block */
    block->state = cache_block_invalid;
    block->epoch = 0;

    /* Get old RRPV from the block */
    int old_rrpv = (((gspcm_list *)(block->data))->rrpv);

    /* Remove block from valid list and insert into free list */
    CACHE_REMOVE_FROM_QUEUE(block, GSPCM_DATA_VALID_HEAD(policy_data)[old_rrpv],
        GSPCM_DATA_VALID_TAIL(policy_data)[old_rrpv]);
    CACHE_APPEND_TO_SQUEUE(block, GSPCM_DATA_FREE_HEAD(policy_data), 
        GSPCM_DATA_FREE_TAIL(policy_data));
  }
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_gspcm(gspcm_data *policy_data, 
  gspcm_gdata *global_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  if (policy_data->following == cache_policy_srrip)
  {
    return cache_get_block_srripm(&(policy_data->srrip_policy_data), way, tag_ptr,
      state_ptr, stream_ptr);
  } 
  else
  {
    PTR_ASSIGN(tag_ptr, (GSPCM_DATA_BLOCKS(policy_data)[way]).tag);
    PTR_ASSIGN(state_ptr, (GSPCM_DATA_BLOCKS(policy_data)[way]).state);
    PTR_ASSIGN(stream_ptr, (GSPCM_DATA_BLOCKS(policy_data)[way]).stream);
  }

  return GSPCM_DATA_BLOCKS(policy_data)[way];
}
