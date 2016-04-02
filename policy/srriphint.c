/*
 *  Copyright (C) 2014  Siddharth Rai
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <assert.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "srriphint.h"
#include "sap.h"

#define RPL_GPU_FIRST             (0)
#define RPL_GPU_COND_FIRST        (1)
#define FILL_TEST_SHIP            (2)
#define FILL_TEST_SRRIP           (3)

#define CACHE_SET(cache, set)     (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)     (&((set)->blocks[way]))
#define BYPASS_RRPV               (-1)

#define INTERVAL_SIZE             (1 << 14)
#define EPOCH_SIZE                (1 << 19)
#define PAGE_COVERAGE             (8)
#define PSEL_WIDTH                (20)
#define PSEL_MIN_VAL              (0x00)  
#define PSEL_MAX_VAL              (0xfffff)  
#define PSEL_MID_VAL              (1 << 19)

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

#define R_GPU_SET(p)              ((p)->set_type == SRRIPHINT_GPU_SET)
#define R_CPU_SET(p)              ((p)->set_type == SRRIPHINT_GPU_COND_SET)
#define F_R_SET(p)                (!R_GPU_SET(p) && !R_CPU_SET(p))
#define R_GPU_FIRST(g)            (SAT_CTR_VAL((g)->rpl_ctr) <= PSEL_MID_VAL)
#define R_GPU_COND(g)             (SAT_CTR_VAL((g)->rpl_ctr) > PSEL_MID_VAL)
#define F_GPU_FIRST(p, g)         (F_R_SET(p) && R_GPU_FIRST(g))
#define GET_RPOLICY(p, g)         ((R_GPU_SET(p) || F_GPU_FIRST(p, g)) ? RPL_GPU_FIRST : RPL_GPU_COND_FIRST)

#define FILL_SHIP_SET(p)          ((p)->set_type == SRRIPHINT_SHIP_FILL_SET)
#define FILL_SRRIP_SET(p)         ((p)->set_type == SRRIPHINT_SRRIP_FILL_SET)
#define FILL_FOLLOW_SET(p)        (!FILL_SHIP_SET(p) && !FILL_SRRIP_SET(p))
#define FILL_FOLLOW_SHIP(g)       (SAT_CTR_VAL((g)->fill_ctr) <= PSEL_MID_VAL)
#define FILL_WITH_SHIP(p, g)      (FILL_FOLLOW_SET(p) && FILL_FOLLOW_SHIP(g))
#define GET_FILL_POLICY(p, g)     ((FILL_SHIP_SET(p) || FILL_WITH_SHIP(p, g)) ? FILL_TEST_SHIP : FILL_TEST_SRRIP)

#define CACHE_UPDATE_BLOCK_STATE(block, tag, va, state_in)        \
do                                                                \
{                                                                 \
  (block)->tag      = (tag);                                      \
  (block)->vtl_addr = (va);                                       \
  (block)->state    = (state_in);                                 \
}while(0)

#define CACHE_SRRIPHINT_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)  \
do                                                                \
{                                                                 \
    int dif = 0;                                                  \
                                                                  \
  for (int i = rrpv - 1; i >= 0; i--)                             \
  {                                                               \
    assert(i <= rrpv);                                            \
                                                                  \
    if ((head_ptr)[i].head)                                       \
    {                                                             \
      if (!dif)                                                   \
      {                                                           \
        dif = rrpv - i;                                           \
      }                                                           \
                                                                  \
      assert((tail_ptr)[i].head);                                 \
      (head_ptr)[i + dif].head  = (head_ptr)[i].head;             \
      (tail_ptr)[i + dif].head  = (tail_ptr)[i].head;             \
      (head_ptr)[i].head        = NULL;                           \
      (tail_ptr)[i].head        = NULL;                           \
                                                                  \
      struct cache_block_t *node = (head_ptr)[i + dif].head;      \
                                                                  \
      /* point data to new RRPV head */                           \
      for (; node; node = node->prev)                             \
      {                                                           \
        node->data = &(head_ptr[i + dif]);                        \
      }                                                           \
    }                                                             \
    else                                                          \
    {                                                             \
      assert(!((tail_ptr)[i].head));                              \
    }                                                             \
  }                                                               \
}while(0)

#define CACHE_GET_BLOCK_STREAM(block, strm)                       \
do                                                                \
{                                                                 \
  strm = (block)->stream;                                         \
}while(0)

#define CACHE_UPDATE_BLOCK_STREAM(block, strm)                    \
do                                                                \
{                                                                 \
  (block)->stream = strm;                                         \
}while(0)

/*
 * Public Variables
 */

/*
 * Private Functions
 */
#define free_list_remove_block(set, blk)                                                      \
do                                                                                            \
{                                                                                             \
  /* Check: free list must be non empty as it contains the current block. */                  \
  assert(SRRIPHINT_DATA_FREE_HEAD(set) && SRRIPHINT_DATA_FREE_HEAD(set));                     \
                                                                                              \
  /* Check: current block must be in invalid state */                                         \
  assert((blk)->state == cache_block_invalid);                                                \
                                                                                              \
  CACHE_REMOVE_FROM_SQUEUE(blk, SRRIPHINT_DATA_FREE_HEAD(set), SRRIPHINT_DATA_FREE_TAIL(set));\
                                                                                              \
  (blk)->next = NULL;                                                                         \
  (blk)->prev = NULL;                                                                         \
                                                                                              \
  /* Reset block state */                                                                     \
  (blk)->busy = 0;                                                                            \
  (blk)->cached = 0;                                                                          \
  (blk)->prefetch = 0;                                                                        \
}while(0);


/* Macros to obtain signature */
#define SHIP_MAX                      (7)
#define SIGNSIZE(g)                   ((g)->sign_size)
#define SHCTSIZE(g)                   ((g)->shct_size)
#define SIGMAX_VAL(g)                 (((1 << SIGNSIZE(g)) - 1))
#define SIGNM(g, i)                   ((((i)->address >> SHCTSIZE(g)) & SIGMAX_VAL(g)))
#define GET_SSIGN(g, i)               (SIGNM(g, i))
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

void shnt_sampler_cache_lookup(shnt_sampler_cache *sampler, srriphint_data *policy_data, 
    memory_trace *info, ub1 update_time);

static ub4 get_set_type_srriphint(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x0f;
  msb_bits = (indx >> 6) & 0x0f;
  mid_bits = (indx >> 4) & 0x03;

  if (lsb_bits == msb_bits && mid_bits == 0)
  {
    return SRRIPHINT_GPU_SET;
  }

  if (lsb_bits == (~msb_bits & 0x0f) && mid_bits == 0)
  {
    return SRRIPHINT_GPU_COND_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 1)
  {
    return SRRIPHINT_SHIP_FILL_SET;
  }

  if (lsb_bits == (~msb_bits & 0xf) && mid_bits == 1)
  {
    return SRRIPHINT_SRRIP_FILL_SET;
  }

  return SRRIPHINT_FOLLOWER_SET;
}

#define BLK_PER_ENTRY (64)

void shnt_sampler_cache_init(shnt_sampler_cache *sampler, ub4 sets, ub4 ways)
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
  sampler->blocks = (srriphint_sampler_entry **)xcalloc(sets, sizeof(srriphint_sampler_entry *));
  assert(sampler->blocks);  

  for (ub4 i = 0; i < sets; i++)
  {
    (sampler->blocks)[i] = (srriphint_sampler_entry *)xcalloc(ways, sizeof(srriphint_sampler_entry));
    assert((sampler->blocks)[i]);

    for (ub4 j = 0; j < ways; j++)
    {
      (sampler->blocks)[i][j].page          = SARP_SAMPLER_INVALID_TAG;
      (sampler->blocks)[i][j].timestamp     = (ub8 *)xcalloc(BLK_PER_ENTRY, sizeof(ub8));
      (sampler->blocks)[i][j].spill_or_fill = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].stream        = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].valid         = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].hit_count     = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_color = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_depth = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_blit  = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_proc  = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
    }
  }
  
  /* Initialize sampler performance counters */
  memset(&(sampler->perfctr), 0 , sizeof(srriphint_sampler_perfctr));
  memset(sampler->stream_occupancy, 0 , sizeof(ub4) * (TST + 1));
}

void shnt_sampler_cache_reset(srriphint_gdata *global_data, shnt_sampler_cache *sampler)
{
  int sampler_occupancy;

  assert(sampler);
#if 0  
  printf("SMPLR RESET : Replacments [%d] Fill [%ld] Hit [%ld]\n", sampler->perfctr.sampler_replace,
      sampler->perfctr.sampler_fill, sampler->perfctr.sampler_hit);
#endif
  sampler_occupancy = 0;
#if 0  
  printf("[C] F:%5ld FR: %5ld S: %5ld SR: %5ld\n", sampler->perfctr.fill_count[CS], 
      sampler->perfctr.fill_reuse_count[CS], sampler->perfctr.spill_count[CS],
      sampler->perfctr.spill_reuse_count[CS]);

  printf("[Z] F:%5ld R: %5ld S: %5ld SR: %5ld\n", sampler->perfctr.fill_count[ZS], 
      sampler->perfctr.fill_reuse_count[ZS], sampler->perfctr.spill_count[ZS],
      sampler->perfctr.spill_reuse_count[ZS]);

  printf("[B] F:%5ld R: %5ld S: %5ld SR: %5ld\n", sampler->perfctr.fill_count[BS], 
      sampler->perfctr.fill_reuse_count[BS], sampler->perfctr.spill_count[BS],
      sampler->perfctr.spill_reuse_count[BS]);
#endif 

  printf("[P] F:%5ld R: %5ld S: %5ld SR: %5ld\n", sampler->perfctr.fill_count[PS], 
      sampler->perfctr.fill_reuse_count[PS], sampler->perfctr.spill_count[PS],
      sampler->perfctr.spill_reuse_count[PS]);
#if 0
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
#endif

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

#define SPEEDUP(n) (n == sarp_stream_x)
void cache_init_srriphint(ub4 set_indx, struct cache_params *params, srriphint_data *policy_data, 
    srriphint_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

  if (set_indx == 0)
  {
    global_data->ways       = params->ways;
    global_data->sign_size  = params->ship_sig_size;
    global_data->shct_size  = params->ship_shct_size;
    global_data->core_size  = params->ship_core_size;
    global_data->cpu_bmc    = 0;

    for (int i = NN; i <= TST; i++)
    {
      global_data->stream_blocks[i] = 0;
      global_data->stream_reuse[i]  = 0;
    }

    /* Allocate and initialize sampler cache */
    global_data->sampler = (shnt_sampler_cache *)xcalloc(1, sizeof(shnt_sampler_cache));
    assert(global_data->sampler);  

    shnt_sampler_cache_init(global_data->sampler, params->sampler_sets, 
        params->sampler_ways);

    global_data->cpu_rpsel  = 0;
    global_data->gpu_rpsel  = 0;
    global_data->cpu_zevct  = 0;
    global_data->gpu_zevct  = 0;
    global_data->cpu_blocks = 0;
    global_data->gpu_blocks = 0;

    global_data->ship_shct = (ub1 *) xcalloc((1 << global_data->shct_size), sizeof(ub1));
    assert(global_data->ship_shct);

    /* Initialize counter table */ 
    for (ub4 i = 0; i < (1 << global_data->shct_size); i++)
    {
      global_data->ship_shct[i] = 0;
    }

    /* Initialize replacement policy selection counter */
    SAT_CTR_INI(global_data->rpl_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->rpl_ctr, PSEL_MID_VAL);

    /* Initialize fill policy selection counter */
    SAT_CTR_INI(global_data->fill_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->fill_ctr, PSEL_MID_VAL);
  }

  policy_data->set_type   = get_set_type_srriphint(set_indx);
  policy_data->cpu_blocks = 0;
  policy_data->gpu_blocks = 0;
  
  switch (policy_data->set_type)
  {
    case SRRIPHINT_SRRIP_SET:
      policy_data->following = cache_policy_srrip;

      /* Set current and default fill policy to SRRIP */
      SRRIPHINT_DATA_CFPOLICY(policy_data) = cache_policy_srrip;
      SRRIPHINT_DATA_DFPOLICY(policy_data) = cache_policy_srrip;
      SRRIPHINT_DATA_CAPOLICY(policy_data) = cache_policy_srrip;
      SRRIPHINT_DATA_DAPOLICY(policy_data) = cache_policy_srrip;
      SRRIPHINT_DATA_CRPOLICY(policy_data) = cache_policy_srrip;
      SRRIPHINT_DATA_DRPOLICY(policy_data) = cache_policy_srrip;
      break;

    case SRRIPHINT_FOLLOWER_SET:
    case SRRIPHINT_GPU_SET:
    case SRRIPHINT_GPU_COND_SET:
    case SRRIPHINT_SHIP_FILL_SET:
    case SRRIPHINT_SRRIP_FILL_SET:
      policy_data->following = cache_policy_srriphint;

      /* Set current and default fill policy to SRRIP */
      SRRIPHINT_DATA_CFPOLICY(policy_data) = cache_policy_srriphint;
      SRRIPHINT_DATA_DFPOLICY(policy_data) = cache_policy_srriphint;
      SRRIPHINT_DATA_CAPOLICY(policy_data) = cache_policy_srriphint;
      SRRIPHINT_DATA_DAPOLICY(policy_data) = cache_policy_srriphint;
      SRRIPHINT_DATA_CRPOLICY(policy_data) = cache_policy_srriphint;
      SRRIPHINT_DATA_DRPOLICY(policy_data) = cache_policy_srriphint;
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create RRPV buckets */
  SRRIPHINT_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SRRIPHINT_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  
  /* Create CPU RRPV buckets */
  SRRIPHINT_DATA_CVALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SRRIPHINT_DATA_CVALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  
  /* Create GPU RRPV buckets */
  SRRIPHINT_DATA_GVALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SRRIPHINT_DATA_GVALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC  

  assert(SRRIPHINT_DATA_VALID_HEAD(policy_data));
  assert(SRRIPHINT_DATA_VALID_TAIL(policy_data));
  assert(SRRIPHINT_DATA_CVALID_HEAD(policy_data));
  assert(SRRIPHINT_DATA_CVALID_TAIL(policy_data));
  assert(SRRIPHINT_DATA_GVALID_HEAD(policy_data));
  assert(SRRIPHINT_DATA_GVALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIPHINT_DATA_MAX_RRPV(policy_data)    = MAX_RRPV;
  SRRIPHINT_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  assert(params->spill_rrpv <= MAX_RRPV);

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SRRIPHINT_DATA_VALID_HEAD(policy_data)[i].rrpv  = i;
    SRRIPHINT_DATA_VALID_HEAD(policy_data)[i].head  = NULL;
    SRRIPHINT_DATA_VALID_TAIL(policy_data)[i].head  = NULL;

    SRRIPHINT_DATA_CVALID_HEAD(policy_data)[i].rrpv = i;
    SRRIPHINT_DATA_CVALID_HEAD(policy_data)[i].head = NULL;
    SRRIPHINT_DATA_CVALID_TAIL(policy_data)[i].head = NULL;

    SRRIPHINT_DATA_GVALID_HEAD(policy_data)[i].rrpv = i;
    SRRIPHINT_DATA_GVALID_HEAD(policy_data)[i].head = NULL;
    SRRIPHINT_DATA_GVALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  SRRIPHINT_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

  SRRIPHINT_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(SRRIPHINT_DATA_FREE_HLST(policy_data));

  SRRIPHINT_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(SRRIPHINT_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  SRRIPHINT_DATA_FREE_HEAD(policy_data) = &(SRRIPHINT_DATA_BLOCKS(policy_data)[0]);
  SRRIPHINT_DATA_FREE_TAIL(policy_data) = &(SRRIPHINT_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SRRIPHINT_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SRRIPHINT_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SRRIPHINT_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SRRIPHINT_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SRRIPHINT_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SRRIPHINT_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

  assert(SRRIPHINT_DATA_MAX_RRPV(policy_data) != 0);

#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_srriphint(srriphint_data *policy_data)
{
  /* Free all data blocks */
  free(SRRIPHINT_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (SRRIPHINT_DATA_VALID_HEAD(policy_data))
  {
    free(SRRIPHINT_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SRRIPHINT_DATA_VALID_TAIL(policy_data))
  {
    free(SRRIPHINT_DATA_VALID_TAIL(policy_data));
  }

  /* Free valid head buckets */
  if (SRRIPHINT_DATA_CVALID_HEAD(policy_data))
  {
    free(SRRIPHINT_DATA_CVALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SRRIPHINT_DATA_CVALID_TAIL(policy_data))
  {
    free(SRRIPHINT_DATA_CVALID_TAIL(policy_data));
  }

  /* Free valid head buckets */
  if (SRRIPHINT_DATA_GVALID_HEAD(policy_data))
  {
    free(SRRIPHINT_DATA_GVALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SRRIPHINT_DATA_GVALID_TAIL(policy_data))
  {
    free(SRRIPHINT_DATA_GVALID_TAIL(policy_data));
  }
}

struct cache_block_t* cache_find_block_srriphint(srriphint_data *policy_data, 
    srriphint_gdata *global_data, memory_trace *info, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SRRIPHINT_DATA_GVALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SRRIPHINT_DATA_CVALID_HEAD(policy_data)[rrpv].head;

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
      switch (policy_data->set_type)
      {
        case SRRIPHINT_GPU_SET:
          SAT_CTR_INC(global_data->rpl_ctr);
          break;

        case SRRIPHINT_GPU_COND_SET:
          SAT_CTR_DEC(global_data->rpl_ctr);
          break;

        case SRRIPHINT_SHIP_FILL_SET:
          if (CPU_STREAM(info->stream))
          {
            SAT_CTR_INC(global_data->fill_ctr);
          }
          break;

        case SRRIPHINT_SRRIP_FILL_SET:
          if (CPU_STREAM(info->stream))
          {
            SAT_CTR_DEC(global_data->fill_ctr);
          }
          break;

        case SRRIPHINT_FOLLOWER_SET:
          break;

        default:
          panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      }
    }
  }

  return node;
}

void cache_fill_block_srriphint(srriphint_data *policy_data, srriphint_gdata *global_data,
    int way, long long tag, enum cache_block_state_t state, int strm, 
    memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  switch (policy_data->following)
  {
    case cache_policy_srrip:
      if (++(global_data->cache_access) >= INTERVAL_SIZE)
      {
        printf("Blocks CPU:%6ld GPU:%6ld\n", global_data->cpu_blocks, global_data->gpu_blocks);
#if 0
        printf("Block Count C:%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->stream_blocks[CS], 
            global_data->stream_blocks[ZS], global_data->stream_blocks[TS], global_data->stream_blocks[BS],
            global_data->stream_blocks[PS]);

        printf("Block Reuse C:%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->stream_reuse[CS], 
            global_data->stream_reuse[ZS], global_data->stream_reuse[TS], global_data->stream_reuse[BS],
            global_data->stream_reuse[PS]);
#endif

        for (ub1 i = 0; i <= TST; i++)
        {
          global_data->stream_reuse[i]  /= 2;
        }

        global_data->cache_access = 0;
        global_data->cpu_zevct    = 0;
        global_data->gpu_zevct    = 0;
      }

    case cache_policy_srriphint:
      if (way != BYPASS_WAY)
      {
        /* Obtain SRRIP specific data */
        block = &(SRRIPHINT_DATA_BLOCKS(policy_data)[way]);
        block->access = 0;

        assert(block->stream == 0);

        /* Get RRPV to be assigned to the new block */

        rrpv = cache_get_fill_rrpv_srriphint(policy_data, global_data, info, block->epoch);

        /* If block is not bypassed */
        if (rrpv != BYPASS_RRPV)
        {
          /* Ensure a valid RRPV */
          assert(rrpv >= 0 && rrpv <= policy_data->max_rrpv); 

          /* Remove block from free list */
          free_list_remove_block(policy_data, block);

          /* Update new block state and stream */
          CACHE_UPDATE_BLOCK_STATE(block, tag, info->vtl_addr, state);
          CACHE_UPDATE_BLOCK_STREAM(block, strm);

          block->dirty            = (info && info->spill) ? TRUE : FALSE;
          block->spill            = (info && info->spill) ? TRUE : FALSE;
          block->last_rrpv        = rrpv;
          block->ship_sign        = SHIPSIGN(global_data, info);
          block->ship_sign_valid  = TRUE;

          assert(block->next == NULL && block->prev == NULL);

          /* Insert block in to the corresponding RRPV queue */
          if (GPU_STREAM(strm))
          {
            CACHE_APPEND_TO_QUEUE(block, 
                SRRIPHINT_DATA_GVALID_HEAD(policy_data)[rrpv], 
                SRRIPHINT_DATA_GVALID_TAIL(policy_data)[rrpv]);

            global_data->gpu_blocks += 1;
            policy_data->gpu_blocks += 1;
          }
          else
          {
            CACHE_APPEND_TO_QUEUE(block, 
                SRRIPHINT_DATA_CVALID_HEAD(policy_data)[rrpv], 
                SRRIPHINT_DATA_CVALID_TAIL(policy_data)[rrpv]);

            global_data->cpu_blocks += 1;
            policy_data->cpu_blocks += 1;

            assert(policy_data->cpu_blocks <= 16);
          }

          if (policy_data->set_type == SRRIPHINT_SRRIP_SET)
          {
            global_data->stream_blocks[info->stream] += 1;
          }
        }
      }
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

  if (info->fill)
  {
    if (++((global_data->sampler)->epoch_length) == EPOCH_SIZE)
    {

      shnt_sampler_cache_reset(global_data, global_data->sampler);

      (global_data->sampler)->epoch_length = 0;
    }
  }

  shnt_sampler_cache_lookup(global_data->sampler, policy_data, info, TRUE);

  SRRIPHINT_DATA_CFPOLICY(policy_data) = SRRIPHINT_DATA_DFPOLICY(policy_data);
}

int cache_replace_block_srriphint(srriphint_data *policy_data, srriphint_gdata *global_data,
    memory_trace *info)
{
  struct  cache_block_t *block;
  struct  cache_block_t *vctm_block;
  sb4     rrpv;
  ub4     min_wayid;
  ub4     gpu_zblocks;
  ub4     cpu_zblocks;

  /* Remove a nonbusy block from the tail */
  min_wayid   = ~(0);
  vctm_block  = NULL;
  cpu_zblocks = 0;
  gpu_zblocks = 0;

  if ((info->stream != CS && info->stream != ZS && info->stream != BS) && info->spill == TRUE)
  {
    min_wayid = BYPASS_WAY;
    goto end;
  }

  /* Try to find an invalid block always from head of the free list. */
  for (block = SRRIPHINT_DATA_FREE_HEAD(policy_data); block; block = block->prev)
  {
    return block->way;
  }

  /* Obtain RRPV from where to replace the block */
  rrpv = cache_get_replacement_rrpv_srriphint(policy_data);

  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= SRRIPHINT_DATA_MAX_RRPV(policy_data));

  /* If there is no block with required RRPV, increment RRPV of all the blocks
   * until we get one with the required RRPV */
  if (!SRRIPHINT_DATA_GVALID_HEAD(policy_data)[rrpv].head && !SRRIPHINT_DATA_CVALID_HEAD(policy_data)[rrpv].head)
  {
    if (!SRRIPHINT_DATA_GVALID_HEAD(policy_data)[rrpv].head)
    {
      /* All blocks which are already pinned are promoted to RRPV 0 
       * and are unpinned. So we iterate through the blocks at RRPV 3 
       * and move all the blocks which are pinned to RRPV 0 */
      CACHE_SRRIPHINT_INCREMENT_RRPV(SRRIPHINT_DATA_GVALID_HEAD(policy_data), 
          SRRIPHINT_DATA_GVALID_TAIL(policy_data), rrpv);
    }

    if (!SRRIPHINT_DATA_CVALID_HEAD(policy_data)[rrpv].head)
    {
      /* All blocks which are already pinned are promoted to RRPV 0 
       * and are unpinned. So we iterate through the blocks at RRPV 3 
       * and move all the blocks which are pinned to RRPV 0 */
      CACHE_SRRIPHINT_INCREMENT_RRPV(SRRIPHINT_DATA_CVALID_HEAD(policy_data), 
          SRRIPHINT_DATA_CVALID_TAIL(policy_data), rrpv);
    }
  }


  switch (SRRIPHINT_DATA_CRPOLICY(policy_data))
  {
    case cache_policy_srriphint:
    case cache_policy_cpulast:
    case cache_policy_srrip:

#define PU_STR(g, s) ((g)->sampler->perfctr.fill_reuse_count[(s)])
#define PU_STF(g, s) ((g)->sampler->perfctr.fill_count[(s)])
#define CPU_FREUSE(g) (PU_STR(g, PS) + PU_STR(g, PS1) + PU_STR(g, PS2) + PU_STR(g, PS3))
#define CPU_FCOUNT(g) (PU_STF(g, PS) + PU_STF(g, PS1) + PU_STF(g, PS2) + PU_STF(g, PS3))

      if (GPU_STREAM(info->stream)) 
      {
        assert(policy_data->set_type == SRRIPHINT_GPU_SET || 
            policy_data->set_type == SRRIPHINT_GPU_COND_SET || 
            SRRIPHINT_FOLLOWER_SET);

        switch(GET_RPOLICY(policy_data, global_data))
        {
          case RPL_GPU_FIRST:
            if (PU_STR(global_data, info->stream) < PU_STF(global_data, info->stream) / 2)
            {
              if (!SRRIPHINT_DATA_GVALID_HEAD(policy_data)[rrpv].head)
              {
                /* All blocks which are already pinned are promoted to RRPV 0 
                 * and are unpinned. So we iterate through the blocks at RRPV 3 
                 * and move all the blocks which are pinned to RRPV 0 */
                CACHE_SRRIPHINT_INCREMENT_RRPV(SRRIPHINT_DATA_GVALID_HEAD(policy_data), 
                    SRRIPHINT_DATA_GVALID_TAIL(policy_data), rrpv);
              }

              for (block = SRRIPHINT_DATA_GVALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
              {
                if (!block->busy && (block->way < min_wayid))
                {
                  min_wayid = block->way;
                  vctm_block  = block;
                }
              }
            }
            break;

          case RPL_GPU_COND_FIRST:
            if (PU_STR(global_data, info->stream) < PU_STF(global_data, info->stream) / 2)
            {
              if (CPU_FREUSE(global_data) > CPU_FCOUNT(global_data) / 2)
              {
                if (!SRRIPHINT_DATA_GVALID_HEAD(policy_data)[rrpv].head)
                {
                  /* All blocks which are already pinned are promoted to RRPV 0 
                   * and are unpinned. So we iterate through the blocks at RRPV 3 
                   * and move all the blocks which are pinned to RRPV 0 */
                  CACHE_SRRIPHINT_INCREMENT_RRPV(SRRIPHINT_DATA_GVALID_HEAD(policy_data), 
                      SRRIPHINT_DATA_GVALID_TAIL(policy_data), rrpv);
                }

                for (block = SRRIPHINT_DATA_GVALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
                {
                  if (!block->busy && (block->way < min_wayid))
                  {
                    min_wayid = block->way;
                    vctm_block  = block;
                  }
                }
              }
            }
            break;

          default:
            panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
        }
      }

#undef GPU_STR
#undef GPU_STF
#undef CPU_FREUSE
#undef CPU_FCOUNT

      if (min_wayid == ~(0))
      {
        for (block = SRRIPHINT_DATA_GVALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid))
          {
            min_wayid = block->way;
            vctm_block  = block;
          }
        }

        for (block = SRRIPHINT_DATA_CVALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid))
          {
            min_wayid = block->way;
            vctm_block  = block;
          }
        }
      }

      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

end:
  if (vctm_block)
  {
    if (GPU_STREAM(vctm_block->stream))
    {
      if (!vctm_block->access)
      {
        global_data->gpu_zevct += 1;
      }

      if (global_data->gpu_blocks)
      {
        global_data->gpu_blocks -= 1;
        policy_data->gpu_blocks -= 1;
      }
    }
    else
    {
      if (!vctm_block->access)
      {
        global_data->cpu_zevct += 1;
      }

      if (global_data->cpu_blocks)
      {
        global_data->cpu_blocks -= 1;
      }

      assert(policy_data->cpu_blocks);

      policy_data->cpu_blocks -= 1;
    }

    if (CPU_STREAM(vctm_block->stream) && vctm_block->ship_sign_valid)
    {
      CACHE_DEC_SHCT(vctm_block, global_data);
    }
  }
  
  if (min_wayid != ~(0) && min_wayid != BYPASS_WAY)
  {
    assert(vctm_block);
  }

  /* If no non busy block can be found, return -1 */
  return (min_wayid != ~(0)) ? min_wayid : -1;
}

void cache_access_block_srriphint(srriphint_data *policy_data, srriphint_gdata *global_data,
    int way, int strm, memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int old_rrpv;
  int new_rrpv;

  blk  = &(SRRIPHINT_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);

  switch (SRRIPHINT_DATA_CAPOLICY(policy_data))
  {
    case cache_policy_srrip:
      if (blk->stream != strm)
      {
        assert(global_data->stream_blocks[blk->stream]);

        global_data->stream_blocks[blk->stream] -= 1;
        global_data->stream_blocks[strm]        += 1;
      }

      global_data->stream_reuse[blk->stream]  += 1;


      if (++(global_data->cache_access) >= INTERVAL_SIZE)
      {
        printf("Blocks CPU:%6ld GPU:%6ld\n", global_data->cpu_blocks, global_data->gpu_blocks);
#if 0
        printf("Block Count C:%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->stream_blocks[CS], 
            global_data->stream_blocks[ZS], global_data->stream_blocks[TS], global_data->stream_blocks[BS],
            global_data->stream_blocks[PS]);

        printf("Block Reuse C:%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->stream_reuse[CS], 
            global_data->stream_reuse[ZS], global_data->stream_reuse[TS], global_data->stream_reuse[BS],
            global_data->stream_reuse[PS]);
#endif
        for (ub1 i = 0; i <= TST; i++)
        {
          global_data->stream_reuse[i]  /= 2;
        }

        global_data->cache_access = 0;
      }

    case cache_policy_srriphint:

      /* Get old RRPV from the block */
      old_rrpv = (((rrip_list *)(blk->data))->rrpv);
      new_rrpv = old_rrpv;

      /* Get new RRPV using policy specific function */
      new_rrpv = cache_get_new_rrpv_srriphint(policy_data, global_data, info, 
          old_rrpv, blk, blk->epoch);

      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        blk->last_rrpv = new_rrpv;

        if (GPU_STREAM(info->stream))
        {
          CACHE_REMOVE_FROM_QUEUE(blk, SRRIPHINT_DATA_GVALID_HEAD(policy_data)[old_rrpv],
              SRRIPHINT_DATA_GVALID_TAIL(policy_data)[old_rrpv]);
          CACHE_APPEND_TO_QUEUE(blk, SRRIPHINT_DATA_GVALID_HEAD(policy_data)[new_rrpv], 
              SRRIPHINT_DATA_GVALID_TAIL(policy_data)[new_rrpv]);
        }
        else
        {
          CACHE_REMOVE_FROM_QUEUE(blk, SRRIPHINT_DATA_CVALID_HEAD(policy_data)[old_rrpv],
              SRRIPHINT_DATA_CVALID_TAIL(policy_data)[old_rrpv]);
          CACHE_APPEND_TO_QUEUE(blk, SRRIPHINT_DATA_CVALID_HEAD(policy_data)[new_rrpv], 
              SRRIPHINT_DATA_CVALID_TAIL(policy_data)[new_rrpv]);
        }
      }

      CACHE_UPDATE_BLOCK_STREAM(blk, strm);

      blk->dirty  |= (info && info->spill) ? TRUE : FALSE;
      blk->spill   = (info && info->spill) ? TRUE : FALSE;
      blk->access += 1;

      if (CPU_STREAM(info->stream) && info->fill && blk->ship_sign_valid)
      {
        CACHE_INC_SHCT(blk, global_data);
      }
      break;

    case cache_policy_bypass:
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

  if (info->fill)
  {
    if (++((global_data->sampler)->epoch_length) == EPOCH_SIZE)
    {

      shnt_sampler_cache_reset(global_data, global_data->sampler);

      (global_data->sampler)->epoch_length = 0;
    }
  }

  shnt_sampler_cache_lookup(global_data->sampler, policy_data, info, FALSE);
}

int cache_get_fill_rrpv_srriphint(srriphint_data *policy_data, 
    srriphint_gdata *global_data, memory_trace *info, ub4 epoch)
{
  srriphint_sampler_perfctr *perfctr;

  int ret_rrpv;
  ub1 strm;

  switch (SRRIPHINT_DATA_CFPOLICY(policy_data))
  {
    case cache_policy_srriphint:
      if (GPU_STREAM(info->stream) && info->spill)
      {
        strm      = NEW_STREAM(info);
        perfctr   = &((global_data->sampler)->perfctr);
        ret_rrpv  = 2;

#define SCOUNT(p, s)      (SMPLRPERF_SPILL(p, s))
#define SRUSE(p, s)       (SMPLRPERF_SREUSE(p, s))
#define SREUSE(p, s)      (SMPLRPERF_SREUSE(p, s))
#define MRUSE(p)          (SMPLRPERF_MREUSE(p))
#define SDLOW(p, s)       (SMPLRPERF_SDLOW(p, s))
#define SDHIGH(p, s)      (SMPLRPERF_SDHIGH(p, s))
#define SMPIN_TH(p, s)    (SRUSE(p, s) * SMTH_DENM > SCOUNT(p, s) * SMTH_NUMR)
#define RRPV1(p, s)       ((SRUSE(p, s) > MRUSE(p) / 2) || SMPIN_TH(p, s))
#define RRPV2(p, s)       ((SRUSE(p, s) > MRUSE(p) / 3))
#define SRD_LOW(p, s)     (SDLOW(p, s) < SDHIGH(p, s))
#define SREUSE_LOW(p, s)  (SREUSE(p, s) == 0 && SCOUNT(p, s) > BYPASS_ACCESS_TH)

        ret_rrpv =  SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;

        if (RRPV1(perfctr, strm) || RRPV2(perfctr, strm))
        {
          ret_rrpv = 0;
        }
        else
        {
          if (SRD_LOW(perfctr, strm) || SREUSE_LOW(perfctr, strm))
          {
            ret_rrpv = 3;
          }
        }

        return ret_rrpv;

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
      }
      else
      {
        strm      = NEW_STREAM(info);
        perfctr   = &((global_data->sampler)->perfctr);

#define MAX_BMC           (32)
#define FREUSE(p, s)      (SMPLRPERF_FREUSE(p, s))
#define FILLS(p, s)       (SMPLRPERF_FILL(p, s))
#define SREUSE(p, s)      (SMPLRPERF_SREUSE(p, s))
#define MRUSE(p)          (SMPLRPERF_MREUSE(p))
#define SPILLS(p, s)      (SMPLRPERF_SPILL(p, s))
#define RRPV1(p, s)       ((SREUSE(p, s) > MRUSE(p) / 2))
#define RRPV2(p, s)       ((SREUSE(p, s) > MRUSE(p) / 3))
#define SMLPR_EFCTV(g, i) ((g)->ship_shct[SHIPSIGN(g, i)])

        if (info->fill)
        {
          switch (GET_FILL_POLICY(policy_data, global_data))
          {
            /* Ship sample set fills with ship */
            case FILL_TEST_SHIP:
              if (CPU_STREAM(info->stream))
              {
                if (!(global_data->ship_shct[SHIPSIGN(global_data, info)]))
                {
                  return SRRIPHINT_DATA_MAX_RRPV(policy_data);
                }
              }
              break;

              /* Non ship sample set fills with srrip */
            case FILL_TEST_SRRIP:
              break;

            default:
              panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
          }
        }

#undef MAX_BMC
#undef FREUSE
#undef FILLS
#undef SREUSE
#undef SPILLS
#undef MRUSE
#undef RRPV1
#undef RRPV2
#undef SMLPR_EFCTV
      }

      return SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;

      break;

    case cache_policy_srrip:
      return SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }
}

int cache_get_replacement_rrpv_srriphint(srriphint_data *policy_data)
{
  return SRRIPHINT_DATA_MAX_RRPV(policy_data);
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
#define RRPV3(p, s)       ((SRUSE(p, s) > SCOUNT(p, s) / 2))
#define SRD_LOW(p, s)     (SDLOW(p, s) < SDHIGH(p, s))
#define SREUSE_LOW(p, s)  (SREUSE(p, s) == 0 && SCOUNT(p, s) > BYPASS_ACCESS_TH)
#define CT_TH1            (64)
#define BT_TH1            (32)
#define TH2               (2)


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

int cache_get_new_rrpv_srriphint(srriphint_data *policy_data, srriphint_gdata *global_data, 
    memory_trace *info, sb4 old_rrpv, struct cache_block_t *block, ub4 epoch)
{
  int ret_rrpv;
  int strm;

  srriphint_sampler_perfctr *perfctr;
  
  perfctr = &((global_data->sampler)->perfctr);
  strm    = NEW_STREAM(info);
  
  ret_rrpv = 0;

  if (GPU_STREAM(info->stream))
  {
    if (info->spill)
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
      }
    }
  }

  return (info && !(info->fill)) ? old_rrpv : ret_rrpv;
}

#undef SCOUNT
#undef FCOUNT
#undef SRUSE
#undef FRUSE
#undef MRUSE
#undef SDLOW
#undef SDHIGH
#undef SHPIN_TH
#undef RRPV1
#undef RRPV2
#undef RRPV3
#undef SRD_LOW
#undef SREUSE_LOW
#undef CT_TH1
#undef BT_TH1
#undef TH2
#undef CSBYTH_D
#undef CSBYTH_N
#undef CFBYTH_D
#undef CFBYTH_N
#undef FBYPASS_TH
#undef SBYPASS_TH
#undef CBYPASS
#undef NCRTCL_BYPASS
#undef CHK_SCRTCL
#undef CHK_SSPD
#undef CHK_FSPD

/* Update state of block. */
void cache_set_block_srriphint(srriphint_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  block = &(SRRIPHINT_DATA_BLOCKS(policy_data))[way];

  /* Check: tag matches with the block's tag. */
  assert(block->tag == tag);

  /* Check: block must be in valid state. It is not possible to set
   * state for an invalid block.*/
  assert(block->state != cache_block_invalid);

  if (state != cache_block_invalid)
  {
    /* Assign access stream */
    CACHE_UPDATE_BLOCK_STATE(block, tag, info->vtl_addr, state);
    CACHE_UPDATE_BLOCK_STREAM(block, stream);
    return;
  }

  /* Invalidate block */
  CACHE_UPDATE_BLOCK_STATE(block, tag, info->vtl_addr, cache_block_invalid);
  CACHE_UPDATE_BLOCK_STREAM(block, NN);
  block->epoch  = 0;

  /* Get old RRPV from the block */
  int old_rrpv = (((rrip_list *)(block->data))->rrpv);

  /* Remove block from valid list and insert into free list */
  if (GPU_STREAM(stream))
  {
    CACHE_REMOVE_FROM_QUEUE(block, SRRIPHINT_DATA_GVALID_HEAD(policy_data)[old_rrpv],
        SRRIPHINT_DATA_GVALID_TAIL(policy_data)[old_rrpv]);
    CACHE_APPEND_TO_SQUEUE(block, SRRIPHINT_DATA_FREE_HEAD(policy_data), 
        SRRIPHINT_DATA_FREE_TAIL(policy_data));
  }
  else
  {
    CACHE_REMOVE_FROM_QUEUE(block, SRRIPHINT_DATA_CVALID_HEAD(policy_data)[old_rrpv],
        SRRIPHINT_DATA_CVALID_TAIL(policy_data)[old_rrpv]);
    CACHE_APPEND_TO_SQUEUE(block, SRRIPHINT_DATA_FREE_HEAD(policy_data), 
        SRRIPHINT_DATA_FREE_TAIL(policy_data));
  }
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_srriphint(srriphint_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  struct cache_block_t ret_block;

  assert(policy_data);
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  memset(&ret_block, 0, sizeof(struct cache_block_t));

  if (way != BYPASS_WAY)
  {
    PTR_ASSIGN(tag_ptr, (SRRIPHINT_DATA_BLOCKS(policy_data)[way]).tag);
    PTR_ASSIGN(state_ptr, (SRRIPHINT_DATA_BLOCKS(policy_data)[way]).state);
    PTR_ASSIGN(stream_ptr, (SRRIPHINT_DATA_BLOCKS(policy_data)[way]).stream);
  }
  else
  {
    PTR_ASSIGN(tag_ptr, 0xdead);
    PTR_ASSIGN(state_ptr, cache_block_invalid);
    PTR_ASSIGN(stream_ptr, NN);
  }

  return (way != BYPASS_WAY) ? SRRIPHINT_DATA_BLOCKS(policy_data)[way] : ret_block;
}

/* Get tag and state of a block. */
int cache_count_block_srriphint(srriphint_data *policy_data, ub1 strm)
{
  int     max_rrpv;
  int     count;
  struct  cache_block_t *head;
  struct  cache_block_t *node;
  
  assert(policy_data);

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;
  count     = 0;
  
  if (GPU_STREAM(strm))
  {
    for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
    {
      head = SRRIPHINT_DATA_GVALID_HEAD(policy_data)[rrpv].head;

      for (node = head; node; node = node->prev)
      {
        assert(node->state != cache_block_invalid);
        if (node->stream == strm)
          count++;
      }
    }
  }
  else
  {
    for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
    {
      head = SRRIPHINT_DATA_CVALID_HEAD(policy_data)[rrpv].head;

      for (node = head; node; node = node->prev)
      {
        assert(node->state != cache_block_invalid);
        if (node->stream == strm)
          count++;
      }
    }
  }

  return count;
}

/* Set policy for next fill */
void cache_set_current_fill_policy_srriphint(srriphint_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  SRRIPHINT_DATA_CFPOLICY(policy_data) = policy;
}

/* Set policy for next access */
void cache_set_current_access_policy_srriphint(srriphint_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  SRRIPHINT_DATA_CAPOLICY(policy_data) = policy;
}

/* Set policy for next replacment */
void cache_set_current_replacement_policy_srriphint(srriphint_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_cpulast);

  SRRIPHINT_DATA_CRPOLICY(policy_data) = policy;
}

/* Update */
void update_shnt_sampler_fill_perfctr(shnt_sampler_cache *sampler, ub4 index, ub4 way, memory_trace *info)
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
void update_shnt_sampler_spill_reuse_perfctr(shnt_sampler_cache *sampler, ub4 index, ub4 way, 
    srriphint_data *policy_data, memory_trace *info, ub1 update_time)
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

void update_shnt_sampler_spill_perfctr(shnt_sampler_cache *sampler, ub4 index, ub4 way, 
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
void update_shnt_sampler_fill_reuse_perfctr(shnt_sampler_cache *sampler, ub4 index, ub4 way,
    srriphint_data *policy_data, memory_trace *info, ub1 update_time)
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

#define GET_SAMPLER_INDX(i, s)    ((((i)->address >> (s)->log_grain_size)) & ((s)->sets - 1))
#define GET_SAMPLER_TAG(i, s)     ((i)->address >> (s)->log_grain_size)
#define GET_SAMPLER_OFFSET(i, s)  (((i)->address >> (s)->log_block_size) & ((s)->entry_size - 1))

static srriphint_sampler_entry* shnt_sampler_cache_get_block(shnt_sampler_cache *sampler, memory_trace *info)
{
  ub4 way;
  ub4 index;
  ub8 page;

  /* Obtain sampler index, tag, offset and cache index of the access */
  index   = GET_SAMPLER_INDX(info, sampler);
  page    = GET_SAMPLER_TAG(info, sampler);

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

#undef GET_SAMPLER_INDX
#undef GET_SAMPLER_TAG
#undef GET_SAMPLER_OFFSET


/* Update fills for current reuse ep ch */
static void update_shnt_sampler_fill_per_reuse_epoch_perfctr(shnt_sampler_cache *sampler, 
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
static void update_shnt_sampler_fill_reuse_per_reuse_epoch_perfctr(shnt_sampler_cache *sampler, 
    ub4 index, ub4 way, srriphint_data *policy_data, memory_trace *info)
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
static void update_shnt_sampler_change_fill_epoch_perfctr(shnt_sampler_cache *sampler, ub4 index, 
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
    update_shnt_sampler_fill_per_reuse_epoch_perfctr(sampler, index, way, info);
  }
}

static void update_shnt_sampler_xstream_perfctr(shnt_sampler_cache *sampler, ub4 index, ub4 way,
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

void shnt_sampler_cache_fill_block(shnt_sampler_cache *sampler, ub4 index, ub4 way, 
    srriphint_data *policy_data, memory_trace *info, ub1 update_time)
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
    update_shnt_sampler_fill_perfctr(sampler, index, way, info);
    update_shnt_sampler_fill_per_reuse_epoch_perfctr(sampler, index, way, info);
  }
  else
  {
    update_shnt_sampler_spill_perfctr(sampler, index, way, info);
  }
  
  sampler->perfctr.sampler_fill += 1;
}

void shnt_sampler_cache_access_block(shnt_sampler_cache *sampler, ub4 index, ub4 way,
    srriphint_data *policy_data, memory_trace *info, ub1 update_time)
{
  ub4 offset;

  /* Obtain sampler entry corresponding to address */
  offset = SAMPLER_OFFSET(info, sampler);
  assert(offset < sampler->entry_size);

  policy_data->hit_count += 1;

  if (info->spill == TRUE)
  {
    /* Update fill count */
    update_shnt_sampler_spill_perfctr(sampler, index, way, info);     
  }
  else
  {
    /* If previous access to sampler was a spill */
    if (sampler->blocks[index][way].spill_or_fill[offset] == TRUE)
    {
      /* Reset reuse-epoch */
      sampler->blocks[index][way].hit_count[offset] = 0;

      /* Increment spill reuse */
      update_shnt_sampler_spill_reuse_perfctr(sampler, index, way, policy_data, info, update_time);

      /* Update dynamic stream flag */
      update_shnt_sampler_xstream_perfctr(sampler, index, way, info);

      /* Increment fill reues */
      update_shnt_sampler_fill_per_reuse_epoch_perfctr(sampler, index, way, info);
    }
    else
    {
      /* Increment fill reuse */
      update_shnt_sampler_fill_reuse_perfctr(sampler, index, way, policy_data, info, update_time);

      /* Increment fill reuse per reues epoch */
      update_shnt_sampler_fill_reuse_per_reuse_epoch_perfctr(sampler, index, way, policy_data, info);
      
      /* Update fill reuse epoch, if epoch has changed, update fill count for
       * new epoch */
      update_shnt_sampler_change_fill_epoch_perfctr(sampler, index, way, info);
    }
    
    /* Update fill count */
    update_shnt_sampler_fill_perfctr(sampler, index, way, info);     
  }

  sampler->blocks[index][way].spill_or_fill[offset] = (info->spill ? TRUE : FALSE);
  sampler->blocks[index][way].stream[offset]        = info->stream;
  sampler->blocks[index][way].timestamp[offset]     = policy_data->miss_count;
  
  sampler->perfctr.sampler_hit += 1;
}

void shnt_sampler_cache_lookup(shnt_sampler_cache *sampler, srriphint_data *policy_data, 
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
        shnt_sampler_cache_fill_block(sampler, index, way, policy_data, info, 
            FALSE);    
      }
      else
      {
        /* If sampler access was a hit */
        shnt_sampler_cache_access_block(sampler, index, way, policy_data, info, 
            TRUE);
      }
    }
  }
}

#undef RPL_GPU_FIRST
#undef RPL_GPU_COND_FIRST
#undef INTERVAL_SIZE
#undef EPOCH_SIZE
#undef RTBL_SIZE_LOG
#undef RTBL_SIZE
#undef RCL_SIGN
#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL
#undef ECTR_WIDTH
#undef ECTR_MIN_VAL
#undef ECTR_MAX_VAL
#undef ECTR_MID_VAL
#undef SRRIPHINT_SRRIP_SET
#undef SRRIPHINT_FOLLOWER_SET
#undef SHIP_MAX
#undef SIGNSIZE
#undef SHCTSIZE
#undef SIGMAX_VAL
#undef SIGNM
#undef GET_SSIGN
#undef SHIPSIGN
#undef CACHE_INC_SHCT
#undef CACHE_DEC_SHCT
#undef R_GPU_SET
#undef R_CPU_SET
#undef F_R_SET
#undef R_GPU_FIRST
#undef R_GPU_COND
#undef F_GPU_FIRST
#undef GET_RPOLICY
#undef FILL_SHIP_SET
#undef FILL_SRRIP_SET
#undef FILL_FOLLOW_SET
#undef FILL_FOLLOW_SHIP
#undef FILL_WITH_SHIP
#undef GET_FILL_POLICY
