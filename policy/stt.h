/*
 * Copyright (C) 2014 Siddharth Rai
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

/*
 * NAME
 *
 * DESCRIPTION
 *
 * PUBLIC FUNCTIONS
 *
 * PRIVATE FUNCTIONS
 */

#ifndef STT_H
#define STT_H

#include "../cache/cache.h"

#if 0
typedef struct pc_stall_data
{
  ub8 eip;          /* Instruction pointer */
  ub8 cumu_stall;   /* Cumulative RPB head stall */
  ub8 hit_count;    /* Hit count */
  ub8 access_count; /* Total access count */
}pc_stall_data;
#endif
typedef struct stall_table
{
  struct  cache_t *pc_lru_list;       /* LRU list of PC */
  ub8     total_l2_miss;              /* Total L2 misses, not reset periodically. */
  ub8     total_llc_access;           /* Total LLC access, not reset periodically. */
  ub8     total_llc_miss;             /* Total LLC miss, not reset periodically. */
  ub8     l2_miss;                    /* LLC access, used for PC classification */
  ub8     llc_access;                 /* Total LLC access */
  ub8     llc_miss;                   /* Total LLC miss */
  ub8     pc_table_hits;              /* Hits seen by PC table */
  ub8     pc_table_access;            /* Access to PC table */
  ub8     pc_table_replacement;       /* Replacements to PC table */
  ub8     total_stall_cycles;         /* Total ROB stalls */
  ub8     stat_l2_miss;               /* LLC access, used for PC classification */
  ub8     stat_llc_access;            /* Total LLC access */
  ub8     stat_llc_miss;              /* Total LLC miss */
  ub8     stat_pc_table_hits;         /* Hits seen by PC table */
  ub8     stat_pc_table_access;       /* Access to PC table */
  ub8     stat_pc_table_replacement;  /* Replacements to PC table */
  ub8     stat_total_stall_cycles;    /* Total ROB stalls */
}stall_table;

#endif
