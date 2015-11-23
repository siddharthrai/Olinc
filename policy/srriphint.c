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

#define CACHE_SET(cache, set)     (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)     (&((set)->blocks[way]))
#define BYPASS_RRPV               (-1)

#define INTERVAL_SIZE             (1 << 19)
#define PSEL_WIDTH                (20)
#define PSEL_MIN_VAL              (0x00)  
#define PSEL_MAX_VAL              (0xfffff)  
#define PSEL_MID_VAL              (1 << 19)

#define ECTR_WIDTH                (20)
#define ECTR_MIN_VAL              (0x00)  
#define ECTR_MAX_VAL              (0xfffff)  
#define ECTR_MID_VAL              (0x3ff)

#define SRRIPHINT_SRRIP_SET       (0)
#define SRRIPHINT_BRRIP_SET       (1)
#define SRRIPHINT_POLICY_SET      (2)
#define SRRIPHINT_REUSE_SET       (3)
#define SRRIPHINT_FOLLOWER_SET    (4)

#define RTBL_SIZE_LOG             (20)
#define RTBL_SIZE                 (1 << RTBL_SIZE_LOG)
#define RCL_SIGN(i)               (((i)->address >> 20) & (RTBL_SIZE - 1))

#define PAGE_COVERAGE             (8)
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
    return SRRIPHINT_SRRIP_SET;
  }
  else
  {
    if (lsb_bits == msb_bits && mid_bits == 1)
    {
      return SRRIPHINT_BRRIP_SET;
    }
    else
    {
      if (lsb_bits == msb_bits && mid_bits == 2)
      {
        return SRRIPHINT_POLICY_SET;
      }
      else
      {
        if (lsb_bits == msb_bits && mid_bits == 3)
        {
          return SRRIPHINT_REUSE_SET;
        }
      }
    }
  }

  return SRRIPHINT_FOLLOWER_SET;
}

/* Returns SARP stream corresponding to access stream based on stream 
 * classification algoritm. */
srriphint_stream get_srriphint_stream(srriphint_gdata *global_data, memory_trace *info)
{
  return info->sap_stream;
}

#define SPEEDUP(n) (n == srriphint_stream_x)

static void srriphint_update_hint_count(srriphint_gdata *global_data, memory_trace *info)
{
  assert(info->stream <= TST);

  if (SPEEDUP(get_srriphint_stream(global_data, info)))
  {
    SAT_CTR_INC(global_data->sarp_hint[info->stream]);
  }
}

#undef SPEEDUP

#define MAX_RANK (3)

static void cache_update_interval_end(srriphint_gdata *global_data)
{
  /* Obtain stream to be spedup */
  ub1 i;
  ub4 val;
  ub1 new_stream;
  ub1 stream_rank[MAX_RANK];

  val         = 0;
  new_stream  = NN;

  for (i = 0; i < MAX_RANK; i++)
  {
    stream_rank[i] = NN;
  }

  /* Select stream to be spedup */
  for (i = 0; i < TST + 1; i++)
  {
    global_data->speedup_stream[i] = FALSE;

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

  global_data->speedup_stream[new_stream] = TRUE;

#define INTER_REUSE_TH (128)

  if (global_data->ct_reuse > INTER_REUSE_TH && 
      new_stream == TS)
  {
    global_data->speedup_stream[CS] = TRUE;
  }

  if (global_data->bt_reuse > INTER_REUSE_TH && 
      new_stream == TS)
  {
    global_data->speedup_stream[BS] = TRUE;
  }

#undef INTER_REUSE_TH
}

#undef MAX_RANK

#define BLK_PER_ENTRY (64)

static void sh_sampler_cache_init(sh_sampler_cache *sampler, ub4 sets, ub4 ways)
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
  sampler->blocks = (sh_sampler_entry **)xcalloc(sets, sizeof(sh_sampler_entry *));
  assert(sampler->blocks);  

  for (ub4 i = 0; i < sets; i++)
  {
    (sampler->blocks)[i] = (sh_sampler_entry *)xcalloc(ways, sizeof(sh_sampler_entry));
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
  memset(&(sampler->perfctr), 0 , sizeof(sh_sampler_perfctr));
  memset(sampler->stream_occupancy, 0 , sizeof(ub4) * (TST + 1));
}

static void sh_sampler_cache_reset(srriphint_gdata *global_data, sh_sampler_cache *sampler)
{
  int sampler_occupancy;

  assert(sampler);

  printf("SMPLR RESET : Replacments [%d] Fill [%ld] Hit [%ld]\n", sampler->perfctr.sampler_replace,
      sampler->perfctr.sampler_fill, sampler->perfctr.sampler_hit);

  sampler_occupancy = 0;
  
  printf("MAX_REUSE[%5d]\n", global_data->sampler->perfctr.max_reuse);

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
  
#undef MAX_RRPV

  for (ub1 strm = NN; strm <= TST; strm++)
  {
    sampler->stream_occupancy[strm] = 0;
  }
  
  printf("Sampler occupancy [%5d] epoch [%3ld]\n", sampler_occupancy, sampler->epoch_count);

  sampler->epoch_count += 1;
}

#undef BLK_PER_ENTRY

/* Update */
static void sh_update_sampler_fill_perfctr(sh_sampler_cache *sampler, ub4 index, ub4 way, memory_trace *info)
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
static void sh_update_sampler_spill_reuse_perfctr(sh_sampler_cache *sampler, ub4 index, ub4 way, 
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

static void sh_update_sampler_spill_perfctr(sh_sampler_cache *sampler, ub4 index, ub4 way, 
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
static void sh_update_sampler_fill_reuse_perfctr(sh_sampler_cache *sampler, ub4 index, ub4 way,
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

/* Update fills for current reuse ep ch */
static void sh_update_sampler_fill_per_reuse_epoch_perfctr(sh_sampler_cache *sampler, 
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
static void sh_update_sampler_fill_reuse_per_reuse_epoch_perfctr(sh_sampler_cache *sampler, 
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
static void sh_update_sampler_change_fill_epoch_perfctr(sh_sampler_cache *sampler, ub4 index, 
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
    sh_update_sampler_fill_per_reuse_epoch_perfctr(sampler, index, way, info);
  }
}

static void sh_update_sampler_xstream_perfctr(sh_sampler_cache *sampler, ub4 index, ub4 way,
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

static void sh_sampler_cache_fill_block(sh_sampler_cache *sampler, ub4 index, ub4 way, 
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
    sh_update_sampler_fill_perfctr(sampler, index, way, info);
    sh_update_sampler_fill_per_reuse_epoch_perfctr(sampler, index, way, info);
  }
  else
  {
    sh_update_sampler_spill_perfctr(sampler, index, way, info);
  }
  
  sampler->perfctr.sampler_fill += 1;
}

static void sh_sampler_cache_access_block(sh_sampler_cache *sampler, ub4 index, ub4 way,
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
    sh_update_sampler_spill_perfctr(sampler, index, way, info);     
  }
  else
  {
    /* If previous access to sampler was a spill */
    if (sampler->blocks[index][way].spill_or_fill[offset] == TRUE)
    {
      /* Reset reuse-epoch */
      sampler->blocks[index][way].hit_count[offset] = 0;

      /* Increment spill reuse */
      sh_update_sampler_spill_reuse_perfctr(sampler, index, way, policy_data, info, update_time);

      /* Update dynamic stream flag */
      sh_update_sampler_xstream_perfctr(sampler, index, way, info);

      /* Increment fill reuse */
      sh_update_sampler_fill_per_reuse_epoch_perfctr(sampler, index, way, info);
    }
    else
    {
      /* Increment fill reuse */
      sh_update_sampler_fill_reuse_perfctr(sampler, index, way, policy_data, info, update_time);

      /* Increment fill reuse per reues epoch */
      sh_update_sampler_fill_reuse_per_reuse_epoch_perfctr(sampler, index, way, policy_data, info);
      
      /* Update fill reuse epoch, if epoch has changed, update fill count for
       * new epoch */
      sh_update_sampler_change_fill_epoch_perfctr(sampler, index, way, info);
    }
    
    /* Update fill count */
    sh_update_sampler_fill_perfctr(sampler, index, way, info);     
  }

  sampler->blocks[index][way].spill_or_fill[offset] = (info->spill ? TRUE : FALSE);
  sampler->blocks[index][way].stream[offset]        = info->stream;
  sampler->blocks[index][way].timestamp[offset]     = policy_data->miss_count;
  
  sampler->perfctr.sampler_hit += 1;
}

static void sh_sampler_cache_lookup(sh_sampler_cache *sampler, srriphint_data *policy_data, 
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
        sh_sampler_cache_fill_block(sampler, index, way, policy_data, info, 
            FALSE);    
      }
      else
      {
        /* If sampler access was a hit */
        sh_sampler_cache_access_block(sampler, index, way, policy_data, info, 
            TRUE);
      }
    }
  }
}

/* Initialize BRRIP for SRRIPHINT policy data */
static void cache_init_brrip_from_srrphint(struct cache_params *params, brrip_data *policy_data,
  srriphint_data *sh_policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  assert(sh_policy_data);

  /* Create per rrpv buckets */
  BRRIP_DATA_VALID_HEAD(policy_data) = SRRIPHINT_DATA_VALID_HEAD(sh_policy_data);
  BRRIP_DATA_VALID_TAIL(policy_data) = SRRIPHINT_DATA_VALID_TAIL(sh_policy_data);
  
  assert(BRRIP_DATA_VALID_HEAD(policy_data));
  assert(BRRIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  BRRIP_DATA_MAX_RRPV(policy_data)    = params->max_rrpv;
  BRRIP_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  /* Create array of blocks */
  BRRIP_DATA_BLOCKS(policy_data) = SRRIPHINT_DATA_BLOCKS(sh_policy_data);

  BRRIP_DATA_FREE_HLST(policy_data) = SRRIPHINT_DATA_FREE_HLST(sh_policy_data);
  BRRIP_DATA_FREE_TLST(policy_data) = SRRIPHINT_DATA_FREE_TLST(sh_policy_data);

  /* Set current and default fill policy to SRRIP */
  BRRIP_DATA_CFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DFPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DAPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_CRPOLICY(policy_data) = cache_policy_brrip;
  BRRIP_DATA_DRPOLICY(policy_data) = cache_policy_brrip;

  assert(BRRIP_DATA_MAX_RRPV(policy_data) != 0);
}

void cache_init_srriphint(ub4 set_indx, struct cache_params *params, srriphint_data *policy_data, 
    srriphint_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

  if (set_indx == 0)
  {
    for (ub1 s = NN; s <= TST; s++)
    {
      for (ub1 i = 0; i < EPOCH_COUNT; i++)
      {
        /* Initialize epoch fill counter */
        global_data->epoch_fctr   = NULL;
        global_data->epoch_hctr   = NULL;
        global_data->epoch_valid  = NULL;
      }

      global_data->fill_recall_table[s] = (ub8 *)xcalloc(RTBL_SIZE, sizeof(ub8));
      assert(global_data->fill_recall_table[s]);

      global_data->fill_recall_table_valid[s] = (ub1 *)xcalloc(RTBL_SIZE, sizeof(ub1));
      assert(global_data->fill_recall_table_valid[s]);

      global_data->spill_recall_table[s] = (ub8 *)xcalloc(RTBL_SIZE, sizeof(ub8));
      assert(global_data->spill_recall_table[s]);

      global_data->spill_recall_table_valid[s] = (ub1 *)xcalloc(RTBL_SIZE, sizeof(ub1));
      assert(global_data->spill_recall_table_valid[s]);

      global_data->total_fill_recall[s]  = 0;
      global_data->total_spill_recall[s] = 0;
    }

    global_data->ways         = params->ways;
    global_data->epoch_count  = EPOCH_COUNT;
    global_data->max_epoch    = MAX_EPOCH;

    /* Initialize per-stream speedup stream selection counter */
    for (ub1 i = 0; i <= TST; i++)
    {
      SAT_CTR_INI(global_data->sarp_hint[i], ECTR_WIDTH, ECTR_MIN_VAL, ECTR_MAX_VAL);
      SAT_CTR_SET(global_data->sarp_hint[i], ECTR_MIN_VAL);
    }

    /* Allocate and initialize sampler cache */
    global_data->sampler = (sh_sampler_cache *)xcalloc(1, sizeof(sh_sampler_cache));
    assert(global_data->sampler);  

    sh_sampler_cache_init(global_data->sampler, params->sampler_sets, params->sampler_ways);
  }

  policy_data->set_type = get_set_type_srriphint(set_indx);
  
  switch (policy_data->set_type)
  {
    case SRRIPHINT_BRRIP_SET:
      policy_data->following = cache_policy_srriphint;
      break;

    case SRRIPHINT_SRRIP_SET:
    case SRRIPHINT_POLICY_SET:
    case SRRIPHINT_REUSE_SET:
    case SRRIPHINT_FOLLOWER_SET:
      policy_data->following = cache_policy_srriphint;
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  SRRIPHINT_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SRRIPHINT_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC  

  assert(SRRIPHINT_DATA_VALID_HEAD(policy_data));
  assert(SRRIPHINT_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIPHINT_DATA_MAX_RRPV(policy_data)    = MAX_RRPV;
  SRRIPHINT_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;
  SRRIPHINT_DATA_USE_EPOCH(policy_data)   = FALSE;

  assert(params->spill_rrpv <= MAX_RRPV);

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SRRIPHINT_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    SRRIPHINT_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    SRRIPHINT_DATA_VALID_TAIL(policy_data)[i].head = NULL;
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
  
  /* Initialize BRRIP policy data */
  cache_init_brrip_from_srrphint(params, &(policy_data->brrip), policy_data);

  /* Set current and default fill policy to SRRIP */
  SRRIPHINT_DATA_CFPOLICY(policy_data) = cache_policy_srriphint;
  SRRIPHINT_DATA_DFPOLICY(policy_data) = cache_policy_srriphint;
  SRRIPHINT_DATA_CAPOLICY(policy_data) = cache_policy_srriphint;
  SRRIPHINT_DATA_DAPOLICY(policy_data) = cache_policy_srriphint;
  SRRIPHINT_DATA_CRPOLICY(policy_data) = cache_policy_srriphint;
  SRRIPHINT_DATA_DRPOLICY(policy_data) = cache_policy_srriphint;

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
}

static ub1 is_fill_block_pinned(srriphint_data *policy_data, srriphint_gdata *global_data, memory_trace *info)
{
  sh_sampler_perfctr *perfctr;

  perfctr = &((global_data->sampler)->perfctr);

  assert(SRRIPHINT_DATA_CFPOLICY(policy_data) == cache_policy_srriphint);

#define SAMPLE_MC(g)          ((g)->sample_miss)
#define POLICY_MC(g)          ((g)->policy_miss)
#define FLWR_SET(p)           ((p)->set_type == SRRIPHINT_FOLLOWER_SET)
#define PLCY_SET(p)           ((p)->set_type == SRRIPHINT_POLICY_SET)

  if ((FLWR_SET(policy_data) && global_data->speedup_stream[info->stream] && 
        (POLICY_MC(global_data) < SAMPLE_MC(global_data))) || 
      (PLCY_SET(policy_data) && global_data->speedup_stream[info->stream]))
  {
    return TRUE;
  }

#undef SAMPLE_MC
#undef POLICY_MC
#undef FLWR_SET
#undef PLCY_SET

  return FALSE;
}

struct cache_block_t* cache_find_block_srriphint(srriphint_data *policy_data, 
    srriphint_gdata *global_data, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SRRIPHINT_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

end:
  switch (policy_data->set_type)
  {
    case SRRIPHINT_SRRIP_SET:
      global_data->sample_fill += 1;
      break;

    case SRRIPHINT_POLICY_SET:
      global_data->policy_fill += 1;
      break;

    case SRRIPHINT_FOLLOWER_SET:
    case SRRIPHINT_REUSE_SET:
    case SRRIPHINT_BRRIP_SET:
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
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
    case cache_policy_brrip:
#if 0
      cache_fill_block_brrip(&(policy_data->brrip), &(global_data->brrip), way, tag, state, strm, info);
      break;
#endif
    case cache_policy_srriphint:

      if (way != BYPASS_WAY)
      {
        /* Obtain SRRIP specific data */
        block = &(SRRIPHINT_DATA_BLOCKS(policy_data)[way]);
        block->epoch  = 0;
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
          block->is_ct_block      = FALSE;
          block->is_bt_block      = FALSE;
          block->is_zt_block      = FALSE;
          block->is_block_pinned  = FALSE;
          block->is_proc_block    = FALSE;
          block->sap_stream       = info->sap_stream;
          block->fill_bucket      = global_data->current_fill_bucket;
          block->is_block_pinned  = is_fill_block_pinned(policy_data, global_data, info);

          /* Insert block in to the corresponding RRPV queue */
          CACHE_APPEND_TO_QUEUE(block, 
              SRRIPHINT_DATA_VALID_HEAD(policy_data)[rrpv], 
              SRRIPHINT_DATA_VALID_TAIL(policy_data)[rrpv]);

          switch (policy_data->set_type)
          {
            case SRRIPHINT_SRRIP_SET:
              global_data->sample_miss += 1;
              break;

            case SRRIPHINT_POLICY_SET:
              global_data->policy_miss += 1;
              break;

            case SRRIPHINT_REUSE_SET:
              global_data->reuse_miss += 1;
              break;

            case SRRIPHINT_BRRIP_SET:
            case SRRIPHINT_FOLLOWER_SET:
              break;

            default:
              panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
          }

          if (policy_data->set_type == SRRIPHINT_POLICY_SET)
          {
            if (info->fill)
            {
              global_data->stream_fill[info->stream] += 1;
            }
            else
            {
              assert(info->spill);

              global_data->stream_spill[info->stream] += 1;
            }
          }

          /* Update recall table entry */
          if (info->fill)
          {
            if (global_data->fill_recall_table_valid[info->stream][RCL_SIGN(info)] == TRUE)
            {
              global_data->fill_recall_table[info->stream][RCL_SIGN(info)] += 1;
              global_data->total_fill_recall[info->stream] += 1;
            }
          }
          else
          {
            assert(info->spill);

            if (global_data->spill_recall_table_valid[info->stream][RCL_SIGN(info)] == TRUE)
            {
              global_data->spill_recall_table[info->stream][RCL_SIGN(info)] += 1;
              global_data->total_spill_recall[info->stream] += 1;
            }
          }
        }
      }
    
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }
  
  /* Update sampler cache */
  sh_sampler_cache_lookup(global_data->sampler, policy_data, info, FALSE);

  if (info) 
  {
    srriphint_update_hint_count(global_data, info);
  }

  if (++(global_data->cache_access) >= INTERVAL_SIZE)
  {
    cache_update_interval_end(global_data);

    sh_sampler_cache_reset(global_data, global_data->sampler);

    printf("CT Reues %ld BT reues %ld\n", global_data->ct_reuse, global_data->bt_reuse);

    printf("Fill Count C    :%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->stream_fill[CS], 
        global_data->stream_fill[ZS], global_data->stream_fill[TS], global_data->stream_fill[BS],
        global_data->stream_fill[PS]);

    printf("Fill No Reuse C :%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->fill_no_reuse[CS], 
        global_data->fill_no_reuse[ZS], global_data->fill_no_reuse[TS], global_data->fill_no_reuse[BS],
        global_data->fill_no_reuse[PS]);

    printf("Spill Count C   :%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->stream_spill[CS], 
        global_data->stream_spill[ZS], global_data->stream_spill[TS], global_data->stream_spill[BS],
        global_data->stream_spill[PS]);

    printf("Spill No Reuse C:%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->spill_no_reuse[CS], 
        global_data->spill_no_reuse[ZS], global_data->spill_no_reuse[TS], global_data->spill_no_reuse[BS],
        global_data->spill_no_reuse[PS]);

    printf("Fill Count C    :%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->stream_fill[CS], 
        global_data->stream_fill[ZS], global_data->stream_fill[TS], global_data->stream_fill[BS],
        global_data->stream_fill[PS]);

    printf("Fill No Reuse C :%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->fill_no_reuse[CS], 
        global_data->fill_no_reuse[ZS], global_data->fill_no_reuse[TS], global_data->fill_no_reuse[BS],
        global_data->fill_no_reuse[PS]);

    printf("Spill Count C   :%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->stream_spill[CS], 
        global_data->stream_spill[ZS], global_data->stream_spill[TS], global_data->stream_spill[BS],
        global_data->stream_spill[PS]);

    printf("Spill No Reuse C:%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->spill_no_reuse[CS], 
        global_data->spill_no_reuse[ZS], global_data->spill_no_reuse[TS], global_data->spill_no_reuse[BS],
        global_data->spill_no_reuse[PS]);

    printf("Z BS evict %6ld Z PS evict %6ld\n", global_data->zero_evict_bucket_bs[TS], 
        global_data->zero_evict_bucket_ps[TS]);

    printf("XZ BS evict %5ld XZ PS evict %5ld\n", global_data->x_zero_evict_bucket_bs[TS], 
        global_data->x_zero_evict_bucket_ps[TS]);

    printf("N BS evict %6ld N PS evict %6ld\n", global_data->non_zero_evict_bucket_bs[TS], 
        global_data->non_zero_evict_bucket_ps[TS]);

    printf("XN BS evict %5ld XN PS evict %5ld\n", global_data->x_non_zero_evict_bucket_bs[TS], 
        global_data->x_non_zero_evict_bucket_ps[TS]);

    for (ub1 i = 0; i <= TST; i++)
    {
      global_data->stream_fill[i]         /= 2;
      global_data->stream_spill[i]        /= 2;
      global_data->fill_no_reuse[i]       /= 2;
      global_data->spill_no_reuse[i]      /= 2;
      global_data->ct_reuse               /= 2;
      global_data->bt_reuse               /= 2;
      global_data->sample_miss            /= 2;
      global_data->policy_miss            /= 2;
      global_data->reuse_miss             /= 2;
    }

    global_data->cache_access               = 0;
    global_data->current_fill_bucket       += 1;

    for (ub1 i = 0; i <= TST; i++)
    {
      global_data->non_zero_evict_bucket_bs[i]   = 0;
      global_data->non_zero_evict_bucket_ps[i]   = 0;
      global_data->zero_evict_bucket_bs[i]       = 0;
      global_data->zero_evict_bucket_ps[i]       = 0;
      global_data->x_non_zero_evict_bucket_bs[i] = 0;
      global_data->x_non_zero_evict_bucket_ps[i] = 0;
      global_data->x_zero_evict_bucket_bs[i]     = 0;
      global_data->x_zero_evict_bucket_ps[i]     = 0;

      /* Reset recall table */
      for (ub8 j = 0; j < RTBL_SIZE; j++)
      {
        global_data->fill_recall_table_valid[i][j]  = FALSE;
        global_data->fill_recall_table[i][j]        = 0;

        global_data->spill_recall_table_valid[i][j] = FALSE;
        global_data->spill_recall_table[i][j]       = 0;
      }

      global_data->total_fill_recall[i]  = 0;
      global_data->total_spill_recall[i] = 0;
    }
  }

  SRRIPHINT_DATA_CFPOLICY(policy_data) = SRRIPHINT_DATA_DFPOLICY(policy_data);
}

int cache_replace_block_srriphint(srriphint_data *policy_data, srriphint_gdata *global_data,
    memory_trace *info)
{
  struct  cache_block_t *block;
  sb4     rrpv;
  sb4     old_rrpv;
  sb4     min_rrpv;
  ub4     min_wayid;

  /* Remove a nonbusy block from the tail */
  min_wayid = ~(0);
  min_rrpv  = !SRRIPHINT_DATA_MAX_RRPV(policy_data);

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

  /* If fill is not bypassed, get the replacement candidate */
  struct  cache_block_t *head_block;

  /* Obtain RRPV from where to replace the block */
  rrpv = cache_get_replacement_rrpv_srriphint(policy_data);

  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= SRRIPHINT_DATA_MAX_RRPV(policy_data));

  head_block = SRRIPHINT_DATA_VALID_HEAD(policy_data)[rrpv].head;

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
      CACHE_SRRIPHINT_INCREMENT_RRPV(SRRIPHINT_DATA_VALID_HEAD(policy_data), 
          SRRIPHINT_DATA_VALID_TAIL(policy_data), rrpv);
    }

    /* Find pinned block at victim rrpv and move it to minimum RRPV */
    for (ub4 way_id = 0; way_id < global_data->ways; way_id++)
    {
      old_block = &SRRIPHINT_DATA_BLOCKS(policy_data)[way_id];
      old_rrpv  = ((rrip_list *)(old_block->data))->rrpv;

      if (old_rrpv == rrpv && old_block->is_block_pinned == TRUE)
      {
        old_block->is_block_pinned  = FALSE;

        /* Move block to min RRPV */
        CACHE_REMOVE_FROM_QUEUE(old_block, SRRIPHINT_DATA_VALID_HEAD(policy_data)[old_rrpv],
            SRRIPHINT_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(old_block, SRRIPHINT_DATA_VALID_HEAD(policy_data)[min_rrpv], 
            SRRIPHINT_DATA_VALID_TAIL(policy_data)[min_rrpv]);
      }
      else
      {
        if (old_rrpv == rrpv && old_block->is_block_pinned == FALSE)
        {
          break;
        }
      }
    }

    head_block = SRRIPHINT_DATA_VALID_HEAD(policy_data)[rrpv].head;
  }while(!head_block);

  /* If there is no block with required RRPV, increment RRPV of all the blocks
   * until we get one with the required RRPV */
  if (SRRIPHINT_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
  {
    CACHE_SRRIPHINT_INCREMENT_RRPV(SRRIPHINT_DATA_VALID_HEAD(policy_data), 
        SRRIPHINT_DATA_VALID_TAIL(policy_data), rrpv);
  }

  switch (SRRIPHINT_DATA_CRPOLICY(policy_data))
  {
    case cache_policy_srriphint:
      for (block = SRRIPHINT_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
      {
        if (!block->busy && block->way < min_wayid)
          min_wayid = block->way;
      }

      if (min_wayid != ~(0))
      {
        if (policy_data->blocks[min_wayid].access == 0)
        {
          if (policy_data->set_type == SRRIPHINT_POLICY_SET)
          {
            if (policy_data->blocks[min_wayid].dirty)
            {
              global_data->spill_no_reuse[policy_data->blocks[min_wayid].stream] += 1;
            }
            else
            {
              global_data->fill_no_reuse[policy_data->blocks[min_wayid].stream] += 1;
            }
          }

          if (policy_data->set_type == SRRIPHINT_SRRIP_SET && 
              policy_data->blocks[min_wayid].fill_bucket == global_data->current_fill_bucket)
          {
            global_data->zero_evict_bucket_bs[policy_data->blocks[min_wayid].stream] += 1;
          }

          if (policy_data->set_type == SRRIPHINT_POLICY_SET && 
              policy_data->blocks[min_wayid].fill_bucket == global_data->current_fill_bucket)
          {
            global_data->zero_evict_bucket_ps[policy_data->blocks[min_wayid].stream] += 1;
          }

          if (policy_data->set_type == SRRIPHINT_SRRIP_SET && 
              policy_data->blocks[min_wayid].fill_bucket != global_data->current_fill_bucket)
          {
            global_data->x_zero_evict_bucket_bs[policy_data->blocks[min_wayid].stream] += 1;
          }

          if (policy_data->set_type == SRRIPHINT_POLICY_SET && 
              policy_data->blocks[min_wayid].fill_bucket != global_data->current_fill_bucket)
          {
            global_data->x_zero_evict_bucket_ps[policy_data->blocks[min_wayid].stream] += 1;
          }

          /* Set recall table entry valid */
          if (info->fill)
          {
            if (global_data->fill_recall_table_valid[info->stream][RCL_SIGN(info)] == FALSE)
            {
              global_data->fill_recall_table_valid[info->stream][RCL_SIGN(info)] = TRUE;
            }
          }
          else
          {
            if (global_data->spill_recall_table_valid[info->stream][RCL_SIGN(info)] == FALSE)
            {
              global_data->spill_recall_table_valid[info->stream][RCL_SIGN(info)] = TRUE;
            }
          }
        }
        else
        {
          if (policy_data->set_type == SRRIPHINT_SRRIP_SET && 
              policy_data->blocks[min_wayid].fill_bucket == global_data->current_fill_bucket)
          {
            global_data->non_zero_evict_bucket_bs[policy_data->blocks[min_wayid].stream] += 1;
          }

          if (policy_data->set_type == SRRIPHINT_POLICY_SET && 
              policy_data->blocks[min_wayid].fill_bucket == global_data->current_fill_bucket)
          {
            global_data->non_zero_evict_bucket_ps[policy_data->blocks[min_wayid].stream] += 1;
          }

          if (policy_data->set_type == SRRIPHINT_SRRIP_SET && 
              policy_data->blocks[min_wayid].fill_bucket != global_data->current_fill_bucket)
          {
            global_data->x_non_zero_evict_bucket_bs[policy_data->blocks[min_wayid].stream] += 1;
          }

          if (policy_data->set_type == SRRIPHINT_POLICY_SET && 
              policy_data->blocks[min_wayid].fill_bucket != global_data->current_fill_bucket)
          {
            global_data->x_non_zero_evict_bucket_ps[policy_data->blocks[min_wayid].stream] += 1;
          }
        }
      }

      break;

    case cache_policy_cpulast:
      /* First try to find a GPU block */
      for (block = SRRIPHINT_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
      {
        if (!block->busy && (block->way < min_wayid && block->stream < TST))
          min_wayid = block->way;
      }

      /* If there so no GPU replacement candidate, replace CPU block */
      if (min_wayid == ~(0))
      {
        for (block = SRRIPHINT_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid))
            min_wayid = block->way;
        }
      }
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

end:
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
    case cache_policy_srriphint:
      /* Get old RRPV from the block */
      old_rrpv = (((rrip_list *)(blk->data))->rrpv);
      new_rrpv = old_rrpv;

      if (info->fill)
      {
#define MX_EP(g)  ((g)->max_epoch)

        blk->epoch  = (blk->epoch == MX_EP(global_data)) ? MX_EP(global_data) : blk->epoch + 1;

#undef MX_EP
      }
      else
      {
        blk->epoch = 0;
      }

      /* Get new RRPV using policy specific function */
      new_rrpv = cache_get_new_rrpv_srriphint(policy_data, global_data, info, 
          old_rrpv, blk, blk->epoch);
      
      /* Update block queue if block got new RRPV */
      if (new_rrpv != old_rrpv)
      {
        blk->last_rrpv = new_rrpv;

        CACHE_REMOVE_FROM_QUEUE(blk, SRRIPHINT_DATA_VALID_HEAD(policy_data)[old_rrpv],
            SRRIPHINT_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, SRRIPHINT_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            SRRIPHINT_DATA_VALID_TAIL(policy_data)[new_rrpv]);
      }
       
      blk->is_block_pinned = is_fill_block_pinned(policy_data, global_data, info);

      if (blk->stream == CS && strm == TS)
      {
        blk->is_ct_block = TRUE;
        global_data->ct_reuse += 1;
      }
      else
      {
        blk->is_ct_block = FALSE;
      }

      if (blk->stream == BS && strm == TS)
      {
        blk->is_bt_block = TRUE;
        global_data->bt_reuse += 1;
      }
      else
      {
        blk->is_bt_block = FALSE;
      }

      CACHE_UPDATE_BLOCK_STREAM(blk, strm);

      blk->dirty  |= (info && info->spill) ? TRUE : FALSE;
      blk->spill   = (info && info->spill) ? TRUE : FALSE;
      blk->access += 1;
      break;

    case cache_policy_bypass:
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

  sh_sampler_cache_lookup(global_data->sampler, policy_data, info, FALSE);

  if (info) 
  {
    srriphint_update_hint_count(global_data, info);
  }

  if (++(global_data->cache_access) >= INTERVAL_SIZE)
  {
    cache_update_interval_end(global_data);

    sh_sampler_cache_reset(global_data, global_data->sampler);

    printf("CT Reues %ld BT reues %ld\n", global_data->ct_reuse, global_data->bt_reuse);

    printf("Fill Count C    :%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->stream_fill[CS], 
        global_data->stream_fill[ZS], global_data->stream_fill[TS], global_data->stream_fill[BS],
        global_data->stream_fill[PS]);

    printf("Fill No Reuse C :%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->fill_no_reuse[CS], 
        global_data->fill_no_reuse[ZS], global_data->fill_no_reuse[TS], global_data->fill_no_reuse[BS],
        global_data->fill_no_reuse[PS]);

    printf("Spill Count C   :%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->stream_spill[CS], 
        global_data->stream_spill[ZS], global_data->stream_spill[TS], global_data->stream_spill[BS],
        global_data->stream_spill[PS]);

    printf("Spill No Reuse C:%6ld Z:%6ld T:%6ld B:%6ld P:%6ld\n", global_data->spill_no_reuse[CS], 
        global_data->spill_no_reuse[ZS], global_data->spill_no_reuse[TS], global_data->spill_no_reuse[BS],
        global_data->spill_no_reuse[PS]);

    printf("Z BS evict %6ld Z PS evict %6ld\n", global_data->zero_evict_bucket_bs[TS], 
        global_data->zero_evict_bucket_ps[TS]);

    printf("XZ BS evict %5ld XZ PS evict %5ld\n", global_data->x_zero_evict_bucket_bs[TS], 
        global_data->x_zero_evict_bucket_ps[TS]);

    printf("N BS evict %6ld N PS evict %6ld\n", global_data->non_zero_evict_bucket_bs[TS], 
        global_data->non_zero_evict_bucket_ps[TS]);

    printf("XN BS evict %5ld XN PS evict %5ld\n", global_data->x_non_zero_evict_bucket_bs[TS], 
        global_data->x_non_zero_evict_bucket_ps[TS]);

    for (ub1 i = 0; i <= TST; i++)
    {
      global_data->stream_fill[i]         /= 2;
      global_data->stream_spill[i]        /= 2;
      global_data->fill_no_reuse[i]       /= 2;
      global_data->spill_no_reuse[i]      /= 2;
      global_data->sample_miss            /= 2;
      global_data->policy_miss            /= 2;
      global_data->reuse_miss             /= 2;
    }

    global_data->cache_access               = 0;
    global_data->current_fill_bucket       += 1;

    for (ub1 i = 0; i <= TST; i++)
    {
      global_data->non_zero_evict_bucket_bs[i]   = 0;
      global_data->non_zero_evict_bucket_ps[i]   = 0;
      global_data->zero_evict_bucket_bs[i]       = 0;
      global_data->zero_evict_bucket_ps[i]       = 0;
      global_data->x_non_zero_evict_bucket_bs[i] = 0;
      global_data->x_non_zero_evict_bucket_ps[i] = 0;
      global_data->x_zero_evict_bucket_bs[i]     = 0;
      global_data->x_zero_evict_bucket_ps[i]     = 0;

      /* Reset recall table */
      for (ub8 j = 0; j < RTBL_SIZE; j++)
      {
        global_data->fill_recall_table_valid[i][j]  = FALSE;
        global_data->fill_recall_table[i][j]        = 0;

        global_data->spill_recall_table_valid[i][j] = FALSE;
        global_data->spill_recall_table[i][j]       = 0;
      }

      global_data->total_fill_recall[i]  = 0;
      global_data->total_spill_recall[i] = 0;
    }
  }
}

int cache_get_fill_rrpv_srriphint(srriphint_data *policy_data, 
    srriphint_gdata *global_data, memory_trace *info, ub4 epoch)
{
  sb4 ret_rrpv;
  sh_sampler_perfctr *perfctr;
  
  perfctr = &((global_data->sampler)->perfctr);

  switch (SRRIPHINT_DATA_CFPOLICY(policy_data))
  {
    case cache_policy_srriphint:

#define SAMPLE_MC(g)          ((g)->sample_miss)
#define POLICY_MC(g)          ((g)->policy_miss)
#define REUSE_MC(g)           ((g)->reuse_miss)
#define FLWR_SET(p)           ((p)->set_type == SRRIPHINT_FOLLOWER_SET)
#define PLCY_SET(p)           ((p)->set_type == SRRIPHINT_POLICY_SET)
#define REUSE_SET(p)          ((p)->set_type == SRRIPHINT_REUSE_SET)
#define FRCL_TH               (2)
#define SRCL_TH               (2)
#define FILL_RTBL(g, i)       ((g)->fill_recall_table[(i)->stream][RCL_SIGN(i)])
#define FILL_RECALL(g, i)     ((g)->total_fill_recall[(i)->stream])
#define SPILL_RTBL(g, i)      ((g)->spill_recall_table[(i)->stream][RCL_SIGN(i)])
#define SPILL_RECALL(g, i)    ((g)->total_fill_recall[(i)->stream])
#define IS_FILL_RCL(g, i)     (FILL_RTBL(g, i) && FILL_RTBL(g, i) > (3 * FILL_RECALL(g, i) / FRCL_TH))
#define IS_SPILL_RCL(g, i)    (SPILL_RTBL(g, i) && SPILL_RTBL(g, i) > (3 * SPILL_RECALL(g, i) / SRCL_TH))
#define F_NRUSE(g, i)         ((g)->fill_no_reuse[(i)->stream])
#define FILL_COUNT(g, i)      ((g)->stream_fill[(i)->stream])
#define IS_FILL_REUSE(g, i)   (F_NRUSE(g, i) > FILL_COUNT(g, i) / 2)
#define S_NRUSE(g, i)         ((g)->spill_no_reuse[(i)->stream])
#define SPILL_COUNT(g, i)     ((g)->stream_spill[(i)->stream])
#define IS_SPILL_REUSE(g, i)  (S_NRUSE(g, i) > SPILL_COUNT(g, i) / 2)
#define FRUSE(p, s)           (SMPLRPERF_FREUSE(p, s))
#define SRUSE(p, s)           (SMPLRPERF_SREUSE(p, s))
#define MRUSE(p)              (SMPLRPERF_MREUSE(p))
#define FRRPV_CHK(p, s)       ((FRUSE(p, s) > MRUSE(p) / 3))
#define SRRPV_CHK(p, s)       ((SRUSE(p, s) > MRUSE(p) / 3))
#define FOLLOW_POLICY(g)      (POLICY_MC(g) < SAMPLE_MC(g) && POLICY_MC(g) < REUSE_MC(g))

      if ((FLWR_SET(policy_data) && global_data->speedup_stream[info->stream] && 
            FOLLOW_POLICY(global_data)) || 
          (PLCY_SET(policy_data) && global_data->speedup_stream[info->stream]))
      {
        if (info->fill)
        {
          return SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;
        }
        else
        {
          return !SRRIPHINT_DATA_MAX_RRPV(policy_data);
        }
      }
      else
      {
        switch (policy_data->set_type)
        {
          case SRRIPHINT_SRRIP_SET:
          case SRRIPHINT_BRRIP_SET:
          case SRRIPHINT_POLICY_SET:
            return SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;

          case SRRIPHINT_REUSE_SET:
            if (info->fill)
            {
              return SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;
            }
            else
            {
              if (SRRPV_CHK(perfctr, info->stream))
              {
                return !SRRIPHINT_DATA_MAX_RRPV(policy_data);
              }
              else
              {
                return SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;
              }
            }

            break;

          case SRRIPHINT_FOLLOWER_SET:
            if (REUSE_MC(global_data) < POLICY_MC(global_data) && REUSE_MC(global_data) < SAMPLE_MC(global_data))
            {
              if (info->fill)
              {
                return SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;
              }
              else
              {
                assert(info->spill);

                if (SRRPV_CHK(perfctr, info->stream))
                {
                  return !SRRIPHINT_DATA_MAX_RRPV(policy_data);
                }
                else
                {
                  return SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;
                }
              }
            }
            else
            {
              return SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;
            }

            break;

          default:
            panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
        }
      }

#undef SAMPLE_MC
#undef POLICY_MC
#undef FLWR_SET
#undef PLCY_SET
#undef REUSE_SET
#undef FRCL_TH
#undef SRCL_TH
#undef FILL_RTBL
#undef FILL_RECALL
#undef SPILL_RTBL
#undef SPILL_RECALL
#undef IS_FILL_RCL
#undef IS_SPILL_RCL
#undef F_NRUSE
#undef FILL_COUNT
#undef IS_FILL_REUSE
#undef S_NRUSE
#undef SPILL_COUNT
#undef IS_SPILL_REUSE
#undef FRUSE
#undef SRUSE
#undef MRUSE
#undef FRRPV_CHK
#undef SRRPV_CHK
    
      break;

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
#define RRPV1(p, s)       ((SRUSE(p, s) > MRUSE(p) / 2))
#define CT_TH1            (64)
#define BT_TH1            (32)
#define TH2               (2)

int cache_get_new_rrpv_srriphint(srriphint_data *policy_data, srriphint_gdata *global_data, 
    memory_trace *info, sb4 old_rrpv, struct cache_block_t *block, ub4 epoch)
{
  sb4 ret_rrpv;
  int strm;

  sh_sampler_perfctr *perfctr;
  
  perfctr = &((global_data->sampler)->perfctr);
  strm    = NEW_STREAM(info);

  ret_rrpv = (info && !(info->fill)) ? old_rrpv : 0;

#define SMPL_SET(p) (policy_data->set_type == SRRIPHINT_SRRIP_SET)
#define PLCY_SET(p) (policy_data->set_type == SRRIPHINT_POLICY_SET)
  
  if (info->spill == TRUE)
  {
    if (RRPV1(perfctr, strm) && old_rrpv >= 3)
    {
      ret_rrpv = 0;
    }
    else
    {
      ret_rrpv = ret_rrpv;
    }
  }
  else
  {
    assert(info->fill == TRUE);
#if 0
    if (GPU_STREAM(info->stream))
    {
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
          if (FRUSE(perfctr, strm, epoch) * CT_TH1 <= FCOUNT(perfctr, strm, epoch))
          {
            ret_rrpv = 3;
          }
        }
      }
    }
#endif
  }

#undef SMPL_SET
#undef PLCY_SET

end:

  return ret_rrpv;
}

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
  CACHE_REMOVE_FROM_QUEUE(block, SRRIPHINT_DATA_VALID_HEAD(policy_data)[old_rrpv],
      SRRIPHINT_DATA_VALID_TAIL(policy_data)[old_rrpv]);
  CACHE_APPEND_TO_SQUEUE(block, SRRIPHINT_DATA_FREE_HEAD(policy_data), 
      SRRIPHINT_DATA_FREE_TAIL(policy_data));

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

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SRRIPHINT_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);
      if (node->stream == strm)
        count++;
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

#undef INTERVAL_SIZE
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
#undef SRRIPHINT_BRRIP_SET
#undef SRRIPHINT_POLICY_SET
#undef SRRIPHINT_FOLLOWER_SET
