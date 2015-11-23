#ifndef CACHE_SIM_H
#define CACHE_SIM_H

#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <map>
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

typedef struct dram_bank
{
  ub1 state;
  ub4 open_row_id;
  ub8 *rows;
}dram_bank;

typedef struct row_stats
{
  ub8 access;
  map<ub8, ub8> blocks;
};

#define PG_CLSTR_SIZE (32)
#define PG_CLSTR_GEN  (0)

typedef struct page_cluster
{
  ub8 id;
  ub1 free;
  ub1 allocated;
  ub1 stream_free[TST + 1];
  ub8 stream_access[TST + 1];
  ub8 page_no[PG_CLSTR_SIZE];
  ub8 valid[PG_CLSTR_SIZE];
}page_cluster;

typedef struct page_mapper
{
  ub8 stream_alloc[TST + 1];    /* Per-stream page allocation */
  ub8 stream_cluster[TST + 1];  /* Pre-stream next cluster */
  ub8 next_cluster;             /* Next cluster */
  map <ub8, ub8> cluster;       /* Map of all clusters */
}page_mapper;

typedef struct page_entry
{
  ub8 frame_no;     /* Page frame number */
  ub1 stream;       /* Stream to which page was allocated */
  ub1 xstream;      /* True, if accessed by non-allocated stream */
  ub8 cluster_id;   /* Cluster id */
  ub1 cluster_off;  /* Cluster offset */
}page_entry;

typedef struct bank_stats
{
  ub8           access; /* # Access to a bank */
  map<ub8, ub8> rows;   /* Rows accessed */
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

typedef struct cs_mshr
{
  cs_qnode rlist; /* Retry list for conflicting requests */
}cs_mshr;

#define TOTAL_BANKS (NUM_CHANS * NUM_RANKS * NUM_BANKS)

struct cachesim_cache
{
  gzFile              trc_file;                                 /* Memory trace file */
  ub8                 indx_mask;                                /* Index mask */
  cache_t            *cache;                                    /* Tag array */
  cs_qnode            rdylist;                                  /* Ready memory requests */
  cs_qnode            plist;                                    /* Pending memory requests */
  ub4                 total_mshr;                               /* # total MSHR */
  ub4                 free_mshr;                                /* # available MSHR */
  ub1                 dramsim_enable;                           /* If true, DRAMSIM is used */
  ub1                 dramsim_trace;                            /* TRUE, if memory trace is DRAM access trace */
  map<ub8, cs_mshr*>  mshr;                                     /* MSHR storage */
  ub8                 access[TST + 1];                          /* Per stream access */
  ub8                 miss[TST + 1];                            /* Per stream miss */
  dram_config         dram_info;                                /* DRAM access info */
  ub8                 itr_count;                                /* Total stats iteration */
  ub8                 stream_access[TST + 1][TOTAL_BANKS + 1];  /* Total per stream per bank access */
  ub8                 bank_access[TST + 1][TOTAL_BANKS + 1];    /* Total per stream access to bank in iteration */
  ub8                 row_access[TST + 1][TOTAL_BANKS + 1];     /* Total per stream per bank row access */
  ub8                 row_coverage[TST + 1][TOTAL_BANKS + 1];   /* Total per stream per bank row access */
  ub8                 ct_row_access[TOTAL_BANKS + 1];           /* Rows common to color and texture */
  ub8                 cz_row_access[TOTAL_BANKS + 1];           /* Rows common to color and depth */
  ub8                 cp_row_access[TOTAL_BANKS + 1];           /* Rows common to color and CPU */
  ub8                 cb_row_access[TOTAL_BANKS + 1];           /* Rows common to color and blitter */
  ub8                 zt_row_access[TOTAL_BANKS + 1];           /* Rows common to depth and texture */
  ub8                 zp_row_access[TOTAL_BANKS + 1];           /* Rows common to depth and CPU */
  ub8                 zb_row_access[TOTAL_BANKS + 1];           /* Rows common to depth and blitter */
  ub8                 tp_row_access[TOTAL_BANKS + 1];           /* Rows common to texture and CPU */
  ub8                 tb_row_access[TOTAL_BANKS + 1];           /* Rows common to texture and blitter */
  ub8                 pb_row_access[TOTAL_BANKS + 1];           /* Rows common to CPU and blitter */
  ub8                 g_bank_access[TOTAL_BANKS + 1];           /* Total per stream access to bank in iteration */
  ub8                 g_row_access[TOTAL_BANKS + 1];            /* Total per stream per bank row access */
  ub8                 g_row_coverage[TOTAL_BANKS + 1];          /* Total per stream per bank row access */
  ub8                 g_bank_access_itr;                        /* Global bank access iterations */
  ub8                 bank_access_itr[TST + 1];                 /* Total access iteration for each bank */
  ub8                 bank_hit[TST + 1];                        /* Number of bank access by each stream */
  page_mapper         i_page_table;                             /* Used for a next level of translation */
  map <ub8, ub8>      page_table;                               /* Page table */
};

#undef TOTAL_BANKS

typedef struct cachesim_cache cachesim_cache;

cache_access_status  cachesim_incl_cache(cachesim_cache *cache, ub8 addr, ub8 rdis, 
  ub1 policy, ub1 stream);
void set_cache_params(struct cache_params *params, LChParameters *lcP, 
  ub8 cache_size, ub8 cache_way, ub8 cache_set);
ub8 mmu_translate(cachesim_cache *cache, memory_trace *info);

#endif
