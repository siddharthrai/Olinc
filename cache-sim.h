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
  ub8 row_access;
  ub8 row_open;
  ub8 btob_hits;
  ub8 max_btob_hits;
  ub8 periodic_row_access;
  map<ub8, ub8> row_blocks;
  map<ub8, ub8> request_queue;
}dram_row;

typedef struct dram_bank
{
  ub1 state;
  ub4 open_row_id;
  map<ub8, ub8> bank_rows;

#if 0
  map<ub8, ub8> row_access;
  map<ub8, ub8> periodic_row_access;
  map<ub8, ub8> request_queue;
#endif

  ub8 *rows;
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
    printf("%ld\n", min.first);
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
  ub8                   cachecycle;                               /* # CPU cycles */
  ub8                   cachecycle_interval;                      /* # Cache interval cycles */
  ub8                   dramcycle;                                /* # DRAM cycles */
  ub1                   dramsim_enable;                           /* If true, DRAMSIM is used */
  ub1                   remap_crtcl;                              /* If true, remap non-critical stream address */
  ub1                   dramsim_trace;                            /* TRUE, if memory trace is DRAM access trace */
  map<ub8, cs_mshr*>    mshr;                                     /* MSHR storage */
  ub8                   access[TST + 1];                          /* Per stream access */
  ub8                   miss[TST + 1];                            /* Per stream miss */
  ub1                   speedup_enabled;                          /* True if DRAM considers speedup hints */
  dram_config           dram_info;                                /* DRAM access info */
  ub8                   itr_count;                                /* Total stats iteration */
  ub8                   stream_access[TST + 1][TOTAL_BANKS + 1];  /* Total per stream per bank access */
  ub8                   bank_access[TST + 1][TOTAL_BANKS + 1];    /* Total per stream access to bank in iteration */
  ub8                   row_access[TST + 1][TOTAL_BANKS + 1];     /* Total per stream per bank row access */
  ub8                   row_coverage[TST + 1][TOTAL_BANKS + 1];   /* Total per stream per bank row access */
  ub8                   ct_row_access[TOTAL_BANKS + 1];           /* Rows common to color and texture */
  ub8                   cz_row_access[TOTAL_BANKS + 1];           /* Rows common to color and depth */
  ub8                   cp_row_access[TOTAL_BANKS + 1];           /* Rows common to color and CPU */
  ub8                   cb_row_access[TOTAL_BANKS + 1];           /* Rows common to color and blitter */
  ub8                   zt_row_access[TOTAL_BANKS + 1];           /* Rows common to depth and texture */
  ub8                   zp_row_access[TOTAL_BANKS + 1];           /* Rows common to depth and CPU */
  ub8                   zb_row_access[TOTAL_BANKS + 1];           /* Rows common to depth and blitter */
  ub8                   tp_row_access[TOTAL_BANKS + 1];           /* Rows common to texture and CPU */
  ub8                   tb_row_access[TOTAL_BANKS + 1];           /* Rows common to texture and blitter */
  ub8                   pb_row_access[TOTAL_BANKS + 1];           /* Rows common to CPU and blitter */
  ub8                   g_bank_access[TOTAL_BANKS + 1];           /* Total per stream access to bank in iteration */
  ub8                   g_row_access[TOTAL_BANKS + 1];            /* Total per stream per bank row access */
  ub8                   g_row_coverage[TOTAL_BANKS + 1];          /* Total per stream per bank row access */
  ub8                   g_bank_access_itr;                        /* Global bank access iterations */
  ub8                   bank_access_itr[TST + 1];                 /* Total access iteration for each bank */
  ub8                   bank_hit[TST + 1];                        /* Number of bank access by each stream */
  ub8                   per_stream_page_access[TST + 1];          /* Total pages accessed in all intervals */
  ub8                   per_stream_clstr_access[TST + 1];         /* Total clusters accessed in all intervals */
  page_mapper           i_page_table;                             /* Used for a next level of translation */
  ub8                   remap_src_access;                         /* Access to src remap set */
  ub8                   remap_tgt_access;                         /* Access to tgt remap set */
  ub8                   agg_remap_src_access;                     /* Aggregate access to src remap set */
  ub8                   agg_remap_tgt_access;                     /* Aggregate access to tgt remap set */
  ub8                   min_remap_tgt_access;                     /* Minimum  access to tgt remap set */
  ub8                   max_remap_src_access;                     /* Maximum access to src remap set */
  vector<ub8>           remap_clstr;                              /* Pages can be replaced remapping */
  vector<ub8>           src_remap_clstr;                          /* Pages to be remapped */
  vector<ub8>           remap_frames;                             /* Mappable frames */
  map<ub8, ub8>         src_frames;                               /* Pages to be remapped */
  map<ub8, ub8>         tgt_frames;                               /* Pages replaced after remap */
  list<pair<ub8, ub8> > src_frame_list;                           /* List of src pages to be remapped */
  list<pair<ub8, ub8> > tgt_frame_list;                           /* List of tgt pages to be replaced */
  map<ub8, ub8>         speedup_rows;                             /* Distinct rows in src frames */
  map<ub8, ub8>         src_rows;                                 /* Distinct rows in src frames */
  map<ub8, ub8>         tgt_rows;                                 /* Distinct rows in tgt frames */
  ub8                   src_page_count;                           /* Pages in src set */
  ub8                   tgt_page_count;                           /* Pages in tgt set */
  ub8                   src_added;                                /* New src pages */
  ub8                   tgt_added;                                /* New tgt pages */
  ub8                   next_remap_clstr;                         /* Next cluster to be used for remapping */
  ub1                   access_thr[TST + 1];                      /* Access threshold to decide src pages */
  ub1                   entry_thr[TST + 1];                       /* Entry threshold to decide tgt pages */
  ub8                   remap_pages[TST + 1];                     /* # pages remapped */
  ub8                   tgt_remap_pages[TST + 1];                 /* # target pages remapped */
  ub8                   remap_requested;                          /* # remaps requested */
  ub8                   remap_done;                               /* # remaps done */
  ub8                   remap_page_count[TST + 1];                /* # Pages that can be remapped */
  ub8                   remap_src_page_count[TST + 1];            /* # Pages that can be remapped */
  ub8                   remap_access_count[TST + 1];              /* # Access that can be remapped */
  ub8                   remap_src_access_count[TST + 1];          /* # Src access that are remapped */
  ub8                   remap_tgt_access_count[TST + 1];          /* # Tgt access that are remapped */
  ub8                   remap_src_access_count_exp[TST + 1];      /* # src access expected on remapping */
  ub8                   remap_tgt_access_count_exp[TST + 1];      /* # Tgt access expected on remapping */
  map<ub8, ub8>         remap_src_clstr;                          /* Clusters can be used as source of remapping */
  ub8                   zero_reuse_page[TST + 1];                 /* Remapped pages that see no reuse */
  ub8                   max_clstr_access;                         /* Maximum access to a cluster */
  ub8                   remap_clstr_count;                        /* Maximum access to a cluster */
  ub8                   remap_src_clstr_count;                    /* Maximum access to a cluster */
  ub8                   src_remap_page_count[TST + 1];            /* # pages remapped for each stream */
  map<ub8, ub8>         per_stream_pages[TST + 1];                /* Per-stream pages accessed in an interval */
  map<ub8, ub8>         per_stream_clstr[TST + 1];                /* Per-stream clusters accessed in an interval */
  map<ub8, ub8>         page_table;                               /* Page table */
  map<ub8, ub8>         remap_page_table;                         /* Remap page table */
  ub8                   cache_access_count;                       /* # access to cache */
  ub8                   per_stream_fill[TST + 1];                 /* # fills by each stream */
  ub8                   per_stream_max_fill;                      /* Maximum fills across all streams */
  sctr                  sarp_hint[TST + 1];                       /* Speedup count */
  ub1                   speedup_stream[TST + 1];                  /* True, if stream is speedup */
  ub1                   speedup_stream_count[TST + 1];            /* True, if stream is speedup */
  ub8                   speedup_clstr[TST + 1];                   /* Clusters accessed by critical stream */
  ub8                   speedup_page[TST + 1];                    /* Pages accessed by speedup stream */
  ub8                   speedup_block[TST + 1];                   /* Blocks accessed by speedup stream */
  ub8                   speedup_access[TST + 1];                  /* Access to blocks of critical stream */
  ub8                   speedup_min_access[TST + 1];              /* Min accesses to a block of critical stream */
  ub8                   speedup_max_access[TST + 1];              /* Max accesses to a block of critical stream */
  ub8                   speedup_page_z_access[TST + 1];           /* Pages not accessed */
  ub1                   shuffle_row;                              /* True if accesses are shuffled across rows */
  map<ub8, ub8>         dramsim_channels;                         /* Per-rank per-bank open rows */
  map<ub8, ub8>         dramsim_row_set;                          /* Set of rows used for dynamic dow mapping */
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
