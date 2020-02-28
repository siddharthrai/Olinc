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

#ifndef MEM_SYSTEM_SDP_H
#define	MEM_SYSTEM_SDP_H

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
#include "srrip.h"
#include "brrip.h"

/* Core identifiers */
#define CS1 (0)
#define CS2 (1)
#define CS3 (2)
#define CS4 (3)
#define CS5 (4)

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_sdp_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}sdp_list;

#define SDP_DATA_FOLLOWING(data)  (data->following)
#define SDP_DATA_STYPE(data)      (data->following)
#define SDP_DATA_BLOCKS(data)     (SAP_DATA_BLOCKS(&(data->sap)))
#define SDP_DATA_VALID_HEAD(data) (SAP_DATA_VALID_HEAD(&(data->sap)))

/* SDP specific data */
typedef struct cache_policy_sdp_data_t
{
  cache_policy_t  following;        /* Policy followed by the set */
  ub1             sdp_sample_type;  /* SDP sample type (BS / PS / CS) */
  sap_data        sap;              /* SAP specific data */
}sdp_data;
  
/* Returns number of total cores known to SDP */
#define SDP_GDATA_CORES(data) (SAP_GDATA_CORES(&(data->sap)))

/* Policy global data */
typedef struct cache_policy_sdp_gdata_t
{
  ub4       sdp_samples;    /* Samples for corrective path */
  ub4       sdp_cpu_cores;  /* Cores in the system */
  ub4       sdp_gpu_cores;  /* Cores in the system */
  ub1       sdp_cpu_tpset;  /* Threshold for P set */
  ub1       sdp_cpu_tqset;  /* Threshold for Q set */
  ub8       next_schedule;  /* Cycle at which to schedule periodic events */
  sap_gdata sap;            /* Global data for component policy */
}sdp_gdata;

/* Policy global data */
typedef struct cache_policy_sdp_stats_t
{
  sap_stats sap;            /* SAP stats */
  ub8       next_schedule;  /* Cycle at which to schedule periodic events */
}sdp_stats;

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

void cache_init_sdp(long long int set_indx, struct cache_params *params, 
  sdp_data *policy_data, sdp_gdata *global_data);

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

void cache_free_sdp(long long int set_indx, sdp_data *policy_data, sdp_gdata *global_data);

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
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Pointer to the block 
 */

struct cache_block_t * cache_find_block_sdp(sdp_data *policy_data, long long tag,
  memory_trace *info);

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

void cache_fill_block_sdp(sdp_data *policy_data, sdp_gdata *global_data, 
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

int  cache_replace_block_sdp(sdp_data *policy_data, sdp_gdata *global_data, memory_trace *info);

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

void cache_access_block_sdp(sdp_data *policy_data, sdp_gdata *global_data,
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

struct cache_block_t cache_get_block_sdp(sdp_data *policy_data, sdp_gdata *global_data, 
  int way, long long *tag_ptr, enum cache_block_state_t *state_ptr, 
  int *stream_ptr);

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

void cache_set_block_sdp(sdp_data *policy_data, sdp_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_SDP_H */
