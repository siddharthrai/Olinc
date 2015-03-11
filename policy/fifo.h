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
#ifndef MEM_SYSTEM_FIFO_H
#define	MEM_SYSTEM_FIFO_H

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

#define FIFO_DATA_BLOCKS(data)     (data->blocks)
#define FIFO_DATA_VALID_HEAD(data) (data->valid_head)
#define FIFO_DATA_VALID_TAIL(data) (data->valid_tail)
#define FIFO_DATA_FREE_HEAD(data)  (data->free_head)
#define FIFO_DATA_FREE_TAIL(data)  (data->free_tail)

/* FIFO specific data */
typedef struct cache_policy_fifo_data_t
{
  struct cache_block_t *valid_head; /* Head pointers */
  struct cache_block_t *valid_tail; /* Tail pointers */
  struct cache_block_t *blocks;     /* Actual blocks */
  struct cache_block_t *free_head;  /* Free list head */
  struct cache_block_t *free_tail;  /* Free list tail */

}fifo_data;

/* FIFO specific handlers */
void cache_init_fifo(struct cache_params *params, fifo_data *policy_data);

void cache_free_fifo(fifo_data *policy_data);

struct cache_block_t * cache_find_block_fifo(fifo_data *policy_data,
  long long tag);

void cache_fill_block_fifo(fifo_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info);

int  cache_replace_block_fifo(fifo_data *policy_data);

void cache_access_block_fifo(fifo_data *policy_data, int way, int strm, 
  memory_trace *info);

void cache_set_block_fifo(fifo_data *policy_data, int way, long long tag, 
  enum cache_block_state_t state, ub1 stream, memory_trace *info);

struct cache_block_t cache_get_block_fifo(fifo_data *policy_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *stream_ptr);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
