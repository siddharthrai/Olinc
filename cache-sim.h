#ifndef CACHE_SIM_H
#define CACHE_SIM_H

#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <map>
#include <stdlib.h>
#include <zlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include "math.h"
#include "support/zfstream.h"
#include "cache/cache.h"
#include "common/common.h"
#include "common/intermod-common.h"
#include "common/List.h"
#include "configloader/ConfigLoader.h"
#include "statisticsmanager/StatisticsManager.h"

using namespace std;

#define CACHE_BLCKOFFSET     (6)
#define CLRSTRUCT(s)         (memset((&s), 0, sizeof(s)))
#define CACHE_L1_BSIZE       ((ub8)((ub8)1 << CACHE_BLCKOFFSET))

#define CACHE_AND(x, y) ((x) & (y)) 
#define CACHE_OR (x, y) ((x) | (y))
#define CACHE_SHL(x, y) (((y) == 64) ? 0 : ((x) << (y)))
#define CACHE_SHR(x, y) (((y) == 64) ? 0 : ((x) >> (y)))

#define CACHE_BIS(x, y) (CAHE_OR((x), CAHE_SHL((ub8)0x1, (y)))) 
#define CACHE_BIC(x, y) (CACHE_AND((x), CACHE_OR(SHL((~(ub8)0), 64 - (y) + 1), SHR((~(ub8)0), (y)))))
#define CACHE_IMSK(x)   ((x)->indx_mask)
#define ADDR_NDX(cache, addr) (CACHE_AND(addr, CACHE_IMSK(cache)) >> CACHE_BLCKOFFSET)

typedef struct cs_mshr
{
  cs_qnode rlist; /* Retry list for conflicting requests */
}cs_mshr;

struct cachesim_cache
{
  gzFile              trc_file;       /* Memory trace file */
  ub8                 indx_mask;      /* Index mask */
  cache_t            *cache;          /* Tag array */
  cs_qnode            rdylist;        /* Ready memory requests */
  cs_qnode            plist;          /* Pending memory requests */
  ub4                 total_mshr;     /* # total MSHR */
  ub4                 free_mshr;      /* # available MSHR */
  ub1                 dramsim_enable; /* If true, DRAMSIM is used */
  map<ub8, cs_mshr*>  mshr;           /* MSHR storage */
  ub8                 access[TST + 1];/* Per stream access */
  ub8                 miss[TST + 1];  /* Per stream miss */
};

typedef struct cachesim_cache cachesim_cache;

cache_access_status  cachesim_incl_cache(cachesim_cache *cache, ub8 addr, ub8 rdis, 
  ub1 policy, ub1 stream);
void set_cache_params(struct cache_params *params, LChParameters *lcP, 
  ub8 cache_size, ub8 cache_way, ub8 cache_set);

#endif
