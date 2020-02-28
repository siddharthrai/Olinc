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
#include "xspdbp.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))
#define SAMPLED_SET             (0)
#define FOLLOWER_SET            (1)

#define CACHE_UPDATE_BLOCK_STATE(block, tag, state)             \
do                                                              \
{                                                               \
  (block)->tag   = (tag);                                       \
  (block)->state = (state);                                     \
}while(0)

#define CACHE_XSPDBP_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)   \
do                                                              \
{                                                               \
    int dif = 0;                                                \
                                                                \
    for (int i = rrpv - 1; i >= 0; i--)                         \
    {                                                           \
      assert(i <= rrpv);                                        \
                                                                \
      if ((head_ptr)[i].head)                                   \
      {                                                         \
        if (!dif)                                               \
        {                                                       \
          dif = rrpv - i;                                       \
        }                                                       \
                                                                \
        assert((tail_ptr)[i].head);                             \
                                                                \
        (head_ptr)[i + dif].head  = (head_ptr)[i].head;         \
        (tail_ptr)[i + dif].head  = (tail_ptr)[i].head;         \
        (head_ptr)[i].head        = NULL;                       \
        (tail_ptr)[i].head        = NULL;                       \
                                                                \
        struct cache_block_t *node = (head_ptr)[i + dif].head;  \
                                                                \
        /* Point data to new RRPV head */                       \
        for (; node; node = node->prev)                         \
        {                                                       \
          node->data = &(head_ptr[i + dif]);                    \
        }                                                       \
      }                                                         \
      else                                                      \
      {                                                         \
        assert(!((tail_ptr)[i].head));                          \
      }                                                         \
    }                                                           \
}while(0)

#define CACHE_GET_BLOCK_STREAM(block, strm)                     \
do                                                              \
{                                                               \
  strm = (block)->stream;                                       \
}while(0)

#define CACHE_UPDATE_BLOCK_STREAM(block, strm)                  \
do                                                              \
{                                                               \
  (block)->stream = strm;                                       \
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

static int get_set_type_xspdbp(long long int indx)
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

static void print_counters(xspdbp_gdata *global_data)
{
#if 0
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
#endif

  /* Total accesses */
  printf("Texture epoch-wise hits:%ld %ld %ld\n", global_data->texture_0_hit, 
      global_data->texture_1_hit, global_data->texture_more_hit); 
  printf("Texture epoch-wise prediction:%ld %ld\n", global_data->texture_1_pred, 
      global_data->texture_2_pred); 
}

void cache_init_xspdbp(long long int set_indx, struct cache_params *params, xspdbp_data *policy_data, xspdbp_gdata *global_data)
{
  /* For XSPDBP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define THRESHOLD       (params->threshold)
#define MEM_ALLOC(size) ((xspdbp_list *)xcalloc(size, sizeof(xspdbp_list)))

  /* Create per rrpv buckets */
  XSPDBP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  XSPDBP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

  /* Set max RRPV for the set */
  XSPDBP_DATA_MAX_RRPV(policy_data)  = MAX_RRPV;
  XSPDBP_DATA_THRESHOLD(policy_data) = THRESHOLD;

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    XSPDBP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    XSPDBP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    XSPDBP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  XSPDBP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

  /* Initialize array of blocks */
  XSPDBP_DATA_FREE_HEAD(policy_data) = &(XSPDBP_DATA_BLOCKS(policy_data)[0]);
  XSPDBP_DATA_FREE_TAIL(policy_data) = &(XSPDBP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&XSPDBP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&XSPDBP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&XSPDBP_DATA_BLOCKS(policy_data)[way])->next  = way ? 
      (&XSPDBP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&XSPDBP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&XSPDBP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV
#undef THRESHOLD
#undef CACHE_ALLOC
  
  if (get_set_type_xspdbp(set_indx) == SAMPLED_SET)
  {
    /* Initialize srrip policy for the set */
    cache_init_srrip(set_indx, params, &(policy_data->srrip_policy_data), 
        &(global_data->srrip));
    policy_data->following = cache_policy_srrip;
  }
  else
  {
    policy_data->following = cache_policy_xspdbp;
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

  SAT_CTR_INI(global_data->acc_all_ctr, 7, 0, 127);      /* Total accesses */

  global_data->texture_0_hit = 0;                        /* Texture hit at epoch 0 */
  global_data->texture_1_hit = 0;                        /* Texture hit at epoch 1 */ 
  
  if (set_indx == 0)
  {
    global_data->bm_thr = 32; 
    global_data->bm_ctr = 0; 
  }
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_xspdbp(xspdbp_data *policy_data, xspdbp_gdata *global_data)
{
  /* Free all data blocks */
  free(XSPDBP_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (XSPDBP_DATA_VALID_HEAD(policy_data))
  {
    free(XSPDBP_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (XSPDBP_DATA_VALID_TAIL(policy_data))
  {
    free(XSPDBP_DATA_VALID_TAIL(policy_data));
  }
  
  /* Free SRRIP specific information */
  if (policy_data->following == cache_policy_srrip)
  {
    cache_free_srrip(&(policy_data->srrip_policy_data));
  }
}

/* Block lookup */
struct cache_block_t * cache_find_block_xspdbp(xspdbp_data *policy_data, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;
  
  node = NULL;

  if (policy_data->following == cache_policy_srrip)
  {
    return cache_find_block_srrip(&(policy_data->srrip_policy_data), tag);
  }
  else
  {
    max_rrpv = policy_data->max_rrpv;

    for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
    {
      head = XSPDBP_DATA_VALID_HEAD(policy_data)[rrpv].head;

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
void cache_fill_block_xspdbp(xspdbp_data *policy_data, xspdbp_gdata *global_data, 
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
  block = &(XSPDBP_DATA_BLOCKS(policy_data)[way]);
  assert(block);
  
  if (policy_data->following == cache_policy_srrip)
  {
    /* Follow RRIP policy */
    cache_fill_block_srrip(&(policy_data->srrip_policy_data), 
        &(global_data->srrip), way, tag, state, strm, info);

    /* Update global counters */
    cache_update_fill_counter_xspdbp(&(policy_data->srrip_policy_data), 
      global_data, way, strm);
  }
  else
  {
    /* Follow XSPDBP */
    assert(policy_data->following == cache_policy_xspdbp);

    /* Remove block from free list */
    CACHE_REMOVE_FROM_SQUEUE(block, XSPDBP_DATA_FREE_HEAD(policy_data),
      XSPDBP_DATA_FREE_TAIL(policy_data));

    /* Update new block state and stream */
    CACHE_UPDATE_BLOCK_STATE(block, tag, state);
    CACHE_UPDATE_BLOCK_STREAM(block, strm);
    block->dirty = (info && info->spill) ? 1 : 0;
    block->epoch = 0;

    /* Get RRPV to be assigned to the new block */
    rrpv = cache_get_fill_rrpv_xspdbp(policy_data, global_data, strm, info);

    /* Insert block in to the corresponding RRPV queue */
    CACHE_APPEND_TO_QUEUE(block, 
        XSPDBP_DATA_VALID_HEAD(policy_data)[rrpv], 
        XSPDBP_DATA_VALID_TAIL(policy_data)[rrpv]);
  }
}

/* Block replacement */
int cache_replace_block_xspdbp(xspdbp_data *policy_data, 
    xspdbp_gdata *global_data, memory_trace *info)
{
  struct cache_block_t *block;

  int rrpv;
  
  if (policy_data->following == cache_policy_srrip)
  {
    return cache_replace_block_srrip(&(policy_data->srrip_policy_data), 
        &(global_data->srrip), info);
  }
  else
  {
    /* Try to find an invalid block. */
    for (block = policy_data->free_head; block; block = block->next)
      return block->way;

    /* Obtain RRPV from where to replace the block */
    rrpv = cache_get_replacement_rrpv_xspdbp(policy_data);

    /* Ensure rrpv is woth in max_rrpv bound */
    assert(rrpv >= 0 && rrpv <= XSPDBP_DATA_MAX_RRPV(policy_data));

    /* If there is no block with required RRPV, increment RRPV of all the blocks
     * until we get one with the required RRPV */
    if (XSPDBP_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
    {
      CACHE_XSPDBP_INCREMENT_RRPV(XSPDBP_DATA_VALID_HEAD(policy_data),
        XSPDBP_DATA_VALID_TAIL(policy_data), rrpv);
    }

    /* Remove a nonbusy block from the tail */
    unsigned int min_wayid = ~(0);

    /* Remove a nonbusy block from the tail */
    for (block = XSPDBP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
    {
      if (!block->busy && block->way < min_wayid)
        min_wayid = block->way;
    }

    /* If no non busy block can be found, return -1 */
    return (min_wayid != ~(0)) ? min_wayid : -1;
  }
}

/* Block aging */
void cache_access_block_xspdbp(xspdbp_data *policy_data, 
  xspdbp_gdata *global_data, int way, int strm, memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int old_rrpv;
  int new_rrpv;
  
  assert(policy_data);
  assert(global_data);
  assert(way >= 0);
  assert(strm <= TST);

  if (policy_data->following == cache_policy_srrip)
  {
    /* Follow SRRIP policy */
    cache_access_block_srrip(&(policy_data->srrip_policy_data), 
        &(global_data->srrip), way, strm, info);

    /* Update global counters */
    if (info && info->fill == TRUE)
    {
      cache_update_hit_counter_xspdbp(&(policy_data->srrip_policy_data), 
        global_data, way, strm); 
    }
  }
  else
  { 
    blk  = &(XSPDBP_DATA_BLOCKS(policy_data)[way]);
    prev = blk->prev;
    next = blk->next;

    /* Check: block's tag and state are valid */
    assert(blk->tag >= 0);
    assert(blk->state != cache_block_invalid);

    /* Follow XSPDBP policy */
    assert(policy_data->following == cache_policy_xspdbp);
    
    if (info)
    {
      /* Get old RRPV from the block */
      old_rrpv = (((xspdbp_list *)(blk->data))->rrpv);

      /* Get new RRPV using policy specific function */
      new_rrpv = cache_get_new_rrpv_xspdbp(policy_data, global_data, way, strm);

      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        CACHE_REMOVE_FROM_QUEUE(blk, XSPDBP_DATA_VALID_HEAD(policy_data)[old_rrpv],
            XSPDBP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, XSPDBP_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            XSPDBP_DATA_VALID_TAIL(policy_data)[new_rrpv]);
      }

      if (info->stream == TS)
      {
        switch (blk->epoch)  
        {
          case 0:
            global_data->texture_0_hit += 1;
            break;

          case 1:
            global_data->texture_1_hit += 1;
            break;

          case 2:
            global_data->texture_more_hit += 1;
            break;
        }
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

    blk->dirty = (info && info->spill) ? 1 : 0;
    CACHE_UPDATE_BLOCK_STREAM(blk, strm);
  }
}

/* Get replacement RRPV */
int cache_get_replacement_rrpv_xspdbp(xspdbp_data *policy_data)
{
  return XSPDBP_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_xspdbp(xspdbp_data *policy_data, xspdbp_gdata *global_data, int way, int strm)
{
  int tmp_rrpv;

  /* Current XSPDBP specific block state */
  unsigned int state;
  state = XSPDBP_DATA_BLOCKS(policy_data)[way].block_state;

  switch (strm)    
  {
    /* Color stream */
    case CS:
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
          return XSPDBP_DATA_MAX_RRPV(policy_data); 
        }
        else
        {
          global_data->texture_2_pred += 1;

          /* Insert block with 0 RRPV */
          return !(XSPDBP_DATA_MAX_RRPV(policy_data));
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
            return XSPDBP_DATA_MAX_RRPV(policy_data); 
          }
          else
          {
            /* Insert block with 0 RRPV */
            return !(XSPDBP_DATA_MAX_RRPV(policy_data));
          }
        }
      }

#undef HI_CT0
#undef FI_CT0
#undef HI_CT1
#undef FI_CT1
#undef TX_BLK
#undef RT_BLK

    case ZS:
      break;
  }

  return 0;
}

/* Update fill counter */
void cache_update_fill_counter_xspdbp(srrip_data *policy_data, xspdbp_gdata *global_data, int way, int strm)
{
  /* XSPDBP counter updates 
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
    SRRIP_DATA_BLOCKS(policy_data)[way].block_state = 0;
  }
  else
  {
    if (strm == ZS)
    {
      SAT_CTR_INC(global_data->z_e0_fill_ctr);
      SRRIP_DATA_BLOCKS(policy_data)[way].block_state = 0;
    }
    else
    {
      if (strm == CS)
      {
        SRRIP_DATA_BLOCKS(policy_data)[way].block_state = 3;
        SAT_CTR_INC(global_data->rt_prod_ctr);
      }
      else
      {
        if (strm == BS)
        {
          SRRIP_DATA_BLOCKS(policy_data)[way].block_state = 3;
          SAT_CTR_INC(global_data->bt_prod_ctr);
        }
        else
        {
          SRRIP_DATA_BLOCKS(policy_data)[way].block_state = 0;
        }
      }
    }
  }

  SAT_CTR_INC(global_data->acc_all_ctr);

  /* If access counter is saturated, half all all other counters */
  if (IS_CTR_SAT(global_data->acc_all_ctr))
  {
    print_counters(global_data);

    SAT_CTR_HLF(global_data->tex_e0_fill_ctr);
    SAT_CTR_HLF(global_data->tex_e0_hit_ctr);
    SAT_CTR_HLF(global_data->tex_e1_fill_ctr);
    SAT_CTR_HLF(global_data->tex_e1_hit_ctr);
    SAT_CTR_HLF(global_data->z_e0_fill_ctr);
    SAT_CTR_HLF(global_data->z_e0_hit_ctr);
    SAT_CTR_HLF(global_data->rt_cons_ctr);
    SAT_CTR_HLF(global_data->rt_prod_ctr);
    SAT_CTR_HLF(global_data->bt_cons_ctr);
    SAT_CTR_HLF(global_data->bt_prod_ctr);

    SAT_CTR_SET(global_data->acc_all_ctr, 0);
  }
}

/* Update hit counter */
void cache_update_hit_counter_xspdbp(srrip_data *policy_data, xspdbp_gdata *global_data, int way, int strm)
{
  /* XSPDBP counter updates 
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
  
  block = &(SRRIP_DATA_BLOCKS(policy_data)[way]);
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
            SAT_CTR_INC(global_data->rt_cons_ctr);
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
int cache_get_fill_rrpv_xspdbp(xspdbp_data *policy_data, 
    xspdbp_gdata *global_data, int strm, memory_trace *info)
{
  /*
   * CS block is filled with RRPV 0
   * TS block if reuse probability of epoc 0 < threshold 3 or 0
   * ZS block if reuse probability of epoc 0 < threshold 3 else 2
   */
  int fill_rrpv;
  int tmp_rrpv;
  int x_rrpv;
 
  assert(strm <= TST);

  if ((global_data->bm_ctr)++ == 0)
  {
    x_rrpv = 0;
  }
  else
  {
    x_rrpv = XSPDBP_DATA_MAX_RRPV(policy_data) - 1;
  }

  if (global_data->bm_ctr == global_data->bm_thr)
  {
    global_data->bm_ctr = 0;
  }
  
  if (info && info->fill == TRUE && info->stream == BS)
  {
    fill_rrpv = XSPDBP_DATA_MAX_RRPV(policy_data);
  }
  else
  {
    fill_rrpv =  XSPDBP_DATA_MAX_RRPV(policy_data) - 1;
  }

  switch (strm)    
  {
    case CS:
      add_region_block(CS, info->stream, info->vtl_addr);

      /* If region table returns RRPV smaller than MAX, assign MIN RRPV */
      tmp_rrpv = get_xstream_reuse_ratio(CS, TS, info->vtl_addr);

      if (tmp_rrpv < XSPDBP_DATA_MAX_RRPV(policy_data))
      {
        fill_rrpv = 0;
      }
      break;

    case BS:
      add_region_block(BS, info->stream, info->vtl_addr);

      /* If region table returns RRPV smaller than MAX, assign MIN RRPV */
      tmp_rrpv = get_xstream_reuse_ratio(BS, TS, info->vtl_addr);

      if (tmp_rrpv < XSPDBP_DATA_MAX_RRPV(policy_data))
      {
        fill_rrpv = x_rrpv;
      }
      break;

    case TS:
      add_region_block(CS, info->stream, info->vtl_addr);
      add_region_block(BS, info->stream, info->vtl_addr);
      add_region_block(ZS, info->stream, info->vtl_addr);

#define HI_CT0(data) ((float)SAT_CTR_VAL(data->tex_e0_hit_ctr))
#define FI_CT0(data) ((float)SAT_CTR_VAL(data->tex_e0_fill_ctr))

      /* If reuse probability of epoch 0 is below the threshold */
      if ((FI_CT0(global_data) / HI_CT0(global_data)) > policy_data->threshold)
      {
        /* Insert block with max RRPV */
        return XSPDBP_DATA_MAX_RRPV(policy_data); 
      }
      else
      {
        global_data->texture_1_pred += 1;

        /* Insert block with 0 RRPV */
        return !(XSPDBP_DATA_MAX_RRPV(policy_data));
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
        fill_rrpv =  XSPDBP_DATA_MAX_RRPV(policy_data); 
      }
      else
      {
        /* Insert block with distant RRPV */
        fill_rrpv = XSPDBP_DATA_MAX_RRPV(policy_data) - 1;
      }

#undef HI_CT
#undef FI_CT

      /* If region table returns RRPV smaller than MAX, assign MIN RRPV */
      tmp_rrpv = get_xstream_reuse_ratio(ZS, TS, info->vtl_addr);

      if (tmp_rrpv < XSPDBP_DATA_MAX_RRPV(policy_data))
      {
        fill_rrpv = x_rrpv;
      }
      break;
  }
  
  return fill_rrpv;
}

/* Update state of the block. */
void cache_set_block_xspdbp(xspdbp_data *policy_data, xspdbp_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream,
  memory_trace *info)
{
  struct cache_block_t *block;
  
  if (policy_data->following == cache_policy_srrip)
  {
    cache_set_block_srrip(&(policy_data->srrip_policy_data), way, tag, state,
      stream, info);
  }
  else
  {
    block = &(XSPDBP_DATA_BLOCKS(policy_data))[way];

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
    int old_rrpv = (((xspdbp_list *)(block->data))->rrpv);

    /* Remove block from valid list and insert into free list */
    CACHE_REMOVE_FROM_QUEUE(block, XSPDBP_DATA_VALID_HEAD(policy_data)[old_rrpv],
        XSPDBP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
    CACHE_APPEND_TO_SQUEUE(block, XSPDBP_DATA_FREE_HEAD(policy_data), 
        XSPDBP_DATA_FREE_TAIL(policy_data));
  }
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_xspdbp(xspdbp_data *policy_data, 
  xspdbp_gdata *global_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  if (policy_data->following == cache_policy_srrip)
  {
    return cache_get_block_srrip(&(policy_data->srrip_policy_data), way, tag_ptr,
      state_ptr, stream_ptr);
  } 
  else
  {
    PTR_ASSIGN(tag_ptr, (XSPDBP_DATA_BLOCKS(policy_data)[way]).tag);
    PTR_ASSIGN(state_ptr, (XSPDBP_DATA_BLOCKS(policy_data)[way]).state);
    PTR_ASSIGN(stream_ptr, (XSPDBP_DATA_BLOCKS(policy_data)[way]).stream);
  }

  return XSPDBP_DATA_BLOCKS(policy_data)[way];
}
