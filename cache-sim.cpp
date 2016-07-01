#include "cache-sim.h"
#include <unistd.h>
#include "inter-stream-reuse.h"
#include "DRAMSim.h"

#define INTERVAL_SIZE             (1 << 19)
#define CYCLE_INTERVAL            (5000000)
#define ROW_ACCESS_INTERVAL       (1 << 19)
#define ROW_SET_SIZE              (6)
#define USE_INTER_STREAM_CALLBACK (FALSE)
#define MAX_CORES                 (4)
#define MAX_RRPV                  (3)
#define MAX_MSHR                  (128)
#define TQ_SIZE                   (128)
#define ST(i)                     ((i)->stream)
#define SSTU(i)                   ((i)->spill == TRUE)
#define CST(i)                    ((i)->stream == CS)
#define ZST(i)                    ((i)->stream == ZS)
#define BST(i)                    ((i)->stream == BS)
#define IS_SPILL_ALLOCATED(i)     (TRUE)
#define ACCESS_BYPASS(i)          (FALSE)
#define MAX_REUSE                 (64)
#define CS_MSHR_ALLOC             (1)
#define CS_MSHR_WAIT              (2)
#define CS_MSHR_FAIL              (3)
#define ECTR_WIDTH                (20)
#define ECTR_MIN_VAL              (0x00)  
#define ECTR_MAX_VAL              (0xfffff)  
#define ECTR_MID_VAL              (0x3ff)

char *dram_config_file_name = NULL;
ub8   pending_requests;
long long dram_size_max = 1LL << 39; /* 512GB maximum dram size. */
long long dram_size     = 1LL << 32; /* 4GB default dram size. */
ub8 read_stall;
ub8 write_stall;
ub8 mshr_stall;
ub8 dram_request_cycle[TST + 1];    /* Total DRAM request cycles */
ub8 dram_tq_cycle[TST + 1];         /* DRAM transaction queue cycles */
ub8 dram_cq_cycle[TST + 1];         /* DRAM command queue cycles */
ub8 real_access;
ub8 real_miss;
ub8 shadow_access;
ub8 shadow_miss;
ub1 access_shadow_tag;
ub8 access_jumped;
ub8 total_interval = 0;

extern  sb1 *stream_names[TST + 1];  

#define BLCKALIGN(addr) (((addr) >> CACHE_BLCKOFFSET) << CACHE_BLCKOFFSET)

  NumericStatistic <ub8> *all_access;  /* Total access */

  NumericStatistic <ub8> *i_access;    /* Input stream access */
  NumericStatistic <ub8> *i_miss;      /* Input stream miss  */
  NumericStatistic <ub8> *i_raccess;   /* Input stream access */
  NumericStatistic <ub8> *i_rmiss;     /* Input stream miss  */
  NumericStatistic <ub8> *i_blocks;    /* Input stream blocks  */
  NumericStatistic <ub8> *i_xevct;     /* Input inter stream evict */
  NumericStatistic <ub8> *i_xcevct;    /* X stream evict by C */
  NumericStatistic <ub8> *i_xzevct;    /* X stream evict by Z */
  NumericStatistic <ub8> *i_xtevct;    /* X stream evict by T */
  NumericStatistic <ub8> *i_xpevct;    /* X stream evict by P */
  NumericStatistic <ub8> *i_xrevct;    /* X stream evict by other streams */
  NumericStatistic <ub8> *i_sevct;     /* Input intra stream evict */
  NumericStatistic <ub8> *i_zevct;     /* Blocks evicted without reuse */
  NumericStatistic <ub8> *i_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *i_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *i_xhit;      /* Input inter stream hit */

  NumericStatistic <ub8> *c_access;    /* Color stream access */
  NumericStatistic <ub8> *c_miss;      /* Color stream miss */
  NumericStatistic <ub8> *c_raccess;   /* Color stream access */
  NumericStatistic <ub8> *c_rmiss;     /* Color stream miss */
  NumericStatistic <ub8> *c_blocks;    /* Color stream blocks */
  NumericStatistic <ub8> *c_cprod;     /* Color Write back */
  NumericStatistic <ub8> *c_ccons;     /* Text C stream consumption */
  NumericStatistic <ub8> *c_xevct;     /* Color inter stream evict */
  NumericStatistic <ub8> *c_sevct;     /* Color intra stream evict */
  NumericStatistic <ub8> *c_zevct;     /* Color evict with out reuse */
  NumericStatistic <ub8> *c_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *c_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *c_xzevct;    /* X stream evict by Z */
  NumericStatistic <ub8> *c_xtevct;    /* X stream evict by T */
  NumericStatistic <ub8> *c_xpevct;    /* X stream evict by P */
  NumericStatistic <ub8> *c_xrevct;    /* X stream evict by other streams */
  NumericStatistic <ub8> *c_xhit;      /* Color inter stream hit */
  NumericStatistic <ub8> *c_zchit;     /* Color Z stream hit */
  NumericStatistic <ub8> *c_tchit;     /* Color T stream hit */
  NumericStatistic <ub8> *c_bchit;     /* Color B stream hit */
  NumericStatistic <ub8> *c_dchit;     /* Color D stream hit */
  NumericStatistic <ub8> *c_e0fill;    /* Epoch 0 fill */
  NumericStatistic <ub8> *c_e0hit;     /* Epoch 0 hit */
  NumericStatistic <ub8> *c_e1fill;    /* Epoch 1 fill */
  NumericStatistic <ub8> *c_e1hit;     /* Epoch 1 hit */
  NumericStatistic <ub8> *c_e2fill;    /* Epoch 2 fill */
  NumericStatistic <ub8> *c_e2hit;     /* Epoch 2 hit */
  NumericStatistic <ub8> *c_e3fill;    /* Epoch 3 fill */
  NumericStatistic <ub8> *c_e3hit;     /* Epoch 3 hit */
  NumericStatistic <ub8> *c_e0evt;     /* Epoch 0 eviction */
  NumericStatistic <ub8> *c_e1evt;     /* Epoch 1 eviction */
  NumericStatistic <ub8> *c_e2evt;     /* Epoch 2 eviction */
  NumericStatistic <ub8> *c_e3evt;     /* Epoch 3 eviction */
  NumericStatistic <ub8> *c_re0fill;   /* Epoch 0 read fill */
  NumericStatistic <ub8> *c_re0hit;    /* Epoch 0 read hit */
  NumericStatistic <ub8> *c_re1fill;   /* Epoch 1 read fill */
  NumericStatistic <ub8> *c_re1hit;    /* Epoch 1 read hit */
  NumericStatistic <ub8> *c_re2fill;   /* Epoch 2 read fill */
  NumericStatistic <ub8> *c_re2hit;    /* Epoch 2 read hit */
  NumericStatistic <ub8> *c_re3fill;   /* Epoch 3 read fill */
  NumericStatistic <ub8> *c_re3hit;    /* Epoch 3 read hit */
  NumericStatistic <ub8> *c_re0evt;    /* Epoch 0 read eviction */
  NumericStatistic <ub8> *c_re1evt;    /* Epoch 1 read eviction */
  NumericStatistic <ub8> *c_re2evt;    /* Epoch 2 read eviction */
  NumericStatistic <ub8> *c_re3evt;    /* Epoch 3 read eviction */

  NumericStatistic <ub8> *z_access;    /* Depth stream access */
  NumericStatistic <ub8> *z_miss;      /* Depth stream miss */
  NumericStatistic <ub8> *z_raccess;   /* Depth stream access */
  NumericStatistic <ub8> *z_rmiss;     /* Depth stream miss */
  NumericStatistic <ub8> *z_blocks;    /* Depth stream blocks */
  NumericStatistic <ub8> *z_xevct;     /* Depth inter stream evict */
  NumericStatistic <ub8> *z_sevct;     /* Depth intra stream evict */
  NumericStatistic <ub8> *z_zevct;     /* Depth evict without reuse */
  NumericStatistic <ub8> *z_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *z_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *z_xcevct;    /* X stream evict by C */
  NumericStatistic <ub8> *z_xtevct;    /* X stream evict by T */
  NumericStatistic <ub8> *z_xpevct;    /* X stream evict by P */
  NumericStatistic <ub8> *z_xrevct;    /* X stream evict by other streams */
  NumericStatistic <ub8> *z_xhit;      /* Depth intra stream evict */
  NumericStatistic <ub8> *z_czhit;     /* Depth C stream evict */
  NumericStatistic <ub8> *z_tzhit;     /* Depth T stream evict */
  NumericStatistic <ub8> *z_bzhit;     /* Depth B stream evict */
  NumericStatistic <ub8> *z_e0fill;    /* Epoch 0 fill */
  NumericStatistic <ub8> *z_e0hit;     /* Epoch 0 hit */
  NumericStatistic <ub8> *z_e1fill;    /* Epoch 1 fill */
  NumericStatistic <ub8> *z_e1hit;     /* Epoch 1 hit */
  NumericStatistic <ub8> *z_e2fill;    /* Epoch 2 fill */
  NumericStatistic <ub8> *z_e2hit;     /* Epoch 2 hit */
  NumericStatistic <ub8> *z_e3fill;    /* Epoch 3 fill */
  NumericStatistic <ub8> *z_e3hit;     /* Epoch 3 hit */
  NumericStatistic <ub8> *z_e0evt;     /* Epoch 0 eviction */
  NumericStatistic <ub8> *z_e1evt;     /* Epoch 1 eviction */
  NumericStatistic <ub8> *z_e2evt;     /* Epoch 2 eviction */
  NumericStatistic <ub8> *z_e3evt;     /* Epoch 3 eviction */
  NumericStatistic <ub8> *z_re0fill;   /* Epoch 0 read fill */
  NumericStatistic <ub8> *z_re0hit;    /* Epoch 0 read hit */
  NumericStatistic <ub8> *z_re1fill;   /* Epoch 1 read fill */
  NumericStatistic <ub8> *z_re1hit;    /* Epoch 1 read hit */
  NumericStatistic <ub8> *z_re2fill;   /* Epoch 2 read fill */
  NumericStatistic <ub8> *z_re2hit;    /* Epoch 2 read hit */
  NumericStatistic <ub8> *z_re3fill;   /* Epoch 3 read fill */
  NumericStatistic <ub8> *z_re3hit;    /* Epoch 3 read hit */
  NumericStatistic <ub8> *z_re0evt;    /* Epoch 0 read eviction */
  NumericStatistic <ub8> *z_re1evt;    /* Epoch 1 read eviction */
  NumericStatistic <ub8> *z_re2evt;    /* Epoch 2 read eviction */
  NumericStatistic <ub8> *z_re3evt;    /* Epoch 3 read eviction */

  NumericStatistic <ub8> *t_access;    /* Text stream access */
  NumericStatistic <ub8> *t_miss;      /* Text stream miss */
  NumericStatistic <ub8> *t_raccess;   /* Text stream access */
  NumericStatistic <ub8> *t_rmiss;     /* Text stream miss */
  NumericStatistic <ub8> *t_blocks;    /* Text stream blocks */
  NumericStatistic <ub8> *t_xevct;     /* Text inter stream evict */
  NumericStatistic <ub8> *t_sevct;     /* Text intra stream evict */
  NumericStatistic <ub8> *t_zevct;     /* Text evict without reuse */
  NumericStatistic <ub8> *t_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *t_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *t_xcevct;    /* X stream evict by C */
  NumericStatistic <ub8> *t_xzevct;    /* X stream evict by T */
  NumericStatistic <ub8> *t_xpevct;    /* X stream evict by P */
  NumericStatistic <ub8> *t_xrevct;    /* X stream evict by other streams */
  NumericStatistic <ub8> *t_xhit;      /* Text inter stream hit */
  NumericStatistic <ub8> *t_cthit;     /* Text C stream hit */
  NumericStatistic <ub8> *t_zthit;     /* Text Z stream hit */
  NumericStatistic <ub8> *t_bthit;     /* Text B stream hit */
  NumericStatistic <ub8> *t_e0fill;    /* Epoch 0 fill */
  NumericStatistic <ub8> *t_e0hit;     /* Epoch 0 hit */
  NumericStatistic <ub8> *t_e1fill;    /* Epoch 1 fill */
  NumericStatistic <ub8> *t_e1hit;     /* Epoch 1 hit */
  NumericStatistic <ub8> *t_e2fill;    /* Epoch 2 fill */
  NumericStatistic <ub8> *t_e2hit;     /* Epoch 2 hit */
  NumericStatistic <ub8> *t_e3fill;    /* Epoch 3 fill */
  NumericStatistic <ub8> *t_e3hit;     /* Epoch 3 hit */
  NumericStatistic <ub8> *t_e0evt;     /* Epoch 0 eviction */
  NumericStatistic <ub8> *t_e1evt;     /* Epoch 1 eviction */
  NumericStatistic <ub8> *t_e2evt;     /* Epoch 2 eviction */
  NumericStatistic <ub8> *t_e3evt;     /* Epoch 3 eviction */
  NumericStatistic <ub8> *t_re0fill;   /* Epoch 0 read fill */
  NumericStatistic <ub8> *t_re0hit;    /* Epoch 0 read hit */
  NumericStatistic <ub8> *t_re1fill;   /* Epoch 1 read fill */
  NumericStatistic <ub8> *t_re1hit;    /* Epoch 1 read hit */
  NumericStatistic <ub8> *t_re2fill;   /* Epoch 2 read fill */
  NumericStatistic <ub8> *t_re2hit;    /* Epoch 2 read hit */
  NumericStatistic <ub8> *t_re3fill;   /* Epoch 3 read fill */
  NumericStatistic <ub8> *t_re3hit;    /* Epoch 3 read hit */
  NumericStatistic <ub8> *t_re0evt;    /* Epoch 0 read eviction */
  NumericStatistic <ub8> *t_re1evt;    /* Epoch 1 read eviction */
  NumericStatistic <ub8> *t_re2evt;    /* Epoch 2 read eviction */
  NumericStatistic <ub8> *t_re3evt;    /* Epoch 3 read eviction */

  NumericStatistic <ub8> *n_access;    /* Instruction stream access */
  NumericStatistic <ub8> *n_miss;      /* Instruction stream miss */
  NumericStatistic <ub8> *n_raccess;   /* Instruction stream access */
  NumericStatistic <ub8> *n_rmiss;     /* Instruction stream miss */
  NumericStatistic <ub8> *n_blocks;    /* Instruction stream blocks */
  NumericStatistic <ub8> *n_xevct;     /* Instruction inter stream evict */
  NumericStatistic <ub8> *n_sevct;     /* Instruction intra stream evict */
  NumericStatistic <ub8> *n_zevct;     /* Instruction evict without reuse */
  NumericStatistic <ub8> *n_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *n_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *n_xcevct;    /* X stream evict by C */
  NumericStatistic <ub8> *n_xzevct;    /* X stream evict by Z */
  NumericStatistic <ub8> *n_xtevct;    /* X stream evict by T */
  NumericStatistic <ub8> *n_xpevct;    /* X stream evict by P */
  NumericStatistic <ub8> *n_xrevct;    /* X stream evict by other streams */
  NumericStatistic <ub8> *n_xhit;      /* Instruction inter stream hit */

  NumericStatistic <ub8> *h_access;    /* HZ stream access */
  NumericStatistic <ub8> *h_miss;      /* HZ stream miss */
  NumericStatistic <ub8> *h_raccess;   /* HZ stream access */
  NumericStatistic <ub8> *h_rmiss;     /* HZ stream miss */
  NumericStatistic <ub8> *h_blocks;    /* HZ stream blocks */
  NumericStatistic <ub8> *h_xevct;     /* HZ inter stream evict */
  NumericStatistic <ub8> *h_sevct;     /* HZ intra stream evict */
  NumericStatistic <ub8> *h_zevct;     /* HZ evict without reuse */
  NumericStatistic <ub8> *h_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *h_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *h_coevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *h_xcevct;    /* X stream evict by C */
  NumericStatistic <ub8> *h_xzevct;    /* X stream evict by Z */
  NumericStatistic <ub8> *h_xtevct;    /* X stream evict by T */
  NumericStatistic <ub8> *h_xpevct;    /* X stream evict by P */
  NumericStatistic <ub8> *h_xrevct;    /* X stream evict by other streams */
  NumericStatistic <ub8> *h_xhit;      /* HZ inter stream hit */

  NumericStatistic <ub8> *b_access;    /* Blitter stream access */
  NumericStatistic <ub8> *b_miss;      /* Blitter stream miss */
  NumericStatistic <ub8> *b_raccess;   /* Blitter stream access */
  NumericStatistic <ub8> *b_rmiss;     /* Blitter stream miss */
  NumericStatistic <ub8> *b_blocks;    /* Blitter stream blocks */
  NumericStatistic <ub8> *b_bprod;     /* Blitter writeback */
  NumericStatistic <ub8> *b_bcons;     /* Texture B stream consumption */
  NumericStatistic <ub8> *b_xevct;     /* Blitter inter stream evict */
  NumericStatistic <ub8> *b_sevct;     /* Blitter intra stream evict */
  NumericStatistic <ub8> *b_zevct;     /* Blitter evict without reuse */
  NumericStatistic <ub8> *b_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *b_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *b_xcevct;    /* X stream evict by C */
  NumericStatistic <ub8> *b_xzevct;    /* X stream evict by Z */
  NumericStatistic <ub8> *b_xtevct;    /* X stream evict by T */
  NumericStatistic <ub8> *b_xpevct;    /* X stream evict by P */
  NumericStatistic <ub8> *b_xrevct;    /* X stream evict by other streams */
  NumericStatistic <ub8> *b_xhit;      /* Blitter inter stream hit */
  NumericStatistic <ub8> *b_cbhit;     /* Blitter C stream hit */
  NumericStatistic <ub8> *b_zbhit;     /* Blitter Z stream hit */
  NumericStatistic <ub8> *b_tbhit;     /* Blitter T stream hit */

  NumericStatistic <ub8> *d_access;    /* DAC stream access */
  NumericStatistic <ub8> *d_miss;      /* DAC stream miss */
  NumericStatistic <ub8> *d_raccess;   /* DAC stream access */
  NumericStatistic <ub8> *d_rmiss;     /* DAC stream miss */
  NumericStatistic <ub8> *d_blocks;    /* DAC stream blocks */
  NumericStatistic <ub8> *d_xevct;     /* DAC inter stream evict */
  NumericStatistic <ub8> *d_sevct;     /* DAC intra stream evict */
  NumericStatistic <ub8> *d_zevct;     /* DAC evict without reuse */
  NumericStatistic <ub8> *d_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *d_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *d_coevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *d_xcevct;    /* X stream evict by C */
  NumericStatistic <ub8> *d_xzevct;    /* X stream evict by Z */
  NumericStatistic <ub8> *d_xtevct;    /* X stream evict by T */
  NumericStatistic <ub8> *d_xpevct;    /* X stream evict by P */
  NumericStatistic <ub8> *d_xrevct;    /* X stream evict by other streams */
  NumericStatistic <ub8> *d_xhit;      /* DAC inter stream hit */
  NumericStatistic <ub8> *d_zdhit;     /* DAC Z stream hit */
  NumericStatistic <ub8> *d_tdhit;     /* DAC T stream hit */
  NumericStatistic <ub8> *d_cdhit;     /* DAC D stream hit */
  NumericStatistic <ub8> *d_bdhit;     /* DAC B stream hit */

  NumericStatistic <ub8> *x_access;    /* Index stream access */
  NumericStatistic <ub8> *x_miss;      /* Index stream miss */
  NumericStatistic <ub8> *x_raccess;   /* Index stream access */
  NumericStatistic <ub8> *x_rmiss;     /* Index stream miss */
  NumericStatistic <ub8> *x_blocks;    /* Index stream blocks */
  NumericStatistic <ub8> *x_xevct;     /* Index inter stream evict */
  NumericStatistic <ub8> *x_sevct;     /* Index intra stream evict */
  NumericStatistic <ub8> *x_zevct;     /* Index evict without reuse */
  NumericStatistic <ub8> *x_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *x_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *x_xcevct;    /* X stream evict by C */
  NumericStatistic <ub8> *x_xzevct;    /* X stream evict by Z */
  NumericStatistic <ub8> *x_xtevct;    /* X stream evict by T */
  NumericStatistic <ub8> *x_xpevct;    /* X stream evict by P */
  NumericStatistic <ub8> *x_xrevct;    /* X stream evict by other streams */
  NumericStatistic <ub8> *x_xhit;      /* Index inter stream hit */

  NumericStatistic <ub8> *p_access;    /* CPU stream access */ 
  NumericStatistic <ub8> *p_miss;      /* CPU stream miss */
  NumericStatistic <ub8> *p_raccess;   /* CPU stream access */ 
  NumericStatistic <ub8> *p_rmiss;     /* CPU stream miss */
  NumericStatistic <ub8> *p_blocks;    /* CPU stream blocks */
  NumericStatistic <ub8> *p_xevct;     /* CPU inter stream evict */
  NumericStatistic <ub8> *p_sevct;     /* CPU intra stream evict */
  NumericStatistic <ub8> *p_zevct;     /* CPU evict without reuse */
  NumericStatistic <ub8> *p_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *p_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *p_xcevct;    /* X stream evict by C */
  NumericStatistic <ub8> *p_xzevct;    /* X stream evict by Z */
  NumericStatistic <ub8> *p_xtevct;    /* X stream evict by T */
  NumericStatistic <ub8> *p_xrevct;    /* X stream evict by T */
  NumericStatistic <ub8> *p_xhit;      /* CPU inter stream hit */

  NumericStatistic <ub8> *g_access;    /* GPGPU stream access */ 
  NumericStatistic <ub8> *g_miss;      /* GPGPU stream miss */
  NumericStatistic <ub8> *g_raccess;   /* GPGPU stream access */ 
  NumericStatistic <ub8> *g_rmiss;     /* GPGPU stream miss */
  NumericStatistic <ub8> *g_blocks;    /* GPGPU stream blocks */
  NumericStatistic <ub8> *g_xevct;     /* GPGPU inter stream evict */
  NumericStatistic <ub8> *g_sevct;     /* GPGPU intra stream evict */
  NumericStatistic <ub8> *g_zevct;     /* GPGPU evict without reuse */
  NumericStatistic <ub8> *g_soevct;    /* Blocks self-evicted without reuse */
  NumericStatistic <ub8> *g_xoevct;    /* Blocks cross-evicted without reuse */
  NumericStatistic <ub8> *g_xcevct;    /* X stream evict by C */
  NumericStatistic <ub8> *g_xzevct;    /* X stream evict by Z */
  NumericStatistic <ub8> *g_xtevct;    /* X stream evict by T */
  NumericStatistic <ub8> *g_xrevct;    /* X stream evict by T */
  NumericStatistic <ub8> *g_xpevct;    /* X stream evict by T */
  NumericStatistic <ub8> *g_xhit;      /* GPGPU inter stream hit */
  NumericStatistic <ub8> *g_xblocks;   /* GPGPU inter stream hit */

  ub8 *per_way_evct[TST + 1];          /* Eviction seen by each way for each stream */
  ub8 reuse_hist[TST + 1][MAX_REUSE + 1]; /* Reuse distance histogram */
  
  ub8 invalid_hits;

/* Returns SARP stream corresponding to access stream based on stream 
 * classification algorithm. */
speedup_stream_type get_srriphint_stream(cachesim_cache *cache, memory_trace *info)
{
  return (speedup_stream_type)(info->sap_stream);
}

void dumpDRAMStats(cachesim_cache *cache)
{
  dram_channel *current_channel;
  dram_rank    *current_rank;
  dram_bank    *current_bank;
  dram_row     *current_row;

  dram_channel *shadow_channel;
  dram_rank    *shadow_rank;
  dram_bank    *shadow_bank;
  dram_row     *shadow_row;

  map<ub8, ub8>::iterator channel_itr;
  map<ub8, ub8>::iterator rank_itr;
  map<ub8, ub8>::iterator bnk_itr;
  map<ub8, ub8>::iterator rw_itr;
  map<ub8, ub8>::iterator blk_itr;

  map<ub8, ub8>::iterator schannel_itr;
  map<ub8, ub8>::iterator srank_itr;
  map<ub8, ub8>::iterator sbnk_itr;
  map<ub8, ub8>::iterator srw_itr;

  ub8 row_count;                        /* #row in all banks */
  ub8 block_count;                      /* #blocks in all banks */
  ub8 gpu_shared_row_count;             /* #GPU shared rows */
  ub8 gpu_shared_row_hits;              /* #GPU shared row hits */
  ub8 gpu_shared_row_misses;            /* #GPU shared row misses */
  ub8 gpu_shared_block_count;           /* #GPU blocks in shared rows */
  ub8 cpu_shared_row_count;             /* #CPU shared rows */
  ub8 cpu_shared_row_hits;              /* #CPU shared row hits */
  ub8 cpu_shared_row_misses;            /* #CPU shared row misses */
  ub8 cpu_shared_block_count;           /* #CPU blocks in shared rows */
  ub8 gpu_unshared_row_count;           /* #GPU unshared rows */
  ub8 gpu_unshared_row_hits;            /* #GPU unshared row hits */
  ub8 gpu_unshared_row_misses;          /* #GPU unshared row misses */
  ub8 gpu_unshared_block_count;         /* #GPU blocks in unshared rows */
  ub8 cpu_unshared_row_count;           /* #CPU unshared rows */
  ub8 cpu_unshared_row_hits;            /* #CPU unshared row hits */
  ub8 cpu_unshared_row_misses;          /* #CPU unshared row misses */
  ub8 cpu_unshared_block_count;         /* #GPU blocks in unshared rows */
  ub8 row_reuse;                        /* # rows reused */
  ub1 rank_count;                       /* # ranks */
  ub1 bank_count;                       /* # banks */
  ub1 current_stream;                   /* Current stream */
  ub1 row_used;                         /* True, if row is already used */
  ub1 gpu_shared;                       /* True, if row is shared by GPU */
  ub1 cpu_shared;                       /* True, if row is shared by CPU */
  ub8 per_stream_blocks[TST + 1];       /* #blocks for each stream */
  ub8 per_stream_row[TST + 1];          /* #rows for each stream */
  ub8 per_stream_shared_row[TST + 1];   /* #shared row for each stream */
  ub1 per_stream_row_used[TST + 1];     /* TRUE, is row is used by a stream */
  
  /* Initialize all counters */
  row_count                 = 0;
  block_count               = 0;
  gpu_shared_row_count      = 0;
  gpu_shared_block_count    = 0;
  gpu_shared_row_hits       = 0;
  gpu_shared_row_misses     = 0;
  cpu_shared_row_count      = 0;
  cpu_shared_block_count    = 0;
  cpu_shared_row_hits       = 0;
  cpu_shared_row_misses     = 0;
  gpu_unshared_row_count    = 0;
  gpu_unshared_block_count  = 0;
  gpu_unshared_row_hits     = 0;
  gpu_unshared_row_misses   = 0;
  cpu_unshared_row_count    = 0;
  cpu_unshared_block_count  = 0;
  cpu_unshared_row_hits     = 0;
  cpu_unshared_row_misses   = 0;
  row_reuse                 = 0;
  rank_count                = 0;
  bank_count                = 0;
  current_channel           = NULL;
  current_rank              = NULL;
  current_bank              = NULL;
  current_row               = NULL;

  memset(per_stream_blocks, 0, sizeof(per_stream_blocks));
  memset(per_stream_row, 0, sizeof(per_stream_row));
  memset(per_stream_shared_row, 0, sizeof(per_stream_shared_row));
  
  /* For each channel in the memory system */
  for (channel_itr = cache->dramsim_channels.begin(); channel_itr != cache->dramsim_channels.end(); channel_itr++)
  {
    current_channel = (dram_channel *)(channel_itr->second);
    assert(current_channel);
    
    /* For each rank connected to each channel */
    for (rank_itr = current_channel->ranks.begin(); rank_itr != current_channel->ranks.end(); rank_itr++)
    {
      current_rank = (dram_rank *)(rank_itr->second);
      assert(current_rank); 
       
      /* For each bank in a rank */
      for (bnk_itr = current_rank->banks.begin(); bnk_itr != current_rank->banks.end(); bnk_itr++)
      {
        current_bank = (dram_bank *)(bnk_itr->second);
        assert(current_bank);

        row_reuse = 0;
        
        /* For each row in a back */
        for (rw_itr = current_bank->bank_rows.begin(); rw_itr != current_bank->bank_rows.end(); rw_itr++)
        {
          current_row = (dram_row *)(rw_itr->second);
          assert(current_row);
          
          /* Reset row use counter */
          memset(per_stream_row_used, 0, sizeof(per_stream_row_used));

          row_used = 0;

          /* Update per stream block count */
          for (blk_itr = current_row->dist_blocks.begin(); blk_itr != current_row->dist_blocks.end(); blk_itr++)
          {
            current_stream = blk_itr->second; 
            assert(current_stream > NN && current_stream <= TST);

            per_stream_blocks[current_stream] += 1;
            
            /* Update row count for a stream */
            if (per_stream_row_used[current_stream] == FALSE)
            {
              per_stream_row_used[current_stream] = TRUE;
              per_stream_row[current_stream]     += 1;
            }
            
            /* Update GPU block count and row count */
            if (GPU_STREAM(current_stream))
            {
              block_count += 1;

              if (row_used == 0)
              {
                row_count += 1;
                row_used   = 1;
              }
            }
          }
          
          row_used = 0;

          ub1 strm;

          /* Check if row is shared by multiple streams */
          for (strm = NN + 1; strm <= TST; strm++)
          {
            if (per_stream_row_used[strm])
            {
              if (row_used)
              {
                row_reuse += 1;

                /* Find #streams sharing a row */
                for (ub1 i = NN + 1; i <= TST; i++)
                {
                  if (per_stream_row_used[i])
                  {
                    per_stream_shared_row[i] += 1;
                  }
                }

                break;
              }

              row_used = 1;
            }
          }

          if (strm == (TST + 1) && current_row->dist_blocks.size())
          {
            for (blk_itr = current_row->dist_blocks.begin(); blk_itr != current_row->dist_blocks.end(); blk_itr++)
            {
              current_stream = blk_itr->second; 
              assert(current_stream > NN && current_stream <= TST);

              if (GPU_STREAM(current_stream))
              {
                gpu_unshared_row_count   += 1; 
                gpu_unshared_row_hits    += current_row->row_hits;
                gpu_unshared_row_misses  += current_row->row_misses;
                gpu_unshared_block_count += current_row->dist_blocks.size();
                break; 
              }

              if (CPU_STREAM(current_stream))
              {
                cpu_unshared_row_count   += 1; 
                cpu_unshared_row_hits    += current_row->row_hits; 
                cpu_unshared_row_misses  += current_row->row_misses; 
                cpu_unshared_block_count += current_row->dist_blocks.size();
                break; 
              }
            }
          }
          else if (strm <= TST)
          {
            gpu_shared = FALSE;
            cpu_shared = FALSE;
            
            for (blk_itr = current_row->dist_blocks.begin(); blk_itr != current_row->dist_blocks.end(); blk_itr++)
            {
              current_stream = blk_itr->second; 
              assert(current_stream > NN && current_stream <= TST);
              
              if (gpu_shared == FALSE && GPU_STREAM(current_stream))
              {
                gpu_shared_row_count  += 1;

                gpu_shared = TRUE;
              }

              if (cpu_shared == FALSE && CPU_STREAM(current_stream))
              {
                cpu_shared_row_count  += 1;

                cpu_shared = TRUE;
              }
            } 
            
            /* Update shared block count */
            for (blk_itr = current_row->dist_blocks.begin(); blk_itr != current_row->dist_blocks.end(); blk_itr++)
            {
              current_stream = blk_itr->second; 
              assert(current_stream > NN && current_stream <= TST);

              if (gpu_shared && GPU_STREAM(current_stream))
              {
                gpu_shared_block_count  += 1;
              }

              if (cpu_shared && CPU_STREAM(current_stream))
              {
                cpu_shared_block_count  += 1;
              }
            }
            
            /* Update GPU and CPU shared/unshared row hit count */
            if (gpu_shared)
            {
              gpu_shared_row_hits   += current_row->row_hits;
              gpu_shared_row_misses += current_row->row_misses;
            }

            if (cpu_shared)
            {
              cpu_shared_row_hits   += current_row->row_hits;
              cpu_shared_row_misses += current_row->row_misses;
            }
          }
          

          current_row->dist_blocks.clear();
        }

        current_bank->bank_rows.clear();
        bank_count++;
      }

      current_rank->banks.clear();
      rank_count++;
    }

    current_channel->ranks.clear();
  }
  
  cache->dramsim_channels.clear();
  
  if (bank_count)
  {
    for (ub1 i = NN + 1; i <= TST; i++)
    {
      per_stream_blocks[i]      /= bank_count;
      per_stream_row[i]         /= bank_count;
      per_stream_shared_row[i]  /= bank_count;

      if (per_stream_row[i])
      {
        per_stream_blocks[i] /= per_stream_row[i];
      }
    }

    row_count                 /= bank_count;
    block_count               /= bank_count;
    gpu_shared_row_count      /= bank_count;
    gpu_shared_row_hits       /= bank_count;
    gpu_shared_row_misses     /= bank_count;
    gpu_shared_block_count    /= bank_count;
    cpu_shared_row_count      /= bank_count;
    cpu_shared_row_hits       /= bank_count;
    cpu_shared_row_misses     /= bank_count;
    cpu_shared_block_count    /= bank_count;
    gpu_unshared_row_count    /= bank_count;
    gpu_unshared_row_hits     /= bank_count;
    gpu_unshared_row_misses   /= bank_count;
    gpu_unshared_block_count  /= bank_count;
    cpu_unshared_row_count    /= bank_count;
    cpu_unshared_row_hits     /= bank_count;
    cpu_unshared_row_misses   /= bank_count;
    cpu_unshared_block_count  /= bank_count;

    if (gpu_shared_row_count)
    {
      gpu_shared_block_count /= gpu_shared_row_count;
    }

    if (gpu_unshared_row_count)
    {
      gpu_unshared_block_count /= gpu_unshared_row_count;
    }

    if (cpu_shared_row_count)
    {
      cpu_shared_block_count /= cpu_shared_row_count;
    }

    if (cpu_unshared_row_count)
    {
      cpu_unshared_block_count /= cpu_unshared_row_count;
    }

    printf("\n%6ld;%3ld;%6ld;%6ld;%6ld;%3ld;%6ld;%6ld;%6ld;%3ld;%6ld;%6ld;%6ld;%3ld;%6ld;%6ld;",
        gpu_shared_block_count, gpu_shared_row_count, gpu_shared_row_hits, gpu_shared_row_misses,
        gpu_unshared_block_count, gpu_unshared_row_count, gpu_unshared_row_hits, gpu_unshared_row_misses,
        cpu_shared_block_count, cpu_shared_row_count, cpu_shared_row_hits, cpu_shared_row_misses,
        cpu_unshared_block_count, cpu_unshared_row_count, cpu_unshared_row_hits, cpu_unshared_row_misses);
  }

  fflush(stdout);
}

void cachesim_dump_per_bank_stats(cachesim_cache *cache, cachesim_cache *shadow_tags, ub1 global_stat = FALSE)
{
  dram_channel *current_channel;
  dram_rank    *current_rank;
  dram_bank    *current_bank;
  dram_row     *current_row;

  dram_channel *shadow_channel;
  dram_rank    *shadow_rank;
  dram_bank    *shadow_bank;
  dram_row     *shadow_row;

  map<ub8, ub8>::iterator channel_itr;
  map<ub8, ub8>::iterator rank_itr;
  map<ub8, ub8>::iterator bnk_itr;
  map<ub8, ub8>::iterator rw_itr;

  map<ub8, ub8>::iterator schannel_itr;
  map<ub8, ub8>::iterator srank_itr;
  map<ub8, ub8>::iterator sbnk_itr;
  map<ub8, ub8>::iterator srw_itr;

  ub8 row_count;
  ub8 interval_count;

  row_count = 0;

#if 0
  printf("\n Average Bank stats per interval \n");
#endif

  for (channel_itr = cache->dramsim_channels.begin(); channel_itr != cache->dramsim_channels.end(); channel_itr++)
  {
    current_channel = (dram_channel *)(channel_itr->second);
    assert(current_channel);
#if 0
    schannel_itr = shadow_tags->dramsim_channels.find(channel_itr->first);
    assert(schannel_itr != shadow_tags->dramsim_channels.end());

    shadow_channel = (dram_channel *)(schannel_itr->second);
#endif
    for (rank_itr = current_channel->ranks.begin(); rank_itr != current_channel->ranks.end(); rank_itr++)
    {
      current_rank = (dram_rank *)(rank_itr->second);
      assert(current_rank); 
#if 0
      srank_itr = shadow_channel->ranks.find(rank_itr->first);
      assert(srank_itr != shadow_channel->ranks.end());

      shadow_rank = (dram_rank *)(srank_itr->second);
#endif
      for (bnk_itr = current_rank->banks.begin(); bnk_itr != current_rank->banks.end(); bnk_itr++)
      {
        current_bank = (dram_bank *)(bnk_itr->second);
        assert(current_bank);
#if 0
        sbnk_itr = shadow_rank->banks.find(bnk_itr->first);
        assert(sbnk_itr != shadow_rank->banks.end());

        shadow_bank = (dram_bank *)(sbnk_itr->second);
#endif

#if 0
        printf ("\n");

        row_count = 0;
        for (rw_itr = current_bank->bank_rows.begin(); rw_itr != current_bank->bank_rows.end(); rw_itr++)
        {
          current_row = (dram_row *)(rw_itr->second);
          assert(current_row);
#if 0
          srw_itr = shadow_bank->bank_rows.find(rw_itr->first);
          assert(srw_itr != shadow_bank->bank_rows.end());

          shadow_row = (dram_row *)(srw_itr->second);

          interval_count = current_row->interval_count;
          if (current_row->global_row_access && interval_count)
#endif      
          if (current_row->row_access)
          {
#if 0
            printf("|%5ld| A: %5ld : %3ld C:%5ld D:%5ld| ", rw_itr->first, 
                (current_row->global_row_access),  
                (shadow_row->global_row_access - current_row->global_row_access),
                (current_row->global_row_critical), 
                (shadow_row->global_btob_hits) - (current_row->global_btob_hits));
            printf("|%5ld| A: %5ld C:%5ld H:%5ld| ", rw_itr->first, 
                current_row->row_access,  
                current_row->row_critical, 
                current_row->row_open);
#endif
            printf("%5ld;%5ld;", rw_itr->first, 
                current_row->row_open);

            if (++row_count == 5)
            {
              row_count = 0;
              printf("\n");
            }
          }
        }
#endif

#if 0
        printf ("\n");

        row_count = 0;
        for (rw_itr = current_bank->bank_rows.begin(); rw_itr != current_bank->bank_rows.end(); rw_itr++)
        {
          current_row = (dram_row *)(rw_itr->second);
          assert(current_row);
#if 0
          srw_itr = shadow_bank->bank_rows.find(rw_itr->first);
          assert(srw_itr != shadow_bank->bank_rows.end());

          shadow_row = (dram_row *)(srw_itr->second);

          interval_count = current_row->interval_count;
          if (current_row->global_row_access && interval_count)
#endif      
          if (current_row->row_access)
          {
#if 0
            printf("|%5ld| A: %5ld : %3ld C:%5ld D:%5ld| ", rw_itr->first, 
                (current_row->global_row_access),  
                (shadow_row->global_row_access - current_row->global_row_access),
                (current_row->global_row_critical), 
                (shadow_row->global_btob_hits) - (current_row->global_btob_hits));
            printf("|%5ld| A: %5ld C:%5ld H:%5ld| ", rw_itr->first, 
                current_row->row_access,  
                current_row->row_critical, 
                current_row->row_open);
#endif
            if (global_stat)
            {
              printf("%5ld;%5ld; ", rw_itr->first, 
                  current_row->g_row_reopen);
            }
            else
            {
              printf("%5ld;%5ld; ", rw_itr->first, 
                  current_row->row_reopen);
            }

            if (++row_count == 5)
            {
              row_count = 0;
              printf("\n");
            }
          }
        }
#endif

        printf("%5ld; ", current_bank->row_reopen);

        printf("\n");
        
        fflush(stdout);

        current_bank->predicted_rows.clear();
#if 0
        printf("\n \n");

        printf ("\n Shadow Bank %ld\n", bnk_itr->first);
        
        ub4 list_size = shadow_bank->access_list.size();
        for (ub4 i = 0; i < list_size; i++)
        {
          printf("%ld ", shadow_bank->access_list.front());
          shadow_bank->access_list.pop_front();
        }
        
        printf("\n");

        row_count = 0;
        for (srw_itr = shadow_bank->bank_rows.begin(); srw_itr != shadow_bank->bank_rows.end(); srw_itr++)
        {
          shadow_row = (dram_row *)(srw_itr->second);
          assert(shadow_row);

          rw_itr = current_bank->bank_rows.find(srw_itr->first);
          if (rw_itr != current_bank->bank_rows.end())
          {
            current_row = (dram_row *)(rw_itr->second);
#if 0
            if (shadow_row->row_access > current_row->row_access)
#endif
            if (shadow_row->row_access)
            {
              printf("|%5ld| A: %5ld C:%5ld D:%5ld| ", srw_itr->first, 
                  shadow_row->row_access,  
                  shadow_row->row_critical, 
                  shadow_row->btob_hits);

              if (++row_count == 5)
              {
                row_count = 0;
                printf("\n");
              }
            }

            if (!current_bank->isDRow(srw_itr->first) && 
                current_bank->predicted_rows.find(srw_itr->first) == current_bank->predicted_rows.end())
            {
              current_bank->predicted_rows.insert(pair<ub8, ub8>(srw_itr->first, 1)); 
            }
          }
          else
          {
            if (shadow_row->row_access)
            {
#if 0
              printf("|%5ld| A: %5ld : %3ld C:%5ld D:%5ld| ", rw_itr->first, 
                  (current_row->global_row_access),  
                  (shadow_row->global_row_access - current_row->global_row_access),
                  (current_row->global_row_critical), 
                  (shadow_row->global_btob_hits) - (current_row->global_btob_hits));
#endif
              printf("|%5ld| A: %5ld C:%5ld D:%5ld| ", srw_itr->first, 
                  shadow_row->row_access,  
                  shadow_row->row_critical, 
                  shadow_row->btob_hits);

              if (++row_count == 5)
              {
                row_count = 0;
                printf("\n");
              }

              if (current_bank->predicted_rows.find(srw_itr->first) == current_bank->predicted_rows.end())
              {
                current_bank->predicted_rows.insert(pair<ub8, ub8>(srw_itr->first, 1)); 
              }
            }
          }
        }
#endif

#if 0        
        row_count = 0;
        printf("\n");
        for (srw_itr = shadow_bank->bank_rows.begin(); srw_itr != shadow_bank->bank_rows.end(); srw_itr++)
        {
          shadow_row = (dram_row *)(srw_itr->second);
          assert(shadow_row);

          rw_itr = current_bank->bank_rows.find(srw_itr->first);
          if (rw_itr != current_bank->bank_rows.end())
          {
            current_row = (dram_row *)(rw_itr->second);
            if (shadow_row->row_access > current_row->row_access)
            {
              printf("%4ld | ", srw_itr->first);
              
              for (ub1 i = 0 ; i < MPAGE_COUNT; i++)
              {
                printf("%3ld", shadow_row->page_access[i]);

                if (++row_count == MPAGE_COUNT)
                {
                  row_count = 0;
                  printf("\n");
                }
              }
            }
          }
          else
          {
            if (shadow_row->row_access)
            {
#if 0
              printf("|%5ld| A: %5ld : %3ld C:%5ld D:%5ld| ", rw_itr->first, 
                  (current_row->global_row_access),  
                  (shadow_row->global_row_access - current_row->global_row_access),
                  (current_row->global_row_critical), 
                  (shadow_row->global_btob_hits) - (current_row->global_btob_hits));
#endif
              printf("%4ld | ", srw_itr->first);
              
              for (ub1 i = 0 ; i < MPAGE_COUNT; i++)
              {
                printf("%3ld", shadow_row->page_access[i]);

                if (++row_count == MPAGE_COUNT)
                {
                  row_count = 0;
                  printf("\n");
                }
              }
            }
          }
        }
#endif

        break;
      }

      break;
    }
    break;
  }
}

void cachesim_update_row_reopen(cachesim_cache *cache)
{
  dram_channel *current_channel;
  dram_rank    *current_rank;
  dram_bank    *current_bank;
  dram_row     *current_row;

  map<ub8, ub8>::iterator channel_itr;
  map<ub8, ub8>::iterator rank_itr;
  map<ub8, ub8>::iterator bnk_itr;
  map<ub8, ub8>::iterator rw_itr;

  ub1 row_count;

  for (channel_itr = cache->dramsim_channels.begin(); channel_itr != cache->dramsim_channels.end(); channel_itr++)
  {
    current_channel = (dram_channel *)(channel_itr->second);
    assert(current_channel);

    for (rank_itr = current_channel->ranks.begin(); rank_itr != current_channel->ranks.end(); rank_itr++)
    {
      current_rank = (dram_rank *)(rank_itr->second);
      assert(current_rank); 

      for (bnk_itr = current_rank->banks.begin(); bnk_itr != current_rank->banks.end(); bnk_itr++)
      {
        current_bank = (dram_bank *)(bnk_itr->second);
        assert(current_bank);

        current_bank->row_reopen    = 0;

        for (rw_itr = current_bank->bank_rows.begin(); rw_itr != current_bank->bank_rows.end(); rw_itr++)
        {
          current_row = (dram_row *)(rw_itr->second);
          assert(current_row);

          current_row->g_row_reopen += current_row->row_reopen;
          current_row->row_reopen    = 0;
        }
      }
    }
  }
}

#define MAX_RANK (2)

static void cachesim_update_interval_end(cachesim_cache *cache)
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
    cache->speedup_stream[i] = FALSE;

    if (SAT_CTR_VAL(cache->sarp_hint[i]) > val)
    {
      val = SAT_CTR_VAL(cache->sarp_hint[i]);
      new_stream = i;
    }
  }

  for (i = 0; i < TST + 1; i++)
  {
    SAT_CTR_RST(cache->sarp_hint[i]);
  }
  
  if (new_stream != NN)
  {
    cache->speedup_stream[new_stream] = TRUE;
  }

  if (cache->speedup_enabled)
  {
    dramsim_set_priority_stream(new_stream);
  }
}

#undef MAX_RANK

#define CHAN(i)             ((i)->newTransactionChan)
#define COLM(i)             ((i)->newTransactionColumn)
#define BANK(i)             ((i)->newTransactionBank)
#define RANK(i)             ((i)->newTransactionRank)
#define ROW(i)              ((i)->newTransactionRow)

void dram_config_init(dram_config *dram_info)
{
  ub4 i;
  ub4 j;
  ub4 k;
  ub1 s;

  for (s = 0; s < TST; s++)
  {
    dram_info->bank[s] = (dram_bank ***)calloc(NUM_CHANS, sizeof(dram_bank ***));
    assert(dram_info);

    for (i = 0; i < NUM_CHANS; i++)
    {
      dram_info->bank[s][i] = (dram_bank **)calloc(NUM_RANKS, sizeof(dram_bank **));
      assert(dram_info->bank[s][i]);

      for (j = 0; j < NUM_RANKS; j++)
      {
        dram_info->bank[s][i][j] = (dram_bank *)calloc(NUM_BANKS, sizeof(dram_bank));
        assert(dram_info->bank[s][i][j]);

        for (k = 0; k < NUM_BANKS; k++)
        {
          dram_info->bank[s][i][j][k].rows = (ub8 *)calloc(NUM_ROWS, sizeof(ub8));
          assert(dram_info->bank[s][i][j][k].rows);

          dram_info->bank[s][i][j][k].state       = DRAM_BANK_IDLE;
          dram_info->bank[s][i][j][k].open_row_id = -1;
        }
      }
    }
  }
  
  dram_info->trans_queue_size     = TQ_SIZE;
  dram_info->trans_queue_entries  = 0;
  dram_info->row_hits             = 0;
  dram_info->row_access           = 0;

  /* Instantiate transaction queue */
  for (i = 0; i < TST + 1; i++)
  {
    dram_info->trans_queue[i] = (cs_qnode *)calloc(HTBLSIZE, sizeof(cs_qnode));
    assert(dram_info->trans_queue);
    
    for (j = 0; j < HTBLSIZE; j++)
    {
      cs_qinit(&(dram_info->trans_queue[i][j]));
    }
  }

  cs_qinit(&(dram_info->trans_inorder_queue));

  dram_info->global_trans_queue = (cs_qnode *)calloc(HTBLSIZE, sizeof(cs_qnode));
  assert(dram_info->trans_queue);

  for (i = 0; i < HTBLSIZE; i++)
  {
    cs_qinit(&(dram_info->global_trans_queue[i]));
  }

  dram_info->bank_list_out = fopen("PerBankInfo.log", "w+");
  assert(dram_info->bank_list_out);
}

ub8 get_row_id(ub8 address)
{
  ub8 physicalAddress;
  ub8 tmpPhysicalAddress;
  ub8 newTransactionChan;
  ub8 newTransactionColumn;
  ub8 newTransactionBank;
  ub8 newTransactionRank;
  ub8 newTransactionRow;
  ub8 tempA;
  ub8 tempB;
  ub8 transactionMask;

  ub4 byteOffsetWidth;
  ub4 transactionSize;
  ub4 channelBitWidth;
  ub4 rankBitWidth;
  ub4 bankBitWidth;
  ub4 rowBitWidth;
  ub4 colBitWidth;
  ub4 colHighBitWidth;
  ub4 colLowBitWidth;
  ub4 llcTagLowBitWidth;

  llcTagLowBitWidth = LOG_BLOCK_SIZE;
  transactionSize   = (JEDEC_DATA_BUS_BITS / 8) * BL;
  transactionMask   = transactionSize - 1;
  channelBitWidth   = log2(NUM_CHANS);
  rankBitWidth      = log2(NUM_RANKS);
  bankBitWidth      = log2(NUM_BANKS);
  rowBitWidth       = log2(NUM_ROWS);
  colBitWidth       = log2(NUM_COLS);

  byteOffsetWidth = log2((JEDEC_DATA_BUS_BITS / 8)); 
  
  tmpPhysicalAddress  = address;
  physicalAddress     = address >> byteOffsetWidth;
  colLowBitWidth      = log2(transactionSize) - byteOffsetWidth;
  physicalAddress     = physicalAddress >> colLowBitWidth;
  colHighBitWidth     = colBitWidth - colLowBitWidth;

  //row:rank:bank:col:chan
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> channelBitWidth;
  tempB = physicalAddress << channelBitWidth;
  newTransactionChan = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> colHighBitWidth;
  tempB = physicalAddress << colHighBitWidth;
  newTransactionColumn = tempA ^ tempB;

  // bank id
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> bankBitWidth;
  tempB = physicalAddress << bankBitWidth;
  newTransactionBank = tempA ^ tempB;

  /* XOR with lower LLC tag bits */
  tempA = tmpPhysicalAddress >> llcTagLowBitWidth;
  tempB = (tempA >> bankBitWidth) << bankBitWidth;
  newTransactionBank = newTransactionBank ^ (tempA ^ tempB);

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rankBitWidth;
  tempB = physicalAddress << rankBitWidth;
  newTransactionRank = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rowBitWidth;
  tempB = physicalAddress << rowBitWidth;
  newTransactionRow = tempA ^ tempB;
  
  return newTransactionRow;
}

ub8 change_row_id(ub8 address, ub8 new_row, ub8 new_col)
{
  ub8 physicalAddress;

  ub4 byteOffsetWidth;
  ub4 transactionSize;
  ub4 channelBitWidth;
  ub4 rankBitWidth;
  ub4 bankBitWidth;
  ub4 rowBitWidth;
  ub4 colBitWidth;
  ub4 colHighBitWidth;
  ub4 colLowBitWidth;
  ub4 llcTagLowBitWidth;
  ub8 address_mask;
  ub8 value_mask;

  llcTagLowBitWidth = LOG_BLOCK_SIZE;
  transactionSize   = (JEDEC_DATA_BUS_BITS / 8) * BL;
  channelBitWidth   = log2(NUM_CHANS);
  rankBitWidth      = log2(NUM_RANKS);
  bankBitWidth      = log2(NUM_BANKS);
  rowBitWidth       = log2(NUM_ROWS);
  colBitWidth       = log2(NUM_COLS);

  byteOffsetWidth = log2((JEDEC_DATA_BUS_BITS / 8)); 
  
  physicalAddress     = address >> byteOffsetWidth;
  colLowBitWidth      = log2(transactionSize) - byteOffsetWidth;
  colHighBitWidth     = colBitWidth - colLowBitWidth;
  
  /* Change row id */
  address_mask = 1;

  for (ub1 i = 1; i < rowBitWidth; i++)
  {
    address_mask <<= 1;
    address_mask  |= 1;
  }
  
  value_mask = address_mask & new_row;
  
  address_mask  <<= (channelBitWidth + rankBitWidth + bankBitWidth + colHighBitWidth + byteOffsetWidth + colLowBitWidth);
  value_mask    <<= (channelBitWidth + rankBitWidth + bankBitWidth + colHighBitWidth + byteOffsetWidth + colLowBitWidth);

#if 0  
  address = ((address & (ub8)(~address_mask)) | value_mask);

  /* Change column id */
  address_mask = 1;

  for (ub1 i = 1; i < colHighBitWidth; i++)
  {
    address_mask <<= 1;
    address_mask  |= 1;
  }
  value_mask = address_mask & new_col;
  
  address_mask  <<= (channelBitWidth + byteOffsetWidth + colLowBitWidth);
  value_mask    <<= (channelBitWidth + byteOffsetWidth + colLowBitWidth);
#endif

  return ((address & (ub8)(~address_mask)) | value_mask);
}

ub8 check_row_id(ub8 address, ub8 new_chn, ub8 new_rnk, ub8 new_bnk, ub8 new_row, ub8 new_col)
{
  ub8 physicalAddress;
  ub8 tmpPhysicalAddress;
  ub8 newTransactionChan;
  ub8 newTransactionColumn;
  ub8 newTransactionBank;
  ub8 newTransactionRank;
  ub8 newTransactionRow;
  ub8 tempA;
  ub8 tempB;
  ub8 transactionMask;

  ub4 byteOffsetWidth;
  ub4 transactionSize;
  ub4 channelBitWidth;
  ub4 rankBitWidth;
  ub4 bankBitWidth;
  ub4 rowBitWidth;
  ub4 colBitWidth;
  ub4 colHighBitWidth;
  ub4 colLowBitWidth;
  ub4 llcTagLowBitWidth;

  llcTagLowBitWidth = LOG_BLOCK_SIZE;
  transactionSize   = (JEDEC_DATA_BUS_BITS / 8) * BL;
  transactionMask   = transactionSize - 1;
  channelBitWidth   = log2(NUM_CHANS);
  rankBitWidth      = log2(NUM_RANKS);
  bankBitWidth      = log2(NUM_BANKS);
  rowBitWidth       = log2(NUM_ROWS);
  colBitWidth       = log2(NUM_COLS);

  byteOffsetWidth = log2((JEDEC_DATA_BUS_BITS / 8)); 
  
  tmpPhysicalAddress  = address;
  physicalAddress     = address >> byteOffsetWidth;
  colLowBitWidth      = log2(transactionSize) - byteOffsetWidth;
  physicalAddress     = physicalAddress >> colLowBitWidth;
  colHighBitWidth     = colBitWidth - colLowBitWidth;

  //row:rank:bank:col:chan
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> channelBitWidth;
  tempB = physicalAddress << channelBitWidth;
  newTransactionChan = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> colHighBitWidth;
  tempB = physicalAddress << colHighBitWidth;
  newTransactionColumn = tempA ^ tempB;

  // bank id
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> bankBitWidth;
  tempB = physicalAddress << bankBitWidth;
  newTransactionBank = tempA ^ tempB;

  /* XOR with lower LLC tag bits */
  tempA = tmpPhysicalAddress >> llcTagLowBitWidth;
  tempB = (tempA >> bankBitWidth) << bankBitWidth;
  newTransactionBank = newTransactionBank ^ (tempA ^ tempB);

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rankBitWidth;
  tempB = physicalAddress << rankBitWidth;
  newTransactionRank = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rowBitWidth;
  tempB = physicalAddress << rowBitWidth;
  newTransactionRow = tempA ^ tempB;
  
  assert(newTransactionChan == new_chn);
  assert(newTransactionRank == new_rnk);
  assert(newTransactionBank == new_bnk);
  assert(newTransactionRow == new_row);
  assert(newTransactionColumn == new_col);
}

#define CACHE_CTIME (.06 * 1000)

ub8 cs_init(cachesim_cache *cache, cache_params params, sb1 *trc_file_name)
{
  assert(cache);

  cs_qinit(&(cache->rdylist));
  cs_qinit(&(cache->gpu_rdylist));
  cs_qinit(&(cache->plist));

  cache->total_mshr = MAX_MSHR;
  cache->free_mshr  = MAX_MSHR;
    
  /* Open trace file stream */
  if (trc_file_name)
  {
    cache->trc_file = gzopen(trc_file_name, "rb9");
    assert(cache->trc_file);
  }
  else
  {
    cache->trc_file = NULL;
  }

  /* Initialize policy specific part of cache */
  cache->cache = cache_init(&params);

  cache->clock_period   = params.clock_period;
  cache->dramsim_enable = params.dramsim_enable;
  cache->dramsim_trace  = params.dramsim_trace;
  cache->shuffle_row    = params.remap_crtcl;
  cache->shuffle_row    = TRUE;
  cache->remap_crtcl    = params.remap_crtcl;
  cache->remap_count    = 0;
  
  for (ub1 s = NN; s <= TST; s++)
  {
    SAT_CTR_INI(cache->sarp_hint[s], ECTR_WIDTH, ECTR_MIN_VAL, ECTR_MAX_VAL);
    SAT_CTR_SET(cache->sarp_hint[s], ECTR_MIN_VAL);
  }
  
  /* Allocate and initialize sampler cache */
  cache->cache_access_count   = 0;
  cache->dram_access_count    = 0;
  
  cache->gpu_fill           = 0;
  cache->gpu_fill_frequency = 1;

  return cache->clock_period;
}

#undef CACHE_CTIME

void cs_fini(cachesim_cache *cache)
{
  cache_free(cache->cache);

  /* Close statistics stream */
  if (cache->trc_file)
  {
    gzclose(cache->trc_file);
  }
}

#define MC_COUNT (2)

int file_can_open_for_read(char *fname)
{
  FILE *f;
  if (!fname[0])
    return 0;
  if (!strcmp(fname, "stdout") || !strcmp(fname, "stderr"))
    return 0;
  f = fopen(fname, "rt");
  if (!f)
    return 0;
  fclose(f);
  return 1;
}

long int dram_init(SimParameters *simP, cache_params params)
{
  long int dram_tck;

  int trans_queue_size = 0;
  
  /* Maximum number of sets in any LLC bank */
  int max_llc_bank_sets = 0;

  /* Next higher power of two, of the number of LLC banks */
  int num_llc_banks_pow_of_2 = 1;
  int llcTagLowBitWidth = 0;

  if (simP->lcP.dramSimConfigFile && !file_can_open_for_read(simP->lcP.dramSimConfigFile))
  {
    printf("%s: cannot read dram configuration file %s", __FUNCTION__, simP->lcP.dramSimConfigFile);
    exit(-1);
  }

#define LLC_BANK        (8)

  max_llc_bank_sets = params.num_sets / LLC_BANK;
  trans_queue_size  = TQ_SIZE;
  
  /* 
   * The DRAM system requires the lowest bit
   * position of LLC tag (llcTagLowBitWidth) for the dram addressing. 
   * This computed as : 
   * llcTagLowBitWidth = log2(block_size) + ceil(log2(num_llc_banks)) + log2(#of_sets_in_LLCbank)
   *
   * */
  while (num_llc_banks_pow_of_2 < LLC_BANK)
    num_llc_banks_pow_of_2 <<= 1;

  llcTagLowBitWidth += LOG_BLOCK_SIZE;
  llcTagLowBitWidth += log2(max_llc_bank_sets);
  llcTagLowBitWidth += log2(num_llc_banks_pow_of_2);

#undef LLC_BANK

  /* Initialize dram device */
  printf("Initializing DRAM controllers:\n");
  printf("\tNumber of DRAM controllers = %d\n", MC_COUNT);
  printf("\tDRAM controller transaction queue size = %d\n", trans_queue_size);
  printf("\tLLC Tag low bit width = %d\n\n", llcTagLowBitWidth);

  dram_tck = dramsim_init(simP->lcP.dramSimConfigFile, (unsigned long long) (dram_size >> 20),
      MC_COUNT, trans_queue_size, llcTagLowBitWidth);

  /* Calculate the tick for CPU and DRAM clocks */
  return dram_tck;
}

#undef MC_COUNT

void dram_fini()
{
  dramsim_done();
}

static void update_dramsim_row_access_stats(cachesim_cache *cache, memory_trace *info)
{
  ub8 physicalAddress;
  ub8 tmpPhysicalAddress;
  ub8 newTransactionChan;
  ub8 newTransactionColumn;
  ub8 newTransactionBank;
  ub8 newTransactionRank;
  ub8 newTransactionRow;
  ub8 tempA;
  ub8 tempB;
  ub8 transactionMask;
  ub8 btob;

  ub4 byteOffsetWidth;
  ub4 transactionSize;
  ub4 channelBitWidth;
  ub4 rankBitWidth;
  ub4 bankBitWidth;
  ub4 rowBitWidth;
  ub4 colBitWidth;
  ub4 colHighBitWidth;
  ub4 colLowBitWidth;
  ub4 llcTagLowBitWidth;

  llcTagLowBitWidth = LOG_BLOCK_SIZE;
  transactionSize   = (JEDEC_DATA_BUS_BITS / 8) * BL;
  transactionMask   = transactionSize - 1;
  channelBitWidth   = log2(NUM_CHANS);
  rankBitWidth      = log2(NUM_RANKS);
  bankBitWidth      = log2(NUM_BANKS);
  rowBitWidth       = log2(NUM_ROWS);
  colBitWidth       = log2(NUM_COLS);
  btob              = 0;

  byteOffsetWidth = log2((JEDEC_DATA_BUS_BITS / 8)); 

  tmpPhysicalAddress  = info->address;
  physicalAddress     = info->address >> byteOffsetWidth;
  colLowBitWidth      = log2(transactionSize) - byteOffsetWidth;
  physicalAddress     = physicalAddress >> colLowBitWidth;
  colHighBitWidth     = colBitWidth - colLowBitWidth;
  
  //row:rank:bank:col:chan
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> channelBitWidth;
  tempB = physicalAddress << channelBitWidth;
  newTransactionChan = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> colHighBitWidth;
  tempB = physicalAddress << colHighBitWidth;
  newTransactionColumn = tempA ^ tempB;

  // bank id
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> bankBitWidth;
  tempB = physicalAddress << bankBitWidth;
  newTransactionBank = tempA ^ tempB;

  /* XOR with lower LLC tag bits */
  tempA = tmpPhysicalAddress >> llcTagLowBitWidth;
  tempB = (tempA >> bankBitWidth) << bankBitWidth;
  newTransactionBank = newTransactionBank ^ (tempA ^ tempB);

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rankBitWidth;
  tempB = physicalAddress << rankBitWidth;
  newTransactionRank = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rowBitWidth;
  tempB = physicalAddress << rowBitWidth;
  newTransactionRow = tempA ^ tempB;

  map<ub8, ub8>::iterator map_itr;
  dram_channel  *current_channel;
  dram_rank     *current_rank;
  dram_bank     *current_bank;
  dram_row      *current_row;

  current_channel = NULL;
  current_rank    = NULL;
  current_bank    = NULL;
  current_row     = NULL;

  /* Find channel, rank , bank , row  and update count */
  map_itr = cache->dramsim_channels.find(newTransactionChan);  

  if (map_itr != cache->dramsim_channels.end())
  {
    current_channel = (dram_channel *)(map_itr->second);
    assert(current_channel);

    map_itr = current_channel->ranks.find(newTransactionRank);

    if (map_itr != current_channel->ranks.end())
    {
      current_rank = (dram_rank *)(map_itr->second);
      assert(current_rank);

      map_itr = current_rank->banks.find(newTransactionBank);
      if (map_itr != current_rank->banks.end())
      {
        current_bank = (dram_bank *)(map_itr->second);

        map_itr = current_bank->bank_rows.find(newTransactionRow);

        if (map_itr != current_bank->bank_rows.end())
        {
          current_row = (dram_row *)(map_itr->second);
          
          /* If block is not in native and migrated set, insert into native 
           * set */
          map_itr = current_row->ntv_blocks.find(info->address);
          if (map_itr == current_row->ntv_blocks.end())
          {
            map_itr = current_row->mig_blocks.find(info->address);
            if (map_itr == current_row->mig_blocks.end())
            {
              current_row->ntv_blocks.insert(pair<ub8, ub8>(info->address, 1));
              current_row->ntv_cols.insert(pair<ub8, ub8>(newTransactionColumn, 0));
            }

            map_itr = current_row->avl_cols.find(newTransactionColumn);
            if (map_itr != current_row->avl_cols.end())
            {
              current_row->avl_cols.erase(newTransactionColumn); 
            }
          }
        }
      }
    }
  }
}

static void update_stream_row_stats(cachesim_cache *cache, cachesim_cache *shadow_tags, 
    ub1 remap, memory_trace *info)
{
  ub8 physicalAddress;
  ub8 tmpPhysicalAddress;
  ub8 newTransactionChan;
  ub8 newTransactionColumn;
  ub8 newTransactionBank;
  ub8 newTransactionRank;
  ub8 newTransactionRow;
  ub8 tempA;
  ub8 tempB;
  ub8 transactionMask;
  ub8 current_btob;
  ub8 shadow_btob;

  ub4 byteOffsetWidth;
  ub4 transactionSize;
  ub4 channelBitWidth;
  ub4 rankBitWidth;
  ub4 bankBitWidth;
  ub4 rowBitWidth;
  ub4 colBitWidth;
  ub4 colHighBitWidth;
  ub4 colLowBitWidth;
  ub4 llcTagLowBitWidth;
  
  map<ub8, ub8>::iterator map_itr;
  map<ub8, ub8>::iterator old_row_itr;
  map<ub8, ub8>::iterator new_row_itr;
  
  stream_row_data *old_row_data;
  stream_row_data *new_row_data;
  
  old_row_data      = NULL;
  new_row_data      = NULL;
  llcTagLowBitWidth = LOG_BLOCK_SIZE;
  transactionSize   = (JEDEC_DATA_BUS_BITS / 8) * BL;
  transactionMask   = transactionSize - 1;
  channelBitWidth   = log2(NUM_CHANS);
  rankBitWidth      = log2(NUM_RANKS);
  bankBitWidth      = log2(NUM_BANKS);
  rowBitWidth       = log2(NUM_ROWS);
  colBitWidth       = log2(NUM_COLS);

  byteOffsetWidth = log2((JEDEC_DATA_BUS_BITS / 8)); 

  tmpPhysicalAddress  = info->address;
  physicalAddress     = info->address >> byteOffsetWidth;
  colLowBitWidth      = log2(transactionSize) - byteOffsetWidth;
  physicalAddress     = physicalAddress >> colLowBitWidth;
  colHighBitWidth     = colBitWidth - colLowBitWidth;

  //row:rank:bank:col:chan
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> channelBitWidth;
  tempB = physicalAddress << channelBitWidth;
  newTransactionChan = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> colHighBitWidth;
  tempB = physicalAddress << colHighBitWidth;
  newTransactionColumn = tempA ^ tempB;

  // bank id
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> bankBitWidth;
  tempB = physicalAddress << bankBitWidth;
  newTransactionBank = tempA ^ tempB;

  /* XOR with lower LLC tag bits */
  tempA = tmpPhysicalAddress >> llcTagLowBitWidth;
  tempB = (tempA >> bankBitWidth) << bankBitWidth;
  newTransactionBank = newTransactionBank ^ (tempA ^ tempB);

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rankBitWidth;
  tempB = physicalAddress << rankBitWidth;
  newTransactionRank = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rowBitWidth;
  tempB = physicalAddress << rowBitWidth;
  newTransactionRow = tempA ^ tempB;

#define NRANK                           ((newTransactionRank * NUM_RANKS) + newTransactionChan)
#define NBANK                           (newTransactionBank)
#define STREAM_ROW(c, r, b, s)          ((c)->per_stream_row[r][b].current_row[s])
#define ROW_TO_STREAM(c, r, b)          ((c)->per_stream_row[r][b].row_to_stream_count)
#define STREAM_ACCESS(c, r, b, s)       ((c)->per_stream_row[r][b].current_access[s])
#define STREAM_IACCESS(c, r, b, s)      ((c)->per_stream_row[r][b].interval_access[s])
#define STREAM_TOTAL(c, r, b, s)        ((c)->per_stream_row[r][b].total_access[s])
#define STREAM_INTERVAL(c, r, b, s)     ((c)->per_stream_row[r][b].total_interval[s])
#define INTERVAL_PAIR(c, r, b, s, v)    (pair<ub8, ub8>(STREAM_INTERVAL(c, r, b, s), v))
#define STREAM_ADD_IVL(c, r, b, s, v)   (STREAM_IACCESS(c, r, b, s).insert(INTERVAL_PAIR(c, r, b, s, v)))
#define STREAM_FIND_IVL(c, r, b, s)     (STREAM_IACCESS(c, r, b, s).find(STREAM_INTERVAL(c, r, b, s)))
#define STREAM_S_IACCESS(c, r, b, s)    ((c)->per_stream_row[r][b].interval_shared_access[s])
#define STREAM_S_TOTAL(c, r, b, s)      ((c)->per_stream_row[r][b].total_shared_access[s])
#define STREAM_S_INTERVAL(c, r, b, s)   ((c)->per_stream_row[r][b].total_shared_interval[s])
#define S_INTERVAL_PAIR(c, r, b, s, v)  (pair<ub8, ub8>(STREAM_S_INTERVAL(c, r, b, s), v))
#define STREAM_ADD_S_IVL(c, r, b, s, v) (STREAM_S_IACCESS(c, r, b, s).insert(S_INTERVAL_PAIR(c, r, b, s, v)))
#define STREAM_FIND_S_IVL(c, r, b, s)   (STREAM_S_IACCESS(c, r, b, s).find(STREAM_S_INTERVAL(c, r, b, s)))

  /* If stream has access to new row, update access and interval counter */
  if (STREAM_ROW(cache, NRANK, NBANK, info->stream) != newTransactionRow)
  {
    /* If row is present in row_to_stream map, get old row data and update 
     * stream count for the row */
    old_row_itr = ROW_TO_STREAM(cache, NRANK, NBANK).find(STREAM_ROW(cache, NRANK, NBANK, info->stream));
    if (old_row_itr != ROW_TO_STREAM(cache, NRANK, NBANK).end())
    {
      old_row_data = (stream_row_data *)(old_row_itr->second);
      if (old_row_data->streams)
      {
        /* Update total access by stream in total access counter. */
        old_row_data->total_access += STREAM_ACCESS(cache, NRANK, NBANK, info->stream);

        /* Decrement streams accessed the row by one */
        old_row_data->streams -= 1;
      }
    }
    
    /* Update sharer count for newly accessed row. If row is accessed for the
     * first time, insert new row in row_to_stream map. */
    new_row_itr = ROW_TO_STREAM(cache, NRANK, NBANK).find(newTransactionRow);
    if (new_row_itr != ROW_TO_STREAM(cache, NRANK, NBANK).end())
    {
      new_row_data = (stream_row_data *)(new_row_itr->second);
      assert(new_row_data);

      new_row_data->streams += 1;

      assert(new_row_data->streams <= TST);
    }
    else
    {
      new_row_data = new stream_row_data;
      assert(new_row_data);
      
      memset(new_row_data, 0, sizeof(stream_row_data));

      new_row_data->streams = 1;

      ROW_TO_STREAM(cache, NRANK, NBANK).insert(pair<ub8, ub8>(newTransactionRow, (ub8)new_row_data));
    }
    
    assert(new_row_data);

    new_row_data->sharer[info->stream] = 1;

    if (STREAM_ACCESS(cache, NRANK, NBANK, info->stream) && old_row_data && old_row_data->streams == 0)
    {
      ub1 sharer_count = 0;
      
      for (ub1 s = NN + 1; s <= TST; s++)
      {
        if (old_row_data->sharer[s])
        {
          sharer_count++;
        }
      }

      assert(sharer_count);

      for (ub1 s = NN + 1; s <= TST; s++)
      {
        if (old_row_data->sharer[s])
        {
          if (sharer_count == 1)
          {
            map_itr = STREAM_FIND_IVL(cache, NRANK, NBANK, s);
            if (map_itr == cache->per_stream_row[NRANK][NBANK].interval_access[s].end())
            {
              STREAM_ADD_IVL(cache, NRANK, NBANK, s, old_row_data->total_access);
            }

            STREAM_TOTAL(cache, NRANK, NBANK, s)    += old_row_data->total_access;
            STREAM_INTERVAL(cache, NRANK, NBANK, s) += 1;
          }
          else
          {
            map_itr = STREAM_FIND_S_IVL(cache, NRANK, NBANK, s);
            if (map_itr == cache->per_stream_row[NRANK][NBANK].interval_shared_access[s].end())
            {
              STREAM_ADD_S_IVL(cache, NRANK, NBANK, s, old_row_data->total_access);
            }

            STREAM_S_TOTAL(cache, NRANK, NBANK, s)    += old_row_data->total_access;
            STREAM_S_INTERVAL(cache, NRANK, NBANK, s) += 1;
          }

          old_row_data->sharer[s] = 0;
        }
      }

      delete old_row_data;

      ROW_TO_STREAM(cache, NRANK, NBANK).erase(STREAM_ROW(cache, NRANK, NBANK, info->stream));
    }

    STREAM_ROW(cache, NRANK, NBANK, info->stream)     = newTransactionRow;
    STREAM_ACCESS(cache, NRANK, NBANK, info->stream)  = 0;
  }
  else
  {
    STREAM_ACCESS(cache, NRANK, NBANK, info->stream) += 1;
  }

#undef NRANK
#undef NBANK
#undef STREAM_ROW
#undef STREAM_ACCESS
#undef STREAM_IACCESS
#undef STREAM_TOTAL
#undef STREAM_INTERVAL
#undef INTERVAL_PAIR
#undef STREAM_ADD_IVL
#undef STREAM_FIND_IVL
#undef STREAM_S_IACCESS
#undef STREAM_S_TOTAL
#undef STREAM_S_INTERVAL
#undef S_INTERVAL_PAIR
#undef STREAM_ADD_S_IVL
#undef STREAM_FIND_S_IVL
}

static void update_dramsim_open_row_stats(cachesim_cache *cache, cachesim_cache *shadow_tags, 
    ub1 remap, memory_trace *info)
{
  ub8 physicalAddress;
  ub8 tmpPhysicalAddress;
  ub8 newTransactionChan;
  ub8 newTransactionColumn;
  ub8 newTransactionBank;
  ub8 newTransactionRank;
  ub8 newTransactionRow;
  ub8 tempA;
  ub8 tempB;
  ub8 transactionMask;
  ub8 current_btob;
  ub8 shadow_btob;

  ub4 byteOffsetWidth;
  ub4 transactionSize;
  ub4 channelBitWidth;
  ub4 rankBitWidth;
  ub4 bankBitWidth;
  ub4 rowBitWidth;
  ub4 colBitWidth;
  ub4 colHighBitWidth;
  ub4 colLowBitWidth;
  ub4 llcTagLowBitWidth;

  llcTagLowBitWidth = LOG_BLOCK_SIZE;
  transactionSize   = (JEDEC_DATA_BUS_BITS / 8) * BL;
  transactionMask   = transactionSize - 1;
  channelBitWidth   = log2(NUM_CHANS);
  rankBitWidth      = log2(NUM_RANKS);
  bankBitWidth      = log2(NUM_BANKS);
  rowBitWidth       = log2(NUM_ROWS);
  colBitWidth       = log2(NUM_COLS);

  byteOffsetWidth = log2((JEDEC_DATA_BUS_BITS / 8)); 

  tmpPhysicalAddress  = info->address;
  physicalAddress     = info->address >> byteOffsetWidth;
  colLowBitWidth      = log2(transactionSize) - byteOffsetWidth;
  physicalAddress     = physicalAddress >> colLowBitWidth;
  colHighBitWidth     = colBitWidth - colLowBitWidth;

  //row:rank:bank:col:chan
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> channelBitWidth;
  tempB = physicalAddress << channelBitWidth;
  newTransactionChan = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> colHighBitWidth;
  tempB = physicalAddress << colHighBitWidth;
  newTransactionColumn = tempA ^ tempB;

  // bank id
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> bankBitWidth;
  tempB = physicalAddress << bankBitWidth;
  newTransactionBank = tempA ^ tempB;

  /* XOR with lower LLC tag bits */
  tempA = tmpPhysicalAddress >> llcTagLowBitWidth;
  tempB = (tempA >> bankBitWidth) << bankBitWidth;
  newTransactionBank = newTransactionBank ^ (tempA ^ tempB);

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rankBitWidth;
  tempB = physicalAddress << rankBitWidth;
  newTransactionRank = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rowBitWidth;
  tempB = physicalAddress << rowBitWidth;
  newTransactionRow = tempA ^ tempB;

  map<ub8, ub8>::iterator map_itr;
  dram_channel  *current_channel;
  dram_rank     *current_rank;
  dram_bank     *current_bank;
  dram_row      *current_row;
  
  current_channel = NULL;
  current_rank    = NULL;
  current_bank    = NULL;
  current_row     = NULL;
  
  /* Find channel, rank , bank , row  and update count */
  map_itr = cache->dramsim_channels.find(newTransactionChan);  
  if (map_itr == cache->dramsim_channels.end())
  {
    current_channel = new dram_channel();
    assert(current_channel);

    current_rank = new dram_rank();
    assert(current_rank);

    current_bank = new dram_bank();
    assert(current_bank);

    current_row = new dram_row();
    assert(current_row);
    
    current_bank->row_reopen            = 0;
    current_row->global_row_access      = 0;
    current_row->global_row_critical    = 0;
    current_row->global_row_open        = 0;
    current_row->global_btob_hits       = 0;
    current_row->interval_count         = 0;
    current_row->row_access             = 0;
    current_row->row_hits               = 0;
    current_row->row_misses             = 0;
    current_row->row_critical           = 0;
    current_row->row_open               = 0;
    current_row->row_reopen             = 0;
    current_row->g_row_reopen           = 0;
    current_row->btob_hits              = 0;
    current_row->max_btob_hits          = 0;
    current_row->periodic_row_access    = 0;
    
    for (ub4 i = 0; i < MPAGE_COUNT; i++)
    {
      current_row->page_access[i] = 0;
    }

    for (ub4 i = 0; i < NUM_COLS / 8; i++)
    {
      current_row->avl_cols.insert(pair<ub8, ub8>(i, 0)); 
    }

    current_bank->bank_rows.insert(pair<ub8, ub8>(newTransactionRow, (ub8)current_row));

    current_rank->banks.insert(pair<ub8, ub8>(newTransactionBank, (ub8)current_bank));

    current_channel->ranks.insert(pair<ub8, ub8>(newTransactionRank, (ub8)current_rank));

    /* Find Rank */  
    cache->dramsim_channels.insert(pair<ub8, ub8>(newTransactionChan , (ub8)current_channel));
  }
  else
  {
    current_channel = (dram_channel *)(map_itr->second);
    assert(current_channel);

    map_itr = current_channel->ranks.find(newTransactionRank);
    if (map_itr != current_channel->ranks.end())
    {
      current_rank = (dram_rank *)(map_itr->second);
      assert(current_rank);

      map_itr = current_rank->banks.find(newTransactionBank);
      if (map_itr == current_rank->banks.end())
      {
        current_bank = new dram_bank();
        assert(current_bank);

        current_bank->row_reopen  = 0;
    
        current_rank->banks.insert(pair<ub8, ub8>(newTransactionBank, (ub8)current_bank));
      }
      else
      {
        current_bank = (dram_bank *)(map_itr->second);
      }
      
      map_itr = current_bank->bank_rows.find(newTransactionRow);
      if (map_itr == current_bank->bank_rows.end())
      {
        current_row = new dram_row();
        assert(current_row);
        
        current_row->global_row_access      = 0;
        current_row->global_row_critical    = 0;
        current_row->global_row_open        = 0;
        current_row->global_btob_hits       = 0;
        current_row->interval_count         = 0;
        current_row->row_access             = 0;
        current_row->row_hits               = 0;
        current_row->row_misses             = 0;
        current_row->row_critical           = 0;
        current_row->btob_hits              = 0;
        current_row->max_btob_hits          = 0;
        current_row->periodic_row_access    = 0;
        current_row->row_open               = 0;
        current_row->row_reopen             = 0;
        current_row->g_row_reopen           = 0;

        for (ub4 i = 0; i < MPAGE_COUNT; i++)
        {
          current_row->page_access[i] = 0;
        }

        for (ub4 i = 0; i < NUM_COLS / 8; i++)
        {
          current_row->avl_cols.insert(pair<ub8, ub8>(i, 0)); 
        }

        current_bank->bank_rows.insert(pair<ub8, ub8>(newTransactionRow, (ub8)current_row));      
      }
      else
      {
        current_row = (dram_row *)(map_itr->second);
      }
    }
    else
    {
      current_rank = new dram_rank();
      assert(current_rank);

      current_bank = new dram_bank();
      assert(current_bank);

      current_row = new dram_row();
      assert(current_row);
        
      current_bank->row_reopen            = 0;

      current_row->global_row_access      = 0;
      current_row->global_row_critical    = 0;
      current_row->global_row_open        = 0;
      current_row->global_btob_hits       = 0;
      current_row->interval_count         = 0;
      current_row->row_access             = 0;
      current_row->row_hits               = 0;
      current_row->row_misses             = 0;
      current_row->row_critical           = 0;
      current_row->btob_hits              = 0;
      current_row->max_btob_hits          = 0;
      current_row->row_open               = 0;
      current_row->row_reopen             = 0;
      current_row->g_row_reopen           = 0;
      current_row->periodic_row_access    = 0;

      for (ub4 i = 0; i < MPAGE_COUNT; i++)
      {
        current_row->page_access[i] = 0;
      }

      for (ub4 i = 0; i < NUM_COLS / 8; i++)
      {
        current_row->avl_cols.insert(pair<ub8, ub8>(i, 0)); 
      }

      current_bank->bank_rows.insert(pair<ub8, ub8>(newTransactionRow, (ub8)current_row));
      current_rank->banks.insert(pair<ub8, ub8>(newTransactionBank, (ub8)current_bank));
      current_channel->ranks.insert(pair<ub8, ub8>(newTransactionRank, (ub8)current_rank));
    }
  }
  
  current_row->row_access           += 1;
  current_row->periodic_row_access  += 1;
  current_bank->open_row_id          = dramsim_get_open_row(info);

  map_itr = current_row->dist_blocks.find(BLCKALIGN(info->address));
  if (map_itr == current_row->dist_blocks.end())
  {
    current_row->dist_blocks.insert(pair<ub8, ub8>(BLCKALIGN(info->address), info->stream));
  }
  
#if 0  
  current_row->page_access[MPAGE_OFFSET(newTransactionColumn)] += 1;

  if (remap)
  {
    map_itr = current_row->mig_blocks.find(info->address);
    if (map_itr == current_row->mig_blocks.end())
    {
      current_row->mig_blocks.insert(pair<ub8, ub8>(info->address, 0));
    }
  }

  if (info && info->sap_stream == speedup_stream_x)
  {
    current_row->row_critical += 1;
  }

  /* Update per-bank row stats */
  map_itr = current_row->request_queue.find(info->address);
  if (map_itr ==  current_row->request_queue.end())
  {
#if 0
    assert(map_itr ==  current_row->request_queue.end());
#endif
    current_row->request_queue.insert(pair<ub8, ub8>(info->address, 1));
  }

  map_itr = current_row->row_blocks.find(info->address);
  if (map_itr == current_row->row_blocks.end())
  {
    current_row->row_blocks.insert(pair<ub8, ub8>(info->address, 1));
  }

  if (current_row->all_blocks.find(info->address) == current_row->all_blocks.end())
  {
    current_row->all_blocks.insert(pair<ub8, ub8>(info->address, 1));
  }

  map_itr = current_row->dist_blocks.find(info->address);
  if (map_itr == current_row->dist_blocks.end())
  {
    current_row->dist_blocks.insert(pair<ub8, ub8>(info->address, info->stream));
  }

  if (newTransactionRow == current_bank->open_row_id)
  {
    current_row->btob_hits += current_row->request_queue.size();
    current_row->request_queue.clear();
  }
  else
  {
    if (current_row->btob_hits > current_row->max_btob_hits)
    {
      current_row->max_btob_hits = current_row->btob_hits;
    }
  }

  if (++cache->dram_access_count >= ROW_ACCESS_INTERVAL)
  {
    cachesim_dump_per_bank_stats(cache, shadow_tags);

    cachesim_update_row_reopen(cache);

    cache->dram_access_count = 0;
  }
#endif
}

static void update_dramsim_access_completion_stats(cachesim_cache *cache, memory_trace *info)
{
  ub8 physicalAddress;
  ub8 tmpPhysicalAddress;
  ub8 newTransactionChan;
  ub8 newTransactionColumn;
  ub8 newTransactionBank;
  ub8 newTransactionRank;
  ub8 newTransactionRow;
  ub8 tempA;
  ub8 tempB;
  ub8 transactionMask;

  ub4 byteOffsetWidth;
  ub4 transactionSize;
  ub4 channelBitWidth;
  ub4 rankBitWidth;
  ub4 bankBitWidth;
  ub4 rowBitWidth;
  ub4 colBitWidth;
  ub4 colHighBitWidth;
  ub4 colLowBitWidth;
  ub4 llcTagLowBitWidth;

  llcTagLowBitWidth = LOG_BLOCK_SIZE;
  transactionSize   = (JEDEC_DATA_BUS_BITS / 8) * BL;
  transactionMask   = transactionSize - 1;
  channelBitWidth   = log2(NUM_CHANS);
  rankBitWidth      = log2(NUM_RANKS);
  bankBitWidth      = log2(NUM_BANKS);
  rowBitWidth       = log2(NUM_ROWS);
  colBitWidth       = log2(NUM_COLS);

  byteOffsetWidth = log2((JEDEC_DATA_BUS_BITS / 8)); 

  tmpPhysicalAddress  = info->address;
  physicalAddress     = info->address >> byteOffsetWidth;
  colLowBitWidth      = log2(transactionSize) - byteOffsetWidth;
  physicalAddress     = physicalAddress >> colLowBitWidth;
  colHighBitWidth     = colBitWidth - colLowBitWidth;

  //row:rank:bank:col:chan
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> channelBitWidth;
  tempB = physicalAddress << channelBitWidth;
  newTransactionChan = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> colHighBitWidth;
  tempB = physicalAddress << colHighBitWidth;
  newTransactionColumn = tempA ^ tempB;

  // bank id
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> bankBitWidth;
  tempB = physicalAddress << bankBitWidth;
  newTransactionBank = tempA ^ tempB;

  /* XOR with lower LLC tag bits */
  tempA = tmpPhysicalAddress >> llcTagLowBitWidth;
  tempB = (tempA >> bankBitWidth) << bankBitWidth;
  newTransactionBank = newTransactionBank ^ (tempA ^ tempB);

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rankBitWidth;
  tempB = physicalAddress << rankBitWidth;
  newTransactionRank = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rowBitWidth;
  tempB = physicalAddress << rowBitWidth;
  newTransactionRow = tempA ^ tempB;

  map<ub8, ub8>::iterator map_itr;
  dram_channel  *current_channel;
  dram_rank     *current_rank;
  dram_bank     *current_bank;
  dram_row      *current_row;
  
  current_channel = NULL;
  current_rank    = NULL;
  current_bank    = NULL;
  current_row     = NULL;
  
  /* Find channel, rank , bank , row  and update count */
  map_itr = cache->dramsim_channels.find(newTransactionChan);  
  if (map_itr != cache->dramsim_channels.end())
  {
    current_channel = (dram_channel *)(map_itr->second);
    assert(current_channel);

    map_itr = current_channel->ranks.find(newTransactionRank);
    if (map_itr != current_channel->ranks.end())
    {
      current_rank = (dram_rank *)(map_itr->second);
      assert(current_rank);

      map_itr = current_rank->banks.find(newTransactionBank);
      if (map_itr != current_rank->banks.end())
      {
        current_bank = (dram_bank *)(map_itr->second);

        map_itr = current_bank->bank_rows.find(newTransactionRow);
        if (map_itr != current_bank->bank_rows.end())
        {
          current_row = (dram_row *)(map_itr->second);

          if (info->prefetch)
          {
            current_row->row_hits += 1;
          }
          else
          {
            current_row->row_misses += 1;
          }
        }
      }
    }
  }
}

#define PG_ACCESS_CNT(i) (((i)->cluster)->page_access[(i)->cluster_off])
#define PG_ACCESS(i)     (PG_ACCESS_CNT(i))

static bool compare_frame_info(pair<ub8, ub8> info1, pair <ub8, ub8> info2)
{
  return (PG_ACCESS((page_info *)(info1.second)) < PG_ACCESS((page_info *)(info2.second)));
}

static bool compare_frame_info_rev(pair<ub8, ub8>info1, pair<ub8, ub8> info2)
{
  return (PG_ACCESS((page_info *)(info1.second)) > PG_ACCESS((page_info *)(info2.second)));
}

#undef PG_ACCESS_CNT
#undef PG_ACCESS

/*
 *
 * Dynamic page mapping:
 *
 *  An access that belongs to an non-critical stream and is above a threshold is mapped
 *  onto a unused page of a critical cluster.
 *
 * */

#define MSHR_FREE(c)  ((c)->free_mshr)

static ub1 cs_is_mshr_allocated(cachesim_cache *cache, memory_trace *info)
{
  ub8 tag;
  ub1 ret;
  map<ub8, cs_mshr*>::iterator itr;

  ret = FALSE;
  tag = BLCKALIGN(info->address);

  /* Find mshr entry */
  itr = (cache->mshr).find(tag);
  ret = (itr != (cache->mshr).end());

  return ret;
}

static ub1 cs_alloc_mshr(cachesim_cache *cache, memory_trace *info)
{
  ub1       ret;
  ub8       tag;
  cs_mshr  *mshr;
  map<ub8, cs_mshr*>::iterator itr;
  map<ub8, cs_mshr*> tmp;

  ret = CS_MSHR_FAIL;
  tag = BLCKALIGN(info->address);
  
  if (MSHR_FREE(cache))
  {
    /* Find mshr entry */
    itr = (cache->mshr).find(tag);

    if (itr == (cache->mshr).end())
    {
      mshr = (cs_mshr *)calloc(1, sizeof(cs_mshr));
      assert(mshr);
        
      cs_qinit(&(mshr->rlist));

      (cache->mshr).insert(pair<ub8, cs_mshr*>(tag, mshr));

      ret = CS_MSHR_ALLOC;

      MSHR_FREE(cache) -= 1;
    }
    else
    {
      /* If entry already allocated, insert request into retry list */
      mshr = (cs_mshr *)(itr->second);
      assert(mshr);

      cs_qappend(&(mshr->rlist), (ub1 *)info);

      ret = CS_MSHR_WAIT;
    }
  }

  return ret;
}

static void cs_free_mshr(cachesim_cache *cache, memory_trace *info)
{
  ub8      tag;
  cs_mshr *mshr;
  map<ub8, cs_mshr*>::iterator itr;

  tag = BLCKALIGN(info->address);

  /* Find mshr entry */
  itr = (cache->mshr).find(tag);
  assert(itr != (cache->mshr).end());

  mshr = itr->second;

  assert(mshr);
  
  /* Enqueue waiting request in to pending request list */
  while (!cs_qempty(&(mshr->rlist)))
  {
    info = (memory_trace *)cs_qdequeue(&(mshr->rlist));
    assert(info);
    
    cs_qappend(&(cache->plist), (ub1 *)info);
  }

  /* Free retry list and mshr entry */
  free(itr->second);

  (cache->mshr).erase(itr);

  MSHR_FREE(cache) += 1;
}

#undef MSHR_FREE

cache_access_status cachesim_fill_block(cachesim_cache *cache, memory_trace *info)
{
  struct cache_block_t     vctm_block;
  enum cache_block_state_t vctm_state;
  cache_access_status      ret;
  long long                vctm_tag;
  memory_trace            *info_out;

  ub8 addr;
  sb8 way;
  ub8 indx;
  sb4 vctm_stream;
  ub1 mshr_alloc;

  assert(cache);
  indx        = ADDR_NDX(cache, info->address);
  addr        = BLCKALIGN(info->address); 
  ret.way     = -1;
  ret.fate    = CACHE_ACCESS_UNK;
  ret.stream  = NN;
  ret.dirty   = 0;
  
  if (info && (info->spill == FALSE || (info->spill == TRUE && IS_SPILL_ALLOCATED(info))))
  {
    way = cache_replace_block(cache->cache, indx, info);
    assert(way != -1);

    vctm_block = cache_get_block(cache->cache, indx, way, &vctm_tag, &vctm_state, 
        &vctm_stream);

    if (way != BYPASS_WAY)
    {
      assert(vctm_stream >= 0 && vctm_stream <= TST);
    
      ret.way       = way;
      ret.fate      = CACHE_ACCESS_MISS;

      /* If current block is valid */
      if (vctm_state != cache_block_invalid)
      {
        ret.tag       = vctm_block.tag;
        ret.vtl_addr  = vctm_block.vtl_addr;
        ret.stream    = vctm_stream;
        ret.dirty     = vctm_block.dirty;
        ret.epoch     = vctm_block.epoch;
        ret.access    = vctm_block.access;
        ret.last_rrpv = vctm_block.last_rrpv;
        ret.fill_rrpv = vctm_block.fill_rrpv;

        if (vctm_block.dirty && cache->dramsim_enable && !cache->shadow_tag)
        {
          info_out = (memory_trace *)calloc(1, sizeof(memory_trace));
          assert(info_out);

          info_out->address     = vctm_block.tag;
          info_out->stream      = vctm_stream;
          info_out->spill       = TRUE;
          info_out->sap_stream  = vctm_block.sap_stream;
          info_out->vtl_addr    = info_out->address;
          info_out->prefetch    = FALSE;

          /* As mshr have been freed for the completed fill. So, it must be available. */
          mshr_alloc = cs_alloc_mshr(cache, info_out);
          assert(mshr_alloc == CS_MSHR_ALLOC);

          /* Allocate DRAM timing stats structure before issuing a DRAM request */
          info_out->policy_data = (void *)calloc(1, sizeof(dram_queue_time));

          if (cache->dramsim_enable)
          {
            update_dramsim_open_row_stats(cache, NULL, FALSE, info_out);
          }

          dramsim_request(TRUE, BLCKALIGN(info_out->address), info_out->stream, info_out); 

          ret.fate  = CACHE_ACCESS_RPLC;

          pending_requests += 1;
        }
        else
        {
          ret.fate  = CACHE_ACCESS_RPLC;
        }

        cache_set_block(cache->cache, indx, way, vctm_tag, cache_block_invalid, 
            vctm_stream, info);

        cache_fill_block(cache->cache, indx, way, addr, cache_block_exclusive,
            info->stream, info);

        /* Update per-stream per-way eviction */
        per_way_evct[vctm_stream][way] += 1;
      }
      else
      {
        assert(vctm_stream == NN);

        if (way != BYPASS_WAY)
        {
          cache_fill_block(cache->cache, indx, way, addr, cache_block_exclusive,
              info->stream, info);
        }
      }
    }
    else
    {
      ret.way   = way;
      ret.fate  = CACHE_ACCESS_MISS;
    }

    ub8 index;         /* Histogram bucket index */
    ub1 stream_indx;   /* Stream number */

    /* Update reuse count histogram */
    stream_indx = (ret.stream >= PS) ? PS : ret.stream;
    index       = (vctm_block.access < MAX_REUSE) ? vctm_block.access : MAX_REUSE;

    reuse_hist[stream_indx][index] += 1;
  } 
  else
  {
    ret.fate  = CACHE_ACCESS_MISS;
  }

  return ret;
}

cache_access_status cachesim_incl_cache(cachesim_cache *cache, cachesim_cache *shadow_tags, 
    ub8 addr, ub8 rdis, ub1 strm, memory_trace *info)
{
  struct    cache_block_t *block;
  ub8       indx;
  ub8       old_tag;
  ub1       mshr_alloc;
  cache_access_status ret;
  memory_trace *info_out;

  assert(cache);

  indx        = ADDR_NDX(cache, addr);
  addr        = BLCKALIGN(addr); 
  ret.way     = -1;
  ret.fate    = CACHE_ACCESS_UNK;
  ret.stream  = NN;
  ret.dirty   = 0;
  
  if (strm == NN || strm > TST)
  {
    cout << "Illegal stream " << (int)strm << endl;
  }

  assert(strm != NN && strm <= TST);

  block = cache_find_block(cache->cache, indx, addr, info);

#if 0
  update_dramsim_row_access_stats(cache, info);
#endif

  /* If access is a miss */
  if (!block)
  {
    assert(info->prefetch == FALSE);

    if (cache->dramsim_enable && info->fill == TRUE)
    {
      /* If mshr is available and DRAM transaction queue has space,
       * send request to DRAM, else NACK the request */
      mshr_alloc = cs_alloc_mshr(cache, info);

      if (mshr_alloc == CS_MSHR_ALLOC && 
          dramsim_will_accept_request(BLCKALIGN(info->address), FALSE, info->stream))
      {
        real_access++;
        real_miss++;

        info_out = (memory_trace *)calloc(1, sizeof(memory_trace));
        assert(info_out);

        info_out->address     = info->address;
        info_out->stream      = info->stream;
        info_out->sap_stream  = info->sap_stream;
        info_out->fill        = TRUE;
        info_out->cycle       = cache->dramcycle;
        info_out->vtl_addr    = info_out->address;
        info_out->prefetch    = FALSE;

        ret.fate = CACHE_ACCESS_MISS;

        ub8 speedup_chn_id;
        ub8 speedup_rnk_id;
        ub8 speedup_bnk_id;
        ub8 original_row_id;
        ub8 speedup_row_id;
        ub8 speedup_col_id;
        ub1 access_remapped;
        map<ub8, ub8>::iterator map_itr;
        row_stats *row_info;
        dram_row  *current_row; 
        access_remapped = FALSE;

        if (cache->dramsim_enable)
        {
          update_dramsim_open_row_stats(cache, shadow_tags, access_remapped, info_out);
        }
        
        /* Allocate DRAM timing stats structure before issuing a DRAM request */
        info_out->policy_data = (void *)calloc(1, sizeof(dram_queue_time));

        dramsim_request(FALSE, BLCKALIGN(info_out->address), info_out->stream, info_out); 

        pending_requests += 1;
      }
      else
      {
        if (mshr_alloc == CS_MSHR_FAIL)
        {
          ret.fate = CACHE_ACCESS_NACK;
          mshr_stall += 1;
        }

        if (!dramsim_will_accept_request(BLCKALIGN(info->address), (info->spill == TRUE), info->stream))
        {
          if (info->spill)
          {
            write_stall += 1;
          }
          else
          {
            read_stall += 1;
          }
        }
      }
    }
    else
    {
      if (cache->dramsim_enable == TRUE)
      {
        assert(info->fill == FALSE);

        if (cache->free_mshr)
        {
          real_access++;
          real_miss++;

          if (cs_is_mshr_allocated(cache, info) == FALSE)
          {
            ret = cachesim_fill_block(cache, info);
          }
          else
          {
            ret.fate = CACHE_ACCESS_MISS;
          }
        }
        else
        {
          ret.fate = CACHE_ACCESS_NACK;
          mshr_stall += 1;
        }
      }
      else
      {
        if (info->fill)
        {
          update_stream_row_stats(cache, NULL, FALSE, info);
        }

        ret = cachesim_fill_block(cache, info);
      }
    }
  }
  else
  {
    ret.fate      = CACHE_ACCESS_HIT;
    ret.tag       = block->tag;
    ret.vtl_addr  = block->vtl_addr;
    ret.way       = block->way;
    ret.stream    = block->stream;
    ret.dirty     = block->dirty;
    ret.epoch     = block->epoch;
    ret.access    = block->access;
    ret.last_rrpv = block->last_rrpv;

    cache_access_block(cache->cache, indx, block->way, strm, info);

    real_access++;
  }
  
  return ret;
}

cache_access_status cachesim_only_dram(cachesim_cache *cache, ub8 addr, 
  ub8 rdis, ub1 strm, memory_trace *info)
{
  struct    cache_block_t *block;
  ub8       indx;
  ub1       mshr_alloc;
  cache_access_status ret;
  memory_trace *info_out;

  assert(cache);

  indx        = ADDR_NDX(cache, addr);
  addr        = BLCKALIGN(addr); 
  ret.way     = -1;
  ret.fate    = CACHE_ACCESS_UNK;
  ret.stream  = NN;
  ret.dirty   = 0;

  if (strm == NN || strm > TST)
  {
    cout << "Illegal stream " << (int)strm << endl;
  }

  assert(strm != NN && strm <= TST);

  assert(info->prefetch == FALSE);

  if (cache->dramsim_enable)
  {
    /* If mshr is available and DRAM transaction queue has space,
     * send request to DRAM, else NACK the request */
    mshr_alloc = cs_alloc_mshr(cache, info);

    if (mshr_alloc == CS_MSHR_ALLOC && 
        dramsim_will_accept_request(BLCKALIGN(info->address), FALSE, info->stream))
    {
      info_out = (memory_trace *)calloc(1, sizeof(memory_trace));
      assert(info_out);

      info_out->address     = info->address;
      info_out->vtl_addr    = info->vtl_addr;
      info_out->stream      = info->stream;
      info_out->sap_stream  = info->sap_stream;
      info_out->fill        = TRUE;

      ret.fate = CACHE_ACCESS_MISS;

      /* Allocate DRAM timing stats structure before issuing a DRAM request */
      info_out->policy_data = (void *)calloc(1, sizeof(dram_queue_time));

      dramsim_request(info->dirty, BLCKALIGN(info_out->address), info_out->stream, info_out); 

      pending_requests += 1;
    }
    else
    {
      if (mshr_alloc == CS_MSHR_FAIL)
      {
        ret.fate = CACHE_ACCESS_NACK;
        mshr_stall += 1;
      }

      if (!dramsim_will_accept_request(BLCKALIGN(info->address), (info->spill == TRUE), info->stream))
      {
        if (info->spill)
        {
          write_stall += 1;
        }
        else
        {
          read_stall += 1;
        }
      }
    }
  }

  return ret;
}

/* Given cache capacity and associativity obtain other cache parameters */
void set_cache_params(struct cache_params *params, LChParameters *lcP, 
  ub8 cache_size, ub8 cache_way, ub8 cache_set)
{
  params->policy                = lcP->policy;
  params->clock_period          = lcP->clockPeriod;
  params->max_rrpv              = lcP->max_rrpv;
  params->spill_rrpv            = lcP->spill_rrpv;
  params->threshold             = lcP->threshold;
  params->ways                  = cache_way;
  params->stream                = lcP->stream;
  params->streams               = lcP->streams;
  params->gsdrrip_cpu_enable    = lcP->cpu_enable;
  params->gsdrrip_gpu_enable    = lcP->gpu_enable;
  params->gsdrrip_gpgpu_enable  = lcP->gpgpu_enable;
  params->bs_epoch              = lcP->useBs;
  params->speedup_enabled       = lcP->speedupEnabled;
  params->remap_crtcl           = lcP->remapCrtcl;
  params->use_step              = lcP->useStep;
  params->sampler_sets          = lcP->samplerSets;
  params->sampler_ways          = lcP->samplerWays;
  params->sarp_pin_blocks       = lcP->pinBlocks;
  params->sarp_cpu_fill_enable  = lcP->cpuFillEnable;
  params->sarp_gpu_fill_enable  = lcP->gpuFillEnable;
  params->dramsim_enable        = lcP->dramSimEnable;
  params->dramsim_trace         = lcP->dramSimTrace;
  params->ship_shct_size        = lcP->shctSize;        /* Ship signature history table size */
  params->ship_sig_size         = lcP->signSize;        /* Ship signature size */
  params->ship_entry_size       = lcP->entrySize;       /* Ship counter width */
  params->ship_core_size        = lcP->coreSize;        /* Ship number of cores */
  params->ship_use_mem          = lcP->useMem;          /* Ship-mem flag */
  params->ship_use_pc           = lcP->usePc;           /* Ship-pc flag */

#define CS_BYTE(cache_size) (cache_size * 1024)
#define SS_BYTE(lcP)        (lcP->cacheBlockSize * cache_way)
  if (!cache_set)
  {
    params->num_sets  = CS_BYTE(cache_size) / SS_BYTE(lcP);
  }
  else
  {
    params->num_sets  = cache_set;
  }

#undef CS_BYTE
#undef SS_BYTE
}

void openStatisticsStream(SimParameters *simP, gzofstream &out_stream)
{
  sb1 *file_name;
  
  file_name = (sb1 *)malloc(strlen(simP->lcP.statFile) + 1);
  assert(file_name);

  sprintf(file_name, "%s", simP->lcP.statFile);

  out_stream.open(file_name, ios::out | ios::binary);

  if (!out_stream.is_open())
  {
    printf("Error opening output stream\n");
    exit(EXIT_FAILURE);
  }
#define CH (4)

  StatisticsManager::instance().setOutputStream(CH, out_stream);

#undef CH
}

static void initStatistics(StatisticsManager *sm)
{
  all_access  = &sm->getNumericStatistic<ub8>("CH_TotalAccess", ub8(0), "UC", NULL);  /* Input stream access */

  i_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "I");  /* Input stream access */
  i_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "I");    /* Input stream access */
  i_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "I"); /* Input stream access */
  i_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "I");   /* Input stream access */
  i_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "I");  /* Input stream access */
  i_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "I");  /* Input stream access */
  i_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "I");  /* Input stream access */
  i_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "I");  /* Input stream access */
  i_soevct    = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "I"); /* Input stream access */
  i_xoevct    = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "I"); /* Input stream access */
  i_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "I"); /* Input stream access */
  i_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "I"); /* Input stream access */
  i_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "I"); /* Input stream access */
  i_xpevct    = &sm->getNumericStatistic<ub8>("CH_XPevict", ub8(0), "UC", "I"); /* Input stream access */
  i_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "I"); /* Input stream access */
  i_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "I");    /* Input stream access */

  c_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "C");  /* Input stream access */
  c_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "C");    /* Input stream access */
  c_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "C"); /* Input stream access */
  c_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "C");   /* Input stream access */
  c_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "C");  /* Input stream access */
  c_cprod     = &sm->getNumericStatistic<ub8>("CH_CProd", ub8(0), "UC", "C");   /* Input stream access */
  c_ccons     = &sm->getNumericStatistic<ub8>("CH_CCons", ub8(0), "UC", "C");   /* Input stream access */
  c_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "C");  /* Input stream access */
  c_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "C");  /* Input stream access */
  c_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "C");  /* Input stream access */
  c_soevct    = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "C"); /* Input stream access */
  c_xoevct    = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "C"); /* Input stream access */
  c_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "C"); /* Input stream access */
  c_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "C"); /* Input stream access */
  c_xpevct    = &sm->getNumericStatistic<ub8>("CH_XPevict", ub8(0), "UC", "C"); /* Input stream access */
  c_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "C"); /* Input stream access */
  c_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "C");    /* Input stream access */
  c_zchit     = &sm->getNumericStatistic<ub8>("CH_ZChit", ub8(0), "UC", "C");   /* Input stream access */
  c_tchit     = &sm->getNumericStatistic<ub8>("CH_TChit", ub8(0), "UC", "C");   /* Input stream access */
  c_bchit     = &sm->getNumericStatistic<ub8>("CH_BChit", ub8(0), "UC", "C");   /* Input stream access */
  c_dchit     = &sm->getNumericStatistic<ub8>("CH_DChit", ub8(0), "UC", "C");   /* Input stream access */
  c_e0fill    = &sm->getNumericStatistic<ub8>("CH_E0fill", ub8(0), "UC", "C");  /* Input stream access */
  c_e0hit     = &sm->getNumericStatistic<ub8>("CH_E0hit", ub8(0), "UC", "C");   /* Input stream access */
  c_e1fill    = &sm->getNumericStatistic<ub8>("CH_E1fill", ub8(0), "UC", "C");  /* Input stream access */
  c_e1hit     = &sm->getNumericStatistic<ub8>("CH_E1hit", ub8(0), "UC", "C");   /* Input stream access */
  c_e2fill    = &sm->getNumericStatistic<ub8>("CH_E2fill", ub8(0), "UC", "C");  /* Input stream access */
  c_e2hit     = &sm->getNumericStatistic<ub8>("CH_E2hit", ub8(0), "UC", "C");   /* Input stream access */
  c_e3fill    = &sm->getNumericStatistic<ub8>("CH_E3fill", ub8(0), "UC", "C");  /* Input stream access */
  c_e3hit     = &sm->getNumericStatistic<ub8>("CH_E3hit", ub8(0), "UC", "C");   /* Input stream access */
  c_e0evt     = &sm->getNumericStatistic<ub8>("CH_E0evt", ub8(0), "UC", "C");   /* Input stream access */
  c_e1evt     = &sm->getNumericStatistic<ub8>("CH_E1evt", ub8(0), "UC", "C");   /* Input stream access */
  c_e2evt     = &sm->getNumericStatistic<ub8>("CH_E2evt", ub8(0), "UC", "C");   /* Input stream access */
  c_e3evt     = &sm->getNumericStatistic<ub8>("CH_E3evt", ub8(0), "UC", "C");   /* Input stream access */
  c_re0fill   = &sm->getNumericStatistic<ub8>("CH_ER0fill", ub8(0), "UC", "C"); /* Input stream access */
  c_re0hit    = &sm->getNumericStatistic<ub8>("CH_RE0hit", ub8(0), "UC", "C");  /* Input stream access */
  c_re1fill   = &sm->getNumericStatistic<ub8>("CH_RE1fill", ub8(0), "UC", "C"); /* Input stream access */
  c_re1hit    = &sm->getNumericStatistic<ub8>("CH_RE1hit", ub8(0), "UC", "C");  /* Input stream access */
  c_re2fill   = &sm->getNumericStatistic<ub8>("CH_RE2fill", ub8(0), "UC", "C"); /* Input stream access */
  c_re2hit    = &sm->getNumericStatistic<ub8>("CH_RE2hit", ub8(0), "UC", "C");  /* Input stream access */
  c_re3fill   = &sm->getNumericStatistic<ub8>("CH_RE3fill", ub8(0), "UC", "C"); /* Input stream access */
  c_re3hit    = &sm->getNumericStatistic<ub8>("CH_RE3hit", ub8(0), "UC", "C");  /* Input stream access */
  c_re0evt    = &sm->getNumericStatistic<ub8>("CH_RE0evt", ub8(0), "UC", "C");  /* Input stream access */
  c_re1evt    = &sm->getNumericStatistic<ub8>("CH_RE1evt", ub8(0), "UC", "C");  /* Input stream access */
  c_re2evt    = &sm->getNumericStatistic<ub8>("CH_RE2evt", ub8(0), "UC", "C");  /* Input stream access */
  c_re3evt    = &sm->getNumericStatistic<ub8>("CH_RE3evt", ub8(0), "UC", "C");  /* Input stream access */

  z_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "Z");  /* Input stream access */
  z_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "Z");    /* Input stream access */
  z_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "Z"); /* Input stream access */
  z_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "Z");   /* Input stream access */
  z_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "Z");  /* Input stream access */
  z_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "Z");  /* Input stream access */
  z_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "Z");  /* Input stream access */
  z_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "Z");  /* Input stream access */
  z_soevct    = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "Z"); /* Input stream access */
  z_xoevct    = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "Z"); /* Input stream access */
  z_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "Z"); /* Input stream access */
  z_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "Z"); /* Input stream access */
  z_xpevct    = &sm->getNumericStatistic<ub8>("CH_XPevict", ub8(0), "UC", "Z"); /* Input stream access */
  z_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "Z"); /* Input stream access */
  z_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "Z");    /* Input stream access */
  z_czhit     = &sm->getNumericStatistic<ub8>("CH_CZhit", ub8(0), "UC", "Z");   /* Input stream access */
  z_tzhit     = &sm->getNumericStatistic<ub8>("CH_TZhit", ub8(0), "UC", "Z");   /* Input stream access */
  z_bzhit     = &sm->getNumericStatistic<ub8>("CH_BZhit", ub8(0), "UC", "Z");   /* Input stream access */
  z_e0fill    = &sm->getNumericStatistic<ub8>("CH_E0fill", ub8(0), "UC", "Z");  /* Input stream access */
  z_e0hit     = &sm->getNumericStatistic<ub8>("CH_E0hit", ub8(0), "UC", "Z");   /* Input stream access */
  z_e1fill    = &sm->getNumericStatistic<ub8>("CH_E1fill", ub8(0), "UC", "Z");  /* Input stream access */
  z_e1hit     = &sm->getNumericStatistic<ub8>("CH_E1hit", ub8(0), "UC", "Z");   /* Input stream access */
  z_e2fill    = &sm->getNumericStatistic<ub8>("CH_E2fill", ub8(0), "UC", "Z");  /* Input stream access */
  z_e2hit     = &sm->getNumericStatistic<ub8>("CH_E2hit", ub8(0), "UC", "Z");   /* Input stream access */
  z_e3fill    = &sm->getNumericStatistic<ub8>("CH_E3fill", ub8(0), "UC", "Z");  /* Input stream access */
  z_e3hit     = &sm->getNumericStatistic<ub8>("CH_E3hit", ub8(0), "UC", "Z");   /* Input stream access */
  z_e0evt     = &sm->getNumericStatistic<ub8>("CH_E0evt", ub8(0), "UC", "Z");   /* Input stream access */
  z_e1evt     = &sm->getNumericStatistic<ub8>("CH_E1evt", ub8(0), "UC", "Z");   /* Input stream access */
  z_e2evt     = &sm->getNumericStatistic<ub8>("CH_E2evt", ub8(0), "UC", "Z");   /* Input stream access */
  z_e3evt     = &sm->getNumericStatistic<ub8>("CH_E3evt", ub8(0), "UC", "Z");   /* Input stream access */
  z_re0fill   = &sm->getNumericStatistic<ub8>("CH_RE0fill", ub8(0), "UC", "Z"); /* Input stream access */
  z_re0hit    = &sm->getNumericStatistic<ub8>("CH_RE0hit", ub8(0), "UC", "Z");  /* Input stream access */
  z_re1fill   = &sm->getNumericStatistic<ub8>("CH_RE1fill", ub8(0), "UC", "Z"); /* Input stream access */
  z_re1hit    = &sm->getNumericStatistic<ub8>("CH_RE1hit", ub8(0), "UC", "Z");  /* Input stream access */
  z_re2fill   = &sm->getNumericStatistic<ub8>("CH_RE2fill", ub8(0), "UC", "Z"); /* Input stream access */
  z_re2hit    = &sm->getNumericStatistic<ub8>("CH_RE2hit", ub8(0), "UC", "Z");  /* Input stream access */
  z_re3fill   = &sm->getNumericStatistic<ub8>("CH_RE3fill", ub8(0), "UC", "Z"); /* Input stream access */
  z_re3hit    = &sm->getNumericStatistic<ub8>("CH_RE3hit", ub8(0), "UC", "Z");  /* Input stream access */
  z_re0evt    = &sm->getNumericStatistic<ub8>("CH_RE0evt", ub8(0), "UC", "Z");  /* Input stream access */
  z_re1evt    = &sm->getNumericStatistic<ub8>("CH_RE1evt", ub8(0), "UC", "Z");  /* Input stream access */
  z_re2evt    = &sm->getNumericStatistic<ub8>("CH_RE2evt", ub8(0), "UC", "Z");  /* Input stream access */
  z_re3evt    = &sm->getNumericStatistic<ub8>("CH_RE3evt", ub8(0), "UC", "Z");  /* Input stream access */

  t_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "T");  /* Input stream access */
  t_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "T");    /* Input stream access */
  t_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "T"); /* Input stream access */
  t_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "T");   /* Input stream access */
  t_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "T");  /* Input stream access */
  t_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "T");  /* Input stream access */
  t_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "T");  /* Input stream access */
  t_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "T");  /* Input stream access */
  t_soevct    = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "T"); /* Input stream access */
  t_xoevct    = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "T"); /* Input stream access */
  t_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "T"); /* Input stream access */
  t_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "T"); /* Input stream access */
  t_xpevct    = &sm->getNumericStatistic<ub8>("CH_XPevict", ub8(0), "UC", "T"); /* Input stream access */
  t_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "T"); /* Input stream access */
  t_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "T");    /* Input stream access */
  t_cthit     = &sm->getNumericStatistic<ub8>("CH_CThit", ub8(0), "UC", "T");   /* Input stream access */
  t_zthit     = &sm->getNumericStatistic<ub8>("CH_ZThit", ub8(0), "UC", "T");   /* Input stream access */
  t_bthit     = &sm->getNumericStatistic<ub8>("CH_BThit", ub8(0), "UC", "T");   /* Input stream access */
  t_e0fill    = &sm->getNumericStatistic<ub8>("CH_E0fill", ub8(0), "UC", "T");  /* Input stream access */
  t_e0hit     = &sm->getNumericStatistic<ub8>("CH_E0hit", ub8(0), "UC", "T");   /* Input stream access */
  t_e1fill    = &sm->getNumericStatistic<ub8>("CH_E1fill", ub8(0), "UC", "T");  /* Input stream access */
  t_e1hit     = &sm->getNumericStatistic<ub8>("CH_E1hit", ub8(0), "UC", "T");   /* Input stream access */
  t_e2fill    = &sm->getNumericStatistic<ub8>("CH_E2fill", ub8(0), "UC", "T");  /* Input stream access */
  t_e2hit     = &sm->getNumericStatistic<ub8>("CH_E2hit", ub8(0), "UC", "T");   /* Input stream access */
  t_e3fill    = &sm->getNumericStatistic<ub8>("CH_E3fill", ub8(0), "UC", "T");   /* Input stream access */
  t_e3hit     = &sm->getNumericStatistic<ub8>("CH_E3hit", ub8(0), "UC", "T");   /* Input stream access */
  t_e0evt     = &sm->getNumericStatistic<ub8>("CH_E0evt", ub8(0), "UC", "T");   /* Input stream access */
  t_e1evt     = &sm->getNumericStatistic<ub8>("CH_E1evt", ub8(0), "UC", "T");   /* Input stream access */
  t_e2evt     = &sm->getNumericStatistic<ub8>("CH_E2evt", ub8(0), "UC", "T");   /* Input stream access */
  t_e3evt     = &sm->getNumericStatistic<ub8>("CH_E3evt", ub8(0), "UC", "T");   /* Input stream access */
  t_re0fill   = &sm->getNumericStatistic<ub8>("CH_RE0fill", ub8(0), "UC", "T"); /* Input stream access */
  t_re0hit    = &sm->getNumericStatistic<ub8>("CH_RE0hit", ub8(0), "UC", "T");  /* Input stream access */
  t_re1fill   = &sm->getNumericStatistic<ub8>("CH_RE1fill", ub8(0), "UC", "T"); /* Input stream access */
  t_re1hit    = &sm->getNumericStatistic<ub8>("CH_RE1hit", ub8(0), "UC", "T");  /* Input stream access */
  t_re2fill   = &sm->getNumericStatistic<ub8>("CH_RE2fill", ub8(0), "UC", "T"); /* Input stream access */
  t_re2hit    = &sm->getNumericStatistic<ub8>("CH_RE2hit", ub8(0), "UC", "T");  /* Input stream access */
  t_re3fill   = &sm->getNumericStatistic<ub8>("CH_RE3fill", ub8(0), "UC", "T"); /* Input stream access */
  t_re3hit    = &sm->getNumericStatistic<ub8>("CH_RE3hit", ub8(0), "UC", "T");  /* Input stream access */
  t_re0evt    = &sm->getNumericStatistic<ub8>("CH_RE0evt", ub8(0), "UC", "T");  /* Input stream access */
  t_re1evt    = &sm->getNumericStatistic<ub8>("CH_RE1evt", ub8(0), "UC", "T");  /* Input stream access */
  t_re2evt    = &sm->getNumericStatistic<ub8>("CH_RE2evt", ub8(0), "UC", "T");  /* Input stream access */
  t_re3evt    = &sm->getNumericStatistic<ub8>("CH_RE3evt", ub8(0), "UC", "T");  /* Input stream access */

  n_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "N");  /* Input stream access */
  n_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "N");    /* Input stream access */
  n_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "N"); /* Input stream access */
  n_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "N");   /* Input stream access */
  n_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "N");  /* Input stream access */
  n_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "N");  /* Input stream access */
  n_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "N");  /* Input stream access */
  n_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "N");  /* Input stream access */
  n_soevct    = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "N"); /* Input stream access */
  n_xoevct    = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "N"); /* Input stream access */
  n_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "N"); /* Input stream access */
  n_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "N"); /* Input stream access */
  n_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "N"); /* Input stream access */
  n_xpevct    = &sm->getNumericStatistic<ub8>("CH_XPevict", ub8(0), "UC", "N"); /* Input stream access */
  n_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "N"); /* Input stream access */
  n_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "N");    /* Input stream access */

  h_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "H");  /* Input stream access */
  h_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "H");    /* Input stream access */
  h_raccess    = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "H");/* Input stream access */
  h_rmiss      = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "H");  /* Input stream access */
  h_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "H");  /* Input stream access */
  h_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "H");  /* Input stream access */
  h_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "H");  /* Input stream access */
  h_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "H");  /* Input stream access */
  h_soevct    = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "H"); /* Input stream access */
  h_xoevct    = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "H"); /* Input stream access */
  h_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "H"); /* Input stream access */
  h_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "H"); /* Input stream access */
  h_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "H"); /* Input stream access */
  h_xpevct    = &sm->getNumericStatistic<ub8>("CH_XPevict", ub8(0), "UC", "H"); /* Input stream access */
  h_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "H"); /* Input stream access */
  h_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "H");    /* Input stream access */

  d_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "D");  /* Input stream access */
  d_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "D");    /* Input stream access */
  d_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "D"); /* Input stream access */
  d_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "D");   /* Input stream access */
  d_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "D");  /* Input stream access */
  d_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "D");  /* Input stream access */
  d_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "D");  /* Input stream access */
  d_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "D");  /* Input stream access */
  d_soevct    = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "D"); /* Input stream access */
  d_xoevct    = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "D"); /* Input stream access */
  d_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "D"); /* Input stream access */
  d_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "D"); /* Input stream access */
  d_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "D"); /* Input stream access */
  d_xpevct    = &sm->getNumericStatistic<ub8>("CH_XPevict", ub8(0), "UC", "D"); /* Input stream access */
  d_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "D"); /* Input stream access */
  d_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "D");    /* Input stream access */
  d_cdhit      = &sm->getNumericStatistic<ub8>("CH_CDhit", ub8(0), "UC", "D");  /* Input stream access */
  d_zdhit      = &sm->getNumericStatistic<ub8>("CH_ZDhit", ub8(0), "UC", "D");  /* Input stream access */
  d_tdhit      = &sm->getNumericStatistic<ub8>("CH_TDhit", ub8(0), "UC", "D");  /* Input stream access */
  d_bdhit      = &sm->getNumericStatistic<ub8>("CH_BDhit", ub8(0), "UC", "D");  /* Input stream access */

  b_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "B");  /* Input stream access */
  b_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "B");    /* Input stream access */
  b_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "B"); /* Input stream access */
  b_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "B");   /* Input stream access */
  b_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "B");  /* Input stream access */
  b_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "B");  /* Input stream access */
  b_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "B");  /* Input stream access */
  b_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "B");  /* Input stream access */
  b_soevct    = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "B"); /* Input stream access */
  b_xoevct    = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "B"); /* Input stream access */
  b_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "B"); /* Input stream access */
  b_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "B"); /* Input stream access */
  b_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "B"); /* Input stream access */
  b_xpevct    = &sm->getNumericStatistic<ub8>("CH_XPevict", ub8(0), "UC", "B"); /* Input stream access */
  b_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "B"); /* Input stream access */
  b_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "B");    /* Input stream access */
  b_bprod     = &sm->getNumericStatistic<ub8>("CH_BProd", ub8(0), "UC", "B");   /* Input stream access */
  b_bcons     = &sm->getNumericStatistic<ub8>("CH_BCons", ub8(0), "UC", "B");   /* Input stream access */
  b_tbhit     = &sm->getNumericStatistic<ub8>("CH_TBhit", ub8(0), "UC", "B");   /* Input stream access */
  b_zbhit     = &sm->getNumericStatistic<ub8>("CH_ZBhit", ub8(0), "UC", "B");   /* Input stream access */
  b_cbhit     = &sm->getNumericStatistic<ub8>("CH_CBhit", ub8(0), "UC", "B");   /* Input stream access */

  x_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "X");  /* Input stream access */
  x_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "X");    /* Input stream access */
  x_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "X"); /* Input stream access */
  x_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "X");   /* Input stream access */
  x_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "X");  /* Input stream access */
  x_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "X");  /* Input stream access */
  x_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "X");  /* Input stream access */
  x_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "X");  /* Input stream access */
  x_soevct    = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "X");  /* Input stream access */
  x_xoevct    = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "X");  /* Input stream access */
  x_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "X"); /* Input stream access */
  x_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "X"); /* Input stream access */
  x_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "X"); /* Input stream access */
  x_xpevct    = &sm->getNumericStatistic<ub8>("CH_XPevict", ub8(0), "UC", "X"); /* Input stream access */
  x_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "X"); /* Input stream access */
  x_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "X");    /* Input stream access */

  p_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "P");  /* Input stream access */
  p_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "P");    /* Input stream access */
  p_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "P"); /* Input stream access */
  p_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "P");   /* Input stream access */
  p_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "P");  /* Input stream access */
  p_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "P");  /* Input stream access */
  p_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "P");  /* Input stream access */
  p_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "P");  /* Input stream access */
  p_soevct     = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "P"); /* Input stream access */
  p_xoevct     = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "P"); /* Input stream access */
  p_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "P");  /* Input stream access */
  p_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "P");  /* Input stream access */
  p_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "P");  /* Input stream access */
  p_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "P");  /* Input stream access */
  p_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "P");     /* Input stream access */

  g_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "G");  /* Input stream access */
  g_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "G");    /* Input stream access */
  g_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "G"); /* Input stream access */
  g_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "G");   /* Input stream access */
  g_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "G");  /* Input stream access */
  g_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "G");  /* Input stream access */
  g_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "G");  /* Input stream access */
  g_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "G");  /* Input stream access */
  g_soevct    = &sm->getNumericStatistic<ub8>("CH_SOevict", ub8(0), "UC", "G"); /* Input stream access */
  g_xoevct    = &sm->getNumericStatistic<ub8>("CH_XOevict", ub8(0), "UC", "G"); /* Input stream access */
  g_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "G"); /* Input stream access */
  g_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "G"); /* Input stream access */
  g_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "G"); /* Input stream access */
  g_xrevct    = &sm->getNumericStatistic<ub8>("CH_XRevict", ub8(0), "UC", "G"); /* Input stream access */
  g_xpevct    = &sm->getNumericStatistic<ub8>("CH_XPevict", ub8(0), "UC", "G"); /* Input stream access */
  g_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "G");    /* Input stream access */
  g_xblocks   = &sm->getNumericStatistic<ub8>("CH_XBlocks", ub8(0), "UC", "G"); /* Input stream access */
}

void closeStatisticsStream(gzofstream &out_stream)
{
  out_stream.close();
}

void dumpOverallStats(cachesim_cache *cache, ub8 cycle, ub8 cachecycle, ub8 dramcycle)
{
  FILE *fout;
  ub1   i;

  fout = fopen("OverallCacheStats.log", "w+");
  assert(fout);

  fprintf(fout, "[LLC]\n");
  
  fprintf(fout, "Cycles = %ld\n", cycle);
  fprintf(fout, "CacheCycles = %ld\n", cachecycle);
  fprintf(fout, "DramCycles = %ld\n", dramcycle);
  fprintf(fout, "CSDramRquestCycles = %ld\n", dram_request_cycle[CS]);
  fprintf(fout, "CSDramTxQueueCycles = %ld\n", dram_tq_cycle[CS]);
  fprintf(fout, "CSDramCmdQueueCycles = %ld\n", dram_cq_cycle[CS]);
  fprintf(fout, "ZSDramRquestCycles = %ld\n", dram_request_cycle[ZS]);
  fprintf(fout, "ZSDramTxQueueCycles = %ld\n", dram_tq_cycle[ZS]);
  fprintf(fout, "ZSDramCmdQueueCycles = %ld\n", dram_cq_cycle[ZS]);
  fprintf(fout, "TSDramRquestCycles = %ld\n", dram_request_cycle[TS]);
  fprintf(fout, "TSDramTxQueueCycles = %ld\n", dram_tq_cycle[TS]);
  fprintf(fout, "TSDramCmdQueueCycles = %ld\n", dram_cq_cycle[TS]);
  fprintf(fout, "BSDramRquestCycles = %ld\n", dram_request_cycle[BS]);
  fprintf(fout, "BSDramTxQueueCycles = %ld\n", dram_tq_cycle[BS]);
  fprintf(fout, "BSDramCmdQueueCycles = %ld\n", dram_cq_cycle[BS]);
  fprintf(fout, "PSDramRquestCycles = %ld\n", dram_request_cycle[PS]);
  fprintf(fout, "PSDramTxQueueCycles = %ld\n", dram_tq_cycle[PS]);
  fprintf(fout, "PSDramCmdQueueCycles = %ld\n", dram_cq_cycle[PS]);
  fprintf(fout, "OSDramRquestCycles = %ld\n", 
      dram_request_cycle[IS] + dram_request_cycle[NS] + dram_request_cycle[XS] + 
      dram_request_cycle[DS] + dram_request_cycle[HS]);

  fprintf(fout, "ReadStall = %ld\n", read_stall);
  fprintf(fout, "WriteStall = %ld\n", write_stall);
  fprintf(fout, "MSHRStall = %ld\n", mshr_stall);

  for (i = NN + 1; i < TST; i++)
  {
    if (cache->access[i])
    {
      fprintf(fout, "%s-Read = %ld\n", stream_names[i], cache->access[i]);
      fprintf(fout, "%s-ReadMiss = %ld\n", stream_names[i], cache->miss[i]);
    }
  }

  fclose(fout);
}

/* Counter update for reads */
#define updateEpochFillCountersForRead(stream_in, epoch_in)  \
{                                                            \
  switch ((stream_in))                                       \
  {                                                          \
    case ZS:                                                 \
                                                             \
      switch ((epoch_in))                                    \
      {                                                      \
        case 0:                                              \
          (*z_re0fill)++;                                    \
          break;                                             \
                                                             \
        case 1:                                              \
          (*z_re1fill)++;                                    \
          break;                                             \
                                                             \
        case 2:                                              \
                                                             \
          (*z_re2fill)++;                                    \
          break;                                             \
                                                             \
        case 3:                                              \
          (*z_re3fill)++;                                    \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    case CS:                                                 \
                                                             \
      switch ((epoch_in))                                    \
      {                                                      \
        case 0:                                              \
          (*c_re0fill)++;                                    \
          break;                                             \
                                                             \
        case 1:                                              \
          (*c_re1fill)++;                                    \
          break;                                             \
                                                             \
        case 2:                                              \
          (*c_re2fill)++;                                    \
          break;                                             \
                                                             \
        case 3:                                              \
          (*c_re3fill)++;                                    \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    case TS:                                                 \
                                                             \
      switch ((epoch_in))                                    \
      {                                                      \
        case 0:                                              \
          (*t_re0fill)++;                                    \
          break;                                             \
                                                             \
        case 1:                                              \
          (*t_re1fill)++;                                    \
          break;                                             \
                                                             \
        case 2:                                              \
          (*t_re2fill)++;                                    \
          break;                                             \
                                                             \
        case 3:                                              \
          (*t_re3fill)++;                                    \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    default:                                                 \
                                                             \
      printf("Unhandeled stream %d for epoch hit counters \n", (int)stream_in); \
      exit(EXIT_FAILURE);                                    \
  }                                                          \
}

#define updateEpochHitCountersForRead(ret_in)                \
{                                                            \
  switch ((ret_in).stream)                                   \
  {                                                          \
    case ZS:                                                 \
                                                             \
      switch ((ret_in).epoch)                                \
      {                                                      \
        case 0:                                              \
          (*z_re0hit)++;                                     \
          break;                                             \
                                                             \
        case 1:                                              \
          (*z_re1hit)++;                                     \
          break;                                             \
                                                             \
        case 2:                                              \
                                                             \
          (*z_re2hit)++;                                     \
          break;                                             \
                                                             \
        case 3:                                              \
          (*z_re3hit)++;                                     \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    case CS:                                                 \
                                                             \
      switch ((ret_in).epoch)                                \
      {                                                      \
        case 0:                                              \
          (*c_re0hit)++;                                     \
          break;                                             \
                                                             \
        case 1:                                              \
          (*c_re1hit)++;                                     \
          break;                                             \
                                                             \
        case 2:                                              \
          (*c_re2hit)++;                                     \
          break;                                             \
                                                             \
        case 3:                                              \
          (*c_re3hit)++;                                     \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    case TS:                                                 \
                                                             \
      switch ((ret_in).epoch)                                \
      {                                                      \
        case 0:                                              \
          (*t_re0hit)++;                                     \
          break;                                             \
                                                             \
        case 1:                                              \
          (*t_re1hit)++;                                     \
          break;                                             \
                                                             \
        case 2:                                              \
          (*t_re2hit)++;                                     \
          break;                                             \
                                                             \
        case 3:                                              \
          (*t_re3hit)++;                                     \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    default:                                                 \
                                                             \
      printf("Unhandeled stream %d for epoch hit counters \n", (int)ret.stream); \
      exit(EXIT_FAILURE);                                    \
  }                                                          \
}

#define  updateEopchEvictCountersForRead(ret_in)              \
{                                                             \
  switch ((ret_in).stream)                                    \
  {                                                           \
    case ZS:                                                  \
                                                              \
      switch ((ret_in).epoch)                                 \
      {                                                       \
        case 0:                                               \
          (*z_re0evt)++;                                      \
          break;                                              \
                                                              \
        case 1:                                               \
          (*z_re1evt)++;                                      \
          break;                                              \
                                                              \
        case 2:                                               \
          (*z_re2evt)++;                                      \
          break;                                              \
                                                              \
        case 3:                                               \
          (*z_re3evt)++;                                      \
          break;                                              \
      }                                                       \
                                                              \
      break;                                                  \
                                                              \
    case CS:                                                  \
                                                              \
      switch ((ret_in).epoch)                                 \
      {                                                       \
        case 0:                                               \
          (*c_re0evt)++;                                      \
          break;                                              \
                                                              \
        case 1:                                               \
          (*c_re1evt)++;                                      \
          break;                                              \
                                                              \
        case 2:                                               \
          (*c_re2evt)++;                                      \
          break;                                              \
                                                              \
        case 3:                                               \
          (*c_re3evt)++;                                      \
          break;                                              \
      }                                                       \
                                                              \
      break;                                                  \
                                                              \
    case TS:                                                  \
                                                              \
      switch ((ret_in).epoch)                                 \
      {                                                       \
        case 0:                                               \
          (*t_re0evt)++;                                      \
          break;                                              \
                                                              \
        case 1:                                               \
          (*t_re1evt)++;                                      \
          break;                                              \
                                                              \
        case 2:                                               \
          (*t_re2evt)++;                                      \
          break;                                              \
                                                              \
        case 3:                                               \
          (*t_re3evt)++;                                      \
          break;                                              \
      }                                                       \
                                                              \
      break;                                                  \
                                                              \
    default:                                                  \
                                                              \
      printf("Unhandeled stream for epoch hit counters \n");  \
      exit(EXIT_FAILURE);                                     \
  }                                                           \
}

/* Counter update for all accesses */
#define updateEpochFillCounters(stream_in, epoch_in)         \
{                                                            \
  switch ((stream_in))                                       \
  {                                                          \
    case ZS:                                                 \
                                                             \
      switch ((epoch_in))                                    \
      {                                                      \
        case 0:                                              \
          (*z_e0fill)++;                                     \
          break;                                             \
                                                             \
        case 1:                                              \
          (*z_e1fill)++;                                     \
          break;                                             \
                                                             \
        case 2:                                              \
                                                             \
          (*z_e2fill)++;                                     \
          break;                                             \
                                                             \
        case 3:                                              \
          (*z_e3fill)++;                                     \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    case CS:                                                 \
                                                             \
      switch ((epoch_in))                                    \
      {                                                      \
        case 0:                                              \
          (*c_e0fill)++;                                     \
          break;                                             \
                                                             \
        case 1:                                              \
          (*c_e1fill)++;                                     \
          break;                                             \
                                                             \
        case 2:                                              \
          (*c_e2fill)++;                                     \
          break;                                             \
                                                             \
        case 3:                                              \
          (*c_e3fill)++;                                     \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    case TS:                                                 \
                                                             \
      switch ((epoch_in))                                    \
      {                                                      \
        case 0:                                              \
          (*t_e0fill)++;                                     \
          break;                                             \
                                                             \
        case 1:                                              \
          (*t_e1fill)++;                                     \
          break;                                             \
                                                             \
        case 2:                                              \
          (*t_e2fill)++;                                     \
          break;                                             \
                                                             \
        case 3:                                              \
          (*t_e3fill)++;                                     \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    default:                                                 \
                                                             \
      printf("Unhandeled stream %d for epoch hit counters \n", (int)stream_in); \
      exit(EXIT_FAILURE);                                    \
  }                                                          \
}

#define updateEpochHitCounters(ret_in)                       \
{                                                            \
  switch ((ret_in).stream)                                   \
  {                                                          \
    case ZS:                                                 \
                                                             \
      switch ((ret_in).epoch)                                \
      {                                                      \
        case 0:                                              \
          (*z_e0hit)++;                                      \
          break;                                             \
                                                             \
        case 1:                                              \
          (*z_e1hit)++;                                      \
          break;                                             \
                                                             \
        case 2:                                              \
                                                             \
          (*z_e2hit)++;                                      \
          break;                                             \
                                                             \
        case 3:                                              \
          (*z_e3hit)++;                                      \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    case CS:                                                 \
                                                             \
      switch ((ret_in).epoch)                                \
      {                                                      \
        case 0:                                              \
          (*c_e0hit)++;                                      \
          break;                                             \
                                                             \
        case 1:                                              \
          (*c_e1hit)++;                                      \
          break;                                             \
                                                             \
        case 2:                                              \
          (*c_e2hit)++;                                      \
          break;                                             \
                                                             \
        case 3:                                              \
          (*c_e3hit)++;                                      \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    case TS:                                                 \
                                                             \
      switch ((ret_in).epoch)                                \
      {                                                      \
        case 0:                                              \
          (*t_e0hit)++;                                      \
          break;                                             \
                                                             \
        case 1:                                              \
          (*t_e1hit)++;                                      \
          break;                                             \
                                                             \
        case 2:                                              \
          (*t_e2hit)++;                                      \
          break;                                             \
                                                             \
        case 3:                                              \
          (*t_e3hit)++;                                      \
          break;                                             \
      }                                                      \
                                                             \
      break;                                                 \
                                                             \
    default:                                                 \
                                                             \
      printf("Unhandeled stream %d for epoch hit counters \n", (int)ret.stream); \
      exit(EXIT_FAILURE);                                    \
  }                                                          \
}

#define  updateEopchEvictCounters(ret_in)                     \
{                                                             \
  switch ((ret_in).stream)                                    \
  {                                                           \
    case ZS:                                                  \
                                                              \
      switch ((ret_in).epoch)                                 \
      {                                                       \
        case 0:                                               \
          (*z_e0evt)++;                                       \
          break;                                              \
                                                              \
        case 1:                                               \
          (*z_e1evt)++;                                       \
          break;                                              \
                                                              \
        case 2:                                               \
          (*z_e2evt)++;                                       \
          break;                                              \
                                                              \
        case 3:                                               \
          (*z_e3evt)++;                                       \
          break;                                              \
      }                                                       \
                                                              \
      break;                                                  \
                                                              \
    case CS:                                                  \
                                                              \
      switch ((ret_in).epoch)                                 \
      {                                                       \
        case 0:                                               \
          (*c_e0evt)++;                                       \
          break;                                              \
                                                              \
        case 1:                                               \
          (*c_e1evt)++;                                       \
          break;                                              \
                                                              \
        case 2:                                               \
          (*c_e2evt)++;                                       \
          break;                                              \
                                                              \
        case 3:                                               \
          (*c_e3evt)++;                                       \
          break;                                              \
      }                                                       \
                                                              \
      break;                                                  \
                                                              \
    case TS:                                                  \
                                                              \
      switch ((ret_in).epoch)                                 \
      {                                                       \
        case 0:                                               \
          (*t_e0evt)++;                                       \
          break;                                              \
                                                              \
        case 1:                                               \
          (*t_e1evt)++;                                       \
          break;                                              \
                                                              \
        case 2:                                               \
          (*t_e2evt)++;                                       \
          break;                                              \
                                                              \
        case 3:                                               \
          (*t_e3evt)++;                                       \
          break;                                              \
      }                                                       \
                                                              \
      break;                                                  \
                                                              \
    default:                                                  \
                                                              \
      printf("Unhandeled stream for epoch hit counters \n");  \
      exit(EXIT_FAILURE);                                     \
  }                                                           \
}

static void resetStatistics()
{
  /* Reset statistics for current iteration */
  all_access->setValue(0);

  i_access->setValue(0);
  i_miss->setValue(0);
  i_raccess->setValue(0);
  i_rmiss->setValue(0);
  i_blocks->setValue(0);
  i_xevct->setValue(0);
  i_sevct->setValue(0);
  i_zevct->setValue(0);
  i_soevct->setValue(0);
  i_xoevct->setValue(0);
  i_xcevct->setValue(0);
  i_xzevct->setValue(0);
  i_xtevct->setValue(0);
  i_xpevct->setValue(0);
  i_xrevct->setValue(0);
  i_xhit->setValue(0);

  c_access->setValue(0);
  c_miss->setValue(0);
  c_raccess->setValue(0);
  c_rmiss->setValue(0);
  c_blocks->setValue(0);
  c_cprod->setValue(0);
  c_ccons->setValue(0);
  c_xevct->setValue(0);
  c_sevct->setValue(0);
  c_zevct->setValue(0);
  c_soevct->setValue(0);
  c_xoevct->setValue(0);
  c_xzevct->setValue(0);
  c_xtevct->setValue(0);
  c_xpevct->setValue(0);
  c_xrevct->setValue(0);
  c_xhit->setValue(0);
  c_zchit->setValue(0);
  c_tchit->setValue(0);
  c_bchit->setValue(0);
  c_dchit->setValue(0);
  c_e0fill->setValue(0);
  c_e0hit->setValue(0);
  c_e1fill->setValue(0);
  c_e1hit->setValue(0);
  c_e2fill->setValue(0);
  c_e2hit->setValue(0);
  c_e3fill->setValue(0);
  c_e3hit->setValue(0);
  c_e0evt->setValue(0);
  c_e1evt->setValue(0);
  c_e2evt->setValue(0);
  c_e3evt->setValue(0);
  c_re0fill->setValue(0);
  c_re0hit->setValue(0);
  c_re1fill->setValue(0);
  c_re1hit->setValue(0);
  c_re2fill->setValue(0);
  c_re2hit->setValue(0);
  c_re3fill->setValue(0);
  c_re3hit->setValue(0);
  c_re0evt->setValue(0);
  c_re1evt->setValue(0);
  c_re2evt->setValue(0);
  c_re3evt->setValue(0);

  z_access->setValue(0);
  z_miss->setValue(0);
  z_raccess->setValue(0);
  z_rmiss->setValue(0);
  z_blocks->setValue(0);
  z_xevct->setValue(0);
  z_sevct->setValue(0);
  z_zevct->setValue(0);
  z_soevct->setValue(0);
  z_xoevct->setValue(0);
  z_xcevct->setValue(0);
  z_xtevct->setValue(0);
  z_xpevct->setValue(0);
  z_xrevct->setValue(0);
  z_xhit->setValue(0);
  z_czhit->setValue(0);
  z_tzhit->setValue(0);
  z_bzhit->setValue(0);
  z_e0fill->setValue(0);
  z_e0hit->setValue(0);
  z_e1fill->setValue(0);
  z_e1hit->setValue(0);
  z_e2fill->setValue(0);
  z_e2hit->setValue(0);
  z_e3fill->setValue(0);
  z_e3hit->setValue(0);
  z_e0evt->setValue(0);
  z_e1evt->setValue(0);
  z_e2evt->setValue(0);
  z_e3evt->setValue(0);
  z_re0fill->setValue(0);
  z_re0hit->setValue(0);
  z_re1fill->setValue(0);
  z_re1hit->setValue(0);
  z_re2fill->setValue(0);
  z_re2hit->setValue(0);
  z_re3fill->setValue(0);
  z_re3hit->setValue(0);
  z_re0evt->setValue(0);
  z_re1evt->setValue(0);
  z_re2evt->setValue(0);
  z_re3evt->setValue(0);

  t_access->setValue(0);
  t_miss->setValue(0);
  t_raccess->setValue(0);
  t_rmiss->setValue(0);
  t_blocks->setValue(0);
  t_xevct->setValue(0);
  t_sevct->setValue(0);
  t_zevct->setValue(0);
  t_soevct->setValue(0);
  t_xoevct->setValue(0);
  t_xcevct->setValue(0);
  t_xzevct->setValue(0);
  t_xpevct->setValue(0);
  t_xrevct->setValue(0);
  t_xhit->setValue(0);
  t_cthit->setValue(0);
  t_zthit->setValue(0);
  t_bthit->setValue(0);
  t_e0fill->setValue(0);
  t_e0hit->setValue(0);
  t_e1fill->setValue(0);
  t_e1hit->setValue(0);
  t_e2fill->setValue(0);
  t_e2hit->setValue(0);
  t_e3fill->setValue(0);
  t_e3hit->setValue(0);
  t_e0evt->setValue(0);
  t_e1evt->setValue(0);
  t_e2evt->setValue(0);
  t_e3evt->setValue(0);
  t_re0fill->setValue(0);
  t_re0hit->setValue(0);
  t_re1fill->setValue(0);
  t_re1hit->setValue(0);
  t_re2fill->setValue(0);
  t_re2hit->setValue(0);
  t_re3fill->setValue(0);
  t_re3hit->setValue(0);
  t_re0evt->setValue(0);
  t_re1evt->setValue(0);
  t_re2evt->setValue(0);
  t_re3evt->setValue(0);

  n_access->setValue(0);
  n_miss->setValue(0);
  n_raccess->setValue(0);
  n_rmiss->setValue(0);
  n_blocks->setValue(0);
  n_xevct->setValue(0);
  n_sevct->setValue(0);
  n_zevct->setValue(0);
  n_soevct->setValue(0);
  n_xoevct->setValue(0);
  n_xcevct->setValue(0);
  n_xzevct->setValue(0);
  n_xtevct->setValue(0);
  n_xpevct->setValue(0);
  n_xrevct->setValue(0);
  n_xhit->setValue(0);

  h_access->setValue(0);
  h_miss->setValue(0);
  h_raccess->setValue(0);
  h_rmiss->setValue(0);
  h_blocks->setValue(0);
  h_xevct->setValue(0);
  h_sevct->setValue(0);
  h_zevct->setValue(0);
  h_soevct->setValue(0);
  h_xoevct->setValue(0);
  h_xcevct->setValue(0);
  h_xzevct->setValue(0);
  h_xtevct->setValue(0);
  h_xcevct->setValue(0);
  h_xzevct->setValue(0);
  h_xtevct->setValue(0);
  h_xpevct->setValue(0);
  h_xrevct->setValue(0);
  h_xhit->setValue(0);

  d_access->setValue(0);
  d_miss->setValue(0);
  d_raccess->setValue(0);
  d_rmiss->setValue(0);
  d_blocks->setValue(0);
  d_xevct->setValue(0);
  d_sevct->setValue(0);
  d_zevct->setValue(0);
  d_xoevct->setValue(0);
  d_xcevct->setValue(0);
  d_xzevct->setValue(0);
  d_xtevct->setValue(0);
  d_xcevct->setValue(0);
  d_xzevct->setValue(0);
  d_xtevct->setValue(0);
  d_xpevct->setValue(0);
  d_xrevct->setValue(0);
  d_xhit->setValue(0);
  d_cdhit->setValue(0);
  d_zdhit->setValue(0);
  d_tdhit->setValue(0);
  d_bdhit->setValue(0);

  b_access->setValue(0);
  b_miss->setValue(0);
  b_raccess->setValue(0);
  b_rmiss->setValue(0);
  b_blocks->setValue(0);
  b_xevct->setValue(0);
  b_sevct->setValue(0);
  b_zevct->setValue(0);
  b_soevct->setValue(0);
  b_xoevct->setValue(0);
  b_xcevct->setValue(0);
  b_xzevct->setValue(0);
  b_xtevct->setValue(0);
  b_xcevct->setValue(0);
  b_xzevct->setValue(0);
  b_xtevct->setValue(0);
  b_xpevct->setValue(0);
  b_xrevct->setValue(0);
  b_xhit->setValue(0);
  b_bprod->setValue(0);
  b_bcons->setValue(0);
  b_cbhit->setValue(0);
  b_zbhit->setValue(0);
  b_tbhit->setValue(0);

  x_access->setValue(0);
  x_miss->setValue(0);
  x_raccess->setValue(0);
  x_rmiss->setValue(0);
  x_blocks->setValue(0);
  x_xevct->setValue(0);
  x_sevct->setValue(0);
  x_zevct->setValue(0);
  x_soevct->setValue(0);
  x_xoevct->setValue(0);
  x_xcevct->setValue(0);
  x_xzevct->setValue(0);
  x_xtevct->setValue(0);
  x_xcevct->setValue(0);
  x_xzevct->setValue(0);
  x_xtevct->setValue(0);
  x_xpevct->setValue(0);
  x_xrevct->setValue(0);
  x_xhit->setValue(0);

  p_access->setValue(0);
  p_miss->setValue(0);
  p_raccess->setValue(0);
  p_rmiss->setValue(0);
  p_blocks->setValue(0);
  p_xevct->setValue(0);
  p_sevct->setValue(0);
  p_zevct->setValue(0);
  p_soevct->setValue(0);
  p_xoevct->setValue(0);
  p_xcevct->setValue(0);
  p_xzevct->setValue(0);
  p_xtevct->setValue(0);
  p_xcevct->setValue(0);
  p_xzevct->setValue(0);
  p_xtevct->setValue(0);
  p_xrevct->setValue(0);
  p_xhit->setValue(0);

  g_access->setValue(0);
  g_miss->setValue(0);
  g_raccess->setValue(0);
  g_rmiss->setValue(0);
  g_blocks->setValue(0);
  g_xevct->setValue(0);
  g_sevct->setValue(0);
  g_zevct->setValue(0);
  g_soevct->setValue(0);
  g_xoevct->setValue(0);
  g_xcevct->setValue(0);
  g_xzevct->setValue(0);
  g_xtevct->setValue(0);
  g_xcevct->setValue(0);
  g_xzevct->setValue(0);
  g_xtevct->setValue(0);
  g_xrevct->setValue(0);
  g_xpevct->setValue(0);
  g_xhit->setValue(0);
  g_xblocks->setValue(0);
}

static void periodicReset()
{
  /* Reset statistics for current iteration */
  i_access->setValue(0);
  i_miss->setValue(0);
  i_raccess->setValue(0);
  i_rmiss->setValue(0);
  i_xevct->setValue(0);
  i_sevct->setValue(0);
  i_zevct->setValue(0);
  i_soevct->setValue(0);
  i_xoevct->setValue(0);
  i_xcevct->setValue(0);
  i_xzevct->setValue(0);
  i_xtevct->setValue(0);
  i_xcevct->setValue(0);
  i_xzevct->setValue(0);
  i_xtevct->setValue(0);
  i_xpevct->setValue(0);
  i_xrevct->setValue(0);
  i_xhit->setValue(0);

  c_access->setValue(0);
  c_miss->setValue(0);
  c_raccess->setValue(0);
  c_rmiss->setValue(0);
  c_xevct->setValue(0);
  c_sevct->setValue(0);
  c_zevct->setValue(0);
  c_soevct->setValue(0);
  c_xoevct->setValue(0);
  c_xzevct->setValue(0);
  c_xtevct->setValue(0);
  c_xzevct->setValue(0);
  c_xtevct->setValue(0);
  c_xpevct->setValue(0);
  c_xrevct->setValue(0);
  c_xhit->setValue(0);
  c_zchit->setValue(0);
  c_tchit->setValue(0);
  c_bchit->setValue(0);
  c_dchit->setValue(0);
  c_e0fill->setValue(0);
  c_e0hit->setValue(0);
  c_e1fill->setValue(0);
  c_e1hit->setValue(0);
  c_e2fill->setValue(0);
  c_e2hit->setValue(0);
  c_e3fill->setValue(0);
  c_e3hit->setValue(0);
  c_e0evt->setValue(0);
  c_e1evt->setValue(0);
  c_e2evt->setValue(0);
  c_e3evt->setValue(0);
  c_re0fill->setValue(0);
  c_re0hit->setValue(0);
  c_re1fill->setValue(0);
  c_re1hit->setValue(0);
  c_re2fill->setValue(0);
  c_re2hit->setValue(0);
  c_re3fill->setValue(0);
  c_re3hit->setValue(0);
  c_re0evt->setValue(0);
  c_re1evt->setValue(0);
  c_re2evt->setValue(0);
  c_re3evt->setValue(0);

  z_access->setValue(0);
  z_miss->setValue(0);
  z_raccess->setValue(0);
  z_rmiss->setValue(0);
  z_xevct->setValue(0);
  z_sevct->setValue(0);
  z_zevct->setValue(0);
  z_soevct->setValue(0);
  z_xoevct->setValue(0);
  z_xcevct->setValue(0);
  z_xtevct->setValue(0);
  z_xcevct->setValue(0);
  z_xtevct->setValue(0);
  z_xpevct->setValue(0);
  z_xrevct->setValue(0);
  z_xhit->setValue(0);
  z_czhit->setValue(0);
  z_tzhit->setValue(0);
  z_bzhit->setValue(0);
  z_e0fill->setValue(0);
  z_e0hit->setValue(0);
  z_e1fill->setValue(0);
  z_e1hit->setValue(0);
  z_e2fill->setValue(0);
  z_e2hit->setValue(0);
  z_e3fill->setValue(0);
  z_e3hit->setValue(0);
  z_e0evt->setValue(0);
  z_e1evt->setValue(0);
  z_e2evt->setValue(0);
  z_e3evt->setValue(0);
  z_re0fill->setValue(0);
  z_re0hit->setValue(0);
  z_re1fill->setValue(0);
  z_re1hit->setValue(0);
  z_re2fill->setValue(0);
  z_re2hit->setValue(0);
  z_re3fill->setValue(0);
  z_re3hit->setValue(0);
  z_re0evt->setValue(0);
  z_re1evt->setValue(0);
  z_re2evt->setValue(0);
  z_re3evt->setValue(0);

  t_access->setValue(0);
  t_miss->setValue(0);
  t_raccess->setValue(0);
  t_rmiss->setValue(0);
  t_xevct->setValue(0);
  t_sevct->setValue(0);
  t_zevct->setValue(0);
  t_soevct->setValue(0);
  t_xoevct->setValue(0);
  t_xcevct->setValue(0);
  t_xzevct->setValue(0);
  t_xcevct->setValue(0);
  t_xzevct->setValue(0);
  t_xpevct->setValue(0);
  t_xrevct->setValue(0);
  t_xhit->setValue(0);
  t_cthit->setValue(0);
  t_zthit->setValue(0);
  t_bthit->setValue(0);
  t_e0fill->setValue(0);
  t_e0hit->setValue(0);
  t_e1fill->setValue(0);
  t_e1hit->setValue(0);
  t_e2fill->setValue(0);
  t_e2hit->setValue(0);
  t_e3fill->setValue(0);
  t_e3hit->setValue(0);
  t_e0evt->setValue(0);
  t_e1evt->setValue(0);
  t_e2evt->setValue(0);
  t_e3evt->setValue(0);
  t_re0fill->setValue(0);
  t_re0hit->setValue(0);
  t_re1fill->setValue(0);
  t_re1hit->setValue(0);
  t_re2fill->setValue(0);
  t_re2hit->setValue(0);
  t_re3fill->setValue(0);
  t_re3hit->setValue(0);
  t_re0evt->setValue(0);
  t_re1evt->setValue(0);
  t_re2evt->setValue(0);
  t_re3evt->setValue(0);

  n_access->setValue(0);
  n_miss->setValue(0);
  n_raccess->setValue(0);
  n_rmiss->setValue(0);
  n_xevct->setValue(0);
  n_sevct->setValue(0);
  n_zevct->setValue(0);
  n_soevct->setValue(0);
  n_xoevct->setValue(0);
  n_xcevct->setValue(0);
  n_xzevct->setValue(0);
  n_xtevct->setValue(0);
  n_xcevct->setValue(0);
  n_xzevct->setValue(0);
  n_xtevct->setValue(0);
  n_xpevct->setValue(0);
  n_xrevct->setValue(0);
  n_xhit->setValue(0);

  h_access->setValue(0);
  h_miss->setValue(0);
  h_raccess->setValue(0);
  h_rmiss->setValue(0);
  h_xevct->setValue(0);
  h_sevct->setValue(0);
  h_zevct->setValue(0);
  h_soevct->setValue(0);
  h_xoevct->setValue(0);
  h_xcevct->setValue(0);
  h_xzevct->setValue(0);
  h_xtevct->setValue(0);
  h_xcevct->setValue(0);
  h_xzevct->setValue(0);
  h_xtevct->setValue(0);
  h_xpevct->setValue(0);
  h_xrevct->setValue(0);
  h_xhit->setValue(0);

  d_access->setValue(0);
  d_miss->setValue(0);
  d_raccess->setValue(0);
  d_rmiss->setValue(0);
  d_xevct->setValue(0);
  d_sevct->setValue(0);
  d_zevct->setValue(0);
  d_soevct->setValue(0);
  d_xoevct->setValue(0);
  d_xcevct->setValue(0);
  d_xzevct->setValue(0);
  d_xtevct->setValue(0);
  d_xcevct->setValue(0);
  d_xzevct->setValue(0);
  d_xtevct->setValue(0);
  d_xpevct->setValue(0);
  d_xrevct->setValue(0);
  d_xhit->setValue(0);
  d_cdhit->setValue(0);
  d_zdhit->setValue(0);
  d_tdhit->setValue(0);
  d_bdhit->setValue(0);

  b_access->setValue(0);
  b_miss->setValue(0);
  b_raccess->setValue(0);
  b_rmiss->setValue(0);
  b_xevct->setValue(0);
  b_sevct->setValue(0);
  b_zevct->setValue(0);
  b_soevct->setValue(0);
  b_xoevct->setValue(0);
  b_xcevct->setValue(0);
  b_xzevct->setValue(0);
  b_xtevct->setValue(0);
  b_xcevct->setValue(0);
  b_xzevct->setValue(0);
  b_xtevct->setValue(0);
  b_xpevct->setValue(0);
  b_xrevct->setValue(0);
  b_xhit->setValue(0);
  b_cbhit->setValue(0);
  b_zbhit->setValue(0);
  b_tbhit->setValue(0);

  x_access->setValue(0);
  x_miss->setValue(0);
  x_raccess->setValue(0);
  x_rmiss->setValue(0);
  x_xevct->setValue(0);
  x_sevct->setValue(0);
  x_zevct->setValue(0);
  x_soevct->setValue(0);
  x_xoevct->setValue(0);
  x_xcevct->setValue(0);
  x_xzevct->setValue(0);
  x_xtevct->setValue(0);
  x_xcevct->setValue(0);
  x_xzevct->setValue(0);
  x_xtevct->setValue(0);
  x_xpevct->setValue(0);
  x_xrevct->setValue(0);
  x_xhit->setValue(0);

  p_access->setValue(0);
  p_miss->setValue(0);
  p_raccess->setValue(0);
  p_rmiss->setValue(0);
  p_xevct->setValue(0);
  p_sevct->setValue(0);
  p_zevct->setValue(0);
  p_soevct->setValue(0);
  p_xoevct->setValue(0);
  p_xcevct->setValue(0);
  p_xzevct->setValue(0);
  p_xtevct->setValue(0);
  p_xcevct->setValue(0);
  p_xzevct->setValue(0);
  p_xtevct->setValue(0);
  p_xrevct->setValue(0);
  p_xhit->setValue(0);

  g_access->setValue(0);
  g_miss->setValue(0);
  g_raccess->setValue(0);
  g_rmiss->setValue(0);
  g_xevct->setValue(0);
  g_sevct->setValue(0);
  g_zevct->setValue(0);
  g_soevct->setValue(0);
  g_xoevct->setValue(0);
  g_xcevct->setValue(0);
  g_xzevct->setValue(0);
  g_xtevct->setValue(0);
  g_xcevct->setValue(0);
  g_xzevct->setValue(0);
  g_xtevct->setValue(0);
  g_xrevct->setValue(0);
  g_xpevct->setValue(0);
  g_xhit->setValue(0);
  g_xblocks->setValue(0);
}

void update_access_stats(cachesim_cache *cache, memory_trace *info, cache_access_status ret)
{
  /* Update statistics only for fill */
  if (info->fill == TRUE)
  {
    if (ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
    {
      cache->miss[info->stream]++;

      if (info->stream >= PS && info->stream <= PS + MAX_CORES)
      {
        (*p_rmiss)++;

      }
      else
      {
        switch (info->stream)
        {
          case IS:
            (*i_rmiss)++;
            break;

          case CS:
            (*c_rmiss)++;
            break;

          case ZS:
            (*z_rmiss)++;
            break;

          case TS:
            (*t_rmiss)++;
            break;

          case NS:
            (*n_rmiss)++;
            break;

          case HS:
            (*h_rmiss)++;
            break;

          case BS:
            (*b_rmiss)++;
            break;

          case DS:
            (*d_rmiss)++;
            break;

          case XS:
            (*x_rmiss)++;
            break;

          case GP:
            (*g_rmiss)++;
            break;

          default:
            cout << "Illegal stream " << (int)(info->stream) << " for address ";
            cout << hex << info->address << " type : ";
            cout << dec << (int)info->fill << " spill: " << dec << (int)info->spill;
            assert(1);
        }
      }

      if (ret.fate == CACHE_ACCESS_RPLC)
      { 
        /* Current stream has a block allocated */
        assert(ret.stream != NN);

        if (ret.stream == CS || ret.stream == ZS || ret.stream == TS)
        {
          updateEopchEvictCountersForRead(ret);
        }
      }
    }
    else
    {
      assert(ret.fate == CACHE_ACCESS_HIT);

      /* Update C, Z, T epoch counters */
      if (info->stream == ret.stream &&
          (ret.stream == CS || ret.stream == ZS || ret.stream == TS))
      {
        if (ret.epoch != 3)
        {
          updateEpochHitCountersForRead(ret);
        }
      }
    }

    if (info->stream == CS || info->stream == ZS || info->stream == TS)
    {
      if (ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
      {
        updateEpochFillCountersForRead(info->stream, 0);
      }
      else
      {
        if (ret.epoch != 3)
        {
          if (info->stream != ret.stream)
          {
            updateEpochFillCountersForRead(info->stream, 0);
          }
          else
          {
            updateEpochFillCountersForRead(info->stream, ret.epoch + 1);
          }
        }
      }
    }
    
    cache->access[info->stream]++;

    if (info->stream >= PS && info->stream <= PS + MAX_CORES)
    {
      (*p_raccess)++;
    }
    else
    {
      switch (info->stream)
      {
        case IS:
          (*i_raccess)++;
          break;

        case CS:
          (*c_raccess)++;

          break;

        case ZS:
          (*z_raccess)++;
          break;

        case TS:
          (*t_raccess)++;
          break;

        case NS:
          (*n_raccess)++;
          break;

        case HS:
          (*h_raccess)++;
          break;

        case BS:
          (*b_raccess)++;
          break;

        case DS:
          (*d_raccess)++;
          break;

        case XS:
          (*x_raccess)++;
          break;

        case GP:
          (*g_raccess)++;
          break;

        default:
          cout << "Illegal stream " << (int)info->stream << " for address ";
          cout << hex << info->address << " type : ";
          cout << dec << (int)info->fill << " spill: " << dec << (int)info->spill;
          assert(0);
      }
    }
  }

  if (ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
  {
    if (info->stream >= PS && info->stream <= PS + MAX_CORES)
    {
      (*p_miss)++;

      if (ret.stream != info->stream)
        (*p_blocks)++;
    }
    else
    {
      switch (info->stream)
      {
        case IS:
          (*i_miss)++;

          if (ret.stream != info->stream)
            (*i_blocks)++;
          break;

        case CS:
          (*c_miss)++;

          if (ret.stream != info->stream)
            (*c_blocks)++;

          break;

        case ZS:
          (*z_miss)++;

          if (ret.stream != info->stream)
            (*z_blocks)++;
          break;

        case TS:
          (*t_miss)++;

          if (ret.stream != info->stream)
            (*t_blocks)++;
          break;

        case NS:
          (*n_miss)++;

          if (ret.stream != info->stream)
            (*n_blocks)++;
          break;

        case HS:
          (*h_miss)++;

          if (ret.stream != info->stream)
            (*h_blocks)++;
          break;

        case BS:
          (*b_miss)++;

          if (ret.stream != info->stream)
            (*b_blocks)++;
          break;

        case DS:
          (*d_miss)++;

          if (ret.stream != info->stream)
            (*d_blocks)++;
          break;

        case XS:
          (*x_miss)++;

          if (ret.stream != info->stream)
            (*x_blocks)++;
          break;


        case GP:
          (*g_miss)++;

          if (ret.stream != info->stream)
            (*g_blocks)++;
          break;

        default:
          cout << "Illegal stream " << (int)(info->stream) << " for address ";
          cout << hex << info->address << " type : ";
          cout << dec << (int)info->fill << " spill: " << dec << (int)info->spill;
          assert(0);
      }
    }

    if (ret.fate == CACHE_ACCESS_RPLC)
    { 
      assert(ret.last_rrpv <= MAX_RRPV);
      assert(ret.stream <= TST);

      /* Current stream has a block allocated */
      assert(ret.stream != NN);

      /* Update C, Z, T epoch counters */
      if (ret.stream == CS || ret.stream == ZS || ret.stream == TS)
      {
        updateEopchEvictCounters(ret);
      }

      if (ret.stream != info->stream)
      {
        if (ret.stream >= PS && ret.stream <= PS + MAX_CORES)
        {
          if (info->stream != PS)
            (*p_xevct)++;

          if (info->stream == CS)
          {
            (*p_xcevct)++;
          }
          else
          {
            if (info->stream == ZS)
            {
              (*p_xcevct)++;
            }
            {
              if (info->stream == TS)
              {
                (*p_xtevct)++;
              }
              else
              {
                if (info->stream != PS)
                {
                  (*p_xrevct)++;
                }
              }
            }
          }
#if 0
          if (p_blocks->getValue())
            (*p_blocks)--;
#endif
        }
        else
        {
          switch(ret.stream)
          {
            case IS:
              (*i_xevct)++;

              if (info->stream == CS)
              {
                (*i_xcevct)++;
              }
              else
              {
                if (info->stream == ZS)
                {
                  (*i_xzevct)++;
                }
                {
                  if (info->stream == TS)
                  {
                    (*i_xtevct)++;
                  }
                  else
                  {
                    if (info->stream == PS)
                    {
                      (*i_xpevct)++;
                    }
                    else
                    {
                      if (info->stream != IS)
                      {
                        (*i_xrevct)++;
                      }
                    }
                  }
                }
              }
#if 0
              assert(i_blocks->getValue());
              (*i_blocks)--;
#endif
              break;

            case CS:
              (*c_xevct)++;

              if (info->stream == ZS)
              {
                (*c_xzevct)++;
              }
              else
              {
                if (info->stream == TS)
                {
                  (*c_xtevct)++;
                }
                else
                {
                  if (info->stream == PS)
                  {
                    (*c_xpevct)++;
                  }
                  else
                  {
                    if (info->stream != CS)
                    {
                      (*c_xrevct)++;
                    }
                  }
                }
              }
#if 0
              assert(c_blocks->getValue(1) && info->stream != CS);
              (*c_blocks)--;
#endif
              break;

            case ZS:
              (*z_xevct)++;

              if (info->stream == CS)
              {
                (*z_xcevct)++;
              }
              else
              {
                if (info->stream == TS)
                {
                  (*z_xtevct)++;
                }
                else
                {
                  if (info->stream == PS)
                  {
                    (*z_xpevct)++;
                  }
                  else
                  {
                    if (info->stream != ZS)
                    {
                      (*z_xrevct)++;
                    }
                  }
                }
              }
#if 0
              assert(z_blocks->getValue());
              (*z_blocks)--;
#endif
              break;

            case TS:
              (*t_xevct)++;

              if (info->stream == CS)
              {
                (*t_xcevct)++;
              }
              else
              {
                if (info->stream == ZS)
                {
                  (*t_xzevct)++;
                }
                else
                {
                  if (info->stream == PS)
                  {
                    (*t_xpevct)++;
                  }
                  else
                  {
                    if (info->stream != TS)
                    {
                      (*t_xrevct)++;
                    }
                  }
                }
              }

              /* At least one block must belong to TS */
#if 0
              assert(t_blocks->getValue());
              (*t_blocks)--;
#endif
              break;

            case NS:
              (*n_xevct)++;

              if (info->stream == CS)
              {
                (*n_xcevct)++;
              }
              else
              {
                if (info->stream == ZS)
                {
                  (*n_xzevct)++;
                }
                else
                {
                  if (info->stream == TS)
                  {
                    (*n_xtevct)++;
                  }
                  else
                  {
                    if (info->stream == PS)
                    {
                      (*n_xpevct)++;
                    }
                    else
                    {
                      if (info->stream != NS)
                      {
                        (*n_xrevct)++;
                      }
                    }
                  }
                }
              }
#if 0
              assert(n_blocks->getValue());
              (*n_blocks)--;
#endif
              break;

            case HS:
              (*h_xevct)++;

              if (info->stream == CS)
              {
                (*h_xcevct)++;
              }
              else
              {
                if (info->stream == ZS)
                {
                  (*h_xzevct)++;
                }
                else
                {
                  if (info->stream == TS)
                  {
                    (*h_xtevct)++;
                  }
                  else
                  {
                    if (info->stream == PS)
                    {
                      (*h_xpevct)++;
                    }
                    else
                    {
                      if (info->stream != HS)
                      {
                        (*h_xrevct)++;
                      }
                    }
                  }
                }
              }
#if 0
              assert(h_blocks->getValue());
              (*h_blocks)--;
#endif
              break;

            case BS:
              (*b_xevct)++;

              if (info->stream == CS)
              {
                (*b_xcevct)++;
              }
              else
              {
                if (info->stream == ZS)
                {
                  (*b_xzevct)++;
                }
                else
                {
                  if (info->stream == TS)
                  {
                    (*b_xtevct)++;
                  }
                  else
                  {
                    if (info->stream == PS)
                    {
                      (*b_xpevct)++;
                    }
                    else
                    {
                      if (info->stream != BS)
                      {
                        (*b_xrevct)++;
                      }
                    }
                  }
                }
              }
#if 0
              assert(b_blocks->getValue());
              (*b_blocks)--;
#endif
              break;

            case DS:
              (*d_xevct)++;

              if (info->stream == CS)
              {
                (*d_xcevct)++;
              }
              else
              {
                if (info->stream == ZS)
                {
                  (*d_xzevct)++;
                }
                else
                {
                  if (info->stream == TS)
                  {
                    (*d_xtevct)++;
                  }
                  else
                  {
                    if (info->stream == PS)
                    {
                      (*d_xpevct)++;
                    }
                    else
                    {
                      if (info->stream != DS)
                      {
                        (*d_xrevct)++;
                      }
                    }
                  }
                }
              }
#if 0
              assert(d_blocks->getValue());
              (*d_blocks)--;
#endif
              break;

            case XS:
              (*x_xevct)++;

              if (info->stream == CS)
              {
                (*x_xcevct)++;
              }
              else
              {
                if (info->stream == ZS)
                {
                  (*x_xzevct)++;
                }
                else
                {
                  if (info->stream == TS)
                  {
                    (*x_xtevct)++;
                  }
                  else
                  {
                    if (info->stream == PS)
                    {
                      (*x_xpevct)++;
                    }
                    else
                    {
                      if (info->stream != XS)
                      {
                        (*x_xrevct)++;
                      }
                    }
                  }
                }
              }

#if 0
              assert(x_blocks->getValue());
              (*x_blocks)--;
#endif
              break;

            case GP:
              (*g_xevct)++;

              if (info->stream == CS)
              {
                (*g_xcevct)++;
              }
              else
              {
                if (info->stream == ZS)
                {
                  (*g_xzevct)++;
                }
                else
                {
                  if (info->stream == TS)
                  {
                    (*g_xtevct)++;
                  }
                  else
                  {
                    if (info->stream == PS)
                    {
                      (*g_xpevct)++;
                    }
                    else
                    {
                      if (info->stream != XS)
                      {
                        (*g_xrevct)++;
                      }
                    }
                  }
                }
              }

              break;

            default:
              cout << "Illegal stream " << (int)(info->stream) << " for address ";
              cout << hex << info->address << " type : ";
              cout << dec << (int)info->fill << " spill: " << dec << (int)info->spill;
              assert(0);
          }
        }
      }
      else
      {
        if (info->stream >= PS && info->stream <= PS + MAX_CORES)
        {
          (*p_sevct)++;
        }
        else
        {
          switch(info->stream)
          {
            case IS:
              (*i_sevct)++;
              break;

            case CS:
              (*c_sevct)++;
              break;

            case ZS:
              (*z_sevct)++;
              break;

            case TS:
              (*t_sevct)++;
              break;

            case NS:
              (*n_sevct)++;
              break;

            case HS:
              (*h_sevct)++;
              break;

            case BS:
              (*b_sevct)++;
              break;

            case DS:
              (*d_sevct)++;
              break;

            case XS:
              (*x_sevct)++;
              break;

            case GP:
              (*g_sevct)++;
              break;

            default:
              cout << "Illegal stream " << (int)(info->stream) << " for address ";
              cout << hex << info->address << " type : ";
              cout << dec << (int)info->fill << " spill: " << dec << (int)info->spill;
              assert(0);
          }
        }
      }

      if (ret.access == 0)
      {
        if (ret.stream >= PS && ret.stream <= PS + MAX_CORES)
        {
          (*p_zevct)++;

          if (info->stream != ret.stream)
          {
            (*p_xoevct)++;
          }
          else
          {
            (*p_soevct)++;
          }
        }
        else
        {
          switch (ret.stream)
          {
            case IS:
              (*i_zevct)++;

              if (info->stream != ret.stream)
              {
                (*i_xoevct)++;
              }
              else
              {
                (*i_soevct)++;
              }
              break;

            case CS:
              (*c_zevct)++;

              if (info->stream != ret.stream)
              {
                (*c_xoevct)++;
              }
              else
              {
                (*c_soevct)++;
              }
              break;

            case ZS:
              (*z_zevct)++;

              if (info->stream != ret.stream)
              {
                (*z_xoevct)++;
              }
              else
              {
                (*z_soevct)++;
              }
              break;

            case TS:
              (*t_zevct)++;

              if (info->stream != ret.stream)
              {
                (*t_xoevct)++;
              }
              else
              {
                (*t_soevct)++;
              }
              break;

            case NS:
              (*n_zevct)++;

              if (info->stream != ret.stream)
              {
                (*n_xoevct)++;
              }
              else
              {
                (*n_soevct)++;
              }
              break;

            case HS:
              (*h_zevct)++;

              if (info->stream != ret.stream)
              {
                (*h_xoevct)++;
              }
              else
              {
                (*h_soevct)++;
              }
              break;

            case BS:
              (*b_zevct)++;

              if (info->stream != ret.stream)
              {
                (*b_xoevct)++;
              }
              else
              {
                (*b_soevct)++;
              }
              break;

            case DS:
              (*d_zevct)++;

              if (info->stream != ret.stream)
              {
                (*d_xoevct)++;
              }
              else
              {
                (*d_soevct)++;
              }
              break;

            case XS:
              (*x_zevct)++;

              if (info->stream != ret.stream)
              {
                (*x_xoevct)++;
              }
              else
              {
                (*x_soevct)++;
              }
              break;

            case GP:
              (*g_zevct)++;

              if (info->stream != ret.stream)
              {
                (*g_xoevct)++;
              }
              else
              {
                (*g_soevct)++;
              }
              break;

            default:
              cout << "Illegal stream " << (int)(ret.stream);
              assert(1);
          }
        }
      }
    }
  }
  else
  {
    assert(ret.fate == CACHE_ACCESS_HIT);

    /* Update C, Z, T epoch counters */
    if (info->stream == ret.stream && 
        (ret.stream == CS || ret.stream == ZS || ret.stream == TS))
    {
      updateEpochHitCounters(ret);
    }

    /* Update stats for incoming stream */
    if (info->stream != ret.stream)
    {
      if (info->stream >= PS && info->stream <= PS + MAX_CORES)
      {
        (*p_xhit)++;
        (*p_blocks)++;
      }
      else
      {
        switch (info->stream)
        {
          case IS:
            (*i_xhit)++;
            (*i_blocks)++;
            break;

          case CS:
            (*c_xhit)++;
            (*c_blocks)++;

            switch (ret.stream)
            {
              case ZS:
                (*c_zchit)++;
                break;

              case TS:
                (*c_tchit)++;
                break;

              case BS:
                (*c_bchit)++;
                break;

              case DS:
                (*c_dchit)++;
                break;
            }

            break;

          case ZS:
            (*z_xhit)++;
            (*z_blocks)++;

            switch (ret.stream)
            {
              case CS:
                (*z_czhit)++;
                break;

              case TS:
                (*z_tzhit)++;
                break;

              case BS:
                (*z_bzhit)++;
                break;
            }

            break;

          case TS:
            (*t_xhit)++;
            (*t_blocks)++;
#if 0
            assert(info->spill == 0);
#endif
            switch (ret.stream)
            {
              case CS:
                (*t_cthit)++;

                if (ret.dirty)
                {
#if 0
                  assert(info->spill == 0);
#endif
                  assert(c_ccons->getValue(1) < c_cprod->getValue(1));

                  (*c_ccons)++;
                }
                break;

              case ZS:
                (*t_zthit)++;
                break;

              case BS:
                (*t_bthit)++;

                if (ret.dirty)
                {
#if 0
                  assert(info->spill == 0 && info->fill == 1);
#endif
                  assert(b_bcons->getValue(1) < b_bprod->getValue(1));

                  (*b_bcons)++;
                }
                break;
            }

            break;

          case NS:
            (*n_xhit)++;
            (*n_blocks)++;
            break;

          case HS:
            (*h_xhit)++;
            (*h_blocks)++;
            break;

          case BS:
            (*b_xhit)++;
            (*b_blocks)++;

            switch (ret.stream)
            {
              case CS:
                (*b_cbhit)++;
                break;

              case TS:
                (*b_tbhit)++;
                break;

              case ZS:
                (*b_zbhit)++;
                break;
            }
            break;

          case DS:
            (*d_xhit)++;
            (*d_blocks)++;

            switch (ret.stream)
            {
              case CS:
                (*d_cdhit)++;
                break;

              case TS:
                (*d_tdhit)++;
                break;

              case ZS:
                (*d_zdhit)++;
                break;

              case BS:
                (*d_bdhit)++;
                break;
            }

            break;

          case XS:
            (*x_xhit)++;
            (*x_blocks)++;

            break;

          case GP:
            (*g_xhit)++;
            (*g_xblocks)++;
            break;

          default:
            cout << "Illegal stream " << (int)info->stream << " for address ";
            cout << hex << info->address << " type : ";
            cout << dec << (int)info->fill << " spill: " << dec << (int)info->spill;
            assert(0);
        }
      }
    }
  }

  if (info->stream == CS || info->stream == ZS || info->stream == TS)
  {
    if (ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
    {
      updateEpochFillCounters(info->stream, 0);
    }
    else
    {
      if (ret.epoch != 3)
      {
        if (info->stream != ret.stream)
        {
          updateEpochFillCounters(info->stream, 0);
        }
        else
        {
          updateEpochFillCounters(info->stream, ret.epoch + 1);
        }
      }
    }
  }

  if (info->stream >= PS && info->stream <= PS + MAX_CORES)
  {
    (*p_access)++;
  }
  else
  {
    switch (info->stream)
    {
      case IS:
        (*i_access)++;
        break;

      case CS:
        (*c_access)++;

        if (info->spill)
        {
          (*c_cprod)++;
        }
        break;

      case ZS:
        (*z_access)++;
        break;

      case TS:
        (*t_access)++;
        break;

      case NS:
        (*n_access)++;
        break;

      case HS:
        (*h_access)++;
        break;

      case BS:
        (*b_access)++;

        if (info->spill)
        {
          (*b_bprod)++;
        }
        break;

      case DS:
        (*d_access)++;
        break;

      case XS:
        (*x_access)++;
        break;

      case GP:
        (*g_access)++;
        break;

      default:
        cout << "Illegal stream " << (int)info->stream << " for address ";
        cout << hex << info->address << " type : ";
        cout << dec << (int)info->fill << " spill: " << dec << (int)info->spill;
        assert(0);
    }
  }
}

ub1 dram_cycle(cachesim_cache *cache, ub8 dramcycle)
{
  memory_trace        *info;
  cache_access_status  ret;
  row_stats           *row_info;
  ub8                  row_id;
  dram_queue_time     *dram_time;

  map<ub8, ub8>::iterator row_itr;

  assert(cache->dramsim_enable);

  dramsim_update();

  info = dram_response();

  if (info)
  {
    assert(pending_requests);

    /* Restore old physical address */
    info->address = info->vtl_addr;

    cs_free_mshr(cache, info);

    pending_requests -= 1;

    assert(dramcycle >= info->cycle);

    if (info->fill)
    {
      dram_request_cycle[info->stream] += dramcycle - info->cycle;

      if (info->policy_data)
      {
        dram_time = (dram_queue_time *)(info->policy_data);

        dram_tq_cycle[info->stream] += dram_time->tq_time;
        dram_cq_cycle[info->stream] += dram_time->cq_time;
      }

      ret = cachesim_fill_block(cache, info);

      update_access_stats(cache, info, ret);
    }
    
    if (info->policy_data)
    {
      free(info->policy_data);
    }
    
    /* Update DRAM access completion stats */
    update_dramsim_access_completion_stats(cache, info);

    free(info);
  }

  return (pending_requests == 0);
}

ub1 cache_cycle(cachesim_cache *cache, cachesim_cache *shadow_tag, ub8 cachecycle)
{
  cs_qnode             *current_queue;  /* Queue for last request */
  cs_qnode             *current_node;   /* Last request node */
  memory_trace         *info;           /* Generic structure used for requests between units */
  memory_trace         *new_info;       /* Generic structure used for requests between units */
  cache_access_status   ret;            /* Cache access status */
  ub1                   sim_end;
  
  memset(&ret, 0, sizeof(cache_access_status));
  
  sim_end = TRUE;

  if (!gzeof(cache->trc_file) || !cs_qempty(&(cache->rdylist)) || !cs_qempty(&(cache->gpu_rdylist)) || !cs_qempty(&(cache->plist)))
  {
    info          = NULL;
    current_queue = NULL;
    current_node  = NULL;

    if ((cs_qempty(&(cache->rdylist)) || cs_qempty(&(cache->gpu_rdylist))) && !gzeof(cache->trc_file))
    {
      new_info = (memory_trace *)calloc(1, sizeof(memory_trace));
      assert(new_info);
  
      gzread(cache->trc_file, (char *)new_info, sizeof(memory_trace));

      if (!gzeof(cache->trc_file))
      {
#if 0
        cs_qappend(&(cache->rdylist), (ub1 *)new_info);  
#endif

        if (GPU_STREAM(new_info->stream))
        {
          cs_qappend(&(cache->gpu_rdylist), (ub1 *)new_info);  
        }
        else
        {
          cs_qappend(&(cache->rdylist), (ub1 *)new_info);  
        }
      }
      else
      {
        free(new_info);
      }
    }

    /* Read entire trace file and run the simulation */

#define QUEUE_HEAD(q)       ((cs_qnode *)((q)->next))
#define QUEUE_HEAD_DATA(q)  (((cs_qnode *)((q)->next))->data)

    if (cs_qempty(&(cache->plist)))
    {
      /* Enqueue GPU accesses based on access rate */
      if (!cs_qempty(&(cache->gpu_rdylist)))
      {
        if (++(cache->gpu_fill) == cache->gpu_fill_frequency)
        {
          current_queue = &(cache->gpu_rdylist);
          current_node  = (cs_qnode*)QUEUE_HEAD(&(cache->gpu_rdylist));

          info = (memory_trace *)QUEUE_HEAD_DATA(&(cache->gpu_rdylist));

          cache->gpu_fill = 0;
        }
        else
        {
          if (!cs_qempty(&(cache->rdylist)))
          {
            current_queue = &(cache->rdylist);
            current_node  = (cs_qnode*)QUEUE_HEAD(&(cache->rdylist));

            info = (memory_trace *)QUEUE_HEAD_DATA(&(cache->rdylist));
          }
        }
      }
      else
      {
        if (!cs_qempty(&(cache->rdylist)))
        {
          current_queue = &(cache->rdylist);
          current_node  = (cs_qnode*)QUEUE_HEAD(&(cache->rdylist));

          info = (memory_trace *)QUEUE_HEAD_DATA(&(cache->rdylist));
        }
      }
    }
    else
    {
      current_queue = &(cache->plist);
      current_node  = (cs_qnode*)QUEUE_HEAD(&(cache->plist));

      info = (memory_trace *)QUEUE_HEAD_DATA(&(cache->plist));
    }

#undef QUEUE_HEAD
#undef QUEUE_HEAD_DATA

    if (info)
#if 0
    if (info && cachecycle >= info->cycle)
#endif
    {
      assert(current_queue);
      assert(current_node);

      info->vtl_addr  = info->address;

      if (cache->dramsim_trace == FALSE)
      {
        /* Increment per-set access count */
        ret = cachesim_incl_cache(cache, shadow_tag, info->address, 0, info->stream, info);

        assert(ret.fate == CACHE_ACCESS_HIT || ret.fate == CACHE_ACCESS_MISS ||
            ret.fate == CACHE_ACCESS_NACK || ret.fate == CACHE_ACCESS_UNK || 
            ret.fate == CACHE_ACCESS_RPLC);

        if (ret.fate == CACHE_ACCESS_HIT || ret.fate == CACHE_ACCESS_MISS ||
            ret.fate == CACHE_ACCESS_UNK || ret.fate == CACHE_ACCESS_RPLC)
        {
          cs_qdelete(current_queue, current_node);

          /* For cache only simulations, fill happens along with lookup. So,
           * info can be freed at this point. */
          if (ret.fate == CACHE_ACCESS_HIT || cache->dramsim_enable == FALSE)
          {
            update_access_stats(cache, info, ret);
          }
          else
          {
            if (info->spill)
            {
              update_access_stats(cache, info, ret);
            }
          }

          /* If request has completed free info */
          if (ret.fate != CACHE_ACCESS_UNK)
          {
            free(info);
          }
        }
      }
      else
      {
        ret = cachesim_only_dram(cache, info->address, 0, info->stream, info);

        if (ret.fate == CACHE_ACCESS_HIT || ret.fate == CACHE_ACCESS_MISS ||
            ret.fate == CACHE_ACCESS_UNK || ret.fate == CACHE_ACCESS_RPLC)
        {
          cs_qdelete(current_queue, current_node);

          /* If request has completed free info */
          if (ret.fate != CACHE_ACCESS_UNK)
          {
            free(info);
          }
        }
      }
      
      /* Update cachecycle interval end */
      if (cache->cachecycle_interval >= CYCLE_INTERVAL)
      {
        cache->cachecycle_interval = 0;
      }
    }

    sim_end = FALSE;
  }

  return sim_end;
}

/* Simulation start */
int main(int argc, char **argv)
{
  ub8  inscnt;
  ub8  per_unit_miss[TST];        /* Per unit misses */
  ub8  per_stream_hits[TST + 1];  /* Per stream hits */
  ub8  per_stream_access[TST + 1];/* Per stream access */
  ub8  acc_per_unit[TST];         /* Per unit access */
  ub4  cache_count;               /* # cache to simulate */
  ub4  way_count;                 /* Associativity */
  ub4  max_min_ratio;
  ub8 *cache_sizes;
  ub8 *cache_ways;
  sb1 *trc_file_name;
  sb1 *config_file_name;
  sb4  opt;
  ub8  cache_set;
  ub8  cache_ctime;
  ub8  shadow_tag_ctime;
  ub8  dram_ctime;
  ub8  cache_next_cycle;
  ub8  dram_next_cycle;
  ub8  current_cycle;
  ub8  total_cycle;
  ub1  dram_done;
  ub1  cache_done;

  gzofstream stats_stream;

  /* Cache configuration parser */
  SimParameters       sim_params;
  struct cache_params params;
  cachesim_cache      l3cache;
  cachesim_cache      shadow_tags;
  ConfigLoader       *cfg_loader;
  StatisticsManager  *sm;

  /* Verify bitness */
  assert(sizeof(ub8) == 8 && sizeof(ub4) == 4);

  inscnt              = 0;
  trc_file_name       = NULL;
  config_file_name    = NULL;
  invalid_hits        = 0;
  pending_requests    = 0;
  total_cycle         = 0;
  cache_done          = TRUE;
  access_shadow_tag   = TRUE;
  access_jumped       = 0;

  for (ub1 stream = NN; stream <= TST; stream++)
  {
    dram_request_cycle[stream]  = 0;
  }

  /* Instantiate statistics manager */
  sm = &StatisticsManager::instance(); 

  while ((opt = getopt(argc, argv, "f:t:")) != -1)
  {
    switch (opt)
    {
      case 'f':
        config_file_name = (sb1 *) malloc(strlen(optarg) + 1);
        strncpy(config_file_name, optarg, strlen(optarg));
        printf("f:%s\n", config_file_name);
        break;

      case 't':
        trc_file_name = (sb1 *) malloc(strlen(optarg) + 1);
        strncpy(trc_file_name, optarg, strlen(optarg));
        printf("t:%s\n", trc_file_name);
        break;

      default:
        printf("Invalid option %c\n", opt);
        exit(EXIT_FAILURE);
    }
  }

  cfg_loader = new ConfigLoader(config_file_name);
  cfg_loader->getParameters(&sim_params);

  /* Ensure max and min are power of two */
  assert(sim_params.lcP.minCacheSize && sim_params.lcP.maxCacheSize);
  assert (!(sim_params.lcP.minCacheSize & (sim_params.lcP.minCacheSize - 1)));
  assert (!(sim_params.lcP.maxCacheSize & (sim_params.lcP.maxCacheSize - 1)));

  max_min_ratio = sim_params.lcP.maxCacheSize / sim_params.lcP.minCacheSize;

  if (max_min_ratio > 1)
  {
    cache_count = log(max_min_ratio) / log(2) + 1;
    assert(sim_params.lcP.minCacheWays == sim_params.lcP.maxCacheWays);
  }
  else
  {
    cache_count = 1;
  }

  if (sim_params.lcP.maxCacheWays)
  {
    max_min_ratio = sim_params.lcP.maxCacheWays / sim_params.lcP.minCacheWays;
  }
  else
  {
    max_min_ratio = 1;
  }

  if (max_min_ratio > 1)
  {
    way_count = log(max_min_ratio) / log(2) + 1;
    assert(sim_params.lcP.minCacheSize == sim_params.lcP.maxCacheSize);
  }
  else
  {
    way_count = 1;
  }

  /* Allocate cache size array */
  cache_sizes = (ub8 *)malloc(sizeof(ub8) * cache_count);
  assert(cache_sizes);

  for (ub1 i = 0; i < cache_count; i++) 
  {
    cache_sizes[i] = (sim_params.lcP.minCacheSize << i);

    assert(cache_sizes[i] >= sim_params.lcP.minCacheSize &&
        cache_sizes[i] <= sim_params.lcP.maxCacheSize);
  }

  /* Allocate cache ways array */
  cache_ways = (ub8 *)malloc(sizeof(ub8) * way_count);
  assert(cache_ways);

  for (ub1 i = 0; i < way_count; i++) 
  {
    if (sim_params.lcP.minCacheWays)
    {
      cache_ways[i] = (sim_params.lcP.minCacheWays << i);
    }
    else
    {
      cache_ways[i] = (1 << i);
    }

    if (sim_params.lcP.minCacheWays && sim_params.lcP.maxCacheWays)
    {
      assert(cache_ways[i] >= sim_params.lcP.minCacheWays &&
          cache_ways[i] <= sim_params.lcP.maxCacheWays);
    }
  }

  if (way_count > 1)
  {
    cache_set = sim_params.lcP.maxCacheSize / sim_params.lcP.maxCacheWays;
  }
  else
  {
    cache_set = 0;
  }

  /* Open statistics stream */
  openStatisticsStream(&sim_params, stats_stream);
  initStatistics(sm);

#if 0
  if (sim_params.lcP.dramSimEnable == FALSE)
  {
    dram_config_init(&(l3cache.dram_info));
  }
#endif

  cout << endl << "Running for " << cache_count << " iterations" << endl;

  for (ub1 c_cache = 0; c_cache < cache_count; c_cache++)
  {
    for (ub1 c_way = 0; c_way < way_count; c_way++)
    {
      CLRSTRUCT(per_unit_miss); 
      CLRSTRUCT(acc_per_unit); 
      CLRSTRUCT(per_stream_hits); 
      CLRSTRUCT(per_stream_access); 
      CLRSTRUCT(l3cache.access); 
      CLRSTRUCT(l3cache.miss); 
      CLRSTRUCT(l3cache.itr_count);
      CLRSTRUCT(l3cache.cachecycle);
      CLRSTRUCT(shadow_tags.access); 
      CLRSTRUCT(shadow_tags.miss); 
      CLRSTRUCT(shadow_tags.itr_count);
      CLRSTRUCT(shadow_tags.cachecycle);

      for (ub1 rank = 0; rank < NUM_CHANS * NUM_RANKS; rank++)
      {
        for (ub1 bank = 0; bank < NUM_BANKS; bank++)
        {
          CLRSTRUCT(l3cache.per_stream_row[rank][bank].current_row); 
          CLRSTRUCT(l3cache.per_stream_row[rank][bank].current_access); 
          CLRSTRUCT(l3cache.per_stream_row[rank][bank].total_access); 
          CLRSTRUCT(l3cache.per_stream_row[rank][bank].total_interval); 
          CLRSTRUCT(l3cache.per_stream_row[rank][bank].max_access); 
          CLRSTRUCT(l3cache.per_stream_row[rank][bank].total_shared_access); 
          CLRSTRUCT(l3cache.per_stream_row[rank][bank].total_shared_interval); 
        }
      }

      set_cache_params(&params, &(sim_params.lcP), cache_sizes[c_cache], cache_ways[c_way], cache_set);

      /* Allocate per stream per way evict histogram */
      for (ub4 i = 0; i <= TST; i++)
      {
        per_way_evct[i] = (ub8 *)malloc(sizeof(ub8) * params.ways);
        assert(per_way_evct[i]);

        memset(per_way_evct[i], 0, sizeof(ub8) * params.ways);
      }

#define LOG_BASE_TWO(a) (log(a) / log(2))

      ub8 block_bits            = LOG_BASE_TWO(CACHE_L1_BSIZE);
      CACHE_IMSK(&l3cache)      = (cache_ways[c_way] == 0) ? 0 : (((ub8)(params.num_sets - 1))) << block_bits;
      CACHE_IMSK(&shadow_tags)  = (cache_ways[c_way] == 0) ? 0 : (((ub8)(params.num_sets - 1))) << block_bits;

#undef LOG_BASE_TWO

      resetStatistics();

      printf("Cache parameters : Size %ld Ways %d Sets %d\n", 
          cache_sizes[c_cache], params.ways, params.num_sets);

      /* Initialize components and record cycle times */
      cache_ctime         = cs_init(&l3cache, params, trc_file_name);
      cache_next_cycle    = cache_ctime;
      l3cache.shadow_tag  = FALSE; 

#define SHADOW_TAG_CTIME (250)

      /* Initialize shadow tags */
      shadow_tag_ctime        = cs_init(&shadow_tags, params, trc_file_name);
      shadow_tags.shadow_tag  = TRUE; 

      if (l3cache.dramsim_enable)
      {
        dram_ctime        = dram_init(&sim_params, params);
        dram_next_cycle   = dram_ctime;
        dram_done         = FALSE;
      }
      else
      {
        dram_done = TRUE;
      }

#define CH (4)

      /* Invoke all components simulators */
      do
      {
        if (l3cache.dramsim_enable)
        {
          current_cycle         = MINIMUM(cache_next_cycle, dram_next_cycle);
          cache_next_cycle      = cache_next_cycle - current_cycle;
          dram_next_cycle       = dram_next_cycle - current_cycle;
        }
        else
        {
          current_cycle         = cache_next_cycle;
          cache_next_cycle      = cache_next_cycle - current_cycle;
        }

        if (!cache_next_cycle)
        {
          cache_done        = cache_cycle(&l3cache, &shadow_tags, l3cache.cachecycle);
          cache_next_cycle  = cache_ctime;

          l3cache.cachecycle          += 1;
          l3cache.cachecycle_interval += 1;
        }

        if (l3cache.dramsim_enable && !dram_next_cycle)
        {
          dram_done = dram_cycle(&l3cache, l3cache.dramcycle);
          dram_next_cycle = dram_ctime;
          l3cache.dramcycle += 1;
        }

        total_cycle += 1;

        if (!inscnt)
        {
          sm->dumpNames("CacheSize", CH, stats_stream);
        }

        inscnt++;

        (*all_access)++;

        if (!(inscnt % 10000000))
        {
          sm->dumpValues(cache_sizes[c_cache], 0, CH, stats_stream);
          periodicReset();
          dumpDRAMStats(&l3cache);
          fflush(stdout);
        }
      }while(!cache_done || !dram_done);

      cachesim_dump_per_bank_stats(&l3cache, &shadow_tags, TRUE);

      /* Finalize component simulators */
      cs_fini(&l3cache);

      if (l3cache.dramsim_enable)
      {
        dram_fini();
      }

      if (cache_count > 1 || (cache_count == 1 && way_count == 1))
      {
        sm->dumpValues(cache_sizes[c_cache], 0, CH, stats_stream);
      }
      else
      {
        sm->dumpValues(cache_ways[c_way], 0, CH, stats_stream);
      }

#undef CH

      for (ub4 i = 0; i <= TST; i++)
      {
        /* Deallocate per-way evict array */
        free(per_way_evct[i]);
      }
    }
  }

  /* Close statistics stream */
  closeStatisticsStream(stats_stream);
  dumpOverallStats(&l3cache, total_cycle, l3cache.cachecycle, l3cache.dramcycle);

  printf("\n");

#define STREAM_ROW(c, r, b, s)        ((c)->per_stream_row[r][b].current_row[s])
#define STREAM_ACCESS(c, r, b, s)     ((c)->per_stream_row[r][b].current_access[s])
#define STREAM_IACCESS(c, r, b, s)    ((c)->per_stream_row[r][b].interval_access[s])
#define STREAM_TOTAL(c, r, b, s)      ((c)->per_stream_row[r][b].total_access[s])
#define STREAM_INTERVAL(c, r, b, s)   ((c)->per_stream_row[r][b].total_interval[s])
#define BGN_IVL(c, r, b, s)           (STREAM_IACCESS(c, r, b, s).begin())
#define END_IVL(c, r, b, s)           (STREAM_IACCESS(c, r, b, s).end())
#define STREAM_S_IACCESS(c, r, b, s)  ((c)->per_stream_row[r][b].interval_shared_access[s])
#define STREAM_S_TOTAL(c, r, b, s)    ((c)->per_stream_row[r][b].total_shared_access[s])
#define STREAM_S_INTERVAL(c, r, b, s) ((c)->per_stream_row[r][b].total_shared_interval[s])
#define S_BGN_IVL(c, r, b, s)         (STREAM_S_IACCESS(c, r, b, s).begin())
#define S_END_IVL(c, r, b, s)         (STREAM_S_IACCESS(c, r, b, s).end())

  map<ub8, ub8>::iterator map_itr;
  ub8 val;
  ub8 diff;
  ub8 var;
  ub8 mean;
  ub8 total_access[TST + 1];
  ub8 total_interval[TST + 1];
  ub8 total_shared_access[TST + 1];
  ub8 total_shared_interval[TST + 1];

  memset(total_access, 0, sizeof(total_access));
  memset(total_interval, 0, sizeof(total_interval));
  memset(total_shared_access, 0, sizeof(total_shared_access));
  memset(total_shared_interval, 0, sizeof(total_shared_interval));

  /* Iterate through all ranks */
  for (ub4 rank = 0; rank < NUM_RANKS * NUM_CHANS; rank++)
  {
    /* Iterate through all banks */
    for (ub4 bank = 0; bank < NUM_BANKS; bank++)
    {
      /* Iterate through all streams */
      for (ub4 s = NN + 1; s <= TST; s++)
      {
        if (STREAM_TOTAL(&l3cache, rank, bank, s))
        {
          total_access[s]   += STREAM_TOTAL(&l3cache, rank, bank, s);
          total_interval[s] += STREAM_INTERVAL(&l3cache, rank, bank, s);
        }

        if (STREAM_S_TOTAL(&l3cache, rank, bank, s))
        {
          total_shared_access[s]   += STREAM_S_TOTAL(&l3cache, rank, bank, s);
          total_shared_interval[s] += STREAM_S_INTERVAL(&l3cache, rank, bank, s);
        }
      }
    }
  }

  /* Iterate through all streams */
  for (ub4 s = NN + 1; s <= TST; s++)
  {
    val   = 0;
    diff  = 0;
    var   = 0;
    mean  = 0;

    if (total_access[s])
    {
      assert(total_interval[s]);

      mean = total_access[s] / total_interval[s];

      /* Iterate through all ranks */
      for (ub4 rank = 0; rank < NUM_RANKS * NUM_CHANS; rank++)
      {
        /* Iterate through all banks */
        for (ub4 bank = 0; bank < NUM_BANKS; bank++)
        {
          /* Get variance */
          for (map_itr = BGN_IVL(&l3cache, rank, bank, s); map_itr != END_IVL(&l3cache, rank, bank, s); map_itr++)
          {
            val = map_itr->second;

            if (val > mean)
            {
              diff = pow((val - mean), 2);
            }
            else
            {
              diff = pow((mean - val), 2);
            }

            var += diff;
          }
        }
      }

      var = var / total_interval[s];

      printf("%3s; %8ld; %8ld; %2ld; %6ld\n", stream_names[s], 
          total_access[s], total_interval[s], mean, var);
    }
  }
  
  printf("\n");

  for (ub4 s = NN + 1; s <= TST; s++)
  {
    val   = 0;
    diff  = 0;
    var   = 0;
    mean  = 0;

    /* Iterate through all streams */
    if (total_shared_access[s])
    {
      assert(total_shared_interval[s]);

      mean = total_shared_access[s] / total_shared_interval[s];

      /* Iterate through all ranks */
      for (ub4 rank = 0; rank < NUM_RANKS * NUM_CHANS; rank++)
      {
        /* Iterate through all banks */
        for (ub4 bank = 0; bank < NUM_BANKS; bank++)
        {
          /* Get variance */
          for (map_itr = S_BGN_IVL(&l3cache, rank, bank, s); 
              map_itr != S_END_IVL(&l3cache, rank, bank, s); map_itr++)
          {
            val = map_itr->second;

            if (val > mean)
            {
              diff = pow((val - mean), 2);
            }
            else
            {
              diff = pow((mean - val), 2);
            }

            var += diff;
          }
        }
      }

      var = var / total_shared_interval[s];

      printf("%3s; %8ld; %8ld; %2ld; %6ld\n", stream_names[s], 
          total_shared_access[s], total_shared_interval[s], mean, var);
    }
  }

#undef STREAM_ROW
#undef STREAM_ACCESS
#undef STREAM_IACCESS
#undef STREAM_TOTAL
#undef STREAM_INTERVAL
#undef BGN_IVL
#undef END_IVL
#undef STREAM_S_IACCESS
#undef STREAM_S_TOTAL
#undef STREAM_S_INTERVAL
#undef S_BGN_IVL
#undef S_END_IVL

  return 0;
}
      
#undef SHADOW_TAG_CTIME
#undef INTERVAL_SIZE
#undef CYCLE_INTERVAL
#undef ROW_SET_SIZE
#undef USE_INTER_STREAM_CALLBACK
#undef MAX_CORES
#undef IS_SPILL_ALLOCATED
#undef ACCESS_BYPASS
#undef ST
#undef SSTU
#undef BLCKALIGN
#undef MAX_RRPV
#undef MAX_MSHR
#undef TQ_SIZE
#undef ECTR_WIDTH
#undef ECTR_MIN_VAL
#undef ECTR_MAX_VAL
#undef ECTR_MID_VAL
#undef TOTAL_BANKS
