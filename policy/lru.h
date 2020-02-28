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

#ifndef MEM_SYSTEM_LRU_H
#define	MEM_SYSTEM_LRU_H

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

#define LRU_DATA_BLOCKS(data)     ((data)->blocks)
#define LRU_DATA_VALID_HLST(data) ((data)->valid_head)
#define LRU_DATA_VALID_TLST(data) ((data)->valid_tail)
#define LRU_DATA_FREE_HLST(data)  ((data)->free_head)
#define LRU_DATA_FREE_TLST(data)  ((data)->free_tail)
#define LRU_DATA_VALID_HEAD(data) ((data)->valid_head->head)
#define LRU_DATA_VALID_TAIL(data) ((data)->valid_tail->head)
#define LRU_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define LRU_DATA_FREE_TAIL(data)  ((data)->free_tail->head)

/* LRU specific data */
typedef struct cache_policy_lru_data_t
{
  list_head_t *valid_head;      /* Head pointers */
  list_head_t *valid_tail;      /* Tail pointers */
  list_head_t *free_head;       /* Free list head */
  list_head_t *free_tail;       /* Free list tail */

  struct cache_block_t *blocks; /* Actual blocks */
}lru_data;

void cache_init_lru(struct cache_params *params, lru_data *policy_data);

void cache_free_lru(lru_data *policy_data);

/* LRU specific handlers */
struct cache_block_t * cache_find_block_lru(lru_data *policy_data, long long tag);

void cache_fill_block_lru(lru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info);

int  cache_replace_block_lru(lru_data *policy_data, memory_trace *info);

void cache_access_block_lru(lru_data *policy_data, int way, int strm, 
  memory_trace *info);

void cache_set_block_lru(lru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);

struct cache_block_t cache_get_block_lru(lru_data *policy_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *sream_ptr);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
