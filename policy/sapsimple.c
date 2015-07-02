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
#include "sapsimple.h"
#include "zlib.h"

#define MIN_EPOCH     (0)
#define MAX_EPOCH     (10)
#define EPOCH_COUNT   (MAX_EPOCH - MIN_EPOCH + 1)
#define INTERVAL_SIZE (1000)

#define PSEL_WIDTH              (10)
#define PSEL_MIN_VAL            (0x00)  
#define PSEL_MAX_VAL            (0x3ff)  
#define PSEL_MID_VAL            (512)

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
#define GET_CURRENT_POLICY(g, f)   ((FSRRIP(f) || ((FDRRIP(f) || FSAPSIMPLE(f)) &&      \
                                    SAT_CTR_VAL((g)->drrip_psel) < PSEL_MID_VAL)) ?     \
                                    PSRRIP : PBRRIP)

#define PSRRIP                        (cache_policy_srrip)
#define PBRRIP                        (cache_policy_brrip)
#define PDRRIP                        (cache_policy_drrip)
#define PSAPSIMPLE                    (cache_policy_sapsimple)
#define SRRIP_SET(p)                  ((p)->set_type == SRRIP_SAMPLED_SET)
#define BRRIP_SET(p)                  ((p)->set_type == BRRIP_SAMPLED_SET)
#define DRRIP_SET(p)                  ((p)->set_type == DRRIP_SAMPLED_SET)
#define PS_SET(p)                     ((p)->set_type == PS_SAMPLED_SET)
#define FLW_SET(p)                    ((p)->set_type == FOLLOWER_SET)
#define FSRRIP(f)                     (f == cache_policy_srrip)
#define FBRRIP(f)                     (f == cache_policy_brrip)
#define FDRRIP(f)                     (f == cache_policy_drrip)
#define FSAPSIMPLE(f)                 (f == cache_policy_sapsimple)
#define FDYN(f)                       (FDRRIP(f) || FSAPSIMPLE(f))
#define SSRRIP(g)                     (SAT_CTR_VAL((g)->drrip_psel) < PSEL_MID_VAL)
#define BSPOLICY(f)                   (FSRRIP(f) || FBRRIP(f) || FDRRIP(f))
#if 0
#define DFOLLOW(p, g, f)              ((FSRRIP(f) || (FDRRIP(f) && SSRRIP(g))) ? PSRRIP : PBRRIP)  
#endif
#define DFOLLOW(p, g, f)              ((FSRRIP(f) || (FDYN(f) && SSRRIP(g))) ? PSRRIP : PBRRIP)  
#define PSSRRIP(g, s)                 (SAT_CTR_VAL((g)->sapsimple_ssel[s]) < PSEL_MID_VAL)
#define PSFOLLOW(g, s)                (PSSRRIP(g, s) ? PSRRIP : PBRRIP)
#define SPEEDUP(n)                    (n == sapsimple_stream_x)
#define PS_POLICY(p, g, f, s, n)      (SPEEDUP(n) ? PSFOLLOW(g, s) : DFOLLOW(p, g, f))
#define FOLLOW_PS(g)                  (SAT_CTR_VAL((g)->sapsimple_psel) > PSEL_MID_VAL ? TRUE : FALSE)
#define FW_POLICY(p, g, f, s, n)      (FOLLOW_PS(g) ? PS_POLICY(p, g, f, s, n) : DFOLLOW(p, g, f))
#define NPOLICY(p, g, f, s, n)        (PS_SET(p) ? PS_POLICY(p, g, f, s, n) : FW_POLICY(p, g, f, s, n))
#if 0
#define NPOLICY(p, g, f, s, n)        (PS_SET(p) ? PS_POLICY(p, g, f, s, n) : DFOLLOW(p, g, f))
#endif
#define CURRENT_POLICY(p, g, f, s, n) (BSPOLICY(f)? DFOLLOW(p, g, f) : NPOLICY(p, g, f, s, n))

static ub4 get_set_type_sapsimple(long long int indx)
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
  else
  {
    if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 0)
    {
      return BRRIP_SAMPLED_SET;
    }
    else
    {
      if (lsb_bits == msb_bits && mid_bits == 1)
      {
        return PS_SAMPLED_SET;
      }
      else
      {
        if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 1)
        {
          return DRRIP_SAMPLED_SET;
        }
      }
    }
  }

  return FOLLOWER_SET;
}
 
/* Returns SAPSIMPLE stream corresponding to access stream based on stream 
 * classification algoritm. */
sapsimple_stream get_sapsimple_stream(memory_trace *info)
{
  return (sapsimple_stream)(info->sap_stream);
}

/* Public interface to initialize SAP statistics */
void cache_init_sapsimple_stats(sapsimple_stats *stats)
{
  if (!stats->sapsimple_stat_file)
  {
    /* Open statistics file */
    stats->sapsimple_stat_file = gzopen("PC-SAPSIMPLE-stats.trace.csv.gz", "wb9");

    assert(stats->sapsimple_stat_file);

    stats->sapsimple_srrip_samples  = 0;
    stats->sapsimple_brrip_samples  = 0;
    stats->sapsimple_srrip_fill     = 0;
    stats->sapsimple_brrip_fill     = 0;
    stats->sapsimple_fill_2         = 0;
    stats->sapsimple_fill_3         = 0;
    stats->sapsimple_hdr_printed    = FALSE;
    stats->next_schedule        = 500 * 1024;
  }
}

/* Public interface to finalize SAP statistic */
void cache_fini_sapsimple_stats(sapsimple_stats *stats)
{
  /* If stats has not been printed, assume it is an offline simulation,
   * and, dump stats now. */
  if (stats->sapsimple_hdr_printed == FALSE)
  {
    cache_dump_sapsimple_stats(stats, 0);
  }

  gzclose(stats->sapsimple_stat_file);
}

/* Public interface to dump periodic SAP statistics */
void cache_dump_sapsimple_stats(sapsimple_stats *stats, ub8 cycle)
{
  /* Print header if not already done */
  if (stats->sapsimple_hdr_printed == FALSE)
  {
    gzprintf(stats->sapsimple_stat_file,
        "CYCLE; SRRIPSAMPLE; BRRIPSAMPLE; SRRIPFILL; BRRIPFILL; FILL2; FILL3\n");

    stats->sapsimple_hdr_printed = TRUE;
  }
  
  /* Dump current counter values */
  gzprintf(stats->sapsimple_stat_file,
      "%ld; %ld; %ld; %ld; %ld %ld %ld\n", cycle, stats->sapsimple_srrip_samples, 
      stats->sapsimple_brrip_samples, stats->sapsimple_srrip_fill, 
      stats->sapsimple_brrip_fill, stats->sapsimple_fill_2, stats->sapsimple_fill_3);
  
  /* Reset all stat counters */
  stats->sapsimple_srrip_fill = 0;
  stats->sapsimple_brrip_fill = 0;
  stats->sapsimple_fill_2     = 0;
  stats->sapsimple_fill_3     = 0;
}

/* Initialize SRRIP from SAPSIMPLE policy data */
static void cache_init_srrip_from_sapsimple(ub8 set_indx, 
    struct cache_params *params, srrip_data *policy_data, 
    srrip_gdata *global_data, sapsimple_data *sapsimple_policy_data, 
    sapsimple_gdata *sapsimple_global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);
  assert(sapsimple_policy_data);
  assert(sapsimple_global_data);
  
  /* Initialize global data */
  if (set_indx == 0)
  {
    for (ub1 s = NN; s <= TST; s++)
    {
      for (ub1 i = 0; i < EPOCH_COUNT; i++)
      {
        /* Initialize epoch fill counter */
        global_data->epoch_fctr = NULL;
        global_data->epoch_hctr = NULL;
      }
    }
  }

  /* Create per rrpv buckets */
  SRRIP_DATA_VALID_HEAD(policy_data) = SAPSIMPLE_DATA_VALID_HEAD(sapsimple_policy_data);
  SRRIP_DATA_VALID_TAIL(policy_data) = SAPSIMPLE_DATA_VALID_TAIL(sapsimple_policy_data);
  
  assert(SRRIP_DATA_VALID_HEAD(policy_data));
  assert(SRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIP_DATA_MAX_RRPV(policy_data)    = params->max_rrpv;
  SRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  /* Create array of blocks */
  SRRIP_DATA_BLOCKS(policy_data) = SAPSIMPLE_DATA_BLOCKS(sapsimple_policy_data);

  SRRIP_DATA_FREE_HLST(policy_data) = SAPSIMPLE_DATA_FREE_HLST(sapsimple_policy_data);
  SRRIP_DATA_FREE_TLST(policy_data) = SAPSIMPLE_DATA_FREE_TLST(sapsimple_policy_data);

  /* Set current and default fill policy to SRRIP */
  SRRIP_DATA_CFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CRPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DRPOLICY(policy_data) = cache_policy_srrip;

  assert(SRRIP_DATA_MAX_RRPV(policy_data) != 0);
}

/* Initialize BRRIP for SAPSIMPLE policy data */
static void cache_init_brrip_from_sapsimple(ub8 set_indx, 
    struct cache_params *params, brrip_data *policy_data, 
    brrip_gdata *global_data, sapsimple_data *sapsimple_policy_data,
    sapsimple_gdata *sapsimple_global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);
  assert(sapsimple_policy_data);
  assert(sapsimple_global_data);

  /* Create per rrpv buckets */
  BRRIP_DATA_VALID_HEAD(policy_data) = SAPSIMPLE_DATA_VALID_HEAD(sapsimple_policy_data);
  BRRIP_DATA_VALID_TAIL(policy_data) = SAPSIMPLE_DATA_VALID_TAIL(sapsimple_policy_data);
  
  assert(BRRIP_DATA_VALID_HEAD(policy_data));
  assert(BRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  BRRIP_DATA_MAX_RRPV(policy_data)    = params->max_rrpv;
  BRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  /* Create array of blocks */
  BRRIP_DATA_BLOCKS(policy_data) = SAPSIMPLE_DATA_BLOCKS(sapsimple_policy_data);

  BRRIP_DATA_FREE_HLST(policy_data) = SAPSIMPLE_DATA_FREE_HLST(sapsimple_policy_data);
  BRRIP_DATA_FREE_TLST(policy_data) = SAPSIMPLE_DATA_FREE_TLST(sapsimple_policy_data);

  /* Set current and default fill policy to SRRIP */
  BRRIP_DATA_CFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CRPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DRPOLICY(policy_data) = cache_policy_brrip;

  assert(BRRIP_DATA_MAX_RRPV(policy_data) != 0);
}

void cache_init_sapsimple(long long int set_indx, struct cache_params *params, 
    sapsimple_data *policy_data, sapsimple_gdata *global_data)
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

    for (ub1 s = 0; s <= TST; s++)
    {
      global_data->epoch_fctr[s] = xcalloc(EPOCH_COUNT, sizeof(sctr));
      assert(global_data->epoch_fctr[s]);

      global_data->epoch_hctr[s] = xcalloc(EPOCH_COUNT, sizeof(sctr));
      assert(global_data->epoch_hctr[s]);

      for (ub1 i = 0; i < EPOCH_COUNT; i++)
      {
        /* Initialize epoch fill counter */
        SAT_CTR_INI(global_data->epoch_fctr[s][i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
        SAT_CTR_RST(global_data->epoch_fctr[s][i]);

        /* Initialize epoch eviction counter */
        SAT_CTR_INI(global_data->epoch_hctr[s][i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
        SAT_CTR_RST(global_data->epoch_hctr[s][i]);
      }
    }

    global_data->access_interval = 0;

    /* Initialize policy selection counter */
    SAT_CTR_INI(global_data->drrip_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->drrip_psel, PSEL_MID_VAL);

    /* Initialize policy selection counter */
    SAT_CTR_INI(global_data->sapsimple_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->sapsimple_psel, PSEL_MID_VAL);

    for (ub4 i = 0; i <= TST; i++) 
    {

      SAT_CTR_INI(global_data->sapsimple_ssel[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_SET(global_data->sapsimple_ssel[i], PSEL_MID_VAL);
    }

    global_data->stats.sapsimple_srrip_samples = 0;
    global_data->stats.sapsimple_brrip_samples = 0;
    global_data->stats.sapsimple_srrip_fill    = 0;
    global_data->stats.sapsimple_brrip_fill    = 0;
    global_data->stats.sapsimple_fill_2        = 0;
    global_data->stats.sapsimple_fill_3        = 0;
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

    /* Initialize SAP global data for SAP statistics for SAPSIMPLE */
    global_data->sdp.sap.sap_streams      = params->sdp_streams;
    global_data->sdp.sap.sap_cpu_cores    = params->sdp_cpu_cores;
    global_data->sdp.sap.sap_gpu_cores    = params->sdp_gpu_cores;
    global_data->sdp.sap.sap_cpu_tpset    = params->sdp_cpu_tpset;
    global_data->sdp.sap.sap_cpu_tqset    = params->sdp_cpu_tqset;
    global_data->sdp.sap.activate_access  = params->sdp_tactivate * params->num_sets;

    /* Initialize SAPSIMPLE stats */
    cache_init_sapsimple_stats(&(global_data->stats));

    /* Initialize SAP statistics for SAPSIMPLE */
    cache_init_sap_stats(&(global_data->sdp.sap.stats), "PC-SAPSIMPLE-SAP-stats.trace.csv.gz");
  }

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  SAPSIMPLE_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SAPSIMPLE_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC

  assert(SAPSIMPLE_DATA_VALID_HEAD(policy_data));
  assert(SAPSIMPLE_DATA_VALID_TAIL(policy_data));

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SAPSIMPLE_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    SAPSIMPLE_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    SAPSIMPLE_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  SAPSIMPLE_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc((size), sizeof(list_head_t)))

  SAPSIMPLE_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(SAPSIMPLE_DATA_FREE_HLST(policy_data));

  SAPSIMPLE_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(SAPSIMPLE_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  SAPSIMPLE_DATA_FREE_HEAD(policy_data) = &(SAPSIMPLE_DATA_BLOCKS(policy_data)[0]);
  SAPSIMPLE_DATA_FREE_TAIL(policy_data) = &(SAPSIMPLE_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SAPSIMPLE_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SAPSIMPLE_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SAPSIMPLE_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SAPSIMPLE_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SAPSIMPLE_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SAPSIMPLE_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV
  ub4 set_type;

  set_type = get_set_type_sapsimple(set_indx);

  /* Initialize SRRIP and BRRIP per policy data using SAPSIMPLE data */
  cache_init_srrip_from_sapsimple(set_indx, params, &(policy_data->srrip), 
      &(global_data->srrip), policy_data, global_data);
  cache_init_brrip_from_sapsimple(set_indx, params, &(policy_data->brrip), 
      &(global_data->brrip), policy_data, global_data);

  switch (set_type)
  {
    case SRRIP_SAMPLED_SET:
      /* Set policy followed by sample */
      policy_data->following = cache_policy_srrip;

      /* Increment SRRIP sample count */
      global_data->stats.sapsimple_srrip_samples += 1;

      break;

    case BRRIP_SAMPLED_SET:
      /* Set policy followed by sample */
      policy_data->following = cache_policy_brrip;

      /* Increment BRRIP sample count */
      global_data->stats.sapsimple_brrip_samples += 1;

      break;

    case DRRIP_SAMPLED_SET:
      policy_data->following = cache_policy_drrip;
      break;

    case FOLLOWER_SET:
    case PS_SAMPLED_SET:
      policy_data->following = cache_policy_sapsimple;
      break;

    default:
      panic("Unknown set type %s %d\n", __FUNCTION__, __LINE__);
  }

  policy_data->set_type = set_type;
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_sapsimple(unsigned int set_indx, sapsimple_data *policy_data, 
  sapsimple_gdata *global_data)
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
  
  /* Free SAP statistics for SAPSIMPLE */
  if (set_indx == 0)
  {
    cache_fini_sapsimple_stats(&(global_data->stats));
    cache_fini_sap_stats(&(global_data->sdp.sap.stats));
  }
}

struct cache_block_t* cache_find_block_sapsimple(sapsimple_data *policy_data, long long tag)
{
  int    max_rrpv;
  struct cache_block_t *head;
  struct cache_block_t *node;

  max_rrpv  = policy_data->srrip.max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SAPSIMPLE_DATA_VALID_HEAD(policy_data)[rrpv].head;

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

static void cache_update_hit_epoch_count(sapsimple_gdata *global_data, 
    struct cache_block_t *block)
{
  assert(block->stream <= TST);
  assert(block->epoch <= MAX_EPOCH);

  SAT_CTR_INC(global_data->epoch_hctr[block->stream][block->epoch]);
}

static void cache_update_fill_epoch_count(sapsimple_gdata *global_data, 
    struct cache_block_t *block)
{
  assert(block->stream <= TST);
  assert(block->epoch <= MAX_EPOCH);

  SAT_CTR_INC(global_data->epoch_fctr[block->stream][block->epoch]);
}

static void cache_update_interval_end(sapsimple_gdata *global_data)
{
  /* Half epoch counters  */
  for (ub1 i = 0; i < EPOCH_COUNT; i++)
  {
    printf("TSEF%d:%5d TSEE%d:%5d CSEF%d:%5d CSEE%d:%5d ZSEF%d:%5d ZSEE%d:%5d "
        "BTEF%d:%5d BTEE%d:%5d PSEF%d:%5d PSEE%d:%5d\n", 
        i, SAT_CTR_VAL(global_data->epoch_fctr[TS][i]), 
        i, SAT_CTR_VAL(global_data->epoch_hctr[TS][i]),
        i, SAT_CTR_VAL(global_data->epoch_fctr[CS][i]), 
        i, SAT_CTR_VAL(global_data->epoch_hctr[CS][i]),
        i, SAT_CTR_VAL(global_data->epoch_fctr[ZS][i]), 
        i, SAT_CTR_VAL(global_data->epoch_hctr[ZS][i]),
        i, SAT_CTR_VAL(global_data->epoch_fctr[BS][i]), 
        i, SAT_CTR_VAL(global_data->epoch_hctr[BS][i]),
        i, SAT_CTR_VAL(global_data->epoch_fctr[PS][i]), 
        i, SAT_CTR_VAL(global_data->epoch_hctr[PS][i]));

    SAT_CTR_HLF(global_data->epoch_fctr[TS][i]);
    SAT_CTR_HLF(global_data->epoch_hctr[TS][i]);
    SAT_CTR_HLF(global_data->epoch_fctr[CS][i]);
    SAT_CTR_HLF(global_data->epoch_hctr[CS][i]);
    SAT_CTR_HLF(global_data->epoch_fctr[ZS][i]);
    SAT_CTR_HLF(global_data->epoch_hctr[ZS][i]);
    SAT_CTR_HLF(global_data->epoch_fctr[BS][i]);
    SAT_CTR_HLF(global_data->epoch_hctr[BS][i]);
    SAT_CTR_HLF(global_data->epoch_fctr[PS][i]);
    SAT_CTR_HLF(global_data->epoch_hctr[PS][i]);
  }

  global_data->access_interval = 0;
}

void cache_fill_block_sapsimple(sapsimple_data *policy_data, 
    sapsimple_gdata *global_data, int way, long long tag, 
    enum cache_block_state_t state, int strm, memory_trace  *info)
{
  enum cache_policy_t   current_policy;
  struct cache_block_t *block;
  sapsimple_stream      sstream;


  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);

  assert(FSRRIP(policy_data->following) || FBRRIP(policy_data->following) || 
      FDRRIP(policy_data->following) || FSAPSIMPLE(policy_data->following));

  switch (policy_data->following)
  {
    case cache_policy_srrip:
      /* Increment fill stats for SRRIP */
      global_data->stats.sample_srrip_fill += 1;
      SAT_CTR_INC(global_data->drrip_psel);
      SAT_CTR_INC(global_data->sapsimple_ssel[info->stream]);
      break;

    case cache_policy_brrip:
      global_data->stats.sample_brrip_fill += 1;
      SAT_CTR_DEC(global_data->drrip_psel);
      SAT_CTR_DEC(global_data->sapsimple_ssel[info->stream]);
      break;

    case cache_policy_drrip:
      assert (policy_data->set_type == DRRIP_SAMPLED_SET);
      SAT_CTR_INC(global_data->sapsimple_psel);
      break;

    case cache_policy_sapsimple:
      /* Nothing to do */
      if (policy_data->set_type == PS_SAMPLED_SET)
      {
        SAT_CTR_DEC(global_data->sapsimple_psel);
      }
      break;

    default:
      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }
  

#define CTR_VAL(d)    (SAT_CTR_VAL((d)->brrip.access_ctr))
#define THRESHOLD(d)  ((d)->brrip.threshold)
  
  sstream = get_sapsimple_stream(info);

#if 0
  current_policy = GET_CURRENT_POLICY(policy_data, global_data);
#endif
  current_policy = CURRENT_POLICY(policy_data, global_data, 
      policy_data->following, info->stream, sstream);

  /* Fill block in all component policies */
  switch (current_policy)
  {
    case cache_policy_srrip:
      cache_fill_block_srrip(&(policy_data->srrip), &(global_data->srrip), way, 
          tag, state, strm, info);

      /* For SAPSIMPLE, BRRIP access counter is incremented on access to all sets
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
      global_data->stats.sapsimple_srrip_fill += 1;
      global_data->stats.sapsimple_fill_2     += 1; 
      break;

    case cache_policy_brrip:
      /* Find fill RRPV to update stats */
      if (CTR_VAL(global_data) == 0) 
      {
        global_data->stats.sapsimple_fill_2 += 1; 
      }
      else
      {
        global_data->stats.sapsimple_fill_3 += 1; 
      }

      cache_fill_block_brrip(&(policy_data->brrip), &(global_data->brrip), way,
          tag, state, strm, info);

      /* Increment fill stats for SRRIP */
      global_data->stats.sapsimple_brrip_fill += 1;
      break;

    default:
      panic("Invalid follower function %s line %d\n", __FUNCTION__, __LINE__);
  }

  block = &(policy_data->blocks[way]);
  assert(block);
  
  /* Update epoch counters */
  if (policy_data->set_type == SRRIP_SAMPLED_SET)
  {
    cache_update_fill_epoch_count(global_data, block);

    if (++(global_data->access_interval) >= INTERVAL_SIZE)
    {
      cache_update_interval_end(global_data);
    }
  }

#undef CTR_VAL
#undef THRESHOLD
}

int cache_replace_block_sapsimple(sapsimple_data *policy_data, sapsimple_gdata *global_data)
{
  int ret_way;

  /* Ensure vaid arguments */
  assert(policy_data);
  assert(global_data);
  
  /* According to the policy choose a replacement candidate */
  switch (GET_CURRENT_POLICY(global_data, policy_data->following))
  {
    case cache_policy_srrip:
      ret_way = cache_replace_block_srrip(&(policy_data->srrip), &(global_data->srrip));
      break; 

    case cache_policy_brrip:
      ret_way = cache_replace_block_brrip(&(policy_data->brrip));
      break;

    default:
      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }

  return ret_way;
}

void cache_access_block_sapsimple(sapsimple_data *policy_data, 
  sapsimple_gdata *global_data, int way, int strm, memory_trace *info)
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

    case cache_policy_sapsimple:
    case cache_policy_drrip:
      /* Nothing to do */
      break;

    default:
      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }

  block = &(policy_data->blocks[way]);
  assert(block);
  
  if (policy_data->set_type == SRRIP_SAMPLED_SET)
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

  if (policy_data->set_type == SRRIP_SAMPLED_SET)
  {
    cache_update_fill_epoch_count(global_data, block);
  }
}

/* Update state of block. */
void cache_set_block_sapsimple(sapsimple_data *policy_data, sapsimple_gdata *global_data, 
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
struct cache_block_t cache_get_block_sapsimple(sapsimple_data *policy_data, 
    sapsimple_gdata *global_data, int way, long long *tag_ptr, 
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
