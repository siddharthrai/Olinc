#ifndef REGION_CACHE_H
#define REGION_CACHE_H

#include "../common/intermod-common.h"
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include "cache.h"
#include "cache-block.h"
#include "access-phase.h"

/* Map of all regions */
typedef struct region_cache_t
{
  ub1 src_stream;             /* Region stream */
  ub1 tgt_stream;             /* Region stream */
  ub8 tgt_access;             /* Source stream access */
  ub8 src_access;             /* Target stream access */
  ub8 real_hits;              /* Real hits seen by the region */
  ub8 predicted_hits;         /* Real hits seen by the region */
  struct cache_t *s_region;   /* Stream region LRU table */
  access_phase   *phases;     /* Phases for given region */
  gzFile         *out_file;   /* Trace output file */
}region_cache;


void region_cache_init(region_cache *region, ub1 src_stream_in, 
    ub1 tgt_stream_in);
 
void region_cache_fini(region_cache *region);

ub4 add_region_cache_block(region_cache *region, ub1 stream_in, ub8 address);

ub4 get_region_reuse_ratio(region_cache *region, ub1 stream_in, ub8 address);
#endif
