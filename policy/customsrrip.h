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

#ifndef MEM_SYSTEM_CUSTOMSRRIP_H
#define	MEM_SYSTEM_CUSTOMSRRIP_H

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "policy.h"
#include "rrip.h"
#include "cache-param.h"
#include "cache-block.h"

#define MIN_EPOCH     (0)
#define MAX_EPOCH     (10)
#define EPOCH_COUNT   (MAX_EPOCH - MIN_EPOCH + 1)

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_customsrrip_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}customsrrip_list;

#define CUSTOMSRRIP_DATA_CFPOLICY(data)   ((data)->current_fill_policy)
#define CUSTOMSRRIP_DATA_DFPOLICY(data)   ((data)->default_fill_policy)
#define CUSTOMSRRIP_DATA_CAPOLICY(data)   ((data)->current_access_policy)
#define CUSTOMSRRIP_DATA_DAPOLICY(data)   ((data)->default_access_policy)
#define CUSTOMSRRIP_DATA_CRPOLICY(data)   ((data)->current_replacement_policy)
#define CUSTOMSRRIP_DATA_DRPOLICY(data)   ((data)->default_replacement_policy)
#define CUSTOMSRRIP_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define CUSTOMSRRIP_DATA_SPILL_RRPV(data) ((data)->spill_rrpv)
#define CUSTOMSRRIP_DATA_BLOCKS(data)     ((data)->blocks)
#define CUSTOMSRRIP_DATA_VALID_HEAD(data) ((data)->valid_head)
#define CUSTOMSRRIP_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define CUSTOMSRRIP_DATA_FREE_HLST(data)  ((data)->free_head)
#define CUSTOMSRRIP_DATA_FREE_TLST(data)  ((data)->free_tail)
#define CUSTOMSRRIP_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define CUSTOMSRRIP_DATA_FREE_TAIL(data)  ((data)->free_tail->head)
#define CUSTOMSRRIP_DATA_USE_EPOCH(data)  ((data)->use_epoch)

/* RRIP specific data */
typedef struct cache_policy_customsrrip_data_t
{
  cache_policy_t current_fill_policy;         /* If non-default fill policy is enforced */
  cache_policy_t default_fill_policy;         /* Default fill policy */
  cache_policy_t current_access_policy;       /* If non-default fill policy is enforced */
  cache_policy_t default_access_policy;       /* Default fill policy */
  cache_policy_t current_replacement_policy;  /* If non-default fill policy is enforced */
  cache_policy_t default_replacement_policy;  /* Default fill policy */
  ub4            max_rrpv;                    /* Maximum RRPV supported */
  ub4            spill_rrpv;                  /* Spill RRPV supported */
  rrip_list     *valid_head;                  /* Head pointers of RRPV specific list */
  rrip_list     *valid_tail;                  /* Tail pointers of RRPV specific list */
  list_head_t   *free_head;                   /* Free list head */
  list_head_t   *free_tail;                   /* Free list tail */
  ub1            use_epoch;                   /* TRUE, if epoch is used in the policy */
  struct cache_block_t *blocks;               /* Actual blocks */
}customsrrip_data;

/* CUSTOMSRRIP cache-wide data */
typedef struct cache_policy_customsrrip_gdata_t
{
  ub1    epoch_count;               /* Total supported epochs */
  ub1    max_epoch;                 /* Maximum supported epochs */
  ub4    max_rrpv;                  /* Maximum RRPV supported */
  ub1    use_ct_hint;               /* TRUE if ct hint is to be used */
  sctr **epoch_fctr;                /* Epoch-wise fill counter */
  sctr **epoch_hctr;                /* Epoch-wise hit counter */
  sctr  *epoch_ctctr;               /* Inter-stream hit counter */
  sctr  *epoch_btctr;               /* Inter-stream hit counter */
  ub1   *epoch_valid;               /* Valid epoch list */
  ub4    fill_rrpv[TST + 1];        /* Fill RRPV for all streams */
  ub8    thrasher_fdemote[TST + 1]; /* # Thrasher stream is demoted */
  ub8    thrasher_hdemote[TST + 1]; /* # Thrasher stream is demoted */
}customsrrip_gdata;

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
 *  global_data (IN)  - Cache-wide data 
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTES
 */

void cache_init_customsrrip(ub4 set_indx, struct cache_params *params, 
    customsrrip_data *policy_data, customsrrip_gdata *global_data);

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

void cache_free_customsrrip(customsrrip_data *policy_data);

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

struct cache_block_t * cache_find_block_customsrrip(customsrrip_data *policy_data, long long tag);

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

void cache_fill_block_customsrrip(customsrrip_data *policy_data, customsrrip_gdata *global_data,
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
 *  global_data (IN)  - Cache-wide data
 *
 * RETURNS
 *  
 *  Way id from where to replace the block
 *
 */

int  cache_replace_block_customsrrip(customsrrip_data *policy_data, 
    customsrrip_gdata *global_data, ub1 last_stream);

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
 *  strm        (IN)  - Access stream
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 *
 */

void cache_access_block_customsrrip(customsrrip_data *policy_data, 
    customsrrip_gdata *global_data, int way, int strm, memory_trace *info);

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

struct cache_block_t cache_get_block_customsrrip(customsrrip_data *policy_data, int way, 
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

void cache_set_block_customsrrip(customsrrip_data *policy_data, int way, long long tag,
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

int cache_get_fill_rrpv_customsrrip(customsrrip_data *policy_data, 
    customsrrip_gdata *global_data, memory_trace *info, ub4 epoch);

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

int cache_get_replacement_rrpv_customsrrip(customsrrip_data *policy_data);

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
 *  policy_data (IN) - Per-set data
 *  global_data (IN) - Cache-wide data
 *  info        (IN) - Access info
 *  epoch       (IN) - Current block epoch
 *
 * RETURNS
 *  
 *  New RRPV
 *
 */

int cache_get_new_rrpv_customsrrip(customsrrip_data *policy_data, customsrrip_gdata *global_data, 
    memory_trace *info, ub4 epoch, ub4 old_rrpv);

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

int cache_count_block_customsrrip(customsrrip_data *policy_data, ub1 strm);

/*
 *
 * NAME
 *  
 *  SetStreamFillRRPV
 *
 * DESCRIPTION
 *  
 *  Sets fill RRPV for given stream
 *
 * PARAMETERS
 *  
 *  global_data (IN) - Cache-wide data 
 *  stream      (IN) - stream
 *  rrpv        (IN) - rrpv to set
 *
 * RETURNS
 *  
 *  Nothing
 */

/* Set policy for next fill */
void cache_set_stream_fill_rrpv_customsrrip(customsrrip_gdata *global_data, 
    ub1 stream, ub4 rrpv);

#endif	/* MEM_SYSTEM_CACHE_H */
