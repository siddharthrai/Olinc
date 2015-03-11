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

#ifndef MEM_SYSTEM_tapdrrip_H
#define	MEM_SYSTEM_tapdrrip_H

#ifdef __cplusplus
# define EXPORT_C extern "C"
#else
# define EXPORT_C
#endif

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "policy.h"
#include "cache-param.h"
#include "cache-block.h"
#include "rrip.h"
#include "srrip.h"
#include "brrip.h"

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_tapdrrip_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}tapdrrip_list;

#define TAPDRRIP_DATA_BLOCKS(data)     ((data)->blocks)
#define TAPDRRIP_DATA_VALID_HEAD(data) ((data)->valid_head)
#define TAPDRRIP_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define TAPDRRIP_DATA_FREE_HEAD(data)  ((data)->free_head)
#define TAPDRRIP_DATA_FREE_TAIL(data)  ((data)->free_tail)

/* TAPDRRIP specific data */
typedef struct cache_policy_tapdrrip_data_t
{
  cache_policy_t  following;
  srrip_data      srrip;
  brrip_data      brrip;

  rrip_list      *valid_head;      /* Head pointers of RRPV specific list */
  rrip_list      *valid_tail;      /* Tail pointers of RRPV specific list */

  struct cache_block_t *blocks;    /* Actual blocks */
  struct cache_block_t *free_head; /* Free list head */
  struct cache_block_t *free_tail; /* Free list tail */
}tapdrrip_data;

/* Policy global data */
typedef struct cache_policy_tapdrrip_gdata_t
{
  sctr        psel;           /* Policy selection counter */
  ub8         srrip_followed; /* # sets following SRRIP */
  ub8         brrip_followed; /* # sets following BRRIP */
  ub1         taprripmask;    /* Flag set if cache is not effective for GPU */
  ub1         txs;            /* Threshold for block life time normalization */
  ub1         talpha;         /* Threshold for performance variance  */
  sctr        gpuaccess;      /* Number of GPU access */
  sctr        cpuaccess;      /* Number of CPU access */
  ub4         core_policy1;   /* Core id following first policy */
  ub4         core_policy2;   /* Core id following second policy */  
  brrip_gdata brrip;          /* BRRIP set wide data */
}tapdrrip_gdata;

/*
 *
 * NAME
 *  
 *  CacheInit - Initialize policy
 *
 * DESCRIPTION
 *
 *  Initializes policy specific data structures 
 *
 * PARAMETERS
 *  
 *  params      (IN)  - Policy specific parameters
 *  policy_data (IN)  - Cache set to be initialized 
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTES
 */

void cache_init_tapdrrip(long long int set_indx, struct cache_params *params, 
  tapdrrip_data *policy_data, tapdrrip_gdata *global_data);

/*
 *
 * NAME
 *  
 *  CacheFree - Free policy
 *
 * DESCRIPTION
 *  
 *  Deallocates all policy specific data
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Set to be freed 
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_free_tapdrrip(tapdrrip_data *policy_data);

/*
 *
 * NAME
 *  
 *  CacheFindBlock - Find block in given set
 *
 * DESCRIPTION
 *  
 *  Finds the block with given tag in the valid list 
 *
 * PARAMETERS
 *  
 *  global_data (IN)  - Cache wide global data
 *  policy_data (IN)  - Cache set in which block is to be looked
 *  tag         (IN)  - Block tag
 *
 * RETURNS
 *  
 *  Pointer to the block 
 */

struct cache_block_t * cache_find_block_tapdrrip(tapdrrip_gdata *global_data, 
  tapdrrip_data *policy_data, long long tag, memory_trace *info);

/*
 *
 * NAME
 *
 *  CacheFillBlock - Fill the block into the set
 *
 * DESCRIPTION
 *  
 *  Update block tag, state and stream 
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Set of the block
 *  way         (IN)  - Way of the block
 *  tag         (IN)  - Tag to be set
 *  state       (IN)  - State to be updated
 *  strm        (IN)  - Access stream
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_fill_block_tapdrrip(tapdrrip_data *policy_data, tapdrrip_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int strm,
  memory_trace *info);

/*
 *
 * NAME
 *  
 *  CacheReplaceBlock - Replace a block
 *
 * DESCRIPTION
 *
 *  Get way from where to replace a block
 *
 * PARAMETERS
 *
 *  policy_data (IN)  - Set of the block 
 *
 * RETURNS
 *  
 *  Way id from where to replace the block
 *
 */

int  cache_replace_block_tapdrrip(tapdrrip_data *policy_data, 
  tapdrrip_gdata *global_data, memory_trace *info);

/*
 *
 * NAME
 *  
 *  CacheAccessBlock - Access a block already in the cache
 *
 * DESCRIPTION
 *    
 *  Update block state on hit
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Set of the block
 *  way         (IN)  - Physical way of the block
 *  strm        (IN)  - Stream accessing the block
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 *
 */

void cache_access_block_tapdrrip(tapdrrip_data *policy_data, tapdrrip_gdata *global_data,
  int way, int strm, memory_trace *info);

/*
 *
 * NAME
 *  
 *  CacheGetBlock  - Get block
 *
 * DESCRIPTION
 *  
 *  Fetches block data for a given way
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Set of the block 
 *  way         (IN)  - Way of the block
 *  tag_ptr     (OUT) - Tag of the block
 *  state_ptr   (OUT) - Current state
 *
 * RETURNS
 *  
 *  Complete block info
 */

struct cache_block_t cache_get_block_tapdrrip(tapdrrip_data *policy_data, 
  tapdrrip_gdata *global_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *stream_ptr);

/*
 *
 * NAME
 *
 *  CacheSetBlock - Cache set block
 *
 * DESCRIPTION
 *  
 *  Update state of the block
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Set of the block
 *  way         (IN)  - Way of the block
 *  tag         (IN)  - Block tag
 *  state       (IN)  - New state
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_set_block_tapdrrip(tapdrrip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
