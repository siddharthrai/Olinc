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

#ifndef MEM_SYSTEM_SHIP_H
#define	MEM_SYSTEM_SHIP_H

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "policy.h"
#include "rrip.h"
#include "cache-param.h"
#include "cache-block.h"

#define USE_NN    (0)  
#define USE_PC    (1)  
#define USE_MEM   (2)  
#define USE_MEMPC (3)  

#define SHIP_DATA_CFPOLICY(data)   ((data)->current_fill_policy)
#define SHIP_DATA_DFPOLICY(data)   ((data)->default_fill_policy)
#define SHIP_DATA_CAPOLICY(data)   ((data)->current_access_policy)
#define SHIP_DATA_DAPOLICY(data)   ((data)->default_access_policy)
#define SHIP_DATA_CRPOLICY(data)   ((data)->current_replacement_policy)
#define SHIP_DATA_DRPOLICY(data)   ((data)->default_replacement_policy)
#define SHIP_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define SHIP_DATA_SPILL_RRPV(data) ((data)->spill_rrpv)
#define SHIP_DATA_BLOCKS(data)     ((data)->blocks)
#define SHIP_DATA_VALID_HEAD(data) ((data)->valid_head)
#define SHIP_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define SHIP_DATA_FREE_HLST(data)  ((data)->free_head)
#define SHIP_DATA_FREE_TLST(data)  ((data)->free_tail)
#define SHIP_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define SHIP_DATA_FREE_TAIL(data)  ((data)->free_tail->head)

/* RRIP specific data */
typedef struct cache_policy_ship_data_t
{
  cache_policy_t current_fill_policy;         /* If non-default fill policy is enforced */
  cache_policy_t default_fill_policy;         /* Default fill policy */
  cache_policy_t current_access_policy;       /* If non-default fill policy is enforced */
  cache_policy_t default_access_policy;       /* Default fill policy */
  cache_policy_t current_replacement_policy;  /* If non-default fill policy is enforced */
  cache_policy_t default_replacement_policy;  /* Default fill policy */
  ub4            max_rrpv;                    /* Maximum RRPV supported */
  ub4            spill_rrpv;                  /* Spill RRPV */
  rrip_list     *valid_head;                  /* Head pointers of RRPV specific list */
  rrip_list     *valid_tail;                  /* Tail pointers of RRPV specific list */
  list_head_t   *free_head;                   /* Free list head */
  list_head_t   *free_tail;                   /* Free list tail */

  struct cache_block_t *blocks;               /* Actual blocks */
}ship_data;

/* SHiP-mem cache-wide data */
typedef struct cache_policy_ship_gdata_t
{
  ub1   threshold;  /* BRRIP threshold */
  ub4   shct_size;  /* Signature history counter table size in log of entries */
  ub4   sign_size;  /* Signature size in bits */
  ub4   core_size;  /* Core count */
  ub4   entry_size; /* Counter size */
  sctr *shct;       /* Signature history counter table */
  ub1   sign_source;/* Source of sign (PC / MEM / PC + MEM) */
  ub1  *brrip_ctr;  /* BRRIP counter */
}ship_gdata;

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

void cache_init_ship(ub4 set_idx, struct cache_params *params, 
    ship_data *policy_data, ship_gdata *global_data);

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

void cache_free_ship(ship_data *policy_data);

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

struct cache_block_t * cache_find_block_ship(ship_data *policy_data, long long tag);

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
 *  global_data (IN)  - Cache-wide data
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

void cache_fill_block_ship(ship_data *policy_data, 
    ship_gdata *global_data, int way, long long tag,
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
 *  global_data (IN)  - Cache-wide data
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Way id from where to replace the block
 *
 */

int  cache_replace_block_ship(ship_data *policy_data, 
    ship_gdata *global_data, memory_trace *info);

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
 *  Old cache block data
 *
 */

struct cache_block_t cache_access_block_ship(ship_data *policy_data, 
    ship_gdata *global_data, int way, int strm, memory_trace *info);

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

struct cache_block_t cache_get_block_ship(ship_data *policy_data, int way, 
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
 *  stream      (IN)  - Access stream
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_set_block_ship(ship_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);

/*
 *
 * NAME
 *
 *  CacheGetFillRRPV 
 *
 * DESCRIPTION
 *  
 *  Obtains fill RRPV  
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Cache set 
 *  global_data (IN)  - Cache-wide data 
 *
 * RETURNS
 *  
 *  Fill RRPV
 *
 */

int cache_get_fill_rrpv_ship(ship_data *policy_data, 
    ship_gdata *global_data, memory_trace *info);

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

int cache_get_replacement_rrpv_ship(ship_data *policy_data);

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
 *  old_rrpv (IN) - Current RRPV of the block
 *
 * RETURNS
 *  
 *  New RRPV
 */

int cache_get_new_rrpv_ship(memory_trace *info, int old_rrpv);

/*
 *
 * NAME
 *  
 *  CountBlocks
 *
 * DESCRIPTION
 *  
 *  Count blocks
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Policy specific data
 *  strm        (IN)  - Accessing stream
 *
 * RETURNS
 *  
 *  Block count
 */

int cache_count_block_ship(ship_data *policy_data, ub1 strm);

/*
 *
 * NAME
 *  
 *  SetCurrentFillPolicy
 *
 * DESCRIPTION
 *  
 *  Sets policy to follow for the next fill
 *
 * PARAMETERS
 *  
 *  policy_data (IN) - Policy specific data
 *  policy      (IN) - Policy to be set 
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_set_current_fill_policy_ship(ship_data *policy_data, cache_policy_t policy);

/*
 *
 * NAME
 *  
 *  SetCurrentAccessPolicy
 *
 * DESCRIPTION
 *  
 *  Sets policy to follow for the next fill
 *
 * PARAMETERS
 *  
 *  policy_data (IN) - Policy specific data
 *  policy      (IN) - Policy to be set 
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_set_current_access_policy_ship(ship_data *policy_data, cache_policy_t policy);

/*
 *
 * NAME
 *  
 *  SetCurrentReplacementPolicy
 *
 * DESCRIPTION
 *  
 *  Sets policy to follow for the next fill
 *
 * PARAMETERS
 *  
 *  policy_data (IN) - Policy specific data
 *  policy      (IN) - Policy to be set 
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_set_current_replacement_policy_ship(ship_data *policy_data, cache_policy_t policy);

#endif	/* MEM_SYSTEM_CACHE_H */
