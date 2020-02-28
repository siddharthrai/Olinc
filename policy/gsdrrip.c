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
#include "gsdrrip.h"
#include "zlib.h"

#define PSEL_WIDTH              (30)
#define PSEL_MIN_VAL            (0x00)  
#define PSEL_MAX_VAL            (1 << PSEL_WIDTH)  
#define PSEL_MID_VAL            (PSEL_MAX_VAL >> 1)

#define SRRIP_SAMPLED_SET       (0)
#define BRRIP_SAMPLED_SET       (1)
#define FOLLOWER_SET            (2)
#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))

#define GST_UNK                 (0)
#define GST1                    (1)
#define GST2                    (2)
#define GST3                    (3)
#define GST4                    (4)
#define GST5                    (5)
#define GST6                    (6)
#define GST7                    (7)
#define GST8                    (8)
#define GST9                    (9)

#define GST_UNKNOWN_SET         (0)
#define GST1_SRRIP_SAMPLED_SET  (1)
#define GST1_BRRIP_SAMPLED_SET  (2)
#define GST2_SRRIP_SAMPLED_SET  (3)
#define GST2_BRRIP_SAMPLED_SET  (4)
#define GST3_SRRIP_SAMPLED_SET  (5)
#define GST3_BRRIP_SAMPLED_SET  (6)
#define GST4_SRRIP_SAMPLED_SET  (7)
#define GST4_BRRIP_SAMPLED_SET  (8)
#define GST5_SRRIP_SAMPLED_SET  (9)
#define GST5_BRRIP_SAMPLED_SET  (10)
#define GST6_SRRIP_SAMPLED_SET  (11)
#define GST6_BRRIP_SAMPLED_SET  (12)
#define GST7_SRRIP_SAMPLED_SET  (13)
#define GST7_BRRIP_SAMPLED_SET  (14)
#define GST8_SRRIP_SAMPLED_SET  (15)
#define GST8_BRRIP_SAMPLED_SET  (16)
#define GST9_SRRIP_SAMPLED_SET  (17)
#define GST9_BRRIP_SAMPLED_SET  (18)
#define GST_FOLLOWER_SET        (19)

#define CPS(i)                    (i == PS)
#define CPS1(i)                   (i == PS1)
#define CPS2(i)                   (i == PS2)
#define CPS3(i)                   (i == PS3)
#define CPS4(i)                   (i == PS4)
#define GPGPU_STREAM(i)           (i == GP)
#define GFX_STREAM(i)             (i < PS)
#define GPU_STREAM(i)             (GPGPU_STREAM(i) || GFX_STREAM(i))
#define CPU_STREAM(i)             (CPS(i) || CPS1(i) || CPS2(i) || CPS3(i) || CPS4(i))

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
#define GET_CURRENT_POLICY(d, gd, s)  (((d)->following[(s)] == cache_policy_srrip ||    \
                                       ((d)->following[(s)] == cache_policy_gsdrrip &&  \
                                        SAT_CTR_VAL((gd)->psel[(s)]) <= PSEL_MID_VAL)) ? \
                                        cache_policy_srrip : cache_policy_brrip)

ub8 fill_count;

static int get_set_type_gsdrrip(long long int indx, ub4 gsdrrip_samples)
{
  int   lsb_bits;
  int   msb_bits;
  int   mid_bits;
  unsigned upper = (indx >> 6) & 0xf;
  unsigned lower = indx & 0xf;
  unsigned middle = (indx >> 4) & 0x3;
  unsigned not_lower = (~lower) & 0xf;
  unsigned alternate1 =  (lower ^ 0xa) & 0xf;
  unsigned alternate2 = (lower ^ 0x5) & 0xf;

  ub1 sample1 = (((upper == lower) && (middle == 0)) ? 1 : 0);
  ub1 sample2 = (((upper == not_lower) && (middle == 0)) ? 1 : 0);
  ub1 sample3 = (((upper == alternate1) && (middle == 0)) ? 1 : 0);
  ub1 sample4 = (((upper == alternate2) && (middle == 0)) ? 1 : 0);
  ub1 sample5 = (((upper == lower) && (middle == 0x3)) ? 1 : 0);
  ub1 sample6 = (((upper == not_lower) && (middle == 0x3)) ? 1 : 0);
  ub1 sample7 = (((upper == alternate1) && (middle == 0x3)) ? 1 : 0);
  ub1 sample8 = (((upper == alternate2) && (middle == 0x3)) ? 1 : 0);
  ub1 sample9 = (((upper == lower) && (middle == 0x1)) ? 1 : 0);
  ub1 sample10 = (((upper == not_lower) && (middle == 0x1)) ? 1 : 0);

  /* BS and PS will always be present */
  assert(gsdrrip_samples >= 1 && gsdrrip_samples <= 18);

  lsb_bits = indx & 0x07;
  msb_bits = (indx >> 7) & 0x07;
  mid_bits = (indx >> 3) & 0x0f;

#if 0
  if (((upper == lower) && (middle == 0)) && gsdrrip_samples >= 1)
  {
    return GST1_SRRIP_SAMPLED_SET;
  }

  if (((upper == not_lower) && (middle == 0)) && gsdrrip_samples >= 2)
  {
    return GST1_BRRIP_SAMPLED_SET;
  }

  if (((upper == alternate1) && (middle == 0)) && gsdrrip_samples >= 3)
  {    
    /* If there is no GPU this is core sample 2, this is done to avoid problems 
     * in policy initializatin. */
    return GST2_SRRIP_SAMPLED_SET;
  }

  if (((upper == alternate2) && (middle == 0)) && gsdrrip_samples >= 4)
  {
    /* If there is no GPU this is core sample 3, this is done to avoid problems 
     * in policy initializatin. */
    return GST2_BRRIP_SAMPLED_SET;
  }

  if (((upper == lower) && (middle == 0x3)) && gsdrrip_samples >= 5)
  {
    /* If there is no GPU this is core sample 4, this is done to avoid problems 
     * in policy initializatin. */
    return GST3_SRRIP_SAMPLED_SET;
  }

  if (((upper == not_lower) && (middle == 0x3)) && gsdrrip_samples >= 6)
  {
    /* If there is no GPU this is core sample 4, this is done to avoid problems 
     * in policy initializatin. */
    return GST3_BRRIP_SAMPLED_SET;
  }

  if (((upper == alternate1) && (middle == 0x3)) && gsdrrip_samples >= 7)
  {
    /* For core sample 7, GPU must be present. */
    return GST4_SRRIP_SAMPLED_SET;
  }

  if (((upper == alternate2) && (middle == 0x3)) && gsdrrip_samples >= 8)
  {
    /* For core sample 7, GPU must be present. */
    return GST4_BRRIP_SAMPLED_SET;
  }

  if (((upper == lower) && (middle == 0x1)) && gsdrrip_samples >= 9)
  {
    /* For core sample 7, GPU must be present. */
    return GST5_SRRIP_SAMPLED_SET;
  }

  if (((upper == not_lower) && (middle == 0x1)) && gsdrrip_samples >= 10)
  {
    /* For core sample 7, GPU must be present. */
    return GST5_BRRIP_SAMPLED_SET;
  }
#endif

  if (lsb_bits == msb_bits && mid_bits == 0 && gsdrrip_samples >= 1)
  {
    return GST1_SRRIP_SAMPLED_SET;
  }

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 0 && gsdrrip_samples >= 2)
  {
    return GST1_BRRIP_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 1 && gsdrrip_samples >= 3)
  {    
    /* If there is no GPU this is core sample 2, this is done to avoid problems 
     * in policy initializatin. */
    return GST2_SRRIP_SAMPLED_SET;
  }

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 1 && gsdrrip_samples >= 4)
  {
    /* If there is no GPU this is core sample 3, this is done to avoid problems 
     * in policy initializatin. */
    return GST2_BRRIP_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 2 && gsdrrip_samples >= 5)
  {
    /* If there is no GPU this is core sample 4, this is done to avoid problems 
     * in policy initializatin. */
    return GST3_SRRIP_SAMPLED_SET;
  }

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 2 && gsdrrip_samples >= 6)
  {
    /* If there is no GPU this is core sample 4, this is done to avoid problems 
     * in policy initializatin. */
    return GST3_BRRIP_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 3 && gsdrrip_samples >= 7)
  {
    /* For core sample 7, GPU must be present. */
    return GST4_SRRIP_SAMPLED_SET;
  }

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 3 && gsdrrip_samples >= 8)
  {
    /* For core sample 7, GPU must be present. */
    return GST4_BRRIP_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 4 && gsdrrip_samples >= 9)
  {
    /* For core sample 7, GPU must be present. */
    return GST5_SRRIP_SAMPLED_SET;
  }

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 4 && gsdrrip_samples >= 10)
  {
    /* For core sample 7, GPU must be present. */
    return GST5_BRRIP_SAMPLED_SET;
  }

#if 0
  if (lsb_bits == msb_bits && mid_bits == 5 && gsdrrip_samples >= 11)
  {
    /* For core sample 7, GPU must be present. */
    return GST6_SRRIP_SAMPLED_SET;
  }

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 5 && gsdrrip_samples >= 12)
  {
    /* For core sample 7, GPU must be present. */
    return GST6_BRRIP_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 6 && gsdrrip_samples >= 13)
  {
    /* For core sample 7, GPU must be present. */
    return GST7_SRRIP_SAMPLED_SET;
  }

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 6 && gsdrrip_samples >= 14)
  {
    /* For core sample 7, GPU must be present. */
    return GST7_BRRIP_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 7 && gsdrrip_samples >= 15)
  {
    /* For core sample 7, GPU must be present. */
    return GST8_SRRIP_SAMPLED_SET;
  }

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 7 && gsdrrip_samples >= 16)
  {
    /* For core sample 7, GPU must be present. */
    return GST8_BRRIP_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 8 && gsdrrip_samples >= 17)
  {
    /* For core sample 7, GPU must be present. */
    return GST9_SRRIP_SAMPLED_SET;
  }

  if (lsb_bits == (~msb_bits & 0x07) && mid_bits == 8 && gsdrrip_samples == 18)
  {
    /* For core sample 7, GPU must be present. */
    return GST9_BRRIP_SAMPLED_SET;
  }
#endif

  return GST_FOLLOWER_SET;
}

static ub1 cache_get_gsdrrip_stream(gsdrrip_gdata *global_data, ub1 stream, 
    ub1 core)
{
  ub1 ret_stream;
  
  ret_stream = NN;

  /* Temporary, until trace generation is fixed */
  if (GFX_STREAM(stream))
  {
    core = 0;
  }
  
  if (global_data->gsdrrip_gpu_enable)
  {
    switch (stream)
    {
      case CS:
      case ZS:
      case TS:
      case BS:
      case IS:
      case HS:
      case NS:
      case XS:
      case DS:
        assert(core == 0);
        ret_stream = GST1;
        break;

      case PS:
      case PS1:
      case PS2:
      case PS3:
      case PS4:
        ret_stream = GST1 + core;
        break;

      default:
        panic("Unknown stream type %s %d %d\n", __FUNCTION__, __LINE__, stream);
    }
  }
  else
  {
    if (global_data->gsdrrip_gpgpu_enable)
    {
      switch (stream)
      {
        case GP:
          assert(core == 0);
          ret_stream = GST1;
          break;

        case PS:
        case PS1:
        case PS2:
        case PS3:
        case PS4:
          ret_stream = GST1 + core;
          break;

        default:
          panic("Unknown stream type %s %d %d\n", __FUNCTION__, __LINE__, stream);
      }
    }
    else
    {
      assert(CPU_STREAM(stream) == TRUE);
      ret_stream = GST1 + core;
    }
  }
  
  assert(ret_stream <= global_data->gsdrrip_streams);

  return ret_stream;
}

/* Public interface to initialize SAP statistics */
void cache_init_gsdrrip_stats(gsdrrip_stats *stats)
{
  if (!stats->gsdrrip_stat_file)
  {
    /* Open statistics file */
    stats->gsdrrip_stat_file = gzopen("PC-GSDRRIP-stats.trace.csv.gz", "wb9");

    assert(stats->gsdrrip_stat_file);

    stats->gsdrrip_srrip_samples  = 0;
    stats->gsdrrip_brrip_samples  = 0;
    stats->gsdrrip_srrip_fill     = 0;
    stats->gsdrrip_brrip_fill     = 0;
    stats->gsdrrip_fill_2         = 0;
    stats->gsdrrip_fill_3         = 0;
    stats->gsdrrip_hdr_printed    = FALSE;
    stats->next_schedule          = 500 * 1024;
  }
}

/* Public interface to finalize SAP statistic */
void cache_fini_gsdrrip_stats(gsdrrip_stats *stats)
{
  /* If stats has not been printed, assume it is an offline simulation,
   * and, dump stats now. */
  if (stats->gsdrrip_hdr_printed == FALSE)
  {
    cache_dump_gsdrrip_stats(stats, 0);
  }

  gzclose(stats->gsdrrip_stat_file);
}

/* Public interface to dump periodic SAP statistics */
void cache_dump_gsdrrip_stats(gsdrrip_stats *stats, ub8 cycle)
{
  /* Print header if not already done */
  if (stats->gsdrrip_hdr_printed == FALSE)
  {
    gzprintf(stats->gsdrrip_stat_file,
        "CYCLE; SRRIPSAMPLE; BRRIPSAMPLE; SRRIPFILL; BRRIPFILL; FILL2; FILL3\n");

    stats->gsdrrip_hdr_printed = TRUE;
  }
  
  /* Dump current counter values */
  gzprintf(stats->gsdrrip_stat_file,
      "%ld; %ld; %ld; %ld; %ld %ld %ld\n", cycle, stats->gsdrrip_srrip_samples, 
      stats->gsdrrip_brrip_samples, stats->gsdrrip_srrip_fill, 
      stats->gsdrrip_brrip_fill, stats->gsdrrip_fill_2, stats->gsdrrip_fill_3);
  
  printf("Total fill:%ld SRRIP fill: %ld BRRIP fill %ld\n", 
      fill_count, stats->sample_srrip_fill, stats->sample_brrip_fill);

  /* Reset all stat counters */
  stats->gsdrrip_srrip_fill = 0;
  stats->gsdrrip_brrip_fill = 0;
  stats->gsdrrip_fill_2     = 0;
  stats->gsdrrip_fill_3     = 0;
}

/* Initialize SRRIP from GSDRRIP policy data */
static void cache_init_srrip_from_gsdrrip(struct cache_params *params, srrip_data *policy_data,
  gsdrrip_data *gsdrrip_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(gsdrrip_policy_data);

  /* Create per rrpv buckets */
  SRRIP_DATA_VALID_HEAD(policy_data) = GSDRRIP_DATA_VALID_HEAD(gsdrrip_policy_data);
  SRRIP_DATA_VALID_TAIL(policy_data) = GSDRRIP_DATA_VALID_TAIL(gsdrrip_policy_data);
  
  assert(SRRIP_DATA_VALID_HEAD(policy_data));
  assert(SRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIP_DATA_MAX_RRPV(policy_data)    = params->max_rrpv;
  SRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  /* Create array of blocks */
  SRRIP_DATA_BLOCKS(policy_data) = GSDRRIP_DATA_BLOCKS(gsdrrip_policy_data);

  SRRIP_DATA_FREE_HLST(policy_data) = GSDRRIP_DATA_FREE_HLST(gsdrrip_policy_data);
  SRRIP_DATA_FREE_TLST(policy_data) = GSDRRIP_DATA_FREE_TLST(gsdrrip_policy_data);

  /* Set current and default fill policy to SRRIP */
  SRRIP_DATA_CFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DFPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DAPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_CRPOLICY(policy_data) = cache_policy_srrip;
  SRRIP_DATA_DRPOLICY(policy_data) = cache_policy_srrip;

  assert(SRRIP_DATA_MAX_RRPV(policy_data) != 0);
}

/* Initialize BRRIP for GSDRRIP policy data */
static void cache_init_brrip_from_gsdrrip(struct cache_params *params, brrip_data *policy_data,
  gsdrrip_data *gsdrrip_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(gsdrrip_policy_data);

  /* Create per rrpv buckets */
  BRRIP_DATA_VALID_HEAD(policy_data) = GSDRRIP_DATA_VALID_HEAD(gsdrrip_policy_data);
  BRRIP_DATA_VALID_TAIL(policy_data) = GSDRRIP_DATA_VALID_TAIL(gsdrrip_policy_data);
  
  assert(BRRIP_DATA_VALID_HEAD(policy_data));
  assert(BRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  BRRIP_DATA_MAX_RRPV(policy_data)    = params->max_rrpv;
  BRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  /* Create array of blocks */
  BRRIP_DATA_BLOCKS(policy_data)    = GSDRRIP_DATA_BLOCKS(gsdrrip_policy_data);

  BRRIP_DATA_FREE_HLST(policy_data) = GSDRRIP_DATA_FREE_HLST(gsdrrip_policy_data);
  BRRIP_DATA_FREE_TLST(policy_data) = GSDRRIP_DATA_FREE_TLST(gsdrrip_policy_data);

  /* Set current and default fill policy to SRRIP */
  BRRIP_DATA_CFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CRPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DRPOLICY(policy_data) = cache_policy_brrip;

  assert(BRRIP_DATA_MAX_RRPV(policy_data) != 0);
}

static void cache_set_following_srrip(gsdrrip_data *policy_data, 
    gsdrrip_gdata *global_data, ub4 stream)
{
  assert(global_data);
  assert(policy_data);
  assert(stream <= global_data->gsdrrip_streams);

  for (ub4 i = 0; i <= global_data->gsdrrip_streams; i++) 
  {
    if (i == stream)
    {
      policy_data->following[i] = cache_policy_srrip;
    }
    else
    {
      policy_data->following[i] = cache_policy_gsdrrip;
    }
  }
}

static void cache_set_following_brrip(gsdrrip_data *policy_data, 
    gsdrrip_gdata *global_data, ub4 stream)
{
  assert(global_data);
  assert(policy_data);
  assert(stream <= global_data->gsdrrip_streams);

  for (ub4 i = 0; i <= global_data->gsdrrip_streams; i++) 
  {
    if (i == stream)
    {
      policy_data->following[i] = cache_policy_brrip;
    }
    else
    {
      policy_data->following[i] = cache_policy_gsdrrip;
    }
  }
}

static void cache_set_following_gsdrrip(gsdrrip_data *policy_data, 
    gsdrrip_gdata *global_data)
{
  assert(policy_data);

  for (ub4 i = 0; i <= global_data->gsdrrip_streams; i++) 
  {
    policy_data->following[i] = cache_policy_gsdrrip;
  }
}

void cache_init_gsdrrip(long long int set_indx, struct cache_params *params, gsdrrip_data *policy_data,
  gsdrrip_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);

  if (set_indx == 0)
  {
    /* TODO: allocate psel for all streams */
    global_data->psel = xcalloc(params->streams + 1, sizeof(sctr));
    assert(global_data->psel);
    
    if (params->gsdrrip_gpu_enable)
    {
      assert(params->gsdrrip_gpgpu_enable == FALSE);
      assert(params->streams >= 5);
    }

    if (params->gsdrrip_gpgpu_enable)
    {
      assert(params->gsdrrip_gpu_enable == FALSE);
      assert(params->streams >= 1);
    }

    /* Initialize policy selection counter for all streams */
    for (ub4 i = 0; i <= params->streams; i++)
    {
      SAT_CTR_INI(global_data->psel[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_SET(global_data->psel[i], PSEL_MID_VAL);
    }
    
    global_data->gsdrrip_streams      = params->streams;
    global_data->gsdrrip_gpu_enable   = params->gsdrrip_gpu_enable;
    global_data->gsdrrip_gpgpu_enable = params->gsdrrip_gpgpu_enable;
    global_data->gsdrrip_cpu_enable   = params->gsdrrip_cpu_enable;
    
    global_data->stats.gsdrrip_srrip_samples = 0;
    global_data->stats.gsdrrip_brrip_samples = 0;
    global_data->stats.gsdrrip_srrip_fill    = 0;
    global_data->stats.gsdrrip_brrip_fill    = 0;
    global_data->stats.gsdrrip_fill_2        = 0;
    global_data->stats.gsdrrip_fill_3        = 0;
    global_data->stats.sample_srrip_fill     = 0;
    global_data->stats.sample_brrip_fill     = 0;
    global_data->stats.sample_srrip_hit      = 0;
    global_data->stats.sample_brrip_hit      = 0;
  
#define BRRIP_CTR_WIDTH     (8)    
#define BRRIP_PSEL_MIN_VAL  (0)    
#define BRRIP_PSEL_MAX_VAL  (255)    

    /* Initialize BRRIP set wide data */
    SAT_CTR_INI(global_data->brrip.access_ctr, BRRIP_CTR_WIDTH, 
      BRRIP_PSEL_MIN_VAL, BRRIP_PSEL_MAX_VAL);
    global_data->brrip.threshold = params->threshold;

    SAT_CTR_INI(global_data->access_count, BRRIP_CTR_WIDTH, 
      BRRIP_PSEL_MIN_VAL, BRRIP_PSEL_MAX_VAL);

#undef BRRIP_CTR_WIDTH
#undef BRRIP_PSEL_MIN_VAL
#undef BRRIP_PSEL_MAX_VAL

    /* Initialize GSDRRIP stats */
    cache_init_gsdrrip_stats(&(global_data->stats));

  }
  
  /* Allocate per-stream following array */ 
  GSDRRIP_DATA_FOLLOWING(policy_data) = 
    (enum cache_policy_t *)xcalloc((params->streams + 1), sizeof (enum cache_policy_t));
  assert(GSDRRIP_DATA_FOLLOWING(policy_data));

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  GSDRRIP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  GSDRRIP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC

  assert(GSDRRIP_DATA_VALID_HEAD(policy_data));
  assert(GSDRRIP_DATA_VALID_TAIL(policy_data));

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    GSDRRIP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    GSDRRIP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    GSDRRIP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  GSDRRIP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc((size), sizeof(list_head_t)))

  GSDRRIP_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(GSDRRIP_DATA_FREE_HLST(policy_data));

  GSDRRIP_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(GSDRRIP_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  GSDRRIP_DATA_FREE_HEAD(policy_data) = &(GSDRRIP_DATA_BLOCKS(policy_data)[0]);
  GSDRRIP_DATA_FREE_TAIL(policy_data) = &(GSDRRIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&GSDRRIP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&GSDRRIP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&GSDRRIP_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&GSDRRIP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&GSDRRIP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&GSDRRIP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

#undef MAX_RRPV

#define PS_SAMPLES (2)  /* # per-stream sample */

  switch (get_set_type_gsdrrip(set_indx, global_data->gsdrrip_streams * PS_SAMPLES))
  {
    case GST1_SRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_srrip(policy_data, global_data, GST1);

      /* Increment SRRIP sample count */
      global_data->stats.gsdrrip_srrip_samples += 1;

      break;

    case GST1_BRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);
      
      /* Set policy followed by sample */
      cache_set_following_brrip(policy_data, global_data, GST1);

      /* Increment BRRIP sample count */
      global_data->stats.gsdrrip_brrip_samples += 1;

      break;

    case GST2_SRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_srrip(policy_data, global_data, GST2);

      /* Increment SRRIP sample count */
      global_data->stats.gsdrrip_srrip_samples += 1;

      break;

    case GST2_BRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);
      
      /* Set policy followed by sample */
      cache_set_following_brrip(policy_data, global_data, GST2);

      /* Increment BRRIP sample count */
      global_data->stats.gsdrrip_brrip_samples += 1;

      break;

    case GST3_SRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_srrip(policy_data, global_data, GST3);

      /* Increment SRRIP sample count */
      global_data->stats.gsdrrip_srrip_samples += 1;

      break;

    case GST3_BRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);
      
      /* Set policy followed by sample */
      cache_set_following_brrip(policy_data, global_data, GST3);

      /* Increment BRRIP sample count */
      global_data->stats.gsdrrip_brrip_samples += 1;

      break;

    case GST4_SRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_srrip(policy_data, global_data, GST4);

      /* Increment SRRIP sample count */
      global_data->stats.gsdrrip_srrip_samples += 1;

      break;

    case GST4_BRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);
      
      /* Set policy followed by sample */
      cache_set_following_brrip(policy_data, global_data, GST4);

      /* Increment BRRIP sample count */
      global_data->stats.gsdrrip_brrip_samples += 1;

      break;

    case GST5_SRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_srrip(policy_data, global_data, GST5);

      /* Increment SRRIP sample count */
      global_data->stats.gsdrrip_srrip_samples += 1;

      break;

    case GST5_BRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);
      
      /* Set policy followed by sample */
      cache_set_following_brrip(policy_data, global_data, GST5);

      /* Increment BRRIP sample count */
      global_data->stats.gsdrrip_brrip_samples += 1;

      break;

    case GST6_SRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_srrip(policy_data, global_data, GST6);

      /* Increment SRRIP sample count */
      global_data->stats.gsdrrip_srrip_samples += 1;

      break;

    case GST6_BRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);
      
      /* Set policy followed by sample */
      cache_set_following_brrip(policy_data, global_data, GST6);

      /* Increment BRRIP sample count */
      global_data->stats.gsdrrip_brrip_samples += 1;

      break;

    case GST7_SRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_srrip(policy_data, global_data, GST7);

      /* Increment SRRIP sample count */
      global_data->stats.gsdrrip_srrip_samples += 1;

      break;

    case GST7_BRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);
      
      /* Set policy followed by sample */
      cache_set_following_brrip(policy_data, global_data, GST7);

      /* Increment BRRIP sample count */
      global_data->stats.gsdrrip_brrip_samples += 1;

      break;

    case GST8_SRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_srrip(policy_data, global_data, GST8);

      /* Increment SRRIP sample count */
      global_data->stats.gsdrrip_srrip_samples += 1;

      break;

    case GST8_BRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);
      cache_set_following_gsdrrip(policy_data, global_data);
      
      /* Set policy followed by sample */
      cache_set_following_brrip(policy_data, global_data, GST8);

      /* Increment BRRIP sample count */
      global_data->stats.gsdrrip_brrip_samples += 1;

      break;

    case GST9_SRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_srrip(policy_data, global_data, GST9);

      /* Increment SRRIP sample count */
      global_data->stats.gsdrrip_srrip_samples += 1;

      break;

    case GST9_BRRIP_SAMPLED_SET:
      /* Initialize SRRIP and BRRIP per policy data using GSDRRIP data */
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_brrip(policy_data, global_data, GST9);

      /* Increment SRRIP sample count */
      global_data->stats.gsdrrip_brrip_samples += 1;

      break;

    case GST_FOLLOWER_SET:
      cache_init_srrip_from_gsdrrip(params, &(policy_data->srrip), policy_data);
      cache_init_brrip_from_gsdrrip(params, &(policy_data->brrip), policy_data);

      /* Set policy followed by sample */
      cache_set_following_gsdrrip(policy_data, global_data);

      break;

    default:
      panic("Unknown set type %s %d\n", __FUNCTION__, __LINE__);
  }

  policy_data->set_type = get_set_type_gsdrrip(set_indx, global_data->gsdrrip_streams * PS_SAMPLES);

#undef PS_SAMPLES
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_gsdrrip(unsigned int set_indx, gsdrrip_data *policy_data, 
  gsdrrip_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Free all data blocks */
  free(GSDRRIP_DATA_BLOCKS(policy_data));
  free(GSDRRIP_DATA_FOLLOWING(policy_data));

  /* Free valid head buckets */
  if (GSDRRIP_DATA_VALID_HEAD(policy_data))
  {
    free(GSDRRIP_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (GSDRRIP_DATA_VALID_TAIL(policy_data))
  {
    free(GSDRRIP_DATA_VALID_TAIL(policy_data));
  }
  
  /* Free SAP statistics for GSDRRIP */
  if (set_indx == 0)
  {
    free(global_data->psel);

    cache_fini_gsdrrip_stats(&(global_data->stats));
  }
}

struct cache_block_t* cache_find_block_gsdrrip(gsdrrip_data *policy_data, long long tag)
{
  int    max_rrpv;
  struct cache_block_t *head;
  struct cache_block_t *node;

  max_rrpv  = policy_data->srrip.max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = GSDRRIP_DATA_VALID_HEAD(policy_data)[rrpv].head;

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

void cache_fill_block_gsdrrip(gsdrrip_data *policy_data, gsdrrip_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int strm,
  memory_trace  *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);

#define GSSTRM(g, i)  (cache_get_gsdrrip_stream((g), (i)->stream, (i)->core))

  assert(policy_data->following[GSSTRM(global_data, info)] == cache_policy_srrip ||
    policy_data->following[GSSTRM(global_data, info)] == cache_policy_brrip || 
    policy_data->following[GSSTRM(global_data, info)] == cache_policy_gsdrrip);

  if (info && info->fill)
  {
    fill_count++;

    assert(GSSTRM(global_data, info) == GST1 || GSSTRM(global_data, info) == GST2);

#if 0
    switch (policy_data->following[GSSTRM(global_data, info)])
    {
      case cache_policy_srrip:
        /* Increment fill stats for SRRIP */
#if 0
        global_data->stats.sample_srrip_fill += 1;
#endif
        SAT_CTR_INC(global_data->psel[GSSTRM(global_data, info)]);
        break;

      case cache_policy_brrip:
#if 0
        global_data->stats.sample_brrip_fill += 1;
#endif
        SAT_CTR_DEC(global_data->psel[GSSTRM(global_data, info)]);
        break;

      case cache_policy_gsdrrip:
        /* Nothing to do */
        break;

      default:
        panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
    }
#endif
    switch (policy_data->set_type)
    {
      case GST1_SRRIP_SAMPLED_SET:
        SAT_CTR_INC(global_data->psel[GST1]);
        break;

      case GST1_BRRIP_SAMPLED_SET:
        SAT_CTR_DEC(global_data->psel[GST1]);
        break;

      case GST2_SRRIP_SAMPLED_SET:
        SAT_CTR_INC(global_data->psel[GST2]);
        break;

      case GST2_BRRIP_SAMPLED_SET:
        SAT_CTR_DEC(global_data->psel[GST2]);
        break;

      case GST3_SRRIP_SAMPLED_SET:
        SAT_CTR_INC(global_data->psel[GST3]);
        break;

      case GST3_BRRIP_SAMPLED_SET:
        SAT_CTR_DEC(global_data->psel[GST3]);
        break;

      case GST4_SRRIP_SAMPLED_SET:
        SAT_CTR_INC(global_data->psel[GST4]);
        break;

      case GST4_BRRIP_SAMPLED_SET:
        SAT_CTR_DEC(global_data->psel[GST4]);
        break;

      case GST5_SRRIP_SAMPLED_SET:
        SAT_CTR_INC(global_data->psel[GST5]);
        break;

      case GST5_BRRIP_SAMPLED_SET:
        SAT_CTR_DEC(global_data->psel[GST5]);
        break;

      default:
        break;
    }

    if ((policy_data->set_type == GST1_SRRIP_SAMPLED_SET) || 
        (policy_data->set_type == GST2_SRRIP_SAMPLED_SET))
    {
      global_data->stats.sample_srrip_fill += 1;
    }
  }

#define CTR_VAL(d)    (SAT_CTR_VAL((d)->brrip.access_ctr))
#define ACTR_VAL(d)   (SAT_CTR_VAL((d)->access_count))
#define THRESHOLD(d)  ((d)->brrip.threshold)

  /* Fill block in all component policies */
  switch (GET_CURRENT_POLICY(policy_data, global_data, GSSTRM(global_data, info)))
  {
    case cache_policy_srrip:
      cache_fill_block_srrip(&(policy_data->srrip), &(global_data->srrip), way, tag, state, strm, 
          info);

      /* For GSDRRIP, BRRIP access counter is incremented on access to all sets
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
      if (CPU_STREAM(info->stream))
        global_data->stats.gsdrrip_srrip_fill += 1;

      global_data->stats.gsdrrip_fill_2     += 1; 
      break;

    case cache_policy_brrip:
      /* Find fill RRPV to update stats */
      if (CTR_VAL(global_data) == 0) 
      {
        global_data->stats.gsdrrip_fill_2 += 1; 
      }
      else
      {
        global_data->stats.gsdrrip_fill_3 += 1; 
      }

      if (info && info->fill)
      {
        SAT_CTR_INC(global_data->access_count);

        if (ACTR_VAL(global_data) == THRESHOLD(global_data))
        {
          SAT_CTR_RST(global_data->access_count);
        }
      }

      CTR_VAL(global_data) = ACTR_VAL(global_data);

      cache_fill_block_brrip(&(policy_data->brrip), &(global_data->brrip), way,
          tag, state, strm, info);

      /* Increment fill stats for SRRIP */
      if (CPU_STREAM(info->stream))
      {
        global_data->stats.gsdrrip_brrip_fill += 1;
      }
      break;

    default:
      panic("Invalid follower function %s line %d\n", __FUNCTION__, __LINE__);
  }

#undef CTR_VAL
#undef ACTR_VAL
#undef THRESHOLD
#undef GSSTRM
}

int cache_replace_block_gsdrrip(gsdrrip_data *policy_data, 
    gsdrrip_gdata *global_data, memory_trace *info)
{
  int ret_way;

  /* Ensure vaid arguments */
  assert(policy_data);
  assert(global_data);
  
#define GSSTRM(g, i) (cache_get_gsdrrip_stream((g), (i)->stream, (i)->core))

  /* According to the policy choose a replacement candidate */
  switch (GET_CURRENT_POLICY(policy_data, global_data, GSSTRM(global_data, info)))
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

#undef GSSTRM

#define INVALID_WAY (-1)

  /* If replacement candidate found, update SAP statistics for GSDRRIP */
  if (ret_way != INVALID_WAY)
  {
    struct cache_block_t *block;

    block = &(policy_data->blocks[ret_way]);
  }

#undef INVALID_WAY

  return ret_way;
}

void cache_access_block_gsdrrip(gsdrrip_data *policy_data, 
  gsdrrip_gdata *global_data, int way, int strm, memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  
#define GSSTRM(g, i) (cache_get_gsdrrip_stream((g), (i)->stream, (i)->core))

  switch (policy_data->following[GSSTRM(global_data, info)])
  {
    case cache_policy_srrip:
      /* Increment fill stats for SRRIP */
      global_data->stats.sample_srrip_hit += 1;
      break;

    case cache_policy_brrip:
      global_data->stats.sample_brrip_hit += 1;
      break;
    
    case cache_policy_gsdrrip:
      /* Nothing to do */
      break;

    default:
      panic("Invalid policy function %s line %d\n", __FUNCTION__, __LINE__);
  }

  switch (GET_CURRENT_POLICY(policy_data, global_data, GSSTRM(global_data, info)))
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

#undef GSSTRM
}

/* Update state of block. */
void cache_set_block_gsdrrip(gsdrrip_data *policy_data, gsdrrip_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  
#define GSSTRM(g, i) (cache_get_gsdrrip_stream((g), (i)->stream, (i)->core))

  /* Call component policies */
  switch (GET_CURRENT_POLICY(policy_data, global_data, GSSTRM(global_data, info)))
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

#undef GSSTRM
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_gsdrrip(gsdrrip_data *policy_data, 
  gsdrrip_gdata *global_data, int way, long long *tag_ptr, 
  enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  struct cache_block_t ret_block;    

  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  assert(tag_ptr);
  assert(state_ptr);
  
  switch (GET_CURRENT_POLICY(policy_data, global_data, GST1))
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

#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
#undef GST_UNK
#undef GST1
#undef GST2
#undef GST3
#undef GST4
#undef GST5
#undef GST6
#undef GST7
#undef GST8
#undef GST9
#undef GST_UNKNOWN_SET
#undef GST1_SRRIP_SAMPLED_SET
#undef GST1_BRRIP_SAMPLED_SET
#undef GST2_SRRIP_SAMPLED_SET
#undef GST2_BRRIP_SAMPLED_SET
#undef GST3_SRRIP_SAMPLED_SET
#undef GST3_BRRIP_SAMPLED_SET
#undef GST4_SRRIP_SAMPLED_SET
#undef GST4_BRRIP_SAMPLED_SET
#undef GST5_SRRIP_SAMPLED_SET
#undef GST5_BRRIP_SAMPLED_SET
#undef GST6_SRRIP_SAMPLED_SET
#undef GST6_BRRIP_SAMPLED_SET
#undef GST7_SRRIP_SAMPLED_SET
#undef GST7_BRRIP_SAMPLED_SET
#undef GST8_SRRIP_SAMPLED_SET
#undef GST8_BRRIP_SAMPLED_SET
#undef GST9_SRRIP_SAMPLED_SET
#undef GST9_BRRIP_SAMPLED_SET
#undef GST_FOLLOWER_SET
#undef CPS
#undef CPS1
#undef CPS2
#undef CPS3
#undef CPS4
#undef GPGPU_STREAM
#undef GFX_STREAM
#undef GPU_STREAM
#undef CPU_STREAM

