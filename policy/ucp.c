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
#include "ucp.h"

#define MAX_CORES               (128)
#define LRU_SAMPLED_SET         (0)
#define FOLLOWER_SET            (2)

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))

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

static void cache_get_new_partition_ucp(ucp_gdata *global_data);

/* Obtain set type based on index */
static int get_set_type_ucp(long long int indx)
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

  return FOLLOWER_SET;
}
 
/* Initialize policy specific data */
void cache_init_ucp(long long int set_indx, struct cache_params *params, 
  ucp_data *policy_data, ucp_gdata *global_data)
{
  int i;
  int j;  

  /* Flag to ensure that global data is initialized exactly once */
  static ub1 global_init = FALSE;

  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);

  /* Initialize global data */
  if (set_indx == 0)
  {
    assert(global_init == FALSE);
    global_init = TRUE;

    global_data->streams  = params->streams;
    global_data->ways     = params->ways;

    assert(params->streams >= 0 && params->streams <= TST + MAX_CORES);

    /* Initialze global data */
    assert(!global_data->per_stream_partition);

    global_data->per_stream_partition  = (ub4 *)xmalloc(sizeof(unsigned int) * params->streams);

    assert(global_data->per_stream_partition);

    global_data->umon = 
      (struct saturating_counter_t **)xmalloc(
      sizeof(struct saturating_counter_t *) * params->streams);

    for (i = 0; i < params->streams; i++)
    {
      global_data->umon[i] = 
        (struct saturating_counter_t *)xmalloc(
         sizeof(struct saturating_counter_t ) * params->ways);
    }

    for (i = 0; i < params->streams; i++)
    {
      for (j = 0; j < params->ways; j++)
      {
#define MAX_VAL ((long long)1 << 20)  

      /* Initialize policy selection counter */
      SAT_CTR_INI(global_data->umon[i][j], 21, 0, MAX_VAL);
      SAT_CTR_SET(global_data->umon[i][j], 0);

#undef MAX_VAL
      }
    }
    
    cache_get_new_partition_ucp(global_data);

    assert(global_data->umon);
  }
  
  switch (get_set_type_ucp(set_indx))
  {
    case LRU_SAMPLED_SET:

      /* Initialize both sampling and following policy */
      cache_init_lru(params, &(policy_data->lru));
      cache_init_salru(params, &(policy_data->salru));
      policy_data->following = cache_policy_ucp;
      break;

    case FOLLOWER_SET:

      /* Initialize only flollower policy */
      cache_init_salru(params, &(policy_data->salru));
      policy_data->following = cache_policy_salru;
      break;

    default:

      panic("Unknown set type %s %d\n", __FUNCTION__, __LINE__);
  }
  
  /* Set partition for salru */
  cache_set_partition(&(policy_data->salru), global_data->per_stream_partition);

#define MAX_VAL (0x1fffff)  

  /* Initialize policy selection counter */
  SAT_CTR_INI(global_data->access, 21, 0, MAX_VAL);
  SAT_CTR_SET(global_data->access, 0);

#undef MAX_VAL
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_ucp(long long set_indx, ucp_data *policy_data, ucp_gdata *global_data)
{
  int i;

  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  
  if (set_indx == 0)
  {
    /* Free global data */
    for (i = 0; i < global_data->streams; i++)
    {
      free(global_data->umon[i]);
    }

    free(global_data->umon);
    free(global_data->per_stream_partition);
  }

  /* Free component policies */
  cache_free_lru(&(policy_data->lru));
  cache_free_salru(&(policy_data->salru));
}

/* Age the block as per policy */
static void cache_access_block_ucp_lru(ucp_data *policy_data, int way, int stream,
  memory_trace *info)
{
  assert(policy_data);
  assert(way >= 0);
  assert(stream <= TST + MAX_CORES);
  assert(policy_data->following == cache_policy_ucp);

  /* First run the policy in UMON */
  cache_access_block_lru(&(policy_data->lru), way, stream, info);
}

/* Find block with the given tag */
static struct cache_block_t* cache_find_block_ucp_lru(ucp_data *policy_data, 
  long long tag)
{
  assert(policy_data->following == cache_policy_ucp);
  return cache_find_block_lru(&(policy_data->lru), tag);
}

static int get_util_val(ucp_gdata *global_data, int strm_in, int oalloc, int nalloc)
{
#define UTILITY(data, strm, alloc) (SAT_CTR_VAL(data->umon[strm][alloc]))

  return (UTILITY(global_data, strm_in, nalloc) - UTILITY(global_data, strm_in, oalloc));

#undef UTILITY
}

static void cache_get_new_partition_ucp(ucp_gdata *global_data)
{
  int balance;  /* Blocks left to be allocated */
  int util;     /* Temporary for utility */
  int alloc;    /* Temporary for current allocation */
  int i;        /* Index variable */ 
  int maxU;     /* Temporary for max utility */
  int maxi;     /* Temporary for max utility stream */

  assert(global_data);
  assert(global_data->ways >= global_data->streams);
  
  memset(global_data->per_stream_partition, 0, sizeof(int) * global_data->streams);
  
  /* Give one block to each stream */
  for (i = 0; i < global_data->streams; i++)
  {
    global_data->per_stream_partition[i] = 1;
  }
  
  /* Set balance to number of ways */
  balance = global_data->ways - global_data->streams;
  assert(balance >= 0);

  while (balance)
  {
    maxU = 0;
    maxi = 0;
    for (i = 0; i < global_data->streams; i++)
    {
      alloc = global_data->per_stream_partition[i];
      util  = get_util_val(global_data, i, alloc, alloc + 1);
      if (util >= maxU)
      {
        maxU = util; 
        maxi = i;
      }
    }
    
    if (maxU == 0)
      break;

    /* Assign new way to the stream with max gain */
    balance -= 1;
    global_data->per_stream_partition[maxi] += 1;
  }

  if (balance)
  {
    /* Give one block to each stream */
    for (i = 0; i < global_data->streams && balance; i++)
    {
      global_data->per_stream_partition[i] += 1;

      balance -= 1;
    }
  }
}

/* Fill new block in the cache */
static void cache_fill_block_ucp_lru(ucp_data *policy_data, int way, 
  long long tag, memory_trace *info)
{
  /* Ensure valid arguments */ 
  assert(policy_data);
  assert(policy_data->following == cache_policy_ucp);

  /* Fill block in lru stack */
  cache_fill_block_lru(&(policy_data->lru), way, tag, cache_block_exclusive,
    info->stream, info);
}

/* Get a replacement candidate */
int cache_replace_block_ucp_lru(ucp_data *policy_data)
{
  return cache_replace_block_lru(&(policy_data->lru));
}

/* Find block with the given tag */
struct cache_block_t* cache_find_block_ucp(ucp_data *policy_data, 
  ucp_gdata *global_data, long long tag, memory_trace *info)
{
  struct  cache_block_t *block;
  int     way;
  int     i;
  int     j;

  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_ucp || 
    policy_data->following == cache_policy_salru);

  /* If set is sampled run the policy in UMON */
  if (policy_data->following == cache_policy_ucp)
  {
    /* First run the policy in UMON */
    block = cache_find_block_ucp_lru(policy_data, tag);
    if (block)
    {
      /* Update UMON counter */
      SAT_CTR_INC(global_data->umon[info->stream - 1][block->way]);

      /* Update the recency */
      cache_access_block_ucp_lru(policy_data, block->way, info->stream, info);
    }
    else
    {
      /* Get the replacement */
      way = cache_replace_block_ucp_lru(policy_data);
      assert(way != -1);
      
      long long vctm_tag;
      enum cache_block_state_t vctm_state;
      int  vctm_stream;

      cache_get_block_lru(&(policy_data->lru), way, &vctm_tag, &vctm_state,
        &vctm_stream);
      
      if (vctm_state != cache_block_invalid)
      {
        cache_set_block_lru(&(policy_data->lru), way, vctm_tag, cache_block_invalid, 
          vctm_stream, info);
      } 

      /* Fill the block */
      cache_fill_block_ucp_lru(policy_data, way, tag, info);
    }

    if (SAT_CTR_VAL(global_data->access) == (1 << 20))
    {
      cache_get_new_partition_ucp(global_data); 
      
      /* Reset access counter */
      SAT_CTR_RST(global_data->access);
      
      /* Reset per stream umon counters */
      for (i = 0; i < global_data->streams; i++)
      {
        for (j = 0; j < global_data->ways; j++)
        {
          SAT_CTR_HLF(global_data->umon[i][j]);
        }
      }
    }
  }

  SAT_CTR_INC(global_data->access);

  return cache_find_block_salru(&(policy_data->salru), tag);
}

/* Fill new block in the cache */
void cache_fill_block_ucp(ucp_data *policy_data, ucp_gdata *global_data,
  int way, long long tag, enum cache_block_state_t state, int stream, 
  memory_trace *info)
{
  /* Ensure valid arguments */ 
  assert(policy_data);
  assert(global_data);
  assert(info);

  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_ucp || 
    policy_data->following == cache_policy_salru);
  
  /* Fill block in all component policies */
  cache_fill_block_salru(&(policy_data->salru), way, tag, state, stream, info);
}

/* Get a replacement candidate */
int cache_replace_block_ucp(ucp_data *policy_data, ucp_gdata *global_data,
  memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);

  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_ucp || 
    policy_data->following == cache_policy_salru);

  return cache_replace_block_salru(&(policy_data->salru), NN, info);
}


/* Age the block as per policy */
void cache_access_block_ucp(ucp_data *policy_data, ucp_gdata *global_data, 
  int way, int stream, memory_trace *info)
{
  assert(policy_data);
  assert(way >= 0);
  assert(stream <= TST + MAX_CORES);
   
  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_ucp || 
    policy_data->following == cache_policy_salru);

  cache_access_block_salru(&(policy_data->salru), way, stream, info);
}

/* Update state of block. */
void cache_set_block_ucp(ucp_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_ucp || 
    policy_data->following == cache_policy_salru);

  cache_set_block_salru(&(policy_data->salru), way, tag, state, stream, info);
}

/* Get tag and state of a block. */
struct cache_block_t cache_get_block_ucp(ucp_data *policy_data, ucp_gdata *global_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  assert(policy_data);
  assert(global_data);
  assert(way >= 0);
  assert(tag_ptr);
  assert(state_ptr);

  /* Ensure set policy is valid */
  assert(policy_data->following == cache_policy_ucp || 
    policy_data->following == cache_policy_salru);

  /* Follow UCP */
  return cache_get_block_salru(&(policy_data->salru), way, tag_ptr, state_ptr,
    stream_ptr);
}
