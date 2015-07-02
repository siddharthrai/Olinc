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

#ifndef MEM_SYSTEM_SAPPRIORITY_H
#define	MEM_SYSTEM_SAPPRIORITY_H

#ifdef __cplusplus
# define EXPORT_C extern "C"
#else
# define EXPORT_C
#endif

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include "policy.h"
#include "rrip.h"
#include "cache-param.h"
#include "cache-block.h"
#include "srrip.h"
#include "brrip.h"
#include "sap.h"
#include "sdp.h"
#include <stdio.h>

#define SIGNSIZE    (14)
#define REGIONSIZE  (14)

/* Streams specific to SAPPRIORITY. SAPPRIORITY controller remaps global stream id to SAPPRIORITY 
 * specific ids */
typedef enum sappriority_stream
{
  sappriority_stream_u = 0,
  sappriority_stream_x,
  sappriority_stream_y,
  sappriority_stream_p,
  sappriority_stream_q,
  sappriority_stream_r
}sappriority_stream;

/* SAPPRIORITY statistics */
typedef struct cache_policy_sappriority_stats_t
{
  FILE *sappriority_stat_file;        /* Stat file */
  ub8   sappriority_srrip_samples;    /* Samples used for SRRIP */
  ub8   sappriority_brrip_samples;    /* Samples used for BRRIP */
  ub8   sample_srrip_fill;            /* Fills in SRRIP samples */
  ub8   sample_brrip_fill;            /* Fills in BRRIP samples */
  ub8   sample_ps_fill;               /* Fills in policy samples */
  ub8   per_stream_ps_fill[TST + 1];  /* Per-stream fills in policy samples */
  ub8   sample_srrip_hit;             /* Hits in SRRIP samples */
  ub8   sample_brrip_hit;             /* Hits in BRRIP samples */
  ub8   sample_ps_hit;                /* Hits in BRRIP samples */
  ub8   sappriority_srrip_fill;       /* Fills using SRRIP */
  ub8   sappriority_brrip_fill;       /* Fills using BRRIP */
  ub8   sappriority_fill_2;           /* SAPPRIORITY fill at RRPV 2 */
  ub8   sappriority_fill_3;           /* SAPPRIORITY fill at RRPV 3 */
  ub8   sappriority_fill[TST + 1];    /* SAPPRIORITY fill at RRPV 3 */
  ub8   sappriority_ps_fill[TST + 1]; /* Per-stream fill */
  ub8   sappriority_ps_thr[TST + 1];  /* Per-stream thrahing fills */
  ub8   sappriority_ps_dep[TST + 1];  /* Per-stream deprioritization */
  ub1   sappriority_hdr_printed;      /* True if header has been printed */
  ub8   epoch_count;                  /* #epochs during entire execution */
  ub8   speedup_epoch_count[TST + 1]; /* #epochs during entire execution */
  ub8   speedup_count[TST + 1];       /* #speedup */
  ub8   thrasher_count[TST + 1];      /* #thrasher */
  ub8   dead_blocks[TST + 1];         /* #thrasher */
  ub8   speedup_srrip_hit[TST + 1];   /* Hits in SRRIP samples for spedup stream */
  ub8   speedup_ps_hit[TST + 1];      /* Hits in BRRIP samples for spedup stream */
  ub8   speedup_srrip_thit[TST + 1];   /* Hits in SRRIP samples for spedup stream */
  ub8   speedup_ps_thit[TST + 1];      /* Hits in BRRIP samples for spedup stream */
  uf8   speedup_phbysh[TST + 1];      /* PH and SH Ratio */
  uf8   speedup_sphbysh;              /* PH and SH Ratio */
  ub8   next_schedule;                /* Cycle to schedule stat collection */
}sappriority_stats;

#define SAPPRIORITY_DATA_BLOCKS(data)     ((data)->blocks)
#define SAPPRIORITY_DATA_VALID_HEAD(data) ((data)->valid_head)
#define SAPPRIORITY_DATA_VALID_TAIL(data) ((data)->valid_tail)
#define SAPPRIORITY_DATA_FREE_HLST(data)  ((data)->free_head)
#define SAPPRIORITY_DATA_FREE_TLST(data)  ((data)->free_tail)
#define SAPPRIORITY_DATA_FREE_HEAD(data)  ((data)->free_head->head)
#define SAPPRIORITY_DATA_FREE_TAIL(data)  ((data)->free_tail->head)

/* SAPPRIORITY specific data */
typedef struct cache_policy_sappriority_data_t
{
  cache_policy_t        following;  /* Currently followed policy */
  ub4                   set_type;   /* Type of the set (sampled / follower)*/
  srrip_data            srrip;      /* SRRIP policy specific data */
  customsrrip_data      customsrrip;/* SRRIP policy specific data */
  brrip_data            brrip;      /* BRRIP policy specific data */
  rrip_list            *valid_head; /* Head pointers of RRPV specific list */
  rrip_list            *valid_tail; /* Tail pointers of RRPV specific list */
  list_head_t          *free_head;  /* Free list head */
  list_head_t          *free_tail;  /* Free list tail */
  struct cache_block_t *blocks;     /* Actual blocks */
}sappriority_data;

typedef struct ct_table
{
  ub1 c_bit;    /* True, if touched by color block */
  ub1 t_bit;    /* True, if touched by texture block */
  ub1 ct_bit;   /* True, if touched by texture block */
  ub1 counted;  /* True if region is already counted */
}ctt;

/* Policy global data */
typedef struct cache_policy_sappriority_gdata_t
{
  sctr              sappriority_ssel[TST + 1];  /* Selection counter for modified policy */
  sctr              drrip_psel;                 /* Policy selection counter */
  sctr              ps_psel;                    /* Policy selection counter */
  sctr              sappriority_psel;           /* Per-stream policy selection counter */
  sappriority_stats stats;                      /* SAPPRIORITY statistics */
  brrip_gdata       brrip;                      /* BRRIP cache-wide data */
  customsrrip_gdata customsrrip;                /* CUSTOMSRRIP cache-wide data */
  srrip_gdata       srrip;                      /* SRRIP cache-wide data */
  sdp_gdata         sdp;                        /* SDP cache wide data for SAP like stats */
  sctr              sappriority_hint[TST + 1];  /* Accumulation counter for per-stream speedup */
  ub8               dbp_sample_fill;            /* Fills in dbp sample */
  ub8               ct_sample_fill;             /* Fills in dbp sample */
  ub8               bt_sample_fill;             /* Fills in dbp sample */
  ub8               dbp_threshold;              /* Fills in dbp sample */
  ub8               per_stream_ct_fill[TST + 1];/* Per-stream fills in policy samples */

  sctr **epoch_fctr;                            /* Per stream epoch fill counter */
  sctr **epoch_hctr;                            /* Per-stream epoch hit counter */
  sctr *epoch_ctctr;                            /* Inter-stream hit counter */
  sctr *epoch_btctr;                            /* Inter-stream hit counter */

  ctt  *ctreuse_table;                          /* Color to texture reuse table */
  ub8   c_count;                                /* #C reuse */
  ub8   t_count;                                /* #T reuse */
  ub8   ct_count;                               /* #CT reuse */
  ub8   used_entries;                           /* #entries used */

  ub8   follower_fctr[TST + 1];                 /* Per stream follower fill counter */
  ub8   follower_hctr[TST + 1];                 /* Per stream follower hit counter */
  ub1  *epoch_valid;                            /* Per-stream epoch hit counter */
  sctr **epoch_xevict;                          /* Per-stream epoch xstream evictions */
  ub1  *epoch_thrasher;                         /* Per-stream thrashing stream */
  ub1  *thrasher_stream;                        /* TRUE, if stream is thrashing */
  ub1   speedup_stream;                         /* Stream to be sped up */
  ub8   access_interval;                        /* #Accesses */
  ub1   bs_epoch;                               /* TRUE, if only baseline samples are used for epoch */
}sappriority_gdata;

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

void cache_init_sappriority(long long int set_indx, struct cache_params *params, 
  sappriority_data *policy_data, sappriority_gdata *global_data);

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
 *  policy_data (IN)  - Per set policy data
 *  global_data (IN)  - Cache wide policy data
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_free_sappriority(unsigned int set_indx, sappriority_data *policy_data,
  sappriority_gdata *global_data);

/*
 *
 * NAME
 *  
 *  InitDrripStats - Initialize SAPPRIORITY stats
 *
 * DESCRIPTION
 *  
 *  Initialize SAPPRIORITY stats
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SAPPRIORITY statistics
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_init_sappriority_stats(sappriority_stats *stats);

/*
 *
 * NAME
 *  
 *  FiniDrripStats - Finalize SAPPRIORITY stats
 *
 * DESCRIPTION
 *  
 *  Finalize SAPPRIORITY stats
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SAPPRIORITY statistics
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_fini_sappriority_stats(sappriority_stats *stats);

/*
 *
 * NAME
 *  
 *  DumpDrripStats - Dump current stats 
 *
 * DESCRIPTION
 *  
 *  Dump current stats and reset counters
 *
 * PARAMETERS
 *  
 *  stats (IN)  - SAPPRIORITY statistics
 *  cycle (IN)  - Current cycle
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_dump_sappriority_stats(sappriority_stats *stats, ub8 cycle);

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
 *  info        (IN)  - Access info
 *  tag         (IN)  - Block tag
 *
 * RETURNS
 *  
 *  Pointer to the block 
 */

struct cache_block_t * cache_find_block_sappriority(sappriority_data *policy_data, 
    sappriority_gdata *global_data, memory_trace *info, long long tag);

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
 *  global_data (IN)  - Cache wide policy data
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

void cache_fill_block_sappriority(sappriority_data *policy_data, sappriority_gdata *global_data, 
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
 *  global_data (IN)  - Cache wide policy data
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Way id from where to replace the block
 *
 */

int  cache_replace_block_sappriority(sappriority_data *policy_data, 
    sappriority_gdata *global_data, memory_trace *info);

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
 *  global_data (IN)  - Cache wide policy data
 *  way         (IN)  - Physical way of the block
 *  strm        (IN)  - Stream accessing the block
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 *
 */

void cache_access_block_sappriority(sappriority_data *policy_data, sappriority_gdata *global_data,
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
 *  global_data (IN)  - Cache wide policy data
 *  way         (IN)  - Way of the block
 *  tag_ptr     (OUT) - Tag of the block
 *  state_ptr   (OUT) - Current state
 *
 * RETURNS
 *  
 *  Complete block info
 */

struct cache_block_t cache_get_block_sappriority(sappriority_data *policy_data, 
  sappriority_gdata *global_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *stream_ptr);

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
 *  global_data (IN)  - Cache wide policy data
 *  way         (IN)  - Way of the block
 *  tag         (IN)  - Block tag
 *  state       (IN)  - New state
 *  info        (IN)  - Access info
 *
 * RETURNS
 *  
 *  Nothing
 */

void cache_set_block_sappriority(sappriority_data *policy_data, sappriority_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info);

sappriority_stream get_sappriority_stream(memory_trace *info);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
