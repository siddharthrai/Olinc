#ifndef CACHE_SIM_H
#define CACHE_SIM_H

#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <map>
#include <algorithm>
#include <list>
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

#define BL                   (8)
#define JEDEC_DATA_BUS_BITS  (64)
#define NUM_CHANS            (2)
#define NUM_RANKS            (1)
#define NUM_BANKS            (8)
#define NUM_ROWS             (32768)
#define NUM_COLS             (1024)
#define LOG_BLOCK_SIZE       (6)

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
 
typedef struct dram_request
{
  ub8 row_id;
  ub8 address;
  ub1 stream;
  ub1 done;
}dram_request;

#define DRAM_BANK_UNK   (0)
#define DRAM_BANK_IDLE  (1)
#define DRAM_BANK_ACT   (2)
#define LOG_MPAGE_COUNT (2)
#define MPAGE_COUNT     (128 >> LOG_MPAGE_COUNT)
#define MPAGE_OFFSET(c) (c >> LOG_MPAGE_COUNT)

typedef struct dram_channel
{
  map<ub8, ub8> ranks;
}dram_channel;

typedef struct dram_rank
{
  map<ub8, ub8> banks;
}dram_rank;

typedef struct dram_row
{
  ub8 global_row_access;        /* Total row access */
  ub8 global_row_critical;      /* Total critical access */
  ub8 global_row_open;          /* Total row hits */
  ub8 global_btob_hits;         /* Total possible hits */
  ub8 interval_count;           /* # interval */
  ub8 row_access;               /* # accesses to row */
  ub8 row_critical;             /* # critical access */
  ub8 row_open;                 /* # row hits */
  ub8 row_reopen;               /* # per interval row conflict */
  ub8 g_row_reopen;             /* # all row conflict */
  ub8 btob_hits;                /* Possible hits */
  ub8 max_btob_hits;            /* Max back to back hits */
  ub8 periodic_row_access;      /* Periodic accesses to a row */
  ub8 d_value;                  /* Obtain request density */
  ub4 available_blocks;         /* Total blocks available in row */
  ub8 page_access[MPAGE_COUNT]; /* Per micro-page access */
  map<ub8, ub8> ntv_blocks;     /* List of native blocks */
  map<ub8, ub8> mig_blocks;     /* List of migrated blocks */
  map<ub8, ub8> ntv_cols;       /* List of native columns */
  map<ub8, ub8> avl_cols;       /* List of available columns */
  map<ub8, ub8> row_blocks;     /* # blocks */
  map<ub8, ub8> request_queue;  /* Pending request queue */
  map<ub8, ub8> response_queue; /* Received response queue */
  map<ub8, ub8> all_blocks;     /* All blocks belonging to the row */
  map<ub8, ub8> dist_blocks;    /* Distinct blocks accessed in the row */
}dram_row;

/*
 * An access to any row is always mapped to a D row with min used blocks. 
 *
 * */
typedef struct dram_bank
{
  ub1 state;
  ub4 open_row_id;
  map<ub8, ub8> bank_rows;      /* Per bank rows */
  map<ub8, ub8> d_rows;         /* Set of D rows */
  map<ub8, ub8> remap_rows;     /* Remapped rows */
  map<ub8, ub8> predicted_rows; /* Predicted rows for reuse */
  list<ub8>     access_list;    /* Sequential list of accessed rows */

#if 0
  map<ub8, ub8> row_access;
  map<ub8, ub8> periodic_row_access;
  map<ub8, ub8> request_queue;
#endif

  ub8 *rows;

  static bool compare(std::pair<ub8, ub8> i, std::pair<ub8, ub8> j)
  {
    return (i.second < j.second);
  }

  ub8 getMaxDRow()
  {
    std::pair<ub8, ub8> min = *max_element(d_rows.begin(), d_rows.end(), compare);
    return min.first;
  }

  ub8 getMinDRow()
  {
    std::pair<ub8, ub8> min = *min_element(d_rows.begin(), d_rows.end(), compare);
    return min.first;
  }

  ub1 isDRow(ub8 row_id)
  {
    return (d_rows.find(row_id) != d_rows.end());
  }
}dram_bank;

typedef struct dram_row_set
{
  ub8           row_access;
  ub8           set_access;
  ub8           open_row_id;
  map<ub8, ub8> rows;
  
  static bool compare(std::pair<ub8, ub8> i, std::pair<ub8, ub8>j)
  {
    dram_row *row1;
    dram_row *row2;
    
    row1 = (dram_row *)(i.second);
    row2 = (dram_row *)(j.second);
    
    return (row1->row_access < row2->row_access);
  }

  ub8 getMaxRow()
  {
    std::pair<ub8, ub8> min = *max_element(rows.begin(), rows.end(), compare);
    return min.first;
  }

  ub8 getMinRow()
  {
    std::pair<ub8, ub8> min = *min_element(rows.begin(), rows.end(), compare);
    return min.first;
  }
}dram_row_set;

typedef struct row_stats
{
  ub8 access;
  ub8 row_hits;
  ub1 stream[TST + 1];
  map<ub8, ub8> blocks;
}row_stats;

#define PG_CLSTR_SIZE (32)
#define PG_CLSTR_GEN  (0)

typedef struct page_cluster
{
  ub8 id;
  ub1 free;
  ub1 allocated;
  ub8 access_count;
  ub1 stream_free[TST + 1];
  ub8 stream_access[TST + 1];
  ub8 page_access[PG_CLSTR_SIZE];
  ub8 page_remap_access[PG_CLSTR_SIZE];
  ub8 page_stream[PG_CLSTR_SIZE];
  ub8 page_no[PG_CLSTR_SIZE];
  ub8 valid[PG_CLSTR_SIZE];
}page_cluster;

typedef struct page_mapper
{
  ub8 stream_alloc[TST + 1];              /* Per-stream page allocation */
  ub8 stream_cluster[TST + 1];            /* Pre-stream next cluster */
  ub8 next_cluster;                       /* Next cluster */
  map <ub8, ub8> cluster;                 /* Map of all clusters */
}page_mapper;

typedef struct page_entry
{
  ub8 frame_no;                           /* Page frame number */
  ub1 stream;                             /* Stream to which page was allocated */
  ub1 xstream;                            /* True, if accessed by non-allocated stream */
  ub8 cluster_id;                         /* Cluster id */
  ub1 cluster_off;                        /* Cluster offset */
}page_entry;

typedef struct bank_stats
{
  ub8           access;                   /* # Access to a bank */
  map<ub8, ub8> rows;                     /* Rows accessed */
}bank_stats;

typedef struct dram_config
{
  ub8 newTransactionChan;                 /* New channel */
  ub8 newTransactionColumn;               /* New column */
  ub8 newTransactionBank;                 /* New bank */
  ub8 newTransactionRank;                 /* New rank */
  ub8 newTransactionRow;                  /* New row-id */
  dram_bank ***bank[TST + 1];             /* DRAM bank state */
  cs_qnode *trans_queue[TST + 1];         /* Per stream transaction hash table */
  cs_qnode *global_trans_queue;           /* global transaction hash table */
  cs_qnode  trans_inorder_queue;          /* Global in order queue */
  ub4 trans_queue_size;                   /* Transaction queue size */
  ub4 trans_queue_entries;                /* # entries in transaction queue */
  sb8 open_row_id;                        /* Currently open row id */
  ub8 row_access;                         /* # row access */
  ub8 row_hits;                           /* # row hits */
  ub1 priority_stream;                    /* Prioritized stream */
  map <ub1, ub8> bank_list;               /* Banks accessed by each stream */
  map <ub1, ub8> g_bank_list;             /* Global bank list for all streams */
  FILE *bank_list_out;                    /* Trace file for bank data */
}dram_config;

typedef struct page_info
{
  page_cluster *cluster;                  /* Cluster page belongs to */
  ub8           cluster_id;               /* Cluster id */
  ub1           cluster_off;              /* Page offset within cluster */
  ub1           valid;                    /* True if info is valid */
  ub1           unused;                   /* True if info is excluded from the set  */
}page_info;

typedef struct cs_mshr
{
  cs_qnode rlist; /* Retry list for conflicting requests */
}cs_mshr;

#define TOTAL_BANKS (NUM_CHANS * NUM_RANKS * NUM_BANKS)

struct cachesim_cache
{
  gzFile                trc_file;                                 /* Memory trace file */
  ub8                   indx_mask;                                /* Index mask */
  ub1                   shadow_tag;                               /* TRUE, if cache is used for shadow tags */
  cache_t              *cache;                                    /* Tag array */
  cs_qnode              rdylist;                                  /* Ready memory requests */
  cs_qnode              plist;                                    /* Pending memory requests */
  ub4                   total_mshr;                               /* # total MSHR */
  ub4                   free_mshr;                                /* # available MSHR */
  ub4                   clock_period;                             /* # Cache clock time period */
  ub8                   cachecycle;                               /* # CPU cycles */
  ub8                   cachecycle_interval;                      /* # Cache interval cycles */
  ub8                   dramcycle;                                /* # DRAM cycles */
  ub1                   dramsim_enable;                           /* If true, DRAMSIM is used */
  ub1                   remap_crtcl;                              /* If true, remap non-critical stream address */
  ub8                   remap_count;                              /* # remap */
  ub1                   dramsim_trace;                            /* TRUE, if memory trace is DRAM access trace */
  map<ub8, cs_mshr*>    mshr;                                     /* MSHR storage */
  ub8                   access[TST + 1];                          /* Per stream access */
  ub8                   miss[TST + 1];                            /* Per stream miss */
  ub1                   speedup_enabled;                          /* True if DRAM considers speedup hints */
  dram_config           dram_info;                                /* DRAM access info */
  ub8                   itr_count;                                /* Total stats iteration */
  map<ub8, ub8>         page_table;                               /* Page table */
  map<ub8, ub8>         remap_page_table;                         /* Remap page table */
  map<ub8, ub8>         remap_rows;                               /* Remapped rows */
  ub8                   cache_access_count;                       /* # access to cache */
  ub8                   dram_access_count;                        /* # access to dram */
  ub8                   per_stream_fill[TST + 1];                 /* # fills by each stream */
  ub8                   per_stream_max_fill;                      /* Maximum fills across all streams */
  sctr                  sarp_hint[TST + 1];                       /* Speedup count */
  ub1                   speedup_stream[TST + 1];                  /* True, if stream is speedup */
  ub1                   shuffle_row;                              /* True if accesses are shuffled across rows */
  map<ub8, ub8>         dramsim_channels;                         /* Per-rank per-bank open rows */
  map<ub8, ub8>         dramsim_row_set;                          /* Set of rows used for dynamic row mapping */
};

#undef TOTAL_BANKS

typedef struct cachesim_cache cachesim_cache;

cache_access_status  cachesim_incl_cache(cachesim_cache *cache, ub8 addr, ub8 rdis, 
  ub1 policy, ub1 stream);
void set_cache_params(struct cache_params *params, LChParameters *lcP, 
  ub8 cache_size, ub8 cache_way, ub8 cache_set);
ub8 mmu_translate(cachesim_cache *cache, memory_trace *info);
void cachesim_update_cycle_interval_end(cachesim_cache *cache);
#endif
