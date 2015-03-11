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

#ifndef MEM_SYSTEM_LIP_H
#define	MEM_SYSTEM_LIP_H

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

#define LIP_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define LIP_DATA_BLOCKS(data)     ((data)->blocks)
#define LIP_DATA_VALID_HEAD(data) ((data)->valid_head)
#define LIP_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define LIP_DATA_FREE_HEAD(data)  ((data)->free_head)
#define LIP_DATA_FREE_TAIL(data)  ((data)->free_tail)

/* LIP specific data */
typedef struct cache_policy_lip_data_t
{
  struct cache_block_t *valid_head; /* Valid list head */
  struct cache_block_t *valid_tail; /* Vail list tail */
  struct cache_block_t *blocks;     /* Actual blocks */
  struct cache_block_t *free_head;  /* Free list head */
  struct cache_block_t *free_tail;  /* Free list tail */
}lip_data;


/*
 *
 * NAME
 *
 *  CacheInitialize - Initialize policy
 *
 * DESCRIPTION
 *
 *  Allocates actual data blocks and the lists for the management policy.
 *
 * PARAMETERS
 *
 *  params      (IN)  - Policy specific parameters
 *  policy_data (OUT) - Cache set to be initialized
 *  
 * RETURNS
 *  
 *  Nothing
 *
 * NOTES
 */

void cache_init_lip(struct cache_params *params, lip_data *policy_data);

/*
 *
 * NAME
 *  
 *  CacheFree - Release policy specific data structures
 *
 * DESCRIPTION
 *  
 *  Deallocates all policy specific data structures
 *
 * PARAMETERS
 *
 *  policy_data (OUT) - Cache set from where data is to be freed
 *
 * RETURNS
 *
 *  Nothing
 *
 */

void cache_free_lip(lip_data *policy_data);


/*
 *
 * NAME
 *  
 *  CacheFindBlock - Find a block
 *
 * DESCRIPTION
 *  
 *  Lookup valid list to find a block
 *
 * PARAMETERS
 *
 *  policy_data (IN)  - Cache set in which block is to be looked
 *  tag         (IN)  - Block tag 
 *
 * RETURNS
 *  
 *  Pointer to the found block
 *
 */

struct cache_block_t * cache_find_block_lip(lip_data *policy_data, long long tag);

/*
 *
 * NAME
 *  
 *  CacheFillBlock - Fill block into the set
 *
 * DESCRIPTION
 *  
 *  Updates block tag, state and stream 
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Set where block is to be filled
 *  way         (IN)  - Way where block is to be filled
 *  tag         (IN)  - Tag to be set
 *  state       (IN)  - Block state to be updated
 *  strm        (IN)  - Access stream 
 *  info        (IN)  - Access info 
 *
 * RETURNS
 *  
 *  Nothing
 *
 */

void cache_fill_block_lip(lip_data *policy_data, int way, long long tag,
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
 *  policy_data (IN)  - Set from where to replace a block
 *
 * RETURNS
 *
 *  Way id from where to replace the block
 *
 */

int  cache_replace_block_lip(lip_data *policy_data);

/*
 *
 * NAME
 *
 *  CacheAccessBlock (IN) - Access a block already in the cache
 *
 * DESCRIPTION
 *
 *  Updates block state on hit 
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Set of the block
 *  way         (IN)  - Way of the block
 *  strm        (IN)  - Access stream
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_access_block_lip(lip_data *policy_data, int way, int strm, 
  memory_trace *info);

/*
 *
 * NAME
 *
 *  CacheGetBlock - Get the block data 
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

struct cache_block_t cache_get_block_lip(lip_data *policy_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *stream_ptr);

/*
 *
 * NAME
 *  
 *  CacheSetBlock - Cache set block state
 *
 * DESCRIPTION
 *  
 *  Updates state of the cache block 
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

void cache_set_block_lip(lip_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
