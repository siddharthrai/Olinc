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

#ifndef MEM_SYSTEM_SRRIPHINT_H
#define	MEM_SYSTEM_SRRIPHINT_H

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "../common/List.h"
#include "zlib.h"
#include "policy.h"
#include "rrip.h"
#include "brrip.h"
#include "cache-param.h"
#include "cache-block.h"

#define SRRIPHINT_SRRIP_SET       (0)
#define SRRIPHINT_GPU_SET         (1)
#define SRRIPHINT_GPU_COND_SET    (2)
#define SRRIPHINT_SHIP_FILL_SET   (3)
#define SRRIPHINT_SRRIP_FILL_SET  (4)
#define SRRIPHINT_FOLLOWER_SET    (5)

#define USE_NN    (0)  
#define USE_PC    (1)  
#define USE_MEM   (2)  
#define USE_MEMPC (3)  

#define INVALID_RRPV (16)

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_srriphint_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}srriphint_list;

// Jimenez's sampler code
typedef struct sdbp_sampler_entry 
{
	ub4 lru_stack_position;
  ub4 tag;
	ub4 trace;
	ub4 prediction;
	ub1 valid;
}sdbp_sampler_entry;

// constructor for sampler entry
void sdbp_sampler_entry_init(sdbp_sampler_entry *entry);

// one sampler set (just a pointer to the entries)
typedef struct sdbp_sampler_set 
{
	sdbp_sampler_entry *blocks;
}sdbp_sampler_set;

// the dead block predictor
typedef struct sdbp_predictor 
{
	sb4 **tables; 	// tables of two-bit counters
}sdbp_predictor;

// SDBP sampler
typedef struct sdbp_sampler 
{
	sb4 nsampler_sets;            // number of sampler sets
  sb4	sampler_modulus;          // determines which LLC sets are sampler sets

	ub4 dan_sampler_assoc;

	// number of bits used to index predictor; determines number of
	// entries in prediction tables (changed for 4MB cache)
	ub4 dan_predictor_index_bits;
	// number of prediction tables
	ub4 dan_predictor_tables;
	// width of prediction saturating counters
	ub4 dan_counter_width;
	// predictor must meet this threshold to predict a block is dead
	ub4 dan_threshold;
	// number of partial tag bits kept per sampler entry
	ub4 dan_sampler_tag_bits;
	// number of trace (partial PC) bits kept per sampler entry
	ub4 dan_sampler_trace_bits;
	// number of entries in prediction table; derived from # of index bits
	ub4 dan_predictor_table_entries;
	// maximum value of saturating counter; derived from counter width
	ub4 dan_counter_max;
	// total number of bits used by all structures; computed in sampler::sampler
	ub4 total_bits_used;

	sdbp_sampler_set *sets;
	sdbp_predictor   *pred;
}sdbp_sampler;

void sdbp_sampler_init(sdbp_sampler *sampler, sb4 nsets, sb4 assoc);
void sdbp_smpler_access(sdbp_sampler *sampler, ub4 tid, sb4 set, ub8 tag, ub8 PC);

void sdbp_sampler_set_init(sdbp_sampler *sampler, sdbp_sampler_set *sampler_set);

void sdbp_predictor_init(sdbp_sampler *sampler, sdbp_predictor *pred);

ub1 sdbp_predictor_get_prediction(sdbp_sampler *sampler, sdbp_predictor *pred, ub4 tid, ub4 trace, sb4 set);

void sdbp_predictor_block_is_dead(sdbp_sampler *sampler, sdbp_predictor *pred, ub4 tid, ub4 trace, ub1 d);

#define SRRIPHINT_DATA_CFPOLICY(data)     ((data)->current_fill_policy)
#define SRRIPHINT_DATA_DFPOLICY(data)     ((data)->default_fill_policy)
#define SRRIPHINT_DATA_CAPOLICY(data)     ((data)->current_access_policy)
#define SRRIPHINT_DATA_DAPOLICY(data)     ((data)->default_access_policy)
#define SRRIPHINT_DATA_CRPOLICY(data)     ((data)->current_replacement_policy)
#define SRRIPHINT_DATA_DRPOLICY(data)     ((data)->default_replacement_policy)
#define SRRIPHINT_DATA_MAX_RRPV(data)     ((data)->max_rrpv)
#define SRRIPHINT_DATA_SPILL_RRPV(data)   ((data)->spill_rrpv)
#define SRRIPHINT_DATA_BLOCKS(data)       ((data)->blocks)
#define SRRIPHINT_DATA_VALID_HEAD(data)   ((data)->valid_head)
#define SRRIPHINT_DATA_VALID_TAIL(data)   ((data)->valid_tail)
#define SRRIPHINT_DATA_CVALID_HEAD(data)  ((data)->cpu_valid_head)
#define SRRIPHINT_DATA_CVALID_TAIL(data)  ((data)->cpu_valid_tail)
#define SRRIPHINT_DATA_GVALID_HEAD(data)  ((data)->gpu_valid_head)
#define SRRIPHINT_DATA_GVALID_TAIL(data)  ((data)->gpu_valid_tail)
#define SRRIPHINT_DATA_FREE_HLST(data)    ((data)->free_head)
#define SRRIPHINT_DATA_FREE_TLST(data)    ((data)->free_tail)
#define SRRIPHINT_DATA_FREE_HEAD(data)    ((data)->free_head->head)
#define SRRIPHINT_DATA_FREE_TAIL(data)    ((data)->free_tail->head)
#define SRRIPHINT_DATA_USE_EPOCH(data)    ((data)->use_epoch)

/* 
 *  Sampler entry:
 *  --------------
 *
 *  Each entry maintains a bitvector of length equal to number of 64B blocks
 *  covered by a sampler entry
 *  Time-stamp is the miss count to the target cache set 
 *
 **/

#define REPOCH_CNT                    (3)

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
#define SMPLRPERF_FRRPV(p, s)         ((p)->stream_fill_rrpv[s])
#define SMPLRPERF_HRRPV(p, s)         ((p)->stream_hit_rrpv[s])

typedef struct srriphint_sampler_perfctr
{
  ub8 sampler_fill;
  ub8 sampler_hit;
  ub8 sampler_replace;
  ub8 sampler_access;
  ub8 sampler_block_found;
  ub8 sampler_invalid_block_found;
  ub8 sstream_reuse;
  ub8 xstream_reuse;
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

#define MAX_RRPV (3)

  ub8 stream_fill_rrpv[TST + 1][MAX_RRPV];
  ub8 stream_hit_rrpv[TST + 1][MAX_RRPV];

#undef MAX_RRPV
}srriphint_sampler_perfctr;

/* 
 * There is one sampler entry for the entire 4KB page. The entry tracks the page on 
 * n-byte block granularity. For each block following counter are maintained
 *
 *  Timestamp         - # misses in the mapping set
 *  spill_of_fill     - Kind of last access (spill / fill)
 *  stream            - Last accessing stream
 *  valid             - True, for a valid block
 *  hit_count         - Reuse count (reuses go upto a maximum)
 *  dynamic_[c,d,b,p] - True, if block is a dynamically generated block
 *
 * All sampler entries are reset after an epoch interval
 *
 * */

typedef struct srriphint_sampler_entry
{
  ub8   page;           /* Tag: page id */
  ub8  *timestamp;      /* Timestamp of last access */
  ub1  *spill_or_fill;  /* Is the last access to the block a spill or a fill? */
  ub1  *stream;         /* Id of the last stream to access this block */
  ub1  *valid;          /* Valid or invalid */
  ub1  *hit_count;      /* Current reuse epoch id */
  ub8  *pc;             /* PC last accessing the block */
  ub8  *reuse;          /* PC last accessing the block */
  ub1  *dynamic_color;  /* True, if dynamic CS */
  ub1  *dynamic_depth;  /* True, if dynamic ZS */
  ub1  *dynamic_blit;   /* True, if dynamic BS */
  ub1  *dynamic_proc;   /* True, if dynamic PS */
}srriphint_sampler_entry;

/*
 * Sampler used for measuring reuses
 *
 */

typedef struct srriphint_d_data
{
  ub8 distance;
  ub8 ppc;
  ub8 pc;
  ub8 reuse;
}shnt_d_data;

typedef struct srriphint_pc_data
{
  ub8       distance;         /* Current measure PC distance */
  ub8       start_pc;         /* Start distance */
  ub8       region_count;     /* Regions touched by the PC */
  ub8       end_pc;           /* Regions touched by the PC */
  ub8       dead_limit;       /* Dead limit, if PC has seen reuse */
  ub8       dead_distance;    /* Dead distance, if PC has seen reuse */
  cs_qnode  ppc[HTBLSIZE];    /* Previous PC */
  cs_qnode  distance_list;    /* All measured distances */
}shnt_pc_data;

#define RPHASE_SIZE   (256 * 1024)
#define SPHASE_SIZE   (128 * 1024)
#define RPHASE_CNT    (2)
#define RPHASE_0      (0)
#define RPHASE_1      (1)

typedef struct srriphint_region_pc
{
  ub8 pc;
  ub8 index;
  ub8 phase_id;
  ub8 use_count;
  ub8 use_distance;
  ub8 distance[RPHASE_CNT];
  ub8 short_distance[RPHASE_CNT];
}shnt_region_pc;

typedef struct srriphint_region_data
{
  ub8       id;                         /* Current measure PC distance */
  ub8       dead_limit;                 /* Limit to consider region to be dead */
  ub8       short_distance[RPHASE_CNT]; /* Latest short distance learned for each phase */
  ub8       distance[RPHASE_CNT];       /* Latest distance learned for each phase */
  ub8       next_distance;              /* Up-to-date next distance */
  ub8       short_next_distance;        /* Up-to-date short next distance */
  ub4       pc_count;                   /* # distinct pc in the region */
  cs_qnode  pc[HTBLSIZE];               /* PC which belong to this region */
}shnt_region_data;

typedef struct srriphint_sampler_cache
{
  ub4             sets;                       /* # sampler sets */
  ub1             ways;                       /* # sampler ways */
  ub1             entry_size;                 /* Sampler entry size */
  ub1             log_grain_size;             /* Log of sampler entry size */
  ub1             log_block_size;             /* Log of sampler entry size */
  ub8             epoch_length;               /* Length of sampler epoch */
  ub8             epoch_count;                /* Total epochs seen by the sampler */
  ub4             stream_occupancy[TST + 1];  /* Block array */
  ub8             spc_count;                  /* Shared PC count */
  ub8             pc_distance;                /* PC reuse distance  */
  cs_qnode        spc_list[HTBLSIZE];         /* List of shared PCs */
  cs_qnode        all_pc_list[HTBLSIZE];      /* List of all PCs */
  ub8             reuse_phase_size;           /* Phase size in terms of access count */
  ub8             reuse_short_phase_size;     /* Short phase size in terms of access count */
  ub8             reuse_phase_count;          /* Phase count */
  ub1             current_reuse_pahse;        /* Current phase of execution for reuse distance */
  ub1             current_short_reuse_pahse;  /* Current phase of execution for reuse distance */
  sctr            region_usage;               /* Counter to track region usage */
  cs_qnode        all_region_list[HTBLSIZE];  /* List of all memory regions */
  srriphint_sampler_entry **blocks;           /* Block array */
  srriphint_sampler_perfctr perfctr;          /* Performance counter used in sampler */
}shnt_sampler_cache;

/* RRIP specific data */
typedef struct cache_policy_srriphint_data_t
{
  cache_policy_t following;                   /* Policy followed by current set */
  cache_policy_t current_fill_policy;         /* If non-default fill policy is enforced */
  cache_policy_t default_fill_policy;         /* Default fill policy */
  cache_policy_t current_access_policy;       /* If non-default fill policy is enforced */
  cache_policy_t default_access_policy;       /* Default fill policy */
  cache_policy_t current_replacement_policy;  /* If non-default fill policy is enforced */
  cache_policy_t default_replacement_policy;  /* Default fill policy */
  ub4            set_indx;                    /* Set index */
  ub4            set_type;                    /* Set type */
  ub4            max_rrpv;                    /* Maximum RRPV supported */
  ub4            spill_rrpv;                  /* Spill RRPV supported */
  rrip_list     *valid_head;                  /* Head pointers of RRPV specific list */
  rrip_list     *valid_tail;                  /* Tail pointers of RRPV specific list */
  list_head_t   *free_head;                   /* Free list head */
  list_head_t   *free_tail;                   /* Free list tail */
  ub8            miss_count;                  /* # miss in the set */
  ub8            hit_count;                   /* # hit in the set */
  ub8            cpu_blocks;                  /* # CPU blocks */
  ub8            gpu_blocks;                  /* # GPU blocks */
  ub8            shared_blocks;               /* # Shared blocks in the set */
  struct cache_block_t *blocks;               /* Actual blocks */
}srriphint_data;

/* SRRIP cache-wide data */
typedef struct cache_policy_srriphint_gdata_t
{
  ub4    ways;                                /* Associativity */
  ub8    cache_access;                        /* # access count */
  ub8    stream_blocks[TST + 1];              /* # per-stream block count */   
  ub8    stream_reuse[TST + 1];               /* # per-stream reuse */   
  ub4    cpu_rpsel;                           /* CPU counter used to choose CPU or GPU first at replacement */
  ub4    gpu_rpsel;                           /* GPU counter used to choose CPU or GPU first at replacement */
  ub4    cpu_rpthr;                           /* CPU threshold used to choose CPU or GPU first at replacement */
  ub4    gpu_rpthr;                           /* GPU threshold  used to choose CPU or GPU first at replacement */
  ub4    cpu_zevct;                           /* CPU counter used to choose CPU or GPU first at replacement */
  ub4    gpu_zevct;                           /* GPU counter used to choose CPU or GPU first at replacement */
  ub4    sign_size;                           /* # Ship signature size */
  ub4    shct_size;                           /* # Counter table entries */
  ub4    core_size;                           /* # Core count */
  ub1   sign_source;                          /* Source of sign (PC / MEM / PC + MEM) */
  ub8    ship_access;                         /* # Access to ship sampler */
  ub8    ship_inc;                            /* # Access to ship sampler */
  ub8    ship_dec;                            /* # Access to ship sampler */
  ub1   *ship_shct;                           /* SHiP signature counter table */
  ub1    threshold;                           /* BRRIP threshold */
  ub1   *brrip_ctr;                           /* BRRIP counter used for ship */
  ub8    cpu_blocks;                          /* CPU blocks in cache */
  ub8    gpu_blocks;                          /* GPU blocks in cache */
  ub1    cpu_bmc;                             /* CPU bimodal fill counter */
  ub8    dead_region;                         /* # replacement find dead region */
  sctr   rpl_ctr;                             /* Replacement policy selection counter */
  sctr   fill_ctr;                            /* Fill policy selection counter */
  gzFile *shared_pc_file;                     /* Trace file for shared PC */
  shnt_sampler_cache *sampler;                /* Sampler cache used for tracking reuses */
  shnt_sampler_cache *cpu_sampler;            /* Sampler cache used for tracking reuses */
  sdbp_sampler *dbp_sampler;                  /* SDBP sampler */
}srriphint_gdata;

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

void cache_init_srriphint(ub4 set_indx, struct cache_params *params, 
    srriphint_data *policy_data, srriphint_gdata *global_data);

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
 *  policy_data (IN)  - Set to be freed 
 *  global_data (IN)  - Cache-wide data
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_free_srriphint(ub4 set_indx, srriphint_data *policy_data, 
    srriphint_gdata *global_data);

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
 *  tag         (IN)  - Block tag
 *
 * RETURNS
 *  
 *  Pointer to the block 
 */

struct cache_block_t * cache_find_block_srriphint(srriphint_data *policy_data, 
    srriphint_gdata *global_data, memory_trace *info, long long tag);

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

void cache_fill_block_srriphint(srriphint_data *policy_data, srriphint_gdata *global_data,
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
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Way id from where to replace the block
 *
 */

int  cache_replace_block_srriphint(srriphint_data *policy_data, 
    srriphint_gdata *global_data, memory_trace *info);

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

void cache_access_block_srriphint(srriphint_data *policy_data, 
    srriphint_gdata *global_data, int way, int strm, memory_trace *info);

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

struct cache_block_t cache_get_block_srriphint(srriphint_data *policy_data, int way, 
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

void cache_set_block_srriphint(srriphint_data *policy_data, int way, long long tag,
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

int cache_get_fill_rrpv_srriphint(srriphint_data *policy_data, 
    srriphint_gdata *global_data, memory_trace *info, ub4 epoch);

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

int cache_get_replacement_rrpv_srriphint(srriphint_data *policy_data);

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
 *  old_rrpv    (IN) - Current RRPV of the block
 *  block       (IN) - Cache block
 *  epoch       (IN) - Current block epoch
 *
 * RETURNS
 *  
 *  New RRPV
 *
 */

int cache_get_new_rrpv_srriphint(srriphint_data *policy_data, srriphint_gdata *global_data, 
    memory_trace *info, sb4 old_rrpv, struct cache_block_t *block, ub4 epoch);

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

int cache_count_block_srriphint(srriphint_data *policy_data, ub1 strm);

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

void cache_set_current_fill_policy_srriphint(srriphint_data *policy_data, cache_policy_t policy);

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

void cache_set_current_access_policy_srriphint(srriphint_data *policy_data, cache_policy_t policy);

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

void cache_set_current_replacement_policy_srriphint(srriphint_data *policy_data, cache_policy_t policy);

#endif	/* MEM_SYSTEM_CACHE_H */
