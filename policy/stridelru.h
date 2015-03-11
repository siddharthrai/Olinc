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

#ifndef MEM_SYSTEM_STRIDELRU_H
#define	MEM_SYSTEM_STRIDELRU_H

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

#define USAGE_PROD (1)
#define USAGE_CONS (2)

#define STRIDELRU_DATA_BLOCKS(data)     ((data)->blocks)
#define STRIDELRU_DATA_VALID_HLST(data) ((data)->valid_head)
#define STRIDELRU_DATA_VALID_TLST(data) ((data)->valid_tail)
#define STRIDELRU_DATA_FREE_HLST(data)  ((data)->free_head)
#define STRIDELRU_DATA_FREE_TLST(data)  ((data)->free_tail)
#define STRIDELRU_DATA_VALID_HEAD(data) ((data)->valid_head->head)
#define STRIDELRU_DATA_VALID_TAIL(data) ((data)->valid_tail->head)
#define STRIDELRU_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define STRIDELRU_DATA_FREE_TAIL(data)  ((data)->free_tail->head)

/* LRU specific data */
typedef struct cache_policy_stridelru_data_t
{
  list_head_t *valid_head;      /* Head pointers */
  list_head_t *valid_tail;      /* Tail pointers */
  list_head_t *free_head;       /* Free list head */
  list_head_t *free_tail;       /* Free list tail */
  
  ub1 stream;                   /* Stream that fills the regions */
  ub1 ways;                     /* Number of ways in a set */
  ub1 out_of_reach;             /* Counter for tracking regions out of range */
  ub1 recycle_blocks;           /* Flag set if blocks are to be recycled */
  ub1 rpl_on_miss;              /* If true, block is replaced on miss */

  struct cache_block_t *blocks; /* Actual blocks */
}stridelru_data;

void cache_init_stridelru(struct cache_params *params, stridelru_data *policy_data);

void cache_free_stridelru(stridelru_data *policy_data);

/* stridelru specific handlers */
struct cache_block_t * cache_find_block_stridelru(stridelru_data *policy_data, 
    long long tag, memory_trace *info);

void cache_fill_block_stridelru(stridelru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info);

int  cache_replace_block_stridelru(stridelru_data *policy_data);

void cache_access_block_stridelru(stridelru_data *policy_data, int way, int strm, 
  memory_trace *info);

void cache_set_block_stridelru(stridelru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);

struct cache_block_t cache_get_block_stridelru(stridelru_data *policy_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *sream_ptr);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
