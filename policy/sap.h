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

#ifndef MEM_SYSTEM_SAP_H
#define	MEM_SYSTEM_SAP_H

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
#include "stdio.h"

/* For the time being only 5 cores are supported. */
#define MAX_SAP_CORES (5)

/* Streams specific to SAP. SAP controller remaps global stream id to SAP 
 * specific ids */
typedef enum sap_stream
{
  sap_stream_x = 1,
  sap_stream_y,
  sap_stream_p,
  sap_stream_q,
  sap_stream_r
}sap_stream;

/* Policy statistics */
typedef struct cache_policy_sap_stats_t
{
  FILE *stat_file;
  ub8   sap_x_fill;       /* X set fill */
  ub8   sap_y_fill;       /* Y set fill */
  ub8   sap_p_fill;       /* P set fill */
  ub8   sap_q_fill;       /* Q set fill */
  ub8   sap_r_fill;       /* R set fill */
  ub8   sap_x_evct;       /* X set blocks evicted */
  ub8   sap_y_evct;       /* Y set blocks evicted */
  ub8   sap_p_evct;       /* P set blocks evicted */
  ub8   sap_q_evct;       /* Q set blocks evicted */
  ub8   sap_r_evct;       /* R set blocks evicted */
  ub8   sap_x_access;     /* X set access */
  ub8   sap_y_access;     /* Y set access */
  ub8   sap_p_access;     /* P set access */
  ub8   sap_q_access;     /* Q set access */
  ub8   sap_r_access;     /* R set access */
  ub8   sap_psetrecat;    /* R set access */
  ub8   next_schedule;    /* Next cycle for dumping stats */
  ub8   schedule_period;  /* Scheduling period */
  ub1   hdr_printed;      /* Set if header is written to file */
}sap_stats;

/* Policy specific data */
typedef struct sap_pdata
{
  sap_stream stream;
}sap_pdata;

#define SAP_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define SAP_DATA_BLOCKS(data)     ((data)->blocks)
#define SAP_DATA_VALID_HEAD(data) ((data)->valid_head)
#define SAP_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define SAP_DATA_FREE_HLST(data)  ((data)->free_head)
#define SAP_DATA_FREE_TLST(data)  ((data)->free_tail)
#define SAP_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define SAP_DATA_FREE_TAIL(data)  ((data)->free_tail->head)
#define SAP_DATA_PSPOLICY(data)   ((data)->per_stream_policy)

/* SAP specific data */
typedef struct cache_policy_sap_data_t
{
  /* SAP specific data */
  ub4         max_rrpv;               /* Maximum RRPV supported */
  rrip_list *valid_head;              /* Head pointers of RRPV specific list */
  rrip_list *valid_tail;              /* Tail pointers of RRPV specific list */

  list_head_t *free_head;             /* Free list head */
  list_head_t *free_tail;             /* Free list tail */

  struct cache_block_t *blocks;       /* Actual blocks */
  
  srrip_data  srrip;                  /* SRRIP specific data */
  cache_policy_t *per_stream_policy;  /* Policy followed by each stream */
}sap_data;

#define SAP_GDATA_TCORES(d) ((d)->sap_cpu_cores  + (d)->sap_gpu_cores )
#define SAP_GDATA_CORES(d)  (((d)->sap_gpu_cores) ? SAP_GDATA_TCORES(d) : (d)->sap_cpu_cores + 1)

/* Policy global data */
typedef struct cache_policy_sap_gdata_t
{
  ub4 sap_cpu_cores;      /* CPU cores known to SAP */
  ub4 sap_gpu_cores;      /* GPU cores known to SAP */
  ub4 sap_streams;        /* Number of streams known to SAP */
  ub4 policy_set;         /* # sets for which policy is set */
  ub1 sap_cpu_tpset;      /* CPU P set threshold */
  ub1 sap_cpu_tqset;      /* CPU Q set threshold */
  ub8 sap_y_access;       /* SAP Y stream access */
  ub8 sap_y_miss;         /* SAP Y stream miss */
  ub8 activate_access;    /* LLC access to activate stream classification */ 
  sap_stats stats;        /* SAP statistics */
  ub8 threshold;          /* Bimodal threshold */
  ub1 sdp_psetinstp;      /* If set, promote pset blocks to RRPV 0 on hit */
  ub1 sdp_greplace;       /* If set, choose min wayid block as victim */
  ub1 sdp_psetbmi;        /* If set, use bimodal insertion for P set */
  ub1 sdp_psetrecat;      /* If set, block is recategorized on hit */
  ub1 sdp_psetbse;        /* If set, only baseline samples are used for P set */
  ub1 sdp_psetmrt;        /* If set, use miss rate threshold for P set */
  ub1 sdp_psethrt;        /* If set, use hit rate threshold for P set */
  sctr access_ctr;        /* Access counter */
}sap_gdata;

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
 *  set_indx    (IN)  - Set index
 *  params      (IN)  - Policy specific parameters
 *  policy_data (OUT) - Cache per-set policy data
 *  global_data (OUT) - Cache wide policy data
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTES
 */

void cache_init_sap(long long int set_indx, struct cache_params *params, 
  sap_data *policy_data, sap_gdata *global_data);

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
 *  set_indx    (IN)  - Set index
 *  policy_data (IN)  - Cache per-set policy data
 *  global_data (IN)  - Cache wide policy data
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_free_sap(long long int set_indx, sap_data *policy_data, sap_gdata *global_data);

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
 *  policy_data (IN)  - Cache per-set policy data
 *  tag         (IN)  - Block tag
 *
 * RETURNS
 *  
 *  Pointer to the block 
 */

struct cache_block_t * cache_find_block_sap(sap_data *policy_data, long long tag);

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
 *  policy_data (IN)  - Per-set policy data
 *  global_data (IN)  - Cache wide policy data
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

void cache_fill_block_sap(sap_data *policy_data, sap_gdata *global_data, int way, long long tag, 
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
 *  policy_data (IN)  - Per-set policy data
 *  global_data (IN)  - Cache wide policy data
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Way id from where to replace the block
 *
 */

int cache_replace_block_sap(sap_data *policy_data, sap_gdata *global_data, 
  memory_trace *info);

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
 *  policy_data (IN)  - Per-set policy data
 *  global_data (IN)  - Cache wide policy data
 *  way         (IN)  - Physical way of the block
 *  strm        (IN)  - Stream accessing the block
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 *
 */

void cache_access_block_sap(sap_data *policy_data, sap_gdata *global_data, 
  int way, int stream, memory_trace *info);

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
 *  global_data (IN)  - Cache wide policy data
 *  way         (IN)  - Block way
 *  tag_ptr     (OUT) - Block tag 
 *  state_ptr   (OUT) - Block state
 *  stream_ptr  (OUT) - Block stream
 *
 * RETURNS
 *  
 *  Complete block info
 */

struct cache_block_t cache_get_block_sap(sap_data *policy_data, sap_gdata *global_data, 
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
 *  policy_data (IN)  - Per-set policy data
 *  global_data (IN)  - Cache-wide policy data
 *  way         (IN)  - Way of the block
 *  tag         (IN)  - Block tag
 *  state       (IN)  - New state
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_set_block_sap(sap_data *policy_data, sap_gdata *global_data,
  int way, long long tag, enum cache_block_state_t state, ub1 stream,
  memory_trace *info);

/*
 *
 * NAME
 *
 *  SetPerStreamPolicy - Set policy for a stream
 *
 * DESCRIPTION
 *  
 *  Update state of the block
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Set of the block
 *  global_data (IN)  - Cache wide policy data
 *  way         (IN)  - Way of the block
 *  tag         (IN)  - Block tag
 *  state       (IN)  - New state
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 */

void set_per_stream_policy_sap(sap_data *policy_data, sap_gdata *global_data, 
  ub4 core, ub1 stream, cache_policy_t policy);

/*
 *
 * NAME
 *
 *  GetSapStream - Obtain SAP specific stream 
 *
 * DESCRIPTION
 *  
 *  Obtains SAP specific stream (X, Y, P, Q, R) based on categorization 
 *  parameters.
 *
 * PARAMETERS
 *  
 *  global_data (IN)  - Cache wide policy data
 *  stream_in   (IN)  - Accessing stream
 *  pid_in      (IN)  - Processing element id
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  SAP specific stream
 */

sap_stream get_sap_stream(sap_gdata *global_data, ub1 stream_in, ub1 pid_in,
  memory_trace *info);

/*
 *
 * NAME
 *
 *  InitSapStats - Initialize SAP specific statistics
 *
 * DESCRIPTION
 *  
 *  Initialize SAP statistics and open stats file
 *
 * PARAMETERS
 *  
 *  stats     (IN)  - SAP statistics
 *  file_name (IN)  - Statistics file
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_init_sap_stats(sap_stats *stats, char *file_name);

/*
 *
 * NAME
 *
 *  FinalizeSapStats - Finalize SAP statistics
 *
 * DESCRIPTION
 *  
 *  Clean up SAP stream an close statistics file
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SAP statistics
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_fini_sap_stats(sap_stats *stats);

/*
 *
 * NAME
 *
 *  UpdateSapCacheAccessStats - Update stats on cache access
 *
 * DESCRIPTION
 *  
 *  Update stats on cache access
 *
 * PARAMETERS
 *  
 *  global_data (IN)  - Cache wide policy stats
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_update_sap_access_stats(sap_gdata *global_data, memory_trace *info, ub1 psetrecat);

/*
 *
 * NAME
 *
 *  UpdateSapFillStats - Update stats on fill
 *
 * DESCRIPTION
 *  
 *  Update stats on fill
 *
 * PARAMETERS
 *  
 *  global_data (IN) - Cache wide policy data
 *  sstream     (IN) - SAP specific stream
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTE
 *  
 *  As SAP specific stream is decided at the time of fill.
 */

void cache_update_sap_fill_stats(sap_gdata *global_data, sap_stream sstream);

/*
 *
 * NAME
 *
 *  UpdateSapReplacementStats - Update stats on replacement
 *
 * DESCRIPTION
 *  
 *  Update stats on replacement
 *
 * PARAMETERS
 *  
 *  global_data (IN) - Cache wide policy data
 *  sstream     (IN) - SAP specific stream
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTE
 *  
 *  As SAP specific stream is decided at the time of fill. So to update
 *  replacement stats same is passed to routine.
 */

void cache_update_sap_replacement_stats(sap_gdata *global_data, sap_stream sstream);

/*
 *
 * NAME
 *
 *  UpdateSapFillStats - Update SAP specific stats on fill and access
 *
 * DESCRIPTION
 *  
 *  Update stats on fill for PC in LRU table
 *
 * PARAMETERS
 *  
 *  global_data (IN) - Cache wide policy data
 *  info        (IN) - Access info
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTE
 *  
 */

void cache_update_sap_fill_stall_stats(sap_gdata *global_data, memory_trace *info);

/*
 *
 * NAME
 *
 *  DumpSapStats - Dump stats into statistics file
 *
 * DESCRIPTION
 *  
 *  Dump stats into statistics file and reset all counters
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SAP statistics
 *  cycle (IN)  - Current cycles
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_dump_sap_stats(sap_stats *stats, ub8 cycle);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
