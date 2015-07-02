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

#include <stdlib.h>
#include <assert.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "stt.h"
#include "../common/intermod-common.h"

#include "cache.h"
#include "cache-block.h"
#include "sap.h"
#include "zlib.h"

#define CACHE_UPDATE_BLOCK_STATE(block, tag, state_in)        \
do                                                            \
{                                                             \
  (block)->tag   = (tag);                                     \
  (block)->state = (state_in);                                \
}while(0)

#define CACHE_UPDATE_BLOCK_SSTREAM(block, strm)               \
do                                                            \
{                                                             \
  (block)->sap_stream = strm;                                 \
}while(0)

#define CACHE_UPDATE_BLOCK_STREAM(block, strm)                \
do                                                            \
{                                                             \
  (block)->stream = strm;                                     \
}while(0)

#define CACHE_UPDATE_BLOCK_PID(block, pid_in)                 \
do                                                            \
{                                                             \
  (block)->pid = pid_in;                                      \
}while(0)

#define CACHE_SAP_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)    \
do                                                            \
{                                                             \
    int dif = 0;                                              \
                                                              \
    for (int i = rrpv - 1; i >= 0; i--)                       \
    {                                                         \
      assert(i <= rrpv);                                      \
                                                              \
      if ((head_ptr)[i].head)                                 \
      {                                                       \
        if (!dif)                                             \
        {                                                     \
          dif = rrpv - i;                                     \
        }                                                     \
                                                              \
        assert((tail_ptr)[i].head);                           \
                                                              \
        (head_ptr)[i + dif].head  = (head_ptr)[i].head;       \
        (tail_ptr)[i + dif].head  = (tail_ptr)[i].head;       \
        (head_ptr)[i].head        = NULL;                     \
        (tail_ptr)[i].head        = NULL;                     \
                                                              \
        struct cache_block_t *node = (head_ptr)[i + dif].head;\
                                                              \
        /* point data to new RRPV head */                     \
        for (; node; node = node->prev)                       \
        {                                                     \
          node->data = &(head_ptr[i + dif]);                  \
        }                                                     \
      }                                                       \
      else                                                    \
      {                                                       \
        assert(!((tail_ptr)[i].head));                        \
      }                                                       \
    }                                                         \
}while(0)

/*
 * Private Functions
 */
#define free_list_remove_block(set, blk)                                                \
do                                                                                      \
{                                                                                       \
        /* Check: free list must be non empty as it contains the current block. */      \
        assert(SAP_DATA_FREE_HEAD(set) && SAP_DATA_FREE_TAIL(set));                     \
                                                                                        \
        /* Check: current block must be in invalid state */                             \
        assert((blk)->state == cache_block_invalid);                                    \
                                                                                        \
        CACHE_REMOVE_FROM_SQUEUE(blk, SAP_DATA_FREE_HEAD(set), SAP_DATA_FREE_TAIL(set));\
                                                                                        \
        (blk)->next = NULL;                                                             \
        (blk)->prev = NULL;                                                             \
                                                                                        \
        /* Reset block state */                                                         \
        (blk)->busy = 0;                                                                \
        (blk)->cached = 0;                                                              \
        (blk)->prefetch = 0;                                                            \
}while(0);

extern struct cpu_t *cpu;
extern ffifo_info ffifo_global_info;
ub1    sdp_base_sample_eval = FALSE;

static void cache_init_srrip_from_sap(srrip_data *policy_data, sap_data *sap_policy_data)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(sap_policy_data);

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV(data)  (SRRIP_DATA_MAX_RRPV(data))

  /* Create per rrpv buckets */
  SRRIP_DATA_VALID_HEAD(policy_data) = SAP_DATA_VALID_HEAD(sap_policy_data);
  SRRIP_DATA_VALID_TAIL(policy_data) = SAP_DATA_VALID_TAIL(sap_policy_data);
  
  assert(SRRIP_DATA_VALID_HEAD(policy_data));
  assert(SRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIP_DATA_MAX_RRPV(policy_data) = SAP_DATA_MAX_RRPV(sap_policy_data);

  /* Assign pointer to block array */
  SRRIP_DATA_BLOCKS(policy_data) = SAP_DATA_BLOCKS(sap_policy_data);

  /* Initialize array of blocks */
  SRRIP_DATA_FREE_HLST(policy_data) = SAP_DATA_FREE_HLST(sap_policy_data);
  SRRIP_DATA_FREE_TLST(policy_data) = SAP_DATA_FREE_TLST(sap_policy_data);

  /* Set current and default fill policy to SRRIP */
  SRRIP_DATA_CFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CRPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DRPOLICY(policy_data) = cache_policy_srrip;
  
  assert(SRRIP_DATA_MAX_RRPV(policy_data) != 0);

#undef MAX_RRPV
}

#define CACHE_SET(tag, cache)   ((tag) % (cache->num_sets))

/* Approximate miss for Pset calculation */
#define PSET_MISS(s, g)         ((s)->access_llc - ((s)->access_llc >> (g)->sap_cpu_tpset))

#define PSET_HIT(s, g)          (((s)->access_llc - (s)->miss_llc) >> (g)->sap_cpu_tpset)

/* P set threshold using static pset threshold passed in configuration */
#define PSET_ST_THR(s, g)       (g->sdp_psethrt ? PSET_HIT(s, g) : PSET_MISS((s), (g)))

/* Overall miss rate for all access in LRU table */
#define PSET_GLOBAL_MR(s)       (((uf8)(s)->llc_miss) / (s)->llc_access)

/* P set threshold using miss rate */
#define PSET_MR_THR(s, st)      ((ub8)((s)->access_llc * PSET_GLOBAL_MR(st)))

/* P and Q set threshold computation */
#define PSET_THR(s, g, st)      (g->sdp_psetmrt ? PSET_MR_THR(s, st) : PSET_ST_THR(s, g))
#define QSET_THR(s, g)          ((s)->total_stall_cycles >> (g)->sap_cpu_tqset)
#define CHK_QSET_HRT(s, g, st)  (((s)->access_llc - (s)->miss_llc) < PSET_THR(s, g, st))
#define CHK_QSET_MRT(s, g, st)  ((s)->miss_llc > PSET_THR(s, g, st))
#define CHK_PTHR(s, g, st)      ((g)->sdp_psethrt ? CHK_QSET_HRT(s, g, st) : CHK_QSET_MRT(s, g, st))

/* Returns SAP stream corresponding to access stream based on stream 
 * classification algoritm. */
sap_stream get_sap_stream(sap_gdata *global_data, ub1 stream_in, 
  ub1 pid_in, memory_trace *info)
{
  assert(1);
}

/* Public interface to initialize SAP statistics */
void cache_init_sap_stats(sap_stats *stats, char *file_name)
{
  if (!stats->stat_file)
  {
    /* Open statistics file */
    if (!file_name)
    {
      stats->stat_file = gzopen("PC-SAP-stats.trace.csv.gz", "wb9");
    }
    else
    {
      stats->stat_file = gzopen(file_name, "wb9");
    }

    assert(stats->stat_file);

    stats->sap_x_fill       = 0;
    stats->sap_y_fill       = 0;
    stats->sap_p_fill       = 0;
    stats->sap_q_fill       = 0;
    stats->sap_r_fill       = 0;
    stats->sap_x_access     = 0;
    stats->sap_y_access     = 0;
    stats->sap_p_access     = 0;
    stats->sap_q_access     = 0;
    stats->sap_r_access     = 0;
    stats->sap_x_evct       = 0;
    stats->sap_y_evct       = 0;
    stats->sap_p_evct       = 0;
    stats->sap_q_evct       = 0;
    stats->sap_r_evct       = 0;
    stats->sap_psetrecat    = 0;
    stats->hdr_printed      = FALSE;
    stats->next_schedule    = 0;
    stats->schedule_period  = 500 * 1024;
  }
}

/* Public interface to finalize SAP statistic */
void cache_fini_sap_stats(sap_stats *stats)
{
  gzclose(stats->stat_file);
}

/* Public interface to update SAP sats on fill */
void cache_update_sap_fill_stats(sap_gdata *global_data, 
  sap_stream sstream)
{
  sap_stats   *stats;

  /* Obtain SAP specific stream */
  assert(sstream <= global_data->sap_streams);
  
  /* Get SAP statistics */
  stats = &(global_data->stats);
  assert(stats);
  
  /* Update stream specific stats */
  switch (sstream)
  {
    case sap_stream_x:
      stats->sap_x_fill += 1;
      break;
    
    case sap_stream_y:
      stats->sap_y_fill += 1;
      break;

    case sap_stream_p:
      stats->sap_p_fill += 1;
      break;

    case sap_stream_q:
      stats->sap_q_fill += 1;
      break;

    case sap_stream_r:
      stats->sap_r_fill += 1;
      break;

    default:
      panic("Invalid SAP stream function %s line %d\n", __FUNCTION__, __LINE__);
  }
}

/* Public interface to update SAP stats on hit. */
void cache_update_sap_access_stats(sap_gdata *global_data, memory_trace *info, ub1 psetrecat)
{
  sap_stream   sstream;
  sap_stats   *stats;
  
  /* Obtain SAP specific stream */
  sstream = get_sap_stream(global_data, info->stream, info->pid, info);
  assert(sstream <= global_data->sap_streams);
  
  /* Obtain SAP stats */
  stats = &(global_data->stats);
  assert(stats);
  
  /* As per stream update access count. */
  switch (sstream)
  {
    case sap_stream_x:
      stats->sap_x_access += 1;
      break;
    
    case sap_stream_y:
      stats->sap_y_access += 1;
      break;

    case sap_stream_p:
      stats->sap_p_access += 1;
      break;

    case sap_stream_q:
      stats->sap_q_access += 1;
      break;

    case sap_stream_r:
      stats->sap_r_access += 1;
      break;

    default:
      panic("Invalid SAP stream function %s line %d\n", __FUNCTION__, __LINE__);
  }

  if (psetrecat)
  {
    stats->sap_psetrecat += 1;
  }
}

/* Public interface to update SAP sats on replacement */
void cache_update_sap_replacement_stats(sap_gdata *global_data, 
  sap_stream sstream)
{
  sap_stats   *stats;

  /* Obtain SAP specific stream */
  assert(sstream <= global_data->sap_streams);
  
  /* Get SAP statistics */
  stats = &(global_data->stats);
  assert(stats);
  
  /* Update stream specific stats */
  switch (sstream)
  {
    case sap_stream_x:
      stats->sap_x_evct += 1;
      break;
    
    case sap_stream_y:
      stats->sap_y_evct += 1;
      break;

    case sap_stream_p:
      stats->sap_p_evct += 1;
      break;

    case sap_stream_q:
      stats->sap_q_evct += 1;
      break;

    case sap_stream_r:
      stats->sap_r_evct += 1;
      break;

    default:
      panic("Invalid SAP stream function %s line %d\n", __FUNCTION__, __LINE__);
  }
}

/* Update fill/access stats */
void cache_update_sap_fill_stall_stats(sap_gdata *global_data, memory_trace *info)
{
  assert(1);
}

/* Update fill/access stats */
void cache_update_sap_access_stall_stats(sap_gdata *global_data, 
    memory_trace *info, ub1 psetrecat)
{
  assert(1);
}

#undef CACHE_SET
#undef PSET_MISS
#undef PSET_AS_THR
#undef PSET_GLOBAL_MR
#undef PSET_BS_THR
#undef PSET_THR
#undef QSET_THR

/* Public interface to dump periodic SAP statistics */
void cache_dump_sap_stats(sap_stats *stats, ub8 cycle)
{
  /* Print header if not already done */
  if (stats->hdr_printed == FALSE)
  {
    gzprintf(stats->stat_file,
        "CYCLE; XFILL; YFILL; PFILL; QFILL; RFILL; " 
        "XEVCT; YEVCT; PEVCT; QEVCT; REVCT; "
        " XACCESS; YACCESS; PACCESS; QACCESS; RACCESS; RECAT\n");
    stats->hdr_printed = TRUE;
  }
  
  /* Dump current counter values */
  gzprintf(stats->stat_file,
      "%ld; %ld; %ld; %ld; %ld; %ld; "
      "%ld; %ld; %ld; %ld; %ld; "
      "%ld; %ld; %ld; %ld; %ld; %d \n", cycle, stats->sap_x_fill, 
      stats->sap_y_fill, stats->sap_p_fill, stats->sap_q_fill,
      stats->sap_r_fill, stats->sap_x_evct, stats->sap_y_evct,
      stats->sap_p_evct, stats->sap_q_evct, stats->sap_r_evct,
      stats->sap_x_access, stats->sap_y_access, stats->sap_p_access,
      stats->sap_q_access, stats->sap_r_access, stats->sap_psetrecat);
  
  /* Reset all stat counters */
  stats->sap_x_fill     = 0;
  stats->sap_y_fill     = 0;
  stats->sap_p_fill     = 0;
  stats->sap_q_fill     = 0;
  stats->sap_r_fill     = 0;
  stats->sap_x_access   = 0;
  stats->sap_y_access   = 0;
  stats->sap_p_access   = 0;
  stats->sap_q_access   = 0;
  stats->sap_r_access   = 0;
  stats->sap_x_evct     = 0;
  stats->sap_y_evct     = 0;
  stats->sap_p_evct     = 0;
  stats->sap_q_evct     = 0;
  stats->sap_r_evct     = 0;
  stats->sap_psetrecat  = 0;
}

void cache_init_sap(long long int set_indx, struct cache_params *params, sap_data *policy_data,
  sap_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);
  
  /* For the time being at max 5 cores are supported (1 gfx + 4 cpu) */
  assert((params->sdp_gpu_cores + params->sdp_cpu_cores) <= MAX_SAP_CORES);

  if (set_indx == 0)
  {
    /* Initialize SAP specific statistics */
    cache_init_sap_stats(&(global_data->stats), NULL);
    
    /* Initialize cache-wide data for SAP. */
    global_data->threshold          = params->threshold;
    global_data->sap_streams        = params->sdp_streams;
    global_data->sap_cpu_cores      = params->sdp_cpu_cores;
    global_data->sap_gpu_cores      = params->sdp_gpu_cores;
    global_data->sap_cpu_tpset      = params->sdp_cpu_tpset;
    global_data->sap_cpu_tqset      = params->sdp_cpu_tqset;
    global_data->sdp_greplace       = params->sdp_greplace;
    global_data->sdp_psetinstp      = params->sdp_psetinstp;
    global_data->activate_access    = params->sdp_tactivate * params->num_sets;
    global_data->sdp_psetbmi        = params->sdp_psetbmi;
    global_data->sdp_psetrecat      = params->sdp_psetrecat;
    global_data->sdp_psetbse        = params->sdp_psetbse;
    global_data->sdp_psetmrt        = params->sdp_psetmrt;
    global_data->sdp_psethrt        = params->sdp_psethrt;
    
    /* Only one hit rate threshold or miss rate threshold can be true */
    if (global_data->sdp_psetmrt)
    {
      assert(global_data->sdp_psethrt == FALSE);
    }

    if (global_data->sdp_psethrt)
    {
      assert(global_data->sdp_psetmrt == FALSE);
    }

    /* Temporary to make it available to CPU core */
    sdp_base_sample_eval = params->sdp_psetbse;

    /* Initialize bimodal access counter */
    SAT_CTR_INI(global_data->access_ctr, 8, 0, 255);
  }
  
  /* Allocatea and initialize per set data */

#define MAX_RRPV (params->max_rrpv)

  /* Set max RRPV for the set */
  SAP_DATA_MAX_RRPV(policy_data)  = MAX_RRPV;

#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  SAP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SAP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

  assert(SAP_DATA_VALID_HEAD(policy_data));
  assert(SAP_DATA_VALID_TAIL(policy_data));

#undef MEM_ALLOC
  
  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SAP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    SAP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    SAP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  SAP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

  SAP_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  SAP_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);

#undef MEM_ALLOC

  /* Initialize array of blocks */
  SAP_DATA_FREE_HEAD(policy_data) = &(SAP_DATA_BLOCKS(policy_data)[0]);
  SAP_DATA_FREE_TAIL(policy_data) = &(SAP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SAP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SAP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SAP_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SAP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SAP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SAP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }
  
  /* Allocate pointer table for per core per stream policy and initialize it to SAP. */
#define TSAP_STREAMS(g)   (g->sap_streams + 1)
#define TOTAL_PSPOLICY(g) (TSAP_STREAMS(g) * SAP_GDATA_CORES(global_data))

  SAP_DATA_PSPOLICY(policy_data) = 
    (cache_policy_t *)xcalloc(TOTAL_PSPOLICY(global_data), sizeof(cache_policy_t));
  assert(SAP_DATA_PSPOLICY(policy_data));

#undef TSAP_STREAMS
#undef TOTAL_PSPOLICY

  
  for (int stream = 1; stream < (global_data->sap_streams + 1) * SAP_GDATA_CORES(global_data); stream++)
  {
    SAP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_sap;
  }
  
  /* Initialize SRRIP component */
  cache_init_srrip_from_sap(&(policy_data->srrip), policy_data);
  
  /* Ensure RRPV is correctly set */
  assert(SAP_DATA_MAX_RRPV(policy_data) != 0);

#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_sap(long long int set_indx, sap_data *policy_data, sap_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Free all data blocks */
  free(SAP_DATA_BLOCKS(policy_data));

  /* Free per-stream policy */
  free(SAP_DATA_PSPOLICY(policy_data));

  /* Free valid head buckets */
  if (SAP_DATA_VALID_HEAD(policy_data))
  {
    free(SAP_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SAP_DATA_VALID_TAIL(policy_data))
  {
    free(SAP_DATA_VALID_TAIL(policy_data));
  }

  /* Clean up SAP stats */
  if (set_indx ==  0)
  {
    cache_fini_sap_stats(&(global_data->stats));
  }
}

/* Get RRPV block to be filled */
static int cache_get_fill_rrpv_sap(sap_data *policy_data, 
  sap_gdata *global_data, memory_trace *info)
{
  sap_stream stream;

  stream = get_sap_stream(global_data, info->stream, info->pid, info);

  switch (stream)
  {
    case sap_stream_x:
      return 0;

    case sap_stream_y:
      return SAP_DATA_MAX_RRPV(policy_data);

    case sap_stream_p:

      if (!global_data->sdp_psetbmi)
      {
        return SAP_DATA_MAX_RRPV(policy_data);
      }
      else
      {
#define CTR_VAL(data)   (SAT_CTR_VAL(data->access_ctr))
#define THRESHOLD(data) (data->threshold)

        /* Block is inserted with long reuse prediction value only if counter
         * value is 0. */
        if (CTR_VAL(global_data) == 0)
        {
          return SAP_DATA_MAX_RRPV(policy_data) - 1;
        }
        else 
        {
          return SAP_DATA_MAX_RRPV(policy_data);
        }

#undef CTR_VAL
#undef THRESHOLD
      }

    case sap_stream_q:
      return 0;

    case sap_stream_r:
      return SAP_DATA_MAX_RRPV(policy_data) - 1;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }
}

/* Get RRPV block is to be promoted on hit */
static int cache_get_new_rrpv_sap(sap_data *policy_data, 
  sap_gdata *global_data, memory_trace *info, int old_rrpv)
{
  sap_stream stream;

  stream = get_sap_stream(global_data, info->stream, info->pid, info);

  switch (stream)
  {
    case sap_stream_x:
      return 0;

    case sap_stream_y:
      /* TODO: Check threshold and reuse count */
      return (global_data->sdp_psetinstp) ? 0 : SAP_DATA_MAX_RRPV(policy_data);

    case sap_stream_p:
      /* TODO: Look at pc stats */
#define RRPV_STEP_DOWN(r)  (((r) == 0) ? (r) : (r) - 1)

      return (global_data->sdp_psetinstp) ? 0 : RRPV_STEP_DOWN(old_rrpv);

#undef RRPV_STEP_DOWN

    case sap_stream_q:
      return 0;

    case sap_stream_r:
      return 0;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }
}

static int cache_get_replacement_rrpv_sap(sap_data *policy_data)
{
  return SAP_DATA_MAX_RRPV(policy_data);
}

struct cache_block_t * cache_find_block_sap(sap_data *policy_data, long long tag)
{
  int    max_rrpv;
  struct cache_block_t *head;
  struct cache_block_t *node;

  max_rrpv  = policy_data->srrip.max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SAP_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

end:
  return node;
}

#define INVALID_WAYID  (~(0))

/* Get replacement candidate based on access stream */
static unsigned int get_stream_aware_candidate(sap_data *policy_data, 
  sap_gdata *global_data, memory_trace *info, int rrpv)
{
  unsigned int           min_wayid;           /* Final wayid */
  unsigned int           min_wayid_all;       /* Victim among all blocks */
  unsigned int          *psmin_wayid;         /* Per stream wayid */
  struct cache_block_t  *block;               /* Cache block */
  enum   cache_policy_t *per_stream_policy;   /* Policy for each stream */
  sap_stream             sstream;             /* SAP specific stream */
  sap_stream             pblock_sstream;      /* SAP specific stream for each block */

  per_stream_policy = SAP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);

  /* Allocate per stream min wayid array */
  psmin_wayid = (unsigned int *)xcalloc((global_data->sap_streams + 1), sizeof(unsigned int));
  assert(psmin_wayid);

  /* Initialize wayid */
  for (int i = 1; i < (global_data->sap_streams); i++)
  {
    psmin_wayid[i] = INVALID_WAYID;
  }
  
  min_wayid     = INVALID_WAYID;
  min_wayid_all = INVALID_WAYID;
  
  /* Get SAP specific stream for input request */
  sstream = get_sap_stream(global_data, info->stream, info->pid, info);

  /* Go through RRPV list and get replacement candidate for each SAP stream */
  for (block = SAP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
  {
    if (!block->busy && per_stream_policy[sstream] == cache_policy_sap &&
        block->way < psmin_wayid[block->sap_stream])
    {
      /* Get block SAP specific srtream */
      pblock_sstream = block->sap_stream; 
      psmin_wayid[pblock_sstream] = block->way;
    }

    if (!block->busy && block->way < min_wayid_all)
    {
      min_wayid_all = block->way; 
    }
  }
  
  /* Based on input stream choose victim from appropriate stream */
  switch(sstream) 
  {
    case sap_stream_x:
    case sap_stream_p:
    case sap_stream_q:
    case sap_stream_r:
      /* Lookup in P, R, Y, Q, X order */
      if (psmin_wayid[sap_stream_p] != INVALID_WAYID) 
      {
        min_wayid = psmin_wayid[sap_stream_p];
      }
      else
      {
        if (psmin_wayid[sap_stream_r] != INVALID_WAYID)
        {
          min_wayid = psmin_wayid[sap_stream_r];
        }
        else
        {
          if (psmin_wayid[sap_stream_y] != INVALID_WAYID)
          {
            min_wayid = psmin_wayid[sap_stream_y];
          }
          else
          {
            if (psmin_wayid[sap_stream_q] != INVALID_WAYID)
            {
              min_wayid = psmin_wayid[sap_stream_q];
            }
            else
            {
              if (psmin_wayid[sap_stream_x] != INVALID_WAYID)
              {
                min_wayid = psmin_wayid[sap_stream_x];
              }
              else
              {
                min_wayid = min_wayid_all;
              }
            }
          }
        }
      }

      assert(min_wayid != INVALID_WAYID);
      break;

    case sap_stream_y:
      /* Lookup P and Y only */
      if (psmin_wayid[sap_stream_p] != INVALID_WAYID) 
      {
        min_wayid = psmin_wayid[sap_stream_p];
      }
      else
      {
        if (psmin_wayid[sap_stream_y] != INVALID_WAYID)
        {
          min_wayid = psmin_wayid[sap_stream_y];
        }
        else
        {
          /* If no candidate bypass the cache */
          min_wayid = BYPASS_WAY;
        }
      }
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }
  
  /* Deallocate per stream min way id */
  free(psmin_wayid);
  
  /* Return min wayid (global / from stream) irrespective of stream */
  return (global_data->sdp_greplace) ? min_wayid_all : min_wayid;
}

/* Get a replacement candidate */
int cache_replace_block_sap(sap_data *policy_data, sap_gdata *global_data,
  memory_trace *info)
{
  int       rrpv;
  unsigned  int min_wayid;

  struct cache_block_t  *block;
  enum   cache_policy_t *per_stream_policy;

  sap_stream sstream;

  /* Ensure vaid arguments */
  assert(global_data);
  assert(policy_data);
  
  per_stream_policy = SAP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);

  /* Get per stream policy map */
  sstream = get_sap_stream(global_data, info->stream, info->pid, info);

  switch (per_stream_policy[info->core * SAP_GDATA_CORES(global_data) + sstream])
  {
    case cache_policy_sap:
      /* Try to find an invalid block always from head of the free list. */
      for (block = SAP_DATA_FREE_HEAD(policy_data); block; block = block->next)
        return block->way;

      /* Obtain RRPV from where to replace the block */
      rrpv = cache_get_replacement_rrpv_sap(policy_data);

      /* Ensure rrpv is with in max_rrpv bound */
      assert(rrpv >= 0 && rrpv <= SAP_DATA_MAX_RRPV(policy_data));

      /* If there is no block with required RRPV, increment RRPV of all the 
       * blocks until we get one with the required RRPV */
      if (SAP_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
      {
        CACHE_SAP_INCREMENT_RRPV(SAP_DATA_VALID_HEAD(policy_data), 
            SAP_DATA_VALID_TAIL(policy_data), rrpv);
      }

      /* Choose candidate form replacement RRPV */
      min_wayid = get_stream_aware_candidate(policy_data, global_data, info, rrpv);
      
      /* Only graphics is allowed to bypass the cache */
      if (min_wayid == BYPASS_WAY)
      {
        assert(info->stream < TST);
      }
    
      if (min_wayid != INVALID_WAYID)
      {
        block = &(SAP_DATA_BLOCKS(policy_data)[min_wayid]);

        if (block->state != cache_block_invalid)
        {
          cache_update_sap_replacement_stats(global_data, block->sap_stream);
        }
      }

      /* TODO: For x and y stream a dead graphics block can be chosen as candidate */

      /* If no non busy block can be found, return -1 */
      return (min_wayid != INVALID_WAYID) ? min_wayid : -1;

    case cache_policy_srrip:
      min_wayid =  cache_replace_block_srrip(&(policy_data->srrip), &(global_data->srrip));

      if (min_wayid != INVALID_WAYID)
      {
        block = &(SAP_DATA_BLOCKS(policy_data)[min_wayid]);

        if (block->state != cache_block_invalid)
        {
          cache_update_sap_replacement_stats(global_data, block->sap_stream);
        }
      }
      
      return min_wayid;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }
}

#undef INVALID_WAYID

/* Fill new block. Basically, block to be filled is already chosen, 
 * This function only updates tag and state bits. */
void cache_fill_block_sap(sap_data *policy_data, sap_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int stream, 
  memory_trace *info)
{
  int         rrpv;
  sap_stream  sstream;

  struct cache_block_t  *block;
  enum   cache_policy_t *per_stream_policy;  

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);
  
  sstream = get_sap_stream(global_data, info->stream, info->pid, info); 

  per_stream_policy = SAP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);

#define CTR_VAL(d)    (SAT_CTR_VAL((d)->access_ctr))
#define THRESHOLD(d)  ((d)->threshold)

  /* Update access counter for bimodal insertion */
  if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
  {
    /* Increment set access count */
    SAT_CTR_INC(global_data->access_ctr);
  }
  else
  {
    /* Reset access count */
    SAT_CTR_RST(global_data->access_ctr);
  }

#undef CTR_VAL
#undef THRESHOLD

  switch (per_stream_policy[info->core * SAP_GDATA_CORES(global_data) + sstream])
  {
    case cache_policy_sap:
      /* Remove the block from free list and update block information */
      if (way != BYPASS_WAY)
      {
        block = &(SRRIP_DATA_BLOCKS(policy_data)[way]);
        assert(block); 
        assert(block->stream == 0);

        /* Remove block from free list */
        free_list_remove_block(policy_data, block);

        /* Update new block state and stream */
        CACHE_UPDATE_BLOCK_STATE(block, tag, state);
        CACHE_UPDATE_BLOCK_PID(block, info->pid);
        CACHE_UPDATE_BLOCK_STREAM(block, info->stream);
        CACHE_UPDATE_BLOCK_SSTREAM(block, sstream);

        block->dirty  = (info && info->spill) ? 1 : 0;
        block->epoch  = 0;
        block->access = 0;

        /* Get RRPV to be assigned to the new block */
        if (info && info->fill == TRUE)
        {
          rrpv = cache_get_fill_rrpv_sap(policy_data, global_data, info);
        }
        else
        {
          rrpv = SAP_DATA_MAX_RRPV(policy_data);
        }

        /* Ensure a valid RRPV */
        assert(rrpv >= 0 && rrpv <= policy_data->max_rrpv); 

        /* Insert block in to the corresponding RRPV queue */
        CACHE_APPEND_TO_QUEUE(block, 
            SAP_DATA_VALID_HEAD(policy_data)[rrpv], 
            SAP_DATA_VALID_TAIL(policy_data)[rrpv]);

        /* Update SAP specific fill stats for accessin PC in LRU table */
        cache_update_sap_fill_stats(global_data, sstream);
        cache_update_sap_fill_stall_stats(global_data, info);
      }
      break;

    case cache_policy_srrip:
      cache_fill_block_srrip(&(policy_data->srrip), &(global_data->srrip), way,
          tag, state, stream, info);
      
      /* Obtain block to update SAP stream, this is done to collect SAP 
       * specific stats on eviction */
      block = &(SRRIP_DATA_BLOCKS(policy_data)[way]);
      assert(block); 
      assert(block->stream == info->stream);
      assert(block->state != cache_block_invalid);

      CACHE_UPDATE_BLOCK_SSTREAM(block, sstream);

      /* Update SAP specific fill stats for accessin PC in LRU table */
      cache_update_sap_fill_stall_stats(global_data, info);
      cache_update_sap_fill_stats(global_data, sstream);
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }
}

/* Update block state on hit */
void cache_access_block_sap( sap_data *policy_data, sap_gdata *global_data, 
  int way, int stream, memory_trace *info)
{
  struct  cache_block_t  *blk;
  struct  cache_block_t  *next;
  struct  cache_block_t  *prev;
  enum    cache_policy_t *per_stream_policy;
  int     old_rrpv;
  int     new_rrpv;
  ub1     psetrecat;

  sap_stream sstream;

  blk       = NULL;
  next      = NULL;
  prev      = NULL;
  psetrecat = 0;
  
  /* Obtain per-stream policy */
  per_stream_policy = SAP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);

  sstream = get_sap_stream(global_data, info->stream, info->pid, info);

  blk  = &(SAP_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);

  switch (per_stream_policy[info->core * SAP_GDATA_CORES(global_data) + sstream])
  {
    case cache_policy_sap:
      if (way != BYPASS_WAY)
      {
        CACHE_UPDATE_BLOCK_STREAM(blk, info->stream);
        CACHE_UPDATE_BLOCK_PID(blk, info->pid);
        blk->dirty  = (info && info->spill) ? 1 : 0;

        /* Update block access count */
        if (blk->stream != info->stream)
        {
          blk->access = 0;
        }
        else
        {
          blk->access += 1;
        }

        /* Update RRPV for read hits. For write hits RRPV doesn't change */
        if (info && info->fill == TRUE)
        {
          /* Get old RRPV from the block */
          old_rrpv = (((srrip_list *)(blk->data))->rrpv);

          /* Get new RRPV using policy specific function */
          new_rrpv = cache_get_new_rrpv_sap(policy_data, global_data, info, old_rrpv);

          /* Update block queue if block got new RRPV */
          if (new_rrpv != old_rrpv)
          {
            CACHE_REMOVE_FROM_QUEUE(blk, SAP_DATA_VALID_HEAD(policy_data)[old_rrpv],
                SAP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
            CACHE_APPEND_TO_QUEUE(blk, SAP_DATA_VALID_HEAD(policy_data)[new_rrpv], 
                SAP_DATA_VALID_TAIL(policy_data)[new_rrpv]);
          }
        }
        
        if (blk->sap_stream == sap_stream_p && blk->sap_stream != sstream && 
            global_data->sdp_psetrecat)
        {
          CACHE_UPDATE_BLOCK_STREAM(blk, sstream);

          /* Set switch to recategorize to update update stats */
          psetrecat = 1;
        }
      }
        
      /* Update SAP specific hit stats */
      cache_update_sap_access_stats(global_data, info, psetrecat);
      cache_update_sap_access_stall_stats(global_data, info, psetrecat);

    case cache_policy_srrip:
      cache_access_block_srrip(&(policy_data->srrip), &(global_data->srrip),
          way, stream, info);
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }
}

/* Update state of the block. */
void cache_set_block_sap(sap_data *policy_data, sap_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream,
  memory_trace *info)
{
  struct cache_block_t  *block;
  enum   cache_policy_t *per_stream_policy;

  sap_stream sstream;

  per_stream_policy = SAP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);

  sstream = get_sap_stream(global_data, info->stream, info->pid, info);
  
  switch (per_stream_policy[info->core * SAP_GDATA_CORES(global_data) + sstream])
  {
    case cache_policy_sap:
      if (way != BYPASS_WAY)
      {
        block = &(SRRIP_DATA_BLOCKS(policy_data))[way];

        /* Check: tag matches with the block's tag. */
        assert(block->tag == tag);

        /* Check: block must be in valid state. It is not possible to set
         * state for an invalid block.*/
        assert(block->state != cache_block_invalid);

        if (state != cache_block_invalid)
        {
          /* Assign access stream */
          CACHE_UPDATE_BLOCK_STATE(block, tag, state);
          CACHE_UPDATE_BLOCK_STREAM(block, info->stream);
          CACHE_UPDATE_BLOCK_PID(block, info->pid);
          return;
        }

        /* Invalidate block */
        CACHE_UPDATE_BLOCK_STATE(block, tag, cache_block_invalid);
        CACHE_UPDATE_BLOCK_STREAM(block, NN);
        block->epoch  = 0;
        block->access = 0;

        /* Get old RRPV from the block */
        int old_rrpv = (((srrip_list *)(block->data))->rrpv);

        /* Remove block from valid list and insert into free list */
        CACHE_REMOVE_FROM_QUEUE(block, SRRIP_DATA_VALID_HEAD(policy_data)[old_rrpv],
            SRRIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_SQUEUE(block, SRRIP_DATA_FREE_HEAD(policy_data), 
            SRRIP_DATA_FREE_TAIL(policy_data));
      }
      break;

    case cache_policy_srrip:
      cache_set_block_srrip(&(policy_data->srrip), way, tag, state, stream, info);
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      break;
  }
}

/* Get tag and state of a block. */
struct cache_block_t cache_get_block_sap(sap_data *policy_data, 
  sap_gdata *global_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  struct cache_block_t ret_block;

  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  assert(tag_ptr);
  assert(state_ptr);
  
  memset(&ret_block, 0, sizeof(struct cache_block_t));

  if (way != BYPASS_WAY)
  {
    PTR_ASSIGN(tag_ptr, (SAP_DATA_BLOCKS(policy_data)[way]).tag);
    PTR_ASSIGN(state_ptr, (SAP_DATA_BLOCKS(policy_data)[way]).state);
    PTR_ASSIGN(stream_ptr, (SAP_DATA_BLOCKS(policy_data)[way]).stream);
  }
  else
  {
    PTR_ASSIGN(tag_ptr, 0xdead);
    PTR_ASSIGN(state_ptr, cache_block_invalid);
    PTR_ASSIGN(stream_ptr, NN);
  }

  return (way != BYPASS_WAY) ? SAP_DATA_BLOCKS(policy_data)[way] : ret_block;
}

/* Set policy for a stream */
void set_per_stream_policy_sap(sap_data *policy_data, sap_gdata *global_data, 
  ub4 core, ub1 stream, cache_policy_t policy)
{
  assert(core < SAP_GDATA_CORES(global_data));
  assert(stream > 0 && stream <= TST);
  assert(policy == cache_policy_srrip || policy == cache_policy_sap);
  
  /* Assign policy to stream */
  if (stream < TST)
  {
    assert(global_data->sap_gpu_cores > 0 && core == 0);
    SAP_DATA_PSPOLICY(policy_data)[core * SAP_GDATA_CORES(global_data) + sap_stream_x] = policy;
    SAP_DATA_PSPOLICY(policy_data)[core * SAP_GDATA_CORES(global_data) + sap_stream_y] = policy;
  }
  else
  {
    /* For CPU core id starts from 1 */
    assert(core > 0);

    SAP_DATA_PSPOLICY(policy_data)[core * SAP_GDATA_CORES(global_data) + sap_stream_p] = policy;
    SAP_DATA_PSPOLICY(policy_data)[core * SAP_GDATA_CORES(global_data) + sap_stream_q] = policy;
    SAP_DATA_PSPOLICY(policy_data)[core * SAP_GDATA_CORES(global_data) + sap_stream_r] = policy;
  }
}

#undef CACHE_UPDATE_BLOCK_STATE
#undef CACHE_UPDATE_BLOCK_STREAM
#undef CACHE_SAP_INCREMENT_RRPV
#undef free_list_remove_block
