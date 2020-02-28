#include "access-phase.h"
#include <string.h>
#include <stdio.h>

#define BLOCK_SIZE  (64)
#define PAGE_SIZE   (4096)
#define PHASE_BITS  (12)
#define PHASE_SIZE  (1 << PHASE_BITS)
#define LBSIZE      (3)
#define BYTEBIT     (8)
#define CTR_W       (20)
#define CTR_M       (0)
#define CTR_X       ((1 << CTR_W) - 1)

void access_phase_init(access_phase *phase, ub1 stream_in, ub4 entry_count_in,
    gzFile *out_file_in)
{
  phase->src_stream   = stream_in;
  phase->tgt_stream   = NN;
  phase->access_count = 0;
  phase->phase_id     = 1;
  phase->entry_count  = entry_count_in;
  phase->out_file     = out_file_in;

  /* Ensure entries are power of 2 */
  assert((entry_count_in & (entry_count_in - 1)) == 0);
  assert(PHASE_BITS >= ACC_BUCKET_BITS);
  assert((CTR_W - (PHASE_BITS - ACC_BUCKET_BITS)) > 0);

  /* Initialize accumulator table */
  phase->fill_table = (sctr *)malloc(sizeof(sctr) * entry_count_in);
  assert(phase->fill_table);

  phase->hit_table = (sctr *)malloc(sizeof(sctr) * entry_count_in);
  assert(phase->hit_table);

  phase->query_table = (ub8 *)malloc(sizeof(ub8) * entry_count_in);
  assert(phase->query_table);

  for (ub4 i = 0; i < entry_count_in; i++)
  {
    SAT_CTR_INI(phase->fill_table[i], CTR_W, CTR_M, CTR_X);
    SAT_CTR_INI(phase->hit_table[i], CTR_W, CTR_M, CTR_X);
    memset(&(phase->query_table[i]), 0, sizeof(ub8));
  }
  
  /* Initialize fill and hit max / min counters */
  phase->fill_max   = 0;
  phase->hit_max    = 0;
  phase->fill_min   = 0xffff;
  phase->hit_min    = 0xffff;
  phase->start_addr = 0;
  phase->end_addr   = 0;
} 

void access_phase_fini(access_phase *phase)
{
  end_phase(phase);
}

void set_phase_target_stream(access_phase *phase, ub1 stream_in)
{
  assert(phase->tgt_stream == NN && stream_in != NN);

  phase->tgt_stream = stream_in;
}

void set_access_phase_end_address(access_phase *phase, ub8 end)
{
  assert(phase);
  phase->end_addr = end; 
}

void set_access_phase_start_address(access_phase *phase, ub8 start)
{
  assert(phase);
  phase->start_addr = start; 
}

#define INVALID_MIN (0xfffff)

void add_phase_block(access_phase *phase, ub1 stream, ub1 miss, ub8 address)
{
  ub8 tag_hash;
  ub4 hit_val;
  ub4 fill_val;
  ub4 old_fill_val;
  ub4 old_hit_val;

  assert(phase);
  assert(stream == phase->src_stream || stream == phase->tgt_stream);

  phase->fill_min   = INVALID_MIN;
  phase->fill_max   = 0;
  phase->hit_min    = INVALID_MIN;
  phase->hit_max    = 0;

  /* Insert blocks into accumulator table */
  tag_hash  = (address / BLOCK_SIZE) % phase->entry_count;

  if (phase->src_stream != phase->tgt_stream)
  {
    if (stream == phase->src_stream)
    {
      SAT_CTR_INC(phase->fill_table[tag_hash]);
    }
    else
    {
      if (SAT_CTR_VAL(phase->fill_table[tag_hash]))
      {
        SAT_CTR_INC(phase->hit_table[tag_hash]);
      }
    }
  }
  else
  {
    if (miss)
    {
      SAT_CTR_INC(phase->fill_table[tag_hash]);
    }
    else
    {
      if (!SAT_CTR_VAL(phase->fill_table[tag_hash]))
      {
        SAT_CTR_INC(phase->fill_table[tag_hash]);
      }
      else
      {
        SAT_CTR_INC(phase->hit_table[tag_hash]);
      }
    }
  }
  
  for (int i = 0; i < phase->entry_count; i++)
  {
    if (SAT_CTR_VAL(phase->fill_table[i]) && 
        (SAT_CTR_VAL(phase->fill_table[i]) < phase->fill_min))
    {
      phase->fill_min = SAT_CTR_VAL(phase->fill_table[i]);
    }

    if (SAT_CTR_VAL(phase->fill_table[i]) && 
        (SAT_CTR_VAL(phase->fill_table[i]) > phase->fill_max))
    {
      phase->fill_max = SAT_CTR_VAL(phase->fill_table[i]);
    }

    if (SAT_CTR_VAL(phase->hit_table[i]) && 
        (SAT_CTR_VAL(phase->hit_table[i]) < phase->hit_min))
    {
      phase->hit_min = SAT_CTR_VAL(phase->hit_table[i]);
    }

    if (SAT_CTR_VAL(phase->hit_table[i]) && 
        (SAT_CTR_VAL(phase->hit_table[i]) > phase->hit_max))
    {
      phase->hit_max = SAT_CTR_VAL(phase->hit_table[i]);
    }
  }
  
  if (phase->fill_min == INVALID_MIN)
  {
    phase->fill_min = 0;
  }

  if (phase->hit_min == INVALID_MIN)
  {
    phase->hit_min = 0;
  }

  phase->access_count++;

  /* If accesses are above a threshold, reset the phase */
  if (phase->access_count >= PHASE_SIZE)
  {
    end_phase(phase);
  }
}

#undef INVALID_MIN

ub4 get_phase_reuse_ratio(access_phase *phase, ub1 stream_in,
    ub8 address)
{
  ub8 tag_hash;
  ub4 hit;
  ub4 fill;
  ub4 reuse_ratio;
  ub4 ret_rrpv;

  assert(phase);
  assert((sizeof(ub4) * BYTEBIT) >= CTR_W);

  reuse_ratio = 0;
  ret_rrpv    = 3;

  /* Insert blocks into accumulator table */
  tag_hash  = (address / BLOCK_SIZE) % phase->entry_count;

  /* Update query count */
  if (stream_in == phase->tgt_stream)
  {
    phase->query_table[tag_hash]++;
  }

  hit   = SAT_CTR_VAL(phase->hit_table[tag_hash]);
  fill  = SAT_CTR_VAL(phase->fill_table[tag_hash]);

  if (fill)
  {
    reuse_ratio =  hit / fill;
  }

#if 0
#define NORM_CONST  (2)

  if (hit && (phase->hit_max - phase->hit_min) > (phase->hit_max >> 2))
  {
    if (phase->hit_max - phase->hit_min)
    {
      ret_rrpv = (((phase->hit_max - hit) * NORM_CONST) / (phase->hit_max - phase->hit_min));
    }
    else
    {
      assert(phase->hit_max && phase->hit_min);
      ret_rrpv = 3;
    }
  }
  
#undef NORM_CONST
#endif

#define REUSE_TH (1)

  if (phase->src_stream != phase->tgt_stream)
  {
    if (hit && reuse_ratio > REUSE_TH)
    {
      ret_rrpv = 0;
    }
    else
    {
      ret_rrpv = 3;
    }
  }

  return ret_rrpv;
}

void end_phase(access_phase *phase)
{
  ub4 fill_val;
  ub4 hit_val;
  ub8 reuse;

  assert(phase);

  if(phase->out_file)
  {
    gzprintf(phase->out_file, "New phase : %ld : %lx : %ld - %ld\n", 
        (phase->phase_id)++, phase, phase->start_addr, phase->end_addr);

    /* Dump counters to trace file and halve the values */
    for (ub4 i = 0; i < phase->entry_count; i++)
    {
      fill_val  = SAT_CTR_VAL(phase->fill_table[i]);
      hit_val   = SAT_CTR_VAL(phase->hit_table[i]);

#define NORM_CONST (3)

      if (phase->fill_max && SAT_CTR_VAL(phase->fill_table[i]) && 
          (phase->fill_max - phase->fill_min) > (phase->fill_max >> 2))
      {
        assert(phase->fill_max >= phase->fill_min);
        fill_val = (( phase->fill_max - fill_val)  * NORM_CONST) / (phase->fill_max - phase->fill_min);
      }

      if (phase->hit_max && SAT_CTR_VAL(phase->hit_table[i]) && 
          (phase->hit_max - phase->hit_min) > (phase->hit_max >> 2))
      {
        assert(phase->hit_max >= phase->hit_min);
        hit_val = ((phase->hit_max - hit_val) * NORM_CONST) / (phase->hit_max - phase->hit_min);
      }

#undef NORM_CONST       

      gzprintf(phase->out_file, "%2d| %2d %2d | %4d %4d | %4d | %d %d\n", i, 
          fill_val, hit_val, SAT_CTR_VAL(phase->fill_table[i]), 
          SAT_CTR_VAL(phase->hit_table[i]), phase->query_table[i], phase->hit_min, phase->hit_max);
    }
  }

  for (ub4 i = 0; i < phase->entry_count; i++)
  {
    SAT_CTR_HLF(phase->hit_table[i]);
    SAT_CTR_HLF(phase->fill_table[i]);
#if 0
    memset(&(phase->query_table[i]), 0, sizeof(ub8));
#endif
  }

  phase->fill_min   = 0x0ffff;
  phase->fill_max   = 0;
  phase->hit_min    = 0x0ffff;
  phase->hit_max    = 0;

  phase->access_count  = 0;
}

void reset_phase(access_phase *phase)
{
  assert(phase);
  for (ub4 i = 0; i < phase->entry_count; i++)
  {
    SAT_CTR_RST(phase->hit_table[i]);
    SAT_CTR_RST(phase->fill_table[i]);
  }

  phase->fill_min     = 0x0ffff;
  phase->fill_max     = 0;
  phase->hit_min      = 0x0ffff;
  phase->hit_max      = 0;
}

#undef BLOCK_SIZE
#undef PAGE_SIZE
#undef ACC_BUCKETS
#undef PHASE_BITS
#undef PHASE_SIZE
#undef CTR_W
#undef CTR_M
#undef CTR_X
