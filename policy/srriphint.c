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

#define THROTTLE_ACTIVE           (FALSE)

#define RPL_GPU_FIRST             (0)
#define RPL_GPU_COND_FIRST        (1)
#define FILL_TEST_SHIP            (2)
#define FILL_TEST_SRRIP           (3)

#define CACHE_SET(cache, set)     (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)     (&((set)->blocks[way]))
#define BYPASS_RRPV               (-1)

#define INTERVAL_SIZE             (1 << 14)
#define EPOCH_SIZE                (1 << 19)
#if 0
#define EPOCH_SIZE                (1 << 21)
#endif
#define PAGE_COVERAGE             (8)
#define PSEL_WIDTH                (20)
#define PSEL_MIN_VAL              (0x00)  
#define PSEL_MAX_VAL              (0xfffff)  
#define PSEL_MID_VAL              (1 << 19)

#define RCTR_WIDTH                (12)
#define RCTR_MIN_VAL              (0)
#define RCTR_MAX_VAL              (0xfff)
#define RCTR_MID_VAL              (0x7ff)

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
#define NEW_STREAM(i)             (KNOWN_STREAM((i)->stream) ? (i)->stream : (i)->stream)
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

#if 0
#define RID(a)                    ((a) & 0x0ffff000)

#define RID(a)                    ((a) & 0xfffffff000)
#endif

#define RID(a)                    ((a) & 0xffffff0000)

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
#define SIGNSIZE(g)     ((g)->sign_size)
#define SHCTSIZE(g)     ((g)->shct_size)
#define COREBTS(g)      ((g)->core_size)
#define SB(g)           ((g)->sign_source == USE_MEMPC)
#define SP(g)           ((g)->sign_source == USE_PC)
#define SM(g)           ((g)->sign_source == USE_MEM)
#define SIGMAX_VAL(g)   (((1 << SIGNSIZE(g)) - 1))
#define SIGNMASK(g)     (SIGMAX_VAL(g) << SHCTSIZE(g))
#define SIGNP(g, i)     (((i)->pc) & SIGMAX_VAL(g))
#define SIGNM(g, i)     ((((i)->address >> SHCTSIZE(g)) & SIGMAX_VAL(g)))
#define SIGNCORE(g, i)  (((i)->core - 1) << (SHCTSIZE(g) - COREBTS(g)))
#define GET_SSIGN(g, i) ((i)->stream < PS ? SIGNM(g, i) : SIGNP(g, i) ^ SIGNCORE(g, i))
#define GET_SIGN(g, i)  (SM(g) ? SIGNM(g, i) : SIGNP(g, i))
#define SHIPSIGN(g, i)  (SB(g) ? GET_SSIGN(g, i) : GET_SIGN(g, i))

#define CACHE_INC_SHCT(block, g)                              \
do                                                            \
{                                                             \
  assert(block->ship_sign <= SIGMAX_VAL(g));                  \
                                                              \
  if ((g)->ship_shct[block->ship_sign] < SIGMAX_VAL(g))       \
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

int max_dead_time = 6;
int dead_times[6] = {15, 5, 2, 0, 0, 0};
int fake_counter;

void shnt_sampler_cache_lookup(srriphint_gdata *global_data, srriphint_data *policy_data, 
    memory_trace *info, ub1 update_time);

// make a trace from a PC (just extract some bits)
unsigned int sdbp_make_trace (sdbp_sampler *sampler, ub4 tid, sdbp_predictor *pred, ub8 PC) 
{
	return PC & ((1 << sampler->dan_sampler_trace_bits)-1);
}

static ub4 get_set_type_srriphint(long long int indx)
{
  return SRRIPHINT_SRRIP_SET;
}

/* Get expected age and update the block. 5 bits of the age are 
 * taken from sequence number learned for PCs. 5 bits are taken 
 * from block address */
static ub8 get_expected_age(srriphint_gdata *global_data, ub8 address, ub8 pc)
{
  cs_qnode           *region_table;
  shnt_region_data   *region_data;
  shnt_region_pc     *region_pc;
  shnt_pc_data       *pc_data;
  ub8                 pc_age; 
  ub8                 block_age; 
  ub8                 final_age;

  pc_age    = 0;
  block_age = 0;
  final_age = 0xdead;

#define GET_AGE(p, b)       (((p) << 12) | (b))
#define RPHASE(g)           ((g)->sampler->current_reuse_pahse)
#define RNPHASE(g)          ((RPHASE(g) + 1) % RPHASE_CNT)
#define USE_SREGION(g)      (!SAT_CTR_VAL(g->sampler->region_usage))
#define SDISTANCE(r, g, p)  ((r)->distance[(p)])
#define LDISTANCE(r, g, p)  ((r)->distance[(p)])
#define RDISTANCE(r, g, p)  (USE_SREGION(g) ? SDISTANCE(r, g, p) : LDISTANCE(r, g, p))

  if (pc == 134534192 || pc == 134513744 || pc == 134615973 || pc == 134616026 || 
    pc == 134615914 || pc == 134614624 || pc == 134515647 || pc == 134515657)
  {
    assert(1);
#if 0
    printf("%d\n", pc);
#endif
  }

  pc_data = (shnt_pc_data *)attila_map_lookup(global_data->sampler->all_pc_list, pc, ATTILA_MASTER_KEY);

  if (pc_data && pc_data->end_pc <= global_data->sampler->pc_distance)
  {
    pc_age = 0xfff; 
  }
  else
  {
    region_table = global_data->sampler->all_region_list;

    region_data = (shnt_region_data *)attila_map_lookup(region_table, RID(address), 
        ATTILA_MASTER_KEY);

#define PC_NOT_DEAD(g, p) ((p) && (((p)->start_pc + (p)->dead_distance) >= (g)->sampler->pc_distance))
#define REGION_LIVE(g, r) ((r)->dead_limit >= (g)->sampler->pc_distance)
#define PC_IS_LIVE(g, p)  ((p) && ((p)->dead_limit >= (g)->sampler->pc_distance))
#define PC_LIST(g, p)     (((p) && (p)->dead_distance < 50 && (p)->dead_distance > 1 && PC_NOT_DEAD(g, p) && !PC_IS_LIVE(g, p)))
#define PC_LIVE(g, p)     (PC_LIST(g, p)? PC_NOT_DEAD(g, p) : PC_IS_LIVE(g, p))

#define IS_LIVE(g, r, p)  (REGION_LIVE(g, r) || PC_LIVE(g, p))

    if (region_data && IS_LIVE(global_data, region_data, pc_data))
    {
      if (pc_data && PC_LIVE(global_data, pc_data))
      {
        if (PC_LIST(global_data, pc_data))
        {
          assert(1);
          fake_counter += 1;
        }
        else
        {
          assert(1);
          fake_counter += 1;
        }

        pc_age = 0x00f;
      }
      else
      {
        region_pc = (shnt_region_pc *)attila_map_lookup(region_data->pc, pc, ATTILA_MASTER_KEY);
        if (region_pc && region_pc->distance[RNPHASE(global_data)])
        {
          if (RDISTANCE(region_pc, global_data, RNPHASE(global_data)) != 0xfffff)
          {
            pc_age = RDISTANCE(region_pc, global_data, RNPHASE(global_data)) & 0xfff; 
          }
          else
          {
            pc_age = RDISTANCE(region_pc, global_data, RNPHASE(global_data)) & 0xfff; 
          }
        }
        else
        {
          if (IS_LIVE(global_data, region_data, pc_data))
          {
            pc_age = RDISTANCE(region_data, global_data, RNPHASE(global_data)) & 0xfff; 
          }
          else
          {
            pc_age = 0xfff;
          }
        }
      }
    }
    else
    {
      pc_age = 0xfff; 
    }

#undef REGION_LIVE
#undef PC_LIVE
#undef IS_LIVE
  }

  block_age = (address >> 6) & 0xff;
  final_age = GET_AGE(pc_age, block_age);
  
  return final_age;

#undef GET_AGE
#undef RPHASE
}

static void increment_dead_limit(srriphint_gdata *global_data, ub8 old_pc, 
    ub8 pc, ub8 reuse_seen)
{
  shnt_pc_data  *pc_data;

  pc_data = (shnt_pc_data *)attila_map_lookup(global_data->sampler->all_pc_list, old_pc, 
      ATTILA_MASTER_KEY);

#define RDSTNCE(g)  ((g)->sampler->pc_distance)

  if (pc_data && pc_data->dead_limit < RDSTNCE(global_data))
  {
    pc_data->dead_limit = RDSTNCE(global_data);
    pc_data->reuse_seen = reuse_seen;

    assert(RDSTNCE(global_data) >= pc_data->start_pc);

    pc_data->dead_distance = RDSTNCE(global_data) - pc_data->start_pc + 10;
  }

#undef RDSTNCE
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
      (sampler->blocks)[i][j].pc            = (ub8 *)xcalloc(BLK_PER_ENTRY, sizeof(ub8));
      (sampler->blocks)[i][j].dynamic_color = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_depth = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_blit  = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));
      (sampler->blocks)[i][j].dynamic_proc  = (ub1 *)xcalloc(BLK_PER_ENTRY, sizeof(ub1));

      for (ub4 off = 0; off < BLK_PER_ENTRY; off++)
      {
        (sampler->blocks)[i][j].pc[off] = 0xdead;
      }
    }
  }
  
  /* Initialize sampler performance counters */
  memset(&(sampler->perfctr), 0 , sizeof(srriphint_sampler_perfctr));
  memset(sampler->stream_occupancy, 0 , sizeof(ub4) * (TST + 1));
  
  sampler->spc_count          = 0;
  sampler->pc_distance        = 0;
  sampler->reuse_phase_size   = 0;
  sampler->reuse_phase_count  = 0;

  for (ub4 i = 0; i < HTBLSIZE; i++)
  {
    cs_qinit(&(sampler->spc_list[i]));
    cs_qinit(&(sampler->all_pc_list[i]));
    cs_qinit(&(sampler->all_region_list[i]));
  }

  /* Initialize replacement policy selection counter */
  SAT_CTR_INI(sampler->region_usage, RCTR_WIDTH, RCTR_MIN_VAL, RCTR_MAX_VAL);
  SAT_CTR_SET(sampler->region_usage, RCTR_MID_VAL);
}

void shnt_sampler_cache_reset(srriphint_gdata *global_data, shnt_sampler_cache *sampler)
{
  int sampler_occupancy;

  assert(sampler);

  sampler_occupancy = 0;

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
        (sampler->blocks)[i][j].pc[off]             = 0xdead;
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

#undef MAX_RRPV
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
    global_data->threshold  = params->threshold;

    if (params->ship_use_pc && params->ship_use_mem)
    {
      global_data->sign_source = USE_MEMPC;
    }
    else
    {
      if (params->ship_use_pc)
      {
        global_data->sign_source = USE_PC;
      }
      else
      {
        global_data->sign_source = USE_MEM;
      }
    }

    for (int i = NN; i <= TST; i++)
    {
      global_data->stream_blocks[i] = 0;
      global_data->stream_reuse[i]  = 0;
    }
    
    global_data->dbp_sampler = (sdbp_sampler *)xcalloc(1, sizeof(sdbp_sampler));
    assert(global_data->dbp_sampler);
    
    sdbp_sampler_init(global_data->dbp_sampler, params->num_sets, params->ways);

    /* Allocate and initialize sampler cache */
    global_data->sampler = (shnt_sampler_cache *)xcalloc(1, sizeof(shnt_sampler_cache));
    assert(global_data->sampler);  

    shnt_sampler_cache_init(global_data->sampler, params->sampler_sets, 
        params->sampler_ways);

    global_data->cpu_rpsel    = 0;
    global_data->gpu_rpsel    = 0;
    global_data->cpu_zevct    = 0;
    global_data->gpu_zevct    = 0;
    global_data->cpu_blocks   = 0;
    global_data->gpu_blocks   = 0;
    global_data->dead_region  = 0;

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

  policy_data->set_indx       = set_indx;
  policy_data->set_type       = get_set_type_srriphint(set_indx);
  policy_data->cpu_blocks     = 0;
  policy_data->gpu_blocks     = 0;
  policy_data->shared_blocks  = 0;
  
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

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create RRPV buckets */
  SRRIPHINT_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(INVALID_RRPV + 1);
  SRRIPHINT_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(INVALID_RRPV + 1);
  
#undef MEM_ALLOC  

  assert(SRRIPHINT_DATA_VALID_HEAD(policy_data));
  assert(SRRIPHINT_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIPHINT_DATA_MAX_RRPV(policy_data)    = MAX_RRPV;
  SRRIPHINT_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  assert(params->spill_rrpv <= MAX_RRPV);

  /* Initialize head nodes */
  for (int i = 0; i <= INVALID_RRPV; i++)
  {
    SRRIPHINT_DATA_VALID_HEAD(policy_data)[i].rrpv  = i;
    SRRIPHINT_DATA_VALID_HEAD(policy_data)[i].head  = NULL;
    SRRIPHINT_DATA_VALID_TAIL(policy_data)[i].head  = NULL;
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

#define CORE_COUNT(g)  ((1 << (g)->core_size) + 1)

  global_data->brrip_ctr = xcalloc(CORE_COUNT(global_data), sizeof(ub1));

#undef CORE_COUNT

  assert(SRRIPHINT_DATA_MAX_RRPV(policy_data) != 0);

#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_srriphint(ub4 set_indx, srriphint_data *policy_data, srriphint_gdata *global_data)
{
  cs_qnode *pc_table;
  cs_qnode *region_table;
  cs_qnode *head;
  cs_qnode *head1;
  cs_qnode *node;
  cs_qnode *node1;
  cs_knode *knode;
  cs_knode *knode1;

  shnt_pc_data *pc_data;
  shnt_d_data  *d_data;

  shnt_region_data *region_data;

  if (set_indx == 0)
  {
    global_data->shared_pc_file = gzopen("Shared-PC-list.csv.gz", "wb9");
    assert(global_data->shared_pc_file);

    pc_table = global_data->sampler->all_pc_list;

    /* Dump statistics */
    for (ub8 i = 0; i < HTBLSIZE; i++)
    {
      head = &pc_table[i];

      /* Iterate through each bucket and dump the PC */  
      for (node = head->next; node != head; node = node->next)
      {
        knode = (cs_knode *)(node->data);

        pc_data = (shnt_pc_data *)(knode->data);
        assert(pc_data);

        gzprintf(global_data->shared_pc_file, "(%ld;%ld;)", knode->key, pc_data->start_pc);

        for (ub8 i = 0; i < HTBLSIZE; i++)
        {
          head1 = &(pc_data->ppc[i]);
          for (node1 = head1->next; node1 != head1; node1 = node1->next)
          {
            knode1 = (cs_knode *)(node1->data);

            d_data = (shnt_d_data *)(knode1->data);
            gzprintf(global_data->shared_pc_file, ";(%ld; %ld; %ld)", d_data->ppc, 
                d_data->distance, d_data->reuse);
          }
        }

        gzprintf(global_data->shared_pc_file, "\n");
      }
    }

    gzclose(global_data->shared_pc_file);

    global_data->shared_pc_file = gzopen("Shared-Region-list.csv.gz", "wb9");
    assert(global_data->shared_pc_file);
    
    region_table = global_data->sampler->all_region_list;

    shnt_region_pc *region_pc;

    /* Dump statistics */
    for (ub8 i = 0; i < HTBLSIZE; i++)
    {
      head = &region_table[i];

      /* Iterate through each bucket and dump the PC */  
      for (node = head->next; node != head; node = node->next)
      {
        knode = (cs_knode *)(node->data);

        region_data = (shnt_region_data *)(knode->data);
        assert(region_data);

        gzprintf(global_data->shared_pc_file, "(%ld;)", knode->key);

        for (ub8 i = 0; i < HTBLSIZE; i++)
        {
          head1 = &(region_data->pc[i]);
          for (node1 = head1->next; node1 != head1; node1 = node1->next)
          {
            knode1 = (cs_knode *)(node1->data);
            region_pc = (shnt_region_pc *)knode1->data;

#define RPHASE(g)   ((g)->sampler->current_reuse_pahse)
#define RNPHASE(g)  ((RPHASE(g) + 1) % RPHASE_CNT)

            gzprintf(global_data->shared_pc_file, ";(%ld; %ld; %ld);", region_pc->pc,
                region_pc->distance[RNPHASE(global_data)], region_pc->distance[RPHASE(global_data)]);

#undef RPHASE
#undef RNPHASE
          }
        }

        gzprintf(global_data->shared_pc_file, "\n");
      }
    }

    gzclose(global_data->shared_pc_file);
  }

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

struct cache_block_t* cache_find_block_srriphint(srriphint_data *policy_data, 
    srriphint_gdata *global_data, memory_trace *info, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  if (info && info->fill && info->pc)
  {
    shnt_pc_data      *pc_data;
    shnt_region_data  *region_data;
    shnt_region_pc    *region_pc;
    
    if (!SAT_CTR_VAL(global_data->sampler->region_usage))
    {
      global_data->dead_region += 1;
    }

    pc_data = attila_map_lookup(global_data->sampler->all_pc_list, info->pc, ATTILA_MASTER_KEY);
    if (!pc_data)
    {
      pc_data = xcalloc(1, sizeof(shnt_pc_data));
      assert(pc_data);

      pc_data->start_pc       = ++(global_data->sampler->pc_distance);
      pc_data->end_pc         = 0xffffffff;
      pc_data->distance       = 0;
      pc_data->region_count   = 0;
      pc_data->dead_limit     = 0;
      pc_data->dead_distance  = 0;
      pc_data->reuse_seen     = 0;

      for (ub4 i = 0; i < HTBLSIZE; i++)
      {
        cs_qinit(&(pc_data->ppc[i]));
      }
      
      cs_qinit(&(pc_data->distance_list));

      attila_map_insert(global_data->sampler->all_pc_list, info->pc, ATTILA_MASTER_KEY, pc_data);
    }
    else
    {
      pc_data->start_pc = global_data->sampler->pc_distance;
    }

    region_data = attila_map_lookup(global_data->sampler->all_region_list, RID(info->address), ATTILA_MASTER_KEY);
    if (!region_data)
    {
      region_data = (shnt_region_data *)xcalloc(1, sizeof(shnt_region_data));
      assert(region_data);
      
      for (ub4 i = 0; i < HTBLSIZE; i++)
      {
        cs_qinit(&(region_data->pc[i]));
      }

#define RPHASE(g)   ((g)->sampler->current_reuse_pahse)
#define RNPHASE(g)  ((RPHASE(g) + 1) % RPHASE_CNT)
#define RDSTNCE(g)  ((g)->sampler->pc_distance)

      region_data->id = RID(info->address);
      region_data->distance[RPHASE(global_data)]        = RDSTNCE(global_data);
      region_data->short_distance[RPHASE(global_data)]  = RDSTNCE(global_data);
      region_data->next_distance                        = 0;
      region_data->short_next_distance                  = 0;
      region_data->distance[RNPHASE(global_data)]       = 0;
      region_data->short_distance[RNPHASE(global_data)] = 0;
      region_data->pc_count                             = 1;
#if 0
      region_data->dead_limit = RDSTNCE(global_data) + 10;
#endif
      region_data->dead_limit = RDSTNCE(global_data) + 0xfffff;


      shnt_region_pc *region_pc;

      region_pc = (shnt_region_pc *)xcalloc(1, sizeof(shnt_region_pc));
      assert(region_pc);
      
      region_pc->pc       = info->pc;
      region_pc->index    = region_data->pc_count;
      region_pc->phase_id = global_data->sampler->reuse_phase_count;

      region_pc->distance[RPHASE(global_data)]  = global_data->sampler->pc_distance;
      region_pc->distance[RNPHASE(global_data)] = 0xfffff;

      region_pc->short_distance[RPHASE(global_data)]  = global_data->sampler->pc_distance;
      region_pc->short_distance[RNPHASE(global_data)] = 0;
      
      attila_map_insert(region_data->pc, info->pc, ATTILA_MASTER_KEY, (ub8)region_pc);

      attila_map_insert(global_data->sampler->all_region_list, region_data->id,
          ATTILA_MASTER_KEY, (ub8)region_data);
    
#undef RPHASE
#undef RNPHASE
#undef RDSTNCE
      
      assert(pc_data);
      pc_data->region_count += 1;
    }
    else
    {
#define RPHASE(g)   ((g)->sampler->current_reuse_pahse)
#define RNPHASE(g)  ((RPHASE(g) + 1) % RPHASE_CNT)
#define RDSTNCE(g)  ((g)->sampler->pc_distance)
        
      if (region_data->distance[RPHASE(global_data)] == 0xfffff)
      {
        region_data->distance[RPHASE(global_data)] = RDSTNCE(global_data);
      }
      else
      {
        region_data->next_distance        = RDSTNCE(global_data);
        region_data->short_next_distance  = RDSTNCE(global_data);
      }

      if (region_data->short_distance[RPHASE(global_data)] == 0xfffff)
      {
        region_data->short_distance[RPHASE(global_data)] = RDSTNCE(global_data);
      }

#if 0
      if (info->pc == 134534192 || info->pc == 134513744 || info->pc == 134615973 || info->pc == 134616026 || 
          info->pc == 134615914 || info->pc == 134614624 || info->pc == 134515647 || info->pc == 134515657)
      {
#if 0
        region_data->dead_limit = RDSTNCE(global_data);
#endif
        region_data->dead_limit = RDSTNCE(global_data) + 1;
      }
      else
      {
        if (info->pc == 134534209)
        {
          region_data->dead_limit = RDSTNCE(global_data) + 10;
        }
        else
        {
          if (info->pc == 134513789)
          {
            region_data->dead_limit = RDSTNCE(global_data) + 110;
          }
          else
          {
            if (info->pc == 134615923)
            {
              region_data->dead_limit = RDSTNCE(global_data) + 200;
            }
            else
            {
              if (info->pc == 134599541)
              {
                region_data->dead_limit = RDSTNCE(global_data) + 90;
              }
              else
              {
                if (info->pc == 134614487)
                {
                  region_data->dead_limit = RDSTNCE(global_data) + 10;
                }
                else
                {
                  region_data->dead_limit = RDSTNCE(global_data) + 50;
                }
              }
            }
          }
        }
      }
#endif
      
      region_pc = attila_map_lookup(region_data->pc, info->pc, ATTILA_MASTER_KEY);
      if (!region_pc)
      {
        region_data->pc_count += 1;

        region_pc = (shnt_region_pc *)xcalloc(1, sizeof(shnt_region_pc));
        assert(region_pc);

        region_pc->pc       = info->pc;
        region_pc->index    = region_data->pc_count;
        region_pc->phase_id = global_data->sampler->reuse_phase_count;

        region_pc->distance[RPHASE(global_data)]        = global_data->sampler->pc_distance;
        region_pc->short_distance[RPHASE(global_data)]  = global_data->sampler->pc_distance;
#if 0
        region_pc->distance[RNPHASE(global_data)]       = 0xfffff;
        region_pc->short_distance[RNPHASE(global_data)] = 0xfffff;
#endif
        pc_data->region_count += 1;

        attila_map_insert(region_data->pc, info->pc, ATTILA_MASTER_KEY, (ub8)region_pc);
      }
#if 0
      else
      {
        region_pc->distance[RPHASE(global_data)] = global_data->sampler->pc_distance;
      }
#endif

      if (region_pc->index > max_dead_time)
      {
        region_data->dead_limit = RDSTNCE(global_data);
      }
      else
      {
        if (region_pc->index == 1)
        {
          region_data->dead_limit = RDSTNCE(global_data) + 15;
        }
        else
        {
          region_data->dead_limit = RDSTNCE(global_data) + dead_times[region_pc->index - 1];
        }
      }

#undef RPHASE
#undef RNPHASE
#undef RDSTNCE
    }
    
#define RPHASE(g)   ((g)->sampler->current_reuse_pahse)
#define RNPHASE(g)  ((RPHASE(g) + 1) % RPHASE_CNT)

    /* If region was not used in the previous phase decrement to region usage
     * counter */
    if (region_data)
    {
      if (region_data->distance[RNPHASE(global_data)] == 0xfffff || 
          region_data->distance[RNPHASE(global_data)] == 0)
      {
        SAT_CTR_DEC(global_data->sampler->region_usage); 
      }
      else
      {
        SAT_CTR_INC(global_data->sampler->region_usage); 
      }
    }

#undef RPHASE
#undef RNPHASE

    shnt_pc_data *dead_pc;
#if 0    
    if (info->pc == 134514245)
    {
      dead_pc = attila_map_lookup(global_data->sampler->all_pc_list, 134513789, ATTILA_MASTER_KEY);
      if (dead_pc)
      {
        dead_pc->end_pc = global_data->sampler->pc_distance;
      }
    }
#endif

    if (++(global_data->sampler->reuse_phase_size) == RPHASE_SIZE)
    {
#define RPHASE(g) ((g)->sampler->current_reuse_pahse)
#define RNPHASE(g)  ((RPHASE(g) + 1) % RPHASE_CNT)

      /* Reset old reuse distance learned in the previous phase */
      cs_qnode *region_table;
      cs_qnode *head;
      cs_qnode *node;
      cs_knode *knode;
      cs_qnode *head1;
      cs_qnode *node1;
      cs_knode *knode1;
      
      shnt_region_data *region_data;
      shnt_region_pc   *region_pc;
      shnt_pc_data     *pc_data;

      region_table = global_data->sampler->all_region_list;

      RPHASE(global_data) = (RPHASE(global_data) + 1) % RPHASE_CNT;

      /* Dump statistics */
      for (ub8 i = 0; i < HTBLSIZE; i++)
      {
        head = &region_table[i];

        /* Iterate through each bucket and dump the PC */  
        for (node = head->next; node != head; node = node->next)
        {
          knode = (cs_knode *)(node->data);

          region_data = (shnt_region_data *)(knode->data);
          assert(region_data);
          
          ub8 max_region = 0;

          /* Iterate through PC list */
          for (ub8 j = 0; j < HTBLSIZE; j++)
          {
            head1 = &(region_data->pc[j]);
            assert(head1);

            for (node1 = head1->next; node1 != head1; node1 = node1->next)
            {
              knode1 = (cs_knode *)(node1->data);

              region_pc = (shnt_region_pc *)(knode1->data);
              assert(region_pc);
              
              pc_data = attila_map_lookup(global_data->sampler->all_pc_list, region_pc->pc, ATTILA_MASTER_KEY);
              assert(pc_data);

              if (pc_data->region_count > max_region)
              {
                region_data->distance[RNPHASE(global_data)] = region_pc->distance[RNPHASE(global_data)];
                max_region = pc_data->region_count;
              }
            } 
          }

          region_data->distance[RPHASE(global_data)]  = 0xfffff;
          region_data->pc_count = 0;

          if (region_data->next_distance != 0)
          {
            region_data->distance[RNPHASE(global_data)] = region_data->next_distance;
            region_data->next_distance = 0;
          }
        }
      }

      for (ub8 i = 0; i < HTBLSIZE; i++)
      {
        head = &(global_data->sampler->all_pc_list[i]);
        assert(head);

        for (node = head->next; node != head; node = node->next)
        {
          knode = (cs_knode *)(node->data);

          pc_data = (shnt_pc_data *)(knode->data);
          assert(region_pc);

          pc_data->region_count = 0;
        }
      }

#undef RPHASE
#undef RNPHASE

      global_data->sampler->reuse_phase_size   = 0;
      global_data->sampler->reuse_phase_count += 1;

      printf("Reuse phase %ld \n", global_data->sampler->reuse_phase_count);
      printf("Dead region %ld\n", global_data->dead_region);

      SAT_CTR_SET(global_data->sampler->region_usage, RCTR_MID_VAL);
      global_data->dead_region = 0;
    }
  }

#if 0
  max_rrpv  = policy_data->max_rrpv;
#endif
  max_rrpv  = INVALID_RRPV;
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
      if (way != BYPASS_WAY)
      {
        /* Obtain SRRIP specific data */
        block = &(SRRIPHINT_DATA_BLOCKS(policy_data)[way]);

        assert(block->stream == 0);

        /* Get RRPV to be assigned to the new block */

        rrpv = cache_get_fill_rrpv_srriphint(policy_data, global_data, info, block->epoch);

        /* If block is not bypassed */
        if (rrpv != BYPASS_RRPV)
        {
          /* Ensure a valid RRPV */
#if 0
          assert(rrpv >= 0 && rrpv <= policy_data->max_rrpv); 
#endif
          assert((rrpv >= 0 && rrpv <= policy_data->max_rrpv) || rrpv == INVALID_RRPV || rrpv == INVALID_RRPV - 1); 

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
          block->access           = 0;
          block->expected_age     = 0;

          if (info->pc && info->fill)
          {
            block->pc       = info->pc;
            block->fill_pc  = info->pc;

            /* Get expected age and update the block. 5 bits of the age are 
             * taken from sequence number learned for PCs. 5 bits are taken 
             * from block address */
            
            cs_qnode           *region_table;
            shnt_region_data   *region_data;
            ub8                 pc_age; 
            ub8                 block_age; 

#define GET_AGE(p, b) (((p) << 8) | (b))
#define RPHASE(g)     (((g)->sampler->current_reuse_pahse + 1) % RPHASE_CNT)

            region_table = global_data->sampler->all_region_list;

            region_data = (shnt_region_data *)attila_map_lookup(region_table, RID(info->address), 
                ATTILA_MASTER_KEY);
            if (region_data)
            {
              pc_age = region_data->distance[RPHASE(global_data)] & 0xff; 
            }
            
            block_age = (info->address >> 6) & 0xff;

            block->expected_age = GET_AGE(pc_age, block_age);

#undef GET_AGE
#undef RPHASE
          }
          else
          {
            block->pc = 0xdead;
          }

          assert(block->next == NULL && block->prev == NULL);

          /* Insert block in to the corresponding RRPV queue */
          CACHE_APPEND_TO_QUEUE(block, 
            SRRIPHINT_DATA_VALID_HEAD(policy_data)[rrpv], 
            SRRIPHINT_DATA_VALID_TAIL(policy_data)[rrpv]);

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

    shnt_sampler_cache_lookup(global_data, policy_data, info, TRUE);

    if (info->pc)
    {
	    if (policy_data->set_indx % global_data->dbp_sampler->sampler_modulus == 0) 
      {
        int set = policy_data->set_indx / global_data->dbp_sampler->sampler_modulus;
        if (set >= 0 && set < global_data->dbp_sampler->nsampler_sets)
        {
          sdbp_sampler_access(global_data->dbp_sampler, info->pid, set,
              info->address, info->pc);
        }
      }
    }
  }

  SRRIPHINT_DATA_CFPOLICY(policy_data) = SRRIPHINT_DATA_DFPOLICY(policy_data);
}

int cache_replace_block_srriphint(srriphint_data *policy_data, srriphint_gdata *global_data,
    memory_trace *info)
{
  struct  cache_block_t *block;
  struct  cache_block_t *vctm_block;
  sb4     rrpv;
  sb4     max_rrpv;
  ub4     min_wayid;
  ub8     max_age;
  ub8     expected_age;
  ub4     gpu_zblocks;
  ub4     cpu_zblocks;

  /* Remove a nonbusy block from the tail */
  min_wayid   = ~(0);
  max_age     = 1;
  max_rrpv    = -1;
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

  /* Get block with valid max age */
  for (rrpv = SRRIP_DATA_MAX_RRPV(policy_data); rrpv >= 0; rrpv--)
  {
    for (block = SRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
    {
#if 0
      if (block->expected_age > max_age)
#endif
      expected_age = get_expected_age(global_data, block->vtl_addr, block->pc);

      if (expected_age > max_age)
      {
        max_age   = expected_age;
        min_wayid = block->way;
        max_rrpv  = rrpv;
      }
    }
  }
  
  /* Obtain RRPV from where to replace the block */
  if (max_rrpv == -1)
  {
    rrpv = cache_get_replacement_rrpv_srriphint(policy_data);
  }
  else
  {
    rrpv = max_rrpv;
  }

  /* Ensure rrpv is with in max_rrpv bound */
  assert((rrpv >= 0 && rrpv <= SRRIPHINT_DATA_MAX_RRPV(policy_data)) || 
      rrpv == INVALID_RRPV || rrpv == INVALID_RRPV - 1);

  if (min_wayid == ~(0))
  {
    /* If there is no block with required RRPV, increment RRPV of all the blocks
     * until we get one with the required RRPV */
    if (!SRRIPHINT_DATA_VALID_HEAD(policy_data)[rrpv].head)
    {
      CACHE_SRRIPHINT_INCREMENT_RRPV(SRRIPHINT_DATA_VALID_HEAD(policy_data), 
          SRRIPHINT_DATA_VALID_TAIL(policy_data), rrpv);
    }

    switch (SRRIPHINT_DATA_CRPOLICY(policy_data))
    {
      case cache_policy_srrip:
        if (min_wayid == ~(0))
        {
          for (block = SRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
          {
            if (!block->busy && block->way < min_wayid)
              min_wayid = block->way;
          }
        }

        /* If a replacement has happeded, update signature counter table  */
        if (min_wayid != ~(0))
        {
          block = &(policy_data->blocks[min_wayid]);

          if (rrpv != INVALID_RRPV && rrpv != INVALID_RRPV - 1)
          {
            CACHE_DEC_SHCT(block, global_data);
          }
        }
        break;

      case cache_policy_cpulast:
        /* First try to find a GPU block */
        for (block = SRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid && block->stream < TST))
            min_wayid = block->way;
        }

        /* If there so no GPU replacement candidate, replace CPU block */
        if (min_wayid == ~(0))
        {
          for (block = SRRIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
          {
            if (!block->busy && (block->way < min_wayid))
              min_wayid = block->way;
          }
        }
        break;

      default:
        panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
    }
  }

end:
  if (min_wayid != ~(0) && policy_data->blocks[min_wayid].pc == 134534209)
  {
    assert(1);
#if 0
    printf("%d\n", min_wayid);
#endif
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

        CACHE_REMOVE_FROM_QUEUE(blk, SRRIPHINT_DATA_VALID_HEAD(policy_data)[old_rrpv],
            SRRIPHINT_DATA_VALID_TAIL(policy_data)[old_rrpv]);
        CACHE_APPEND_TO_QUEUE(blk, SRRIPHINT_DATA_VALID_HEAD(policy_data)[new_rrpv], 
            SRRIPHINT_DATA_VALID_TAIL(policy_data)[new_rrpv]);
      }

      CACHE_UPDATE_BLOCK_STREAM(blk, strm);

      blk->dirty  |= (info && info->spill) ? TRUE : FALSE;
      blk->spill   = (info && info->spill) ? TRUE : FALSE;

#if 0
      if (info->fill && blk->ship_sign_valid)
#endif
      if (info->fill)
      {
        blk->access += 1;

        if (new_rrpv != INVALID_RRPV && new_rrpv != INVALID_RRPV - 1)
        {
          CACHE_INC_SHCT(blk, global_data);
        }
        
        if (info->pc)
        {
          blk->pc = info->pc;
        }
        else
        {
          blk->pc = 0xdead;
        }
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

      printf("\nSampler shared PC %ld \n", (global_data->sampler)->spc_count);
    }

    shnt_sampler_cache_lookup(global_data, policy_data, info, FALSE);

    if (info->pc)
    {
	    if (policy_data->set_indx % global_data->dbp_sampler->sampler_modulus == 0) 
      {
		    int set = policy_data->set_indx / global_data->dbp_sampler->sampler_modulus;
		    if (set >= 0 && set < global_data->dbp_sampler->nsampler_sets)
        {
          sdbp_sampler_access(global_data->dbp_sampler, info->pid, set,
              info->address, info->pc);
        }
      }
    }
  }
}

int cache_get_fill_rrpv_srriphint(srriphint_data *policy_data, 
    srriphint_gdata *global_data, memory_trace *info, ub4 epoch)
{
  int ret_rrpv;
  
  ret_rrpv = SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;

  if (info->fill)
  {
    if (global_data->brrip_ctr[info->core] == global_data->threshold - 1)
    {
      global_data->brrip_ctr[info->core] = 0;
      ret_rrpv = SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;
    }
    else
    {
      if (global_data->ship_shct[SHIPSIGN(global_data, info)])
      {
        ret_rrpv = SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1;
      }
      else
      {
        ret_rrpv = SRRIPHINT_DATA_MAX_RRPV(policy_data);
      }
    }

    global_data->brrip_ctr[info->core] += 1;
  }

  return ret_rrpv;
}

int cache_get_replacement_rrpv_srriphint(srriphint_data *policy_data)
{
  /* Replacement RRPV decision is based on the state of lists at MAX_RRPV and INVALID_RRPV.
   * 
   * If there are blocks at MAX_RRPV, MAX_RRPV is chosen for victimization. else If there 
   * are blocks in list at INVALID_RRPV, blocks at RRPV below MAX_RRPV ate aged and 
   * INVALID_RRPV is chosen for victimization. If these conditions fail MAX_RRPV is chosed
   * for victimization.
   * */
#if 0
  if (SRRIPHINT_DATA_VALID_HEAD(policy_data)[SRRIPHINT_DATA_MAX_RRPV(policy_data)].head)
  {
    return SRRIPHINT_DATA_MAX_RRPV(policy_data);
  }

  if (SRRIPHINT_DATA_VALID_HEAD(policy_data)[INVALID_RRPV].head)
  {
#if 0
    if (!SRRIPHINT_DATA_VALID_HEAD(policy_data)[SRRIPHINT_DATA_MAX_RRPV(policy_data) - 1].head)
    {
      CACHE_SRRIPHINT_INCREMENT_RRPV(SRRIPHINT_DATA_VALID_HEAD(policy_data), 
          SRRIPHINT_DATA_VALID_TAIL(policy_data), SRRIPHINT_DATA_MAX_RRPV(policy_data));
    }
#endif    

    assert(policy_data->shared_blocks);
    
    if (policy_data->shared_blocks > 2)
    {
      policy_data->shared_blocks -= 1;
      return INVALID_RRPV;
    }
    
    if (SRRIPHINT_DATA_VALID_HEAD(policy_data)[2].head)
    {
      return 2;
    }
    else if (SRRIPHINT_DATA_VALID_HEAD(policy_data)[1].head)
    {
      return 1;
    }
    else if (SRRIPHINT_DATA_VALID_HEAD(policy_data)[0].head)
    {
      return 0;
    }

  }
#endif

  return SRRIPHINT_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_srriphint(srriphint_data *policy_data, srriphint_gdata *global_data, 
    memory_trace *info, sb4 old_rrpv, struct cache_block_t *block, ub4 epoch)
{
  int new_rrpv;

  /* Return replacement RRPV based on the state of MAX_RRPV and INVALID_RRPV */
#define LTP0(i) ((i)->pc == 134515657 || (i)->pc == 134515647 || (i)->pc == 134603813 || (i)->pc == 134558835)
#define LTP1(i) ((i)->pc == 134559019 || (i)->pc == 134558903)
#define LTP2(i) ((i)->pc == 134513744 || (i)->pc == 134518229 || (i)->pc == 134558835)
#define LTP3(i) ((i)->pc == 134605840 || (i)->pc == 134615624 || (i)->pc == 134615658)
#define LTP4(i) ((i)->pc == 134615572 || (i)->pc == 134822902)

#if 0
#define LTP(i)  (LTP0(i) || LTP1(i) || LTP2(i) || LTP3(i) || LTP4(i))
#endif

#define LTP(i)  (LTP4(i))

  new_rrpv = 0;

#if 0
  if (!LTP(info) && info->fill && info->pc)
  {
	  unsigned int trace = sdbp_make_trace(global_data->dbp_sampler, info->pid, 
        global_data->dbp_sampler->pred, info->pc);

    if (sdbp_predictor_get_prediction(global_data->dbp_sampler, global_data->dbp_sampler->pred, 
        info->pid, trace, policy_data->set_indx))
    {
      new_rrpv = 3;
    }
  }
#endif

  return (info->spill) ? old_rrpv : new_rrpv;

#undef LTP0
#undef LTP1
#undef LTP2
#undef LTP3
#undef LTP
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

#if 0
  max_rrpv  = policy_data->max_rrpv;
#endif
  max_rrpv  = INVALID_RRPV;
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
  
  if (info->pc)
  {
    sampler->blocks[index][way].pc[offset] = info->pc;
  }
  else
  {
    sampler->blocks[index][way].pc[offset] = 0xdead;
  }

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
  
  ub8 old_pc;

  if (info->pc)
  {
    old_pc = sampler->blocks[index][way].pc[offset];

    if (old_pc != 0xdead && info->pc != old_pc)
    {
      /* Insert pc into shared PC list */
      if (!attila_map_lookup(sampler->spc_list, old_pc, ATTILA_MASTER_KEY))
      {
        sampler->spc_count += 1;
        attila_map_insert(sampler->spc_list, old_pc, ATTILA_MASTER_KEY, info->pc);
      }

      shnt_pc_data *pc_data;
      shnt_d_data *d_data;

      pc_data = (shnt_pc_data *)attila_map_lookup(sampler->all_pc_list, old_pc, ATTILA_MASTER_KEY);
      if (pc_data)
      {
        d_data = (shnt_d_data *)attila_map_lookup(pc_data->ppc, info->pc, ATTILA_MASTER_KEY);
        if (!d_data)
        {
          d_data = (shnt_d_data*)xcalloc(1, sizeof(shnt_d_data));
          assert(d_data);

          d_data->distance  = ++(sampler->pc_distance);
          d_data->ppc       = info->pc;
          d_data->pc        = old_pc;
          d_data->reuse     = 0;

          /* Insert distance into a list */
          attila_map_insert(pc_data->ppc, info->pc, ATTILA_MASTER_KEY, (ub8)d_data);
        }
        else
        {
          d_data->reuse += 1;
        }
      }
    }

    sampler->blocks[index][way].pc[offset] = info->pc;
  }

  sampler->perfctr.sampler_hit += 1;
}

void shnt_sampler_cache_lookup(srriphint_gdata *global_data, srriphint_data *policy_data, 
    memory_trace *info, ub1 update_time)
{
  shnt_sampler_cache *sampler;

  ub4 way;
  ub4 index;
  ub8 page;
  ub8 offset;
  ub1 strm;
  
  sampler = global_data->sampler;

  /* Obtain sampler index, tag, offset and cache index of the access */
  index   = SAMPLER_INDX(info, sampler);
  page    = SAMPLER_TAG(info, sampler);
  offset  = SAMPLER_OFFSET(info, sampler);
  strm    = NEW_STREAM(info);

#if 0
  if ((offset % ((1 << (LOG_GRAIN_SIZE - LOG_BLOCK_SIZE)) / PAGE_COVERAGE)) == 0)
#endif
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
          sampler->blocks[index][way].pc[off]             = 0xdead;
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
        shnt_sampler_cache_fill_block(sampler, index, way, policy_data, info, FALSE);    
      }
      else
      {
        increment_dead_limit(global_data, sampler->blocks[index][way].pc[offset],
            info->pc, sampler->blocks[index][way].hit_count[offset]);

        /* If sampler access was a hit */
        shnt_sampler_cache_access_block(sampler, index, way, policy_data, info, TRUE);
      }
      
      if (info->pc)
      {
        sampler->blocks[index][way].pc[offset] = info->pc;
      }
      else
      {
        sampler->blocks[index][way].pc[offset] = 0xdead;
      }
    }
  }
}

// constructor for a sampler set
void sdbp_sampler_set_init(sdbp_sampler *sampler, sdbp_sampler_set *sampler_set)
{
  // allocate some sampler entries
  sampler_set->blocks = (sdbp_sampler_entry *)xcalloc(1, sizeof(sdbp_sampler_entry) * sampler->dan_sampler_assoc);
  assert(sampler_set->blocks);

  // initialize the LRU replacement algorithm for these entries
  for (int i = 0; i < sampler->dan_sampler_assoc; i++)
  {
    sampler_set->blocks[i].lru_stack_position = i;
  }
}

// constructor for sampler entry
void sdbp_sampler_entry_init(sdbp_sampler_entry *entry)
{
  entry->lru_stack_position = 0;
  entry->valid      = FALSE;
  entry->tag        = 0;
  entry->trace      = 0;
  entry->prediction = 0;
}

void sdbp_sampler_init(sdbp_sampler *sampler, int nsets, int assoc) 
{
	// four-core version gets slightly different parameters
  
	sampler->dan_sampler_assoc = 12;

	// number of bits used to index predictor; determines number of
	// entries in prediction tables (changed for 4MB cache)

	sampler->dan_predictor_index_bits = 12;

	// number of prediction tables

	sampler->dan_predictor_tables = 3;

	// width of prediction saturating counters

	sampler->dan_counter_width = 2;

	// predictor must meet this threshold to predict a block is dead

	sampler->dan_threshold = 8;

	// number of partial tag bits kept per sampler entry

	sampler->dan_sampler_tag_bits = 31;

	// number of trace (partial PC) bits kept per sampler entry

	sampler->dan_sampler_trace_bits = 31;

	// here, we figure out the total number of bits used by the various
	// structures etc.  along the way we will figure out how many
	// sampler sets we have room for

	// figure out number of entries in each table

	sampler->dan_predictor_table_entries = 1 << sampler->dan_predictor_index_bits;

	// compute the total number of bits used by the replacement policy

	// total number of bits available for the contest

	int nbits_total = (nsets * assoc * 8 + 1024);

	// the real LRU policy consumes log(assoc) bits per block

	int nbits_lru = assoc * nsets * (int) log2 (assoc);

	// the dead block predictor consumes (counter width) * (number of tables) 
	// * (entries per table) bits

	int nbits_predictor = 
		sampler->dan_counter_width * sampler->dan_predictor_tables * sampler->dan_predictor_table_entries;

	// one prediction bit per cache block.

	int nbits_cache = 1 * nsets * assoc;

	// some extra bits we account for to be safe; figure we need about 85 bits
	// for the various run-time constants and variables the CRC guys might want
	// to charge us for.  in reality we leave a bigger surplus than this so we
	// should be safe.

	int nbits_extra = 85; 

	// number of bits left over for the sampler sets

	int nbits_left_over = 
		nbits_total - (nbits_predictor + nbits_cache + nbits_lru + nbits_extra);

	// number of bits in one sampler set: associativity of sampler * bits per sampler block entry

	int nbits_one_sampler_set = 
		sampler->dan_sampler_assoc 
		// tag bits, valid bit, prediction bit, trace bits, lru stack position bits
		* (sampler->dan_sampler_tag_bits + 1 + 1 + 4 + sampler->dan_sampler_trace_bits);

	// maximum number of sampler of sets we can afford with the space left over

	sampler->nsampler_sets = nbits_left_over / nbits_one_sampler_set;

	// compute the maximum saturating counter value; predictor constructor
	// needs this so we do it here

	sampler->dan_counter_max = (1 << sampler->dan_counter_width) -1;

	// make a predictor
	sampler->pred = (sdbp_predictor *)xcalloc(1, sizeof(sdbp_predictor));
  assert(sampler->pred);
  
  sdbp_predictor_init(sampler, sampler->pred);

	// we should have at least one sampler set
	assert (sampler->nsampler_sets >= 0);

	// make the sampler sets
	sampler->sets = (sdbp_sampler_set *)xcalloc(1, sizeof(sdbp_sampler_set) * sampler->nsampler_sets);
  assert(sampler->sets);
  
  /* Initialize sampler sets */
  for (ub4 i = 0; i < sampler->nsampler_sets; i++)
  {
    sdbp_sampler_set_init(sampler, &(sampler->sets[i]));
  }

	// figure out what should divide evenly into a set index to be
	// considered a sampler set
	sampler->sampler_modulus = nsets / sampler->nsampler_sets;

	// compute total number of bits used; we can print this out to validate
	// the computation in the paper

	sampler->total_bits_used = 
		(nbits_total - nbits_left_over) + (nbits_one_sampler_set * sampler->nsampler_sets);
	//fprintf (stderr, "total bits used %d\n", total_bits_used);
}

// access the sampler with an LLC tag
void sdbp_sampler_access(sdbp_sampler *sampler, ub4 tid, sb4 set, ub8 tag, ub8 PC) 
{
	// get a pointer to this set's sampler entries
	sdbp_sampler_entry *blocks = &sampler->sets[set].blocks[0];

	// get a partial tag to search for
	unsigned int partial_tag = tag & ((1 << sampler->dan_sampler_tag_bits)-1);

	// assume we do not miss
	ub1 miss = FALSE;

	// this will be the way of the sampler entry we end up hitting or replacing
	int i;

	// search for a matching tag
	for (i=0; i < sampler->dan_sampler_assoc; i++) 
  {
    if (blocks[i].valid && (blocks[i].tag == partial_tag)) 
    {
      // we know this block is not dead; inform the predictor
      sdbp_predictor_block_is_dead(sampler, sampler->pred, tid, blocks[i].trace, FALSE);
      break;
    }
  }

	// did we find a match?
	if (i == sampler->dan_sampler_assoc) 
  {
		// no, so this is a miss in the sampler
		miss = TRUE;

		// look for an invalid block to replace
		for (i=0; i < sampler->dan_sampler_assoc; i++) 
    {
      if (blocks[i].valid == FALSE) 
      {
        break;
      }
    }

		// no invalid block?  look for a dead block.
		if (i == sampler->dan_sampler_assoc) 
    {
			// find the LRU dead block
			for (i = 0; i < sampler->dan_sampler_assoc; i++) 
      {
        if (blocks[i].prediction) 
        {
          break;
        }
      }
		}

		// no invalid or dead block?  use the LRU block
		if (i == sampler->dan_sampler_assoc) 
    {
			int j;

			for (j = 0; j < sampler->dan_sampler_assoc; j++)
      {
				if (blocks[j].lru_stack_position == (unsigned int) (sampler->dan_sampler_assoc - 1)) 
        {
          break;
        }
      }

			assert (j < sampler->dan_sampler_assoc);

			i = j;
		}

		// previous trace leads to block being dead; inform the predictor
		sdbp_predictor_block_is_dead(sampler, sampler->pred, tid, blocks[i].trace, TRUE);

		// fill the victim block
		blocks[i].tag   = partial_tag;
		blocks[i].valid = TRUE;
	}

	// record the trace
	blocks[i].trace = sdbp_make_trace(sampler, tid, sampler->pred, PC);

	// get the next prediction for this entry
	blocks[i].prediction = sdbp_predictor_get_prediction(sampler, sampler->pred, tid, blocks[i].trace, -1);

	// now the replaced entry should be moved to the MRU position
	ub4 position = blocks[i].lru_stack_position;

	for(sb4 way = 0; way < sampler->dan_sampler_assoc; way++)
  {
		if (blocks[way].lru_stack_position < position)
    {
			blocks[way].lru_stack_position++;
    }
  }

	blocks[i].lru_stack_position = 0;
}

// hash three numbers into one
static unsigned int mix (unsigned int a, unsigned int b, unsigned int c) 
{
	a=a-b;  a=a-c;  a=a^(c >> 13);
	b=b-c;  b=b-a;  b=b^(a << 8);
	c=c-a;  c=c-b;  c=c^(b >> 13);
	return c;
}

// first hash function
static unsigned int f1 (unsigned int x) 
{
	return mix (0xfeedface, 0xdeadb10c, x);
}

// second hash function
static unsigned int f2 (unsigned int x) 
{
	return mix (0xc001d00d, 0xfade2b1c, x);
}

// generalized hash function
static unsigned int fi (unsigned int x, int i) 
{
	return f1 (x) + (f2 (x) >> i);
}

// hash a trace, thread ID, and predictor table number into a predictor table index
static unsigned int sdbp_predictor_get_table_index(sdbp_sampler *sampler, ub4 tid, ub4 trace, sb4 t) 
{
	unsigned int x = fi(trace ^ (tid << 2), t);

	return x & ((1 << sampler->dan_predictor_index_bits)-1);
}

// constructor for the predictor
void sdbp_predictor_init(sdbp_sampler *sampler, sdbp_predictor *pred) 
{
  // make the tables
  pred->tables = (int **)xcalloc(1, sizeof(int *) * sampler->dan_predictor_tables);

  // initialize each table to all 0s
  for (int i = 0; i < sampler->dan_predictor_tables; i++) 
  {
    pred->tables[i] = (int *)xcalloc(1, sizeof(int) * sampler->dan_predictor_table_entries);
    memset(pred->tables[i], 0, sizeof(int) * sampler->dan_predictor_table_entries);
  }
}

// inform the predictor that a block is either dead or not dead
void sdbp_predictor_block_is_dead(sdbp_sampler *sampler, sdbp_predictor *pred, 
    ub4 tid, ub4 trace, ub1 d) 
{

	// for each predictor table...
	for (int i = 0; i < sampler->dan_predictor_tables; i++) 
  {
		// ...get a pointer to the corresponding entry in that table
		int *c = &(pred->tables[i][sdbp_predictor_get_table_index(sampler, tid, trace, i)]);

		// if the block is dead, increment the counter
		if (d) 
    {
			if (*c < sampler->dan_counter_max) (*c)++;
		} 
    else 
    {
			// otherwise, decrease the counter
			if (i & 1) 
      {
				// odd numbered tables decrease exponentially
				(*c) >>= 1;
			} 
      else 
      {
				// even numbered tables decrease by one
				if (*c > 0) 
        {
          (*c)--;
        }
			}
		}
	}
}

// get a prediction for a given trace
ub1 sdbp_predictor_get_prediction (sdbp_sampler *sampler, sdbp_predictor *pred, ub4 tid, ub4 trace, sb4 set) 
{
	// start the confidence sum as 0
	int conf = 0;

	// for each table...
	for (int i = 0; i < sampler->dan_predictor_tables; i++) 
  {
		// ...get the counter value for that table...
		int val = pred->tables[i][sdbp_predictor_get_table_index(sampler, tid, trace, i)];

		// and add it to the running total
		conf += val;
	}

	// if the counter is at least the threshold, the block is predicted dead
	return conf >= sampler->dan_threshold;
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
#undef SIGNSIZE
#undef SHCTSIZE
#undef COREBTS
#undef SB
#undef SP
#undef SM
#undef SIGMAX_VAL
#undef SIGNMASK
#undef SIGNP
#undef SIGNM
#undef SIGNCORE
#undef GET_SSIGN
#undef GET_SIGN
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
#undef RID
