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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <assert.h>

#include "cache.h"
#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "tapdrrip.h"

#define SRRIP_SAMPLED_SET       (0)
#define BRRIP_SAMPLED_SET       (1)
#define FOLLOWER_SET            (2)

/* Converts rop id to stream id */
#define ROP_ID(info)            ((info->mapped_rop  > 1) ? 3 : info->mapped_rop + 1)
#define CPU_ID(info)            (TST + info->pid)
#define STREAM_ID(info)         (info ? info->stream < TST ? ROP_ID(info) : CPU_ID(info) : NN)


#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))

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

extern long int *colorWriteV2FillRate;

static int get_set_type_tapdrrip(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x0f;
  msb_bits = (indx >> 6) & 0x0f;
  mid_bits = (indx >> 4) & 0x03;

  if (lsb_bits == msb_bits && mid_bits == 0)
  {
    return SRRIP_SAMPLED_SET;
  }
  else
  {
    if (lsb_bits == (~msb_bits & 0x0f) && mid_bits == 0)
    {
      return BRRIP_SAMPLED_SET;
    }
  }

  return FOLLOWER_SET;
}

static void cache_init_srrip_from_tapdrrip(struct cache_params *params, srrip_data *policy_data,
  tapdrrip_data *tapdrrip_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(tapdrrip_policy_data);

  /* Create per rrpv buckets */
  SRRIP_DATA_VALID_HEAD(policy_data) = TAPDRRIP_DATA_VALID_HEAD(tapdrrip_policy_data);
  SRRIP_DATA_VALID_TAIL(policy_data) = TAPDRRIP_DATA_VALID_TAIL(tapdrrip_policy_data);
  
  assert(SRRIP_DATA_VALID_HEAD(policy_data));
  assert(SRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIP_DATA_MAX_RRPV(policy_data) = params->max_rrpv;

  /* Create array of blocks */
  SRRIP_DATA_BLOCKS(policy_data) = TAPDRRIP_DATA_BLOCKS(tapdrrip_policy_data);

  SRRIP_DATA_FREE_HEAD(policy_data) = TAPDRRIP_DATA_FREE_HEAD(tapdrrip_policy_data);
  SRRIP_DATA_FREE_TAIL(policy_data) = TAPDRRIP_DATA_FREE_TAIL(tapdrrip_policy_data);

  /* Set current and default fill policy to SRRIP */
  SRRIP_DATA_CFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CRPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DRPOLICY(policy_data) = cache_policy_srrip;
}

static void cache_init_brrip_from_tapdrrip(struct cache_params *params, brrip_data *policy_data,
  tapdrrip_data *tapdrrip_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(tapdrrip_policy_data);

  /* Create per rrpv buckets */
  BRRIP_DATA_VALID_HEAD(policy_data) = TAPDRRIP_DATA_VALID_HEAD(tapdrrip_policy_data);
  BRRIP_DATA_VALID_TAIL(policy_data) = TAPDRRIP_DATA_VALID_TAIL(tapdrrip_policy_data);
  
  assert(BRRIP_DATA_VALID_HEAD(policy_data));
  assert(BRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  BRRIP_DATA_MAX_RRPV(policy_data) = params->max_rrpv;

  /* Create array of blocks */
  BRRIP_DATA_BLOCKS(policy_data) = TAPDRRIP_DATA_BLOCKS(tapdrrip_policy_data);

  BRRIP_DATA_FREE_HEAD(policy_data) = TAPDRRIP_DATA_FREE_HEAD(tapdrrip_policy_data);
  BRRIP_DATA_FREE_TAIL(policy_data) = TAPDRRIP_DATA_FREE_TAIL(tapdrrip_policy_data);

  /* Set current and default fill policy to SRRIP */
  BRRIP_DATA_CFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CRPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DRPOLICY(policy_data) = cache_policy_brrip;
}

void cache_init_tapdrrip(long long int set_indx, struct cache_params *params, tapdrrip_data *policy_data,
  tapdrrip_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(srrip_list)))

  /* Create per rrpv buckets */
  TAPDRRIP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  TAPDRRIP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

  assert(TAPDRRIP_DATA_VALID_HEAD(policy_data));
  assert(TAPDRRIP_DATA_VALID_TAIL(policy_data));

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    TAPDRRIP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    TAPDRRIP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    TAPDRRIP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  TAPDRRIP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

  /* Initialize array of blocks */
  TAPDRRIP_DATA_FREE_HEAD(policy_data) = &(TAPDRRIP_DATA_BLOCKS(policy_data)[0]);
  TAPDRRIP_DATA_FREE_TAIL(policy_data) = &(TAPDRRIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&TAPDRRIP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&TAPDRRIP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&TAPDRRIP_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&TAPDRRIP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&TAPDRRIP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&TAPDRRIP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV
#undef CACHE_ALLOC

  switch (get_set_type_tapdrrip(set_indx))
  {
    case SRRIP_SAMPLED_SET:

      cache_init_srrip_from_tapdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_tapdrrip(params, &(policy_data->brrip), policy_data);
      policy_data->following = cache_policy_srrip;
      break;

    case BRRIP_SAMPLED_SET:

      cache_init_srrip_from_tapdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_tapdrrip(params, &(policy_data->brrip), policy_data);
      policy_data->following = cache_policy_brrip;
      break;

    case FOLLOWER_SET:

      cache_init_srrip_from_tapdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_tapdrrip(params, &(policy_data->brrip), policy_data);
      policy_data->following = cache_policy_tapdrrip;
      break;

    default:

      panic("Unknown set type %s %d\n", __FUNCTION__, __LINE__);
  }
  
  if (set_indx == 0)
  {
#define MAX_VAL (1023)  
#define MID_VAL (512)

    /* Initialize policy selection counter */
    SAT_CTR_INI(global_data->psel, 10, 0, MAX_VAL);
    SAT_CTR_SET(global_data->psel, MID_VAL);

#undef MAX_VAL
#undef MID_VAL

#define MAX_VAL (1023)  

    SAT_CTR_INI(global_data->gpuaccess, 10, 0, MAX_VAL);
    SAT_CTR_INI(global_data->cpuaccess, 10, 0, MAX_VAL);
    SAT_CTR_SET(global_data->gpuaccess, 0);
    SAT_CTR_SET(global_data->cpuaccess, 0);

#undef MAX_VAL

    global_data->core_policy1 = params->core_policy1;
    global_data->core_policy2 = params->core_policy2;
    global_data->txs          = params->txs;
    global_data->talpha       = params->talpha;
    global_data->taprripmask  = FALSE;

    global_data->srrip_followed = 0;
    global_data->brrip_followed = 0;

#define MAX_VAL (255)  

    SAT_CTR_INI(global_data->brrip.access_ctr, 8, 0, MAX_VAL);
    global_data->brrip.threshold = params->threshold;

#undef MAX_VAL
  }
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_tapdrrip(tapdrrip_data *policy_data)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Free component policies */
  cache_free_srrip(&(policy_data->srrip));
  cache_free_brrip(&(policy_data->brrip));
}

struct cache_block_t * cache_find_block_tapdrrip(tapdrrip_gdata *global_data,
  tapdrrip_data *policy_data, long long tag, memory_trace *info)
{
  assert(1);
}

void cache_fill_block_tapdrrip(tapdrrip_data *policy_data, tapdrrip_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int strm,
  memory_trace  *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);

  assert(policy_data->following == cache_policy_srrip ||
    policy_data->following == cache_policy_brrip || 
    policy_data->following == cache_policy_tapdrrip);
  
  switch (policy_data->following)
  {
    case cache_policy_srrip:

      SAT_CTR_INC(global_data->psel);
      break;

    case cache_policy_brrip:

      SAT_CTR_DEC(global_data->psel);
      break;
    
    case cache_policy_tapdrrip:

      /* Nothing to do */
      break;

    default:

      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }
  
  if (STREAM_ID(info) == global_data->core_policy1)
  {
    cache_set_current_fill_policy_srrip(&(policy_data->srrip), cache_policy_lru);
    cache_set_current_fill_policy_brrip(&(policy_data->brrip), cache_policy_lru);
  }
  else
  {
    if(STREAM_ID(info) == global_data->core_policy2)
    {
      cache_set_current_fill_policy_srrip(&(policy_data->srrip), cache_policy_lip);
      cache_set_current_fill_policy_brrip(&(policy_data->brrip), cache_policy_lip);
    }
    else
    {
      /* For graphics if taprripmask is set fillow SRRIP policy */
      if (STREAM_ID(info) < TST && global_data->taprripmask == TRUE)
      {
        cache_set_current_fill_policy_srrip(&(policy_data->srrip), cache_policy_brrip);
        cache_set_current_fill_policy_brrip(&(policy_data->brrip), cache_policy_brrip);
      }
    }
  }

  /* Fill block in all component policies */
  cache_fill_block_srrip(&(policy_data->srrip), &(global_data->srrip), way, tag, state,
    STREAM_ID(info), info);
  cache_fill_block_brrip(&(policy_data->brrip),&(global_data->brrip), way, tag,
    state, STREAM_ID(info), info);
}

int cache_replace_block_tapdrrip(tapdrrip_data *policy_data, 
  tapdrrip_gdata *global_data, memory_trace *info)
{
  /* Ensure vaid arguments */
  assert(policy_data);
  assert(global_data);
  
  /* According to the policy choose a replacement candidate */
  switch (policy_data->following)
  {
    case cache_policy_srrip:

      return cache_replace_block_srrip(&(policy_data->srrip), 
          &(global_data->srrip), info);
      break; 

    case cache_policy_brrip:

      return cache_replace_block_brrip(&(policy_data->brrip), info);
      break;

    case cache_policy_tapdrrip:

#define MID_VAL (512)
      
      /* For GPU if rrip mask is set replace GPU block first */
      if (STREAM_ID(info) < TST && global_data->taprripmask == TRUE)
      {
        cache_set_current_replacement_policy_srrip(&(policy_data->srrip),
          cache_policy_cpulast);
        cache_set_current_replacement_policy_brrip(&(policy_data->brrip),
          cache_policy_cpulast);
      }

      if (SAT_CTR_VAL(global_data->psel) < MID_VAL)
      {
        /* Follow SRRIP */  
        global_data->srrip_followed += 1;
        return cache_replace_block_srrip(&(policy_data->srrip), 
            &(global_data->srrip), info);
      }
      else
      {
        /* Follow BRRIP */
        global_data->brrip_followed += 1;
        return cache_replace_block_brrip(&(policy_data->brrip), info);
      }

#undef MID_VAL
      break;

    default:

      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }

  return -1;
}

void cache_access_block_tapdrrip(tapdrrip_data *policy_data, 
  tapdrrip_gdata *global_data, int way, int strm, memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  
  if (STREAM_ID(info) < TST && global_data->taprripmask)
  {
    cache_set_current_access_policy_srrip(&(policy_data->srrip),
      cache_policy_bypass);
    cache_set_current_access_policy_brrip(&(policy_data->brrip),
      cache_policy_bypass);
  }

  cache_access_block_srrip(&(policy_data->srrip), &(global_data->srrip), way,
      strm, info);
  cache_access_block_brrip(&(policy_data->brrip), &(global_data->brrip), way, strm, info);
}

/* Update state of block. */
void cache_set_block_tapdrrip(tapdrrip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Call component policies */
  cache_set_block_srrip(&(policy_data->srrip), way, tag, state, stream, info);
  cache_set_block_brrip(&(policy_data->brrip), way, tag, state, stream, info);
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_tapdrrip(tapdrrip_data *policy_data, tapdrrip_gdata *global_data,
  int way, long long *tag_ptr, enum cache_block_state_t *state_ptr, 
  int *stream_ptr)
{
  struct cache_block_t ret_block;    

  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  assert(tag_ptr);
  assert(state_ptr);
  
  switch (policy_data->following)
  {
    case cache_policy_srrip:
      
      /* Follow SRRIP */
      return cache_get_block_srrip(&(policy_data->srrip), way, tag_ptr, 
        state_ptr, stream_ptr);
      break;

    case cache_policy_brrip:
      
      /* Follow BRRIP */
      return cache_get_block_brrip(&(policy_data->brrip), way, tag_ptr,
        state_ptr, stream_ptr);
      break;

    case cache_policy_tapdrrip:

      /* Follow policy based on policy selection counter  */
      if (SAT_CTR_VAL(global_data->psel) > 512)
      {
        /* Follow SRRIP policy */
        return cache_get_block_srrip(&(policy_data->srrip), way, tag_ptr,
          state_ptr, stream_ptr);
      }
      else
      {
        /* Follow BRRIP policy */
        return cache_get_block_brrip(&(policy_data->brrip), way, tag_ptr, 
          state_ptr, stream_ptr);
      }

      break;

    default:

      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
      return ret_block;
  }
}
