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
#include "helmdrrip.h"

#define SRRIP_SAMPLED_SET       (1)
#define BRRIP_SAMPLED_SET       (2)
#define GPUCORE1_SAMPLED_SET    (3)
#define GPUCORE2_SAMPLED_SET    (4)
#define CPUCORE1_SAMPLED_SET    (5)
#define CPUCORE2_SAMPLED_SET    (6)
#define FOLLOWER_SET            (7)

/* Converts rop id to stream id */
#define ROP_ID(info)            ((info->mapped_rop  > 1) ? 3 : info->mapped_rop + 1)
#define CPU_ID(info)            (TST + info->pid)
#define STREAM_ID(info)         (info ? info->stream < TST ? info->stream : CPU_ID(info) : NN)


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
extern shader_info  shader_global_info;

static int get_set_type_helmdrrip(long long int indx)
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
    else
    {
      if (lsb_bits == msb_bits && mid_bits == 0x3)
      {
        return CPUCORE1_SAMPLED_SET;
      }
      else
      {
        if (lsb_bits == (~msb_bits & 0x0f) && mid_bits == 0x03)
        {
          return CPUCORE2_SAMPLED_SET;
        }
      }
    }
  }

  return FOLLOWER_SET;
}

static void cache_init_srrip_from_helmdrrip(struct cache_params *params, srrip_data *policy_data,
  helmdrrip_data *helmdrrip_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(helmdrrip_policy_data);

  /* Create per rrpv buckets */
  SRRIP_DATA_VALID_HEAD(policy_data) = HELMDRRIP_DATA_VALID_HEAD(helmdrrip_policy_data);
  SRRIP_DATA_VALID_TAIL(policy_data) = HELMDRRIP_DATA_VALID_TAIL(helmdrrip_policy_data);
  
  assert(SRRIP_DATA_VALID_HEAD(policy_data));
  assert(SRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIP_DATA_MAX_RRPV(policy_data) = params->max_rrpv;

  /* Create array of blocks */
  SRRIP_DATA_BLOCKS(policy_data) = HELMDRRIP_DATA_BLOCKS(helmdrrip_policy_data);

  SRRIP_DATA_FREE_HEAD(policy_data) = HELMDRRIP_DATA_FREE_HEAD(helmdrrip_policy_data);
  SRRIP_DATA_FREE_TAIL(policy_data) = HELMDRRIP_DATA_FREE_TAIL(helmdrrip_policy_data);

  /* Set current and default fill policy to SRRIP */
  SRRIP_DATA_CFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CRPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DRPOLICY(policy_data) = cache_policy_srrip;
}

static void cache_init_brrip_from_helmdrrip(struct cache_params *params, brrip_data *policy_data,
  helmdrrip_data *helmdrrip_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(helmdrrip_policy_data);

  /* Create per rrpv buckets */
  BRRIP_DATA_VALID_HEAD(policy_data) = HELMDRRIP_DATA_VALID_HEAD(helmdrrip_policy_data);
  BRRIP_DATA_VALID_TAIL(policy_data) = HELMDRRIP_DATA_VALID_TAIL(helmdrrip_policy_data);
  
  assert(BRRIP_DATA_VALID_HEAD(policy_data));
  assert(BRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  BRRIP_DATA_MAX_RRPV(policy_data) = params->max_rrpv;

  /* Create array of blocks */
  BRRIP_DATA_BLOCKS(policy_data) = HELMDRRIP_DATA_BLOCKS(helmdrrip_policy_data);

  BRRIP_DATA_FREE_HEAD(policy_data) = HELMDRRIP_DATA_FREE_HEAD(helmdrrip_policy_data);
  BRRIP_DATA_FREE_TAIL(policy_data) = HELMDRRIP_DATA_FREE_TAIL(helmdrrip_policy_data);

  /* Set current and default fill policy to SRRIP */
  BRRIP_DATA_CFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CRPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DRPOLICY(policy_data) = cache_policy_brrip;
}

void cache_init_helmdrrip(long long int set_indx, struct cache_params *params, helmdrrip_data *policy_data,
  helmdrrip_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(srrip_list)))

  /* Create per rrpv buckets */
  HELMDRRIP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  HELMDRRIP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

  assert(HELMDRRIP_DATA_VALID_HEAD(policy_data));
  assert(HELMDRRIP_DATA_VALID_TAIL(policy_data));

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    HELMDRRIP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    HELMDRRIP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    HELMDRRIP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  HELMDRRIP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

  /* Initialize array of blocks */
  HELMDRRIP_DATA_FREE_HEAD(policy_data) = &(HELMDRRIP_DATA_BLOCKS(policy_data)[0]);
  HELMDRRIP_DATA_FREE_TAIL(policy_data) = &(HELMDRRIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&HELMDRRIP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&HELMDRRIP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&HELMDRRIP_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&HELMDRRIP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&HELMDRRIP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&HELMDRRIP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV
#undef CACHE_ALLOC

  switch (get_set_type_helmdrrip(set_indx))
  {
    case SRRIP_SAMPLED_SET:
      cache_init_srrip_from_helmdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_helmdrrip(params, &(policy_data->brrip), policy_data);

      policy_data->following    = cache_policy_srrip;
      policy_data->helm_settype = SRRIP_SAMPLED_SET;
      break;

    case BRRIP_SAMPLED_SET:
      cache_init_srrip_from_helmdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_helmdrrip(params, &(policy_data->brrip), policy_data);

      policy_data->following    = cache_policy_brrip;
      policy_data->helm_settype = BRRIP_SAMPLED_SET;
      break;

    case CPUCORE1_SAMPLED_SET:
      cache_init_srrip_from_helmdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_helmdrrip(params, &(policy_data->brrip), policy_data);

      policy_data->following    = cache_policy_helmdrrip;
      policy_data->helm_settype = CPUCORE1_SAMPLED_SET;
      break;

    case CPUCORE2_SAMPLED_SET:
      cache_init_srrip_from_helmdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_helmdrrip(params, &(policy_data->brrip), policy_data);

      policy_data->following    = cache_policy_helmdrrip;
      policy_data->helm_settype = CPUCORE2_SAMPLED_SET;
      break;

    case FOLLOWER_SET:
      cache_init_srrip_from_helmdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_helmdrrip(params, &(policy_data->brrip), policy_data);

      policy_data->following    = cache_policy_helmdrrip;
      policy_data->helm_settype = FOLLOWER_SET;
      break;

    default:
      panic("Unknown set type %s %d\n", __FUNCTION__, __LINE__);
  }
  
  if (set_indx == 0)
  {
#define MAX_VAL (0x3ff)  
#define MID_VAL (1 << 8)

    /* Initialize policy selection counter */
    SAT_CTR_INI(global_data->psel, 10, 0, MAX_VAL);
    SAT_CTR_SET(global_data->psel, MID_VAL);

#undef MAX_VAL
#undef MID_VAL

#define MAX_VAL (0x1fffff)  

    SAT_CTR_INI(global_data->access, 21, 0, MAX_VAL);
    SAT_CTR_SET(global_data->access, 0);

#undef MAX_VAL

#define MAX_VAL (0x3ff)  

    SAT_CTR_INI(global_data->gpuaccess, 10, 0, MAX_VAL);
    SAT_CTR_INI(global_data->cpuaccess, 10, 0, MAX_VAL);
    SAT_CTR_SET(global_data->gpuaccess, 0);
    SAT_CTR_SET(global_data->cpuaccess, 0);

#undef MAX_VAL

    global_data->core_policy1   = params->core_policy1;
    global_data->core_policy2   = params->core_policy2;
    global_data->highthr        = params->highthr;
    global_data->lowthr         = params->lowthr;
    global_data->tlpthr         = params->tlpthr;
    global_data->mthr           = params->mthr;
    global_data->pthr           = params->pthr;
    global_data->cpucore1_miss  = 0;
    global_data->cpucore2_miss  = 0;

    global_data->srrip_followed     = 0;
    global_data->brrip_followed     = 0;
    global_data->gpucore1_followed  = 0;
    global_data->gpucore2_followed  = 0;
    global_data->cpucore1_followed  = 0;
    global_data->cpucore2_followed  = 0;

#define MAX_VAL (255)  

    SAT_CTR_INI(global_data->brrip.access_ctr, 8, 0, MAX_VAL);
    global_data->brrip.threshold = params->threshold;

#undef MAX_VAL
  }
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_helmdrrip(helmdrrip_data *policy_data)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Free component policies */
  cache_free_srrip(&(policy_data->srrip));
  cache_free_brrip(&(policy_data->brrip));
}

/* Obtain TLP threshold */
static void cache_get_tlp_threshold_helmdrrip(helmdrrip_gdata *global_data)
{
  assert(1);
}

static void cache_get_low_high_threshold_helmdrrip(helmdrrip_gdata *global_data)
{
  ub8 expected;
  ub8 predicted;

  predicted = global_data->tlpthr;
  expected  = (global_data->highthr + global_data->lowthr) / 2;

  if (predicted < expected)
  {
    global_data->highthr = expected;
  }
  else
  {
    global_data->lowthr = expected; 
  }
}

struct cache_block_t * cache_find_block_helmdrrip(helmdrrip_gdata *global_data,
  helmdrrip_data *policy_data, long long tag, memory_trace *info)
{
  int    max_rrpv;
  struct cache_block_t *head;
  struct cache_block_t *node;

  max_rrpv  = policy_data->srrip.max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = HELMDRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

end:
  if (SAT_CTR_VAL(global_data->access) == (1 << 19))
  {
    /* Find TLP threshold */
    cache_get_tlp_threshold_helmdrrip(global_data);

    /* Find new low and high threshold */  
    cache_get_low_high_threshold_helmdrrip(global_data);
  }

  /* Update GPU / CPU side access counter */
  if (info && info->stream < TST)
  {
    SAT_CTR_INC(global_data->gpuaccess);
  }
  else
  {
    SAT_CTR_INC(global_data->cpuaccess);
  }
  
  SAT_CTR_INC(global_data->gpuaccess);

  return node;
}

void cache_fill_block_helmdrrip(helmdrrip_data *policy_data, helmdrrip_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int strm,
  memory_trace  *info)
{
  assert(1);
}

int cache_replace_block_helmdrrip(helmdrrip_data *policy_data, 
  helmdrrip_gdata *global_data, memory_trace *info)
{
  /* Ensure vaid arguments */
  assert(policy_data);
  assert(global_data);
  
  /* According to the policy choose a replacement candidate */
  switch (policy_data->following)
  {
    case cache_policy_srrip:

      return cache_replace_block_srrip(&(policy_data->srrip), &(global_data->srrip));
      break; 

    case cache_policy_brrip:

      return cache_replace_block_brrip(&(policy_data->brrip));
      break;

    case cache_policy_helmdrrip:

#define MID_VAL (512)
      
      if (SAT_CTR_VAL(global_data->psel) < MID_VAL)
      {
        /* Follow SRRIP */  
        global_data->srrip_followed += 1;
        return cache_replace_block_srrip(&(policy_data->srrip), &(global_data->srrip));
      }
      else
      {
        /* Follow BRRIP */
        global_data->brrip_followed += 1;
        return cache_replace_block_brrip(&(policy_data->brrip));
      }

#undef MID_VAL
      break;

    default:

      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }

  return -1;
}

void cache_access_block_helmdrrip(helmdrrip_data *policy_data, 
  helmdrrip_gdata *global_data, int way, int strm, memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  
  cache_access_block_srrip(&(policy_data->srrip), &(global_data->srrip), way, strm, info);
  cache_access_block_brrip(&(policy_data->brrip), &(global_data->brrip), way, strm, info);
}

/* Update state of block. */
void cache_set_block_helmdrrip(helmdrrip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Call component policies */
  cache_set_block_srrip(&(policy_data->srrip), way, tag, state, stream, info);
  cache_set_block_brrip(&(policy_data->brrip), way, tag, state, stream, info);
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_helmdrrip(helmdrrip_data *policy_data, helmdrrip_gdata *global_data,
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

    case cache_policy_helmdrrip:

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
