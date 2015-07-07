#include "cache-sim.h"
#include <unistd.h>
#include "inter-stream-reuse.h"

#define USE_INTER_STREAM_CALLBACK (FALSE)
#define MAX_CORES                 (128)
#define MAX_RRPV                  (3)
#define ST(i)                     ((i)->stream)
#define SSTU(i)                   ((i)->sap_stream == sapsimple_stream_u)
#define IS_SPILL_ALLOCATED(i)     (ST(i) == CS || ST(i) == BS || ST(i) == PS)
#define ACCESS_BYPASS(i)          (ST(i) == PS && SSTU(i))
#define MAX_REUSE                 (64)
#define INTERVAL                  (256 * 1024)

InterStreamReuse *reuse_ct_cbk = NULL; /* Callback for CT reuse */
InterStreamReuse *reuse_cc_cbk = NULL; /* Callback for CC reuse */
InterStreamReuse *reuse_bt_cbk = NULL; /* Callback for BT reuse */
InterStreamReuse *reuse_bb_cbk = NULL; /* Callback for BB reuse */
InterStreamReuse *reuse_zt_cbk = NULL; /* Callback for ZT reuse */
InterStreamReuse *reuse_zz_cbk = NULL; /* Callback for ZZ reuse */
InterStreamReuse *reuse_cb_cbk = NULL; /* Callback for CB reuse */
InterStreamReuse *reuse_tt_cbk = NULL; /* Callback for TT reuse */
InterStreamReuse *reuse_pp_cbk = NULL; /* Callback for TT reuse */

#define BLCKALIGN(addr) ((addr >> CACHE_BLCKOFFSET) << CACHE_BLCKOFFSET)

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
  NumericStatistic <ub8> *t_xhit;      /* Text intet stream hit */
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

  ub8 *per_way_evct[TST + 1];          /* Eviction seen by each way for each stream */
  ub8 reuse_hist[TST + 1][MAX_REUSE + 1]; /* Reuse distance histogram */
  
  ub8 invalid_hits;

void dump_hist_for_stream(ub1 stream)
{
  /* Dump Reuse stats in file */
  gzofstream   out_stream;
  char         tracefile_name[100];
  extern  sb1 *stream_names[TST + 1];  

  /* Dump Reuse stats in file */
  assert(strlen("CHR--stats.trace.csv.gz") + 1 < 100);
  sprintf(tracefile_name, "CHR-%s-stats.trace.csv.gz", stream_names[stream]);
  out_stream.open(tracefile_name, ios::out | ios::binary);

  if (!out_stream.is_open())
  {
    printf("Error opening output stream\n");
    exit(EXIT_FAILURE);
  }
  
  printf("Dumping reuse distance histogram %s \n", tracefile_name);

  out_stream << "Reuse; Blocks" << endl;

  for (ub8 i = 0; i <= MAX_REUSE; i++)
  {
    out_stream << std::dec << i << ";" << reuse_hist[stream][i] << endl;
  }

  out_stream.close();
}

cache_access_status cachesim_incl_cache( cachesim_cache *cache, ub8 addr, 
  ub8 rdis, ub1 policy, ub1 strm, memory_trace *info)
{
  struct    cache_block_t *block;
  struct    cache_block_t  vctm_block;
  enum      cache_block_state_t vctm_state;
  long long int vctm_tag;
  sb8       way;
  ub8       indx;
  sb4       vctm_stream;

  cache_access_status ret;

  assert(cache);

  indx        = ADDR_NDX(cache, addr);
  addr        = BLCKALIGN(addr); 
  ret.way     = -1;
  ret.fate    = CACHE_ACCESS_UNK;
  ret.stream  = NN;
  ret.dirty   = 0;
  
  if (strm == NN || strm > TST + MAX_CORES - 1)
  {
    cout << "Illegal stream " << (int)strm << endl;
  }

  assert(strm != NN && strm <= TST + MAX_CORES - 1);

  block = cache_find_block(cache->cache, indx, addr, info);

  /* If access is a miss */
  if (!block)
  {
    assert(info->prefetch == FALSE);

    /* If access is not bypassed fill the block */
#if 0
    if (info)
#endif
    if (info && (info->spill == FALSE || (info->spill == TRUE && IS_SPILL_ALLOCATED(info))))
    {
      way = cache_replace_block(cache->cache, indx, info);
      assert(way != -1);

      vctm_block = cache_get_block(cache->cache, indx, way, &vctm_tag, &vctm_state, 
          &vctm_stream);

      assert(vctm_stream >= 0 && vctm_stream <= TST + MAX_CORES - 1);
      
      ret.tag       = vctm_block.tag;
      ret.vtl_addr  = vctm_block.vtl_addr;
      ret.way       = way;
      ret.stream    = vctm_stream;
      ret.fate      = CACHE_ACCESS_MISS;
      ret.dirty     = vctm_block.dirty;
      ret.epoch     = vctm_block.epoch;
      ret.access    = vctm_block.access;
      ret.last_rrpv = vctm_block.last_rrpv;
      ret.fill_rrpv = vctm_block.fill_rrpv;

      /* If current block is valid */
      if (vctm_state != cache_block_invalid)
      {
        ret.fate  = CACHE_ACCESS_RPLC;
        cache_set_block(cache->cache, indx, way, vctm_tag, cache_block_invalid, 
            vctm_stream, info);

        /* Update per-stream per-way eviction */
        per_way_evct[vctm_stream][way] += 1;
      }
      else
      {
        assert(vctm_stream == NN);
      }

      cache_fill_block(cache->cache, indx, way, addr, cache_block_exclusive,
          strm, info);
      
      ub8 index;         /* Histogram bucket index */
      ub1 stream_indx;   /* Stream number */

      /* Update reuse count histogram */
      stream_indx = (ret.stream >= PS) ? PS : ret.stream;
      index       = (vctm_block.access < MAX_REUSE) ? vctm_block.access : MAX_REUSE;

      reuse_hist[stream_indx][index] += 1;

      return ret;
    } 
    else
    {
      ret.fate  = CACHE_ACCESS_MISS;
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
    
    /**/
    if (info->fill == TRUE || info->stream == BS)
    {
      cache_access_block(cache->cache, indx, block->way, strm, info);
    }
    else
    {
      /* Obtain old block state */
      cache_get_block(cache->cache, indx, block->way, &vctm_tag, &vctm_state, 
        &vctm_stream);

      assert(vctm_state != cache_block_invalid);
      
      /* Set new stream */
      cache_set_block(cache->cache, indx, block->way, vctm_tag, vctm_state, 
        strm, info);
    }

    return ret;
  }
  
  return ret;
}

/* Given cache capacity and associativity obtain other cache parameters */
void set_cache_params(struct cache_params *params, LChParameters *lcP, 
  ub8 cache_size, ub8 cache_way, ub8 cache_set)
{
  params->policy        = lcP->policy;
  params->max_rrpv      = lcP->max_rrpv;
  params->spill_rrpv    = lcP->spill_rrpv;
  params->threshold     = lcP->threshold;
  params->ways          = cache_way;
  params->stream        = lcP->stream;
  params->streams       = lcP->streams;
  params->bs_epoch      = lcP->useBs;
  params->use_step      = lcP->useStep;

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

void closeStatisticsStream(gzofstream &out_stream)
{
  out_stream.close();
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
}

/* Simulation start */
int main(int argc, char **argv)
{
  ub8  rdis;
  ub8  inscnt;
  ub8  address;
  ub8  per_unit_miss[TST];        /* Per unit misses */
  ub8  per_stream_hits[TST + 1];  /* Per stream hits */
  ub8  per_stream_access[TST + 1];/* Per stream access */
  ub8  acc_per_unit[TST];         /* Per unit access */
  ub8 *per_set_evct;              /* Eviction seen by each set */
  ub8 *per_set_access;            /* Accesses seen by each set */
  ub8 **per_rrpv_evct;            /* Replacements seen by each rrpv for each stream */
  ub8 **per_rrpv_hit;             /* Hits seen by each rrpv for each stream */
  ub4  cache_count;               /* # cache to simulate */
  ub4  way_count;                 /* Associativity */
  ub4  max_min_ratio;
  ub8 *cache_sizes;
  ub8 *cache_ways;
  sb1 *trc_file_name;
  sb1 *config_file_name;
  sb4  opt;
  ub8  cache_set;
  ub8  indx;

  gzFile trc_file;
  gzofstream stats_stream;

  /* Cache configuration parser */
  ConfigLoader  *cfg_loader;
  SimParameters  sim_params;

  StatisticsManager *sm;

  struct cache_params params;

  memory_trace          info;       /* Generic structure used for requests between units */
  memory_trace_in_file  trace_info;       /* Generic structure used for requests between units */
  cachesim_cache        l3cache;

  /* Verify bitness */
  assert(sizeof(ub8) == 8 && sizeof(ub4) == 4);

  rdis              = 0;
  inscnt            = 0;
  trc_file_name     = NULL;
  config_file_name  = NULL;
  invalid_hits      = 0;
  
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

  /* Add statistics counter */
  sm = &StatisticsManager::instance(); 

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

  cout << "Running for " << cache_count << " iterations\n";

  /* Instantiate reuse callback */
  if (USE_INTER_STREAM_CALLBACK)
  {
    reuse_ct_cbk = new InterStreamReuse(CS, TS, sim_params.lcP.useVa);
    reuse_cc_cbk = new InterStreamReuse(CS, CS, sim_params.lcP.useVa);
    reuse_bt_cbk = new InterStreamReuse(BS, TS, sim_params.lcP.useVa);
    reuse_bb_cbk = new InterStreamReuse(BS, BS, sim_params.lcP.useVa);
    reuse_zt_cbk = new InterStreamReuse(ZS, TS, sim_params.lcP.useVa);
    reuse_zz_cbk = new InterStreamReuse(ZS, ZS, sim_params.lcP.useVa);
    reuse_cb_cbk = new InterStreamReuse(CS, BS, sim_params.lcP.useVa);
    reuse_tt_cbk = new InterStreamReuse(TS, TS, sim_params.lcP.useVa);
    reuse_pp_cbk = new InterStreamReuse(PS, PS, sim_params.lcP.useVa);
  }
  
  for (ub1 c_cache = 0; c_cache < cache_count; c_cache++)
  {
    for (ub1 c_way = 0; c_way < way_count; c_way++)
    {
      CLRSTRUCT(l3cache); 
      CLRSTRUCT(per_unit_miss); 
      CLRSTRUCT(acc_per_unit); 
      CLRSTRUCT(per_stream_hits); 
      CLRSTRUCT(per_stream_access); 

      set_cache_params(&params, &(sim_params.lcP), cache_sizes[c_cache], cache_ways[c_way], cache_set);

      /* Allocate per stream per way evict histogram */
      for (ub4 i = 0; i <= TST; i++)
      {
        per_way_evct[i] = (ub8 *)malloc(sizeof(ub8) * params.ways);
        assert(per_way_evct[i]);

        memset(per_way_evct[i], 0, sizeof(ub8) * params.ways);
      }

#define LOG_BASE_TWO(a) (log(a) / log(2))

      ub8 block_bits       = LOG_BASE_TWO(CACHE_L1_BSIZE);
      CACHE_IMSK(&l3cache) = (cache_ways[c_way] == 0) ? 0 : (((ub8)(params.num_sets - 1))) << block_bits;

#undef LOG_BASE_TWO

      resetStatistics();

      printf("Cache parameters : Size %d Ways %d Sets %d\n", 
          cache_sizes[c_cache], params.ways, params.num_sets);

      per_set_evct = new ub8[params.num_sets];
      assert(per_set_evct);

      per_set_access = new ub8[params.num_sets];
      assert(per_set_access);

      per_rrpv_evct = new ub8*[TST + 1];
      assert(per_rrpv_evct);

      for (ub4 i = 0; i <= TST; i++)
      {
        per_rrpv_evct[i] = new ub8[MAX_RRPV + 1];
        assert(per_rrpv_evct[i]);
      }

      per_rrpv_hit = new ub8*[TST + 1];
      assert(per_rrpv_hit);

      for (ub4 i = 0; i <= TST; i++)
      {
        per_rrpv_hit[i] = new ub8[MAX_RRPV + 1];
        assert(per_rrpv_hit[i]);
      }

      /* Initialize policy specific part of cache */
      l3cache.cache = cache_init(&params);

      /* Open trace file stream */
      trc_file = gzopen(trc_file_name, "rb9");
      assert(trc_file);

      /* Start callback */
      if (reuse_ct_cbk)
      {
        reuse_ct_cbk->StartCbk();
      }

      if (reuse_cc_cbk)
      {
        reuse_cc_cbk->StartCbk();
      }

      if (reuse_bt_cbk)
      {
        reuse_bt_cbk->StartCbk();
      }

      if (reuse_bb_cbk)
      {
        reuse_bb_cbk->StartCbk();
      }

      if (reuse_zt_cbk)
      {
        reuse_zt_cbk->StartCbk();
      }

      if (reuse_zz_cbk)
      {
        reuse_zz_cbk->StartCbk();
      }

      if (reuse_cb_cbk)
      {
        reuse_cb_cbk->StartCbk();
      }

      if (reuse_tt_cbk)
      {
        reuse_tt_cbk->StartCbk();
      }

      if (reuse_pp_cbk)
      {
        reuse_pp_cbk->StartCbk();
      }

      /* Read entire trace file and run the simulation */
      while (!gzeof(trc_file))
      {
        gzread(trc_file, (char *)&info, sizeof(memory_trace));
#if 0
        gzread(trc_file, (char *)&trace_info, sizeof(memory_trace_in_file));
#endif

        if (!gzeof(trc_file))
        {
#if 0
          info.address  = trace_info.address;
          info.stream   = trace_info.stream;
          info.fill     = trace_info.fill;
          info.spill    = trace_info.spill;
#endif
          address = info.address;

          /* NN is used for all streams */
          if ((params.stream == NN || ((params.stream == OS) && 
                  (info.stream == XS || info.stream == DS ||
                   info.stream == BS || info.stream == NS ||
                   info.stream == HS)) || params.stream == info.stream) && 
              !ACCESS_BYPASS(&info))
          {
            /* Make a cache access */
#if 0
            cache_access_status ret = 
              cachesim_incl_cache(&l3cache, address, rdis, params.policy,
                  (info.stream > (TST)) ? TST : info.stream, &info);
#endif
            indx = ADDR_NDX(&l3cache, address);

            /* Increment per-set access count */
            per_set_access[indx]++;

            /* Issue callback for reuse */
            if (reuse_ct_cbk)
            {
              reuse_ct_cbk->CacheAccessBeginCbk(info, indx, per_set_access[indx], per_set_evct[indx]);
            }

            if (reuse_cc_cbk)
            {
              reuse_cc_cbk->CacheAccessBeginCbk(info, indx, per_set_access[indx], per_set_evct[indx]);
            }

            if (reuse_bt_cbk)
            {
              reuse_bt_cbk->CacheAccessBeginCbk(info, indx, per_set_access[indx], per_set_evct[indx]);
            }

            if (reuse_bb_cbk)
            {
              reuse_bb_cbk->CacheAccessBeginCbk(info, indx, per_set_access[indx], per_set_evct[indx]);
            }

            if (reuse_zt_cbk)
            {
              reuse_zt_cbk->CacheAccessBeginCbk(info, indx, per_set_access[indx], per_set_evct[indx]);
            }

            if (reuse_zz_cbk)
            {
              reuse_zz_cbk->CacheAccessBeginCbk(info, indx, per_set_access[indx], per_set_evct[indx]);
            }

            if (reuse_cb_cbk)
            {
              reuse_cb_cbk->CacheAccessBeginCbk(info, indx, per_set_access[indx], per_set_evct[indx]);
            }

            if (reuse_tt_cbk)
            {
              reuse_tt_cbk->CacheAccessBeginCbk(info, indx, per_set_access[indx], per_set_evct[indx]);
            }

            if (reuse_pp_cbk)
            {
              reuse_pp_cbk->CacheAccessBeginCbk(info, indx, per_set_access[indx], per_set_evct[indx]);
            }

            cache_access_status ret = 
              cachesim_incl_cache(&l3cache, address, rdis, params.policy,
                  info.stream, &info);

            assert(ret.fate != CACHE_ACCESS_UNK);
            
            per_stream_access[info.stream]++;

            /* Update statistics only for fill */
            if (info.fill == TRUE)
            {
              if (ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
              {
                if (info.stream >= PS && info.stream <= PS + MAX_CORES - 1)
                {
                  (*p_rmiss)++;

                }
                else
                {
                  switch (info.stream)
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

                    default:
                      cout << "Illegal stream " << (int)(info.stream) << " for address ";
                      cout << hex << info.address << " type : ";
                      cout << dec << (int)info.fill << " spill: " << dec << (int)info.spill;
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
                if (info.stream == ret.stream &&
                    (ret.stream == CS || ret.stream == ZS || ret.stream == TS))
                {
                  if (ret.epoch != 3)
                  {
                    updateEpochHitCountersForRead(ret);
                  }
                }
              }

              if (info.stream == CS || info.stream == ZS || info.stream == TS)
              {
                if (ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
                {
                  updateEpochFillCountersForRead(info.stream, 0);
                }
                else
                {
                  if (ret.epoch != 3)
                  {
                    if (info.stream != ret.stream)
                    {
                      updateEpochFillCountersForRead(info.stream, 0);
                    }
                    else
                    {
                      updateEpochFillCountersForRead(info.stream, ret.epoch + 1);
                    }
                  }
                }
              }

              if (info.stream >= PS && info.stream <= PS + MAX_CORES - 1)
              {
                (*p_raccess)++;
              }
              else
              {
                switch (info.stream)
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

                  default:
                    cout << "Illegal stream " << (int)info.stream << " for address ";
                    cout << hex << info.address << " type : ";
                    cout << dec << (int)info.fill << " spill: " << dec << (int)info.spill;
                    assert(0);
                }
              }
            }

            if (ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
            {
              if (info.stream >= PS && info.stream <= PS + MAX_CORES - 1)
              {
                (*p_miss)++;

                if (ret.stream != info.stream)
                  (*p_blocks)++;
              }
              else
              {
                switch (info.stream)
                {
                  case IS:
                    (*i_miss)++;

                    if (ret.stream != info.stream)
                      (*i_blocks)++;
                    break;

                  case CS:
                    (*c_miss)++;

                    if (ret.stream != info.stream)
                      (*c_blocks)++;

                    break;

                  case ZS:
                    (*z_miss)++;

                    if (ret.stream != info.stream)
                      (*z_blocks)++;
                    break;

                  case TS:
                    (*t_miss)++;

                    if (ret.stream != info.stream)
                      (*t_blocks)++;
                    break;

                  case NS:
                    (*n_miss)++;

                    if (ret.stream != info.stream)
                      (*n_blocks)++;
                    break;

                  case HS:
                    (*h_miss)++;

                    if (ret.stream != info.stream)
                      (*h_blocks)++;
                    break;

                  case BS:
                    (*b_miss)++;

                    if (ret.stream != info.stream)
                      (*b_blocks)++;
                    break;

                  case DS:
                    (*d_miss)++;

                    if (ret.stream != info.stream)
                      (*d_blocks)++;
                    break;

                  case XS:
                    (*x_miss)++;

                    if (ret.stream != info.stream)
                      (*x_blocks)++;
                    break;

                  default:
                    cout << "Illegal stream " << (int)(info.stream) << " for address ";
                    cout << hex << info.address << " type : ";
                    cout << dec << (int)info.fill << " spill: " << dec << (int)info.spill;
                    assert(0);
                }
              }

              if (ret.fate == CACHE_ACCESS_RPLC)
              { 
                per_set_evct[indx]++;

                assert(ret.last_rrpv <= MAX_RRPV);
                assert(ret.stream <= TST);
                
                per_rrpv_evct[ret.stream][ret.last_rrpv]++;

                /* Issue callback for reuse */
                if (reuse_ct_cbk)
                {
                  reuse_ct_cbk->CacheAccessReplaceCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
                }

                if (reuse_cc_cbk)
                {
                  reuse_cc_cbk->CacheAccessReplaceCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
                }

                if (reuse_bt_cbk)
                {
                  reuse_bt_cbk->CacheAccessReplaceCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
                }

                if (reuse_bb_cbk)
                {
                  reuse_bb_cbk->CacheAccessReplaceCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
                }

                if (reuse_zt_cbk)
                {
                  reuse_zt_cbk->CacheAccessReplaceCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
                }

                if (reuse_zz_cbk)
                {
                  reuse_zz_cbk->CacheAccessReplaceCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
                }

                if (reuse_cb_cbk)
                {
                  reuse_cb_cbk->CacheAccessReplaceCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
                }

                if (reuse_tt_cbk)
                {
                  reuse_tt_cbk->CacheAccessReplaceCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
                }

                if (reuse_pp_cbk)
                {
                  reuse_pp_cbk->CacheAccessReplaceCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
                }

                /* Current stream has a block allocated */
                assert(ret.stream != NN);

                /* Update C, Z, T epoch counters */
                if (ret.stream == CS || ret.stream == ZS || ret.stream == TS)
                {
                  updateEopchEvictCounters(ret);
                }

                if (ret.stream != info.stream)
                {
                  if (ret.stream >= PS && ret.stream <= PS + MAX_CORES - 1)
                  {
                    if (info.stream != PS)
                      (*p_xevct)++;

                    if (info.stream == CS)
                    {
                      (*p_xcevct)++;
                    }
                    else
                    {
                      if (info.stream == ZS)
                      {
                        (*p_xcevct)++;
                      }
                      {
                        if (info.stream == TS)
                        {
                          (*p_xtevct)++;
                        }
                        else
                        {
                          if (info.stream != PS)
                          {
                            (*p_xrevct)++;
                          }
                        }
                      }
                    }

                    if (p_blocks->getValue())
                      (*p_blocks)--;
                  }
                  else
                  {
                    switch(ret.stream)
                    {
                      case IS:
                        (*i_xevct)++;

                        if (info.stream == CS)
                        {
                          (*i_xcevct)++;
                        }
                        else
                        {
                          if (info.stream == ZS)
                          {
                            (*i_xzevct)++;
                          }
                          {
                            if (info.stream == TS)
                            {
                              (*i_xtevct)++;
                            }
                            else
                            {
                              if (info.stream == PS)
                              {
                                (*i_xpevct)++;
                              }
                              else
                              {
                                if (info.stream != IS)
                                {
                                  (*i_xrevct)++;
                                }
                              }
                            }
                          }
                        }

                        assert(i_blocks->getValue());
                        (*i_blocks)--;
                        break;

                      case CS:
                        (*c_xevct)++;

                        if (info.stream == ZS)
                        {
                          (*c_xzevct)++;
                        }
                        else
                        {
                          if (info.stream == TS)
                          {
                            (*c_xtevct)++;
                          }
                          else
                          {
                            if (info.stream == PS)
                            {
                              (*c_xpevct)++;
                            }
                            else
                            {
                              if (info.stream != CS)
                              {
                                (*c_xrevct)++;
                              }
                            }
                          }
                        }

                        assert(c_blocks->getValue(1) && info.stream != CS);
                        (*c_blocks)--;

                        break;

                      case ZS:
                        (*z_xevct)++;

                        if (info.stream == CS)
                        {
                          (*z_xcevct)++;
                        }
                        else
                        {
                          if (info.stream == TS)
                          {
                            (*z_xtevct)++;
                          }
                          else
                          {
                            if (info.stream == PS)
                            {
                              (*z_xpevct)++;
                            }
                            else
                            {
                              if (info.stream != ZS)
                              {
                                (*z_xrevct)++;
                              }
                            }
                          }
                        }

                        assert(z_blocks->getValue());
                        (*z_blocks)--;
                        break;

                      case TS:
                        (*t_xevct)++;

                        if (info.stream == CS)
                        {
                          (*t_xcevct)++;
                        }
                        else
                        {
                          if (info.stream == ZS)
                          {
                            (*t_xzevct)++;
                          }
                          else
                          {
                            if (info.stream == PS)
                            {
                              (*t_xpevct)++;
                            }
                            else
                            {
                              if (info.stream != TS)
                              {
                                (*t_xrevct)++;
                              }
                            }
                          }
                        }

                        /* At least one block must belong to TS */
                        assert(t_blocks->getValue());
                        (*t_blocks)--;
                        break;

                      case NS:
                        (*n_xevct)++;

                        if (info.stream == CS)
                        {
                          (*n_xcevct)++;
                        }
                        else
                        {
                          if (info.stream == ZS)
                          {
                            (*n_xzevct)++;
                          }
                          else
                          {
                            if (info.stream == TS)
                            {
                              (*n_xtevct)++;
                            }
                            else
                            {
                              if (info.stream == PS)
                              {
                                (*n_xpevct)++;
                              }
                              else
                              {
                                if (info.stream != NS)
                                {
                                  (*n_xrevct)++;
                                }
                              }
                            }
                          }
                        }

                        assert(n_blocks->getValue());
                        (*n_blocks)--;
                        break;

                      case HS:
                        (*h_xevct)++;

                        if (info.stream == CS)
                        {
                          (*h_xcevct)++;
                        }
                        else
                        {
                          if (info.stream == ZS)
                          {
                            (*h_xzevct)++;
                          }
                          else
                          {
                            if (info.stream == TS)
                            {
                              (*h_xtevct)++;
                            }
                            else
                            {
                              if (info.stream == PS)
                              {
                                (*h_xpevct)++;
                              }
                              else
                              {
                                if (info.stream != HS)
                                {
                                  (*h_xrevct)++;
                                }
                              }
                            }
                          }
                        }

                        assert(h_blocks->getValue());
                        (*h_blocks)--;
                        break;

                      case BS:
                        (*b_xevct)++;

                        if (info.stream == CS)
                        {
                          (*b_xcevct)++;
                        }
                        else
                        {
                          if (info.stream == ZS)
                          {
                            (*b_xzevct)++;
                          }
                          else
                          {
                            if (info.stream == TS)
                            {
                              (*b_xtevct)++;
                            }
                            else
                            {
                              if (info.stream == PS)
                              {
                                (*b_xpevct)++;
                              }
                              else
                              {
                                if (info.stream != BS)
                                {
                                  (*b_xrevct)++;
                                }
                              }
                            }
                          }
                        }

                        assert(b_blocks->getValue());
                        (*b_blocks)--;
                        break;

                      case DS:
                        (*d_xevct)++;

                        if (info.stream == CS)
                        {
                          (*d_xcevct)++;
                        }
                        else
                        {
                          if (info.stream == ZS)
                          {
                            (*d_xzevct)++;
                          }
                          else
                          {
                            if (info.stream == TS)
                            {
                              (*d_xtevct)++;
                            }
                            else
                            {
                              if (info.stream == PS)
                              {
                                (*d_xpevct)++;
                              }
                              else
                              {
                                if (info.stream != DS)
                                {
                                  (*d_xrevct)++;
                                }
                              }
                            }
                          }
                        }

                        assert(d_blocks->getValue());
                        (*d_blocks)--;

                        break;

                      case XS:
                        (*x_xevct)++;

                        if (info.stream == CS)
                        {
                          (*x_xcevct)++;
                        }
                        else
                        {
                          if (info.stream == ZS)
                          {
                            (*x_xzevct)++;
                          }
                          else
                          {
                            if (info.stream == TS)
                            {
                              (*x_xtevct)++;
                            }
                            else
                            {
                              if (info.stream == PS)
                              {
                                (*x_xpevct)++;
                              }
                              else
                              {
                                if (info.stream != XS)
                                {
                                  (*x_xrevct)++;
                                }
                              }
                            }
                          }
                        }

                        assert(x_blocks->getValue());
                        (*x_blocks)--;
                        break;

                      default:
                        cout << "Illegal stream " << (int)(info.stream) << " for address ";
                        cout << hex << info.address << " type : ";
                        cout << dec << (int)info.fill << " spill: " << dec << (int)info.spill;
                        assert(0);
                    }
                  }
                }
                else
                {
                  if (info.stream >= PS && info.stream <= PS + MAX_CORES - 1)
                  {
                    (*p_sevct)++;
                  }
                  else
                  {
                    switch(info.stream)
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

                      default:
                        cout << "Illegal stream " << (int)(info.stream) << " for address ";
                        cout << hex << info.address << " type : ";
                        cout << dec << (int)info.fill << " spill: " << dec << (int)info.spill;
                        assert(0);
                    }
                  }
                }

                if (ret.access == 0)
                {
                  if (ret.stream >= PS && ret.stream <= PS + MAX_CORES - 1)
                  {
                    (*p_zevct)++;

                    if (info.stream != ret.stream)
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

                        if (info.stream != ret.stream)
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

                        if (info.stream != ret.stream)
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

                        if (info.stream != ret.stream)
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

                        if (info.stream != ret.stream)
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

                        if (info.stream != ret.stream)
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

                        if (info.stream != ret.stream)
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

                        if (info.stream != ret.stream)
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

                        if (info.stream != ret.stream)
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

                        if (info.stream != ret.stream)
                        {
                          (*x_xoevct)++;
                        }
                        else
                        {
                          (*x_soevct)++;
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

              per_rrpv_hit[ret.stream][ret.last_rrpv]++;
              per_stream_hits[ret.stream]++;

              /* Issue callback for reuse */
              if (reuse_ct_cbk)
              {
                reuse_ct_cbk->CacheAccessHitCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
              }

              if (reuse_cc_cbk)
              {
                reuse_cc_cbk->CacheAccessHitCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
              }

              if (reuse_bt_cbk)
              {
                reuse_bt_cbk->CacheAccessHitCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
              }

              if (reuse_bb_cbk)
              {
                reuse_bb_cbk->CacheAccessHitCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
              }

              if (reuse_zt_cbk)
              {
                reuse_zt_cbk->CacheAccessHitCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
              }

              if (reuse_zz_cbk)
              {
                reuse_zz_cbk->CacheAccessHitCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
              }

              if (reuse_cb_cbk)
              {
                reuse_cb_cbk->CacheAccessHitCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
              }

              if (reuse_tt_cbk)
              {
                reuse_tt_cbk->CacheAccessHitCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
              }

              if (reuse_pp_cbk)
              {
                reuse_pp_cbk->CacheAccessHitCbk(info, ret, indx, per_set_access[indx], per_set_evct[indx]);
              }

              /* Update C, Z, T epoch counters */
              if (info.stream == ret.stream && 
                  (ret.stream == CS || ret.stream == ZS || ret.stream == TS))
              {
                updateEpochHitCounters(ret);
              }

              /* Update stats for incoming stream */
              if (info.stream != ret.stream)
              {
                if (info.stream >= PS && info.stream <= PS + MAX_CORES - 1)
                {
                  (*p_xhit)++;
                  (*p_blocks)++;
                }
                else
                {
                  switch (info.stream)
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

                      assert(info.spill == 0);

                      switch (ret.stream)
                      {
                        case CS:
                          (*t_cthit)++;

                          if (ret.dirty)
                          {
                            assert(info.spill == 0);
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
                            assert(info.spill == 0 && info.fill == 1);
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

                    default:
                      cout << "Illegal stream " << (int)info.stream << " for address ";
                      cout << hex << info.address << " type : ";
                      cout << dec << (int)info.fill << " spill: " << dec << (int)info.spill;
                      assert(0);
                  }
                }

                if (ret.stream >= PS && ret.stream <= PS + MAX_CORES - 1)
                {
                  assert(p_blocks->getValue());
                  (*p_blocks)--;
                }
                else
                {
                  switch (ret.stream)
                  {
                    case IS:
                      assert(i_blocks->getValue());
                      (*i_blocks)--;
                      break;

                    case CS:

                      assert(c_blocks->getValue(1) && info.stream != CS);
                      (*c_blocks)--;
#if 0
                      assert(cache_count_block(l3cache.cache, CS) == c_blocks->getValue());
#endif
                      break;

                    case ZS:
                      assert(z_blocks->getValue());
                      (*z_blocks)--;
                      break;

                    case TS:
                      assert(t_blocks->getValue());
                      (*t_blocks)--;
                      break;

                    case NS:
                      assert(n_blocks->getValue());
                      (*n_blocks)--;
                      break;

                    case HS:
                      assert(h_blocks->getValue());
                      (*h_blocks)--;
                      break;

                    case BS:
                      assert(b_blocks->getValue());
                      (*b_blocks)--;
                      break;

                    case DS:
                      assert(d_blocks->getValue());
                      (*d_blocks)--;
                      break;

                    case XS:
                      assert(x_blocks->getValue());
                      (*x_blocks)--;
                      break;

                    default:
                      cout << "Illegal stream " << (int)ret.stream << " for address ";
                      cout << hex << info.address << " type : ";
                      cout << dec << (int)info.fill << " spill: " << dec << (int)info.spill;
                      assert(0);
                  }
                }
              }
            }

            if (info.stream == CS || info.stream == ZS || info.stream == TS)
            {
              if (ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
              {
                updateEpochFillCounters(info.stream, 0);
              }
              else
              {
                if (ret.epoch != 3)
                {
                  if (info.stream != ret.stream)
                  {
                    updateEpochFillCounters(info.stream, 0);
                  }
                  else
                  {
                    updateEpochFillCounters(info.stream, ret.epoch + 1);
                  }
                }
              }
            }

            if (info.stream >= PS && info.stream <= PS + MAX_CORES - 1)
            {
              (*p_access)++;
            }
            else
            {
              switch (info.stream)
              {
                case IS:
                  (*i_access)++;
                  break;

                case CS:
                  (*c_access)++;

                  if (info.spill)
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

                  if (info.spill)
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

                default:
                  cout << "Illegal stream " << (int)info.stream << " for address ";
                  cout << hex << info.address << " type : ";
                  cout << dec << (int)info.fill << " spill: " << dec << (int)info.spill;
                  assert(0);
              }
            }

#define CH (4)

            if (!inscnt)
              sm->dumpNames("CacheSize", CH, stats_stream);

            inscnt++;
            (*all_access)++;

            if (!(inscnt % 10000))
            {
#if 0
              cout << ".";
              std::cout.flush();
#endif

              sm->dumpValues(cache_sizes[c_cache], 0, CH, stats_stream);
              periodicReset();
              
              cout << setw(10) << "ACS: " << per_stream_access[CS] << setw(10) << " AZS: " << per_stream_access[ZS];
              cout << setw(10) << " ATS: " << per_stream_access[TS] << endl;
              cout << setw(10) << "HCS: " << per_stream_hits[CS] << setw(10) << " HZS: " << per_stream_hits[ZS];
              cout << setw(10) << " HTS: " << per_stream_hits[TS] << endl;

              CLRSTRUCT(per_stream_hits);
              CLRSTRUCT(per_stream_access);
            }
#undef CH
          }
        }
        else
        {
          cout << "\n Simulation done!\n";
        }
      }

      /* Start callback */
      if (reuse_ct_cbk)
      {
        reuse_ct_cbk->ExitCbk();
      }

      if (reuse_cc_cbk)
      {
        reuse_cc_cbk->ExitCbk();
      }

      if (reuse_bt_cbk)
      {
        reuse_bt_cbk->ExitCbk();
      }

      if (reuse_bb_cbk)
      {
        reuse_bb_cbk->ExitCbk();
      }

      if (reuse_zt_cbk)
      {
        reuse_zt_cbk->ExitCbk();
      }

      if (reuse_zz_cbk)
      {
        reuse_zz_cbk->ExitCbk();
      }

      if (reuse_cb_cbk)
      {
        reuse_cb_cbk->ExitCbk();
      }

      if (reuse_tt_cbk)
      {
        reuse_tt_cbk->ExitCbk();
      }

      if (reuse_pp_cbk)
      {
        reuse_pp_cbk->ExitCbk();
      }

      /* Free InterStreamReuse object */
      if (reuse_ct_cbk)
      {
        delete reuse_ct_cbk;
      }

      if (reuse_cc_cbk)
      {
        delete reuse_cc_cbk;
      }

      if (reuse_bt_cbk)
      {
        delete reuse_bt_cbk;
      }

      if (reuse_bb_cbk)
      {
        delete reuse_bb_cbk;
      }

      if (reuse_zt_cbk)
      {
        delete reuse_zt_cbk;
      }

      if (reuse_zz_cbk)
      {
        delete reuse_zz_cbk;
      }

      if (reuse_cb_cbk)
      {
        delete reuse_cb_cbk;
      }

      if (reuse_tt_cbk)
      {
        delete reuse_tt_cbk;
      }

      if (reuse_pp_cbk)
      {
        delete reuse_pp_cbk;
      }

      /* Close statistics stream */
      gzclose(trc_file);
#if 0
      /* Write stats to file */
      if (c_cache == 0)
      {
        if (cache_count > 1 || (cache_count == 1 && way_count == 1))
        {
          sm->dumpNames("CacheSize", 4, stats_stream);
        }
        else
        {
          sm->dumpNames("CacheWay", 4, stats_stream);
        }
      }
#endif

#define CH (4)

      if (cache_count > 1 || (cache_count == 1 && way_count == 1))
      {
        sm->dumpValues(cache_sizes[c_cache], 0, CH, stats_stream);
      }
      else
      {
        sm->dumpValues(cache_ways[c_way], 0, CH, stats_stream);
      }

#undef CH

      printf("Per-stream per-way eviction\n");

      for (ub4 i = 0; i <= TST; i++)
      {
        printf("Stream %d\n", i);

        for (ub4 j = 0; j < params.ways; j++)
        {
          printf("%d %lld\n", j, per_way_evct[i][j]);
        }
        printf("\n");

        /* Deallocate per-way evict array */
        free(per_way_evct[i]);
      }
      
      cache_free(l3cache.cache);
    }
  }

  /* Close statistics stream */
  closeStatisticsStream(stats_stream);
  
  printf("Eviction RRPV histogram\n");

  for (ub4 i = 0; i <= TST; i++)
  {
    printf("Stream %d\n", i);

    for (ub4 j = 0; j <= MAX_RRPV; j++)
    {
      printf("%d %lld\n", j, per_rrpv_evct[i][j]);
    }

    printf("\n");
  }

  printf("Hit RRPV histogram\n");

  for (ub4 i = 0; i <= TST; i++)
  {
    printf("Stream %d\n", i);

    for (ub4 j = 0; j <= MAX_RRPV; j++)
    {
      printf("%d %lld\n", j, per_rrpv_hit[i][j]);
    }

    printf("\n");
  }
  
  dump_hist_for_stream(BS);
  dump_hist_for_stream(TS);
  dump_hist_for_stream(ZS);
  dump_hist_for_stream(CS);
  dump_hist_for_stream(PS);

  return 0;
}

#undef USE_INTER_STREAM_CALLBACK
#undef MAX_CORES
#undef IS_SPILL_ALLOCATED
#undef ACCESS_BYPASS
#undef ST
#undef SSTU
#undef BLCKALIGN
#undef MAX_RRPV
#undef INTERVAL
