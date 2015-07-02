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

#ifndef MEM_SYSTEM_SAPDBP_H
#define	MEM_SYSTEM_SAPDBP_H

#ifdef __cplusplus
# define EXPORT_C extern "C"
#else
# define EXPORT_C
#endif

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "policy.h"
#include "rrip.h"
#include "cache-param.h"
#include "cache-block.h"
#include "srrip.h"
#include "brrip.h"
#include "sap.h"
#include "sdp.h"
#include <stdio.h>

/* Streams specific to SAPDBP. SAPDBP controller remaps global stream id to SAPDBP 
 * specific ids */
typedef enum sapdbp_stream
{
  sapdbp_stream_u = 0,
  sapdbp_stream_x,
  sapdbp_stream_y,
  sapdbp_stream_p,
  sapdbp_stream_q,
  sapdbp_stream_r
}sapdbp_stream;

/* SAPDBP statistics */
typedef struct cache_policy_sapdbp_stats_t
{
  FILE *sapdbp_stat_file;       /* Samples used for SRRIP */
  ub8   sapdbp_srrip_samples;   /* Samples used for SRRIP */
  ub8   sapdbp_brrip_samples;   /* Samples used for BRRIP */
  ub8   sample_srrip_fill;      /* Fills in SRRIP samples */
  ub8   sample_brrip_fill;      /* Fills in BRRIP samples */
  ub8   sample_srrip_hit;       /* Fills in SRRIP samples */
  ub8   sample_brrip_hit;       /* Fills in BRRIP samples */
  ub8   sapdbp_srrip_fill;      /* Fills using SRRIP */
  ub8   sapdbp_brrip_fill;      /* Fills using BRRIP */
  ub8   sapdbp_fill_2;          /* SAPDBP fill at RRPV 2 */
  ub8   sapdbp_fill_3;          /* SAPDBP fill at RRPV 3 */
  ub8   sapdbp_fill[TST + 1];   /* SAPDBP fill at RRPV 3 */
  ub8   sapdbp_ps_fill[TST + 1];/* Per-stream fill */
  ub8   sapdbp_ps_thr[TST + 1]; /* Per-stream thrahing fills */
  ub8   sapdbp_ps_dep[TST + 1]; /* Per-stream deprioritization */
  ub1   sapdbp_hdr_printed;     /* True if header has been printed */
  ub8   epoch_count;            /* #epochs during entire execution */
  ub8   speedup_count[TST + 1]; /* #speedup */
  ub8   thrasher_count[TST + 1];/* #thrasher */
  ub8   dead_blocks[TST + 1];   /* #thrasher */
  ub8   next_schedule;          /* Cycle to schedule stat collection */
}sapdbp_stats;

#define SAPDBP_DATA_BLOCKS(data)     ((data)->blocks)
#define SAPDBP_DATA_VALID_HEAD(data) ((data)->valid_head)
#define SAPDBP_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define SAPDBP_DATA_FREE_HLST(data)  ((data)->free_head)
#define SAPDBP_DATA_FREE_TLST(data)  ((data)->free_tail)
#define SAPDBP_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define SAPDBP_DATA_FREE_TAIL(data)  ((data)->free_tail->head)

/* SAPDBP specific data */
typedef struct cache_policy_sapdbp_data_t
{
  cache_policy_t        following;  /* Currently followed policy */
  ub4                   set_type;   /* Type of the set (sampled / follower)*/
  srrip_data            srrip;      /* SRRIP policy specific data */
  brrip_data            brrip;      /* BRRIP policy specific data */
  rrip_list            *valid_head; /* Head pointers of RRPV specific list */
  rrip_list            *valid_tail; /* Tail pointers of RRPV specific list */
  list_head_t          *free_head;  /* Free list head */
  list_head_t          *free_tail;  /* Free list tail */
  struct cache_block_t *blocks;     /* Actual blocks */
}sapdbp_data;

/* Policy global data */
typedef struct cache_policy_sapdbp_gdata_t
{
  sctr          sapdbp_ssel[TST + 1];   /* Selection counter for modified policy */
  sctr          drrip_psel;             /* Policy selection counter */
  sctr          sapdbp_psel;            /* Per-stream policy selection counter */
  sapdbp_stats  stats;                  /* SAPDBP statistics */
  brrip_gdata   brrip;                  /* BRRIP cache wide data */
  sdp_gdata     sdp;                    /* SDP cache wide data for SAP like stats */
  srrip_gdata   srrip;                  /* SDP cache wide data for SAP like stats */
  sctr          sapdbp_hint[TST + 1];   /* Accumulation counter for per-stream speedup */

  sctr **epoch_fctr;                    /* Per stream epoch fill counter */
  sctr **epoch_hctr;                    /* Per-stream epoch hit counter */
  ub1  *epoch_valid;                    /* Per-stream epoch hit counter */
  sctr **epoch_xevict;                  /* Per-stream epoch xstream evictions */
  ub1  *epoch_thrasher;                 /* Per-stream thrashing stream */
  ub1  *thrasher_stream;                /* TRUE, if stream is thrashing */
  ub1   speedup_stream;                 /* Stream to be sped up */
  ub8   access_interval;                /* #Accesses */
  ub1   bs_epoch;                       /* TRUE, if only baseline samples are used for epoch */
}sapdbp_gdata;

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

void cache_init_sapdbp(long long int set_indx, struct cache_params *params, 
  sapdbp_data *policy_data, sapdbp_gdata *global_data);

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
 *  policy_data (IN)  - Per set policy data
 *  global_data (IN)  - Cache wide policy data
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_free_sapdbp(unsigned int set_indx, sapdbp_data *policy_data,
  sapdbp_gdata *global_data);

/*
 *
 * NAME
 *  
 *  InitDrripStats - Initialize SAPDBP stats
 *
 * DESCRIPTION
 *  
 *  Initialize SAPDBP stats
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SAPDBP statistics
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_init_sapdbp_stats(sapdbp_stats *stats);

/*
 *
 * NAME
 *  
 *  FiniDrripStats - Finalize SAPDBP stats
 *
 * DESCRIPTION
 *  
 *  Finalize SAPDBP stats
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SAPDBP statistics
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_fini_sapdbp_stats(sapdbp_stats *stats);

/*
 *
 * NAME
 *  
 *  DumpDrripStats - Dump current stats 
 *
 * DESCRIPTION
 *  
 *  Dump current stats and reset counters
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SAPDBP statistics
 *  cycle (IN)  - Current cycle
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_dump_sapdbp_stats(sapdbp_stats *stats, ub8 cycle);

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
 *  global_data (IN)  - Cache-wide data 
 *  info        (IN)  - Access info
 *  tag         (IN)  - Block tag
 *
 * RETURNS
 *  
 *  Pointer to the block 
 */

struct cache_block_t * cache_find_block_sapdbp(sapdbp_data *policy_data, 
    sapdbp_gdata *global_data, memory_trace *info, long long tag);

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

void cache_fill_block_sapdbp(sapdbp_data *policy_data, sapdbp_gdata *global_data, 
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
 *  global_data (IN)  - Cache wide policy data
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Way id from where to replace the block
 *
 */

int  cache_replace_block_sapdbp(sapdbp_data *policy_data, 
    sapdbp_gdata *global_data, memory_trace *info);

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

void cache_access_block_sapdbp(sapdbp_data *policy_data, sapdbp_gdata *global_data,
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
 *  global_data (IN)  - Cache wide policy data
 *  way         (IN)  - Way of the block
 *  tag_ptr     (OUT) - Tag of the block
 *  state_ptr   (OUT) - Current state
 *
 * RETURNS
 *  
 *  Complete block info
 */

struct cache_block_t cache_get_block_sapdbp(sapdbp_data *policy_data, 
  sapdbp_gdata *global_data, int way, long long *tag_ptr, 
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

void cache_set_block_sapdbp(sapdbp_data *policy_data, sapdbp_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info);

sapdbp_stream get_sapdbp_stream(memory_trace *info);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
