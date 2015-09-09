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

#ifndef MEM_SYSTEM_BRRIP_H
#define	MEM_SYSTEM_BRRIP_H

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "policy.h"
#include "rrip.h"
#include "cache-param.h"
#include "cache-block.h"

#define MIN_EPOCH     (0)
#define MAX_EPOCH     (3)
#define EPOCH_COUNT   (MAX_EPOCH - MIN_EPOCH + 1)

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_brrip_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}brrip_list;

#define BRRIP_DATA_THRESHOLD(data)  ((data)->threshold)
#define BRRIP_DATA_CFPOLICY(data)   ((data)->current_fill_policy)
#define BRRIP_DATA_DFPOLICY(data)   ((data)->default_fill_policy)
#define BRRIP_DATA_CAPOLICY(data)   ((data)->current_access_policy)
#define BRRIP_DATA_DAPOLICY(data)   ((data)->default_access_policy)
#define BRRIP_DATA_CRPOLICY(data)   ((data)->current_replacement_policy)
#define BRRIP_DATA_DRPOLICY(data)   ((data)->default_replacement_policy)
#define BRRIP_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define BRRIP_DATA_SPILL_RRPV(data) ((data)->spill_rrpv)
#define BRRIP_DATA_BLOCKS(data)     ((data)->blocks)
#define BRRIP_DATA_VALID_HEAD(data) ((data)->valid_head)
#define BRRIP_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define BRRIP_DATA_FREE_HLST(data)  ((data)->free_head)
#define BRRIP_DATA_FREE_TLST(data)  ((data)->free_tail)
#define BRRIP_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define BRRIP_DATA_FREE_TAIL(data)  ((data)->free_tail->head)
#define BRRIP_DATA_USE_EPOCH(data)  ((data)->use_epoch)

/* RRIP specific data */
typedef struct cache_policy_brrip_data_t
{
  ub4             threshold;                  /* Bimodal threshold */     
  cache_policy_t  current_fill_policy;        /* If non-default fill policy is enforced */
  cache_policy_t  default_fill_policy;        /* Default fill policy */
  cache_policy_t  current_access_policy;      /* If non-default access policy is enforced */
  cache_policy_t  default_access_policy;      /* Default access policy */
  cache_policy_t  current_replacement_policy; /* If non-default replacement policy is enforced */
  cache_policy_t  default_replacement_policy; /* Default replacement policy */
  ub4             max_rrpv;                   /* Maximum RRPV supported */
  ub4             spill_rrpv;                 /* Spill RRPV supported */
  rrip_list      *valid_head;                 /* Head pointers of RRPV specific list */
  rrip_list      *valid_tail;                 /* Tail pointers of RRPV specific list */
  list_head_t    *free_head;                  /* Free list head */
  list_head_t    *free_tail;                  /* Free list tail */
  ub1             use_epoch;                  /* TRUE, if epoch are used in policy */
  struct cache_block_t *blocks;               /* Actual blocks */
}brrip_data;

/* BRRIP global data */
typedef struct cache_policy_brrip_gdata_t
{
  ub4  threshold;     /* Bimodal threshold */
  ub1  epoch_count;   /* Total supported epochs */
  ub1  max_epoch;     /* Maximum supported epochs */
  sctr **epoch_fctr;  /* Per-stream epoch fill counter */
  sctr **epoch_hctr;  /* Per-stream epoch hit counter */
  ub1  *epoch_valid;  /* Valid epoch list */
  sctr access_ctr;    /* Current BRRIP access */
}brrip_gdata;

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
 *  global_data (IN)  - Cache wide data
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTES
 */

void cache_init_brrip(struct cache_params *params, brrip_data *policy_data,
  brrip_gdata *global_data);

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

void cache_free_brrip(brrip_data *policy_data);

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
struct cache_block_t * cache_find_block_brrip(brrip_data *policy_data, long long tag);

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
 *  global_data (IN)  - Cache wide data
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

void cache_fill_block_brrip(brrip_data *policy_data, brrip_gdata *global_data,
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

int  cache_replace_block_brrip(brrip_data *policy_data);

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
 *  global_data (IN)  - Cache-wide data
 *  way         (IN)  - Physical way of the block
 *  strm        (IN)  - Stream accessing the block
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 *
 */

void cache_access_block_brrip(brrip_data *policy_data, brrip_gdata *global_data,
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

struct cache_block_t cache_get_block_brrip(brrip_data *policy_data, int way, 
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream);

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

void cache_set_block_brrip(brrip_data *policy_data, int way, long long tag, 
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
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Fill RRPV
 *
 */

int cache_get_fill_rrpv_brrip(brrip_data *policy_data, 
    brrip_gdata *global_data, memory_trace *info, ub4 epoch);

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

int cache_get_replacement_rrpv_brrip(brrip_data *policy_data);

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
 *  policy_data (IN) - Cache set data
 *  global_data (IN) - Cache-wide data
 *  info        (IN) - Access info
 *  old_rrpv    (IN) - Current block RRPV
 *  epoch       (IN) - Current block epoch
 *
 * RETURNS
 *  
 *  New RRPV
 */

int cache_get_new_rrpv_brrip(brrip_data *policy_data, brrip_gdata *global_data, 
    memory_trace *info, sb4 old_rrpv, ub4 epoch);

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

void cache_set_current_fill_policy_brrip(brrip_data *policy_data, cache_policy_t policy);

/*
 *
 * NAME
 *  
 *  SetCurrentAccessPolicy
 *
 * DESCRIPTION
 *  
 *  Sets policy to follow for the next access
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

void cache_set_current_access_policy_brrip(brrip_data *policy_data, cache_policy_t policy);

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

void cache_set_current_replacement_policy_brrip(brrip_data *policy_data, cache_policy_t policy);

#endif	/* MEM_SYSTEM_CACHE_H */
