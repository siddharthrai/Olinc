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
#include "drrip.h"
#include "zlib.h"

#define PSEL_WIDTH    (30)
#define PSEL_MIN_VAL  (0x00)  
#define PSEL_MAX_VAL  (1 << 30)  
#define PSEL_MID_VAL  (PSEL_MAX_VAL / 2)

#define SRRIP_SAMPLED_SET       (0)
#define BRRIP_SAMPLED_SET       (1)
#define FOLLOWER_SET            (2)
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
#define FSRRIP(f)                     (f == cache_policy_srrip)
#define FBRRIP(f)                     (f == cache_policy_brrip)
#define FDRRIP(f)                     (f == cache_policy_drrip)
#define FDYN(f)                       (FDRRIP(f))
#define SSRRIP(g)                     (SAT_CTR_VAL((g)->drrip_psel) < PSEL_MID_VAL)
#define DFOLLOW(p, g, f)              ((FSRRIP(f) || (FDYN(f) && SSRRIP(g))) ? PSRRIP : PBRRIP)  
#define CURRENT_POLICY(p, g, f, s, n) (DFOLLOW(p, g, f))
#define NEW_POLICY(i)                 (((i)->stream != TS) ? PBRRIP : PSRRIP)
#define DSRRIP(p, i)                  (((i) && (i)->spill) || FSRRIP((p)->following))
#define GET_CURRENT_POLICY(d, gd, i)  ((DSRRIP(d, i) || (FDRRIP((d)->following) &&  \
                                       SAT_CTR_VAL((gd)->psel) <= PSEL_MID_VAL)) ?\
                                       cache_policy_srrip : cache_policy_brrip)

static int get_set_type_drrip(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x1f;
  msb_bits = (indx >> 5) & 0x1f;

  if (lsb_bits == msb_bits)
  {
    return SRRIP_SAMPLED_SET;
  }
  else
  {
    if (lsb_bits == (~msb_bits & 0x1f))
    {
      return BRRIP_SAMPLED_SET;
    }
  }

  return FOLLOWER_SET;
}
 
/* Public interface to initialize SAP statistics */
void cache_init_drrip_stats(drrip_stats *stats)
{
  if (!stats->drrip_stat_file)
  {
    /* Open statistics file */
    stats->drrip_stat_file = gzopen("PC-DRRIP-stats.trace.csv.gz", "wb9");

    assert(stats->drrip_stat_file);

    stats->drrip_srrip_samples  = 0;
    stats->drrip_brrip_samples  = 0;
    stats->drrip_srrip_fill     = 0;
    stats->drrip_brrip_fill     = 0;
    stats->drrip_fill_2         = 0;
    stats->drrip_fill_3         = 0;
    stats->drrip_hdr_printed    = FALSE;
    stats->next_schedule        = 500 * 1024;
  }
}

/* Public interface to finalize SAP statistic */
void cache_fini_drrip_stats(drrip_stats *stats)
{
  /* If stats has not been printed, assume it is an offline simulation,
   * and, dump stats now. */
  if (stats->drrip_hdr_printed == FALSE)
  {
    cache_dump_drrip_stats(stats, 0);
  }

  gzclose(stats->drrip_stat_file);
}

/* Public interface to dump periodic SAP statistics */
void cache_dump_drrip_stats(drrip_stats *stats, ub8 cycle)
{
  /* Print header if not already done */
  if (stats->drrip_hdr_printed == FALSE)
  {
    gzprintf(stats->drrip_stat_file,
        "CYCLE; SRRIPSAMPLE; BRRIPSAMPLE; SRRIPFILL; BRRIPFILL; FILL2; FILL3\n");

    stats->drrip_hdr_printed = TRUE;
  }
  
  /* Dump current counter values */
  gzprintf(stats->drrip_stat_file,
      "%ld; %ld; %ld; %ld; %ld %ld %ld\n", cycle, stats->drrip_srrip_samples, 
      stats->drrip_brrip_samples, stats->drrip_srrip_fill, 
      stats->drrip_brrip_fill, stats->drrip_fill_2, stats->drrip_fill_3);
  
  /* Reset all stat counters */
  stats->drrip_srrip_fill = 0;
  stats->drrip_brrip_fill = 0;
  stats->drrip_fill_2     = 0;
  stats->drrip_fill_3     = 0;
}

/* Initialize SRRIP from DRRIP policy data */
static void cache_init_srrip_from_drrip(ub4 set_indx, 
    struct cache_params *params, srrip_data *policy_data, 
    srrip_gdata *global_data, drrip_data *drrip_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(drrip_policy_data);

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
  SRRIP_DATA_VALID_HEAD(policy_data) = DRRIP_DATA_VALID_HEAD(drrip_policy_data);
  SRRIP_DATA_VALID_TAIL(policy_data) = DRRIP_DATA_VALID_TAIL(drrip_policy_data);
  
  assert(SRRIP_DATA_VALID_HEAD(policy_data));
  assert(SRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIP_DATA_MAX_RRPV(policy_data)    = params->max_rrpv;
  SRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  /* Create array of blocks */
  SRRIP_DATA_BLOCKS(policy_data) = DRRIP_DATA_BLOCKS(drrip_policy_data);

  SRRIP_DATA_FREE_HLST(policy_data) = DRRIP_DATA_FREE_HLST(drrip_policy_data);
  SRRIP_DATA_FREE_TLST(policy_data) = DRRIP_DATA_FREE_TLST(drrip_policy_data);

  /* Set current and default fill policy to SRRIP */
  SRRIP_DATA_CFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CRPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DRPOLICY(policy_data) = cache_policy_srrip;

  assert(SRRIP_DATA_MAX_RRPV(policy_data) != 0);
}

/* Initialize BRRIP for DRRIP policy data */
static void cache_init_brrip_from_drrip(struct cache_params *params, brrip_data *policy_data,
  drrip_data *drrip_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(drrip_policy_data);

  /* Create per rrpv buckets */
  BRRIP_DATA_VALID_HEAD(policy_data) = DRRIP_DATA_VALID_HEAD(drrip_policy_data);
  BRRIP_DATA_VALID_TAIL(policy_data) = DRRIP_DATA_VALID_TAIL(drrip_policy_data);
  
  assert(BRRIP_DATA_VALID_HEAD(policy_data));
  assert(BRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  BRRIP_DATA_MAX_RRPV(policy_data)    = params->max_rrpv;
  BRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  /* Create array of blocks */
  BRRIP_DATA_BLOCKS(policy_data) = DRRIP_DATA_BLOCKS(drrip_policy_data);

  BRRIP_DATA_FREE_HLST(policy_data) = DRRIP_DATA_FREE_HLST(drrip_policy_data);
  BRRIP_DATA_FREE_TLST(policy_data) = DRRIP_DATA_FREE_TLST(drrip_policy_data);

  /* Set current and default fill policy to SRRIP */
  BRRIP_DATA_CFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CRPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DRPOLICY(policy_data) = cache_policy_brrip;

  assert(BRRIP_DATA_MAX_RRPV(policy_data) != 0);
}

void cache_init_drrip(long long int set_indx, struct cache_params *params, drrip_data *policy_data,
  drrip_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);

  if (set_indx == 0)
  {
    /* Initialize policy selection counter */
    SAT_CTR_INI(global_data->psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->psel, PSEL_MID_VAL);
    
    for (ub4 i = 0; i <= TST; i++) 
    {
      /* Initialize policy selection counter */
      SAT_CTR_INI(global_data->drrip_psel[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_SET(global_data->drrip_psel[i], PSEL_MID_VAL);
    }

    global_data->stats.drrip_srrip_samples = 0;
    global_data->stats.drrip_brrip_samples = 0;
    global_data->stats.drrip_srrip_fill    = 0;
    global_data->stats.drrip_brrip_fill    = 0;
    global_data->stats.drrip_fill_2        = 0;
    global_data->stats.drrip_fill_3        = 0;
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

    SAT_CTR_INI(global_data->bsample, BRRIP_CTR_WIDTH, BRRIP_PSEL_MIN_VAL, 
        BRRIP_PSEL_MAX_VAL);

    SAT_CTR_INI(global_data->bfollower, BRRIP_CTR_WIDTH, BRRIP_PSEL_MIN_VAL, 
        BRRIP_PSEL_MAX_VAL);

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

    /* Initialize SAP global data for SAP statistics for DRRIP */
    global_data->sdp.sap.sap_streams      = params->sdp_streams;
    global_data->sdp.sap.sap_cpu_cores    = params->sdp_cpu_cores;
    global_data->sdp.sap.sap_gpu_cores    = params->sdp_gpu_cores;
    global_data->sdp.sap.sap_cpu_tpset    = params->sdp_cpu_tpset;
    global_data->sdp.sap.sap_cpu_tqset    = params->sdp_cpu_tqset;
    global_data->sdp.sap.activate_access  = params->sdp_tactivate * params->num_sets;
    
    /* Initialize DRRIP stats */
    cache_init_drrip_stats(&(global_data->stats));

    /* Initialize SAP statistics for DRRIP */
    cache_init_sap_stats(&(global_data->sdp.sap.stats), "PC-DRRIP-SAP-stats.trace.csv.gz");
  }

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  DRRIP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  DRRIP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC

  assert(DRRIP_DATA_VALID_HEAD(policy_data));
  assert(DRRIP_DATA_VALID_TAIL(policy_data));

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    DRRIP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    DRRIP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    DRRIP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  DRRIP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc((size), sizeof(list_head_t)))

  DRRIP_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(DRRIP_DATA_FREE_HLST(policy_data));

  DRRIP_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(DRRIP_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  DRRIP_DATA_FREE_HEAD(policy_data) = &(DRRIP_DATA_BLOCKS(policy_data)[0]);
  DRRIP_DATA_FREE_TAIL(policy_data) = &(DRRIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&DRRIP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&DRRIP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&DRRIP_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&DRRIP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&DRRIP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&DRRIP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV

  switch (get_set_type_drrip(set_indx))
  {
    case SRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using DRRIP data */
      cache_init_srrip_from_drrip(set_indx, params, &(policy_data->srrip), 
          &(global_data->srrip), policy_data);
      cache_init_brrip_from_drrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      policy_data->following = cache_policy_srrip;

      /* Increment SRRIP sample count */
      global_data->stats.drrip_srrip_samples += 1;

      break;

    case BRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using DRRIP data */
      cache_init_srrip_from_drrip(set_indx, params, &(policy_data->srrip), 
          &(global_data->srrip), policy_data);
      cache_init_brrip_from_drrip(params, &(policy_data->brrip), policy_data);
      
      /* Set policy followed by sample */
      policy_data->following = cache_policy_brrip;

      /* Increment BRRIP sample count */
      global_data->stats.drrip_brrip_samples += 1;

      break;

    case FOLLOWER_SET:
      cache_init_srrip_from_drrip(set_indx, params, &(policy_data->srrip),
          &(global_data->srrip), policy_data);
      cache_init_brrip_from_drrip(params, &(policy_data->brrip), policy_data);
      policy_data->following = cache_policy_drrip;
      break;

    default:
      panic("Unknown set type %s %d\n", __FUNCTION__, __LINE__);
  }
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_drrip(unsigned int set_indx, drrip_data *policy_data, 
  drrip_gdata *global_data)
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
  
  /* Free SAP statistics for DRRIP */
  if (set_indx == 0)
  {
    cache_fini_drrip_stats(&(global_data->stats));
    cache_fini_sap_stats(&(global_data->sdp.sap.stats));
  }
}

struct cache_block_t* cache_find_block_drrip(drrip_data *policy_data, 
    drrip_gdata *global_data, long long tag, memory_trace *info)
{
  int    max_rrpv;
  struct cache_block_t *head;
  struct cache_block_t *node;

  max_rrpv  = policy_data->srrip.max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = DRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

end:
  if (!node)
  {
    if (info->fill)
    {
      switch (policy_data->following)
      {
        case cache_policy_srrip:
          /* Increment fill stats for SRRIP */
          global_data->stats.sample_srrip_fill += 1;
          SAT_CTR_INC(global_data->psel);
          SAT_CTR_INC(global_data->drrip_psel[info->stream]);
          break;

        case cache_policy_brrip:
          global_data->stats.sample_brrip_fill += 1;
          SAT_CTR_DEC(global_data->psel);
          SAT_CTR_DEC(global_data->drrip_psel[info->stream]);

#define CTR_VAL(d)    (SAT_CTR_VAL((d)->bsample))
#define THRESHOLD(d)  ((d)->brrip.threshold)

          if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
          {
            SAT_CTR_INC(global_data->bsample);
          }
          else
          {
            SAT_CTR_RST(global_data->bsample);
          }

          SAT_CTR_VAL(global_data->brrip.access_ctr) = CTR_VAL(global_data);

#undef CTR_VAL
#undef THRESHOLD
          break;

        case cache_policy_drrip:

#define CTR_VAL(d)    (SAT_CTR_VAL((d)->bfollower))
#define THRESHOLD(d)  ((d)->brrip.threshold)

          /* Nothing to do */
          if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
          {
            SAT_CTR_INC(global_data->bfollower);
          }
          else
          {
            SAT_CTR_RST(global_data->bfollower);
          }

          SAT_CTR_VAL(global_data->brrip.access_ctr) = CTR_VAL(global_data);

#undef CTR_VAL
#undef THRESHOLD
          break;

        default:
          panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
      }
    }
  }

  return node;
}

void cache_fill_block_drrip(drrip_data *policy_data, drrip_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int strm,
  memory_trace  *info)
{
  enum cache_policy_t current_policy;

  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);

  assert(policy_data->following == cache_policy_srrip ||
      policy_data->following == cache_policy_brrip || 
      policy_data->following == cache_policy_drrip);
  

#define CTR_VAL(d)    (SAT_CTR_VAL((d)->brrip.access_ctr))
#define THRESHOLD(d)  ((d)->brrip.threshold)
  
  if (info->spill == TRUE)
  {
    current_policy = cache_policy_srrip;
  }
  else
  {
    current_policy = GET_CURRENT_POLICY(policy_data, global_data, info);
  }
  
#if 0
  current_policy = NEW_POLICY(info);
#endif

  /* Fill block in all component policies */
  switch (current_policy)
  {
    case cache_policy_srrip:
      cache_fill_block_srrip(&(policy_data->srrip), &(global_data->srrip), way, tag, state, strm, 
          info);

      /* For DRRIP, BRRIP access counter is incremented on access to all sets
       * irrespective of the followed policy. So, if followed policy is SRRIP
       * we increment counter here. */
#if 0
      if (CTR_VAL(global_data) < THRESHOLD(global_data) - 1)
      {
        SAT_CTR_INC(global_data->brrip.access_ctr);
      }
      else
      {
        SAT_CTR_RST(global_data->brrip.access_ctr);
      }
#endif
      /* Increment fill stats for SRRIP */
      global_data->stats.drrip_srrip_fill += 1;
      global_data->stats.drrip_fill_2     += 1; 
      break;

    case cache_policy_brrip:
      /* Find fill RRPV to update stats */
      if (CTR_VAL(global_data) == 0) 
      {
        global_data->stats.drrip_fill_2 += 1; 
      }
      else
      {
        global_data->stats.drrip_fill_3 += 1; 
      }

      cache_fill_block_brrip(&(policy_data->brrip), &(global_data->brrip), way,
          tag, state, strm, info);

      /* Increment fill stats for SRRIP */
      global_data->stats.drrip_brrip_fill += 1;
      break;

    default:
      panic("Invalid follower function %s line %d\n", __FUNCTION__, __LINE__);
  }

#undef CTR_VAL
#undef THRESHOLD
}

int cache_replace_block_drrip(drrip_data *policy_data, drrip_gdata *global_data, 
    memory_trace *info)
{
  int ret_way;

  /* Ensure vaid arguments */
  assert(policy_data);
  assert(global_data);
  
  /* According to the policy choose a replacement candidate */
  switch (GET_CURRENT_POLICY(policy_data, global_data, (memory_trace *)NULL))
  {
    case cache_policy_srrip:
      ret_way = cache_replace_block_srrip(&(policy_data->srrip), 
          &(global_data->srrip), info);
      break; 

    case cache_policy_brrip:
      ret_way = cache_replace_block_brrip(&(policy_data->brrip), info);
      break;

    default:
      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }

#define INVALID_WAY (-1)

  /* If replacement candidate found, update SAP statistics for DRRIP */
  if (ret_way != INVALID_WAY)
  {
    struct cache_block_t *block;

    block = &(policy_data->blocks[ret_way]);
  }

#undef INVALID_WAY

  return ret_way;
}

void cache_access_block_drrip(drrip_data *policy_data, 
  drrip_gdata *global_data, int way, int strm, memory_trace *info)
{
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
    
    case cache_policy_drrip:
      /* Nothing to do */
      break;

    default:
      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }

  switch (GET_CURRENT_POLICY(policy_data, global_data, info)) 
  {
    case cache_policy_srrip:
      cache_access_block_srrip(&(policy_data->srrip), &(global_data->srrip), way, strm, info);
      break;

    case cache_policy_brrip:
      cache_access_block_brrip(&(policy_data->brrip), &(global_data->brrip), 
          way, strm, info);
      break;

    default:
      panic("Invalid follower function %s line %d\n", __FUNCTION__, __LINE__);
  }
}

/* Update state of block. */
void cache_set_block_drrip(drrip_data *policy_data, drrip_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  
  /* Call component policies */
  switch (GET_CURRENT_POLICY(policy_data, global_data, info))
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
struct cache_block_t cache_get_block_drrip(drrip_data *policy_data, drrip_gdata *global_data,
  int way, long long *tag_ptr, enum cache_block_state_t *state_ptr, 
  int *stream_ptr)
{
  struct cache_block_t ret_block;    

  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  assert(tag_ptr);
  assert(state_ptr);
  
  switch (GET_CURRENT_POLICY(policy_data, global_data, (memory_trace *)NULL))
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

#undef PSRRIP
#undef PBRRIP
#undef FSRRIP
#undef FBRRIP
#undef FDRRIP
#undef FDYN
#undef SSRRIP
#undef DFOLLOW
#undef CURRENT_POLICY
#undef DSRRIP
#undef NEW_POLICY
#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
