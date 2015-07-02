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

#ifndef MEM_SYSTEM_GSHP_H
#define	MEM_SYSTEM_GSHP_H

#ifdef __cplusplus
# define EXPORT_C extern "C"
#else
# define EXPORT_C
#endif

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "policy.h"
#include "cache-block.h"
#include "srrip.h"

/* Head node of a list, which corresponds to a particular RRPV */
typedef struct cache_list_head_gshp_t
{
  ub4 rrpv;
  struct cache_block_t *head;
}gshp_list;

#define GSHP_DATA_MAX_RRPV(data)   ((data)->max_rrpv)
#define GSHP_DATA_BLOCKS(data)     ((data)->blocks)
#define GSHP_DATA_VALID_HEAD(data) ((data)->valid_head)
#define GSHP_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define GSHP_DATA_FREE_HEAD(data)  ((data)->free_head)
#define GSHP_DATA_FREE_TAIL(data)  ((data)->free_tail)

/* RRIP specific data */
typedef struct cache_policy_gshp_data_t
{
  cache_policy_t  following;                    /* Policy set is following */

  ub4        max_rrpv;                          /* Maximum RRPV */
  ub4        threshold;                         /* Eviction threshold */
  gshp_list *valid_head;                        /* Valid block head */
  gshp_list *valid_tail;                        /* Valid block tail */

  struct cache_block_t *blocks;                 /* Actual blocks */
  struct cache_block_t *free_head;              /* Free list head */
  struct cache_block_t *free_tail;              /* Free list tail */

}gshp_data;

typedef struct cache_policy_gshp_gdata_t
{
  /* Eight counter to be used for GSHP reuse probability learning */
  struct saturating_counter_t tex_e0_fill_ctr;  /* Texture epoch 0 fill */
  struct saturating_counter_t tex_e0_hit_ctr;   /* Texture epoch 0 hits */
  struct saturating_counter_t tex_e1_fill_ctr;  /* Texture epoch 1 fill */
  struct saturating_counter_t tex_e1_hit_ctr;   /* Texture epoch 1 hits */
  struct saturating_counter_t z_e0_fill_ctr;    /* Depth fill */
  struct saturating_counter_t z_e0_hit_ctr;     /* Depth hits */
  struct saturating_counter_t rt_prod_ctr;      /* Render target produced */
  struct saturating_counter_t rt_cons_ctr;      /* Render target consumed */
  struct saturating_counter_t acc_all_ctr;      /* Total accesses */
  srrip_gdata srrip;
}gshp_gdata;

/* GSHP cache management handlers */
void cache_fill_block_gshp(gshp_data *policy_data, gshp_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int strm, 
  memory_trace *info);

int  cache_replace_block_gshp(gshp_data *policy_data, gshp_gdata *global_data);

void cache_access_block_gshp(gshp_data *policy_data, gshp_gdata *global_data, 
  int way, int strm, memory_trace *info);

struct cache_block_t * cache_find_block_gshp(gshp_data *policy_data, long long tag);

int cache_get_fill_rrpv_gshp(gshp_data *policy_data, gshp_gdata *global_data,
  int strm, memory_trace *info);

int cache_get_replacement_rrpv_gshp(gshp_data *policy_data);

int cache_get_new_rrpv_gshp(gshp_data *policy_data, gshp_gdata *global_data, 
  int way, int strm);

void cache_update_fill_counter_gshp(srrip_data *policy_data, gshp_gdata *global_data,
  int way, int strm);

void cache_update_hit_counter_gshp(srrip_data *policy_data, gshp_gdata *global_data, 
  int way, int strm);

void cache_init_gshp(long long int set_indx, struct cache_params *params, 
  gshp_data *policy_data, gshp_gdata *global_data);

void cache_free_gshp(gshp_data *policy_data);

struct cache_block_t cache_get_block_gshp(gshp_data *policy_data, gshp_gdata *global_data, int way, 
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr);

void cache_set_block_gshp(gshp_data *policy_data, gshp_gdata *global_data, int way, 
  long long tag, enum cache_block_state_t state, ub1 stream, memory_trace *info);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
