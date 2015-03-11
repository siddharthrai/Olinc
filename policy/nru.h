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

#ifndef MEM_SYSTEM_NRU_H
#define	MEM_SYSTEM_NRU_H

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

#define NRU_DATA_WAYS(data)   ((data)->ways)
#define NRU_DATA_BLOCKS(data) ((data)->blocks)

/* NRU specific data */
typedef struct cache_policy_nru_data_t
{
  ub4    ways;                      /* Ways in a set */
  struct cache_block_t *blocks;     /* Actual blocks */
}nru_data;

void cache_init_nru(struct cache_params *params, nru_data *policy_data);

void cache_free_nru(nru_data *policy_data);

/* NRU specific handlers */
struct cache_block_t * cache_find_block_nru(nru_data *policy_data, long long tag);

void cache_fill_block_nru(nru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info);

int  cache_replace_block_nru(nru_data *policy_data);

void cache_access_block_nru(nru_data *policy_data, int way, int strm, 
  memory_trace *info);

void cache_set_block_nru(nru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info);

struct cache_block_t cache_get_block_nru(nru_data *policy_data, int way, 
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
