/* * Copyright (C) 2014  Siddharth Rai
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
 * 02110-1301, USA.
 */

#include <stdlib.h>
#include <assert.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "../common/intermod-common.h"

#include "cache.h"
#include "cache-block.h"
#include "sarp.h"
#include "zlib.h"

#define PSEL_WIDTH                (30)
#define PSEL_MIN_VAL              (0x00)  
#define PSEL_MAX_VAL              (1 << 30)  
#define PSEL_MID_VAL              (PSEL_MAX_VAL / 2)
#define BYTH_NUMR                 (1)
#define BYTH_DENM                 (8)
#define SMTH_NUMR                 (1)
#define SMTH_DENM                 (8)
#define SHTH_NUMR                 (1)
#define SHTH_DENM                 (16)
#define BYPASS_ACCESS_TH          (256 * 1024)
#define EPOCH_SIZE                (1 << 19)
#define CPS(i)                    (i == PS)
#define CPS1(i)                   (i == PS1)
#define CPS2(i)                   (i == PS2)
#define CPS3(i)                   (i == PS3)
#define CPS4(i)                   (i == PS4)
#define GPU_STREAM(i)             (i < PS)
#define CPU_STREAM(i)             (CPS(i) || CPS1(i) || CPS2(i) || CPS3(i) || CPS4(i))
#define SARP_SAMPLER_INVALID_TAG  (0xfffffffffffffffULL)
#define STREAM_OCC_THRESHOLD      (32)
#define LOG_GRAIN_SIZE            (12)
#define LOG_BLOCK_SIZE            (6)
#define DYNC(b, o)                ((b).dynamic_color[o] == TRUE)
#define DYNZ(b, o)                ((b).dynamic_depth[o] == TRUE)
#define DYNB(b, o)                ((b).dynamic_blit[o] == TRUE)
#define DYNP(b, o)                ((b).dynamic_proc[o] == TRUE)
#define KNOWN_STREAM(s)           (s == CS || s == ZS || s == TS || s == BS || s == PS)
#define OTHER_STREAM(s)           (!(KNOWN_STREAM(s)))
#define NEW_STREAM(i)             (KNOWN_STREAM((i)->stream) ? (i)->stream : OS)
#define DYNBP(b ,o, i)            (DYNB(b ,o) ? DBS : DYNP(b ,o) ? DPS : NEW_STREAM(i))
#define SARP_STREAM(b, o, i)      (DYNC(b, o) ? DCS : DYNZ(b, o) ? DZS : DYNBP(b, o, i))
#define OLD_STREAM(b, o)          (KNOWN_STREAM((b).stream[o]) ? (b).stream[o] : OS)
#define ODYNBP(b ,o, i)           (DYNB(b ,o) ? DBS : DYNP(b ,o) ? DPS : OLD_STREAM(b, o))
#define SARP_OSTREAM(b, o, i)     (DYNC(b, o) ? DCS : DYNZ(b, o) ? DZS : ODYNBP(b, o, i))

#define CACHE_UPDATE_BLOCK_SSTREAM(block, strm)               \
do                                                            \
{                                                             \
  (block)->sarp_stream = strm;                                \
}while(0)

#define CACHE_UPDATE_BLOCK_PID(block, pid_in)                 \
do                                                            \
{                                                             \
  (block)->pid = pid_in;                                      \
}while(0)

#define CACHE_SARP_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)   \
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
        assert(SARP_DATA_FREE_HEAD(set) && SARP_DATA_FREE_TAIL(set));                   \
                                                                                        \
        /* Check: current block must be in invalid state */                             \
        assert((blk)->state == cache_block_invalid);                                    \
                                                                                        \
        CACHE_REMOVE_FROM_SQUEUE(blk, SARP_DATA_FREE_HEAD(set), SARP_DATA_FREE_TAIL(set));\
                                                                                        \
        (blk)->next = NULL;                                                             \
        (blk)->prev = NULL;                                                             \
                                                                                        \
        /* Reset block state */                                                         \
        (blk)->busy = 0;                                                                \
        (blk)->cached = 0;                                                              \
        (blk)->prefetch = 0;                                                            \
}while(0);

/* 
 * Current policy is decided based on drrip_psel, sarp_psel and 
 * sarp_ssel counter values.
 *
 *  If current set is PS_SAMPLED_SET and if sarp_psel counter is smaller
 *  than MID_VALUE, and stream is in speedup set, sarp_ssel counter is 
 *  consulted to decide the policy. Otherwise, drrip_psel counter is consulted to 
 *  decide the policy.
 *
 * */

#define PSRRIP                        (cache_policy_srrip)
#define PBRRIP                        (cache_policy_brrip)
#define PSARP                         (cache_policy_sarp)
#define FSRRIP(f)                     (f == PSRRIP)
#define FBRRIP(f)                     (f == PBRRIP)
#define FSARP(f)                      (f == PSARP)
#define CSRRIPGSRRIP(p)               ((p)->set_type == SARP_CSRRIP_GSRRIP_SET)
#define CSRRIPGBRRIP(p)               ((p)->set_type == SARP_CSRRIP_GBRRIP_SET)
#define CBRRIPGSRRIP(p)               ((p)->set_type == SARP_CBRRIP_GSRRIP_SET)
#define CBRRIPGBRRIP(p)               ((p)->set_type == SARP_CBRRIP_GBRRIP_SET)
#define CSRRIP_SET(p)                 (CSRRIPGSRRIP(p) || CSRRIPGBRRIP(p))
#define CBRRIP_SET(p)                 (CBRRIPGSRRIP(p) || CBRRIPGBRRIP(p))
#define GSRRIP_SET(p)                 (CSRRIPGSRRIP(p) || CBRRIPGSRRIP(p))
#define GBRRIP_SET(p)                 (CSRRIPGBRRIP(p) || CBRRIPGBRRIP(p))
#define GSET(p, i)                    (GSRRIP_SET(p) ? PSRRIP : GBRRIP_SET(p) ? PBRRIP : PSARP)
#define CSET(p, i)                    (CSRRIP_SET(p) ? PSRRIP : CBRRIP_SET(p) ? PBRRIP : PSARP)
#define DPOLICY(p, i)                 (CPU_STREAM((i)->stream) ? CSET(p, i) : GSET(p, i))
#define PCSRRIPGSRRIP(i)              (PSRRIP)
#define PCSRRIPGBRRIP(i)              (CPU_STREAM((i)->stream) ? PSRRIP : PBRRIP)
#define PCBRRIPGSRRIP(i)              (CPU_STREAM((i)->stream) ? PBRRIP : PSRRIP)
#define PCBRRIPGBRRIP(i)              (PBRRIP)
#define BRRIP_VAL(g)                  (SAT_CTR_VAL((g)->brrip_psel))
#define SRRIP_VAL(g)                  (SAT_CTR_VAL((g)->srrip_psel))
#define SARP_VAL(g)                   (SAT_CTR_VAL((g)->sarp_psel))
#define PCBRRIP(p, g, i)              ((BRRIP_VAL(g) <= PSEL_MID_VAL) ? PCSRRIPGBRRIP(i) : PCBRRIPGBRRIP(i))
#define PCSRRIP(p, g, i)              ((SRRIP_VAL(g) <= PSEL_MID_VAL) ? PCSRRIPGSRRIP(i) : PCBRRIPGSRRIP(i))
#define DPSARP(p, g, i)               ((SARP_VAL(g) <= PSEL_MID_VAL) ? PCSRRIP(p, g, i) : PCBRRIP(p, g, i))
#define SPSARP(p, g, i, f)            (FSARP(f) ? DPSARP(p, g, i) : f)
#define FANDH(i, h)                   ((i)->fill == TRUE && (h) == TRUE)
#define FANDM(i, h)                   ((i)->fill == TRUE && (h) == FALSE)
#define CURRENT_POLICY(p, g, f, i, h) ((FANDM(i, h)) ? SPSARP(p, g, i, f) : f)
#define FOLLOW_POLICY(f, i)           ((!FSARP(f)) && ((i)->fill) ? f : PSARP)

extern struct cpu_t *cpu;
extern ffifo_info ffifo_global_info;
extern ub1 sdp_base_sample_eval;

static ub4 get_set_type_sarp(long long int indx, ub4 sarp_samples)
{
  int   lsb_bits;
  int   msb_bits;

  /* BS and PS will always be present */
  assert(sarp_samples >= 2 && sarp_samples <= 4);

  lsb_bits = indx & 0x1f;
  msb_bits = (indx >> 5) & 0x1f;
  
  /* CPU and GPU both are running SRRIP */
  if (msb_bits == lsb_bits && sarp_samples >= 1)
  {
    return SARP_CSRRIP_GSRRIP_SET;
  }
  
  /* CPU is running SRRIP while GPU is running BRRIP */
  if (msb_bits == (~lsb_bits & 0x1f) && sarp_samples >= 2)
  {
    return SARP_CBRRIP_GSRRIP_SET;
  }
  
  /* CPU is running BRRIP while, GPU is running SRRIP  */
  if ((msb_bits ^ lsb_bits) == 0xa && sarp_samples >= 3)
  {
    return SARP_CSRRIP_GBRRIP_SET;
  }
  
  /* CPU and GPU both are running BRRIP */
  if ((msb_bits ^ (~lsb_bits & 0x1f)) == 0xa && sarp_samples >= 4)
  {
    return SARP_CBRRIP_GBRRIP_SET;
  }

  return SARP_FOLLOWER_SET;
}

static void cache_init_srrip_from_sarp(struct cache_params *params, 
    srrip_data *policy_data, sarp_data *sarp_policy_data)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(sarp_policy_data);

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV(data)  (SRRIP_DATA_MAX_RRPV(data))

  /* Create per rrpv buckets */
  SRRIP_DATA_VALID_HEAD(policy_data) = SARP_DATA_VALID_HEAD(sarp_policy_data);
  SRRIP_DATA_VALID_TAIL(policy_data) = SARP_DATA_VALID_TAIL(sarp_policy_data);
  
  assert(SRRIP_DATA_VALID_HEAD(policy_data));
  assert(SRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIP_DATA_MAX_RRPV(policy_data) = SARP_DATA_MAX_RRPV(sarp_policy_data);

  /* Assign pointer to block array */
  SRRIP_DATA_BLOCKS(policy_data) = SARP_DATA_BLOCKS(sarp_policy_data);

  /* Initialize array of blocks */
  SRRIP_DATA_FREE_HLST(policy_data) = SARP_DATA_FREE_HLST(sarp_policy_data);
  SRRIP_DATA_FREE_TLST(policy_data) = SARP_DATA_FREE_TLST(sarp_policy_data);

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

static void cache_init_brrip_from_sarp(struct cache_params *params, 
    brrip_data *policy_data, sarp_data *sarp_policy_data)
{
    /* For BRRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

    /* Create per rrpv buckets */
    BRRIP_DATA_VALID_HEAD(policy_data) = SARP_DATA_VALID_HEAD(sarp_policy_data);
    BRRIP_DATA_VALID_TAIL(policy_data) = SARP_DATA_VALID_TAIL(sarp_policy_data);

#undef MEM_ALLOC

    /* Set max RRPV for the set */
    BRRIP_DATA_MAX_RRPV(policy_data)  = SARP_DATA_MAX_RRPV(sarp_policy_data);
    BRRIP_DATA_THRESHOLD(policy_data) = params->threshold;

    /* Initialize head nodes */
    for (int i = 0; i <= MAX_RRPV; i++)
    {
      BRRIP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
      BRRIP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
      BRRIP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
    }

    /* Create array of blocks */
    BRRIP_DATA_BLOCKS(policy_data) = SARP_DATA_BLOCKS(sarp_policy_data);


    BRRIP_DATA_FREE_HLST(policy_data) = SARP_DATA_FREE_HLST(sarp_policy_data);
    assert(BRRIP_DATA_FREE_HLST(policy_data));

    BRRIP_DATA_FREE_TLST(policy_data) = SARP_DATA_FREE_TLST(sarp_policy_data);
    assert(BRRIP_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

    /* Initialize array of blocks */
    BRRIP_DATA_FREE_HEAD(policy_data) = &(BRRIP_DATA_BLOCKS(policy_data)[0]);
    BRRIP_DATA_FREE_TAIL(policy_data) = &(BRRIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

    /* Set current and default fill policies */
    BRRIP_DATA_CFPOLICY(policy_data) = cache_policy_brrip;
    BRRIP_DATA_DFPOLICY(policy_data) = cache_policy_brrip;
    BRRIP_DATA_CAPOLICY(policy_data) = cache_policy_brrip;
    BRRIP_DATA_DAPOLICY(policy_data) = cache_policy_brrip;
    BRRIP_DATA_CRPOLICY(policy_data) = cache_policy_brrip;
    BRRIP_DATA_DRPOLICY(policy_data) = cache_policy_brrip;
    
#undef MAX_RRPV
#undef CACHE_ALLOC
}

/* Returns SARP stream corresponding to access stream based on stream 
 * classification algoritm. */
sarp_stream get_sarp_stream(sarp_gdata *global_data, ub1 stream_in, 
  ub1 pid_in, memory_trace *info)
{
  return sarp_stream_u;
}

/* Public interface to initialize SARP statistics */
void cache_init_sarp_stats(sarp_stats *stats, char *file_name)
{
  if (!stats->stat_file)
  {
    /* Open statistics file */
    if (!file_name)
    {
      stats->stat_file = gzopen("PC-SARP-stats.trace.csv.gz", "wb9");
    }
    else
    {
      stats->stat_file = gzopen(file_name, "wb9");
    }

    assert(stats->stat_file);

    stats->sarp_x_fill       = 0;
    stats->sarp_y_fill       = 0;
    stats->sarp_p_fill       = 0;
    stats->sarp_q_fill       = 0;
    stats->sarp_r_fill       = 0;
    stats->sarp_x_access     = 0;
    stats->sarp_y_access     = 0;
    stats->sarp_p_access     = 0;
    stats->sarp_q_access     = 0;
    stats->sarp_r_access     = 0;
    stats->sarp_x_evct       = 0;
    stats->sarp_y_evct       = 0;
    stats->sarp_p_evct       = 0;
    stats->sarp_q_evct       = 0;
    stats->sarp_r_evct       = 0;
    stats->sarp_psetrecat    = 0;
    stats->hdr_printed            = FALSE;
    stats->next_schedule          = 0;
    stats->schedule_period        = 500 * 1024;

    for (ub4 i = 0; i <= TST; i++)
    {
      stats->sarp_x_realsrrip[i] = 0;
      stats->sarp_x_realbrrip[i] = 0;
      stats->sarp_speedup[i]     = 0;
    }
  }
}

/* Public interface to finalize SARP statistic */
void cache_fini_sarp_stats(sarp_stats *stats)
{
  gzclose(stats->stat_file);
}

/* Public interface to update SARP sats on fill */
void cache_update_sarp_fill_stats(sarp_gdata *global_data, 
  sarp_stream sstream)
{
  sarp_stats   *stats;

  /* Obtain SARP specific stream */
  assert(sstream <= global_data->sarp_streams);
  
  /* Get SARP statistics */
  stats = &(global_data->stats);
  assert(stats);
  
  /* Update stream specific stats */
  switch (sstream)
  {
    case sarp_stream_x:
      stats->sarp_x_fill += 1;
      break;
    
    case sarp_stream_y:
      stats->sarp_y_fill += 1;
      break;

    case sarp_stream_p:
      stats->sarp_p_fill += 1;
      break;

    case sarp_stream_q:
      stats->sarp_q_fill += 1;
      break;

    case sarp_stream_r:
      stats->sarp_r_fill += 1;
      break;

    default:
      panic("Invalid SARP stream function %s line %d\n", __FUNCTION__, __LINE__);
  }
}

/* Public interface to update SARP sats on fill */
void cache_update_sarp_realfill_stats(sarp_gdata *global_data, 
  sarp_stream sstream, enum cache_policy_t fill_policy, 
  memory_trace *info)
{
  sarp_stats   *stats;

  /* Obtain SARP specific stream */
  assert(sstream <= global_data->sarp_streams);
  assert(info->stream <= TST);

  /* Get SARP statistics */
  stats = &(global_data->stats);
  assert(stats);
  
  /* Update stream specific stats */
  switch (sstream)
  {
    case sarp_stream_x:
      if (fill_policy == cache_policy_brrip)
      {
        stats->sarp_x_realbrrip[info->stream] += 1;
      }
      else
      {
        stats->sarp_x_realsrrip[info->stream] += 1;
      }
      break;
    
    case sarp_stream_y:
    case sarp_stream_p:
    case sarp_stream_q:
    case sarp_stream_r:
      break;

    default:
      panic("Invalid SARP stream function %s line %d\n", __FUNCTION__, __LINE__);
  }
}

/* Public interface to update SARP stats on hit. */
void cache_update_sarp_access_stats(sarp_gdata *global_data, sarp_stream sstream)
{
  sarp_stats   *stats;
  
  /* Obtain SARP specific stream */
  assert(sstream <= global_data->sarp_streams);
  
  /* Obtain SARP stats */
  stats = &(global_data->stats);
  assert(stats);
  
  /* As per stream update access count. */
  switch (sstream)
  {
    case sarp_stream_x:
      stats->sarp_x_access += 1;
      break;
    
    case sarp_stream_y:
      stats->sarp_y_access += 1;
      break;

    case sarp_stream_p:
      stats->sarp_p_access += 1;
      break;

    case sarp_stream_q:
      stats->sarp_q_access += 1;
      break;

    case sarp_stream_r:
      stats->sarp_r_access += 1;
      break;

    default:
      panic("Invalid SARP stream function %s line %d\n", __FUNCTION__, __LINE__);
  }
}

/* Public interface to update SARP sats on replacement */
void cache_update_sarp_replacement_stats(sarp_gdata *global_data, 
  sarp_stream sstream)
{
  sarp_stats   *stats;

  /* Obtain SARP specific stream */
  assert(sstream <= global_data->sarp_streams);
  
  /* Get SARP statistics */
  stats = &(global_data->stats);
  assert(stats);
  
  /* Update stream specific stats */
  switch (sstream)
  {
    case sarp_stream_x:
      stats->sarp_x_evct += 1;
      break;
    
    case sarp_stream_y:
      stats->sarp_y_evct += 1;
      break;

    case sarp_stream_p:
      stats->sarp_p_evct += 1;
      break;

    case sarp_stream_q:
      stats->sarp_q_evct += 1;
      break;

    case sarp_stream_r:
      stats->sarp_r_evct += 1;
      break;

    default:
      panic("Invalid SARP stream function %s line %d\n", __FUNCTION__, __LINE__);
  }
}

/* Public interface to dump periodic SARP statistics */
void cache_dump_sarp_stats(sarp_stats *stats, ub8 cycle)
{
  /* Print header if not already done */
  if (stats->hdr_printed == FALSE)
  {
    gzprintf(stats->stat_file,
        "CYCLE; XFILL; YFILL; PFILL; QFILL; RFILL; " 
        "XEVCT; YEVCT; PEVCT; QEVCT; REVCT; "
        " XACCESS; YACCESS; PACCESS; QACCESS; RACCESS; RECAT; RSRRIPC;"
        "RBRRIPC; RSRRIPT; RBRRIPT; RSRRIPZ; RBRRIPZ; SPEEDUPC; SPEEDUPZ; SPEEDUPT\n");
    stats->hdr_printed = TRUE;
  }
  
  /* Dump current counter values */
  gzprintf(stats->stat_file,
      "%ld; %ld; %ld; %ld; %ld; %ld; "
      "%ld; %ld; %ld; %ld; %ld; "
      "%ld; %ld; %ld; %ld; %ld; %d; %d; %d; %d; %d; %d; %d; %d; %d; %d\n", cycle, stats->sarp_x_fill, 
      stats->sarp_y_fill, stats->sarp_p_fill, stats->sarp_q_fill,
      stats->sarp_r_fill, stats->sarp_x_evct, stats->sarp_y_evct,
      stats->sarp_p_evct, stats->sarp_q_evct, stats->sarp_r_evct,
      stats->sarp_x_access, stats->sarp_y_access, stats->sarp_p_access,
      stats->sarp_q_access, stats->sarp_r_access, stats->sarp_psetrecat,
      stats->sarp_x_realsrrip[CS], stats->sarp_x_realbrrip[CS], 
      stats->sarp_x_realsrrip[TS], stats->sarp_x_realbrrip[TS],
      stats->sarp_x_realsrrip[ZS], stats->sarp_x_realbrrip[ZS], 
      stats->sarp_speedup[CS], stats->sarp_speedup[ZS], stats->sarp_speedup[TS]);
  
  /* Reset all stat counters */
  stats->sarp_x_fill     = 0;
  stats->sarp_y_fill     = 0;
  stats->sarp_p_fill     = 0;
  stats->sarp_q_fill     = 0;
  stats->sarp_r_fill     = 0;
  stats->sarp_x_access   = 0;
  stats->sarp_y_access   = 0;
  stats->sarp_p_access   = 0;
  stats->sarp_q_access   = 0;
  stats->sarp_r_access   = 0;
  stats->sarp_x_evct     = 0;
  stats->sarp_y_evct     = 0;
  stats->sarp_p_evct     = 0;
  stats->sarp_q_evct     = 0;
  stats->sarp_r_evct     = 0;
  stats->sarp_psetrecat  = 0;

  for (ub4 i = 0; i <= TST; i++)
  {
    stats->sarp_x_realsrrip[i] = 0;
    stats->sarp_x_realbrrip[i] = 0;
    stats->sarp_speedup[i]     = 0;
  }
}

#define BLK_PER_ENTRY (64)

void sampler_cache_init(sampler_cache *sampler, ub4 sets, ub4 ways)
{
  assert(sampler);
  assert(sampler->blocks == NULL);
  
  /* Initialize sampler */
  sampler->sets           = sets;
  sampler->ways           = ways;
  sampler->entry_size     = BLK_PER_ENTRY;
  sampler->log_grain_size = LOG_GRAIN_SIZE;
  sampler->log_block_size = LOG_BLOCK_SIZE;
  sampler->epoch_length   = 0;
  
  /* Allocate sampler sets */ 
  sampler->blocks = (sampler_entry **)xcalloc(sets, sizeof(sampler_entry *));
  assert(sampler->blocks);  

  for (ub4 i = 0; i < sets; i++)
  {
    (sampler->blocks)[i] = (sampler_entry *)xcalloc(ways, sizeof(sampler_entry));
    assert((sampler->blocks)[i]);

    for (ub4 j = 0; j < ways; j++)
    {
      (sampler->blocks)[i][j].page           = SARP_SAMPLER_INVALID_TAG;
      (sampler->blocks)[i][j].timestamp      = (ub8 *)xcalloc(BLK_PER_ENTRY, sizeof(ub8));
      (sampler->blocks)[i][j].spill_or_fill  = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].stream         = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].valid          = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].hit_count      = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_color  = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_depth  = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_blit   = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_proc   = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
    }
  }
  
  /* Initialize sampler performance counters */
  memset(&(sampler->perfctr), 0 , sizeof(sampler_perfctr));
  memset(sampler->stream_occupancy, 0 , sizeof(ub4) * (TST + 1));
}

void sampler_cache_reset(sampler_cache *sampler)
{
  int sampler_occupancy;

  assert(sampler);
  
  sampler_occupancy = 0;
  
  printf("[C] F:%5d FR: %5d S: %5d SR: %5d\n", sampler->perfctr.fill_count[CS], 
      sampler->perfctr.fill_reuse_count[CS], sampler->perfctr.spill_count[CS],
      sampler->perfctr.spill_reuse_count[CS]);

  printf("[Z] F:%5d R: %5d S: %5d SR: %5d\n", sampler->perfctr.fill_count[ZS], 
      sampler->perfctr.fill_reuse_count[ZS], sampler->perfctr.spill_count[ZS],
      sampler->perfctr.spill_reuse_count[ZS]);

  printf("[B] F:%5d R: %5d S: %5d SR: %5d\n", sampler->perfctr.fill_count[BS], 
      sampler->perfctr.fill_reuse_count[BS], sampler->perfctr.spill_count[BS],
      sampler->perfctr.spill_reuse_count[BS]);

  printf("[T] F:%5d R: %5d\n", sampler->perfctr.fill_count[TS], 
      sampler->perfctr.fill_reuse_count[TS]);

  printf("[C] FDLOW:%5d FDHIGH: %5d SDLOW:%5d SDHIGH: %5d\n", sampler->perfctr.fill_reuse_distance_low[CS], 
      sampler->perfctr.fill_reuse_distance_high[CS], sampler->perfctr.spill_reuse_distance_low[CS],
      sampler->perfctr.spill_reuse_distance_high[CS]);

  printf("[Z] FDLOW:%5d FDHIGH: %5d SDLOW:%5d SDHIGH: %5d\n", sampler->perfctr.fill_reuse_distance_low[ZS], 
      sampler->perfctr.fill_reuse_distance_high[ZS], sampler->perfctr.spill_reuse_distance_low[ZS],
      sampler->perfctr.spill_reuse_distance_high[ZS]);

  printf("[B] FDLOW:%5d FDHIGH: %5d SDLOW:%5d SDHIGH: %5d\n", sampler->perfctr.fill_reuse_distance_low[BS], 
      sampler->perfctr.fill_reuse_distance_high[BS], sampler->perfctr.spill_reuse_distance_low[BS],
      sampler->perfctr.spill_reuse_distance_high[BS]);

  printf("[T] SDLOW:%5d SDHIGH: %5d\n", sampler->perfctr.fill_reuse_distance_low[TS], 
      sampler->perfctr.fill_reuse_distance_high[TS]);

  /* Reset sampler */
  for (ub4 i = 0; i < sampler->sets; i++)
  {
    for (ub4 j = 0; j < sampler->ways; j++)
    {
      if ((sampler->blocks)[i][j].page != SARP_SAMPLER_INVALID_TAG)
      {
        sampler_occupancy++;
      }

      (sampler->blocks)[i][j].page = SARP_SAMPLER_INVALID_TAG;

      for (ub4 off = 0; off < BLK_PER_ENTRY; off++)
      {
#if 0
        (sampler->blocks)[i][j].timestamp[off]      = 0;
#endif
        (sampler->blocks)[i][j].spill_or_fill[off]  = FALSE;
        (sampler->blocks)[i][j].stream[off]         = NN;
        (sampler->blocks)[i][j].valid[off]          = 0;
        (sampler->blocks)[i][j].hit_count[off]      = 0;
        (sampler->blocks)[i][j].dynamic_color[off]  = 0;
        (sampler->blocks)[i][j].dynamic_depth[off]  = 0;
        (sampler->blocks)[i][j].dynamic_blit[off]   = 0;
        (sampler->blocks)[i][j].dynamic_proc[off]   = 0;
      }
    }
  }
   
  /* Halve sampler performance counters */
  sampler->perfctr.max_reuse /= 2;

  for (ub1 strm = NN; strm <= TST; strm++)
  {
    sampler->perfctr.fill_count[strm]                 /= 2;
    sampler->perfctr.spill_count[strm]                /= 2;
    sampler->perfctr.fill_reuse_count[strm]           /= 2;
    sampler->perfctr.spill_reuse_count[strm]          /= 2;
    sampler->perfctr.fill_reuse_distance_high[strm]   /= 2;
    sampler->perfctr.fill_reuse_distance_low[strm]    /= 2;
    sampler->perfctr.spill_reuse_distance_high[strm]  /= 2;
    sampler->perfctr.spill_reuse_distance_low[strm]   /= 2;

    sampler->perfctr.fill_count_epoch[strm]            = 0;
    
    for (ub1 ep = 0; ep < REPOCH_CNT; ep++)
    {
      sampler->perfctr.fill_count_per_reuse_epoch[strm][ep] /= 2;
      sampler->perfctr.fill_reuse_per_reuse_epoch[strm][ep] /= 2;
      sampler->perfctr.fill_reuse_distance_high_per_reuse_epoch[strm][ep] /= 2;
      sampler->perfctr.fill_reuse_distance_low_per_reuse_epoch[strm][ep]  /= 2;
    }
  }
  
  for (ub1 strm = NN; strm <= TST; strm++)
  {
    sampler->stream_occupancy[strm] = 0;
  }
  
  printf("Sampler occupancy [%5d] epoch [%3ld]\n", sampler_occupancy, sampler->epoch_count);

  sampler->epoch_count += 1;
}

#undef BLK_PER_ENTRY

#define FDLOW(p, s)       (SMPLRPERF_FDLOW(p, s))
#define FDHIGH(p, s)      (SMPLRPERF_FDHIGH(p, s))
#define FCOUNT(p, s)      (SMPLRPERF_FILL(p, s))
#define FREUSE(p, s)      (SMPLRPERF_FREUSE(p, s))
#define FRD_LOW(p, s)     (FDLOW(p, s) < FDHIGH(p, s))
#define FREUSE_LOW(p, s)  (FREUSE(p, s) == 0 && FCOUNT(p, s) > BYPASS_ACCESS_TH)
#define FBYPASS_TH(p, s)  (FREUSE(p, s) * BYTH_DENM <= FCOUNT(p, s) * BYTH_NUMR)
#define SDLOW(p, s)       (SMPLRPERF_SDLOW(p, s))
#define SDHIGH(p, s)      (SMPLRPERF_SDHIGH(p, s))
#define SCOUNT(p, s)      (SMPLRPERF_SPILL(p, s))
#define SREUSE(p, s)      (SMPLRPERF_SREUSE(p, s))
#define SRD_LOW(p, s)     (SDLOW(p, s) < SDHIGH(p, s))
#define SREUSE_LOW(p, s)  (SREUSE(p, s) == 0 && SCOUNT(p, s) > BYPASS_ACCESS_TH)
#define SBYPASS_TH(p, s)  (SREUSE(p, s) * BYTH_DENM <= SCOUNT(p, s) * BYTH_NUMR)

static ub1 is_fill_bypass(sampler_cache *sampler, memory_trace *info)
{
  ub1 bypass;

  bypass = FALSE;
  
  /* Only GPU fill/spill are bypassed */
  if (CPU_STREAM(info->stream) == FALSE)
  {
    if (info->fill == TRUE)
    {
      if (FRD_LOW(&(sampler->perfctr), info->stream) || 
          FREUSE_LOW(&(sampler->perfctr), info->stream))
      {
        if (FBYPASS_TH(&(sampler->perfctr), info->stream))
          bypass = TRUE;  
      }
    }
    else
    {
      if (SRD_LOW(&(sampler->perfctr), info->stream) || 
          SREUSE_LOW(&(sampler->perfctr), info->stream))
      {
        if (SBYPASS_TH(&(sampler->perfctr), info->stream))
          bypass = TRUE;  
      }
    }
  }

  return bypass;
}

#undef FDLOW
#undef FDHIGH
#undef FCOUNT
#undef FRUSE
#undef FRD_LOW
#undef FREUSE_LOW
#undef SDLOW
#undef SDHIGH
#undef SCOUNT
#undef SRUSE
#undef SRD_LOW
#undef SREUSE_LOW

#define SCOUNT(p, s)    (SMPLRPERF_SPILL(p, s))
#define SRUSE(p, s)     (SMPLRPERF_SREUSE(p, s))
#define MRUSE(p)        (SMPLRPERF_MREUSE(p))
#define SHPIN_TH(p, s)  (SRUSE(p, s) * SHTH_DENM > SCOUNT(p, s) * SHTH_NUMR)
#define SMPIN_TH(p, s)  (SRUSE(p, s) * SMTH_DENM > SCOUNT(p, s) * SMTH_NUMR)
#define PINHBLK(p, s)   ((SRUSE(p, s) > MRUSE(p) / 2) || SHPIN_TH(p, s))
#define PINMBLK(p, s)   ((SRUSE(p, s) > MRUSE(p) / 2) || SMPIN_TH(p, s))

static ub1 is_hit_block_pinned(sampler_cache *sampler, memory_trace *info)
{
  if (info->spill == TRUE) 
  {
    if (PINHBLK(&(sampler->perfctr), info->stream)) 
      return TRUE;
  }

  return FALSE;
}

static ub1 is_miss_block_pinned(sampler_cache *sampler, memory_trace *info)
{
  if (info->spill == TRUE) 
  {
    if (PINMBLK(&(sampler->perfctr), info->stream)) 
      return TRUE;
  }

  return FALSE;
}

#undef SCOUNT
#undef SRUSE
#undef MRUSE
#undef SHPIN_TH
#undef SMPIN_TH
#undef PINHBLK
#undef PINMBLK

#define SCOUNT(p, s)      (SMPLRPERF_SPILL(p, s))
#define SRUSE(p, s)       (SMPLRPERF_SREUSE(p, s))
#define MRUSE(p)          (SMPLRPERF_MREUSE(p))
#define SDLOW(p, s)       (SMPLRPERF_SDLOW(p, s))
#define SDHIGH(p, s)      (SMPLRPERF_SDHIGH(p, s))
#define SMPIN_TH(p, s)    (SRUSE(p, s) * SMTH_DENM > SCOUNT(p, s) * SMTH_NUMR)
#define RRPV1(p, s)       ((SRUSE(p, s) > MRUSE(p) / 2) || SMPIN_TH(p, s))
#define RRPV2(p, s)       ((SRUSE(p, s) > MRUSE(p) / 3))
#define SRD_LOW(p, s)     (SDLOW(p, s) < SDHIGH(p, s))
#define SREUSE_LOW(p, s)  (SREUSE(p, s) == 0 && SCOUNT(p, s) > BYPASS_ACCESS_TH)

int cache_get_fill_rrpv_sarp(sarp_data *policy_data, sarp_gdata *global_data,
    memory_trace *info)
{
  sampler_perfctr *perfctr;
  int ret_rrpv;
  
  assert(policy_data);
  assert(global_data);
  assert(info);

  /* Ensure current policy is sarp */
  assert(info->spill == TRUE && info->fill == FALSE);
  
  perfctr   = &((global_data->sampler)->perfctr);
  ret_rrpv  = 2;

  if (RRPV1(perfctr, info->stream) || RRPV2(perfctr, info->stream))
  {
    ret_rrpv = 0;
  }
  else
  {
    if (SRD_LOW(perfctr, info->stream) || SREUSE_LOW(perfctr, info->stream))
    {
      ret_rrpv = 3;
    }
  }
  
  return ret_rrpv;
}

#undef SCOUNT
#undef SRUSE
#undef MRUSE
#undef SDLOW
#undef SDHIGH
#undef SMPIN_TH
#undef RRPV1
#undef RRPV2
#undef SRD_LOW
#undef SREUSE_LOW

int cache_get_replacement_rrpv_sarp(sarp_data *policy_data)
{
  return SARP_DATA_MAX_RRPV(policy_data);
}

#define SCOUNT(p, s)      (SMPLRPERF_SPILL(p, s))
#define FCOUNT(p, s, e)   (SMPLRPERF_FILL_RE(p, s, e))
#define SRUSE(p, s)       (SMPLRPERF_SREUSE(p, s))
#define FRUSE(p, s, e)    (SMPLRPERF_FREUSE_RE(p, s, e))
#define MRUSE(p)          (SMPLRPERF_MREUSE(p))
#define SDLOW(p, s)       (SMPLRPERF_SDLOW(p, s))
#define SDHIGH(p, s)      (SMPLRPERF_SDHIGH(p, s))
#define SHPIN_TH(p, s)    (SRUSE(p, s) * SHTH_DENM > SCOUNT(p, s) * SHTH_NUMR)
#define RRPV1(p, s)       ((SRUSE(p, s) > MRUSE(p) / 2) || SHPIN_TH(p, s))
#define RRPV2(p, s)       ((SRUSE(p, s) > MRUSE(p) / 3))
#define SRD_LOW(p, s)     (SDLOW(p, s) < SDHIGH(p, s))
#define SREUSE_LOW(p, s)  (SREUSE(p, s) == 0 && SCOUNT(p, s) > BYPASS_ACCESS_TH)
#define CT_TH1            (64)
#define BT_TH1            (32)
#define TH2               (2)

int cache_get_new_rrpv_sarp(sarp_data *policy_data, sarp_gdata *global_data, 
    memory_trace *info, struct cache_block_t *block)
{
  int ret_rrpv;
  int epoch;

  sampler_perfctr *perfctr;
  
  ret_rrpv  = 0;
  perfctr   = &((global_data->sampler)->perfctr);
  epoch     = block->epoch;

  if (info->spill == TRUE)
  {
    if (RRPV1(perfctr, info->stream) || RRPV2(perfctr, info->stream))
    {
      ret_rrpv = 0; 
    }
    else
    {
      if (SRD_LOW(perfctr, info->stream) || SREUSE_LOW(perfctr, info->stream))
      {
        ret_rrpv = 3;
      }
      else
      {
        ret_rrpv = 2;
      }
    }
  }
  else
  {
    assert(info->fill == TRUE);
  
    if (block->is_ct_block)
    {
      if (FRUSE(perfctr, info->stream, epoch) * CT_TH1 <= FCOUNT(perfctr, info->stream, epoch))
      {
        ret_rrpv = 3;
      }
      else
      {
        if (FRUSE(perfctr, info->stream, epoch) * TH2 <= FCOUNT(perfctr, info->stream, epoch))
        {
          ret_rrpv = 2;
        }
      }
    }
    else
    {
      if (block->is_bt_block)
      {
        if (FRUSE(perfctr, info->stream, epoch) * BT_TH1 <= FCOUNT(perfctr, info->stream, epoch))
        {
          ret_rrpv = 3;
        }
        else
        {
          if (FRUSE(perfctr, info->stream, epoch) * TH2 <= FCOUNT(perfctr, info->stream, epoch))
          {
            ret_rrpv = 2;
          }
        }
      }
    }
  }

  return ret_rrpv;
}

#undef SCOUNT
#undef SRUSE
#undef FRUSE
#undef MRUSE
#undef SDLOW
#undef SHPIN_TH
#undef RRPV1
#undef RRPV2
#undef SRD_LOW
#undef SREUSE_LOW
#undef CT_TH1
#undef BT_TH1
#undef TH2

void cache_init_sarp(long long int set_indx, struct cache_params *params,
    sarp_data *policy_data, sarp_gdata *global_data)
{
  ub4 set_type;

  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);
  
  srand(0);

  /* Obtain current set type */
#define SARP_SAMPLES (4)  

  set_type = get_set_type_sarp(set_indx, SARP_SAMPLES);

#undef SARP_SAMPLES

  /* Get set type */
  if (set_indx == 0)
  {
    /* Initialize DRRIP policy selection counter */
    SAT_CTR_INI(global_data->srrip_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->srrip_psel, PSEL_MID_VAL);
    
    /* Initialize DRRIP policy selection counter */
    SAT_CTR_INI(global_data->brrip_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->brrip_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->sarp_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->sarp_psel, PSEL_MID_VAL);
    
    /* Initialize per-stream policy selection counter */
    for (ub1 i = 0; i <= TST; i++)
    {
      SAT_CTR_INI(global_data->sarp_ssel[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_SET(global_data->sarp_ssel[i], PSEL_MID_VAL);
    }

    /* Initialize SARP specific statistics */
    cache_init_sarp_stats(&(global_data->stats), NULL);
    
    /* Initialize cache-wide data for SARP. */
    global_data->threshold            = params->threshold;
    global_data->sarp_streams         = params->sdp_streams;

    /* Initialize bimodal access counter */
    SAT_CTR_INI(global_data->access_ctr, 8, 0, 255);

    SAT_CTR_INI(global_data->brrip.access_ctr, 8, 0, 255);
    global_data->brrip.threshold  = params->threshold; 

    /* Allocate and initialize sampler cache */
    global_data->sampler = (sampler_cache *)xcalloc(1, sizeof(sampler_cache));
    assert(global_data->sampler);  

    sampler_cache_init(global_data->sampler, params->sampler_sets, 
        params->sampler_ways);

    for (ub1 i = 0; i < SARP_SAMPLE_COUNT; i++)
    {
      SAT_CTR_INI(global_data->baccess[i], 8, 0, 255);
    }
  }
  
  /* Allocate and initialize per set data */
#define MAX_RRPV (params->max_rrpv)

  /* Set max RRPV for the set */
  SARP_DATA_MAX_RRPV(policy_data)  = MAX_RRPV;
  SARP_DATA_SET_TYPE(policy_data)  = set_type;

#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  SARP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SARP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

  assert(SARP_DATA_VALID_HEAD(policy_data));
  assert(SARP_DATA_VALID_TAIL(policy_data));

#undef MEM_ALLOC
  
  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SARP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    SARP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    SARP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  SARP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

  SARP_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  SARP_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);

#undef MEM_ALLOC

  /* Initialize array of blocks */
  SARP_DATA_FREE_HEAD(policy_data) = &(SARP_DATA_BLOCKS(policy_data)[0]);
  SARP_DATA_FREE_TAIL(policy_data) = &(SARP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SARP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SARP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SARP_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SARP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SARP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SARP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }
  
  /* Allocate pointer table for per core per stream policy and initialize it to SARP. */
#define TOTAL_PSPOLICY(g) (TST + 1)

  SARP_DATA_PSPOLICY(policy_data) = 
    (cache_policy_t *)xcalloc(TOTAL_PSPOLICY(global_data), sizeof(cache_policy_t));
  assert(SARP_DATA_PSPOLICY(policy_data));

#undef TOTAL_PSPOLICY

  for (int stream = 0; stream <= TST; stream++)
  {
    if (CPU_STREAM(stream))
    {
      switch (set_type)
      {
        case SARP_CSRRIP_GSRRIP_SET:
        case SARP_CSRRIP_GBRRIP_SET:
          SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_srrip; 
          break;

        case SARP_CBRRIP_GSRRIP_SET:
        case SARP_CBRRIP_GBRRIP_SET:
          SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_brrip; 
          break;

        case SARP_FOLLOWER_SET:
          SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_sarp; 
          break;

        default:
          panic("%s: line no %d - invalid sample type", __FUNCTION__, __LINE__);
      }
    }
    else
    {
      switch (set_type)
      {
        case SARP_CSRRIP_GSRRIP_SET:
        case SARP_CBRRIP_GSRRIP_SET:
          SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_srrip; 
          break;

        case SARP_CBRRIP_GBRRIP_SET:
        case SARP_CSRRIP_GBRRIP_SET:
          SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_brrip; 
          break;

        case SARP_FOLLOWER_SET:
          SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_sarp; 
          break;

        default:
          panic("%s: line no %d - invalid sample type", __FUNCTION__, __LINE__);
      }
    }
  }
  
  /* Initialize SRRIP component */
  cache_init_srrip_from_sarp(params, &(policy_data->srrip), policy_data);
  cache_init_brrip_from_sarp(params, &(policy_data->brrip), policy_data);
  
  /* Ensure RRPV is correctly set */
  assert(SARP_DATA_MAX_RRPV(policy_data) != 0);

#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_sarp(long long int set_indx, sarp_data *policy_data,
    sarp_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Free all data blocks */
  free(SARP_DATA_BLOCKS(policy_data));

  /* Free per-stream policy */
  free(SARP_DATA_PSPOLICY(policy_data));

  /* Free valid head buckets */
  if (SARP_DATA_VALID_HEAD(policy_data))
  {
    free(SARP_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SARP_DATA_VALID_TAIL(policy_data))
  {
    free(SARP_DATA_VALID_TAIL(policy_data));
  }

  /* Clean up SARP stats */
  if (set_indx == 0)
  {
    cache_fini_sarp_stats(&(global_data->stats));

    printf("Bypass [%5d] MAX_REUSE[%5d]\n", global_data->bypass_count, global_data->sampler->perfctr.max_reuse);

    sampler_cache_reset(global_data->sampler);
  }
}

/* Get RRPV block to be filled */
enum cache_policy_t cache_get_fill_policy_sarp(enum cache_policy_t current,
    sarp_stream stream)
{
  switch (stream)
  {
    case sarp_stream_x:
      return current;

    case sarp_stream_y:
      return current;

    case sarp_stream_p:
      return cache_policy_brrip;

    case sarp_stream_q:
    case sarp_stream_r:
      return current;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }
}

struct cache_block_t * cache_find_block_sarp(sarp_data *policy_data, 
    sarp_gdata *global_data, long long tag, memory_trace *info)
{
  int    max_rrpv;
  struct cache_block_t *head;
  struct cache_block_t *node;

  max_rrpv  = policy_data->srrip.max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SARP_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

end:
   
  if (info)
  {
    info->sap_stream = get_sarp_stream(global_data, info->stream, info->pid, info);
    /* Update sampler epoch length */

    if (info->fill)
    {
      if (++((global_data->sampler)->epoch_length) == EPOCH_SIZE)
      {
        sampler_cache_reset(global_data->sampler);
        
        /* Reset epoch length */
        (global_data->sampler)->epoch_length = 0;
      }
    }
  }
  
  if (!node)
  {
    policy_data->miss_count += 1;
  }

  sampler_cache_lookup(global_data->sampler, policy_data, info);

  return node;
}

/* Fill new block. Basically, block to be filled is already chosen, 
 * This function only updates tag and state bits. */
void cache_fill_block_sarp(sarp_data *policy_data, 
    sarp_gdata *global_data, int way, long long tag, 
    enum cache_block_state_t state, int stream, memory_trace *info)
{
  sarp_stream  sstream;
  
  struct cache_block_t  *block;
  enum   cache_policy_t *per_stream_policy;  
  enum   cache_policy_t  current_policy;  
  enum   cache_policy_t  following_policy;  

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);
  
  /* Obtain SAP stream */
  sstream = get_sarp_stream(global_data, info->stream, info->pid, info); 
  
  info->sap_stream = sstream;

  /* Get per-stream policy */
  per_stream_policy = SARP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);

  following_policy  = FOLLOW_POLICY(per_stream_policy[info->stream], info);
  
  assert(FSRRIP(following_policy) || FBRRIP(following_policy) || FSARP(following_policy));
  
  /* 
   * Update SARP counter
   *
   *  The counter is used to decide whether to follow DRRIP or 
   *  modified policy. The counter is incremented whenever there is
   *  a miss in the sample to be followed by DRRIP and it is decremented 
   *  everytime there is a miss in the policy sample.
   *
   **/
  
  /* Update sampler set counter only for reads */
  if (info->fill == TRUE)
  {
    switch (policy_data->set_type)
    {
      case SARP_CSRRIP_GSRRIP_SET:
        assert(following_policy == cache_policy_srrip || 
            following_policy == cache_policy_sarp);

        SAT_CTR_INC(global_data->srrip_psel);

        if (SAT_CTR_VAL(global_data->srrip_psel) <= PSEL_MID_VAL)
        {
          SAT_CTR_INC(global_data->sarp_psel);
        }
        break;

      case SARP_CBRRIP_GSRRIP_SET:
        if (CPU_STREAM(info->stream))
        {
          assert(following_policy == cache_policy_brrip || 
              following_policy == cache_policy_sarp);

          /* Update brrip epsilon counter only for graphics */
#define CTR_VAL(g)    (SAT_CTR_VAL((g)->baccess[SARP_CBRRIP_GSRRIP_SET]))
#define THRESHOLD(g)  ((g)->threshold)

          /* Update access counter for bimodal insertion */
          if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
          {
            /* Increment set access count */
            SAT_CTR_INC(global_data->baccess[SARP_CBRRIP_GSRRIP_SET]);
          }
          else
          {
            /* Reset access count */
            SAT_CTR_RST(global_data->baccess[SARP_CBRRIP_GSRRIP_SET]);
          }

          /* Set new value to brrip policy global data */
          SAT_CTR_VAL(global_data->brrip.access_ctr) = CTR_VAL(global_data);

#undef CTR_VAL
#undef THRESHOLD
        }
        else
        {
          assert(following_policy == cache_policy_srrip || 
              following_policy == cache_policy_sarp);
        }

        SAT_CTR_DEC(global_data->srrip_psel);

        if (SAT_CTR_VAL(global_data->srrip_psel) > PSEL_MID_VAL)
        {
          SAT_CTR_INC(global_data->sarp_psel);
        }
        break;

      case SARP_CSRRIP_GBRRIP_SET:
        if (CPU_STREAM(info->stream))
        {
          assert(following_policy == cache_policy_srrip || 
              following_policy == cache_policy_sarp);
        }
        else
        {
          assert(following_policy == cache_policy_brrip || 
              following_policy == cache_policy_sarp);

          /* Update brrip epsilon counter only for graphics */
#define CTR_VAL(g)    (SAT_CTR_VAL((g)->baccess[SARP_CSRRIP_GBRRIP_SET]))
#define THRESHOLD(g)  ((g)->threshold)

          /* Update access counter for bimodal insertion */
          if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
          {
            /* Increment set access count */
            SAT_CTR_INC(global_data->baccess[SARP_CSRRIP_GBRRIP_SET]);
          }
          else
          {
            /* Reset access count */
            SAT_CTR_RST(global_data->baccess[SARP_CSRRIP_GBRRIP_SET]);
          }

          /* Set new value to brrip policy global data */
          SAT_CTR_VAL(global_data->brrip.access_ctr) = CTR_VAL(global_data);

#undef CTR_VAL
#undef THRESHOLD
        }

        SAT_CTR_INC(global_data->brrip_psel); 

        if (SAT_CTR_VAL(global_data->brrip_psel) <= PSEL_MID_VAL)
        {
          SAT_CTR_DEC(global_data->sarp_psel);
        }
        break;

      case SARP_CBRRIP_GBRRIP_SET:
        assert(following_policy == cache_policy_brrip || 
            following_policy == cache_policy_sarp);

        SAT_CTR_DEC(global_data->brrip_psel); 

        if (SAT_CTR_VAL(global_data->brrip_psel) > PSEL_MID_VAL)
        {
          SAT_CTR_DEC(global_data->sarp_psel);
        }

        /* Update brrip epsilon counter only for graphics */
#define CTR_VAL(g)    (SAT_CTR_VAL((g)->baccess[SARP_CBRRIP_GBRRIP_SET]))
#define THRESHOLD(g)  ((g)->threshold)

        /* Update access counter for bimodal insertion */
        if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
        {
          /* Increment set access count */
          SAT_CTR_INC(global_data->baccess[SARP_CBRRIP_GBRRIP_SET]);
        }
        else
        {
          /* Reset access count */
          SAT_CTR_RST(global_data->baccess[SARP_CBRRIP_GBRRIP_SET]);
        }

        /* Set new value to brrip policy global data */
        SAT_CTR_VAL(global_data->brrip.access_ctr) = CTR_VAL(global_data);

#undef CTR_VAL
#undef THRESHOLD
        break;

      case SARP_FOLLOWER_SET:
        /* Update brrip epsilon counter only for graphics */
        if (CPU_STREAM(info->stream))
        {
#define CTR_VAL(g)    (SAT_CTR_VAL((g)->baccess[SARP_FOLLOWER_CPU_SET]))
#define THRESHOLD(g)  ((g)->threshold)

          /* Update access counter for bimodal insertion */
          if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
          {
            /* Increment set access count */
            SAT_CTR_INC(global_data->baccess[SARP_FOLLOWER_CPU_SET]);
          }
          else
          {
            /* Reset access count */
            SAT_CTR_RST(global_data->baccess[SARP_FOLLOWER_CPU_SET]);
          }

          /* Set new value to brrip policy global data */
          SAT_CTR_VAL(global_data->brrip.access_ctr) = CTR_VAL(global_data);

#undef CTR_VAL
#undef THRESHOLD
        }
        else
        {
#define CTR_VAL(g)    (SAT_CTR_VAL((g)->baccess[SARP_FOLLOWER_GPU_SET]))
#define THRESHOLD(g)  ((g)->threshold)

          /* Update access counter for bimodal insertion */
          if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
          {
            /* Increment set access count */
            SAT_CTR_INC(global_data->baccess[SARP_FOLLOWER_GPU_SET]);
          }
          else
          {
            /* Reset access count */
            SAT_CTR_RST(global_data->baccess[SARP_FOLLOWER_GPU_SET]);
          }

          /* Set new value to brrip policy global data */
          SAT_CTR_VAL(global_data->brrip.access_ctr) = CTR_VAL(global_data);

#undef CTR_VAL
#undef THRESHOLD

        }

        break;

      default:
        panic("%s: line no %d - invalid sample type", __FUNCTION__, __LINE__);
    }
  }

  /* Get the policy based on policy selection counters */
  current_policy = CURRENT_POLICY(policy_data, global_data, following_policy, info, FALSE);
  
  if (way != BYPASS_WAY)
  {
    switch (current_policy)
    {
      case cache_policy_srrip:
        assert(way != BYPASS_WAY);

        cache_fill_block_srrip(&(policy_data->srrip), 
            &(global_data->srrip), way, tag, state, stream, info);

        /* Obtain block to update SARP stream, this is done to collect SARP 
         * specific stats on eviction */
        block = &(SRRIP_DATA_BLOCKS(policy_data)[way]);
        assert(block); 
        assert(block->stream == info->stream);
        assert(block->state != cache_block_invalid);
        break;

      case cache_policy_brrip:
        assert(way != BYPASS_WAY);

        cache_fill_block_brrip(&(policy_data->brrip), &(global_data->brrip), 
            way, tag, state, stream, info);

        /* Obtain block to update SARP stream, this is done to collect SARP 
         * specific stats on eviction */
        block = &(SRRIP_DATA_BLOCKS(policy_data)[way]);
        assert(block); 
        assert(block->stream == info->stream);
        assert(block->state != cache_block_invalid);
        break;

      case cache_policy_sarp:
        if (way != BYPASS_WAY)
        {
          int rrpv;

          /* Obtain SARP specific data */
          block = &(SARP_DATA_BLOCKS(policy_data)[way]);

          assert(block->stream == NN);

          /* Get RRPV to be assigned to the new block */
          rrpv = cache_get_fill_rrpv_sarp(policy_data, global_data, info);

          /* Ensure a valid RRPV */
          assert(rrpv >= 0 && rrpv <= policy_data->max_rrpv); 

          /* Remove block from free list */
          free_list_remove_block(policy_data, block);

          /* Update new block state and stream */
          CACHE_UPDATE_BLOCK_STATE(block, tag, state);
          CACHE_UPDATE_BLOCK_STREAM(block, info->stream);

          block->dirty      = (info && info->spill) ? TRUE : FALSE;
          block->last_rrpv  = rrpv;

          block->dirty  = (info && info->spill) ? TRUE : FALSE;
          block->epoch  = 0;

          if (info->spill == TRUE)
          {
            if (is_miss_block_pinned(global_data->sampler, info))
            {
              block->is_block_pinned = TRUE;
            }
          }

          /* If block is spilled, and block is to be pinned set the flag  */
          /* Insert block in to the corresponding RRPV queue */
          CACHE_APPEND_TO_QUEUE(block, 
              SARP_DATA_VALID_HEAD(policy_data)[rrpv], 
              SARP_DATA_VALID_TAIL(policy_data)[rrpv]);
        }

        break;

      default:
        panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
    }
  }
}

#define INVALID_WAYID  (~(0))

/* Get a replacement candidate */
int cache_replace_block_sarp(sarp_data *policy_data, sarp_gdata *global_data,
    memory_trace *info)
{
  enum cache_policy_t  *per_stream_policy;
  enum cache_policy_t   following_policy;
  enum cache_policy_t   current_policy;
  
  int     ret_wayid;
  int     min_rrpv;
  int     rrpv;
  struct  cache_block_t *block;

  /* Ensure vaid arguments */
  assert(global_data);
  assert(policy_data);
  
  ret_wayid = BYPASS_WAY;
  min_rrpv  = !SARP_DATA_MAX_RRPV(policy_data);

  per_stream_policy = SARP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);

  /* Get per stream policy map */
  following_policy = FOLLOW_POLICY(per_stream_policy[info->stream], info);

  assert(FSRRIP(following_policy) || FBRRIP(following_policy) || 
      FSARP(following_policy));

  /* Get the current policy based on policy selection counter */
  current_policy = CURRENT_POLICY(policy_data, global_data, following_policy, 
      info, FALSE);

  if (!is_fill_bypass(global_data->sampler, info))
  {
    switch (current_policy)
    {
      case cache_policy_srrip:
#if 0
        return cache_replace_block_srrip(&(policy_data->srrip), &(global_data->srrip));
#endif
      case cache_policy_brrip:
#if 0
        return cache_replace_block_brrip(&(policy_data->brrip));
#endif
      case cache_policy_sarp:
        /* Try to find an invalid block always from head of the free list. */
        for (block = SARP_DATA_FREE_HEAD(policy_data); block; block = block->prev)
          return block->way;

        /* If fill is not bypassed, get the replacement candidate */
        if (!is_fill_bypass(global_data->sampler, info))
        {
          /* Obtain RRPV from where to replace the block */
          rrpv = cache_get_replacement_rrpv_sarp(policy_data);

          /* Ensure rrpv is with in max_rrpv bound */
          assert(rrpv >= 0 && rrpv <= SARP_DATA_MAX_RRPV(policy_data));

          /* If there is no block with required RRPV, increment RRPV of all the blocks
           * until we get one with the required RRPV */
          while (SARP_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
          {
            /* All blocks which are already pinned are promoted to RRPV 0 
             * and are unpinned. So we iterate through the blocks at RRPV 3 
             * and move all the blocks which are pinned to RRPV 0 */
            CACHE_SARP_INCREMENT_RRPV(SARP_DATA_VALID_HEAD(policy_data), 
                SARP_DATA_VALID_TAIL(policy_data), rrpv);

#if 0
            for (block = SARP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
            {
              if (block->is_block_pinned == TRUE)
              {
                block->is_block_pinned = FALSE;

                /* Move block to min RRPV */
                CACHE_REMOVE_FROM_QUEUE(block, SARP_DATA_VALID_HEAD(policy_data)[rrpv],
                    SARP_DATA_VALID_TAIL(policy_data)[rrpv]);
                CACHE_APPEND_TO_QUEUE(block, SARP_DATA_VALID_HEAD(policy_data)[min_rrpv], 
                    SARP_DATA_VALID_TAIL(policy_data)[min_rrpv]);
              }
            }
#endif
          }

          /* Remove a nonbusy block from the tail */
          unsigned int min_wayid = ~(0);

          for (block = SARP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
          {
            if (!block->busy && block->way < min_wayid)
              min_wayid = block->way;
          }

          ret_wayid = (min_wayid != ~(0)) ? min_wayid : -1;
        }
        else
        {
          global_data->bypass_count++;
        }

        break;

      default:
        panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
        ret_wayid = 0;
    }
  }
  else
  {
    global_data->bypass_count++;
  }

  return ret_wayid;
}

#undef INVALID_WAYID

static void update_block_xstream(struct cache_block_t *block, memory_trace *info)
{
  if (block->stream == CS && info->stream == TS)
  {
    block->is_ct_block = TRUE;
  }

  if (block->stream == ZS && info->stream != ZS)
  {
    block->is_zt_block = TRUE; 
  }

  if (block->stream == BS && info->stream != BS)
  {
    block->is_bt_block = TRUE; 
  }
}

/* Update block state on hit */
void cache_access_block_sarp(sarp_data *policy_data, sarp_gdata *global_data,
    int way, int stream, memory_trace *info)
{
  struct  cache_block_t  *blk;
  enum    cache_policy_t *per_stream_policy;
  enum    cache_policy_t  following_policy;
  enum    cache_policy_t  current_policy;
  int     old_rrpv;
  int     new_rrpv;

  sarp_stream sstream;
  
  assert(policy_data);
  assert(global_data);
  assert(way != BYPASS_WAY);

  blk = NULL;
  
  /* Obtain per-stream policy */
  per_stream_policy = SARP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);

  sstream = get_sarp_stream(global_data, info->stream, info->pid, info);
  
  info->sap_stream = sstream;

  blk = &(SARP_DATA_BLOCKS(policy_data)[way]);
  assert(blk);

  old_rrpv  = 0;
  new_rrpv  = 0;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);

  following_policy = FOLLOW_POLICY(per_stream_policy[info->stream], info);

  assert(FSRRIP(following_policy) || FBRRIP(following_policy) || FSARP(following_policy));
  
  /* Obtain policy based on policy selection counter */
  current_policy = CURRENT_POLICY(policy_data, global_data, following_policy, 
      info, TRUE);

  switch (current_policy)
  {
    case cache_policy_srrip:
      cache_access_block_srrip(&(policy_data->srrip), &(global_data->srrip), 
          way, stream, info);
      break;

    case cache_policy_brrip:
      cache_access_block_brrip(&(policy_data->brrip), &(global_data->brrip), 
          way, stream, info);
      break;

    case cache_policy_sarp:
      /* Update inter stream flags */
      update_block_xstream(blk, info);
      
      /* Update block epoch */
      if (info && blk->stream == info->stream)
      {
#define MX_EP  (REPOCH_CNT)

        blk->epoch  = (blk->epoch < MX_EP) ? blk->epoch + 1 : blk->epoch;

#undef MX_EP
      }
      else
      {
        blk->epoch = 0;
      }

      /* Get old RRPV from the block */
      old_rrpv = (((rrip_list *)(blk->data))->rrpv);

      /* Get new RRPV using policy specific function */
      new_rrpv = cache_get_new_rrpv_sarp(policy_data, global_data, info, blk);

      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        CACHE_REMOVE_FROM_QUEUE(blk, SRRIP_DATA_VALID_HEAD(policy_data)[old_rrpv],
            SRRIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, SRRIP_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            SRRIP_DATA_VALID_TAIL(policy_data)[new_rrpv]);
      }

      blk->dirty = (info->spill) ? TRUE : FALSE;
      
      if (info->spill == TRUE)
      {
        if (is_hit_block_pinned(global_data->sampler, info))
        {
          blk->is_block_pinned = TRUE;
        }
      }
      
      CACHE_UPDATE_BLOCK_PC(blk, info->pc);
      CACHE_UPDATE_BLOCK_STREAM(blk, info->stream);
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }
}

/* Update state of the block. */
void cache_set_block_sarp(sarp_data *policy_data, sarp_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream,
  memory_trace *info)
{
  struct cache_block_t  *block;
  enum   cache_policy_t *per_stream_policy;
  enum   cache_policy_t  current_policy;
  enum   cache_policy_t  following_policy;

  sarp_stream sstream;

  block = &(SARP_DATA_BLOCKS(policy_data))[way];

  /* Incoming tag should match the current block tag. */
  assert(block->tag == tag);

  /* Obtian SAP stream */
  sstream = get_sarp_stream(global_data, info->stream, info->pid, info);

  /* Setting state of an invalid block is not allowed. */
  assert(block->state != cache_block_invalid);
  
  /* Obtain per stream policy list */
  per_stream_policy = SARP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);
  
  following_policy  = FOLLOW_POLICY(per_stream_policy[info->stream], info);

  assert(FSRRIP(following_policy) || FBRRIP(following_policy) || FSARP(following_policy));

  current_policy = CURRENT_POLICY(policy_data, global_data, following_policy, info, FALSE);
  
  switch (current_policy)
  {
    case cache_policy_srrip:
      cache_set_block_srrip(&(policy_data->srrip), way, tag, state, stream, info);
      break;

    case cache_policy_brrip:
      cache_set_block_brrip(&(policy_data->brrip), way, tag, state, stream, info);
      break;

    case cache_policy_sarp:
      block = &(SARP_DATA_BLOCKS(policy_data))[way];

      /* Check: tag matches with the block's tag. */
      assert(block->tag == tag);

      /* Check: block must be in valid state. It is not possible to set
       * state for an invalid block.*/
      assert(block->state != cache_block_invalid);

      if (state != cache_block_invalid)
      {
        /* Assign access stream */
        CACHE_UPDATE_BLOCK_STATE(block, tag, state);
        CACHE_UPDATE_BLOCK_STREAM(block, stream);
        return;
      }

      /* Invalidate block */
      CACHE_UPDATE_BLOCK_STATE(block, tag, cache_block_invalid);
      CACHE_UPDATE_BLOCK_STREAM(block, NN);
      block->epoch  = 0;

      /* Get old RRPV from the block */
      int old_rrpv = (((rrip_list *)(block->data))->rrpv);

      /* Remove block from valid list and insert into free list */
      CACHE_REMOVE_FROM_QUEUE(block, SARP_DATA_VALID_HEAD(policy_data)[old_rrpv],
          SARP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
      CACHE_APPEND_TO_SQUEUE(block, SARP_DATA_FREE_HEAD(policy_data), 
          SARP_DATA_FREE_TAIL(policy_data));
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      break;
  }
}

/* Get tag and state of a block. */
struct cache_block_t cache_get_block_sarp(sarp_data *policy_data, 
  sarp_gdata *global_data, int way, long long *tag_ptr, 
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
    PTR_ASSIGN(tag_ptr, (SARP_DATA_BLOCKS(policy_data)[way]).tag);
    PTR_ASSIGN(state_ptr, (SARP_DATA_BLOCKS(policy_data)[way]).state);
    PTR_ASSIGN(stream_ptr, (SARP_DATA_BLOCKS(policy_data)[way]).stream);
  }
  else
  {
    PTR_ASSIGN(tag_ptr, 0xdead);
    PTR_ASSIGN(state_ptr, cache_block_invalid);
    PTR_ASSIGN(stream_ptr, NN);
  }

  return (way != BYPASS_WAY) ? SARP_DATA_BLOCKS(policy_data)[way] : ret_block;
}

/* Set policy for a stream */
void set_per_stream_policy_sarp(sarp_data *policy_data, sarp_gdata *global_data, 
  ub4 core, ub1 stream, cache_policy_t policy)
{
  assert(core < SARP_GDATA_CORES(global_data));
  assert(stream > NN && stream <= TST);

#define SRRIP_POLICY(p) ((p) == cache_policy_srrip)
#define BRRIP_POLICY(p) ((p) == cache_policy_brrip)
#define DRRIP_POLICY(p) ((p) == cache_policy_drrip)
#define SARP_POLICY(p)  ((p) == cache_policy_sarp)

  assert(SRRIP_POLICY(policy) || BRRIP_POLICY(policy) || DRRIP_POLICY(policy) ||  SARP_POLICY(policy));
  
#undef SRRIP_POLICY
#undef BRRIP_POLICY
#undef DRRIP_POLICY
#undef SARP_POLICY

  /* Assign policy to stream */
  if (GPU_STREAM(stream))
  {
    assert(global_data->sarp_gpu_cores > 0 && core == 0);

    SARP_DATA_PSPOLICY(policy_data)[stream] = policy;
  }
  else
  {
    /* For CPU core id starts from 1 */
    assert(core > 0);

    SARP_DATA_PSPOLICY(policy_data)[stream] = policy;
  }
}

#define SAMPLER_INDX(i, s)    ((((i)->address >> (s)->log_grain_size)) & ((s)->sets - 1))
#define SAMPLER_TAG(i, s)     ((i)->address >> (s)->log_grain_size)
#define SAMPLER_OFFSET(i, s)  (((i)->address >> (s)->log_block_size) & ((s)->entry_size - 1))

/* UPdate */
void update_sampler_fill_perfctr(sampler_cache *sampler, ub4 index, ub4 way, memory_trace *info)
{
  ub1 offset;
  ub1 strm;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  strm = SARP_STREAM(sampler->blocks[index][way], offset, info);

  SMPLRPERF_FILL(&(sampler->perfctr), strm) += 1;

  /* Update epoch counter for stats */
  SMPLRPERF_FILL_EP(&(sampler->perfctr), strm) += 1;
}

/* If previous access was spill */
void update_sampler_spill_reuse_perfctr(sampler_cache *sampler, ub4 index, ub4 way, 
    sarp_data *policy_data, memory_trace *info)
{
  ub1 strm;
  ub1 offset;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  strm = SARP_OSTREAM(sampler->blocks[index][way], offset, info);

  /* Verify sampler data */
  assert(sampler->blocks[index][way].spill_or_fill[offset] == TRUE);
  assert(sampler->blocks[index][way].timestamp[offset] <= policy_data->miss_count);

#define REUSE_DISTANCE(s, i, w, o, p) ((p)->miss_count - (s)->blocks[i][w].timestamp[o])

  if (REUSE_DISTANCE(sampler, index, way, offset, policy_data) <= sampler->ways)
  {
    SMPLRPERF_SDLOW(&(sampler->perfctr), strm) += 1;
  }
  else
  {
    SMPLRPERF_SDHIGH(&(sampler->perfctr), strm) += 1;
  }
  
  /* Inrement spill reuse */
  SMPLRPERF_SREUSE(&(sampler->perfctr), strm) += 1;
  
  /* Update max reuse */
  if (SMPLRPERF_SREUSE(&(sampler->perfctr), strm) > SMPLRPERF_MREUSE(&(sampler->perfctr)))
  {
    SMPLRPERF_MREUSE(&(sampler->perfctr)) = SMPLRPERF_SREUSE(&(sampler->perfctr), strm);
  }

#undef REUSE_DISTANCE
}

void update_sampler_spill_perfctr(sampler_cache *sampler, ub4 index, ub4 way, 
    memory_trace *info)
{
  ub1 offset;
  ub1 strm;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  strm = SARP_STREAM(sampler->blocks[index][way], offset, info);

  SMPLRPERF_SPILL(&(sampler->perfctr), strm) += 1;
}

/* If previous access was spill */
void update_sampler_fill_reuse_perfctr(sampler_cache *sampler, ub4 index, ub4 way,
    sarp_data *policy_data, memory_trace *info)
{
  ub1 strm;
  ub1 offset;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  strm = SARP_OSTREAM(sampler->blocks[index][way], offset, info);

  /* Reset hit_count */
  assert(sampler->blocks[index][way].spill_or_fill[offset] == FALSE);
  assert(sampler->blocks[index][way].timestamp[offset] <= policy_data->miss_count);

#define REUSE_DISTANCE(s, i, w, o, p) ((p)->miss_count - (s)->blocks[i][w].timestamp[o])

  if (REUSE_DISTANCE(sampler, index, way, offset, policy_data) <= sampler->ways)
  {
    SMPLRPERF_FDLOW(&(sampler->perfctr), strm) += 1;
  }
  else
  {
    SMPLRPERF_FDHIGH(&(sampler->perfctr), strm) += 1;
  }
  
  /* Inrement spill reuse */
  SMPLRPERF_FREUSE(&(sampler->perfctr), strm) += 1;
  
  /* Update max reuse */
  if (SMPLRPERF_FREUSE(&(sampler->perfctr), strm) > SMPLRPERF_MREUSE(&(sampler->perfctr)))
  {
    SMPLRPERF_MREUSE(&(sampler->perfctr)) = SMPLRPERF_FREUSE(&(sampler->perfctr), strm);
  }

#undef REUSE_DISTANCE
}

/* Update fills for current reuse ep ch */
static void update_sampler_fill_per_reuse_epoch_perfctr(sampler_cache *sampler, 
    ub4 index, ub4 way, memory_trace *info)
{
  ub1 strm;
  ub1 epoch;
  ub1 offset;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  /* Get real stream */
  strm = SARP_STREAM(sampler->blocks[index][way], offset, info);

  epoch = sampler->blocks[index][way].hit_count[offset];

  /* increment fill count for the incoming stream */
  SMPLRPERF_FILL_RE(&(sampler->perfctr), strm, epoch) += 1;
}

/* Update fills reuse for current epoch */
static void update_sampler_fill_reuse_per_reuse_epoch_perfctr(sampler_cache *sampler, 
    ub4 index, ub4 way, sarp_data *policy_data, memory_trace *info)
{
  ub1 strm;
  ub1 epoch;
  ub1 offset;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  /* Get real stream */
  strm = SARP_OSTREAM(sampler->blocks[index][way], offset, info);

  epoch = sampler->blocks[index][way].hit_count[offset];
  
  /* Increment reuse distance */
#define REUSE_DISTANCE(s, i, w, o, p) ((p)->miss_count - (s)->blocks[i][w].timestamp[o])

  if (REUSE_DISTANCE(sampler, index, way, offset, policy_data) <= sampler->ways)
  {
    SMPLRPERF_FDLOW_RE(&(sampler->perfctr), strm, epoch) += 1;
  }
  else
  {
    SMPLRPERF_FDHIGH_RE(&(sampler->perfctr), strm, epoch) += 1;
  }
  
  /* Inrement fill reuse */
  SMPLRPERF_FREUSE_RE(&(sampler->perfctr), strm, epoch) += 1;
  
  /* Update max reuse */
  if (SMPLRPERF_FREUSE_RE(&(sampler->perfctr), strm, epoch) > SMPLRPERF_MREUSE(&(sampler->perfctr)))
  {
    SMPLRPERF_MREUSE(&(sampler->perfctr)) = SMPLRPERF_FREUSE_RE(&(sampler->perfctr), strm, epoch);
  }

#undef REUSE_DISTANCE
}

/* Update fills reuse */
static void update_sampler_change_fill_epoch_perfctr(sampler_cache *sampler, ub4 index, 
    ub4 way, memory_trace *info)
{
  ub4 offset;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  /* If reuse epoch is changed increment fills in new fill-epoch */
  if (sampler->blocks[index][way].hit_count[offset] < REPOCH_CNT) 
  {
    sampler->blocks[index][way].hit_count[offset] += 1;
    update_sampler_fill_per_reuse_epoch_perfctr(sampler, index, way, info);
  }
}

static void update_sampler_xstream_perfctr(sampler_cache *sampler, ub4 index, ub4 way,
    memory_trace *info)
{
  ub1 offset;

  offset  = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);
  
  if ((sampler->blocks)[index][way].stream[offset] == CS && info->stream == TS)
  {
    (sampler->blocks)[index][way].dynamic_color[offset] = TRUE;
  }

  if ((sampler->blocks)[index][way].stream[offset] == ZS && info->stream != ZS)
  {
    (sampler->blocks)[index][way].dynamic_depth[offset] = TRUE; 
  }

  if ((sampler->blocks)[index][way].stream[offset] == BS && info->stream != BS)
  {
    (sampler->blocks)[index][way].dynamic_blit[offset] = TRUE; 
  }

  if ((sampler->blocks)[index][way].stream[offset] == PS)
  {
    (sampler->blocks)[index][way].dynamic_proc[offset] = TRUE; 
  }
}

void sampler_cache_fill_block(sampler_cache *sampler, ub4 index, ub4 way, 
    sarp_data *policy_data, memory_trace *info)
{
  ub8 offset;
  ub8 page;

  /* Obtain sampler entry corresponding to address */
  page    = SAMPLER_TAG(info, sampler);
  offset  = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);
  
  /* Fill relevant entry in sampler entry */
  sampler->blocks[index][way].page                  = page;
  sampler->blocks[index][way].spill_or_fill[offset] = (info->spill ? TRUE : FALSE);
  sampler->blocks[index][way].stream[offset]        = info->stream;
  sampler->blocks[index][way].valid[offset]         = TRUE;
  sampler->blocks[index][way].hit_count[offset]     = 0;
  sampler->blocks[index][way].dynamic_color[offset] = FALSE;
  sampler->blocks[index][way].dynamic_depth[offset] = FALSE;
  sampler->blocks[index][way].dynamic_blit[offset]  = FALSE;
  sampler->blocks[index][way].dynamic_proc[offset]  = FALSE;
  sampler->blocks[index][way].timestamp[offset]     = policy_data->miss_count;

  /* Update fill */
  if (info->fill == TRUE)
  {
    update_sampler_fill_perfctr(sampler, index, way, info);
    update_sampler_fill_per_reuse_epoch_perfctr(sampler, index, way, info);
  }
  else
  {
    update_sampler_spill_perfctr(sampler, index, way, info);
  }
}

void sampler_cache_access_block(sampler_cache *sampler, ub4 index, ub4 way,
    sarp_data *policy_data, memory_trace *info)
{
  ub4 offset;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  if (info->spill == TRUE)
  {
    /* Update fill count */
    update_sampler_spill_perfctr(sampler, index, way, info);     
  }
  else
  {
    /* If previous access to sampler was a spill */
    if (sampler->blocks[index][way].spill_or_fill[offset] == TRUE)
    {
      /* Reset reuse-epoch */
      sampler->blocks[index][way].hit_count[offset] = 0;

      /* Increment spill reuse */
      update_sampler_spill_reuse_perfctr(sampler, index, way, policy_data, info);

      /* Update dynamic stream flag */
      update_sampler_xstream_perfctr(sampler, index, way, info);

      /* Increment fill reues */
      update_sampler_fill_per_reuse_epoch_perfctr(sampler, index, way, info);
    }
    else
    {
      /* Increment fill reuse */
      update_sampler_fill_reuse_perfctr(sampler, index, way, policy_data, info);

      /* Increment fill reuse per reues epoch */
      update_sampler_fill_reuse_per_reuse_epoch_perfctr(sampler, index, way, policy_data, info);
      
      /* Update fill reuse epoch, if epoch has changed, update fill count for
       * new epoch */
      update_sampler_change_fill_epoch_perfctr(sampler, index, way, info);
    }
    
    /* Update fill count */
    update_sampler_fill_perfctr(sampler, index, way, info);     
  }

  sampler->blocks[index][way].spill_or_fill[offset] = (info->spill ? TRUE : FALSE);
  sampler->blocks[index][way].stream[offset]        = info->stream;
  sampler->blocks[index][way].timestamp[offset]     = policy_data->miss_count;
}

void sampler_cache_lookup(sampler_cache *sampler, sarp_data *policy_data, 
    memory_trace *info)
{
  ub4 way;
  ub4 index;
  ub8 page;
  ub8 offset;
  ub1 strm;

  /* Obtain sampler index, tag, offset and cache index of the access */
  index       = SAMPLER_INDX(info, sampler);
  page        = SAMPLER_TAG(info, sampler);
  offset      = SAMPLER_OFFSET(info, sampler);
  strm        = NEW_STREAM(info);

  for (way = 0; way < sampler->ways; way++)
  {
    if (sampler->blocks[index][way].page == page)
    {
      break;
    }
  }
  
  /* Find an invalid entry in sampler cache */
  if (way == sampler->ways)
  { 
    /* Walk through entire sampler set */
    for (way = 0; way < sampler->ways; way++)
    {
      if (sampler->blocks[index][way].page == SARP_SAMPLER_INVALID_TAG)
        break;
    }
  }

  if (way == sampler->ways)
  {
    // No invalid entry
    //// Replace only if stream occupancy in the sampler is still below minimum occupancy threshold
    if (sampler->stream_occupancy[strm] < STREAM_OCC_THRESHOLD)
    {
      way = (ub4)(random() % sampler->ways);

      sampler->blocks[index][way].page = SARP_SAMPLER_INVALID_TAG;

      for (ub1 off = 0; off < sampler->entry_size; off++)
      {
        sampler->blocks[index][way].valid[off]          = 0;
        sampler->blocks[index][way].dynamic_color[off]  = 0;
        sampler->blocks[index][way].dynamic_depth[off]  = 0;
        sampler->blocks[index][way].dynamic_blit[off]   = 0;
        sampler->blocks[index][way].dynamic_proc[off]   = 0;
      }
    }
  }
  
  /* Sampler is updated on hit or on miss and an old entry is replaced */
  if (way < sampler->ways)
  {
    if (sampler->blocks[index][way].page == SARP_SAMPLER_INVALID_TAG ||
        sampler->blocks[index][way].valid[offset] == 0)
    {

      if (sampler->blocks[index][way].page == SARP_SAMPLER_INVALID_TAG)
      {
        sampler->stream_occupancy[info->stream] += 1;

        if (strm == OS)
        {
          sampler->stream_occupancy[strm] += 1;
        }
      }

      /* If sampler access was a miss and an entry has been replaced */
      sampler_cache_fill_block(sampler, index, way, policy_data, info);    
    }
    else
    {
      /* If sampler access was a hit */
      sampler_cache_access_block(sampler, index, way, policy_data, info);
    }
  }
}

#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
#undef BYTH_NUMR
#undef BYTH_DENM
#undef SMTH_NUMR
#undef SMTH_DENM
#undef SHTH_NUMR
#undef SHTH_DENM
#undef BYPASS_ACCESS_TH
#undef EPOCH_SIZE
#undef CPS
#undef CPS1
#undef CPS2
#undef CPS3
#undef CPS4
#undef CPU_STREAM
#undef SARP_SAMPLER_INVALID_TAG
#undef STREAM_OCC_THRESHOLD
#undef LOG_GRAIN_SIZE
#undef LOG_BLOCK_SIZE
#undef DYNC
#undef DYNZ
#undef DYNB
#undef DYNP
#undef DYNBP
#undef SARP_STREAM
#undef KNOWN_STREAM
#undef OTHER_STREAM
#undef NEW_STREAM
#undef CACHE_UPDATE_BLOCK_STREAM
#undef CACHE_SARP_INCREMENT_RRPV
#undef free_list_remove_block
#undef PSRRIP
#undef PBRRIP
#undef PSARP
#undef FSRRIP
#undef FBRRIP
#undef FSARP
#undef CSRRIPGSRRIP
#undef CSRRIPGBRRIP
#undef CBRRIPGSRRIP
#undef CBRRIPGBRRIP
#undef CSRRIP_SET
#undef CBRRIP_SET
#undef GSRRIP_SET
#undef GBRRIP_SET
#undef GSET
#undef CSET
#undef DPOLICY
#undef PCSRRIPGSRRIP
#undef PCSRRIPGBRRIP
#undef PCBRRIPGSRRIP
#undef PCBRRIPGBRRIP
#undef PCBRRIP
#undef PCSRRIP
#undef DPSARP
#undef SPSARP
#undef FANDH
#undef FANDM
#undef SPEEDUP
#undef CURRENT_POLICY
#undef FOLLOW_POLICY
#undef SAMPLER_INDX
#undef SAMPLER_TAG
#undef SAMPLER_OFFSET
