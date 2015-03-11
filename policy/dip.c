/*
 * Copyright (C) 2014  Siddharth Rai
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
#include "dip.h"

#define LRU_SAMPLED_SET         (0)
#define BIP_SAMPLED_SET         (1)
#define FOLLOWER_SET            (2)
#define PSEL_WIDTH              (10)
#define PSEL_MIN_VAL            (0x00)
#define PSEL_MAX_VAL            (0x3ff)
#define PSEL_MID_VAL            (512)


#define CACHE_SET(cache, set)  (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)  (&((set)->blocks[way]))

#define CACHE_UPDATE_BLOCK_STATE(block, tag, state)           \
do                                                            \
{                                                             \
  (block)->tag   = (tag);                                     \
  (block)->state = (state);                                   \
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
        /* free list must be non empty */                                               \
        assert(DIP_DATA_FREE_HEAD(set) && DIP_DATA_FREE_TAIL(set));                     \
                                                                                        \
        /* Check: current block must be in invalid state */                             \
        assert((blk)->state == cache_block_invalid);                                    \
                                                                                        \
        CACHE_REMOVE_FROM_SQUEUE(blk, DIP_DATA_FREE_HEAD(set), DIP_DATA_FREE_TAIL(set));\
                                                                                        \
        (blk)->next = NULL;                                                             \
        (blk)->prev = NULL;                                                             \
                                                                                        \
        /* Reset block state */                                                         \
        (blk)->busy = 0;                                                                \
        (blk)->cached = 0;                                                              \
        (blk)->prefetch = 0;                                                            \
}while(0);

/* Returns currently followed policy */
#define GET_CURRENT_POLICY(d, gd) (((d)->following == cache_policy_lru ||     \
                                    ((d)->following == cache_policy_dip &&    \
                                    SAT_CTR_VAL((gd)->psel) < PSEL_MID_VAL)) ?\
                                    cache_policy_lru : cache_policy_bip)

/* Obtain set type based on index */
static int get_set_type_dip(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x0f;
  msb_bits = (indx >> 6) & 0x0f;
  mid_bits = (indx >> 4) & 0x03;

  if (lsb_bits == msb_bits && mid_bits == 0)
  {
    return LRU_SAMPLED_SET;
  }
  else
  {
    if (lsb_bits == (~msb_bits & 0x0f) && mid_bits == 0)
    {
      return BIP_SAMPLED_SET;
    }
  }

  return FOLLOWER_SET;
}

static void cache_init_lru_from_dip(struct cache_params *params, lru_data *policy_data,
  dip_data *dip_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(dip_policy_data);

  /* Initialize valid head and tail pointers */
  LRU_DATA_VALID_HLST(policy_data) = DIP_DATA_VALID_HLST(dip_policy_data);
  LRU_DATA_VALID_TLST(policy_data) = DIP_DATA_VALID_TLST(dip_policy_data);

  LRU_DATA_BLOCKS(policy_data) = DIP_DATA_BLOCKS(dip_policy_data);
  assert(LRU_DATA_BLOCKS(policy_data));

  /* Initialize array of blocks */
  LRU_DATA_FREE_HLST(policy_data) = DIP_DATA_FREE_HLST(dip_policy_data);
  LRU_DATA_FREE_TLST(policy_data) = DIP_DATA_FREE_TLST(dip_policy_data);
}

static void cache_init_bip_from_dip(struct cache_params *params, bip_data *policy_data,
  dip_data *dip_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(dip_policy_data);

  /* Initialize valid head and tail pointers */
  BIP_DATA_VALID_HLST(policy_data) = DIP_DATA_VALID_HLST(dip_policy_data);
  BIP_DATA_VALID_TLST(policy_data) = DIP_DATA_VALID_TLST(dip_policy_data);

  BIP_DATA_BLOCKS(policy_data) = DIP_DATA_BLOCKS(dip_policy_data);
  assert(BIP_DATA_BLOCKS(policy_data));

  /* Initialize array of blocks */
  BIP_DATA_FREE_HLST(policy_data) = DIP_DATA_FREE_HLST(dip_policy_data);
  BIP_DATA_FREE_TLST(policy_data) = DIP_DATA_FREE_TLST(dip_policy_data);

  SAT_CTR_INI(policy_data->access_ctr, 8, 0, 255);
}


/* Initialize policy specific data */
void cache_init_dip(long long int set_indx, struct cache_params *params, 
  dip_data *policy_data, dip_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

  DIP_DATA_VALID_HLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(DIP_DATA_VALID_HLST(policy_data));

  DIP_DATA_VALID_TLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(DIP_DATA_VALID_TLST(policy_data));

  DIP_DATA_FREE_HLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(DIP_DATA_FREE_HLST(policy_data));

  DIP_DATA_FREE_TLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(DIP_DATA_FREE_TLST(policy_data));

  /* Initialize valid head and tail pointers */
  DIP_DATA_VALID_HEAD(policy_data) = NULL;
  DIP_DATA_VALID_TAIL(policy_data) = NULL;

#define MEM_ALLOC(size) (xcalloc(size, sizeof(struct cache_block_t)))
  
  DIP_DATA_BLOCKS(policy_data) = (struct cache_block_t *)MEM_ALLOC(params->ways);
  assert(DIP_DATA_BLOCKS(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  DIP_DATA_FREE_HEAD(policy_data) = &(DIP_DATA_BLOCKS(policy_data)[0]);
  DIP_DATA_FREE_TAIL(policy_data) = &(DIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&DIP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&DIP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&DIP_DATA_BLOCKS(policy_data)[way])->next  = way ? 
      (&DIP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&DIP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&DIP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

  assert(global_data);

  switch  (get_set_type_dip(set_indx))
  {
    case LRU_SAMPLED_SET:

      /* Initialize lru policy */
      cache_init_lru_from_dip(params, &(policy_data->lru), policy_data);
      cache_init_bip_from_dip(params, &(policy_data->bip), policy_data);

      policy_data->following = cache_policy_lru;
      break;

    case BIP_SAMPLED_SET:

      /* Initialize bip policy */
      cache_init_lru_from_dip(params, &(policy_data->lru), policy_data);
      cache_init_bip_from_dip(params, &(policy_data->bip), policy_data);

      policy_data->following = cache_policy_bip;
      break;

    case FOLLOWER_SET:

      /* Initialize both the component policies */
      cache_init_lru_from_dip(params, &(policy_data->lru), policy_data);
      cache_init_bip_from_dip(params, &(policy_data->bip), policy_data);

      policy_data->following = cache_policy_dip;
      break;

    default:

      panic("Unknown set type %s %d\n", __FUNCTION__, __LINE__);
  }

  /* Initialize global policy data */
  SAT_CTR_INI(global_data->psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
  SAT_CTR_SET(global_data->psel, PSEL_MID_VAL);
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_dip(dip_data *policy_data)
{
  /* Ensure valid arguments */
  assert(policy_data);

  /* Free all data blocks */
  free(DIP_DATA_BLOCKS(policy_data));
}

/* Find block with the given tag */
struct cache_block_t * cache_find_block_dip(dip_data *policy_data, long long tag)
{
  struct cache_block_t *head;
  struct cache_block_t *node;

  head = DIP_DATA_VALID_HEAD(policy_data);

  for (node = head; node; node = node->prev)
  {
    assert(node->state != cache_block_invalid);

    if (node->tag == tag)
      break;
  }

  return node;
}

/* Fill new block in the cache */
void cache_fill_block_dip(dip_data *policy_data, dip_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int stream,
  memory_trace *info)
{
  /* Ensure valid arguments */ 
  assert(policy_data);

  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_lru || 
      policy_data->following == cache_policy_bip ||
      policy_data->following == cache_policy_dip);

  /* Fill block in all component policies */
  switch (GET_CURRENT_POLICY(policy_data, global_data))
  {
    case cache_policy_lru:
      cache_fill_block_lru(&(policy_data->lru), way, tag, state, stream, info);
      break;

    case cache_policy_bip:
      cache_fill_block_bip(&(policy_data->bip), way, tag, state, stream, info);
      break;

    default:
      panic("Invalid follower function %s line %d\n", __FUNCTION__, __LINE__);
  }

  switch (policy_data->following)
  {
    case cache_policy_lru:

      SAT_CTR_INC(global_data->psel); 
      break; 

    case cache_policy_bip:

      SAT_CTR_DEC(global_data->psel);
      break;
    
    case cache_policy_dip:

      /* Nothing to do */
      break;

    default:
      panic("Unknown set type function %s line %d\n", __FUNCTION__, __LINE__);
  }
}
/* Get a replacement candidate */
int cache_replace_block_dip(dip_data *policy_data, dip_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);

  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_lru || 
      policy_data->following == cache_policy_bip ||
      policy_data->following == cache_policy_dip);

  /* According to the policy choose a replacement candidate */
  switch (GET_CURRENT_POLICY(policy_data, global_data))
  {
    case cache_policy_lru:

      return cache_replace_block_lru(&(policy_data->lru));
      break; 

    case cache_policy_bip:

      return cache_replace_block_bip(&(policy_data->bip));
      break;

    default:
      panic("Invalid follower function %s line %d\n", __FUNCTION__, __LINE__);
  }

  return -1;
}

/* Age the block as per policy */
void cache_access_block_dip(dip_data *policy_data, dip_gdata *global_data, 
  int way, int stream, memory_trace *info)
{
  assert(policy_data);
  assert(global_data);
  assert(way >= 0);
  assert(stream <= TST);

  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_lru || 
      policy_data->following == cache_policy_bip ||
      policy_data->following == cache_policy_dip);

  /* Update all component policy stack */
  switch (GET_CURRENT_POLICY(policy_data, global_data))
  {
    case cache_policy_lru:
      cache_access_block_lru(&(policy_data->lru), way, stream, info);
      break;

    case cache_policy_bip:
      cache_access_block_bip(&(policy_data->bip), way, stream, info);
      break;

    default:
      panic("Invalid follower finction %s line %d\n", __FUNCTION__, __LINE__);
  }
}

/* Update state of block. */
void cache_set_block_dip(dip_data *policy_data, dip_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_lru || 
      policy_data->following == cache_policy_bip ||
      policy_data->following == cache_policy_dip);

  /* Invoke component policy functions */
  switch (GET_CURRENT_POLICY(policy_data, global_data))
  {
    case cache_policy_lru:
      cache_set_block_lru(&(policy_data->lru), way, tag, state, stream, info);
      break;

    case cache_policy_bip:
      cache_set_block_bip(&(policy_data->bip), way, tag, state, stream, info);
      break;

    default:
      panic("Invalid follower function %s line %d", __FUNCTION__, __LINE__);
  }
}

/* Get tag and state of a block. */
struct cache_block_t cache_get_block_dip(dip_data *policy_data, dip_gdata *global_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  struct cache_block_t ret_block;

  assert(policy_data);
  assert(global_data);
  assert(way >= 0);
  assert(tag_ptr);
  assert(state_ptr);

  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_lru || 
      policy_data->following == cache_policy_bip ||
      policy_data->following == cache_policy_dip);

  switch (GET_CURRENT_POLICY(policy_data, global_data))
  {
    case cache_policy_lru:
      return cache_get_block_lru(&(policy_data->lru), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_bip:
      return cache_get_block_bip(&(policy_data->bip), way, tag_ptr, state_ptr, stream_ptr);

    default:
      panic("Invalid policy function %s line %d \n", __FUNCTION__, __LINE__);
  }
}

#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
#undef GET_CURRENT_POLICY
