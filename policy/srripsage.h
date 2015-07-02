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

#ifndef MEM_SYSTEM_SRRIPSAGE_H
#define	MEM_SYSTEM_SRRIPSAGE_H

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "policy.h"
#include "rrip.h"
#include "srrip.h"
#include "cache-param.h"
#include "cache-block.h"

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_srripsage_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}srripsage_list;

#define SRRIPSAGE_DATA_CFPOLICY(data)   ((data)->current_fill_policy)
#define SRRIPSAGE_DATA_DFPOLICY(data)   ((data)->default_fill_policy)
#define SRRIPSAGE_DATA_CAPOLICY(data)   ((data)->current_access_policy)
#define SRRIPSAGE_DATA_DAPOLICY(data)   ((data)->default_access_policy)
#define SRRIPSAGE_DATA_CRPOLICY(data)   ((data)->current_replacement_policy)
#define SRRIPSAGE_DATA_DRPOLICY(data)   ((data)->default_replacement_policy)
#define SRRIPSAGE_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define SRRIPSAGE_DATA_SPILL_RRPV(data) ((data)->spill_rrpv)
#define SRRIPSAGE_DATA_BLOCKS(data)     ((data)->blocks)
#define SRRIPSAGE_DATA_VALID_HEAD(data) ((data)->valid_head)
#define SRRIPSAGE_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define SRRIPSAGE_DATA_FREE_HLST(data)  ((data)->free_head)
#define SRRIPSAGE_DATA_FREE_TLST(data)  ((data)->free_tail)
#define SRRIPSAGE_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define SRRIPSAGE_DATA_FREE_TAIL(data)  ((data)->free_tail->head)
#define SRRIPSAGE_DATA_RRPV_BLCKS(data) ((data)->rrpv_blocks)

/* RRIP specific data */
typedef struct cache_policy_srripsage_data_t
{
  cache_policy_t following;                   /* Policy set is following */
  ub4            max_rrpv;                    /* Maximum RRPV supported */
  ub4            threshold;                   /* Threshold */
  ub4            spill_rrpv;                  /* Spill RRPV supported */
  rrip_list     *valid_head;                  /* Head pointers of RRPV specific list */
  rrip_list     *valid_tail;                  /* Tail pointers of RRPV specific list */
  list_head_t   *free_head;                   /* Free list head */
  list_head_t   *free_tail;                   /* Free list tail */
  srrip_data     srrip_policy_data;           /* If set is following srrip */
  ub8            evictions;                   /* Total evictions in set */  
  ub8            last_eviction;               /* Last eviction before last hit */
  ub1            *rrpv_blocks;                /* #blocks at each RRPV */
  ub8            per_stream_fill[TST + 1];    /* Fills for each stream */
  ub1            hit_post_fill[TST + 1];      /* TRUE if there was a hit after fill */
  ub1            set_type;                    /* Type of set SAMPLER / FOLLOWER */
  ub1            fill_at_lru[TST + 1];        /* If set, fill at LRU */
  ub1            dem_to_lru[TST + 1];         /* If set, demote to LRU */
  struct cache_block_t *blocks;               /* Actual blocks */
}srripsage_data;

#define MAX_THR (4)

typedef struct cache_policy_srripsage_gdata_t
{
  ub8 bm_ctr;                                 /* Bimodal counter */
  ub8 bm_thr;                                 /* Bimodal threshold */
  ub4 active_stream;                          /* Current stream for threshold computation */
  ub4 lru_sample_set;                         /* # LRU sample sets */
  ub8 lru_sample_access[TST + 1];             /* # Access to LRU sample sets */
  ub8 lru_sample_hit[TST + 1];                /* # Hits to LRU sample sets */
  ub8 lru_sample_dem[TST + 1];                /* # Hits to LRU sample sets */
  ub4 mru_sample_set;                         /* # MRU sample sets */
  ub8 mru_sample_access[TST + 1];             /* # Access to MRU sample sets */
  ub8 mru_sample_hit[TST + 1];                /* # Hits to MRU sample sets */
  ub8 mru_sample_dem[TST + 1];                /* # Hits to MRU sample sets */
  ub4 thr_sample_set;                         /* # THR sample sets */
  ub8 thr_sample_access[TST + 1];             /* # Access to THR sample sets */
  ub8 thr_sample_hit[TST + 1];                /* # Hits to THR sample sets */
  ub8 thr_sample_dem[TST + 1];                /* # Hits to THR sample sets */
  ub8 per_stream_fill[TST + 1];               /* Fills for each stream */
  ub8 per_stream_hit[TST + 1];                /* Hits for each stream */
  ub8 per_stream_xevct[TST + 1];              /* Self evict for each stream */
  ub8 per_stream_sevct[TST + 1];              /* Self evict for each stream */
  ub8 per_stream_demote[4][TST + 1];          /* Demotion for each stream */
  ub8 per_stream_rrpv_hit[4][TST + 1];        /* Hits for each stream for each RRPV */
  ub8 access_count;                           /* Total accesses to cache */

  ub8 rcy_thr[MAX_THR][TST + 1];              /* Per-stream recency threshold */
  ub8 per_stream_reuse[TST + 1];              /* Reuse seen by blocks at RRPV 0 */
  ub8 per_stream_reuse_blocks[TST + 1];       /* Blocks at RRPV 0 */
  ub8 per_stream_max_reuse[TST + 1];          /* Reuse seen so far for each stream at RRPV 0 */
  ub8 per_stream_cur_thr[TST + 1];            /* Current threshold for each stream at RRPV 0*/
  ub8 occ_thr[MAX_THR][TST + 1];              /* Per-stream occupancy threshold */
  ub8 per_stream_oreuse[TST + 1];             /* Reuse seen by blocks at RRPV 0 */
  ub8 per_stream_oreuse_blocks[TST + 1];      /* Blocks at RRPV 2 */
  ub8 per_stream_max_oreuse[TST + 1];         /* Reuse seen so far for each stream at RRPV 2 */
  ub8 per_stream_occ_thr[TST + 1];            /* Current threshold for each stream at RRPV 2*/
  ub8 dem_thr[MAX_THR][TST + 1];              /* Per-stream occupancy threshold */
  ub8 per_stream_dreuse[TST + 1];             /* Reuse seen by demoted blocks */
  ub8 per_stream_dreuse_blocks[TST + 1];      /* Demoted blocks */
  ub8 per_stream_max_dreuse[TST + 1];         /* Reuse seen so far for each stream by demoted blocks */
  ub8 per_stream_dem_thr[TST + 1];            /* Current threshold for each stream */
  ub1 fill_at_head[TST + 1];                  /* True if block is to be filled at the head of the arrival list */
  ub1 demote_at_head[TST + 1];                /* True if block is to be demoted at the head of the arrival list */
  ub1 demote_on_hit[TST + 1];                 /* True if block is to be filled at the head of the arrival list */
  ub8 *rrpv_blocks;                           /* #blocks at each RRPV */
  ub8 fills_at_head[TST + 1];                 /* # block filled at the head of the arrival list */
  ub8 fills_at_tail[TST + 1];                 /* # block filled at the tail of the arrival list */
  ub8 dems_at_head[TST + 1];                  /* True if block is to be filled at the head of the arrival list */
  ub8 dems_at_tail[TST + 1];                  /* True if block is to be filled at the head of the arrival list */
  ub8 fail_demotion[TST + 1];                 /* Premature demotions */
  ub8 valid_demotion[TST + 1];                /* Expected demotions */

  /* Eight counter to be used for SRRIPDBP reuse probability learning */
  struct saturating_counter_t tex_e0_fill_ctr;    /* Texture epoch 0 fill */
  struct saturating_counter_t tex_e0_hit_ctr;     /* Texture epoch 0 hits */
  struct saturating_counter_t tex_e1_fill_ctr;    /* Texture epoch 1 fill */
  struct saturating_counter_t tex_e1_hit_ctr;     /* Texture epoch 1 hits */
  struct saturating_counter_t acc_all_ctr;        /* Total accesses */

  struct saturating_counter_t fath_ctr[TST + 1];  /* Per-stream dueling Counter for fills */
  struct saturating_counter_t fathm_ctr[TST + 1]; /* Per-stream dueling Counter for fills */
  struct saturating_counter_t dem_ctr[TST + 1];   /* Per-stream dueling Counter for demotions */
  struct saturating_counter_t demm_ctr[TST + 1];  /* Per-stream dueling Counter for demotions */
  struct saturating_counter_t cb_ctr;             /* CB reuse counter */
  struct saturating_counter_t bc_ctr;             /* CB reuse counter */
  struct saturating_counter_t ct_ctr;             /* CT reuse counter */
  struct saturating_counter_t bt_ctr;             /* BT reuse counter */
  struct saturating_counter_t tb_ctr;             /* TB reuse counter */
  struct saturating_counter_t zt_ctr;             /* ZT reuse counter */

  struct saturating_counter_t gfath_ctr;          /* Global dueling Counter for fills */
  struct saturating_counter_t gfathm_ctr;         /* Global dueling Counter for misses */
  struct saturating_counter_t gdem_ctr;           /* Global dueling Counter for demotions */

  struct saturating_counter_t fill_list_fctr[TST + 1];  /* Fill-list fill counter */
  struct saturating_counter_t fill_list_hctr[TST + 1];  /* Fill-list hit counter */

  srrip_gdata srrip;                              /* SRRIP global data */
}srripsage_gdata;

#undef MAX_THR

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

void cache_init_srripsage(int set_indx, struct cache_params *params, 
    srripsage_data *policy_data, srripsage_gdata *global_data);

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

void cache_free_srripsage(srripsage_data *policy_data);

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
 *  global_data (IN)  - Cache wide policy data
 *  tag         (IN)  - Block tag
 *  strm        (IN)  - Access stream
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Pointer to the block 
 */

struct cache_block_t * cache_find_block_srripsage(srripsage_data *policy_data, 
    srripsage_gdata *global_data, long long tag, int strm, memory_trace *info);

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

void cache_fill_block_srripsage(srripsage_data *policy_data, 
    srripsage_gdata *global_data, int way, long long tag, 
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
 *  global_data (IN)  - Cache-wide policy data
 *
 * RETURNS
 *  
 *  Way id from where to replace the block
 *
 */

int  cache_replace_block_srripsage(srripsage_data *policy_data, 
    srripsage_gdata *global_data, memory_trace *info);

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

void cache_access_block_srripsage(srripsage_data *policy_data, 
    srripsage_gdata *global_data, int way, int strm, memory_trace *info);

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

struct cache_block_t cache_get_block_srripsage(srripsage_data *policy_data, int way, 
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

void cache_set_block_srripsage(srripsage_data *policy_data, int way, long long tag,
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

int cache_get_fill_rrpv_srripsage(srripsage_data *policy_data, 
    srripsage_gdata *global_data, memory_trace *info);

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

int cache_get_replacement_rrpv_srripsage(srripsage_data *policy_data);

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

int cache_get_new_rrpv_srripsage(srripsage_data *policy_data, 
    srripsage_gdata *global_data, memory_trace *info, int way,
    int old_rrpv, unsigned long long old_recency);

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

int cache_count_block_srripsage(srripsage_data *policy_data, ub1 strm);

ub1 cache_override_fill_at_head(srripsage_data *policy_data, 
    srripsage_gdata *global_data, memory_trace *info);
#endif	/* MEM_SYSTEM_CACHE_H */
