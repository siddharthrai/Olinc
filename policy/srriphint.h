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
#include "policy.h"
#include "rrip.h"
#include "brrip.h"
#include "cache-param.h"
#include "cache-block.h"

/* For the time being only 5 cores are supported. */
#define REPOCH_CNT    (1)
#define MIN_EPOCH     (0)
#define MAX_EPOCH     (3)
#define EPOCH_COUNT   (MAX_EPOCH - MIN_EPOCH + 1)

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
#define SMPLRPERF_FRRPV(p, s)         ((p)->stream_fill_rrpv[s])
#define SMPLRPERF_HRRPV(p, s)         ((p)->stream_hit_rrpv[s])

typedef struct sh_sampler_perfctr
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
}sh_sampler_perfctr;

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

typedef struct sh_sampler_entry
{
  ub8  page;          /* Tag: page id */
  ub8 *timestamp;     /* Timestamp of last access */
  ub1 *spill_or_fill; /* Is the last access to the block a spill or a fill? */
  ub1 *stream;        /* Id of the last stream to access this block */
  ub1 *valid;         /* Valid or invalid */
  ub1 *hit_count;     /* Current reuse epoch id */
  ub1 *dynamic_color; /* True, if dynamic CS */
  ub1 *dynamic_depth; /* True, if dynamic ZS */
  ub1 *dynamic_blit;  /* True, if dynamic BS */
  ub1 *dynamic_proc;  /* True, if dynamic PS */
}sh_sampler_entry;

/*
 * Sampler used for measuring reuses
 *
 */

typedef struct sh_sampler_cache
{
  ub4                sets;                     /* # sampler sets */
  ub1                ways;                     /* # sampler ways */
  ub1                entry_size;               /* Sampler entry size */
  ub1                log_grain_size;           /* Log of sampler entry size */
  ub1                log_block_size;           /* Log of sampler entry size */
  ub8                epoch_length;             /* Length of sampler epoch */
  ub8                epoch_count;              /* Total epochs seen by the sampler */
  ub4                stream_occupancy[TST + 1];/* Block array */
  sh_sampler_entry **blocks;                   /* Block array */
  sh_sampler_perfctr perfctr;                  /* Performance counter used in sampler */
}sh_sampler_cache;

/* Streams specific to SAP. SAP controller remaps global stream id to SAP 
 * specific ids */
typedef enum srriphint_stream
{
  srriphint_stream_x = 1,
  srriphint_stream_y,
  srriphint_stream_p,
  srriphint_stream_q,
  srriphint_stream_r
}srriphint_stream;

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_srriphint_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}srriphint_list;

#define SRRIPHINT_DATA_CFPOLICY(data)   ((data)->current_fill_policy)
#define SRRIPHINT_DATA_DFPOLICY(data)   ((data)->default_fill_policy)
#define SRRIPHINT_DATA_CAPOLICY(data)   ((data)->current_access_policy)
#define SRRIPHINT_DATA_DAPOLICY(data)   ((data)->default_access_policy)
#define SRRIPHINT_DATA_CRPOLICY(data)   ((data)->current_replacement_policy)
#define SRRIPHINT_DATA_DRPOLICY(data)   ((data)->default_replacement_policy)
#define SRRIPHINT_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define SRRIPHINT_DATA_SPILL_RRPV(data) ((data)->spill_rrpv)
#define SRRIPHINT_DATA_BLOCKS(data)     ((data)->blocks)
#define SRRIPHINT_DATA_VALID_HEAD(data) ((data)->valid_head)
#define SRRIPHINT_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define SRRIPHINT_DATA_FREE_HLST(data)  ((data)->free_head)
#define SRRIPHINT_DATA_FREE_TLST(data)  ((data)->free_tail)
#define SRRIPHINT_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define SRRIPHINT_DATA_FREE_TAIL(data)  ((data)->free_tail->head)
#define SRRIPHINT_DATA_USE_EPOCH(data)  ((data)->use_epoch)

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
  ub4            set_type;                    /* Set type */
  ub4            max_rrpv;                    /* Maximum RRPV supported */
  ub4            spill_rrpv;                  /* Spill RRPV supported */
  rrip_list     *valid_head;                  /* Head pointers of RRPV specific list */
  rrip_list     *valid_tail;                  /* Tail pointers of RRPV specific list */
  list_head_t   *free_head;                   /* Free list head */
  list_head_t   *free_tail;                   /* Free list tail */
  ub1            use_epoch;                   /* TRUE, if epoch is used in the policy */
  ub8            miss_count;                  /* # miss in a set */
  ub8            hit_count;                   /* # hits in a set */
  brrip_data     brrip;                       /* BRRIP policy specific data */
  struct cache_block_t *blocks;               /* Actual blocks */
}srriphint_data;

/* SRRIP cache-wide data */
typedef struct cache_policy_srriphint_gdata_t
{
  ub4    ways;                                /* Associativity */
  ub1    epoch_count;                         /* Total supported epochs */
  ub1    max_epoch;                           /* Maximum supported epochs */
  sctr **epoch_fctr;                          /* Epoch-wise fill counter */
  sctr **epoch_hctr;                          /* Epoch-wise hit counter */
  ub1   *epoch_valid;                         /* Valid epoch list */
  ub8    policy_fill;                         /* Fill in the policy sample */
  ub8    sample_fill;                         /* Fill in srrip sample */
  ub8    policy_miss;                         /* Miss in policy sample */
  ub8    reuse_miss;                          /* Miss in reuse sample */
  ub8    sample_miss;                         /* Miss in srrip sample */
  sctr   sarp_hint[TST + 1];                  /* Accumulation counter for per-stream speedup */
  ub1    speedup_stream[TST + 1];             /* True, if stream is sped-up */
  ub8    fill_no_reuse[TST + 1];              /* Eviction without reuse */   
  ub8    spill_no_reuse[TST + 1];             /* Eviction without reuse */   
  ub8    stream_fill[TST + 1];                /* # per-stream fill */   
  ub8    stream_spill[TST + 1];               /* # per-stream fill */   
  ub8    ct_reuse;                            /* # CT reuse blocks */
  ub8    bt_reuse;                            /* # BT reuse blocks */
  ub8    cache_access;                        /* # access count */
  ub8    current_fill_bucket;                 /* Current fill bucket number */
  ub8    zero_evict_bucket_bs[TST + 1];       /* # Zero eviction in current bucket */
  ub8    non_zero_evict_bucket_bs[TST + 1];   /* # Non-zero eviction in current bucket */
  ub8    zero_evict_bucket_ps[TST + 1];       /* # Zero eviction in current bucket */
  ub8    non_zero_evict_bucket_ps[TST + 1];   /* # Non-zero eviction in current bucket */
  ub8    x_zero_evict_bucket_bs[TST + 1];     /* # Zero eviction in current bucket */
  ub8    x_non_zero_evict_bucket_bs[TST + 1]; /* # Non-zero eviction in current bucket */
  ub8    x_zero_evict_bucket_ps[TST + 1];     /* # Zero eviction in current bucket */
  ub8    x_non_zero_evict_bucket_ps[TST + 1]; /* # Non-zero eviction in current bucket */
  ub8   *fill_recall_table[TST + 1];          /* # Fill block is recall table */ 
  ub1   *fill_recall_table_valid[TST + 1];    /* # Fill block is recall valid entry */ 
  ub8    total_fill_recall[TST + 1];          /* # Fills recalled */
  ub8   *spill_recall_table[TST + 1];         /* # Spill block is recall table */ 
  ub1   *spill_recall_table_valid[TST + 1];   /* # Spill block recall valid entry */ 
  ub8    total_spill_recall[TST + 1];         /* # Spill recalled */
  sh_sampler_cache *sampler;                  /* # Sampler cache for working-set sampling */
  brrip_gdata brrip;                          /* BRRIP specific cache-wide data */
  sctr   psel;                                /* Policy selection counter */
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
 *  policy_data (IN)  - Set to be freed 
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_free_srriphint(srriphint_data *policy_data);

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
    srriphint_gdata *global_data, long long tag);

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
