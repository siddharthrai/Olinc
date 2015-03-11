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
#include "gspct.h"
#include <math.h>

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))
#define SRRIPM_SAMPLED_SET      (0)
#define SRRIPT_SAMPLED_SET      (1)
#define FOLLOWER_SET            (2)

#define CACHE_UPDATE_BLOCK_STATE(block, tag, state)           \
do                                                            \
{                                                             \
  (block)->tag   = (tag);                                     \
  (block)->state = (state);                                   \
}while(0)

#define CACHE_GSPCT_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)  \
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

static int get_set_type_gspct(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x0f;
  msb_bits = (indx >> 6) & 0x0f;
  mid_bits = (indx >> 4) & 0x03;

  if (lsb_bits == msb_bits && mid_bits == 0)
  {
    return SRRIPM_SAMPLED_SET;
  }
  else
  {
    if (lsb_bits == (~msb_bits & 0x0f) && mid_bits == 0)
    {
      return SRRIPT_SAMPLED_SET;
    }
  }

  return FOLLOWER_SET;
}

static void print_counters(gspct_gdata *global_data)
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

void cache_init_gspct(long long int set_indx, struct cache_params *params, gspct_data *policy_data, gspct_gdata *global_data)
{
  /* For GSPC blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define THRESHOLD       (params->threshold)
#define MEM_ALLOC(size) ((gspct_list *)xcalloc(size, sizeof(gspct_list)))

  /* Create per rrpv buckets */
  GSPCT_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  GSPCT_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

  /* Set max RRPV for the set */
  GSPCT_DATA_MAX_RRPV(policy_data)  = MAX_RRPV;
  GSPCT_DATA_THRESHOLD(policy_data) = THRESHOLD;

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    GSPCT_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    GSPCT_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    GSPCT_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  GSPCT_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

  /* Initialize array of blocks */
  GSPCT_DATA_FREE_HEAD(policy_data) = &(GSPCT_DATA_BLOCKS(policy_data)[0]);
  GSPCT_DATA_FREE_TAIL(policy_data) = &(GSPCT_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&GSPCT_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&GSPCT_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&GSPCT_DATA_BLOCKS(policy_data)[way])->next  = way ? 
      (&GSPCT_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&GSPCT_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&GSPCT_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV
#undef THRESHOLD
#undef CACHE_ALLOC
  
  if (get_set_type_gspct(set_indx) == SRRIPM_SAMPLED_SET)
  {
    /* Initialize srrip policy for the set */
    cache_init_srripm(params, &(policy_data->srripm_policy_data));
    policy_data->following = cache_policy_srripm;
  }
  else
  {
    if (get_set_type_gspct(set_indx) == SRRIPT_SAMPLED_SET)
    {
      /* Initialize srrip policy for the set */
      cache_init_srript(params, &(policy_data->srript_policy_data));
      policy_data->following = cache_policy_srript;
    }
    else
    {
      /* Initialize gspc policy for the set */
      policy_data->following = cache_policy_gspct;
    }
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

  SAT_CTR_INI(global_data->bt_fill, 8, 0, 255);          /* BT blocks filled */
  SAT_CTR_INI(global_data->bt_reuse, 8, 0, 255);         /* BT blocks evicted after reuse */
  SAT_CTR_INI(global_data->bt_no_reuse, 8, 0, 255);      /* BT blocks evicted without reuse */

  SAT_CTR_INI(global_data->acc_all_ctr, 7, 0, 127);      /* Total accesses */
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_gspct(gspct_data *policy_data, gspct_gdata *global_data)
{
  /* Free all data blocks */
  free(GSPCT_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (GSPCT_DATA_VALID_HEAD(policy_data))
  {
    free(GSPCT_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (GSPCT_DATA_VALID_TAIL(policy_data))
  {
    free(GSPCT_DATA_VALID_TAIL(policy_data));
  }
  
  /* Free SRRIPM specific information */
  if (policy_data->following == cache_policy_srripm)
  {
    cache_free_srripm(&(policy_data->srripm_policy_data));
  }

  /* Free SRRIPT specific information */
  if (policy_data->following == cache_policy_srript)
  {
    cache_free_srript(&(policy_data->srript_policy_data));
  }
}

/* Block lookup */
struct cache_block_t* cache_find_block_gspct(gspct_data *policy_data, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;
  
  node = NULL;

  if (policy_data->following == cache_policy_srripm)
  {
    return cache_find_block_srripm(&(policy_data->srripm_policy_data), tag);
  }
  else
  {
    if (policy_data->following == cache_policy_srript)
    {
      return cache_find_block_srript(&(policy_data->srript_policy_data), tag);
    }
    else
    {
      max_rrpv = policy_data->max_rrpv;

      for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
      {
        head = GSPCT_DATA_VALID_HEAD(policy_data)[rrpv].head;

        for (node = head; node; node = node->prev)
        {
          assert(node->state != cache_block_invalid);

          if (node->tag == tag)
            goto end;
        }
      }
    }
  }

end:
  return node;
}

/* Block insertion */
void cache_fill_block_gspct(gspct_data *policy_data, gspct_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int strm, 
  memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  assert(policy_data);
  assert(global_data);
  assert(way >= 0);
  assert(tag >= 0);
  assert(strm <= TST);
  assert(state != cache_block_invalid);
  
  /* Get the cache block */
  block = &(GSPCT_DATA_BLOCKS(policy_data)[way]);
  assert(block);
  
  if (policy_data->following == cache_policy_srripm)
  {
    /* Follow RRIP policy */
    cache_fill_block_srripm(&(policy_data->srripm_policy_data), way, tag, 
      state, strm, info);
    
    if (info && (strm == BS && info->spill == TRUE))
    {
      global_data->bt_blocks += 1;
    }
  }
  else
  {
    if (policy_data->following == cache_policy_srript)
    {
      /* Update global counters */
      cache_update_fill_counter_srript(&(policy_data->srript_policy_data), 
        global_data, way, strm);

      /* Follow RRIP policy */
      cache_fill_block_srript(&(policy_data->srript_policy_data), way, tag, 
          state, strm, info);
    }
    else
    {
      /* Follow GSPC */
      assert(policy_data->following == cache_policy_gspct);

      /* Remove block from free list */
      CACHE_REMOVE_FROM_SQUEUE(block, GSPCT_DATA_FREE_HEAD(policy_data),
          GSPCT_DATA_FREE_TAIL(policy_data));

      /* Update new block state and stream */
      CACHE_UPDATE_BLOCK_STATE(block, tag, state);
      CACHE_UPDATE_BLOCK_STREAM(block, strm);

      block->dirty  = (info && info->spill) ? 1 : 0;
      block->epoch  = 0;
      block->access = 0;

      if (info && (strm == BS && info->spill == TRUE))
      {
        block->is_bt_block = TRUE;
        SAT_CTR_INC(global_data->bt_fill);
      }
      else
      {
        block->is_bt_block = FALSE;
      }

      /* Get RRPV to be assigned to the new block */
      rrpv = cache_get_fill_rrpv_gspct(policy_data, global_data, strm);

      /* Insert block in to the corresponding RRPV queue */
      CACHE_APPEND_TO_QUEUE(block, 
          GSPCT_DATA_VALID_HEAD(policy_data)[rrpv], 
          GSPCT_DATA_VALID_TAIL(policy_data)[rrpv]);
    }
  }
}

/* Block replacement */
int cache_replace_block_gspct(gspct_data *policy_data, gspct_gdata *global_data)
{
  struct  cache_block_t *block;
  int     rrpv;
  ub8     avg_reuse;
  ub8     reuse_ratio;
  
  if (SAT_CTR_VAL(global_data->bt_no_reuse))
  {
    reuse_ratio = (int)ceil((float)(SAT_CTR_VAL(global_data->bt_no_reuse) + 
      SAT_CTR_VAL(global_data->bt_reuse)) / SAT_CTR_VAL(global_data->bt_reuse));
  }
  else
  {
    reuse_ratio = 1;
  }

  if (global_data->bt_blocks)
  {
    avg_reuse = (int)(ceil((float)(global_data->bt_access * reuse_ratio) / (global_data->bt_blocks)));
  
    printf("Average reuse %ld reuse ratio %ld\n", avg_reuse, reuse_ratio);
  }
  
  if (policy_data->following == cache_policy_srripm)
  {
    int ret_way = cache_replace_block_srripm(&(policy_data->srripm_policy_data));

    /* If to be replaced block is BT block update bt_block counter */
    block = &(SRRIPM_DATA_BLOCKS(&(policy_data->srripm_policy_data))[ret_way]);

    if (block->state != cache_block_invalid && block->is_bt_block == TRUE)
    {
      ub8 bt_access;

      bt_access = block->access;

      assert(global_data->bt_access >= bt_access);
      assert(global_data->bt_blocks > 0);

      global_data->bt_blocks  -= 1;
      global_data->bt_access  -= bt_access;
      
      if (bt_access < avg_reuse)
      {
        if (bt_access == 0)
        {
          printf("Replaced BT block with no reuse %ld\n", bt_access);
          SAT_CTR_INC(global_data->bt_no_reuse);
        }
        else
        {
          SAT_CTR_INC(global_data->bt_reuse);

          if (bt_access < avg_reuse)
          {
            printf("Replaced BT block before avg reuse \n");
          }

          if (bt_access < (int)ceil((float)avg_reuse / 4))
          {
            printf("Replaced BT block before reuse / 4 access\n");
          }
          else
          {
            if (bt_access < (int)ceil((float)avg_reuse / 2))
            {
              printf("Replaced BT block before reuse / 2 access\n");
            }
            else
            {
              if (bt_access > (int)ceil(3 * (float)avg_reuse / 4))
              {
                printf("Replaced BT block before reuse * 3 / 4  access\n");
              }
              else
              {
                printf("Replaced BT block before reuse > * 3 / 4 access\n");
              }
            }
          }
        }
      }
      else
      {
        printf("Replaced BT block after reuse \n");
      }
    }

    return ret_way;
  }
  else
  {
    if (policy_data->following == cache_policy_srript)
    {
      return cache_replace_block_srript(&(policy_data->srript_policy_data));
    }
    else
    {
      /* Try to find an invalid block. */
      for (block = policy_data->free_head; block; block = block->next)
        return block->way;

      /* Obtain RRPV from where to replace the block */
      rrpv = cache_get_replacement_rrpv_gspct(policy_data);

      /* Ensure rrpv is woth in max_rrpv bound */
      assert(rrpv >= 0 && rrpv <= GSPCT_DATA_MAX_RRPV(policy_data));

      /* If there is no block with required RRPV, increment RRPV of all the blocks
       * until we get one with the required RRPV */
      if (GSPCT_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
      {
        CACHE_GSPCT_INCREMENT_RRPV(GSPCT_DATA_VALID_HEAD(policy_data),
            GSPCT_DATA_VALID_TAIL(policy_data), rrpv);
      }

      /* Remove a nonbusy block from the tail */
      unsigned int min_wayid = ~(0);
      ub8      bt_access;
      int      bt_way;
      struct cache_block_t *rep_block;

      bt_way    = -1;
      rep_block = NULL;

      /* Remove a nonbusy block from the tail */
      for (block = GSPCT_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
      {
        if (!block->busy && block->way < min_wayid && block->is_bt_block == FALSE)
        {
          min_wayid = block->way;
          bt_access = block->access;
          rep_block = block;
        }
        else
        {
          if (block->is_bt_block == TRUE && bt_way == -1)
          {
            bt_way = block->way;
          }
        }
      }
#if 0
      if (bt_access == 0)
      {
        printf("Replaced BT block with no reuse %ld\n", bt_access);
      }
      else
      {
        if (bt_access < (int)ceil((float)avg_reuse / 4))
        {
          printf("Replaced BT block before reuse / 4 access\n");
        }
        else
        {
          if (bt_access < (int)ceil((float)avg_reuse / 2))
          {
            printf("Replaced BT block before reuse / 2 access\n");
          }
          else
          {
            if (bt_access > (int)ceil(3 * (float)avg_reuse / 4))
            {
              printf("Replaced BT block before reuse * 3 / 4  access\n");
            }
            else
            {
              printf("Replaced BT block before reuse > * 3 / 4 access\n");
            }
          }
        }
      }
#endif

      int bt_blocks_with_no_access = 0;

      for (block = GSPCT_DATA_VALID_TAIL(policy_data)[GSPCT_DATA_MAX_RRPV(policy_data)].head; block; block = block->next)
      {
        if (block->is_bt_block && block->access == 0)
        {
          bt_blocks_with_no_access += 1;
        }
      }

      for (block = GSPCT_DATA_VALID_TAIL(policy_data)[GSPCT_DATA_MAX_RRPV(policy_data) - 1].head; block; block = block->next)
      {
        if (block->is_bt_block && block->access == 0)
        {
          bt_blocks_with_no_access += 1;
        }
      }

      if (rep_block)
      {
        if (bt_blocks_with_no_access && rep_block->stream == TS)
        {
          printf("BT blocks with no access %d %lld\n", bt_blocks_with_no_access, 
              rep_block->tag);
        }
      }

      /* If no non busy block can be found, return -1 */
      return (min_wayid != ~(0)) ? min_wayid : (bt_way != -1) ? bt_way : -1;
    }
  }
}

/* Block aging */
void cache_access_block_gspct(gspct_data *policy_data, 
  gspct_gdata *global_data, int way, int strm, memory_trace *info)
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
  assert(strm <= TST);
  
  if (SAT_CTR_VAL(global_data->bt_no_reuse))
  {
    reuse_ratio = (int)ceil((float)(SAT_CTR_VAL(global_data->bt_no_reuse) + 
      SAT_CTR_VAL(global_data->bt_reuse)) / SAT_CTR_VAL(global_data->bt_reuse));
  }
  else
  {
    reuse_ratio = 1;
  }

  if (policy_data->following == cache_policy_srripm)
  {
    blk = &(SRRIPM_DATA_BLOCKS(&(policy_data->srripm_policy_data))[way]);
    assert(blk);
    
    /* For BT block in the sampled set increase access count */
    if (blk->is_bt_block)
    {
      global_data->bt_access += 1; 
      assert(global_data->bt_access >= blk->access);
    }
    else
    {
      if (info && (strm == BS && info->spill == TRUE))
      {
        global_data->bt_blocks += 1;
        global_data->bt_access += 1; 
      }
    }

    /* Follow SRRIP policy */
    cache_access_block_srripm(&(policy_data->srripm_policy_data), way, strm, 
      info);
  }
  else
  { 
    if (policy_data->following == cache_policy_srript)
    {
      /* Update global counters */
      if (info && info->fill == TRUE)
      {
        cache_update_hit_counter_srript(&(policy_data->srript_policy_data), 
            global_data, way, strm); 
      }

      /* Follow SRRIP policy */
      cache_access_block_srript(&(policy_data->srript_policy_data), way, strm, 
          info);
    }
    else
    {
      blk  = &(GSPCT_DATA_BLOCKS(policy_data)[way]);
      prev = blk->prev;
      next = blk->next;

      /* Check: block's tag and state are valid */
      assert(blk->tag >= 0);
      assert(blk->state != cache_block_invalid);

      /* Follow GSPC policy */
      assert(policy_data->following == cache_policy_gspct);

      blk->dirty   = (info && info->spill) ? 1 : 0;
      blk->access += 1;

      if (info && (strm == BS && info->spill == TRUE))
      {
        blk->is_bt_block = TRUE;
        SAT_CTR_INC(global_data->bt_fill);
      }

      if (info && (info->fill == TRUE))
      {
        /* Get old RRPV from the block */
        old_rrpv = (((gspct_list *)(blk->data))->rrpv);

        /* Get new RRPV using policy specific function */
        if (blk->is_bt_block == TRUE)
        {
          /* For blitter find RRPV based on reuse */
          ub8 bt_access;
          ub8 bt_blocks;

          bt_access = global_data->bt_access * reuse_ratio;
          bt_blocks = global_data->bt_blocks;

#if 0
          if (bt_blocks && bt_access)
          {
            printf(" reuse %d , no reuse %d \n", SAT_CTR_VAL(global_data->bt_reuse), SAT_CTR_VAL(global_data->bt_no_reuse));
            printf(" BS Access %d , Blocks %d reuse ratio %d \n", global_data->bt_access, bt_blocks, reuse_ratio);
            printf(" BS Access / Blocks %d \n", (int)ceil((float)bt_access / (float)bt_blocks));
          }
#endif

          /* If blocks use has exceeded average reuse, set, its RRPV to 0 */
          if (bt_blocks && blk->access > (int)ceil((float)bt_access / bt_blocks))
          {
            new_rrpv = GSPCT_DATA_MAX_RRPV(policy_data) - 1;
#if 0
            printf("BT block dead\n");
#endif
          }
          else
          {
            new_rrpv = 0;

            if (SAT_CTR_VAL(global_data->bt_no_reuse) && SAT_CTR_VAL(global_data->bt_reuse))
            {
              printf("BT reuse ratio %d \n", 
                  (int)ceil((float)(SAT_CTR_VAL(global_data->bt_no_reuse) + SAT_CTR_VAL(global_data->bt_reuse)) /
                    SAT_CTR_VAL(global_data->bt_no_reuse)));
            }
          }
        }
        else
        {
          new_rrpv = cache_get_new_rrpv_gspct(policy_data, global_data, way, strm);
        }

        /* Update block queue if block got new RRPV */
        if (new_rrpv != old_rrpv)
        {
          CACHE_REMOVE_FROM_QUEUE(blk, GSPCT_DATA_VALID_HEAD(policy_data)[old_rrpv],
              GSPCT_DATA_VALID_TAIL(policy_data)[old_rrpv]);
          CACHE_APPEND_TO_QUEUE(blk, GSPCT_DATA_VALID_HEAD(policy_data)[new_rrpv], 
              GSPCT_DATA_VALID_TAIL(policy_data)[new_rrpv]);
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
      else
      {
        if (blk->is_bt_block)
        {
          /* Get old RRPV from the block */
          old_rrpv = (((gspct_list *)(blk->data))->rrpv);

          if (SAT_CTR_VAL(global_data->bt_fill) > reuse_ratio)
          {
            new_rrpv = GSPCT_DATA_MAX_RRPV(policy_data) - 1; 
            SAT_CTR_SET(global_data->bt_fill, 0);
          }
          else
          { 
            new_rrpv = 0;
          }

          /* Update block queue if block got new RRPV */
          if (new_rrpv != old_rrpv)
          {
            CACHE_REMOVE_FROM_QUEUE(blk, GSPCT_DATA_VALID_HEAD(policy_data)[old_rrpv],
                GSPCT_DATA_VALID_TAIL(policy_data)[old_rrpv]);
            CACHE_APPEND_TO_QUEUE(blk, GSPCT_DATA_VALID_HEAD(policy_data)[new_rrpv], 
                GSPCT_DATA_VALID_TAIL(policy_data)[new_rrpv]);
          }
        }
      }

      CACHE_UPDATE_BLOCK_STREAM(blk, strm);
    }
  }
}

/* Get replacement RRPV */
int cache_get_replacement_rrpv_gspct(gspct_data *policy_data)
{
  return GSPCT_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_gspct(gspct_data *policy_data, gspct_gdata *global_data, int way, int strm)
{
  /* Current GSPC specific block state */
  unsigned int state;
   
  state = GSPCT_DATA_BLOCKS(policy_data)[way].block_state;

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
          return GSPCT_DATA_MAX_RRPV(policy_data); 
        }
        else
        {
          /* Insert block with 0 RRPV */
          return !(GSPCT_DATA_MAX_RRPV(policy_data));
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
            return GSPCT_DATA_MAX_RRPV(policy_data); 
          }
          else
          {
            /* Insert block with 0 RRPV */
            return !(GSPCT_DATA_MAX_RRPV(policy_data));
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

  return GSPCT_DATA_MAX_RRPV(policy_data) - 1;
}

/* Update fill counter */
void cache_update_fill_counter_srript(srript_data *policy_data, gspct_gdata *global_data, int way, int strm)
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
    SRRIPT_DATA_BLOCKS(policy_data)[way].block_state = 0;
  }
  else
  {
    if (strm == ZS)
    {
      SAT_CTR_INC(global_data->z_e0_fill_ctr);
      SRRIPT_DATA_BLOCKS(policy_data)[way].block_state = 0;
    }
    else
    {
      if (strm == CS)
      {
        SRRIPT_DATA_BLOCKS(policy_data)[way].block_state = 3;
        SAT_CTR_INC(global_data->rt_prod_ctr);
      }
      else
      {
        if (strm == BS)
        {
          SRRIPT_DATA_BLOCKS(policy_data)[way].block_state = 3;
          SAT_CTR_INC(global_data->bt_prod_ctr);
        }
        else
        {
          SRRIPT_DATA_BLOCKS(policy_data)[way].block_state = 0;
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

    if (SAT_CTR_VAL(global_data->bt_reuse) > 128)
    {
      SAT_CTR_HLF(global_data->bt_reuse);
    }

    if (SAT_CTR_VAL(global_data->bt_no_reuse) > 128)
    {
      SAT_CTR_HLF(global_data->bt_no_reuse);
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
void cache_update_hit_counter_srript(srript_data *policy_data, gspct_gdata *global_data, int way, int strm)
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
  
  block = &(SRRIPT_DATA_BLOCKS(policy_data)[way]);
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
int cache_get_fill_rrpv_gspct(gspct_data *policy_data, gspct_gdata *global_data, int strm)
{
  /*
   * CS block is filled with RRPV 0
   * TS block if reuse probability of epoc 0 < threshold 3 or 0
   * ZS block if reuse probability of epoc 0 < threshold 3 else 2
   */

  assert(strm <= TST);

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
        return GSPCT_DATA_MAX_RRPV(policy_data);
      }
      else
      {
        /* If prodution, cinsmption ratio is below half of the threshold */
        if ((ub4)(PROD_CTR(global_data) / CONS_CTR(global_data)) > (policy_data->threshold / 2))
        {
          /* Insert block with distant RRPV */
          return GSPCT_DATA_MAX_RRPV(policy_data) - 1;
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

      if (SAT_CTR_VAL(global_data->bt_no_reuse) && SAT_CTR_VAL(global_data->bt_reuse))
      {
        printf("Prod %ld Cons %ld\n", SAT_CTR_VAL(global_data->bt_prod_ctr), 
            SAT_CTR_VAL(global_data->bt_cons_ctr));

        printf("BT reuse ratio %d \n", 
            (int)ceil((float)(SAT_CTR_VAL(global_data->bt_no_reuse) + SAT_CTR_VAL(global_data->bt_reuse)) /
              SAT_CTR_VAL(global_data->bt_no_reuse)));
      }

      ub8 reuse_ratio;

      if (SAT_CTR_VAL(global_data->bt_no_reuse))
      {
        reuse_ratio = (int)ceil((float)(SAT_CTR_VAL(global_data->bt_no_reuse) + 
              SAT_CTR_VAL(global_data->bt_reuse)) / SAT_CTR_VAL(global_data->bt_no_reuse));
      }
      else
      {
        reuse_ratio = 1;
      }

      if (SAT_CTR_VAL(global_data->bt_fill) > reuse_ratio)
      {
        /* Reset fill counter */
        SAT_CTR_SET(global_data->bt_fill, 0);

        /* Fill block with max RRPV - 1 */
        return GSPCT_DATA_MAX_RRPV(policy_data) - 1; 
      }
      else
      { 
        /* Fill block with min RRPV */
        return 0;
      }

      /* If production, consumption ratio is below threshold */
      if (!CONS_CTR(global_data) || 
          (CONS_CTR(global_data) && ((ub4)(PROD_CTR(global_data) / CONS_CTR(global_data))) > policy_data->threshold))
      {
        /* Insert block with max RRPV */
        return GSPCT_DATA_MAX_RRPV(policy_data);
      }
      else
      {
        /* If prodution, cinsmption ratio is below half of the threshold */
        if ((ub4)(PROD_CTR(global_data) / CONS_CTR(global_data)) > (policy_data->threshold / 2))
        {
          /* Insert block with distant RRPV */
          return GSPCT_DATA_MAX_RRPV(policy_data) - 1;
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
        return GSPCT_DATA_MAX_RRPV(policy_data); 
      }
      else
      {
        /* Insert block with 0 RRPV */
        return !(GSPCT_DATA_MAX_RRPV(policy_data));
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
        return GSPCT_DATA_MAX_RRPV(policy_data); 
      }
      else
      {
        /* Insert block with distant RRPV */
        return GSPCT_DATA_MAX_RRPV(policy_data) - 1;
      }

#undef HI_CT
#undef FI_CT

      break;
  }

  return GSPCT_DATA_MAX_RRPV(policy_data) - 1;
}


/* Update state of the block. */
void cache_set_block_gspct(gspct_data *policy_data, gspct_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream,
  memory_trace *info)
{
  struct cache_block_t *block;
  
  if (policy_data->following == cache_policy_srripm)
  {
    cache_set_block_srripm(&(policy_data->srripm_policy_data), way, tag, state,
      stream, info);
  }
  else
  {
    if (policy_data->following == cache_policy_srript)
    {
      cache_set_block_srript(&(policy_data->srript_policy_data), way, tag, state,
          stream, info);
    }
    else
    {
      block = &(GSPCT_DATA_BLOCKS(policy_data))[way];

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
      int old_rrpv = (((gspct_list *)(block->data))->rrpv);

      /* Remove block from valid list and insert into free list */
      CACHE_REMOVE_FROM_QUEUE(block, GSPCT_DATA_VALID_HEAD(policy_data)[old_rrpv],
          GSPCT_DATA_VALID_TAIL(policy_data)[old_rrpv]);
      CACHE_APPEND_TO_SQUEUE(block, GSPCT_DATA_FREE_HEAD(policy_data), 
          GSPCT_DATA_FREE_TAIL(policy_data));
    }
  }
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_gspct(gspct_data *policy_data, 
  gspct_gdata *global_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  if (policy_data->following == cache_policy_srripm)
  {
    return cache_get_block_srripm(&(policy_data->srripm_policy_data), way, tag_ptr,
      state_ptr, stream_ptr);
  } 
  else
  {
    if (policy_data->following == cache_policy_srript)
    {
      return cache_get_block_srript(&(policy_data->srript_policy_data), way, tag_ptr,
          state_ptr, stream_ptr);
    } 
    else
    {
      PTR_ASSIGN(tag_ptr, (GSPCT_DATA_BLOCKS(policy_data)[way]).tag);
      PTR_ASSIGN(state_ptr, (GSPCT_DATA_BLOCKS(policy_data)[way]).state);
      PTR_ASSIGN(stream_ptr, (GSPCT_DATA_BLOCKS(policy_data)[way]).stream);
    }
  }

  return GSPCT_DATA_BLOCKS(policy_data)[way];
}
