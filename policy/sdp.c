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
#include "sdp.h"

#define CACHE_SET(cache, set) (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way) (&((set)->blocks[way]))
#define GFX_CORE              (0)

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

#define SDP_MAX_SAMPLES (7)

static int get_set_type_sdp(struct cache_params *params, long long int indx, 
  ub4 sdp_samples)
{
  int   lsb_bits;
  int   msb_bits;
  int   mid_bits;

  /* BS and PS will always be present */
  assert(sdp_samples >= 2 && sdp_samples <= 7);
  
  lsb_bits = indx & 0x0f;
  msb_bits = (indx >> 6) & 0x0f;
  mid_bits = (indx >> 4) & 0x03;
  
  if (lsb_bits == msb_bits && mid_bits == 0 && sdp_samples >= 1)
  {
    return SDP_BS_SAMPLED_SET;
  }
  else
  {
    if (lsb_bits == (~msb_bits & 0x0f) && mid_bits == 0 && sdp_samples >= 2)
    {
      return SDP_PS_SAMPLED_SET;
    }
    else
    {
      if (lsb_bits == msb_bits && mid_bits == 0x01 && sdp_samples >= 3)
      {    
        /* If there is no GPU this is core sample 2, this is done to avoid problems 
         * in policy initializatin. */
        if (params->sdp_gpu_cores)
        {
          return SDP_CS1_SAMPLED_SET;
        }
        else
        {
          return SDP_CS2_SAMPLED_SET;
        }
      }
      else
      {
        if (lsb_bits == (~msb_bits & 0x0f) && mid_bits == 0x01 && sdp_samples >= 4)
        {
          /* If there is no GPU this is core sample 3, this is done to avoid problems 
           * in policy initializatin. */
          if (params->sdp_gpu_cores)
          {
            return SDP_CS2_SAMPLED_SET;
          }
          else
          {
            return SDP_CS3_SAMPLED_SET;
          }
        }
        else
        {
          if (lsb_bits == msb_bits && mid_bits == 0x10 && sdp_samples >= 5)
          {
            /* If there is no GPU this is core sample 4, this is done to avoid problems 
             * in policy initializatin. */
            if (params->sdp_gpu_cores)
            {
              return SDP_CS3_SAMPLED_SET;
            }
            else
            {
              return SDP_CS4_SAMPLED_SET;
            }
          }
          else
          {
            if (lsb_bits == (~msb_bits & 0x0f) && mid_bits == 0x10 && sdp_samples >= 6)
            {
              /* If there is no GPU this is core sample 4, this is done to avoid problems 
               * in policy initializatin. */
              if (params->sdp_gpu_cores)
              {
                return SDP_CS4_SAMPLED_SET;
              }
              else
              {
                return SDP_CS5_SAMPLED_SET;
              }
            }
            else
            {
              if (lsb_bits == msb_bits && mid_bits == 0x11 && sdp_samples == 7)
              {
                /* For core sample 7, GPU must be present. */
                assert(params->sdp_gpu_cores > 0);
                return SDP_CS5_SAMPLED_SET;
              }
            }
          }
        }
      }
    }
  }

  return SDP_FOLLOWER_SET;
}

void cache_init_sdp(long long int set_indx, struct cache_params *params,
  sdp_data *policy_data, sdp_gdata *global_data)
{
  int cores;

  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(global_data);
  
  /* Samples to be used for corrective path (Two fixed samples are used for
   * policy and baseline) */
#define FXD_SAMPL    (2)
#define SDP_SAMPL(p) (p->sdp_gpu_cores + p->sdp_cpu_cores + FXD_SAMPL)

  global_data->sdp_samples    = SDP_SAMPL(params);
  global_data->sdp_gpu_cores  = params->sdp_gpu_cores;
  global_data->sdp_cpu_cores  = params->sdp_cpu_cores;
  global_data->sdp_cpu_tpset  = params->sdp_cpu_tpset;
  global_data->sdp_cpu_tqset  = params->sdp_cpu_tqset;

#define GFX_PRESENT(p)        (p->sdp_gpu_cores != 0)
#define GFX_NOT_PRESENT(p)    (p->sdp_gpu_cores == 0)
#define IS_GFX_STREAM(s)      (s < TST)
#define IS_CPU_STREAM(s)      (s >= TST)
#define GFX_ONLY_STREAM(c, s) ((c == 0 && IS_GFX_STREAM(s)))
#define CPU_ONLY_STREAM(c, s) ((c > 0 && IS_CPU_STREAM(s)))
#define CORE_STREAM(c, s)     (GFX_ONLY_STREAM(c, s)|| CPU_ONLY_STREAM(c, s))

  /* Total cores seen by the policy. It includes CPU as well as GPU. Core id 0 is fixed
   * for GPU, so if GPU is not present total cores are CPU cores + 1. */
  cores = (GFX_PRESENT(params)) ? 
          params->sdp_gpu_cores + params->sdp_cpu_cores : 
          params->sdp_cpu_cores + 1;
  
  /* Initialize policies for various sample sets. It is used for corrective 
   * path samples (BS, PS, CS) */
  switch (get_set_type_sdp(params, set_indx, SDP_SAMPL(params)))
  {
    case SDP_BS_SAMPLED_SET:
      /* For baseline samples, all cores and all stream run 
       * SRRIP.
       *
       * Baseline samples evaluate perfromance of baseline policy.
       */
      cache_init_sap(set_indx, params, &(policy_data->sap), &(global_data->sap));

      /* Set SRRIP for all cores */
      for (int core = 0; core < cores; core++)
      {
        /* TODO: Each core should know streams it supports, for the time it is 
         * assumed that all cores support all streams */
        if (core == GFX_CORE && GFX_PRESENT(params))
        {
          for (ub1 stream = NN + 1; stream <= TST; stream++)
          {
            /* Initialize stream for all cores. If GFX core is not present 
             * only CPU streams are initialized else for GFX core, graphic
             * streams and for CPU per core streams are initialized */
            if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
                (GFX_PRESENT(params) && CORE_STREAM(core, stream)))
            {
              set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap), 
                  core, stream, cache_policy_srrip);
            }
          }
        }
      }

      policy_data->following        = cache_policy_sap;
      policy_data->sdp_sample_type  = SDP_BS_SAMPLED_SET;
      break;

    case SDP_PS_SAMPLED_SET:
      /* For policy sample all cores run Stream Aware Policy (SAP)
       *
       * Policy samples evaluate performance of SAP. 
       */
      cache_init_sap(set_indx, params, &(policy_data->sap), &(global_data->sap));

      policy_data->following        = cache_policy_sap;
      policy_data->sdp_sample_type  = SDP_PS_SAMPLED_SET;
      break;

    case SDP_CS1_SAMPLED_SET:
      /* For core sample 1, core 1 runs SAP and rest of the cores run SRRIP.
       *
       * These sets evaluate SAP performance only for core 1. 
       */
      cache_init_sap(set_indx, params, &(policy_data->sap),
          &(global_data->sap));

      /* Set SAP for core 1 */
      for (ub1 stream = NN + 1; stream <= TST; stream++)
      {
        /* Initialize stream for all cores. If GFX core is not present 
         * only CPU streams are initialized else for GFX core, graphic
         * streams and for CPU per core streams are initialized */
        if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
            (GFX_PRESENT(params) && CORE_STREAM(CS1, stream)))
        {
          set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap), 
              CS1, stream, cache_policy_sap);
        }
      }

      /* Set SRRIP for all other cores */
      for (int core = 1; core < cores; core++)
      {
        for (ub1 stream = NN + 1; stream <= TST && core != CS1; stream++)
        {
          /* Initialize stream for all cores. If GFX core is not present 
           * only CPU streams are initialized else for GFX core, graphic
           * streams and for CPU per core streams are initialized */
          if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
              (GFX_PRESENT(params) && CORE_STREAM(core, stream)))
          {
            set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap),
                core, stream, cache_policy_srrip);
          }
        }
      }

      policy_data->following        = cache_policy_sap;
      policy_data->sdp_sample_type  = SDP_CS1_SAMPLED_SET;
      break;

    case SDP_CS2_SAMPLED_SET:
      /* Initialize SAP */
      cache_init_sap(set_indx, params, &(policy_data->sap), &(global_data->sap));

      /* Set SAP for core 2 */
      for (ub1 stream = NN + 1; stream <= TST; stream++)
      {
        /* Initialize stream for all cores. If GFX core is not present 
         * only CPU streams are initialized else for GFX core, graphic
         * streams and for CPU per core streams are initialized */
        if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
            (GFX_PRESENT(params) && CORE_STREAM(CS2, stream)))
        {
          set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap), 
              CS2, stream, cache_policy_sap);
        }
      }

      /* Set SRRIP for all other cores */
      for (int core = 0; core < cores; core++)
      {
        if (core == GFX_CORE && GFX_PRESENT(params))
        {
          for (ub1 stream = NN + 1; stream <= TST && core != CS2; stream++)
          {
            /* Initialize stream for all cores. If GFX core is not present 
             * only CPU streams are initialized else for GFX core, graphic
             * streams and for CPU per core streams are initialized */
            if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
                (GFX_PRESENT(params) && CORE_STREAM(core, stream)))
            {
              set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap), 
                  core, stream, cache_policy_srrip);
            }
          }
        }
      }

      policy_data->following        = cache_policy_sap;
      policy_data->sdp_sample_type  = SDP_CS2_SAMPLED_SET;
      break;

    case SDP_CS3_SAMPLED_SET:
      /* Initialize SAP */
      cache_init_sap(set_indx, params, &(policy_data->sap), &(global_data->sap));

      /* Set SAP for core 3 */
      for (ub1 stream = NN + 1; stream <= TST; stream++)
      {
        /* Initialize stream for all cores. If GFX core is not present 
         * only CPU streams are initialized else for GFX core, graphic
         * streams and for CPU per core streams are initialized */
        if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
            (GFX_PRESENT(params) && CORE_STREAM(CS3, stream)))
        {
          set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap), 
              CS3, stream, cache_policy_sap);
        }
      }

      /* Set SRRIP for all other cores */
      for (int core = 0; core < cores; core++)
      {
        if (core == GFX_CORE && GFX_PRESENT(params))
        {
          for (ub1 stream = NN + 1; stream <= TST && core != CS3; stream++)
          {
            /* Initialize stream for all cores. If GFX core is not present 
             * only CPU streams are initialized else for GFX core, graphic
             * streams and for CPU per core streams are initialized */
            if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
                (GFX_PRESENT(params) && CORE_STREAM(core, stream)))
            {
              set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap), 
                  core, stream, cache_policy_srrip);
            }
          }
        }
      }

      policy_data->following        = cache_policy_sap;
      policy_data->sdp_sample_type  = SDP_CS3_SAMPLED_SET;
      break;

    case SDP_CS4_SAMPLED_SET:
      /* Initialize SAP */
      cache_init_sap(set_indx, params, &(policy_data->sap), &(global_data->sap));

      /* Set SAP for core 3 */
      for (ub1 stream = NN + 1; stream <= TST; stream++)
      {
        /* Initialize stream for all cores. If GFX core is not present 
         * only CPU streams are initialized else for GFX core, graphic
         * streams and for CPU per core streams are initialized */
        if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
            (GFX_PRESENT(params) && CORE_STREAM(CS4, stream)))
        {
          set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap), 
              CS4, stream, cache_policy_sap);
        }
      }

      /* Set SRRIP for all othre cores */
      for (int core = 0; core < cores; core++)
      {
        if (core == GFX_CORE && GFX_PRESENT(params))
        {
          for (ub1 stream = NN + 1; stream <= TST && core != CS4; stream++)
          {
            /* Initialize stream for all cores. If GFX core is not present 
             * only CPU streams are initialized else for GFX core, graphic
             * streams and for CPU per core streams are initialized */
            if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
                (GFX_PRESENT(params) && CORE_STREAM(core, stream)))
            {
              set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap), 
                  core, stream, cache_policy_srrip);
            }
          }
        }
      }

      policy_data->following        = cache_policy_sap;
      policy_data->sdp_sample_type  = SDP_CS4_SAMPLED_SET;
      break;

    case SDP_CS5_SAMPLED_SET:
      /* Initialize SAP */
      cache_init_sap(set_indx, params, &(policy_data->sap), &(global_data->sap));

      /* Set SAP for core 4 */
      for (ub1 stream = NN + 1; stream <= TST; stream++)
      {
        /* Initialize stream for all cores. If GFX core is not present 
         * only CPU streams are initialized else for GFX core, graphic
         * streams and for CPU per core streams are initialized */
        if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
            (GFX_PRESENT(params) && CORE_STREAM(CS5, stream)))
        {
          set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap), 
              CS5, stream, cache_policy_sap);
        }
      }

      /* Set SRRIP for all othre cores */
      for (int core = 0; core < cores; core++)
      {
        if (core == GFX_CORE && GFX_PRESENT(params))
        {
          for (ub1 stream = NN + 1; stream <= TST && core != CS5; stream++)
          {
            /* Initialize stream for all cores. If GFX core is not present 
             * only CPU streams are initialized else for GFX core, graphic
             * streams and for CPU per core streams are initialized */
            if ((GFX_NOT_PRESENT(params) && IS_CPU_STREAM(stream)) ||
                (GFX_PRESENT(params) && CORE_STREAM(core, stream)))
            {
              set_per_stream_policy_sap(&(policy_data->sap), &(global_data->sap), 
                  core, stream, cache_policy_srrip);
            }
          }
        }
      }

      policy_data->following        = cache_policy_sap;
      policy_data->sdp_sample_type  = SDP_CS5_SAMPLED_SET;
      break;

    case SDP_FOLLOWER_SET:
      cache_init_sap(set_indx, params, &(policy_data->sap), &(global_data->sap));

      policy_data->following        = cache_policy_sap;
      policy_data->sdp_sample_type  = SDP_FOLLOWER_SET;
      break;

    default:
      panic("Unknown set type %s %d\n", __FUNCTION__, __LINE__);
  }

#undef FXD_SAMPL
#undef SDP_SAMPL
#undef GFX_PRESENT
#undef GFX_NOT_PRESENT
#undef IS_GFX_STREAM
#undef IS_CPU_STREAM
#undef GFX_ONLY_STREAM
#undef CPU_ONLY_STREAM
#undef CORE_STREAM
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_sdp(long long int set_indx, sdp_data *policy_data, sdp_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Free component policies */
  cache_free_sap(set_indx, &(policy_data->sap), &(global_data->sap));
}

struct cache_block_t* cache_find_block_sdp(sdp_data *policy_data, long long tag, memory_trace *info)
{
  info->sdp_sample = policy_data->sdp_sample_type;
  return cache_find_block_sap(&(policy_data->sap), tag);
}

void cache_fill_block_sdp(sdp_data *policy_data, sdp_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, int strm,
  memory_trace  *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);

  cache_fill_block_sap(&(policy_data->sap), &(global_data->sap), way, tag, state, strm, info);
}

int cache_replace_block_sdp(sdp_data *policy_data, sdp_gdata *global_data, memory_trace *info)
{
  /* Ensure vaid arguments */
  assert(policy_data);
  assert(global_data);
  
  return cache_replace_block_sap(&(policy_data->sap), &(global_data->sap), info);
}

void cache_access_block_sdp(sdp_data *policy_data, 
  sdp_gdata *global_data, int way, int strm, memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);


  cache_access_block_sap(&(policy_data->sap), &(global_data->sap), way, strm, info);
}

/* Update state of block. */
void cache_set_block_sdp(sdp_data *policy_data, sdp_gdata *global_data, 
  int way, long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info)
{
  /* Ensure valid arguments */
  assert(policy_data);
  
  /* Call component policies */
  cache_set_block_sap(&(policy_data->sap), &(global_data->sap), way, tag, state, stream, info);
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_sdp(sdp_data *policy_data, sdp_gdata *global_data,
  int way, long long *tag_ptr, enum cache_block_state_t *state_ptr, 
  int *stream_ptr)
{
  /* Ensure valid arguments */
  assert(policy_data);
  assert(global_data);
  assert(tag_ptr);
  assert(state_ptr);
  
  return cache_get_block_sap(&(policy_data->sap), &(global_data->sap), way, tag_ptr, 
    state_ptr, stream_ptr);
}
