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

#define INTERVAL_SIZE             (1 << 19)
#define PAGE_COVERAGE             (8)
#define PSEL_WIDTH                (30)
#define PSEL_MIN_VAL              (0x00)  
#define PSEL_MAX_VAL              (1 << PSEL_WIDTH)  
#define PSEL_MID_VAL              (PSEL_MAX_VAL / 2)

#define ECTR_WIDTH                (20)
#define ECTR_MIN_VAL              (0x00)  
#define ECTR_MAX_VAL              (0xfffff)  
#define ECTR_MID_VAL              (0x3ff)

#define BYTH_NUMR                 (1)
#define BYTH_DENM                 (8)
#define SMTH_NUMR                 (1)
#define SMTH_DENM                 (8)
#define SHTH_NUMR                 (1)
#define SHTH_DENM                 (16)
#define BYPASS_ACCESS_TH          (128 * 1024)
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
#define KNOWN_STREAM(s)           (s == CS || s == ZS || s == TS || s == BS || s >= PS)
#define OTHER_STREAM(s)           (!(KNOWN_STREAM(s)))
#define NEW_STREAM(i)             (KNOWN_STREAM((i)->stream) ? (i)->stream : OS)
#define DYNBP(b ,o, i)            (DYNB(b ,o) ? DBS : DYNP(b ,o) ? DPS : NEW_STREAM(i))
#define SARP_STREAM(b, o, i)      (DYNC(b, o) ? DCS : DYNZ(b, o) ? DZS : DYNBP(b, o, i))
#define OLD_STREAM(b, o)          (KNOWN_STREAM((b).stream[o]) ? (b).stream[o] : OS)
#define ODYNBP(b ,o, i)           (DYNB(b ,o) ? DBS : DYNP(b ,o) ? DPS : OLD_STREAM(b, o))
#define SARP_OSTREAM(b, o, i)     (DYNC(b, o) ? DCS : DYNZ(b, o) ? DZS : ODYNBP(b, o, i))
#define SAMPLER_INDX(i, s)        ((((i)->address >> (s)->log_grain_size)) & ((s)->sets - 1))
#define SAMPLER_TAG(i, s)         ((i)->address >> (s)->log_grain_size)
#define SAMPLER_OFFSET(i, s)      (((i)->address >> (s)->log_block_size) & ((s)->entry_size - 1))
#define CR_DTH                    (2)
#define CR_NTH                    (1)
#define KNOWN_CSTREAM(s)          (s == CS || s == ZS || s == TS)
#define STRMSPD(g, s)             ((g)->speedup_count[s])
#define SPDSTRM(g)                ((g)->speedup_stream)
#define INSPD_RANGE(g, s)         (STRMSPD(g, s) * CR_DTH >= STRMSPD(g, SPDSTRM(g)) * CR_NTH)
#define CRITICAL_COND(g, s)       (s == SPDSTRM(g) || INSPD_RANGE(g, s))
#define SMPLR(g)                  ((g)->sampler)
#define GST(i)                    ((i)->stream)
#define CPRD(g, s)                ((g)->sampler->epoch_count == 0 && (s == CS || s == BS))
#define ALWAYS_CTRCL(g, s)        (CPRD(g, s) || s == PS || s == BS)
#define FOUND_CTRCL(g, s)         (KNOWN_CSTREAM(s) && CRITICAL_COND(g, s))
#define CHK_CRTCL(g, s)           (ALWAYS_CTRCL(g, s) || FOUND_CTRCL(g, s))
#define CHK_CTA(g, i)             (SMPLR(g)->perfctr.fill_count[DCS])
#define CHK_BTA(g, i)             (SMPLR(g)->perfctr.fill_count[DBS])
#define CTC_CNT(g, i)             (CHK_CTA(g, i) && GST(i) == CS)
#define BTC_CNT(g, i)             (CHK_BTA(g, i) && GST(i) == BS)
#define IS_INTERS(g, i)           ((i)->spill && (CTC_CNT(g, i)|| BTC_CNT(g, i)))
#define INSTRS_CRTCL(g, i)        (CHK_CRTCL(g, TS) || CHK_CRTCL(g, GST(i)))
#define CRITICAL_STREAM(g, i)     (IS_INTERS(g, i) ? INSTRS_CRTCL(g, i) : CHK_CRTCL(g, GST(i)))

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
#define GFILL_VAL(g)                  (SAT_CTR_VAL((g)->gpu_srrip_brrip_psel))
#define PCBRRIP(p, g, i)              ((BRRIP_VAL(g) <= PSEL_MID_VAL) ? PCSRRIPGBRRIP(i) : PCBRRIPGBRRIP(i))
#define PCSRRIP(p, g, i)              ((SRRIP_VAL(g) <= PSEL_MID_VAL) ? PCSRRIPGSRRIP(i) : PCBRRIPGSRRIP(i))
#define FANDH(i, h)                   ((i)->fill == TRUE && (h) == TRUE)
#define FANDM(i, h)                   (GPU_STREAM(i->stream) && (i)->fill == TRUE && (h) == FALSE)
#define DPSARP(p, g, i)               ((SARP_VAL(g) <= PSEL_MID_VAL) ? PCSRRIP(p, g, i) : PCBRRIP(p, g, i))
#define GFENBL(g)                     ((g)->gpu_fill_enable == TRUE)
#define DGFILL(p, g, i)               ((GFILL_VAL(g) <= PSEL_MID_VAL) ? PSRRIP : PBRRIP)
#define CHGFILL(p, g, i)              (GFENBL(g) ? DGFILL(p, g, i) : PSRRIP)
#define SPSARP(p, g, i, f)            (FSARP(f) ? CHGFILL(p, g, i) : f)
#define CURRENT_POLICY(p, g, f, i, h) ((FANDM(i, h)) ? SPSARP(p, g, i, f) : f)
#define FOLLOW_POLICY(f, i)           ((!FSARP(f) && ((i)->fill)) ? f : PSARP)

/* Macros to obtain signature */
#define SHIP_MAX                      (7)
#define SIGNSIZE(g)                   ((g)->sign_size)
#define SHCTSIZE(g)                   ((g)->shct_size)
#define COREBTS(g)                    ((g)->core_size)
#define SIGMAX_VAL(g)                 (((1 << SIGNSIZE(g)) - 1))
#define SIGNMASK(g)                   (SIGMAX_VAL(g) << SHCTSIZE(g))
#define SIGNP(g, i)                   (((i)->pc) & SIGMAX_VAL(g))
#define SIGNM(g, i)                   ((((i)->address >> SHCTSIZE(g)) & SIGMAX_VAL(g)))
#define SIGNCORE(g, i)                (((i)->core - 1) << (SHCTSIZE(g) - COREBTS(g)))
#define GET_SSIGN(g, i)               ((i)->stream < PS ? SIGNM(g, i) : SIGNP(g, i) ^ SIGNCORE(g, i))
#define SHIPSIGN(g, i)                (GET_SSIGN(g, i))

#define CACHE_INC_SHCT(block, g)                              \
do                                                            \
{                                                             \
  assert(block->ship_sign <= SIGMAX_VAL(g));                  \
                                                              \
  if ((g)->ship_shct[block->ship_sign] < SHIP_MAX)            \
  {                                                           \
    (g)->ship_shct[block->ship_sign] += 1;                    \
  }                                                           \
}while(0)

#define CACHE_DEC_SHCT(block, g)                              \
do                                                            \
{                                                             \
  assert(block->ship_sign <= SIGMAX_VAL(g));                  \
                                                              \
  if (!block->access)                                         \
  {                                                           \
    if ((g)->ship_shct[block->ship_sign] > 0)                 \
    {                                                         \
      (g)->ship_shct[block->ship_sign] -= 1;                  \
    }                                                         \
  }                                                           \
}while(0)

extern struct cpu_t *cpu;
extern ffifo_info ffifo_global_info;
extern ub1 sdp_base_sample_eval;

void sampler_cache_lookup(sampler_cache *sampler, sarp_data *policy_data, 
    memory_trace *info, ub1 update_time);

sampler_entry* sampler_cache_get_block(sampler_cache *sampler, memory_trace *info);

static ub1 is_ship_sample_set(long long int indx)
{
  ub4 upper;
  ub4 lower;
  ub4 alternate;

  upper     = (indx >> 5) & 0x1f;
  lower     = indx & 0x1f;
  alternate = (lower ^ 0xa) & 0x1f;

  if ((upper == alternate))
  { 
    /* These are the SHiP-PC training sets */
    return TRUE;
  }

  return FALSE;
}

static ub4 get_set_type_sarp(long long int indx, ub4 sarp_samples)
{
  ub4 upper;
  ub4 lower;
  ub4 alternate;
  ub4 middle;
  ub4 not_lower;
  ub4 alternate1;
  ub4 alternate2;
  ub4 set_type;
  
  set_type = SARP_FOLLOWER_SET;

  /* BS and PS will always be present */
  assert(sarp_samples >= 2 && sarp_samples <= 30);

  upper     = (indx >> 7) & 0x7;
  lower     = indx & 0x7;
  middle    = (indx >> 3) & 0xf;
  not_lower = (~lower) & 0x7;

  if ((upper == lower) && (middle == 0))
  {
    /* These are SRRIP sets */
    set_type = SARP_GPU_SRRIP_SAMPLE;
  }

  if ((upper == not_lower) && (middle == 0))
  {
    /* These are BRRIP sets */
    set_type = SARP_GPU_BRRIP_SAMPLE;
  }

  if ((upper == not_lower) && (middle == 0x1))
  {
      /* These are bypass samples */
    set_type = SARP_ZS_BYPASS_SAMPLE;
  }

  if ((upper == not_lower) && (middle == 0x2))
  {
    /* These are non-bypass samplee */
    set_type = SARP_ZS_NO_BYPASS_SAMPLE;
  }

  alternate   = (lower ^ 0x5) & 0x7;
  alternate1  = (lower ^ 0x6) & 0x7;
  alternate2  = (lower ^ 0x4) & 0x7;

  if ((upper == lower) && (middle == 0x3))
  {
    /* CORE1 never pin */
    set_type = SARP_CORE1_NOT_PIN_SAMPLE;
  }

  if ((upper == not_lower) && (middle == 0x3))
  {
    /* CORE1 pin if policy says so */
    set_type = SARP_CORE1_PIN_SAMPLE;
  }
  
  
  if (((upper == lower) && (middle == 0x1)))
  {
    /* CORE2 never pin */
    set_type = SARP_CORE2_NOT_PIN_SAMPLE;
  }

  if ((upper == lower) && (middle == 0x2))
  {
    /* CORE3 pin if policy says so */
    set_type = SARP_CORE2_PIN_SAMPLE;
  }
  
  if (((upper == alternate) && (middle == 0)))
  {
    /* CORE3 never pin */
    set_type = SARP_CORE3_NOT_PIN_SAMPLE;
  }

  if (((upper == alternate) && (middle == 0x3)))
  {
    /* CORE4 pin if policy says so */
    set_type = SARP_CORE3_PIN_SAMPLE;
  }
  
  if (((upper == alternate) && (middle == 0x1)))
  {
    /* CORE4 never pin */
    set_type = SARP_CORE4_NOT_PIN_SAMPLE;
  }

  if (((upper == alternate) && (middle == 0x2)))
  {
    /* CORE4 pin if policy says so */
    set_type = SARP_CORE4_PIN_SAMPLE;
  }
  
  if (((upper == alternate1) && (middle == 0)))
  {
    /* CS never pin */
    set_type = SARP_CS_NOT_PIN_SAMPLE;
  }

  if (((upper == alternate1) && (middle == 0x3)))
  {
    /* CS pin if policy says so */
    set_type = SARP_CS_PIN_SAMPLE;
  }
  
  if (((upper == alternate1) && (middle == 0x1)))
  {
    /* ZS never pin */
    set_type = SARP_ZS_NOT_PIN_SAMPLE;
  }

  if (((upper == alternate1) && (middle == 0x2)))
  {
    /* ZS pin if policy says so */
    set_type = SARP_ZS_PIN_SAMPLE;
  }
  
  if (((upper == alternate2) && (middle == 0)))
  {
    /* BS never pin */
    set_type = SARP_BS_NOT_PIN_SAMPLE;
  }

  if (((upper == alternate2) && (middle == 0x3)))
  {
    /* BS pin if policy says so */
    set_type = SARP_BS_PIN_SAMPLE;
  }
  
  if (((upper == alternate2) && (middle == 0x1)))
  {
    /* OS never pin */
    set_type = SARP_OS_NOT_PIN_SAMPLE;
  }

  if (((upper == alternate2) && (middle == 0x2)))
  {
    /* OS pin if policy says so */
    set_type = SARP_OS_PIN_SAMPLE;
  }
  

  if (((upper == not_lower) && (middle == 0x4)))
  {
    /* Only promote */
    set_type = SARP_SPILL_HIT_PROMOTE_SAMPLE;
  }

  if (((upper == not_lower) && (middle == 0x7)))
  {
    /* Promote and pin */
    set_type = SARP_SPILL_HIT_PIN_SAMPLE;
  }
  
  return set_type;
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
sarp_stream get_sarp_stream(sarp_gdata *global_data, memory_trace *info)
{
  sarp_stream ret_stream;

  if (global_data->speedup_enabled)
  {
    ret_stream = info->sap_stream;
  }
  else
  {
    ret_stream = sarp_stream_u;
  }

  return ret_stream;
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

#define SPEEDUP(n) (n == sarp_stream_x)

static void sarp_update_hint_count(sarp_gdata *global_data, memory_trace *info)
{
  assert(info->stream <= TST);
  
  if (SPEEDUP(get_sarp_stream(global_data, info)))
  {
    SAT_CTR_INC(global_data->sarp_hint[info->stream]);
  }
}

#undef SPEEDUP

#define MAX_RANK (3)

static void cache_update_interval_end(sarp_gdata *global_data)
{
  /* Obtain stream to be spedup */
  ub1 i;
  ub4 val;
  ub1 new_stream;
  ub1 old_stream;
  ub1 stream_rank[MAX_RANK];

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
    if (SAT_CTR_VAL(global_data->sarp_hint[i]) > val)
    {
      val = SAT_CTR_VAL(global_data->sarp_hint[i]);
      new_stream = i;
    }
  }

  for (i = 0; i < TST + 1; i++)
  {
    SAT_CTR_RST(global_data->sarp_hint[i]);
  }

  global_data->speedup_stream = new_stream;
  global_data->speedup_count[new_stream] += 1;
}

#undef MAX_RANK

void sampler_cache_reset(sarp_gdata *global_data, sampler_cache *sampler)
{
  int sampler_occupancy;

  assert(sampler);
  
  printf("SMPLR RESET : Replacments [%d] Fill [%ld] Hit [%ld]\n", sampler->perfctr.sampler_replace,
      sampler->perfctr.sampler_fill, sampler->perfctr.sampler_hit);

  sampler_occupancy = 0;
  
  printf("Bypass [%5d] MAX_REUSE[%5d]\n", global_data->bypass_count, 
      global_data->sampler->perfctr.max_reuse);

  printf("SPEEDUP C:%ld Z:%ld T:%ld\n", global_data->speedup_count[CS],
      global_data->speedup_count[ZS], global_data->speedup_count[TS]);

  printf("[C] F:%5ld FR: %5ld S: %5ld SR: %5ld\n", sampler->perfctr.fill_count[CS], 
      sampler->perfctr.fill_reuse_count[CS], sampler->perfctr.spill_count[CS],
      sampler->perfctr.spill_reuse_count[CS]);

  printf("[Z] F:%5ld R: %5ld S: %5ld SR: %5ld\n", sampler->perfctr.fill_count[ZS], 
      sampler->perfctr.fill_reuse_count[ZS], sampler->perfctr.spill_count[ZS],
      sampler->perfctr.spill_reuse_count[ZS]);

  printf("[B] F:%5ld R: %5ld S: %5ld SR: %5ld\n", sampler->perfctr.fill_count[BS], 
      sampler->perfctr.fill_reuse_count[BS], sampler->perfctr.spill_count[BS],
      sampler->perfctr.spill_reuse_count[BS]);

  printf("[P] F:%5ld R: %5ld S: %5ld SR: %5ld\n", sampler->perfctr.fill_count[PS], 
      sampler->perfctr.fill_reuse_count[PS], sampler->perfctr.spill_count[PS],
      sampler->perfctr.spill_reuse_count[PS]);

  printf("[T] F:%5ld R: %5ld\n", sampler->perfctr.fill_count[TS], 
      sampler->perfctr.fill_reuse_count[TS]);

  printf("[O] F:%5ld R: %5ld\n", sampler->perfctr.fill_count[OS], 
      sampler->perfctr.fill_reuse_count[OS]);

  printf("[CT] F:%5ld R: %5ld\n", sampler->perfctr.fill_count[DCS], 
      sampler->perfctr.fill_reuse_count[DCS]);

  printf("[C] FDLOW:%5ld FDHIGH: %5ld SDLOW:%5ld SDHIGH: %5ld\n", sampler->perfctr.fill_reuse_distance_low[CS], 
      sampler->perfctr.fill_reuse_distance_high[CS], sampler->perfctr.spill_reuse_distance_low[CS],
      sampler->perfctr.spill_reuse_distance_high[CS]);

  printf("[Z] FDLOW:%5ld FDHIGH: %5ld SDLOW:%5ld SDHIGH: %5ld\n", sampler->perfctr.fill_reuse_distance_low[ZS], 
      sampler->perfctr.fill_reuse_distance_high[ZS], sampler->perfctr.spill_reuse_distance_low[ZS],
      sampler->perfctr.spill_reuse_distance_high[ZS]);

  printf("[B] FDLOW:%5ld FDHIGH: %5ld SDLOW:%5ld SDHIGH: %5ld\n", sampler->perfctr.fill_reuse_distance_low[BS], 
      sampler->perfctr.fill_reuse_distance_high[BS], sampler->perfctr.spill_reuse_distance_low[BS],
      sampler->perfctr.spill_reuse_distance_high[BS]);

  printf("[P] FDLOW:%5ld FDHIGH: %5ld SDLOW:%5ld SDHIGH: %5ld\n", sampler->perfctr.fill_reuse_distance_low[PS], 
      sampler->perfctr.fill_reuse_distance_high[PS], sampler->perfctr.spill_reuse_distance_low[PS],
      sampler->perfctr.spill_reuse_distance_high[PS]);

  printf("[T] SDLOW:%5ld SDHIGH: %5ld\n", sampler->perfctr.fill_reuse_distance_low[TS], 
      sampler->perfctr.fill_reuse_distance_high[TS]);

  printf("[O] SDLOW:%5ld SDHIGH: %5ld\n", sampler->perfctr.fill_reuse_distance_low[OS], 
      sampler->perfctr.fill_reuse_distance_high[OS]);

  printf("[CT] FDLOW:%5ld FDHIGH: %5ld SDLOW:%5ld SDHIGH: %5ld\n", sampler->perfctr.fill_reuse_distance_low[DCS], 
      sampler->perfctr.fill_reuse_distance_high[DCS], sampler->perfctr.spill_reuse_distance_low[DCS],
      sampler->perfctr.spill_reuse_distance_high[DCS]);

  printf("[BT] FDLOW:%5ld FDHIGH: %5ld SDLOW:%5ld SDHIGH: %5ld\n", sampler->perfctr.fill_reuse_distance_low[DBS], 
      sampler->perfctr.fill_reuse_distance_high[DBS], sampler->perfctr.spill_reuse_distance_low[DBS],
      sampler->perfctr.spill_reuse_distance_high[DBS]);

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
        (sampler->blocks)[i][j].timestamp[off]      = 0;
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

    for (ub1 ep = 0; ep <= REPOCH_CNT; ep++)
    {
      sampler->perfctr.fill_count_per_reuse_epoch[strm][ep] /= 2;
      sampler->perfctr.fill_reuse_per_reuse_epoch[strm][ep] /= 2;
      sampler->perfctr.fill_reuse_distance_high_per_reuse_epoch[strm][ep] /= 2;
      sampler->perfctr.fill_reuse_distance_low_per_reuse_epoch[strm][ep]  /= 2;
    }
    
#define MAX_RRPV (3)
    
    for (ub1 rrpv = 0; rrpv < MAX_RRPV; rrpv++)
    {
      sampler->perfctr.stream_fill_rrpv[strm][rrpv] = 0;
      sampler->perfctr.stream_hit_rrpv[strm][rrpv]  = 0;
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
#undef FBYPASS_TH
#undef SBYPASS_TH

#define SCOUNT(p, s)    (SMPLRPERF_SPILL(p, s))
#define SRUSE(p, s)     (SMPLRPERF_SREUSE(p, s))
#define MRUSE(p)        (SMPLRPERF_MREUSE(p))
#define SHPIN_TH(p, s)  (SRUSE(p, s) * SHTH_DENM > SCOUNT(p, s) * SHTH_NUMR)
#define SMPIN_TH(p, s)  (SRUSE(p, s) * SMTH_DENM > SCOUNT(p, s) * SMTH_NUMR)
#define PINHBLK(p, s)   ((SRUSE(p, s) > MRUSE(p) / 2) || SHPIN_TH(p, s))
#define PINMBLK(p, s)   ((SRUSE(p, s) > MRUSE(p) / 2) || SMPIN_TH(p, s))

static ub1 is_hit_block_pinned(sarp_data *policy_data, sarp_gdata *global_data, 
    sampler_cache *sampler, memory_trace *info)
{
  ub1 strm;
  ub1 ret_val;

  strm    = NEW_STREAM(info);
  ret_val = FALSE;
  
  if (info->spill == TRUE) 
  {
    switch (policy_data->set_type)
    {
      case SARP_SPILL_HIT_PROMOTE_SAMPLE:
        ret_val = FALSE;
        break;

      case SARP_SPILL_HIT_PIN_SAMPLE:
        if (PINHBLK(&(sampler->perfctr), strm)) 
        {
          ret_val = TRUE;
        }
        break;

      default:
        assert(policy_data->set_type <= SARP_SAMPLE_COUNT);

        if (SAT_CTR_VAL(global_data->spill_hit_psel) <= PSEL_MID_VAL)
        {
          if (PINHBLK(&(sampler->perfctr), strm)) 
          {
            ret_val = TRUE;
          }
        }
    }
  }

  return ret_val;
}

static ub1 is_miss_block_pinned(sampler_cache *sampler, memory_trace *info)
{
  ub1 strm;

  strm = NEW_STREAM(info);
  
  if (info->spill == TRUE) 
  {
    if (PINMBLK(&(sampler->perfctr), strm)) 
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

static ub1 is_block_unpinned(sarp_data *policy_data, sarp_gdata *global_data, 
    memory_trace *info)
{
  ub1 ret_val;

  ret_val = FALSE;

  if (CPU_STREAM(info->stream))
  {
    switch (info->core) 
    {
      case 1:
        if (policy_data->set_type == SARP_CORE1_NOT_PIN_SAMPLE || 
            (policy_data->set_type != SARP_CORE1_PIN_SAMPLE && 
             SAT_CTR_VAL(global_data->core1_pin_psel) <= PSEL_MID_VAL))
        {
          ret_val = TRUE; 
        }

        break;

      case 2:
        if (policy_data->set_type == SARP_CORE2_NOT_PIN_SAMPLE || 
            (policy_data->set_type != SARP_CORE2_PIN_SAMPLE && 
             SAT_CTR_VAL(global_data->core2_pin_psel) <= PSEL_MID_VAL))
        {
          ret_val = TRUE; 
        }
        break;

      case 3:
        if (policy_data->set_type == SARP_CORE3_NOT_PIN_SAMPLE || 
            (policy_data->set_type != SARP_CORE3_PIN_SAMPLE && 
             SAT_CTR_VAL(global_data->core3_pin_psel) <= PSEL_MID_VAL))
        {
          ret_val = TRUE; 
        }
        break;

      case 4:
        if (policy_data->set_type == SARP_CORE4_NOT_PIN_SAMPLE || 
            (policy_data->set_type != SARP_CORE4_PIN_SAMPLE && 
             SAT_CTR_VAL(global_data->core4_pin_psel) <= PSEL_MID_VAL))
        {
          ret_val = TRUE; 
        }
        break;
    }
  }
  else
  {
    switch (NEW_STREAM(info))
    {
      case CS:
        if (policy_data->set_type == SARP_CS_NOT_PIN_SAMPLE || 
            (policy_data->set_type != SARP_CS_PIN_SAMPLE && 
             SAT_CTR_VAL(global_data->cs_pin_psel) <= PSEL_MID_VAL))
        {
          ret_val = TRUE; 
        }
        break;

      case ZS:
        if (policy_data->set_type == SARP_ZS_NOT_PIN_SAMPLE || 
            (policy_data->set_type != SARP_ZS_PIN_SAMPLE && 
             SAT_CTR_VAL(global_data->cs_pin_psel) <= PSEL_MID_VAL))
        {
          ret_val = TRUE; 
        }
        break;

      case BS:
        if (policy_data->set_type == SARP_BS_NOT_PIN_SAMPLE || 
            (policy_data->set_type != SARP_BS_PIN_SAMPLE && 
             SAT_CTR_VAL(global_data->cs_pin_psel) <= PSEL_MID_VAL))
        {
          ret_val = TRUE; 
        }
        break;

      case OS:
        if (policy_data->set_type == SARP_OS_NOT_PIN_SAMPLE || 
            (policy_data->set_type != SARP_OS_PIN_SAMPLE && 
             SAT_CTR_VAL(global_data->cs_pin_psel) <= PSEL_MID_VAL))
        {
          ret_val = TRUE; 
        }
        break;
    }
  }

  return ret_val;
}


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
    memory_trace *info, struct cache_block_t *block)
{
  sampler_perfctr *perfctr;
  int ret_rrpv;
  ub1 strm;

  assert(policy_data);
  assert(global_data);
  assert(info);

  /* Ensure current policy is sarp */
  assert(info->spill == TRUE || (info->fill == TRUE && CPU_STREAM(info->stream)));

  strm      = NEW_STREAM(info);
  perfctr   = &((global_data->sampler)->perfctr);
  ret_rrpv  = 2;


  if (GPU_STREAM(info->stream))
  {
#define CHK_SCRTCL(g, i) ((g)->speedup_enabled ? CRITICAL_STREAM(g, i) : TRUE)
    if (CHK_SCRTCL(global_data, info))
    {
      if (RRPV1(perfctr, strm) || RRPV2(perfctr, strm))
      {
        ret_rrpv = 0;
        global_data->stream_pfill[info->stream] += 1;
      }
      else
      {
        if (SRD_LOW(perfctr, strm) || SREUSE_LOW(perfctr, strm))
        {
          ret_rrpv = 3;
        }
      }
    }
    else
    {

      assert(global_data->speedup_enabled);

#define CSBYTH_D            (2)
#define CSBYTH_N            (1)
#define CFBYTH_D            (2)
#define CFBYTH_N            (1)
#define FCOUNT(p, s)        (SMPLRPERF_FILL(p, s))
#define FREUSE(p, s)        (SMPLRPERF_FREUSE(p, s))
#define FUSE_TH(sp, s)      (FREUSE(&((sp)->perfctr), s) * CFBYTH_D <= FCOUNT(&((sp)->perfctr), s) * CFBYTH_N)
#define SUSE_TH(sp, s)      (SREUSE(&((sp)->perfctr), s) * CSBYTH_D <= SCOUNT(&((sp)->perfctr), s) * CSBYTH_N)
#define CLOWUSE(sp, i)      ((i)->fill ? FUSE_TH(sp, (i)->stream) : SUSE_TH(sp, (i)->stream))
#define NCRTCL_LOWUSE(g, i) (!CRITICAL_STREAM(g, i) && CLOWUSE((g)->sampler, i))

      if (NCRTCL_LOWUSE(global_data, info))
      {
        ret_rrpv = 3;
        global_data->stream_fdemote[info->stream] += 1;
      }

#undef CSBYTH_D
#undef CSBYTH_N
#undef CFBYTH_D
#undef CFBYTH_N
#undef FCOUNT
#undef FREUSE
#undef FUSE_TH
#undef SUSE_TH
#undef CLOWUSE
#undef NCRTCL_LOWUSE
    }
  }
  else
  {
    assert(block->ship_sign_valid == TRUE);

#define CTR_VAL(g)    (SAT_CTR_VAL((g)->baccess[SARP_SHIP_SAMPLE]))
#define THRESHOLD(g)  ((g)->threshold)
    
    if (global_data->cpu_fill_enable)
    {
      /* Use ship for CPU fill */
      if (CTR_VAL(global_data) == THRESHOLD(global_data) - 1)
      {
        ret_rrpv = 2;
        SAT_CTR_RST(global_data->baccess[SARP_SHIP_SAMPLE]);
      }
      else
      {
        if (global_data->ship_shct[block->ship_sign] == 0)
        {
          ret_rrpv = 3;
        }
        else
        {
          ret_rrpv = 2;
        }
      }

      SAT_CTR_INC(global_data->baccess[SARP_SHIP_SAMPLE]);
    }
    else
    {
      ret_rrpv = 2;
    }
#undef CTR_VAL
#undef THRESHOLD
  }

  return ret_rrpv;
}

#undef CHK_SCRTCL
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

#if 0
#define CSBYTH_D                (2)
#endif

#define CSBYTH_D                (2)
#define CSBYTH_N                (1)
#define CFBYTH_D                (2)
#define CFBYTH_N                (1)
#define FBYPASS_TH(sp, s, e)    (FRUSE(&((sp)->perfctr), s, e) * CFBYTH_D <= FCOUNT(&((sp)->perfctr), s, e) * CFBYTH_N)
#define SBYPASS_TH(sp, s)       (SRUSE(&((sp)->perfctr), s) * CSBYTH_D <= SCOUNT(&((sp)->perfctr), s) * CSBYTH_N)
#define CBYPASS(sp, i, e)       ((i)->fill ? FBYPASS_TH(sp, (i)->stream, e) : SBYPASS_TH(sp, (i)->stream))
#define NCRTCL_BYPASS(g, i, e)  (!CRITICAL_STREAM(g, i) && CBYPASS((g)->sampler, i, e))
#define CHK_SCRTCL(g, i)        ((g)->speedup_enabled ? CRITICAL_STREAM(g, i) : TRUE)
#define CHK_SSPD(g, i, e)       ((g)->speedup_enabled ? !NCRTCL_BYPASS(g, i, e) : TRUE)
#define CHK_FSPD(g, i, e)       ((g)->speedup_enabled ? NCRTCL_BYPASS(g, i, e) : FALSE)

int cache_get_new_rrpv_sarp(sarp_data *policy_data, sarp_gdata *global_data, 
    memory_trace *info, struct cache_block_t *block, int old_rrpv)
{
  int ret_rrpv;
  int epoch;
  int strm;

  sampler_perfctr *perfctr;
  
  perfctr = &((global_data->sampler)->perfctr);
  epoch   = block->epoch;
  strm    = NEW_STREAM(info);

  ret_rrpv  = CHK_SCRTCL(global_data, info) ? 0 : old_rrpv;

  if (info->spill == TRUE)
  {
    switch (policy_data->set_type)
    {
      case SARP_SPILL_HIT_PROMOTE_SAMPLE:
        if (RRPV1(perfctr, strm) && old_rrpv >= 3)
        {
          ret_rrpv = 2;
        }
        else
        {
          ret_rrpv = old_rrpv;
        }
        break;

      case SARP_SPILL_HIT_PIN_SAMPLE:
        if (RRPV1(perfctr, strm) && old_rrpv >= 3)
        {
          ret_rrpv = 0;
        }
        else
        {
          ret_rrpv = old_rrpv;
        }
        break;

      default:
        assert(policy_data->set_type <= SARP_SAMPLE_COUNT);

        if (SAT_CTR_VAL(global_data->spill_hit_psel) <= PSEL_MID_VAL)
        {
          if (RRPV1(perfctr, strm) && old_rrpv >= 3)
          {
            ret_rrpv = 0;
          }
          else
          {
            ret_rrpv = old_rrpv;
          }
        }
        else
        {
          if (RRPV1(perfctr, strm) && old_rrpv >= 3)
          {
            ret_rrpv = 2;
          }
          else
          {
            ret_rrpv = old_rrpv;
          }
        }
    }
  }
  else
  {
    assert(info->fill == TRUE);

    if (block->is_ct_block)
    {
      if (FRUSE(perfctr, DCS, epoch) * CT_TH1 <= FCOUNT(perfctr, DCS, epoch))
      {
        ret_rrpv = 3;
      }
      else
      {
        if (FRUSE(perfctr, DCS, epoch) * TH2 <= FCOUNT(perfctr, DCS, epoch))
        {
          ret_rrpv = 2;
        }
      }
    }
    else
    {
      if (block->is_bt_block)
      {
        if (FRUSE(perfctr, DBS, epoch) * CT_TH1 <= FCOUNT(perfctr, DBS, epoch))
        {
          ret_rrpv = 3;
        }
        else
        {
          if (FRUSE(perfctr, DBS, epoch) * TH2 <= FCOUNT(perfctr, DBS, epoch))
          {
            ret_rrpv = 2;
          }
        }
      }
      else
      {
        if (global_data->speedup_enabled && 
            FRUSE(perfctr, strm, epoch) * CT_TH1 <= FCOUNT(perfctr, strm, epoch))
        {
          global_data->stream_hdemote[info->stream] += 1;
          ret_rrpv = 3;
        }
      }
    }
  }

  return ret_rrpv;
}

#undef CSBYTH_D
#undef CSBYTH_N
#undef CFBYTH_D
#undef CFBYTH_N
#undef FBYPASS_TH
#undef SBYPASS_TH
#undef CBYPASS
#undef NCRTCL_BYPASS
#undef SCOUNT
#undef SRUSE
#undef FCOUNT
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
#undef TH
#undef TH2
#undef CHK_SCRTCL
#undef CHK_SSPD
#undef CHK_FSPD

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

  policy_data->is_ship_sampler = is_ship_sample_set(set_indx);

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

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->gpu_srrip_brrip_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->gpu_srrip_brrip_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->zs_bypass_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->zs_bypass_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->core1_pin_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->core1_pin_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->core2_pin_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->core2_pin_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->core3_pin_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->core3_pin_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->core4_pin_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->core4_pin_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->cs_pin_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->cs_pin_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->zs_pin_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->zs_pin_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->bs_pin_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->bs_pin_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->os_pin_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->os_pin_psel, PSEL_MID_VAL);

    /* Initialize SAP policy selection counter */
    SAT_CTR_INI(global_data->spill_hit_psel, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->spill_hit_psel, PSEL_MID_VAL);

    /* Initialize per-stream policy selection counter */
    for (ub1 i = 0; i <= TST; i++)
    {
      SAT_CTR_INI(global_data->sarp_ssel[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_SET(global_data->sarp_ssel[i], PSEL_MID_VAL);

      SAT_CTR_INI(global_data->sarp_hint[i], ECTR_WIDTH, ECTR_MIN_VAL, ECTR_MAX_VAL);
      SAT_CTR_SET(global_data->sarp_hint[i], ECTR_MIN_VAL);
    }

    /* Initialize SARP specific statistics */
    cache_init_sarp_stats(&(global_data->stats), NULL);

    /* Initialize cache-wide data for SARP. */
    global_data->threshold        = params->threshold;
    global_data->sarp_streams     = params->sdp_streams;
    global_data->pin_blocks       = params->sarp_pin_blocks;
    global_data->ways             = params->ways;
    global_data->speedup_enabled  = params->speedup_enabled;
    global_data->speedup_stream   = NN;
    global_data->speedup_interval = 0;
    global_data->sign_size        = params->ship_sig_size;
    global_data->shct_size        = params->ship_shct_size;
    global_data->core_size        = params->ship_core_size;
    global_data->cpu_fill_enable  = params->sarp_cpu_fill_enable;
    global_data->gpu_fill_enable  = params->sarp_gpu_fill_enable;
    global_data->ship_access      = 0;
    global_data->ship_access      = 0;

    memset(global_data->fmiss_count, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->smiss_count, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->stream_pinned, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->stream_bypass, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->stream_fdemote, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->stream_hdemote, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->stream_pfill, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->stream_phit, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->stream_access, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->stream_miss, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->speedup_count, 0, sizeof(ub8) * (TST + 1));

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

    global_data->ship_shct = (ub1 *) xcalloc((1 << global_data->shct_size), sizeof(ub1));
    assert(global_data->ship_shct);

#define SHCT_ENTRY_SIZE (3)
#define CTR_MIN_VAL     (0)  
#define CTR_MAX_VAL     ((1 << SHCT_ENTRY_SIZE) - 1)

    /* Initialize counter table */ 
    for (ub4 i = 0; i < (1 << global_data->shct_size); i++)
    {
      global_data->ship_shct[i] = 0;
    }

#undef SHCT_ENTRY_SIZE
#undef CTR_MIN_VAL
#undef CTR_MAX_VAL
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
    switch (set_type)
    {
      case SARP_GPU_SRRIP_SAMPLE:
        if (GPU_STREAM(stream))
        {
          SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_srrip; 
        }
        else
        {
          SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_sarp; 
        }
        break;

      case SARP_GPU_BRRIP_SAMPLE:
        if (GPU_STREAM(stream))
        {
          SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_brrip; 
        }
        else
        {
          SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_sarp; 
        }
        break;

      case SARP_ZS_BYPASS_SAMPLE:
      case SARP_ZS_NO_BYPASS_SAMPLE:
      case SARP_CORE1_NOT_PIN_SAMPLE:
      case SARP_CORE1_PIN_SAMPLE:
      case SARP_CORE2_NOT_PIN_SAMPLE:
      case SARP_CORE2_PIN_SAMPLE:
      case SARP_CORE3_NOT_PIN_SAMPLE:
      case SARP_CORE3_PIN_SAMPLE:
      case SARP_CORE4_NOT_PIN_SAMPLE:
      case SARP_CORE4_PIN_SAMPLE:
      case SARP_CS_NOT_PIN_SAMPLE:
      case SARP_CS_PIN_SAMPLE:
      case SARP_ZS_NOT_PIN_SAMPLE:
      case SARP_ZS_PIN_SAMPLE:
      case SARP_BS_NOT_PIN_SAMPLE:
      case SARP_BS_PIN_SAMPLE:
      case SARP_OS_NOT_PIN_SAMPLE:
      case SARP_OS_PIN_SAMPLE:
      case SARP_SPILL_HIT_PROMOTE_SAMPLE:
      case SARP_SPILL_HIT_PIN_SAMPLE:
      case SARP_FOLLOWER_SET:
        SARP_DATA_PSPOLICY(policy_data)[stream] = cache_policy_sarp; 
        break;

      default:
        panic("%s: line no %d - invalid sample type", __FUNCTION__, __LINE__);
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
    printf("SHIP ACCESS %ld SHIP DEC %ld SHIP INC %ld\n", global_data->ship_access,
        global_data->ship_dec, global_data->ship_inc);

    sampler_cache_reset(global_data, global_data->sampler);
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
  
  if (policy_data->is_ship_sampler)
  {
    global_data->ship_access += 1;
  }

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
    info->sap_stream = get_sarp_stream(global_data, info);
    /* Update sampler epoch length */
  }

  if (!node)
  {
    enum cache_policy_t *per_stream_policy;
    enum cache_policy_t  following_policy;

    /* Get per-stream policy */
    per_stream_policy = SARP_DATA_PSPOLICY(policy_data);
    assert(per_stream_policy);

    following_policy  = FOLLOW_POLICY(per_stream_policy[info->stream], info);

    /* Update sampler set counter only for reads */
    if (info->fill == TRUE)
    {
      switch (policy_data->set_type)
      {
        case SARP_GPU_SRRIP_SAMPLE:
          SAT_CTR_INC(global_data->gpu_srrip_brrip_psel);
          break;

        case SARP_GPU_BRRIP_SAMPLE:
          SAT_CTR_DEC(global_data->gpu_srrip_brrip_psel);

          /* Update brrip epsilon counter only for graphics */
#define CTR_VAL(g)    (SAT_CTR_VAL((g)->baccess[SARP_GPU_BRRIP_SAMPLE]))
#define THRESHOLD(g)  ((g)->threshold)

          /* Update access counter for bimodal insertion */
          if (CTR_VAL(global_data) < THRESHOLD(global_data))
          {
            /* Increment set access count */
            SAT_CTR_INC(global_data->baccess[SARP_GPU_BRRIP_SAMPLE]);
          }

          if (CTR_VAL(global_data) >= THRESHOLD(global_data))
          {
            assert(CTR_VAL(global_data) == THRESHOLD(global_data));

            /* Reset access count */
            SAT_CTR_RST(global_data->baccess[SARP_GPU_BRRIP_SAMPLE]);
          }


#undef CTR_VAL
#undef THRESHOLD
          break;

        case SARP_ZS_BYPASS_SAMPLE:
          SAT_CTR_INC(global_data->zs_bypass_psel);
          break;

        case SARP_ZS_NO_BYPASS_SAMPLE:
          SAT_CTR_DEC(global_data->zs_bypass_psel);
          break;

        case SARP_CORE1_NOT_PIN_SAMPLE:
          SAT_CTR_INC(global_data->core1_pin_psel);
          break;

        case SARP_CORE1_PIN_SAMPLE:
          SAT_CTR_DEC(global_data->core1_pin_psel);
          break;

        case SARP_CORE2_NOT_PIN_SAMPLE:
          SAT_CTR_INC(global_data->core2_pin_psel);
          break;

        case SARP_CORE2_PIN_SAMPLE:
          SAT_CTR_DEC(global_data->core2_pin_psel);
          break;

        case SARP_CORE3_NOT_PIN_SAMPLE:
          SAT_CTR_INC(global_data->core3_pin_psel);
          break;

        case SARP_CORE3_PIN_SAMPLE:
          SAT_CTR_DEC(global_data->core3_pin_psel);
          break;

        case SARP_CORE4_NOT_PIN_SAMPLE:
          SAT_CTR_INC(global_data->core4_pin_psel);
          break;

        case SARP_CORE4_PIN_SAMPLE:
          SAT_CTR_DEC(global_data->core4_pin_psel);
          break;

        case SARP_CS_NOT_PIN_SAMPLE:
          SAT_CTR_INC(global_data->cs_pin_psel);
          break;

        case SARP_CS_PIN_SAMPLE:
          SAT_CTR_DEC(global_data->cs_pin_psel);
          break;

        case SARP_ZS_NOT_PIN_SAMPLE:
          SAT_CTR_INC(global_data->zs_pin_psel);
          break;

        case SARP_ZS_PIN_SAMPLE:
          SAT_CTR_DEC(global_data->zs_pin_psel);
          break;

        case SARP_BS_NOT_PIN_SAMPLE:
          SAT_CTR_INC(global_data->bs_pin_psel);
          break;

        case SARP_BS_PIN_SAMPLE:
          SAT_CTR_DEC(global_data->bs_pin_psel);
          break;

        case SARP_OS_NOT_PIN_SAMPLE:
          SAT_CTR_INC(global_data->os_pin_psel);
          break;

        case SARP_OS_PIN_SAMPLE:
          SAT_CTR_DEC(global_data->os_pin_psel);
          break;

        case SARP_SPILL_HIT_PROMOTE_SAMPLE:
          SAT_CTR_DEC(global_data->spill_hit_psel);
          break;

        case SARP_SPILL_HIT_PIN_SAMPLE:
          SAT_CTR_INC(global_data->spill_hit_psel);
          break;

        case SARP_FOLLOWER_SET:
          break;

        default:
          panic("%s: line no %d - invalid sample type", __FUNCTION__, __LINE__);
      }

      /* Update brrip epsilon counter only for graphics */
#define CTR_VAL(g)    (SAT_CTR_VAL((g)->baccess[SARP_FOLLOWER_GPU_SET]))
#define THRESHOLD(g)  ((g)->threshold)

      if (policy_data->set_type != SARP_GPU_SRRIP_SAMPLE && 
          policy_data->set_type != SARP_GPU_BRRIP_SAMPLE && 
          GPU_STREAM(info->stream))
      {
        /* Update access counter for bimodal insertion */
        if (CTR_VAL(global_data) < THRESHOLD(global_data))
        {
          /* Increment set access count */
          SAT_CTR_INC(global_data->baccess[SARP_FOLLOWER_GPU_SET]);
        }

        if (CTR_VAL(global_data) >= THRESHOLD(global_data))
        {
          assert(CTR_VAL(global_data) == THRESHOLD(global_data));

          /* Reset access count */
          SAT_CTR_RST(global_data->baccess[SARP_FOLLOWER_GPU_SET]);
        }
      }

#undef CTR_VAL
#undef THRESHOLD

    }
  }

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
  
  /* Increment miss count for incoming stream */
  if (info->spill)
  {
    global_data->smiss_count[info->stream] += 1;
  }
  else
  {
    global_data->fmiss_count[info->stream] += 1;
  }

  /* Obtain SAP stream */
  sstream = get_sarp_stream(global_data, info); 

  info->sap_stream = sstream;
  
  /* Get per-stream policy */
  per_stream_policy = SARP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);

  following_policy  = FOLLOW_POLICY(per_stream_policy[info->stream], info);

  assert(FSRRIP(following_policy) || FBRRIP(following_policy) || FSARP(following_policy));

  /* Update sampler set counter only for reads */
  if (info->fill == TRUE)
  {
    switch (policy_data->set_type)
    {
      case SARP_GPU_BRRIP_SAMPLE:

        /* Update brrip epsilon counter only for graphics */
#define CTR_VAL(g)    (SAT_CTR_VAL((g)->baccess[SARP_GPU_BRRIP_SAMPLE]))

        /* Set new value to brrip policy global data */
        SAT_CTR_VAL(global_data->brrip.access_ctr) = CTR_VAL(global_data);

#undef CTR_VAL
        break;

      default:
#define CTR_VAL(g)    (SAT_CTR_VAL((g)->baccess[SARP_FOLLOWER_GPU_SET]))

        /* Set new value to brrip policy global data */
        SAT_CTR_VAL(global_data->brrip.access_ctr) = CTR_VAL(global_data);

#undef CTR_VAL
        break;
    }

  }

  /* 
   * Update SARP counter
   *
   *  The counter is used to decide whether to follow DRRIP or 
   *  modified policy. The counter is incremented whenever there is
   *  a miss in the sample to be followed by DRRIP and it is decremented 
   *  everytime there is a miss in the policy sample.
   *
   **/

  /* Get the policy based on policy selection counters */
  current_policy = CURRENT_POLICY(policy_data, global_data, following_policy, info, FALSE);

  if (way != BYPASS_WAY)
  {
    policy_data->miss_count += 1;

    switch (current_policy)
    {
      case cache_policy_srrip:
        assert(way != BYPASS_WAY);
        assert(info->fill);

        cache_fill_block_srrip(&(policy_data->srrip), 
            &(global_data->srrip), way, tag, state, stream, info);

        /* Obtain block to update SARP stream, this is done to collect SARP 
         * specific stats on eviction */
        block = &(SRRIP_DATA_BLOCKS(policy_data)[way]);
        assert(block); 
        assert(block->stream == info->stream);
        assert(block->state != cache_block_invalid);

        if (CPU_STREAM(info->stream))
        {
          block->ship_sign        = SHIPSIGN(global_data, info);
          block->ship_sign_valid  = TRUE;
        }
        else
        {
          block->ship_sign_valid = FALSE;
        }
        break;

      case cache_policy_brrip:
        assert(way != BYPASS_WAY);
        assert(info->fill);
          
        cache_fill_block_brrip(&(policy_data->brrip), &(global_data->brrip), 
            way, tag, state, stream, info);

        /* Obtain block to update SARP stream, this is done to collect SARP 
         * specific stats on eviction */
        block = &(SRRIP_DATA_BLOCKS(policy_data)[way]);
        assert(block); 
        assert(block->stream == info->stream);
        assert(block->state != cache_block_invalid);

        if (CPU_STREAM(info->stream))
        {
          block->ship_sign        = SHIPSIGN(global_data, info);
          block->ship_sign_valid  = TRUE;
        }
        else
        {
          block->ship_sign_valid = FALSE;
        }
        break;

      case cache_policy_sarp:
        if (way != BYPASS_WAY)
        {
          int rrpv;

          /* Obtain SARP specific data */
          block = &(SARP_DATA_BLOCKS(policy_data)[way]);

          assert(block->stream == NN);

          if (CPU_STREAM(info->stream))
          {
            block->ship_sign        = SHIPSIGN(global_data, info);
            block->ship_sign_valid  = TRUE;
          }
          else
          {
            block->ship_sign_valid = FALSE;
          }

          /* Get RRPV to be assigned to the new block */
          rrpv = cache_get_fill_rrpv_sarp(policy_data, global_data, info, block);
          
          /* Update stream fill rrpv counter */
          SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), info->stream)[rrpv] += 1;

          /* Ensure a valid RRPV */
          assert(rrpv >= 0 && rrpv <= policy_data->max_rrpv); 

          /* Remove block from free list */
          free_list_remove_block(policy_data, block);

          /* Update new block state and stream */
          CACHE_UPDATE_BLOCK_STATE(block, tag, state);
          CACHE_UPDATE_BLOCK_STREAM(block, info->stream);

          block->dirty           = (info && info->spill) ? TRUE : FALSE;
          block->spill           = (info && info->spill) ? TRUE : FALSE;
          block->last_rrpv       = rrpv;
          block->access          = 0;
          block->epoch           = 0;
          block->is_ct_block     = 0;
          block->is_zt_block     = 0;
          block->is_bt_block     = 0;
          block->is_proc_block   = 0;
          block->is_block_pinned = 0;

          if (info->spill == TRUE)
          {
#define CSBYTH_D            (2)
#define CSBYTH_N            (1)
#define CSRD_D              (2)
#define CSRD_N              (1)
#define SCOUNT(p, s)        (SMPLRPERF_SPILL(p, s))
#define SREUSE(p, s)        (SMPLRPERF_SREUSE(p, s))
#define SBYPASS_TH(sp, s)   (SREUSE(&((sp)->perfctr), s) * CSBYTH_D <= SCOUNT(&((sp)->perfctr), s) * CSBYTH_N)
#define SDLOW(p, s)         (SMPLRPERF_SDLOW(p, s))
#define SDHIGH(p, s)        (SMPLRPERF_SDHIGH(p, s))
#define SRD_LOW(p, s)       (SDLOW(p, s) * CSRD_D < SDHIGH(p, s) * CSRD_N)
#define CBYPASS(sp, i)      (SBYPASS_TH(sp, (i)->stream))
#define NCRTCL_BYPASS(g, i) (!CRITICAL_STREAM(g, i) && CBYPASS((g)->sampler, i))
#define EVAL_PCOND(g, i)    ((g)->pin_blocks && is_miss_block_pinned((g)->sampler, i))
#define EVAL_SCOND(g, i)    (!NCRTCL_BYPASS(g, i) && EVAL_PCOND(g, i))
#define PIN_MISS_BLK(g, i)  (((g)->speedup_enabled) ? EVAL_SCOND(g, i) : EVAL_PCOND(g, i))

            if (PIN_MISS_BLK(global_data, info))
            {
              block->is_block_pinned = TRUE;

              global_data->stream_pinned[info->stream] += 1;
            }
            
#undef CSBYTH_D
#undef CSBYTH_N
#undef SCOUNT
#undef SREUSE
#undef SBYPASS_TH
#undef SDLOW
#undef SDHIGH
#undef SRD_LOW
#undef CBYPASS
#undef NCRTCL_BYPASS
#undef EVAL_PCOND
#undef EVAL_SCOND
#undef PIN_MISS_BLK

            if (is_block_unpinned(policy_data, global_data, info))
            {
              block->is_block_pinned = FALSE;
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
  
  sampler_cache_lookup(global_data->sampler, policy_data, info, TRUE);

  if (info)
  {
    global_data->stream_access[info->stream] += 1;
    global_data->stream_miss[info->stream] += 1;

    sarp_update_hint_count(global_data, info);
      
    if (++(global_data->speedup_interval) == INTERVAL_SIZE)
    {
      cache_update_interval_end(global_data);
      global_data->speedup_interval = 0;
    }

    if (info->fill)
    {
      if (++((global_data->sampler)->epoch_length) == EPOCH_SIZE)
      {

        printf("Fill RRPV 0: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), CS)[0], 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), ZS)[0],
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), TS)[0]);

        printf("Fill RRPV 2: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), CS)[2], 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), ZS)[2],
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), TS)[2]);

        printf("Fill RRPV 3: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), CS)[3], 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), ZS)[3],
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), TS)[3]);

        printf("Hit RRPV 0: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), CS)[0], 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), ZS)[0],
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), TS)[0]);

        printf("Hit RRPV 2: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), CS)[2], 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), ZS)[2],
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), TS)[2]);

        printf("Hit RRPV 3: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), CS)[3], 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), ZS)[3],
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), TS)[3]);

        printf("ZS bypass %ld\n", global_data->stream_bypass[ZS]);
        printf("CS bypass %ld\n", global_data->stream_bypass[CS]);
        printf("TS bypass %ld\n", global_data->stream_bypass[TS]);
        printf("IS bypass %ld\n", global_data->stream_bypass[IS]);

        printf("ZS fill demote %ld\n", global_data->stream_fdemote[ZS]);
        printf("CS fill demote %ld\n", global_data->stream_fdemote[CS]);
        printf("TS fill demote %ld\n", global_data->stream_fdemote[TS]);
        printf("IS fill demote %ld\n", global_data->stream_fdemote[IS]);

        printf("ZS hit demote %ld\n", global_data->stream_hdemote[ZS]);
        printf("CS hit demote %ld\n", global_data->stream_hdemote[CS]);
        printf("TS hit demote %ld\n", global_data->stream_hdemote[TS]);
        printf("IS hit demote %ld\n", global_data->stream_hdemote[IS]);

        /* Reset sampler cache and epoch length */
        sampler_cache_reset(global_data, global_data->sampler);
        (global_data->sampler)->epoch_length = 0;

        printf("\nPriority hints : speedup stream %d\n", global_data->speedup_stream);

        printf("CSAccess:%ld CSMiss:%ld\n", global_data->stream_access[CS],
            global_data->stream_miss[CS]);

        printf("ZSAccess:%ld ZSMiss:%ld\n", global_data->stream_access[ZS],
            global_data->stream_miss[ZS]);

        printf("TSAccess:%ld TSMiss:%ld\n", global_data->stream_access[TS],
            global_data->stream_miss[TS]);

        printf("CSPin:%ld CSPFill:%ld CSPhit:%ld\n", 
            global_data->stream_pinned[CS], global_data->stream_pfill[CS],
            global_data->stream_phit[CS]);

        printf("ZSPin:%ld ZSPFill:%ld ZSPhit:%ld\n", 
            global_data->stream_pinned[ZS], global_data->stream_pfill[ZS],
            global_data->stream_phit[ZS]);

        printf("TSPin:%ld TSPFill:%ld TSPhit:%ld\n", 
            global_data->stream_pinned[TS], global_data->stream_pfill[TS],
            global_data->stream_phit[TS]);

        for (ub1 strm = NN; strm <= TST; strm++)
        {
          global_data->stream_pfill[strm]   = 0;
          global_data->stream_phit[strm]    = 0;
        }

        memset(global_data->stream_pinned , 0, sizeof(ub8) * (TST + 1));
        memset(global_data->stream_access , 0, sizeof(ub8) * (TST + 1));
        memset(global_data->stream_miss , 0, sizeof(ub8) * (TST + 1));
      }
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
  int     old_rrpv;
  struct  cache_block_t *block;

  /* Ensure vaid arguments */
  assert(global_data);
  assert(policy_data);
  
  ret_wayid = BYPASS_WAY;
  min_rrpv  = !SARP_DATA_MAX_RRPV(policy_data);

  if ((info->stream != CS && info->stream != ZS && info->stream != BS) && info->spill == TRUE)
  {
    goto end;
  }
  
  /* Bypass depth spills */
  if (info->spill && info->stream == ZS)
  {
#define Z_BYPASS(p)     ((p)->set_type == SARP_ZS_BYPASS_SAMPLE)
#define Z_NO_BYPASS(p)  ((p)->set_type == SARP_ZS_NO_BYPASS_SAMPLE)
#define F_BYPASS(g)     (SAT_CTR_VAL((g)->zs_bypass_psel) <= PSEL_MID_VAL)

    if (Z_BYPASS(policy_data) || (!Z_NO_BYPASS(policy_data) && F_BYPASS(global_data)))
    {
      goto end;
    }

#undef Z_BYPASS
#undef Z_NO_BYPASS
#undef F_BYPASS
  }

  per_stream_policy = SARP_DATA_PSPOLICY(policy_data);
  assert(per_stream_policy);
   
  /* Get per stream policy map */
  following_policy = FOLLOW_POLICY(per_stream_policy[info->stream], info);

  assert(FSRRIP(following_policy) || FBRRIP(following_policy) || 
      FSARP(following_policy));

  /* Get the current policy based on policy selection counter */
  current_policy = CURRENT_POLICY(policy_data, global_data, following_policy, 
      info, FALSE);

#define IS_CRITICAL_STREAM(g, i)  ((g)->speedup_enabled ? CRITICAL_STREAM(g, i) : TRUE)

    switch (current_policy)
    {
      case cache_policy_srrip:
      case cache_policy_brrip:
      case cache_policy_sarp:
        /* Try to find an invalid block always from head of the free list. */
        for (block = SARP_DATA_FREE_HEAD(policy_data); block; block = block->prev)
        {
          return block->way;
        }

        /* If fill is not bypassed, get the replacement candidate */
        struct  cache_block_t *head_block;

        /* Obtain RRPV from where to replace the block */
        rrpv = cache_get_replacement_rrpv_sarp(policy_data);

        /* Ensure rrpv is with in max_rrpv bound */
        assert(rrpv >= 0 && rrpv <= SARP_DATA_MAX_RRPV(policy_data));

        head_block = SARP_DATA_VALID_HEAD(policy_data)[rrpv].head;

        /* If there is no block with required RRPV, increment RRPV of all the blocks
         * until we get one with the required RRPV */
        do
        {
          struct  cache_block_t *old_block;

          /* All blocks which are already pinned are promoted to RRPV 0 
           * and are unpinned. So we iterate through the blocks at RRPV 3 
           * and move all the blocks which are pinned to RRPV 0 */
          if (head_block == NULL)
          {
            CACHE_SARP_INCREMENT_RRPV(SARP_DATA_VALID_HEAD(policy_data), 
                SARP_DATA_VALID_TAIL(policy_data), rrpv);
          }

          /* Find pinned block at victim rrpv and move it to minimum RRPV */
          for (ub4 way_id = 0; way_id < global_data->ways; way_id++)
          {
            old_block = &SARP_DATA_BLOCKS(policy_data)[way_id];
            old_rrpv  = ((rrip_list *)(old_block->data))->rrpv;
            if (old_rrpv == rrpv && old_block->is_block_pinned == TRUE)
            {
              old_block->is_block_pinned  = FALSE;

              /* Move block to min RRPV */
              CACHE_REMOVE_FROM_QUEUE(old_block, SARP_DATA_VALID_HEAD(policy_data)[old_rrpv],
                  SARP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
              CACHE_APPEND_TO_QUEUE(old_block, SARP_DATA_VALID_HEAD(policy_data)[min_rrpv], 
                  SARP_DATA_VALID_TAIL(policy_data)[min_rrpv]);
            }
            else
            {
              if (old_rrpv == rrpv && old_block->is_block_pinned == FALSE)
              {
                break;
              }
            }
          }
        
          head_block = SARP_DATA_VALID_HEAD(policy_data)[rrpv].head;
        }while(!head_block);

        /* Remove a nonbusy block from the tail */
        unsigned int min_wayid = ~(0);

        for (block = SARP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && !block->is_block_pinned && block->way < min_wayid)
            min_wayid = block->way;
        }

        /* If a replacement has happeded, update signature counter table  */
        if (policy_data->is_ship_sampler == TRUE)
        {
          block = &(policy_data->blocks[min_wayid]);
          
          if (!(block->access))
          {
            global_data->ship_dec += 1;
          }

          if (min_wayid != ~(0))
          {

            if (block->ship_sign_valid)
            {
              CACHE_DEC_SHCT(block, global_data);
            }
          }
        }
#if 0
        if (min_wayid != ~(0))
        {
          printf("REP: %ld\n", policy_data->blocks[min_wayid].tag >> 6);
        }
#endif
        ret_wayid = (min_wayid != ~(0)) ? min_wayid : -1;

        break;

      default:
        panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
        ret_wayid = 0;
    }

#undef IS_CRITICAL_STREAM

end:

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

  if (block->stream >= PS && info->stream == (PS + info->core - 1))
  {
    block->is_proc_block = TRUE; 
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

  sstream = get_sarp_stream(global_data, info);
  
  info->sap_stream = sstream;

  blk = &(SARP_DATA_BLOCKS(policy_data)[way]);
  assert(blk);

  old_rrpv  = 0;
  new_rrpv  = 0;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);

  /* Get per-stream policy */
  following_policy = FOLLOW_POLICY(per_stream_policy[info->stream], info);

  assert(FSRRIP(following_policy) || FBRRIP(following_policy) || FSARP(following_policy));
  
  /* Obtain policy based on policy selection counter */
  current_policy = CURRENT_POLICY(policy_data, global_data, following_policy, 
      info, TRUE);

  switch (current_policy)
  {
    case cache_policy_srrip:
    case cache_policy_brrip:
    case cache_policy_sarp:
      /* Update block epoch */
      if (info->fill && blk->spill == FALSE)
      {
      /* Update block epoch */
#define MX_EP  (REPOCH_CNT)

        blk->epoch  = (blk->epoch < MX_EP) ? blk->epoch + 1 : blk->epoch;

#undef MX_EP
      }
      else
      {
        blk->epoch = 0;
      }

      if (info->fill && blk->spill)
      {
        /* Update inter stream flags */
        update_block_xstream(blk, info);
      }

      /* Get old RRPV from the block */
      old_rrpv = (((rrip_list *)(blk->data))->rrpv);
      
      /* Get new RRPV using policy specific function */
      new_rrpv = cache_get_new_rrpv_sarp(policy_data, global_data, info, blk, old_rrpv);

      /* Update stream hit rrpv counter */
      SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), info->stream)[new_rrpv] += 1;

      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        CACHE_REMOVE_FROM_QUEUE(blk, SRRIP_DATA_VALID_HEAD(policy_data)[old_rrpv],
            SRRIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, SRRIP_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            SRRIP_DATA_VALID_TAIL(policy_data)[new_rrpv]);
      }

      blk->dirty |= (info && info->spill) ? TRUE : FALSE;
      blk->spill  = (info && info->spill) ? TRUE : FALSE;
      
      if (info->spill == TRUE)
      {
#define CSBYTH_D              (2)
#define CSBYTH_N              (1)
#define SCOUNT(p, s)          (SMPLRPERF_SPILL(p, s))
#define SREUSE(p, s)          (SMPLRPERF_SREUSE(p, s))
#define SBYPASS_TH(sp, s)     (SREUSE(&((sp)->perfctr), s) * CSBYTH_D <= SCOUNT(&((sp)->perfctr), s) * CSBYTH_N)
#define CBYPASS(sp, i)        SBYPASS_TH(sp, (i)->stream)
#define NCRTCL_BYPASS(g, i)   (!CRITICAL_STREAM(g, i) && CBYPASS((g)->sampler, i))
#define EVAL_PCOND(p, g, i)   ((g)->pin_blocks && is_hit_block_pinned(p, g, (g)->sampler, i))
#define EVAL_SCOND(p, g, i)   (!NCRTCL_BYPASS(g, i) && EVAL_PCOND(p, g, i))
#define PIN_HIT_BLK(p, g, i)  ((g)->speedup_enabled ? EVAL_SCOND(p, g, i) : EVAL_PCOND(p, g, i))

        if (PIN_HIT_BLK(policy_data, global_data, info))
        {
          blk->is_block_pinned = TRUE;

          global_data->stream_pinned[info->stream] += 1;
        }
        else
        {
          blk->is_block_pinned = FALSE;
        }
        
#undef CSBYTH_D
#undef CSBYTH_N
#undef SCOUNT
#undef SREUSE
#undef SBYPASS_TH
#undef CBYPASS
#undef NCRTCL_BYPASS
#undef EVAL_PCOND
#undef EVAL_SCOND
#undef PIN_HIT_BLK

        if (old_rrpv != new_rrpv)
        {
          if (is_block_unpinned(policy_data, global_data, info))
          {
            blk->is_block_pinned = FALSE;
          }
        }
      }
      else
      {
        /* Update signature counter table  */
        if (policy_data->is_ship_sampler == TRUE)
        {

          global_data->ship_inc += 1;
          if (blk->ship_sign_valid)
          {
            CACHE_INC_SHCT(blk, global_data);
          }
        }

        blk->is_block_pinned = FALSE;
      }
      
      if (info->fill)
      {
        blk->access += 1;
      }

      CACHE_UPDATE_BLOCK_PC(blk, info->pc);
      CACHE_UPDATE_BLOCK_STREAM(blk, info->stream);
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

  sampler_cache_lookup(global_data->sampler, policy_data, info, FALSE);

  if (info)
  {
    global_data->stream_access[info->stream] += 1;

    sarp_update_hint_count(global_data, info);

    if (++(global_data->speedup_interval) == INTERVAL_SIZE)
    {
      cache_update_interval_end(global_data);
      global_data->speedup_interval = 0;
    }

    if (info->fill)
    {
      if (++((global_data->sampler)->epoch_length) == EPOCH_SIZE)
      {

        printf("Fill RRPV 0: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), CS)[0], 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), ZS)[0],
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), TS)[0]);

        printf("Fill RRPV 2: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), CS)[2], 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), ZS)[2],
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), TS)[2]);

        printf("Fill RRPV 3: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), CS)[3], 
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), ZS)[3],
            SMPLRPERF_FRRPV(&(global_data->sampler->perfctr), TS)[3]);

        printf("Hit RRPV 0: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), CS)[0], 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), ZS)[0],
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), TS)[0]);

        printf("Hit RRPV 2: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), CS)[2], 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), ZS)[2],
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), TS)[2]);

        printf("Hit RRPV 3: C:%ld Z:%ld T:%ld\n", 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), CS)[3], 
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), ZS)[3],
            SMPLRPERF_HRRPV(&(global_data->sampler->perfctr), TS)[3]);
        
        printf("ZS bypass %ld\n", global_data->stream_bypass[ZS]);
        printf("CS bypass %ld\n", global_data->stream_bypass[CS]);
        printf("TS bypass %ld\n", global_data->stream_bypass[TS]);

        printf("ZS fill demote %ld\n", global_data->stream_fdemote[ZS]);
        printf("CS fill demote %ld\n", global_data->stream_fdemote[CS]);
        printf("TS fill demote %ld\n", global_data->stream_fdemote[TS]);

        printf("ZS hit demote %ld\n", global_data->stream_hdemote[ZS]);
        printf("CS hit demote %ld\n", global_data->stream_hdemote[CS]);
        printf("TS hit demote %ld\n", global_data->stream_hdemote[TS]);

        sampler_cache_reset(global_data, global_data->sampler);

        /* Reset epoch length */
        (global_data->sampler)->epoch_length = 0;

        printf("\nPriority hints : speedup stream %d\n", global_data->speedup_stream);

        printf("CSAccess:%ld CSMiss:%ld\n", global_data->stream_access[CS],
            global_data->stream_miss[CS]);

        printf("CSPin:%ld CSPFill:%ld CSPhit:%ld\n", 
            global_data->stream_pinned[CS], global_data->stream_pfill[CS],
            global_data->stream_phit[CS]);

        printf("ZSAccess:%ld ZSMiss:%ld\n", global_data->stream_access[ZS],
            global_data->stream_miss[ZS]);

        printf("ZSPin:%ld ZSPFill:%ld ZSPhit:%ld\n", 
            global_data->stream_pinned[ZS], global_data->stream_pfill[ZS],
            global_data->stream_phit[ZS]);

        printf("TSPin:%ld TSPFill:%ld TSPhit:%ld\n", 
            global_data->stream_pinned[TS], global_data->stream_pfill[TS],
            global_data->stream_phit[TS]);

        for (ub1 strm = NN; strm <= TST; strm++)
        {
          global_data->stream_pfill[strm]   = 0;
          global_data->stream_phit[strm]    = 0;
        }

        memset(global_data->stream_pinned , 0, sizeof(ub8) * (TST + 1));
        memset(global_data->stream_access , 0, sizeof(ub8) * (TST + 1));
        memset(global_data->stream_miss , 0, sizeof(ub8) * (TST + 1));
      }
    }
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
  sstream = get_sarp_stream(global_data, info);

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
    sarp_data *policy_data, memory_trace *info, ub1 update_time)
{
  ub1 strm;
  ub1 ostrm;
  ub1 offset;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

#define OSTREAM(s, i, w, o) ((s)->blocks[i][w].stream[o])

  ostrm = OSTREAM(sampler, index, way, offset);
  strm  = KNOWN_STREAM(ostrm) ? ostrm : OS;

#undef OSTREAM

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
  
  /* If reuse is xstream reuse */
  if (strm != ostrm)
  {
    sampler->perfctr.xstream_reuse += 1;
  }
  else
  {
    sampler->perfctr.sstream_reuse += 1;
  }
}

void update_sampler_spill_perfctr(sampler_cache *sampler, ub4 index, ub4 way, 
    memory_trace *info)
{
  ub1 offset;
  ub1 strm;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  strm = NEW_STREAM(info);
  
  /* Spill cant be generated by dynamic stream */
  assert(strm != DCS && strm != DZS && strm != DBS && strm != DPS);

  SMPLRPERF_SPILL(&(sampler->perfctr), strm) += 1;
}

/* If previous access was spill */
void update_sampler_fill_reuse_perfctr(sampler_cache *sampler, ub4 index, ub4 way,
    sarp_data *policy_data, memory_trace *info, ub1 update_time)
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

  /* If reuse is xstream reuse */
  if (strm != info->stream)
  {
    sampler->perfctr.xstream_reuse += 1;
  }
  else
  {
    sampler->perfctr.sstream_reuse += 1;
  }
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
    sarp_data *policy_data, memory_trace *info, ub1 update_time)
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
  
  sampler->perfctr.sampler_fill += 1;
}

void sampler_cache_access_block(sampler_cache *sampler, ub4 index, ub4 way,
    sarp_data *policy_data, memory_trace *info, ub1 update_time)
{
  ub4 offset;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  policy_data->hit_count += 1;

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
      update_sampler_spill_reuse_perfctr(sampler, index, way, policy_data, info, update_time);

      /* Update dynamic stream flag */
      update_sampler_xstream_perfctr(sampler, index, way, info);

      /* Increment fill reues */
      update_sampler_fill_per_reuse_epoch_perfctr(sampler, index, way, info);
    }
    else
    {
      /* Increment fill reuse */
      update_sampler_fill_reuse_perfctr(sampler, index, way, policy_data, info, update_time);

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
  
  sampler->perfctr.sampler_hit += 1;
}

void sampler_cache_lookup(sampler_cache *sampler, sarp_data *policy_data, 
    memory_trace *info, ub1 update_time)
{
  ub4 way;
  ub4 index;
  ub8 page;
  ub8 offset;
  ub1 strm;

  /* Obtain sampler index, tag, offset and cache index of the access */
  index   = SAMPLER_INDX(info, sampler);
  page    = SAMPLER_TAG(info, sampler);
  offset  = SAMPLER_OFFSET(info, sampler);
  strm    = NEW_STREAM(info);
  
  if ((offset % ((1 << (LOG_GRAIN_SIZE - LOG_BLOCK_SIZE)) / PAGE_COVERAGE)) == 0)
  {
    sampler->perfctr.sampler_access += 1;

    for (way = 0; way < sampler->ways; way++)
    {
      if (sampler->blocks[index][way].page == page)
      {
        assert(page != SARP_SAMPLER_INVALID_TAG);

        sampler->perfctr.sampler_block_found += 1;
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
        {
          sampler->perfctr.sampler_invalid_block_found += 1;
          break;
        }
      }
    }

    if (way == sampler->ways)
    {
      /* No invalid entry. Replace only if stream occupancy in the sampler is 
       * still below minimum occupancy threshold
       **/

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

        sampler->perfctr.sampler_replace += 1;
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
        sampler_cache_fill_block(sampler, index, way, policy_data, info, 
            FALSE);    
      }
      else
      {
        /* If sampler access was a hit */
        sampler_cache_access_block(sampler, index, way, policy_data, info, 
            TRUE);
      }
    }
  }
}

sampler_entry* sampler_cache_get_block(sampler_cache *sampler, memory_trace *info)
{
  ub4 way;
  ub4 index;
  ub8 page;
  ub8 offset;
  ub1 strm;

  /* Obtain sampler index, tag, offset and cache index of the access */
  index   = SAMPLER_INDX(info, sampler);
  page    = SAMPLER_TAG(info, sampler);
  offset  = SAMPLER_OFFSET(info, sampler);

  for (way = 0; way < sampler->ways; way++)
  {
    if (sampler->blocks[index][way].page == page)
    {
      assert(page != SARP_SAMPLER_INVALID_TAG);
      break;
    }
  }

  return (way < sampler->ways) ? &(sampler->blocks[index][way]) : NULL;
}

#undef INTERVAL_SIZE
#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
#undef ECTR_WIDTH
#undef ECTR_MIN_VAL
#undef ECTR_MAX_VAL
#undef ECTR_MID_VAL
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
#undef STRMSPD
#undef SPDSTRM
#undef CR_TH
#undef CRITICAL_STREAM
#undef KNOWN_CSTREAM
#undef INSPD_RANGE
#undef CRITICAL_COND
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
#undef GFILL_VAL
#undef PCBRRIP
#undef PCSRRIP
#undef DPSARP
#undef DGFILL
#undef CHGFILL
#undef SPSARP
#undef FANDH
#undef FANDM
#undef SPEEDUP
#undef CURRENT_POLICY
#undef GFENBL
#undef FOLLOW_POLICY
#undef SAMPLER_INDX
#undef SAMPLER_TAG
#undef SAMPLER_OFFSET
#undef SMPLR
#undef CPRD
#undef CHK_CRTCL
#undef ALWAYS_CTRCL
#undef FOUND_CTRCL
#undef GST
#undef CHK_CTA
#undef CHK_BTA
#undef CTC_CNT
#undef BTC_CNT
#undef IS_INTERS
#undef INSTRS_CRTCL
#undef SIGNSIZE
#undef SHCTSIZE
#undef COREBTS
#undef SIGMAX_VAL
#undef SIGNMASK
#undef SIGNP
#undef SIGNM
#undef SIGNCORE
#undef GET_SSIGN
#undef SHIPSIGN
