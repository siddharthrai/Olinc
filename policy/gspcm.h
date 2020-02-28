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

#ifndef MEM_SYSTEM_GSPCM_H
#define	MEM_SYSTEM_GSPCM_H

#ifdef __cplusplus
# define EXPORT_C extern "C"
#else
# define EXPORT_C
#endif

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "cache-block.h"
#include "policy.h"
#include "srripm.h"

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_gspcm_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}gspcm_list;

#define GSPCM_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define GSPCM_DATA_THRESHOLD(data)  ((data)->threshold)
#define GSPCM_DATA_BLOCKS(data)     ((data)->blocks)
#define GSPCM_DATA_VALID_HEAD(data) ((data)->valid_head)
#define GSPCM_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define GSPCM_DATA_FREE_HEAD(data)  ((data)->free_head)
#define GSPCM_DATA_FREE_TAIL(data)  ((data)->free_tail)

/* RRIP specific data */
typedef struct cache_policy_gspcm_data_t
{
  cache_policy_t  following;                    /* Policy set is following */

  ub4        max_rrpv;                          /* Maximum RRPV */
  ub4        threshold;                         /* Eviction threshold */
  gspcm_list *valid_head;                        /* Valid block head */
  gspcm_list *valid_tail;                        /* Valid block tail */

  struct cache_block_t *blocks;                 /* Actual blocks */
  struct cache_block_t *free_head;              /* Free list head */
  struct cache_block_t *free_tail;              /* Free list tail */
  
  srripm_data srrip_policy_data;                /* If set is following srripm */
}gspcm_data;

typedef struct cache_policy_gspcm_gdata_t
{
  /* Eight counter to be used for GSPC reuse probability learning */
  struct saturating_counter_t tex_e0_fill_ctr;              /* Texture epoch 0 fill */
  struct saturating_counter_t tex_e0_hit_ctr;               /* Texture epoch 0 hits */
  struct saturating_counter_t tex_e1_fill_ctr;              /* Texture epoch 1 fill */
  struct saturating_counter_t tex_e1_hit_ctr;               /* Texture epoch 1 hits */
  struct saturating_counter_t z_e0_fill_ctr;                /* Depth fill */
  struct saturating_counter_t z_e0_hit_ctr;                 /* Depth hits */
  struct saturating_counter_t rt_prod_ctr;                  /* Render target produced */
  struct saturating_counter_t rt_cons_ctr;                  /* Render target consumed */
  struct saturating_counter_t bt_prod_ctr;                  /* Render target produced */
  struct saturating_counter_t bt_cons_ctr;                  /* Render target consumed */
  struct saturating_counter_t acc_all_ctr;                  /* Total accesses */
  struct saturating_counter_t blk_fill[TST + MAX_CORES];    /* Filled blocks */
  struct saturating_counter_t blk_no_reuse[TST + MAX_CORES];/* Block not reused */
  struct saturating_counter_t blk_reuse[TST + MAX_CORES];   /* Block reused */
  ub8    total_blocks[TST + MAX_CORES];                     /* Blocks in sampled sets */
  ub8    total_access[TST + MAX_CORES];                     /* Accesses */
  ub8    prev_bt_prod;                                      /* Previous BT counter */
  ub8    prev_bt_cons;                                      /* Previous BT counter */
}gspcm_gdata;

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

void cache_init_gspcm(long long int set_indx, struct cache_params *params, 
  gspcm_data *policy_data, gspcm_gdata *global_data);

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

void cache_free_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data);

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

struct cache_block_t * cache_find_block_gspcm(gspcm_data *policy_data, long long tag);

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

void cache_fill_block_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data, 
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

int  cache_replace_block_gspcm(gspcm_data *policy_data, 
    gspcm_gdata *global_data, memory_trace *info);

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

void cache_access_block_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data,
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

struct cache_block_t cache_get_block_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data, 
  int way, long long *tag_ptr, enum cache_block_state_t *state_ptr, 
  int *stream);

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

void cache_set_block_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream,
  memory_trace *info);

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

int cache_get_fill_rrpv_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data, int strm);

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

int cache_get_replacement_rrpv_gspcm(gspcm_data *policy_data);

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

int cache_get_new_rrpv_gspcm(gspcm_data *policy_data, gspcm_gdata *global_data, 
  int way, int strm);

/*
 *
 * NAME
 *  
 *  CacheUpdateFillCounter
 *
 * DESCRIPTION
 *  
 *  Update GSPC spesific fill counters 
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Cache Set
 *  global_data (OUT) - GSPC specific cache wide data
 *  way         (IN)  - Block way
 *  strm        (IN)  - Access stream
 *
 * RETURNS
 *  Nothing
 */

void cache_update_fill_counter_gspcm(srripm_data *policy_data, 
  gspcm_gdata *global_data, int way, int strm);

/*
 *
 * NAME
 *  
 *  CacheUpdateHitCounter
 *
 * DESCRIPTION
 *  
 *  Update GSPC specific hit counter 
 *
 * PARAMETERS
 *  
 *  policy_data (IN)  - Cache Set
 *  global_data (OUT) - GSPC specific cache wide data
 *  way         (IN)  - Block way
 *  strm        (IN)  - Access stream
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_update_hit_counter_gspcm(srripm_data *policy_data,
  gspcm_gdata *global_data, int way, int strm);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
