#ifndef ACCESS_PHASE_H
#define ACCESS_PHASE_H

#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include "zlib.h"

#define ACC_BUCKET_BITS   (5)
#define ACC_BUCKETS       (1 << ACC_BUCKET_BITS)

/* Map of all regions */
typedef struct access_phase
{
  ub8     phase_id;     /* Phase id */
  ub8     start_addr;   /* Start address of the region */
  ub8     end_addr;     /* End address of the region */
  ub1     src_stream;   /* Region stream */
  ub1     tgt_stream;   /* Target stream */
  sctr   *fill_table;   /* Target region table indexed by hash */
  sctr   *hit_table;    /* Source region table indexed by hash */
  ub8    *query_table;  /* Count of reuse ratio query */
  ub8     access_count; /* Total number of access */
  ub4     entry_count;  /* Number of entries in the table */
  gzFile *out_file;     /* Output trace file */
  ub4     fill_min;     /* Minimum fill counter */
  ub4     fill_max;     /* Maximum fill counter */
  ub4     hit_min;      /* Minimum hit counter */
  ub4     hit_max;      /* Maximum hit counter */
}access_phase;

void access_phase_init(access_phase *phase, ub1 stream_in, ub4 entry_count_in,
    gzFile *out_file_in);

void set_access_phase_end_address(access_phase *phase, ub8 end);

void set_access_phase_start_address(access_phase *phase, ub8 start);

void set_phase_target_stream(access_phase *phase, ub1 stream_in);

void add_phase_block(access_phase *phase, ub1 stream, ub1 miss, ub8 address);

ub4 get_phase_reuse_ratio(access_phase *phase, ub1 stream_in, ub8 address);

void end_phase(access_phase *phase);

void reset_phase(access_phase *phase);

#endif
