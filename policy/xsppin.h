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

#ifndef MEM_SYSTEM_XSPPIN_H
#define	MEM_SYSTEM_XSPPIN_H

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "policy.h"
#include "rrip.h"
#include "cache-param.h"
#include "cache-block.h"
#include "stream-region.h"

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_xsppin_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}xsppin_list;

#define XSPPIN_DATA_CFPOLICY(data)   ((data)->current_fill_policy)
#define XSPPIN_DATA_DFPOLICY(data)   ((data)->default_fill_policy)
#define XSPPIN_DATA_CAPOLICY(data)   ((data)->current_access_policy)
#define XSPPIN_DATA_DAPOLICY(data)   ((data)->default_access_policy)
#define XSPPIN_DATA_CRPOLICY(data)   ((data)->current_replacement_policy)
#define XSPPIN_DATA_DRPOLICY(data)   ((data)->default_replacement_policy)
#define XSPPIN_DATA_SET_INDEX(data)  ((data)->set_idx)
#define XSPPIN_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define XSPPIN_DATA_WAYS(data)       ((data)->ways)
#define XSPPIN_DATA_BLOCKS(data)     ((data)->blocks)
#define XSPPIN_DATA_VALID_HEAD(data) ((data)->valid_head)
#define XSPPIN_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define XSPPIN_DATA_FREE_HLST(data)  ((data)->free_head)
#define XSPPIN_DATA_FREE_TLST(data)  ((data)->free_tail)
#define XSPPIN_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define XSPPIN_DATA_FREE_TAIL(data)  ((data)->free_tail->head)

/* RRIP specific data */
typedef struct cache_policy_xsppin_data_t
{
  cache_policy_t current_fill_policy;         /* If non-default fill policy is enforced */
  cache_policy_t default_fill_policy;         /* Default fill policy */
  cache_policy_t current_access_policy;       /* If non-default fill policy is enforced */
  cache_policy_t default_access_policy;       /* Default fill policy */
  cache_policy_t current_replacement_policy;  /* If non-default fill policy is enforced */
  cache_policy_t default_replacement_policy;  /* Default fill policy */
  ub4            set_idx;                     /* Set index */
  ub4            max_rrpv;                    /* Maximum RRPV supported */
  ub4            ways;                        /* Number of ways */
  rrip_list     *valid_head;                  /* Head pointers of RRPV specific list */
  rrip_list     *valid_tail;                  /* Tail pointers of RRPV specific list */
  list_head_t   *free_head;                   /* Free list head */
  list_head_t   *free_tail;                   /* Free list tail */
  ub4            threshold;                   /* Threshold for bimodal policy */
  ub4            current_ctr;                 /* Threshold for bimodal policy */
  ub8            fills[TST];                  /* Per-stream fills */
  ub8            max_fills;                   /* Per-stream maximum fills */
  ub8            min_fills;                   /* Per-stream minimum fills */
  struct cache_block_t *blocks;               /* Actual blocks */
}xsppin_data;

/* XSPPIN cache wide data */
typedef struct cache_policy_xsppin_gdata_t
{
  ub4 bm_ctr;                                 /* Current value of a counter */
  ub4 bm_thr;                                 /* Bimodal threshold */
  ub8 access_count;                           /* Total accesses */
  ub8 access_seq[TST][2];                     /* Accesses between reuse */
  ub8 per_stream_fill[TST];                   /* Fills for each stream */
  ub8 per_stream_hit[TST];                    /* Hits for each stream */
  ub8 *per_stream_frrpv[TST];                 /* #Per-stream fill for each RRPV */
  ub8 reuse_distance[TST];                    /* Reuse distance for each stream */
  ub8 ct_predicted;                           /* # predicted CT */
  ub8 bt_predicted;                           /* # predicted BT */
  ub8 zt_predicted;                           /* # predicted ZT */
  ub8 ct_evict;                               /* # evicted CT without use */
  ub8 bt_evict;                               /* # evicted BT without use */
  ub8 zt_evict;                               /* # evicted ZT without use */
  ub8 ct_correct;                             /* # CT used by texture */
  ub8 bt_correct;                             /* # BT used by texture */
  ub8 zt_correct;                             /* # ZT used by texture */
  ub8 ct_used;                                /* # used CT */
  ub8 bt_used;                                /* # used BT */
  ub8 zt_used;                                /* # used ZT */
  ub8 ct_use_possible;                        /* # used CT */
  ub8 bt_use_possible;                        /* # used CT */
  ub8 zt_use_possible;                        /* # used CT */
  ub8 ct_block_count;                         /* #CT blocks */ 
  ub8 bt_block_count;                         /* #BT blocks */ 
  ub8 zt_block_count;                         /* #ZT blocks */ 
  struct cs_qnode *ct_blocks;                 /* Hash table of all CT blocks */ 
  struct cs_qnode *bt_blocks;                 /* Hash table of all BT blocks */ 
  struct cs_qnode *zt_blocks;                 /* Hash table of all ZT blocks */ 
}xsppin_gdata;

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

void cache_init_xsppin(int set, struct cache_params *params, xsppin_data *policy_data,
    xsppin_gdata *global_data);

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

void cache_free_xsppin(xsppin_data *policy_data);

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
 *  global_data (IN)  - Cache-wide policy data 
 *  tag         (IN)  - Block tag
 *
 * RETURNS
 *  
 *  Pointer to the block 
 */

struct cache_block_t* cache_find_block_xsppin(xsppin_data *policy_data, 
    xsppin_gdata *global_data, long long tag, memory_trace *info);

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
 *  global_data (IN)  - Cache-wide policy data
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

void cache_fill_block_xsppin(xsppin_data *policy_data, 
    xsppin_gdata *global_data, int way, long long tag, 
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

int  cache_replace_block_xsppin(xsppin_data *policy_data, xsppin_gdata *global_data);

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

void cache_access_block_xsppin(xsppin_data *policy_data, xsppin_gdata *global_data, 
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

struct cache_block_t cache_get_block_xsppin(xsppin_data *policy_data, int way, 
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

void cache_set_block_xsppin(xsppin_data *policy_data, int way, long long tag,
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
 *
 * RETURNS
 *  
 *  Fill RRPV
 *
 */

int cache_get_fill_rrpv_xsppin(xsppin_data *policy_data, 
    xsppin_gdata *global_data, memory_trace *info, ub1 *is_x_block);

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

int cache_get_replacement_rrpv_xsppin(xsppin_data *policy_data);

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

int cache_get_new_rrpv_xsppin(xsppin_data *policy_data, xsppin_gdata *global_data,
    int old_rrpv, memory_trace *info);

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

int cache_count_block_xsppin(xsppin_data *policy_data, ub1 strm);

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

void cache_set_current_fill_policy_xsppin(xsppin_data *policy_data, cache_policy_t policy);

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

void cache_set_current_access_policy_xsppin(xsppin_data *policy_data, cache_policy_t policy);

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

void cache_set_current_replacement_policy_xsppin(xsppin_data *policy_data, cache_policy_t policy);

#endif	/* MEM_SYSTEM_CACHE_H */
