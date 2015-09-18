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

#ifndef MEM_SYSTEM_SAPSIMPLE_H
#define	MEM_SYSTEM_SAPSIMPLE_H

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

/* Streams specific to SAPSIMPLE. SAPSIMPLE controller remaps global stream id to SAPSIMPLE 
 * specific ids */
typedef enum sapsimple_stream
{
  sapsimple_stream_u = 0,
  sapsimple_stream_x,
  sapsimple_stream_y,
  sapsimple_stream_p,
  sapsimple_stream_q,
  sapsimple_stream_r
}sapsimple_stream;

/* SAPSIMPLE statistics */
typedef struct cache_policy_sapsimple_stats_t
{
  FILE *sapsimple_stat_file;      /* Samples used for SRRIP */
  ub8   sapsimple_srrip_samples;  /* Samples used for SRRIP */
  ub8   sapsimple_brrip_samples;  /* Samples used for BRRIP */
  ub8   sample_srrip_fill;        /* Fills in SRRIP samples */
  ub8   sample_brrip_fill;        /* Fills in BRRIP samples */
  ub8   sample_srrip_hit;         /* Fills in SRRIP samples */
  ub8   sample_brrip_hit;         /* Fills in BRRIP samples */
  ub8   sapsimple_srrip_fill;     /* Fills using SRRIP */
  ub8   sapsimple_brrip_fill;     /* Fills using BRRIP */
  ub8   sapsimple_fill_2;         /* SAPSIMPLE fill at RRPV 2 */
  ub8   sapsimple_fill_3;         /* SAPSIMPLE fill at RRPV 3 */
  ub1   sapsimple_hdr_printed;    /* True if header has been printed */
  ub8   next_schedule;            /* Cycle to schedule stat collection */
}sapsimple_stats;

#define SAPSIMPLE_DATA_BLOCKS(data)     ((data)->blocks)
#define SAPSIMPLE_DATA_VALID_HEAD(data) ((data)->valid_head)
#define SAPSIMPLE_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define SAPSIMPLE_DATA_FREE_HLST(data)  ((data)->free_head)
#define SAPSIMPLE_DATA_FREE_TLST(data)  ((data)->free_tail)
#define SAPSIMPLE_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define SAPSIMPLE_DATA_FREE_TAIL(data)  ((data)->free_tail->head)

/* SAPSIMPLE specific data */
typedef struct cache_policy_sapsimple_data_t
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
}sapsimple_data;

/* Policy global data */
typedef struct cache_policy_sapsimple_gdata_t
{
  sctr        sapsimple_ssel[TST + 1];  /* Selection counter for modified policy */
  sctr        drrip_psel;               /* Policy selection counter */
  sctr        sapsimple_psel;           /* Per-stream policy selection counter */
  sapsimple_stats stats;                /* SAPSIMPLE statistics */
  brrip_gdata brrip;                    /* BRRIP cache wide data */
  sdp_gdata   sdp;                      /* SDP cache wide data for SAP like stats */
  srrip_gdata srrip;                    /* SDP cache wide data for SAP like stats */

  sctr **epoch_fctr;                    /* Per stream epoch fill counter */
  sctr **epoch_hctr;                    /* Per-stream epoch hit counter */

  ub8  access_interval;
}sapsimple_gdata;

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

void cache_init_sapsimple(long long int set_indx, struct cache_params *params, 
  sapsimple_data *policy_data, sapsimple_gdata *global_data);

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

void cache_free_sapsimple(unsigned int set_indx, sapsimple_data *policy_data,
  sapsimple_gdata *global_data);

/*
 *
 * NAME
 *  
 *  InitDrripStats - Initialize SAPSIMPLE stats
 *
 * DESCRIPTION
 *  
 *  Initialize SAPSIMPLE stats
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SAPSIMPLE statistics
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_init_sapsimple_stats(sapsimple_stats *stats);

/*
 *
 * NAME
 *  
 *  FiniDrripStats - Finalize SAPSIMPLE stats
 *
 * DESCRIPTION
 *  
 *  Finalize SAPSIMPLE stats
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SAPSIMPLE statistics
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_fini_sapsimple_stats(sapsimple_stats *stats);

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
 *  stats (IN)  - SAPSIMPLE statistics
 *  cycle (IN)  - Current cycle
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_dump_sapsimple_stats(sapsimple_stats *stats, ub8 cycle);

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

struct cache_block_t * cache_find_block_sapsimple(sapsimple_data *policy_data, long long tag);

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

void cache_fill_block_sapsimple(sapsimple_data *policy_data, sapsimple_gdata *global_data, 
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

int  cache_replace_block_sapsimple(sapsimple_data *policy_data, 
    sapsimple_gdata *global_data, memory_trace *info);

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

void cache_access_block_sapsimple(sapsimple_data *policy_data, sapsimple_gdata *global_data,
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

struct cache_block_t cache_get_block_sapsimple(sapsimple_data *policy_data, 
  sapsimple_gdata *global_data, int way, long long *tag_ptr, 
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

void cache_set_block_sapsimple(sapsimple_data *policy_data, sapsimple_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info);

sapsimple_stream get_sapsimple_stream(memory_trace *info);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
