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

#ifndef MEM_SYSTEM_GSDRRIP_H
#define	MEM_SYSTEM_GSDRRIP_H

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
#include <stdio.h>

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_gsdrrip_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}gsdrrip_list;

/* GSDRRIP statistics */
typedef struct cache_policy_gsdrrip_stats_t
{
  FILE *gsdrrip_stat_file;      /* Samples used for SRRIP */
  ub8   gsdrrip_srrip_samples;  /* Samples used for SRRIP */
  ub8   gsdrrip_brrip_samples;  /* Samples used for BRRIP */
  ub8   sample_srrip_fill;    /* Fills in SRRIP samples */
  ub8   sample_brrip_fill;    /* Fills in BRRIP samples */
  ub8   sample_srrip_hit;     /* Fills in SRRIP samples */
  ub8   sample_brrip_hit;     /* Fills in BRRIP samples */
  ub8   gsdrrip_srrip_fill;     /* Fills using SRRIP */
  ub8   gsdrrip_brrip_fill;     /* Fills using BRRIP */
  ub8   gsdrrip_fill_2;         /* GSDRRIP fill at RRPV 2 */
  ub8   gsdrrip_fill_3;         /* GSDRRIP fill at RRPV 3 */
  ub1   gsdrrip_hdr_printed;    /* True if header has been printed */
  ub8   next_schedule;        /* Cycle to schedule stat collection */
}gsdrrip_stats;

#define GSDRRIP_DATA_FOLLOWING(data)  ((data)->following)
#define GSDRRIP_DATA_VALID_HEAD(data) ((data)->valid_head)
#define GSDRRIP_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define GSDRRIP_DATA_FREE_HLST(data)  ((data)->free_head)
#define GSDRRIP_DATA_FREE_TLST(data)  ((data)->free_tail)
#define GSDRRIP_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define GSDRRIP_DATA_FREE_TAIL(data)  ((data)->free_tail->head)
#define GSDRRIP_DATA_BLOCKS(data)     ((data)->blocks)

/* GSDRRIP specific data */
typedef struct cache_policy_gsdrrip_data_t
{
  ub1                   set_type;   /* Set type */
  cache_policy_t       *following;  /* Currently followed policy */
  srrip_data            srrip;      /* SRRIP policy specific data */
  brrip_data            brrip;      /* BRRIP policy specific data */
  rrip_list            *valid_head; /* Head pointers of RRPV specific list */
  rrip_list            *valid_tail; /* Tail pointers of RRPV specific list */
  list_head_t          *free_head;  /* Free list head */
  list_head_t          *free_tail;  /* Free list tail */
  struct cache_block_t *blocks;     /* Actual blocks */
}gsdrrip_data;

/* Policy global data */
typedef struct cache_policy_gsdrrip_gdata_t
{
  ub1           gsdrrip_streams;      /* Number of streams */
  ub1           gsdrrip_cpu_enable;   /* True, if CPU is present */
  ub1           gsdrrip_gpu_enable;   /* True, if GPU is present */
  ub1           gsdrrip_gpgpu_enable; /* True, if GPGPU is present */
  sctr         *psel;                 /* Policy selection counter */
  sctr          access_count;         /* Access to decide bimodal insertion  */
  gsdrrip_stats stats;                /* GSDRRIP statistics */
  brrip_gdata   brrip;                /* BRRIP cache wide data */
  srrip_gdata   srrip;                /* BRRIP cache wide data */
}gsdrrip_gdata;

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

void cache_init_gsdrrip(long long int set_indx, struct cache_params *params, 
  gsdrrip_data *policy_data, gsdrrip_gdata *global_data);

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

void cache_free_gsdrrip(unsigned int set_indx, gsdrrip_data *policy_data,
  gsdrrip_gdata *global_data);

/*
 *
 * NAME
 *  
 *  InitDrripStats - Initialize GSDRRIP stats
 *
 * DESCRIPTION
 *  
 *  Initialize GSDRRIP stats
 *
 * PARAMETERS
 *  
 *  stats (IN)  - GSDRRIP statistics
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_init_gsdrrip_stats(gsdrrip_stats *stats);

/*
 *
 * NAME
 *  
 *  FiniDrripStats - Finalize GSDRRIP stats
 *
 * DESCRIPTION
 *  
 *  Finalize GSDRRIP stats
 *
 * PARAMETERS
 *  
 *  stats (IN)  - GSDRRIP statistics
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_fini_gsdrrip_stats(gsdrrip_stats *stats);

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
 *  stats (IN)  - GSDRRIP statistics
 *  cycle (IN)  - Current cycle
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_dump_gsdrrip_stats(gsdrrip_stats *stats, ub8 cycle);

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

struct cache_block_t * cache_find_block_gsdrrip(gsdrrip_data *policy_data, long long tag);

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

void cache_fill_block_gsdrrip(gsdrrip_data *policy_data, gsdrrip_gdata *global_data, 
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
 *
 * RETURNS
 *  
 *  Way id from where to replace the block
 *
 */

int  cache_replace_block_gsdrrip(gsdrrip_data *policy_data, 
    gsdrrip_gdata *global_data, memory_trace *info);

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

void cache_access_block_gsdrrip(gsdrrip_data *policy_data, gsdrrip_gdata *global_data,
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

struct cache_block_t cache_get_block_gsdrrip(gsdrrip_data *policy_data, 
  gsdrrip_gdata *global_data, int way, long long *tag_ptr, 
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

void cache_set_block_gsdrrip(gsdrrip_data *policy_data, gsdrrip_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
