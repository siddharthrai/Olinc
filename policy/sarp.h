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

#ifndef MEM_SYSTEM_SARP_H
#define	MEM_SYSTEM_SARP_H

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
#define MAX_SARP_CORES (5)
#define REPOCH_CNT     (3)

#define SARP_CSRRIP_GSRRIP_SET  (0)
#define SARP_CBRRIP_GSRRIP_SET  (1)
#define SARP_CSRRIP_GBRRIP_SET  (2)
#define SARP_CBRRIP_GBRRIP_SET  (3)
#define SARP_FOLLOWER_SET       (4)
#define SARP_FOLLOWER_CPU_SET   (5)
#define SARP_FOLLOWER_GPU_SET   (6)
#define SARP_SAMPLE_COUNT       (7)

/* 
 *  Sampler entry:
 *  --------------
 *
 *  Each entry maintains a bitvector of length equal to number of 64B blocks
 *  covered by a sampler entry
 *  Time-stamp is the miss count to the target cache set 
 *
 **/

#define SMPLRPERF_MREUSE(p)           ((p)->max_reuse)
#define SMPLRPERF_FILL(p, s)          ((p)->fill_count[s])
#define SMPLRPERF_FILL_EP(p, s)       ((p)->fill_count_epoch[s])
#define SMPLRPERF_SPILL(p, s)         ((p)->spill_count[s])
#define SMPLRPERF_FREUSE(p, s)        ((p)->fill_reuse_count[s])
#define SMPLRPERF_SREUSE(p, s)        ((p)->spill_reuse_count[s])
#define SMPLRPERF_FDHIGH(p, s)        ((p)->fill_reuse_distance_high[s])
#define SMPLRPERF_SDHIGH(p, s)        ((p)->spill_reuse_distance_high[s])
#define SMPLRPERF_FDLOW(p, s)         ((p)->fill_reuse_distance_low[s])
#define SMPLRPERF_SDLOW(p, s)         ((p)->spill_reuse_distance_low[s])
#define SMPLRPERF_FILL_RE(p, s, e)    ((p)->fill_count_per_reuse_epoch[s][e])
#define SMPLRPERF_FREUSE_RE(p, s, e)  ((p)->fill_reuse_per_reuse_epoch[s][e])
#define SMPLRPERF_FDHIGH_RE(p, s, e)  ((p)->fill_reuse_distance_high_per_reuse_epoch[s][e])
#define SMPLRPERF_FDLOW_RE(p, s, e)   ((p)->fill_reuse_distance_low_per_reuse_epoch[s][e])

typedef struct sampler_perfctr
{
  ub8 sampler_fill;
  ub8 sampler_hit;
  ub8 sampler_replace;
  ub8 sampler_access;
  ub8 sampler_block_found;
  ub8 sampler_invalid_block_found;
  ub8 max_reuse;
  ub8 fill_count[TST + 1];
  ub8 fill_count_epoch[TST + 1];
  ub8 spill_count[TST + 1];
  ub8 fill_reuse_count[TST + 1];
  ub8 spill_reuse_count[TST + 1];
  ub8 fill_reuse_distance_high[TST + 1];
  ub8 spill_reuse_distance_high[TST + 1];
  ub8 fill_reuse_distance_low[TST + 1];
  ub8 spill_reuse_distance_low[TST + 1];
  ub8 fill_count_per_reuse_epoch[TST + 1][REPOCH_CNT + 1];
  ub8 fill_reuse_per_reuse_epoch[TST + 1][REPOCH_CNT + 1];
  ub8 fill_reuse_distance_high_per_reuse_epoch[TST + 1][REPOCH_CNT + 1];
  ub8 fill_reuse_distance_low_per_reuse_epoch[TST + 1][REPOCH_CNT + 1];
}sampler_perfctr;

typedef struct sampler_entry
{
  ub8  page;          /* Tag: page id */
  ub8 *timestamp;     /* Timestamp of last access */
  ub1 *spill_or_fill; /* Is the last access to the block a spill or a fill? */
  ub1 *stream;        /* Id of the last stream to access this block */
  ub1 *valid;         /* Valid or invalid */
  ub1 *hit_count;     /* Current reuse epoch id */
  ub1 *dynamic_color; /* True, if dynamic cs */
  ub1 *dynamic_depth; /* True, if dynamic zs */
  ub1 *dynamic_blit;  /* True, if dynamic bs */
  ub1 *dynamic_proc;  /* True, if dynamic ps */
}sampler_entry;

/*
 * Sampler used for measuring reuses
 *
 */

typedef struct sampler_cache
{
  ub4             sets;                     /* # sampler sets */
  ub1             ways;                     /* # sampler ways */
  ub1             entry_size;               /* Sampler entry size */
  ub1             log_grain_size;           /* Log of sampler entry size */
  ub1             log_block_size;           /* Log of sampler entry size */
  ub8             epoch_length;             /* Length of sampler epoch */
  ub8             epoch_count;              /* Total epochs seen by the sampler */
  ub4             stream_occupancy[TST + 1];/* Block arrary */
  sampler_entry **blocks;                   /* Block arrary */
  sampler_perfctr perfctr;                  /* Performance counter used in sampler */
}sampler_cache;

/* Streams specific to SARP. SARP controller remaps global stream id to SARP 
 * specific ids */
typedef enum sarp_stream
{
  sarp_stream_u = 0,
  sarp_stream_x,
  sarp_stream_y,
  sarp_stream_p,
  sarp_stream_q,
  sarp_stream_r
}sarp_stream;

/* Policy statistics */
typedef struct cache_policy_sarp_stats_t
{
  FILE *stat_file;
  ub8   sarp_x_fill;               /* X set fill */
  ub8   sarp_y_fill;               /* Y set fill */
  ub8   sarp_p_fill;               /* P set fill */
  ub8   sarp_q_fill;               /* Q set fill */
  ub8   sarp_r_fill;               /* R set fill */
  ub8   sarp_x_evct;               /* X set blocks evicted */
  ub8   sarp_y_evct;               /* Y set blocks evicted */
  ub8   sarp_p_evct;               /* P set blocks evicted */
  ub8   sarp_q_evct;               /* Q set blocks evicted */
  ub8   sarp_r_evct;               /* R set blocks evicted */
  ub8   sarp_x_access;             /* X set access */
  ub8   sarp_y_access;             /* Y set access */
  ub8   sarp_p_access;             /* P set access */
  ub8   sarp_q_access;             /* Q set access */
  ub8   sarp_r_access;             /* R set access */
  ub8   sarp_psetrecat;            /* R set access */
  ub8   sarp_x_realsrrip[TST + 1]; /* # X set has been filled using speedup hint */
  ub8   sarp_x_realbrrip[TST + 1]; /* # X set has been filled using speedup hint */
  ub8   sarp_speedup[TST + 1];     /* # Speedup hint is used */
  ub8   next_schedule;                  /* Next cycle for dumping stats */
  ub8   schedule_period;                /* Scheduling period */
  ub1   hdr_printed;                    /* Set if header is written to file */
}sarp_stats;

/* Policy specific data */
typedef struct sarp_pdata
{
  sarp_stream stream;
}sarp_pdata;

#define SARP_DATA_SET_TYPE(data)   ((data)->set_type)
#define SARP_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define SARP_DATA_BLOCKS(data)     ((data)->blocks)
#define SARP_DATA_VALID_HEAD(data) ((data)->valid_head)
#define SARP_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define SARP_DATA_FREE_HLST(data)  ((data)->free_head)
#define SARP_DATA_FREE_TLST(data)  ((data)->free_tail)
#define SARP_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define SARP_DATA_FREE_TAIL(data)  ((data)->free_tail->head)
#define SARP_DATA_PSPOLICY(data)   ((data)->per_stream_policy)

/* SARP specific data */
typedef struct cache_policy_sarp_data_t
{
  /* SARP specific data */
  ub4         set_type;               /* SDPSIMPLE set type */
  ub4         max_rrpv;               /* Maximum RRPV supported */
  ub8         miss_count;             /* # misses seen by the set */
  rrip_list *valid_head;              /* Head pointers of RRPV specific list */
  rrip_list *valid_tail;              /* Tail pointers of RRPV specific list */

  list_head_t *free_head;             /* Free list head */
  list_head_t *free_tail;             /* Free list tail */

  struct cache_block_t *blocks;       /* Actual blocks */
  
  srrip_data  srrip;                  /* SRRIP specific data */
  brrip_data  brrip;                  /* SRRIP specific data */
  cache_policy_t *per_stream_policy;  /* Policy followed by each stream */
}sarp_data;

#define SARP_GDATA_TCORES(d) ((d)->sarp_cpu_cores  + (d)->sarp_gpu_cores )
#define SARP_GDATA_CORES(d)  (((d)->sarp_gpu_cores) ? SARP_GDATA_TCORES(d) : (d)->sarp_cpu_cores + 1)
#define SAMPLES              SARP_SAMPLE_COUNT

/* Policy global data */
typedef struct cache_policy_sarp_gdata_t
{
  ub4 ways;                   /* # ways */
  ub1 pin_blocks;             /* If true blocks are pinned as per algorithm  */
  ub4 sarp_cpu_cores;         /* CPU cores known to SARP */
  ub4 sarp_gpu_cores;         /* GPU cores known to SARP */
  ub4 sarp_streams;           /* Number of streams known to SARP */
  ub4 policy_set;             /* # sets for which policy is set */
  ub1 sarp_cpu_tpset;         /* CPU P set threshold */
  ub1 sarp_cpu_tqset;         /* CPU Q set threshold */
  ub8 sarp_y_access;          /* SARP Y stream access */
  ub8 sarp_y_miss;            /* SARP Y stream miss */
  ub8 activate_access;        /* LLC access to activate stream classification */ 
  ub1 activation_locked;      /* LLC access to activate stream classification */ 
  sarp_stats stats;           /* SARP statistics */
  ub8 threshold;              /* Bimodal threshold */
  ub1 sdpsimple_psetinstp;    /* If set, promote pset blocks to RRPV 0 on hit */
  ub1 sdpsimple_greplace;     /* If set, choose min wayid block as victim */
  ub1 sdpsimple_psetbmi;      /* If set, use bimodal insertion for P set */
  ub1 sdpsimple_psetchar;     /* If set, use hierarchy aware insertion for P set */
  ub1 sdpsimple_psetrecat;    /* If set, block is recategorized on hit */
  ub1 sdpsimple_sampleset;    /* #Baseline sample sets */
  ub1 sdpsimple_psetbse;      /* If set, only baseline samples are used for P set */
  ub1 sdpsimple_psetmrt;      /* If set, use miss rate threshold for P set */
  ub1 sdpsimple_psethrt;      /* If set, use hit rate threshold for P set */
  sctr access_ctr;            /* Access counter */
  srrip_gdata srrip;          /* SRRIP specific global data */
  brrip_gdata brrip;          /* BRRIP specific global data */
  sctr srrip_psel;            /* Selection counter for SRRIP_SRRIP and SRRIP_BRRIP sample */
  sctr brrip_psel;            /* Selection counter for BRRIP_SRRIP and BRRIP_BRRIP sample */
  sctr sarp_psel;             /* Global selection counter */
  sctr sarp_ssel[TST + 1];    /* Per-stream DRRIP policy counter */
  ub8  fmiss_count[TST + 1];  /* Per-stream miss count */
  ub8  smiss_count[TST + 1];  /* Per-stream miss count */
  sctr baccess[SAMPLES];      /* Separate BRRIP counter to decide epsilon for each BRRIP sampled set */
  ub8  bypass_count;          /* # bypass */
  sampler_cache *sampler;     /* Sampler cache used for tracking reuses */
}sarp_gdata;

#undef SAMPLES

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
 *  set_type    (IN)  - Set type decided by SDPSIMPLE
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTES
 */

void cache_init_sarp(long long int set_indx, struct cache_params *params, 
    sarp_data *policy_data, sarp_gdata *global_data);

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

void cache_free_sarp(long long int set_indx, sarp_data *policy_data, sarp_gdata *global_data);

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
 *  info        (OUT) - Access info
 *
 * RETURNS
 *  
 *  Pointer to the block 
 */

struct cache_block_t* cache_find_block_sarp(sarp_data *policy_data, 
    sarp_gdata *global_data, long long tag, memory_trace *info);

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

void cache_fill_block_sarp(sarp_data *policy_data, sarp_gdata *global_data,
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
 *  policy_data (IN)  - Per-set policy data
 *  global_data (IN)  - Cache wide policy data
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Way id from where to replace the block
 *
 */

int cache_replace_block_sarp(sarp_data *policy_data, sarp_gdata *global_data, 
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

void cache_access_block_sarp(sarp_data *policy_data, sarp_gdata *global_data,
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

struct cache_block_t cache_get_block_sarp(sarp_data *policy_data, sarp_gdata *global_data, 
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

void cache_set_block_sarp(sarp_data *policy_data, sarp_gdata *global_data,
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

void set_per_stream_policy_sarp(sarp_data *policy_data, sarp_gdata *global_data, 
  ub4 core, ub1 stream, cache_policy_t policy);

/*
 *
 * NAME
 *
 *  GetSapStream - Obtain SARP specific stream 
 *
 * DESCRIPTION
 *  
 *  Obtains SARP specific stream (X, Y, P, Q, R) based on categorization 
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
 *  SARP specific stream
 */

sarp_stream get_sarp_stream(sarp_gdata *global_data, ub1 stream_in, ub1 pid_in,
  memory_trace *info);

/*
 *
 * NAME
 *
 *  InitSapStats - Initialize SARP specific statistics
 *
 * DESCRIPTION
 *  
 *  Initialize SARP statistics and open stats file
 *
 * PARAMETERS
 *  
 *  stats     (IN)  - SARP statistics
 *  file_name (IN)  - Statistics file
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_init_sarp_stats(sarp_stats *stats, char *file_name);

/*
 *
 * NAME
 *
 *  FinalizeSapStats - Finalize SARP statistics
 *
 * DESCRIPTION
 *  
 *  Clean up SARP stream an close statistics file
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SARP statistics
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_fini_sarp_stats(sarp_stats *stats);

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

void cache_update_sarp_access_stats(sarp_gdata *global_data, sarp_stream ssream);

/*
 *
 * NAME
 *
 *  UpdateSapFillRealStats - Update stats on fill
 *
 * DESCRIPTION
 *  
 *  Update stats for speedup based fill policy decision
 *
 * PARAMETERS
 *  
 *  global_data (IN) - Cache wide policy data
 *  sstream     (IN) - SARP specific stream
 *  fill_policy (IN) - Fill policy
 *  info        (IN) - Access info
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTE
 *  
 */

void cache_update_sarp_realfill_stats(sarp_gdata *global_data, 
    sarp_stream sstream, enum cache_policy_t fill_policy, 
    memory_trace *info);

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
 *  sstream     (IN) - SARP specific stream
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTE
 *  
 *  As SARP specific stream is decided at the time of fill.
 */

void cache_update_sarp_fill_stats(sarp_gdata *global_data, sarp_stream sstream);

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
 *  sstream     (IN) - SARP specific stream
 *
 * RETURNS
 *  
 *  Nothing
 *
 * NOTE
 *  
 *  As SARP specific stream is decided at the time of fill. So to update
 *  replacement stats same is passed to routine.
 */

void cache_update_sarp_replacement_stats(sarp_gdata *global_data, sarp_stream sstream);

/*
 *
 * NAME
 *
 *  UpdateSapFillStats - Update SARP specific stats on fill and access
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

void cache_update_sarp_fill_stall_stats(sarp_gdata *global_data, memory_trace *info);

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
 *  stats (IN)  - SARP statistics
 *  cycle (IN)  - Current cycles
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_dump_sarp_stats(sarp_stats *stats, ub8 cycle);

/*
 *
 * NAME
 *
 *  PenalizeCPUCore - Choose a PC to penalize base on a criteria 
 *
 * DESCRIPTION
 *  
 *  Chooses a PC that has least number of stall cycles.
 *
 * PARAMETERS
 *  
 *  global_data    (IN)  - SARP cache wide data
 *  max_bsmps_core (IN)  - CPU core to penalize
 *
 * RETURNS
 *  
 *  Nothing
 */

void sarp_penalize_cpu_core(sarp_gdata *global_data, ub8 max_bsmps_core);

/*
 *
 * NAME
 *
 *  PenalizeGraphicsCore - Choose a graphics stream to penalize
 *
 * DESCRIPTION
 *  
 *  Choose a graphics stream with minimum number of ROB stall cycles to 
 *  penalize.
 *
 * PARAMETERS
 *  
 *  Nothing
 *
 * RETURNS
 *  
 *  Nothing
 */

void sarp_penalize_gfx_core();
      

/*
 *
 * NAME
 *
 *  UnlockActivation - Unlock activation period
 *
 * DESCRIPTION
 *  
 * PARAMETERS
 *  
 *  Nothing
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_sarp_unlock_activation(sarp_gdata *global_data);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
