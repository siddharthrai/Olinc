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

#ifndef MEM_SYSTEM_SALRU_H
#define	MEM_SYSTEM_SALRU_H

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "policy.h"
#include "cache-param.h"
#include "cache-block.h"

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_salru_t
{
  ub4 stream;
  struct cache_block_t *head;
}salru_list;

#define SALRU_DATA_STREAMS(data)    ((data)->streams)
#define SALRU_DATA_WAYS(data)       ((data)->ways)
#define SALRU_DATA_CFPOLICY(data)   ((data)->current_fill_policy)
#define SALRU_DATA_DFPOLICY(data)   ((data)->default_fill_policy)
#define SALRU_DATA_BLOCKS(data)     ((data)->blocks)
#define SALRU_DATA_VALID_HEAD(data) ((data)->valid_head)
#define SALRU_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define SALRU_DATA_FREE_HEAD(data)  ((data)->free_head)
#define SALRU_DATA_FREE_TAIL(data)  ((data)->free_tail)
#define SALRU_DATA_PARTITIONS(data) ((data)->per_stream_partition)
#define SALRU_DATA_ALLOCATED(data)  ((data)->per_stream_allocated)

/* SALRU specific data */
typedef struct cache_policy_salru_data_t
{
  ub4            streams;             /* Number of streams */
  ub4            ways;                /* Number of ways */
  cache_policy_t current_fill_policy; /* If non-default fill policy is enforced */
  cache_policy_t default_fill_policy; /* Default fill policy */
  salru_list    *valid_head;          /* Head pointers of RRPV specific list */
  salru_list    *valid_tail;          /* Tail pointers of RRPV specific list */

  struct cache_block_t *blocks;       /* Actual blocks */
  struct cache_block_t *free_head;    /* Free list head */
  struct cache_block_t *free_tail;    /* Free list tail */
  ub4    *per_stream_partition;       /* Per stream partition */
  ub4    *per_stream_allocated;       /* Per stream partition */
}salru_data;

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

void cache_init_salru(struct cache_params *params, salru_data *policy_data);

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

void cache_free_salru(salru_data *policy_data);

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
 *  policy_data (IN)  - Cache set in which block is to be looked
 *  tag         (IN)  - Block tag
 *
 * RETURNS
 *  
 *  Pointer to the block 
 */

struct cache_block_t * cache_find_block_salru(salru_data *policy_data, long long tag);

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

void cache_fill_block_salru(salru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info);

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

int  cache_replace_block_salru(salru_data *policy_data, int stream, memory_trace *info);

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
 *  strm        (IN)  - Access stream
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 *
 */

void cache_access_block_salru(salru_data *policy_data, int way, int strm, 
  memory_trace *info);

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

struct cache_block_t cache_get_block_salru(salru_data *policy_data, int way, 
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr);

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
 *  strm        (IN)  - Access stream
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_set_block_salru(salru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);

/*
 *
 * NAME
 *  
 *  CacheGetReplacementRRPV
 *
 * DESCRIPTION
 *  
 *  Obtain RRPV from where to replace the block
 *
 * PARAMETERS
 *  
 *  policy_data (IN) - Cache set  
 *
 * RETURNS
 *  
 *  Replacement RRPV
 */

ub4 cache_get_replacement_stream_salru(salru_data *policy_data, int stream,
  memory_trace *info);

/*
 *
 * NAME
 *  
 *  CacheGetNewRRPV
 *
 * DESCRIPTION
 *  
 *  Obtains new RRPV for aging
 *
 * PARAMETERS
 *  
 *  policy_data (IN) - Cache set
 *  way         (IN) - Accessed way
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_update_recency_salru(salru_data *policy_data, ub4 way_in);

/*
 *
 * NAME
 *  
 *  CacheSetPartition
 *
 * DESCRIPTION
 *  
 *  Set pointer to global partition and allocation array to per set policy
 *
 * PARAMETERS
 *  
 *  policy_data (IN) - Cache set
 *  part        (IN) - Partitions for each stream
 *  alloc       (IN) - Allocation for each stream
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_set_partition(salru_data *policy_data, ub4 *part);

/*
 *
 * NAME
 *  
 *  SetCurrentFillPolicy
 *
 * DESCRIPTION
 *  
 *  Set fill policy to follow in the next fill
 *
 * PARAMETERS
 *  
 *  policy_data (IN) - Cache set
 *  policy      (IN) - Policy to be set
 *
 * RETURNS
 *  
 *  Nothing
 */
void cache_set_current_fill_policy_salru(salru_data *policy_data, cache_policy_t policy);

#endif	/* MEM_SYSTEM_CACHE_H */
