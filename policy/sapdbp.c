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

#include "cache.h"
#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "sapdbp.h"
#include "zlib.h"

#define MIN_EPOCH     (0)
#define MAX_EPOCH     (10)
#define EPOCH_COUNT   (MAX_EPOCH - MIN_EPOCH + 1)
#define INTERVAL_SIZE (1000)

#define PSEL_WIDTH              (10)
#define PSEL_MIN_VAL            (0x00)  
#define PSEL_MAX_VAL            (0x3ff)  
#define PSEL_MID_VAL            (512)

#define ECTR_WIDTH              (20)
#define ECTR_MIN_VAL            (0x00)  
#define ECTR_MAX_VAL            (0xfffff)  
#define ECTR_MID_VAL            (0x3ff)

#define SRRIP_SAMPLED_SET       (0)
#define BRRIP_SAMPLED_SET       (1)
#define DRRIP_SAMPLED_SET       (2)
#define PS_SAMPLED_SET          (3)
#define FOLLOWER_SET            (4)

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))

/*
 * Private Functions
 */
#define free_list_remove_block(set, blk)                                                \
do                                                                                      \
{                                                                                       \
        /* Check: free list must be non empty as it contains the current block. */      \
        assert((set)->free_head && (set)->free_tail);                                   \
                                                                                        \
        /* Check: current block must be in invalid state */                             \
        assert((blk)->state == cache_block_invalid);                                    \
                                                                                        \
        CACHE_REMOVE_FROM_SQUEUE(blk, set->free_head, set->free_tail);                  \
                                                                                        \
        (blk)->next = NULL;                                                             \
        (blk)->prev = NULL;                                                             \
                                                                                        \
        /* Reset block state */                                                         \
        (blk)->busy = 0;                                                                \
        (blk)->cached = 0;                                                              \
        (blk)->prefetch = 0;                                                            \
}while(0);

/* Returns currently followed policy */

#define PSRRIP                        (cache_policy_srrip)
#define PBRRIP                        (cache_policy_brrip)
#define PDRRIP                        (cache_policy_drrip)
#define PSAPDBP                       (cache_policy_sapdbp)
#define SRRIP_SET(p)                  ((p)->set_type == SRRIP_SAMPLED_SET)
#define BRRIP_SET(p)                  ((p)->set_type == BRRIP_SAMPLED_SET)
#define DRRIP_SET(p)                  ((p)->set_type == DRRIP_SAMPLED_SET)
#define PS_SET(p)                     ((p)->set_type == PS_SAMPLED_SET)
#define FLW_SET(p)                    ((p)->set_type == FOLLOWER_SET)
#define FSRRIP(f)                     (f == cache_policy_srrip)
#define FBRRIP(f)                     (f == cache_policy_brrip)
#define FDRRIP(f)                     (f == cache_policy_drrip)
#define FSAPDBP(f)                    (f == cache_policy_sapdbp)
#define THRASHER(g, s, n)             ((g)->thrasher_stream[s] == TRUE)
#define FDYN(f)                       (FDRRIP(f) || FSAPDBP(f))
#define SSRRIP(g)                     (SAT_CTR_VAL((g)->drrip_psel) < PSEL_MID_VAL)
#define BSPOLICY(f)                   (FSRRIP(f) || FBRRIP(f) || FDRRIP(f))
#define DFOLLOW(p, g, f)              ((FSRRIP(f) || (FDYN(f) && SSRRIP(g))) ? PSRRIP : PBRRIP)  
#define PSSRRIP(g, s)                 (SAT_CTR_VAL((g)->sapdbp_ssel[s]) < PSEL_MID_VAL)
#define PSFOLLOW(g, s)                (PSSRRIP(g, s) ? PSRRIP : PBRRIP)
#define SPEEDUP(n)                    (n == sapdbp_stream_x)
#define NSPEEDUP(g, s)                ((g)->speedup_stream == s)
#define PSS_POLICY(p, g, f, s, n)     (NSPEEDUP(g, s) ? PSFOLLOW(g, s) : DFOLLOW(p, g, f))
#define PS_POLICY(p, g, f, s, n)      (THRASHER(g, s, n) ? PBRRIP : PSS_POLICY(p, g, f, s, n))
#define FOLLOW_PS(g)                  (SAT_CTR_VAL((g)->sapdbp_psel) > PSEL_MID_VAL ? TRUE : FALSE)
#define FW_POLICY(p, g, f, s, n)      (FOLLOW_PS(g) ? PS_POLICY(p, g, f, s, n) : DFOLLOW(p, g, f))
#define NPOLICY(p, g, f, s, n)        (PS_SET(p) ? PS_POLICY(p, g, f, s, n) : FW_POLICY(p, g, f, s, n))
#define CURRENT_POLICY(p, g, f, s, n) (BSPOLICY(f)? DFOLLOW(p, g, f) : NPOLICY(p, g, f, s, n))
#define GET_CURRENT_POLICY(g, f)      ((FSRRIP(f) || (FDYN(f) && SSRRIP(g))) ? PSRRIP : PBRRIP)

static ub4 get_set_type_sapdbp(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x07;
  msb_bits = (indx >> 7) & 0x07;
  mid_bits = (indx >> 3) & 0x0f;

  if (lsb_bits == msb_bits && mid_bits == 0)
  {
    return SRRIP_SAMPLED_SET;
  }

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 0)
  {
    return BRRIP_SAMPLED_SET;
  }

#if 0
  if (lsb_bits == msb_bits && mid_bits == 1)
  {
    return PS_SAMPLED_SET;
  }
#endif

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 1)
  {
    return DRRIP_SAMPLED_SET;
  }

  return FOLLOWER_SET;
}
 
/* Returns SAPDBP stream corresponding to access stream based on stream 
 * classification algoritm. */
sapdbp_stream get_sapdbp_stream(memory_trace *info)
{
  return (sapdbp_stream)(info->sap_stream);
}

/* Public interface to initialize SAP statistics */
void cache_init_sapdbp_stats(sapdbp_stats *stats)
{
  if (!stats->sapdbp_stat_file)
  {
    /* Open statistics file */
    stats->sapdbp_stat_file = gzopen("PC-SAPDBP-stats.trace.csv.gz", "wb9");

    assert(stats->sapdbp_stat_file);
    
    for (ub1 i = NN; i < TST + 1; i++)
    {
      stats->sapdbp_ps_fill[i]  = 0;
      stats->sapdbp_ps_thr[i]   = 0;
      stats->sapdbp_ps_dep[i]   = 0;
      stats->speedup_count[i]   = 0;
      stats->thrasher_count[i]  = 0;
      stats->dead_blocks[i]     = 0;
    }

    stats->sapdbp_srrip_samples  = 0;
    stats->sapdbp_brrip_samples  = 0;
    stats->sapdbp_srrip_fill     = 0;
    stats->sapdbp_brrip_fill     = 0;
    stats->sapdbp_fill_2         = 0;
    stats->sapdbp_fill_3         = 0;
    stats->sapdbp_hdr_printed    = FALSE;
    stats->next_schedule         = 500 * 1024;
    stats->epoch_count           = 0;
  }
}

/* Public interface to finalize SAP statistic */
void cache_fini_sapdbp_stats(sapdbp_stats *stats)
{
  /* If stats has not been printed, assume it is an offline simulation,
   * and, dump stats now. */
  if (stats->sapdbp_hdr_printed == FALSE)
  {
    cache_dump_sapdbp_stats(stats, 0);
  }

  gzclose(stats->sapdbp_stat_file);
}

/* Public interface to dump periodic SAP statistics */
void cache_dump_sapdbp_stats(sapdbp_stats *stats, ub8 cycle)
{
  /* Print header if not already done */
  if (stats->sapdbp_hdr_printed == FALSE)
  {
    gzprintf(stats->sapdbp_stat_file,
        "CYCLE; SRRIPSAMPLE; BRRIPSAMPLE; SRRIPFILL; BRRIPFILL; FILL2; FILL3; "
        "CFILL; ZFILL; TFILL; BFILL; PFILL; "
        "CTHR; ZTHR; TTHR; BTHR; PTHR; CSPD; ZSPD; TSPD; BSPD; PSPD\n");

    stats->sapdbp_hdr_printed = TRUE;
  }
  
#define PER_SP(s, t) ((s)->epoch_count ? ((s)->speedup_count[(t)] * 100) / (s)->epoch_count : 0)
#define PER_TH(s, t) ((s)->epoch_count ? ((s)->thrasher_count[(t)] * 100) / (s)->epoch_count : 0)

  /* Dump current counter values */
  gzprintf(stats->sapdbp_stat_file,
      "%ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld; %ld\n", 
      cycle, stats->sapdbp_srrip_samples, 
      stats->sapdbp_brrip_samples, stats->sapdbp_srrip_fill, 
      stats->sapdbp_brrip_fill, stats->sapdbp_fill_2, stats->sapdbp_fill_3,
      stats->sapdbp_ps_fill[CS], stats->sapdbp_ps_fill[ZS], stats->sapdbp_ps_fill[TS],
      stats->sapdbp_ps_fill[BS], stats->sapdbp_ps_fill[PS],
      PER_TH(stats, CS), PER_TH(stats, ZS), PER_TH(stats, TS),
      PER_TH(stats, BS), PER_TH(stats, PS),
      PER_SP(stats, CS), PER_SP(stats, ZS), PER_SP(stats, TS),
      PER_SP(stats, BS), PER_SP(stats, PS));
  
#undef PER_SP
#undef PER_TH

  /* Reset all stat counters */
#if 0
  stats->sapdbp_srrip_fill = 0;
  stats->sapdbp_brrip_fill = 0;
  stats->sapdbp_fill_2     = 0;
  stats->sapdbp_fill_3     = 0;

  for (ub1 i = NN; i < TST + 1; i++)
  {
      stats->sapdbp_ps_fill[i]  = 0;
      stats->sapdbp_ps_thr[i]   = 0;
      stats->sapdbp_ps_dep[i]   = 0;
  }
#endif
}

/* Initialize SRRIP from SAPDBP policy data */
static void cache_init_srrip_from_sapdbp(ub8 set_indx, 
    struct cache_params *params, srrip_data *policy_data, 
    srrip_gdata *global_data, sapdbp_data *sapdbp_policy_data, 
    sapdbp_gdata *sapdbp_global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);
  assert(sapdbp_policy_data);
  assert(sapdbp_global_data);
  
  /* Initialize global data */
  if (set_indx == 0)
  {
    for (ub1 s = NN; s <= TST; s++)
    {
      for (ub1 i = 0; i < EPOCH_COUNT; i++)
      {
        /* Initialize epoch fill counter */
        global_data->epoch_fctr   = sapdbp_global_data->epoch_fctr;
        global_data->epoch_hctr   = sapdbp_global_data->epoch_hctr;
        global_data->epoch_valid  = sapdbp_global_data->epoch_valid;
      }
    }

    global_data->epoch_count  = EPOCH_COUNT;
    global_data->max_epoch    = MAX_EPOCH; 
  }

  /* Create per rrpv buckets */
  SRRIP_DATA_VALID_HEAD(policy_data) = SAPDBP_DATA_VALID_HEAD(sapdbp_policy_data);
  SRRIP_DATA_VALID_TAIL(policy_data) = SAPDBP_DATA_VALID_TAIL(sapdbp_policy_data);
  
  assert(SRRIP_DATA_VALID_HEAD(policy_data));
  assert(SRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIP_DATA_MAX_RRPV(policy_data)    = params->max_rrpv;
  SRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;
  
  if (sapdbp_policy_data->set_type == FOLLOWER_SET)
  {
    SRRIP_DATA_USE_EPOCH(policy_data) = TRUE;
  }
  else
  {
    SRRIP_DATA_USE_EPOCH(policy_data) = FALSE;
  }

  /* Create array of blocks */
  SRRIP_DATA_BLOCKS(policy_data) = SAPDBP_DATA_BLOCKS(sapdbp_policy_data);

  SRRIP_DATA_FREE_HLST(policy_data) = SAPDBP_DATA_FREE_HLST(sapdbp_policy_data);
  SRRIP_DATA_FREE_TLST(policy_data) = SAPDBP_DATA_FREE_TLST(sapdbp_policy_data);

  /* Set current and default fill policy to SRRIP */
  SRRIP_DATA_CFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CRPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DRPOLICY(policy_data) = cache_policy_srrip;

  assert(SRRIP_DATA_MAX_RRPV(policy_data) != 0);
}

/* Initialize BRRIP for SAPDBP policy data */
static void cache_init_brrip_from_sapdbp(ub8 set_indx, 
    struct cache_params *params, brrip_data *policy_data, 
    brrip_gdata *global_data, sapdbp_data *sapdbp_policy_data,
    sapdbp_gdata *sapdbp_global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);
  assert(sapdbp_policy_data);
  assert(sapdbp_global_data);

  /* Initialize global data */
  if (set_indx == 0)
  {
    for (ub1 s = NN; s <= TST; s++)
    {
      for (ub1 i = 0; i < EPOCH_COUNT; i++)
      {
        /* Initialize epoch fill counter */
        global_data->epoch_fctr   = sapdbp_global_data->epoch_fctr;
        global_data->epoch_hctr   = sapdbp_global_data->epoch_hctr;
        global_data->epoch_valid  = sapdbp_global_data->epoch_valid;
      }
    }

    global_data->epoch_count  = EPOCH_COUNT;
    global_data->max_epoch    = MAX_EPOCH; 
  }

  /* Create per rrpv buckets */
  BRRIP_DATA_VALID_HEAD(policy_data) = SAPDBP_DATA_VALID_HEAD(sapdbp_policy_data);
  BRRIP_DATA_VALID_TAIL(policy_data) = SAPDBP_DATA_VALID_TAIL(sapdbp_policy_data);
  
  assert(BRRIP_DATA_VALID_HEAD(policy_data));
  assert(BRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  BRRIP_DATA_MAX_RRPV(policy_data)    = params->max_rrpv;
  BRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  if (sapdbp_policy_data->set_type == FOLLOWER_SET)
  {
    BRRIP_DATA_USE_EPOCH(policy_data) = TRUE;
  }
  else
  {
    BRRIP_DATA_USE_EPOCH(policy_data) = FALSE;
  }

  /* Create array of blocks */
  BRRIP_DATA_BLOCKS(policy_data) = SAPDBP_DATA_BLOCKS(sapdbp_policy_data);

  BRRIP_DATA_FREE_HLST(policy_data) = SAPDBP_DATA_FREE_HLST(sapdbp_policy_data);
  BRRIP_DATA_FREE_TLST(policy_data) = SAPDBP_DATA_FREE_TLST(sapdbp_policy_data);

  /* Set current and default fill policy to SRRIP */
  BRRIP_DATA_CFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CRPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DRPOLICY(policy_data) = cache_policy_brrip;

  assert(BRRIP_DATA_MAX_RRPV(policy_data) != 0);
}

void cache_init_sapdbp(long long int set_indx, struct cache_params *params, 
    sapdbp_data *policy_data, sapdbp_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);

  if (set_indx == 0)
  {
    global_data->epoch_fctr = xcalloc(TST + 1, sizeof(sctr *));
    assert(global_data->epoch_fctr);

    global_data->epoch_hctr = xcalloc(TST + 1, sizeof(sctr *));
    assert(global_data->epoch_hctr);
    
    global_data->epoch_valid = xcalloc(TST + 1, sizeof(ub1));
    assert(global_data->epoch_valid);
    
    /* Allocate xstream eviction array */
    global_data->epoch_xevict = xcalloc(TST + 1, sizeof(sctr *));
    assert(global_data->epoch_xevict);

    /* Allocate xstream eviction array */
    global_data->epoch_thrasher = xcalloc(TST + 1, sizeof(ub1));
    assert(global_data->epoch_thrasher);

    /* Allocate xstream eviction array */
    global_data->thrasher_stream = xcalloc(TST + 1, sizeof(ub1));
    assert(global_data->thrasher_stream);

    for (ub1 s = 0; s <= TST; s++)
    {
      global_data->epoch_fctr[s] = xcalloc(EPOCH_COUNT, sizeof(sctr));
      assert(global_data->epoch_fctr[s]);

      global_data->epoch_hctr[s] = xcalloc(EPOCH_COUNT, sizeof(sctr));
      assert(global_data->epoch_hctr[s]);
      
      global_data->epoch_valid[s] = FALSE;

      for (ub1 i = 0; i < EPOCH_COUNT; i++)
      {
        /* Initialize epoch fill counter */
        SAT_CTR_INI(global_data->epoch_fctr[s][i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
        SAT_CTR_RST(global_data->epoch_fctr[s][i]);

        /* Initialize epoch eviction counter */
        SAT_CTR_INI(global_data->epoch_hctr[s][i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
        SAT_CTR_RST(global_data->epoch_hctr[s][i]);
      }

      global_data->epoch_xevict[s] = xcalloc((TST + 1), sizeof(sctr));
      assert(global_data->epoch_xevict[s]);

      for (ub1 i = 0; i < TST + 1; i++)
      {
        /* Initialize epoch fill counter */
        SAT_CTR_INI(global_data->epoch_xevict[s][i], ECTR_WIDTH, ECTR_MIN_VAL, ECTR_MAX_VAL);
        SAT_CTR_RST(global_data->epoch_xevict[s][i]);
      }

      global_data->epoch_thrasher[s]  = NN;
      global_data->bs_epoch           = params->bs_epoch;
    }

    global_data->access_interval = 0;

    /* Initialize policy selection counter */
    SAT_CTR_INI(global_data->drrip_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->drrip_psel, PSEL_MID_VAL);

    /* Initialize policy selection counter */
    SAT_CTR_INI(global_data->sapdbp_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->sapdbp_psel, PSEL_MID_VAL);

    for (ub4 i = 0; i <= TST; i++) 
    {

      SAT_CTR_INI(global_data->sapdbp_ssel[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_SET(global_data->sapdbp_ssel[i], PSEL_MID_VAL);

      SAT_CTR_INI(global_data->sapdbp_hint[i], ECTR_WIDTH, ECTR_MIN_VAL, ECTR_MAX_VAL);
      SAT_CTR_SET(global_data->sapdbp_hint[i], ECTR_MIN_VAL);
    }

    global_data->stats.sapdbp_srrip_samples = 0;
    global_data->stats.sapdbp_brrip_samples = 0;
    global_data->stats.sapdbp_srrip_fill    = 0;
    global_data->stats.sapdbp_brrip_fill    = 0;
    global_data->stats.sapdbp_fill_2        = 0;
    global_data->stats.sapdbp_fill_3        = 0;
    global_data->stats.sample_srrip_fill   = 0;
    global_data->stats.sample_brrip_fill   = 0;
    global_data->stats.sample_srrip_hit    = 0;
    global_data->stats.sample_brrip_hit    = 0;

#define BRRIP_CTR_WIDTH     (8)    
#define BRRIP_PSEL_MIN_VAL  (0)    
#define BRRIP_PSEL_MAX_VAL  (255)    

    /* Initialize BRRIP set wide data */
    SAT_CTR_INI(global_data->brrip.access_ctr, BRRIP_CTR_WIDTH, 
        BRRIP_PSEL_MIN_VAL, BRRIP_PSEL_MAX_VAL);
    global_data->brrip.threshold = params->threshold;

#undef BRRIP_CTR_WIDTH
#undef BRRIP_PSEL_MIN_VAL
#undef BRRIP_PSEL_MAX_VAL

    /* Samples to be used for corrective path (Two fixed samples are used for
     * policy and baseline) */
#define FXD_SAMPL    (2)
#define SDP_SAMPL(p) (p->sdp_gpu_cores + p->sdp_cpu_cores + FXD_SAMPL)

    global_data->sdp.sdp_samples    = SDP_SAMPL(params);
    global_data->sdp.sdp_gpu_cores  = params->sdp_gpu_cores;
    global_data->sdp.sdp_cpu_cores  = params->sdp_cpu_cores;
    global_data->sdp.sdp_cpu_tpset  = params->sdp_cpu_tpset;
    global_data->sdp.sdp_cpu_tqset  = params->sdp_cpu_tqset;

#undef FXD_SAMPL
#undef SDP_SAMPL

    /* Initialize SAP global data for SAP statistics for SAPDBP */
    global_data->sdp.sap.sap_streams      = params->sdp_streams;
    global_data->sdp.sap.sap_cpu_cores    = params->sdp_cpu_cores;
    global_data->sdp.sap.sap_gpu_cores    = params->sdp_gpu_cores;
    global_data->sdp.sap.sap_cpu_tpset    = params->sdp_cpu_tpset;
    global_data->sdp.sap.sap_cpu_tqset    = params->sdp_cpu_tqset;
    global_data->sdp.sap.activate_access  = params->sdp_tactivate * params->num_sets;

    /* Initialize SAPDBP stats */
    cache_init_sapdbp_stats(&(global_data->stats));

    /* Initialize SAP statistics for SAPDBP */
    cache_init_sap_stats(&(global_data->sdp.sap.stats), "PC-SAPDBP-SAP-stats.trace.csv.gz");
  }

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  SAPDBP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SAPDBP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC

  assert(SAPDBP_DATA_VALID_HEAD(policy_data));
  assert(SAPDBP_DATA_VALID_TAIL(policy_data));

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SAPDBP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    SAPDBP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    SAPDBP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  SAPDBP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc((size), sizeof(list_head_t)))

  SAPDBP_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(SAPDBP_DATA_FREE_HLST(policy_data));

  SAPDBP_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(SAPDBP_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  SAPDBP_DATA_FREE_HEAD(policy_data) = &(SAPDBP_DATA_BLOCKS(policy_data)[0]);
  SAPDBP_DATA_FREE_TAIL(policy_data) = &(SAPDBP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SAPDBP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SAPDBP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SAPDBP_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SAPDBP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SAPDBP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SAPDBP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV
  ub4 set_type;

  set_type = get_set_type_sapdbp(set_indx);

  /* Initialize SRRIP and BRRIP per policy data using SAPDBP data */
  cache_init_srrip_from_sapdbp(set_indx, params, &(policy_data->srrip), 
      &(global_data->srrip), policy_data, global_data);
  cache_init_brrip_from_sapdbp(set_indx, params, &(policy_data->brrip), 
      &(global_data->brrip), policy_data, global_data);

  switch (set_type)
  {
    case SRRIP_SAMPLED_SET:
      /* Set policy followed by sample */
      policy_data->following = cache_policy_srrip;

      /* Increment SRRIP sample count */
      global_data->stats.sapdbp_srrip_samples += 1;

      break;

    case BRRIP_SAMPLED_SET:
      /* Set policy followed by sample */
      policy_data->following = cache_policy_brrip;

      /* Increment BRRIP sample count */
      global_data->stats.sapdbp_brrip_samples += 1;

      break;

    case DRRIP_SAMPLED_SET:
      policy_data->following = cache_policy_drrip;
      break;

    case FOLLOWER_SET:
#if 0
    case PS_SAMPLED_SET:
#endif
      policy_data->following = cache_policy_sapdbp;
      break;

    default:
      panic("Unknown set type %s %d\n", __FUNCTION__, __LINE__);
  }

  policy_data->set_type = set_type;
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_sapdbp(unsigned int set_indx, sapdbp_data *policy_data, 
  sapdbp_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Free all data blocks */
  free(SRRIP_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (SRRIP_DATA_VALID_HEAD(policy_data))
  {
    free(SRRIP_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SRRIP_DATA_VALID_TAIL(policy_data))
  {
    free(SRRIP_DATA_VALID_TAIL(policy_data));
  }
  
  /* Free SAP statistics for SAPDBP */
  if (set_indx == 0)
  {
    cache_fini_sapdbp_stats(&(global_data->stats));
    cache_fini_sap_stats(&(global_data->sdp.sap.stats));
  }
}

static void cache_update_interval_end(sapdbp_gdata *global_data)
{
#if 0
  cache_dump_sapdbp_stats(&(global_data->stats), 0);
#endif

  /* Reset old thrasher flag for all streams */
  for (ub1 i = 0; i < TST + 1; i++)
  {
    global_data->thrasher_stream[i]  = FALSE;
  }

  /* Reset old epoch counters and obtain new thrasher streams */
  for (ub1 i = 0; i < TST + 1; i++)
  {
    ub1 thrasher;
    ub4 val;

    printf("CEVCT%d:%5d ZEVCT%d:%5d TEVCT%d:%5d BEVCT%d:%5d PEVCT%d:%5d\n", 
        i, SAT_CTR_VAL(global_data->epoch_xevict[i][CS]), 
        i, SAT_CTR_VAL(global_data->epoch_xevict[i][ZS]),
        i, SAT_CTR_VAL(global_data->epoch_xevict[i][TS]), 
        i, SAT_CTR_VAL(global_data->epoch_xevict[i][BS]),
        i, SAT_CTR_VAL(global_data->epoch_xevict[i][PS]));


    thrasher = NN;
    val      = 0;

    /* Obtain thrasher stream and half the counter */
    for (ub1 j = 0; j < TST + 1; j++)
    {
      if (SAT_CTR_VAL(global_data->epoch_xevict[i][j]) > val)
      {
        val = SAT_CTR_VAL(global_data->epoch_xevict[i][j]);
        thrasher = j;
      }

#if 0
      SAT_CTR_HLF(global_data->epoch_xevict[i][j]);
#endif
      SAT_CTR_RST(global_data->epoch_xevict[i][j]);
    }

    /* Update new thrasher stream */
    global_data->epoch_thrasher[i]          = thrasher;
    global_data->thrasher_stream[thrasher]  = TRUE;
  }

  printf("CTHR:%5d ZTHR:%5d TTHR:%5d BTHR:%5d PTHR:%5d\n", 
      global_data->epoch_thrasher[CS], 
      global_data->epoch_thrasher[ZS], 
      global_data->epoch_thrasher[TS], 
      global_data->epoch_thrasher[BS], 
      global_data->epoch_thrasher[PS]); 

#define MAX_RANK (3)

#define RERANK(r)                           \
  do                                        \
  {                                         \
    for (sb1 j = MAX_RANK - 2; j >= 0; j--) \
    {                                       \
      (r)[j + 1] = (r)[j];                  \
    }                                       \
  }while(0)

  /* Obtain stream to be spedup */
  ub4 val;
  ub1 new_stream;
  ub1 old_stream;
  ub1 stream_rank[MAX_RANK];
  ub1 i;

  val         = 0;
  new_stream  = NN;
  old_stream  = global_data->speedup_stream;

  for (i = 0; i < MAX_RANK; i++)
  {
    stream_rank[i] = NN;
  }

  /* Select stream to be spedup */
  for (i = 0; i < TST + 1; i++)
  {
    if (SAT_CTR_VAL(global_data->sapdbp_hint[i]) > val)
    {
      val = SAT_CTR_VAL(global_data->sapdbp_hint[i]);
      new_stream = i;
      RERANK(stream_rank);
      stream_rank[0] = i;
    }
  }

  for (i = 0; i < MAX_RANK; i++)
  {
    if (stream_rank[i] == old_stream)
    {
      break;
    }
  }

  /* If stream is not in MAX_RANK, assign new stream to be speadup */
  if (i == MAX_RANK)
  {
    global_data->speedup_stream = new_stream;
  }


  if (global_data->speedup_stream != NN)
  {
    global_data->stats.epoch_count += 1;
    global_data->stats.speedup_count[global_data->speedup_stream] += 1;
    global_data->stats.thrasher_count[global_data->epoch_thrasher[global_data->speedup_stream]] += 1;
  }

  /* Reset hit counter array */
  for (i = 0; i < TST + 1; i++)
  {
    SAT_CTR_RST(global_data->sapdbp_hint[i]);

    /* Reinitialize epoch valid list */
    global_data->epoch_valid[i] = FALSE;
  }

#define SSTRM(g)  ((g)->speedup_stream)
#define ETHR(g)   ((g)->epoch_thrasher[SSTRM(g)])

  /* Set thrashing stream corresponding to selected stream valid */
  assert(ETHR(global_data) <= TST);

  if (SSTRM(global_data) != NN)
  {
    global_data->epoch_valid[ETHR(global_data)] = TRUE;   
  }

  printf("SPDSTRM:%5d THRSTRM:%5d\n", global_data->speedup_stream, ETHR(global_data));

  /* Reset epoch fill and hit counters  */
  for (i = 0; i < EPOCH_COUNT; i++)
  {
    printf("EPOCHF%d:%5d EPOCHH%d:%5d\n", 
        i, SAT_CTR_VAL(global_data->epoch_fctr[ETHR(global_data)][i]), 
        i, SAT_CTR_VAL(global_data->epoch_hctr[ETHR(global_data)][i]));

    for (ub1 j = 0; j < TST + 1; j++)
    {
      SAT_CTR_HLF(global_data->epoch_fctr[j][i]);
      SAT_CTR_HLF(global_data->epoch_hctr[j][i]);
    }
  }

  global_data->access_interval = 0;

  /* Dump thrasher and speedup stats */

  if (global_data->stats.epoch_count)
  {
#define PER_SP(g, s) (((g)->stats.speedup_count[(s)] * 100) / (g)->stats.epoch_count)
#define PER_TH(g, s) (((g)->stats.thrasher_count[(s)] * 100) / (g)->stats.epoch_count)

    printf("SPC; SPZ; SPT; SPB; SPP\n");
    printf("%ld; %ld; %ld; %ld; %ld\n", 
        PER_SP(global_data, CS), PER_SP(global_data, ZS), PER_SP(global_data, TS),
        PER_SP(global_data, BS), PER_SP(global_data, PS)); 

    printf("THC; THZ; THT; THB; THP\n");
    printf("%ld; %ld; %ld; %ld; %ld\n", 
        PER_TH(global_data, CS),PER_TH(global_data, ZS), PER_TH(global_data, TS),
        PER_TH(global_data, BS), PER_TH(global_data, PS)); 
  }

#undef ETHR
#undef SSTRM
#undef MAX_RANK
#undef RERANK
#undef PER_SP
#undef PER_TH
}

static void cache_update_hint_count(sapdbp_gdata *global_data, memory_trace *info)
{
  assert(info->stream <= TST);
  
  if (SPEEDUP(get_sapdbp_stream(info)))
  {
    SAT_CTR_INC(global_data->sapdbp_hint[info->stream]);
  }
}

struct cache_block_t* cache_find_block_sapdbp(sapdbp_data *policy_data, 
    sapdbp_gdata *global_data, memory_trace *info, long long tag)
{
  int    max_rrpv;
  struct cache_block_t *head;
  struct cache_block_t *node;

  max_rrpv  = policy_data->srrip.max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SAPDBP_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

end:
  
  if (policy_data->set_type == FOLLOWER_SET)
  {
    /* Update hint counter array for all accesses */
    cache_update_hint_count(global_data, info);
  }

  if (policy_data->set_type == DRRIP_SAMPLED_SET)
  {
    if (++(global_data->access_interval) >= INTERVAL_SIZE)
    {
      cache_update_interval_end(global_data);
    }
  }

  return node;
}

static void cache_update_hit_epoch_count(sapdbp_gdata *global_data, 
    struct cache_block_t *block)
{
  ub1 epoch;

  assert(block->stream <= TST);

  if (block->epoch > MAX_EPOCH)
  {
    epoch = MAX_EPOCH;
  }
  else
  {
    epoch = block->epoch;
  }

  SAT_CTR_INC(global_data->epoch_hctr[block->stream][epoch]);
}

static void cache_update_fill_epoch_count(sapdbp_gdata *global_data, 
    struct cache_block_t *block)
{
  ub1 epoch;

  assert(block->stream <= TST);

  if (block->epoch > MAX_EPOCH)
  {
    epoch = MAX_EPOCH;
  }
  else
  {
    epoch = block->epoch;
  }

  SAT_CTR_INC(global_data->epoch_fctr[block->stream][epoch]);
}

static void cache_update_xstream_evct_count(sapdbp_gdata *global_data, 
    struct cache_block_t *block, memory_trace *info)
{
  assert(block->stream <= TST);
  assert(info->stream <= TST);

  SAT_CTR_INC(global_data->epoch_xevict[block->stream][info->stream]);
}

static void cache_update_fill_stats(sapdbp_gdata *global_data, 
    memory_trace *info)
{
  global_data->stats.sapdbp_ps_fill[info->stream] += 1;
}

static void cache_update_thr_stats(sapdbp_gdata *global_data, 
    memory_trace *info)
{
  if (THRASHER(global_data, info->stream, get_sapdbp_stream(info)))
  {
    global_data->stats.sapdbp_ps_thr[info->stream] += 1;
  }
}

static void cache_update_dep_stats(sapdbp_data *policy_data, 
    sapdbp_gdata *global_data, memory_trace *info)
{
  if (THRASHER(global_data, info->stream, get_sapdbp_stream(info)) &&
      DFOLLOW(policy_data, global_data, policy_data->following) == cache_policy_srrip)
  {
    global_data->stats.sapdbp_ps_dep[info->stream] += 1;
  }
}

void cache_fill_block_sapdbp(sapdbp_data *policy_data, 
    sapdbp_gdata *global_data, int way, long long tag, 
    enum cache_block_state_t state, int strm, memory_trace  *info)
{
  enum cache_policy_t   current_policy;
  struct cache_block_t *block;
  sapdbp_stream      sstream;


  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);

  assert(FSRRIP(policy_data->following) || FBRRIP(policy_data->following) || 
      FDRRIP(policy_data->following) || FSAPDBP(policy_data->following));

  switch (policy_data->following)
  {
    case cache_policy_srrip:
      /* Increment fill stats for SRRIP */
      global_data->stats.sample_srrip_fill += 1;
      SAT_CTR_INC(global_data->drrip_psel);
      SAT_CTR_INC(global_data->sapdbp_ssel[info->stream]);
      break;

    case cache_policy_brrip:
      global_data->stats.sample_brrip_fill += 1;
      SAT_CTR_DEC(global_data->drrip_psel);
      SAT_CTR_DEC(global_data->sapdbp_ssel[info->stream]);
      break;

    case cache_policy_drrip:
      assert (policy_data->set_type == DRRIP_SAMPLED_SET);
      SAT_CTR_INC(global_data->sapdbp_psel);
      break;

    case cache_policy_sapdbp:
      /* Nothing to do */
      if (policy_data->set_type == PS_SAMPLED_SET)
      {
        SAT_CTR_DEC(global_data->sapdbp_psel);
      }
      break;

    default:
      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }
  

#define CTR_VAL(d)    (SAT_CTR_VAL((d)->brrip.access_ctr))
#define THRESHOLD(d)  ((d)->brrip.threshold)
  
  sstream = get_sapdbp_stream(info);

  current_policy = GET_CURRENT_POLICY(global_data, policy_data->following);
#if 0
  current_policy = CURRENT_POLICY(policy_data, global_data, 
      policy_data->following, info->stream, sstream);
#endif

  /* Fill block in all component policies */
  switch (current_policy)
  {
    case cache_policy_srrip:
      cache_fill_block_srrip(&(policy_data->srrip), &(global_data->srrip), way, 
          tag, state, strm, info);

      /* For SAPDBP, BRRIP access counter is incremented on access to all sets
       * irrespective of the followed policy. So, if followed policy is SRRIP
       * we increment counter here. */
      if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
      {
        SAT_CTR_INC(global_data->brrip.access_ctr);
      }
      else
      {
        SAT_CTR_RST(global_data->brrip.access_ctr);
      }

      /* Increment fill stats for SRRIP */
      global_data->stats.sapdbp_srrip_fill += 1;
      global_data->stats.sapdbp_fill_2     += 1; 
      break;

    case cache_policy_brrip:
      /* Find fill RRPV to update stats */
      if (CTR_VAL(global_data) == 0) 
      {
        global_data->stats.sapdbp_fill_2 += 1; 
      }
      else
      {
        global_data->stats.sapdbp_fill_3 += 1; 
      }

      cache_fill_block_brrip(&(policy_data->brrip), &(global_data->brrip), way,
          tag, state, strm, info);

      /* Increment fill stats for SRRIP */
      global_data->stats.sapdbp_brrip_fill += 1;
      break;

    default:
      panic("Invalid follower function %s line %d\n", __FUNCTION__, __LINE__);
  }

  block = &(policy_data->blocks[way]);
  assert(block);
  
  /* Update epoch counters */
  if (policy_data->set_type == BRRIP_SAMPLED_SET)
  {
    cache_update_fill_epoch_count(global_data, block);
#if 0
    if (++(global_data->access_interval) >= INTERVAL_SIZE)
    {
      cache_update_interval_end(global_data);
    }
#endif
  }
  
  if (policy_data->set_type == FOLLOWER_SET)
  {
    cache_update_fill_stats(global_data, info);
    cache_update_thr_stats(global_data, info);
    cache_update_dep_stats(policy_data, global_data, info);
#if 0
    /* Update hint counter array for all accesses */
    cache_update_hint_count(global_data, info);
#endif
  }

#undef CTR_VAL
#undef THRESHOLD
}

int cache_replace_block_sapdbp(sapdbp_data *policy_data, 
    sapdbp_gdata *global_data, memory_trace *info)
{
  int ret_way;

  /* Ensure vaid arguments */
  assert(policy_data);
  assert(global_data);
  
  /* According to the policy choose a replacement candidate */
  switch (GET_CURRENT_POLICY(global_data, policy_data->following))
  {
    case cache_policy_srrip:
      ret_way = cache_replace_block_srrip(&(policy_data->srrip), 
          &(global_data->srrip), info);
      break; 

    case cache_policy_brrip:
      ret_way = cache_replace_block_brrip(&(policy_data->brrip));
      break;

    default:
      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }
  
  /* Update xstream evict counters */
  if (ret_way != -1)  
  {
    if (policy_data->set_type == DRRIP_SAMPLED_SET)
    {
      cache_update_xstream_evct_count(global_data, &(policy_data->blocks[ret_way]), 
          info);
    }
  }

  return ret_way;
}

void cache_access_block_sapdbp(sapdbp_data *policy_data, 
  sapdbp_gdata *global_data, int way, int strm, memory_trace *info)
{
  struct cache_block_t *block;

  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);

  switch (policy_data->following)
  {
    case cache_policy_srrip:
      /* Increment fill stats for SRRIP */
      global_data->stats.sample_srrip_hit += 1;
      break;

    case cache_policy_brrip:
      global_data->stats.sample_brrip_hit += 1;
      break;

    case cache_policy_sapdbp:
    case cache_policy_drrip:
      /* Nothing to do */
      break;

    default:
      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }

  block = &(policy_data->blocks[way]);
  assert(block);
  
  if (policy_data->set_type == DRRIP_SAMPLED_SET)
  {
    cache_update_hit_epoch_count(global_data, block);
  }

  switch (GET_CURRENT_POLICY(global_data, policy_data->following)) 
  {
    case cache_policy_srrip:
      cache_access_block_srrip(&(policy_data->srrip), &(global_data->srrip), way, strm, info);
      break;

    case cache_policy_brrip:
      cache_access_block_brrip(&(policy_data->brrip), &(global_data->brrip), way, strm, info);
      break;

    default:
      panic("Invalid follower function %s line %d\n", __FUNCTION__, __LINE__);
  }


  block = &(policy_data->blocks[way]);
  assert(block);

  if (policy_data->set_type == DRRIP_SAMPLED_SET)
  {
    cache_update_fill_epoch_count(global_data, block);
  }
}

/* Update state of block. */
void cache_set_block_sapdbp(sapdbp_data *policy_data, sapdbp_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  
  /* Call component policies */
  switch (GET_CURRENT_POLICY(global_data, policy_data->following))
  {
    case cache_policy_srrip:
      cache_set_block_srrip(&(policy_data->srrip), way, tag, state, stream, info);
      break;

    case cache_policy_brrip:
      cache_set_block_brrip(&(policy_data->brrip), way, tag, state, stream, info);
      break;

    default:
      panic("Invalid folower function %s line %d\n", __FUNCTION__, __LINE__);
  }
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_sapdbp(sapdbp_data *policy_data, 
    sapdbp_gdata *global_data, int way, long long *tag_ptr, 
    enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  struct cache_block_t ret_block;    

  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  assert(tag_ptr);
  assert(state_ptr);
  
  switch (GET_CURRENT_POLICY(global_data, policy_data->following))
  {
    case cache_policy_srrip:
      return cache_get_block_srrip(&(policy_data->srrip), way, tag_ptr, 
        state_ptr, stream_ptr);
      break;

    case cache_policy_brrip:
      return cache_get_block_brrip(&(policy_data->brrip), way, tag_ptr,
        state_ptr, stream_ptr);
      break;

    default:
      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
      return ret_block;
  }
}

#undef MIN_EPOCH
#undef MAX_EPOCH
#undef EPOCH_COUNT
#undef INTERVAL_SIZE
#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
