#include "cache-sim.h"
#include <unistd.h>
#include "inter-stream-reuse.h"
#include "DRAMSim.h"

#define USE_INTER_STREAM_CALLBACK (FALSE)
#define MAX_CORES                 (128)
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


char *dram_config_file_name = NULL;
ub8   pending_requests;
long long dram_size_max = 1LL << 39; /* 512GB maximum dram size. */
long long dram_size     = 1LL << 32; /* 4GB default dram size. */
ub8 read_stall;
ub8 write_stall;
ub8 mshr_stall;

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

void print_dram_stats(ub1 stream, dram_config *dram_info)
{
  ub8 total_rows;
  ub8 rows_access;
  ub8 min_access;
  ub8 max_access;
  ub8 i;
  ub8 j;
  ub8 k;
  ub8 l;
  ub8 avg_access;
  ub8 above_avg_access;
  ub8 below_avg_access;
  ub8 below_max_access;

  total_rows        = 0;
  rows_access       = 0;
  min_access        = 0xffffff;
  max_access        = 0;
  avg_access        = 0;
  above_avg_access  = 0;
  below_avg_access  = 0;
  below_max_access  = 0;

  dram_bank *bank;

  for (i = 0; i < NUM_CHANS; i++)
  {
    for (j = 0; j < NUM_RANKS; j++)  
    {
      for (k = 0; k < NUM_BANKS; k++)
      {
        bank = &(dram_info->bank[stream][i][j][k]);

        for (l = 0; l < NUM_ROWS; l++)
        {
          if (bank->rows[l])
          {
            total_rows  += 1;     
            if (bank->rows[l]< min_access)
            {
              min_access = bank->rows[l];
            }

            if (bank->rows[l] > max_access)
            {
              max_access = bank->rows[l];
            }
          }
          
          rows_access += bank->rows[l];
        }
      }
    }
  }
  
  avg_access = (ub8)(rows_access / total_rows);
  
  for (i = 0; i < NUM_CHANS; i++)
  {
    for (j = 0; j < NUM_RANKS; j++)  
    {
      for (k = 0; k < NUM_BANKS; k++)
      {
        bank = &(dram_info->bank[stream][i][j][k]);

        for (l = 0; l < NUM_ROWS; l++)
        {
          if (bank->rows[l])
          {
            if (bank->rows[l] > avg_access)
            {
              above_avg_access += 1;
            }

            if (bank->rows[l] < max_access / 2)
            {
              below_max_access += bank->rows[l];
            }

            if (bank->rows[l] < avg_access)
            {
              below_avg_access += bank->rows[l];
            }
          }
        }
      }
    }
  }

  printf(" ROW: %ld ACC: %ld BELOWAVG: %ld BELOWMAX: %ld AVG: %ld MIN: %ld MAX:%ld AboveAvg: %ld \n",
      total_rows, rows_access, below_avg_access, below_max_access, avg_access, 
      min_access, max_access, above_avg_access);
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
  physicalAddress     = address >> colLowBitWidth;
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

void exec_frfcfs(dram_config *dram_info, dram_bank *bank)
{
  dram_request *next_request;

  while (dram_info->trans_queue_entries)
  {
    dram_info->row_access++;

    next_request = NULL;
  
    /* Find a request with current open row id */
    if (bank->open_row_id != -1) 
    {
      assert(bank->state == DRAM_BANK_ACT);

      next_request = 
        (dram_request *)attila_map_lookup(dram_info->global_trans_queue, bank->open_row_id, ATTILA_MASTER_KEY);
      if (next_request)
      {
        attila_map_delete(dram_info->global_trans_queue, bank->open_row_id, next_request->address);     
        attila_map_delete(dram_info->trans_queue[next_request->stream], bank->open_row_id, 
            next_request->address);

        next_request->done = 1;

        (dram_info->trans_queue_entries)--;
        dram_info->row_hits++;
      }
    }

    /* Find next request in FCFS order */
    if (!next_request)
    {
      do
      {
        next_request = (dram_request *)cs_qdequeue(&(dram_info->trans_inorder_queue));  

        if (next_request && next_request->done)
        {
          free(next_request);
        }
      }while(next_request && next_request->done);

      if (next_request)
      {
        bank->open_row_id = get_row_id(next_request->address);  
        bank->state       = DRAM_BANK_ACT;

        attila_map_delete(dram_info->global_trans_queue, next_request->row_id, next_request->address);     
        attila_map_delete(dram_info->trans_queue[next_request->stream], next_request->row_id, 
            next_request->address);

        free(next_request);

        (dram_info->trans_queue_entries)--;
      }
    }
  } 

  while (cs_qsize(&(dram_info->trans_inorder_queue)))
  {
    next_request = (dram_request *)cs_qdequeue(&(dram_info->trans_inorder_queue));
    assert(next_request->done);
    assert(!attila_map_lookup(dram_info->global_trans_queue, next_request->row_id, next_request->address));
    assert(!attila_map_lookup(dram_info->trans_queue[next_request->stream], next_request->row_id, 
          next_request->address));
  }
}

void get_address_map(cachesim_cache *cache, ub1 stream, memory_trace *info, dram_config *dram_info)
{
  ub8 physicalAddress;
  ub8 tmpPhysicalAddress;
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

  dram_request *request;
  dram_bank    *bank;

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
#if 0
  tmpPhysicalAddress  = mmu_translate(cache, info);
#endif
  physicalAddress     = tmpPhysicalAddress >> byteOffsetWidth;
  colLowBitWidth      = log2(transactionSize) - byteOffsetWidth;
  colHighBitWidth     = colBitWidth - colLowBitWidth;
  physicalAddress     = physicalAddress >> colLowBitWidth; 

  //row:rank:bank:col:chan
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> channelBitWidth;
  tempB = physicalAddress << channelBitWidth;
  dram_info->newTransactionChan = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> colHighBitWidth;
  tempB = physicalAddress << colHighBitWidth;
  dram_info->newTransactionColumn = tempA ^ tempB;

  // bank id
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> bankBitWidth;
  tempB = physicalAddress << bankBitWidth;
  dram_info->newTransactionBank = tempA ^ tempB;

  /* XOR with lower LLC tag bits */
  tempA = tmpPhysicalAddress >> llcTagLowBitWidth;
  tempB = (tempA >> bankBitWidth) << bankBitWidth;
  dram_info->newTransactionBank = dram_info->newTransactionBank ^ (tempA ^ tempB);

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rankBitWidth;
  tempB = physicalAddress << rankBitWidth;
  dram_info->newTransactionRank = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rowBitWidth;
  tempB = physicalAddress << rowBitWidth;
  dram_info->newTransactionRow = tempA ^ tempB;

  assert(dram_info->bank[stream][CHAN(dram_info)][RANK(dram_info)][BANK(dram_info)].rows);
  bank = &(dram_info->bank[stream][CHAN(dram_info)][RANK(dram_info)][BANK(dram_info)]);

  /* Emulate DRAM access */
  request = (dram_request *)calloc(1, sizeof(dram_request));
  assert(request);

  request->address = tmpPhysicalAddress;
  request->row_id  = get_row_id(tmpPhysicalAddress);
  request->stream  = info->stream;

  /* Insert request in per-stream transaction queue */
  attila_map_insert(dram_info->trans_queue[info->stream], request->row_id, request->address, (ub1 *)request);

  /* Insert request in global inorder queue */
  cs_qappend(&(dram_info->trans_inorder_queue), (ub1 *)request);

  /* Insert request in global transaction hash table */
  attila_map_insert(dram_info->global_trans_queue, request->row_id, request->address, (ub1 *)request);

  map<ub1, ub8>::iterator dram_bank_info_itr;
  map<ub8, ub8>::iterator dram_row_info_itr;
  map<ub8, ub8>::iterator block_itr;
  map<ub1, ub8> *bank_list;
  ub1            bank_id;
  bank_stats    *per_bank_stats;
  row_stats     *per_row_stats;

#define BANKS_CHANS   (NUM_RANKS * NUM_BANKS)
#define BANKS_RANKS   (NUM_BANKS)
#define CHAN_ID(d)    ((d)->newTransactionChan)
#define RANK_ID(d)    ((d)->newTransactionRank)
#define ROW_ID(d)     ((d)->newTransactionRow)
#define BANK_ID(d)    (CHAN_ID(d) * BANKS_CHANS + RANK_ID(d) * BANKS_RANKS + (d)->newTransactionBank)

  if (info->fill)
  {
    bank->rows[ROW(dram_info)] += 1;

    /* Update per-stream access count */
    dram_bank_info_itr = dram_info->bank_list.find(info->stream);

    if (dram_bank_info_itr == dram_info->bank_list.end())
    {
      bank_list = new map <ub1, ub8>;
      bank_id = BANK_ID(dram_info);

      per_bank_stats = new bank_stats;
      per_bank_stats->access = 1;

      per_row_stats = new row_stats;
      per_row_stats->access = 1;
      per_row_stats->blocks.insert(pair<ub8, ub8>(tmpPhysicalAddress, 1));
      per_bank_stats->rows.insert(pair<ub8, ub8>(ROW_ID(dram_info), (ub8)per_row_stats));

      bank_list->insert(pair<ub1, ub8>(bank_id, (ub8)per_bank_stats));
      dram_info->bank_list.insert(pair<int, ub8>(info->stream, (ub8)bank_list));
    }
    else
    {
      bank_list = (map <ub1, ub8> *)(dram_bank_info_itr->second);
      bank_id = BANK_ID(dram_info);
      dram_bank_info_itr = bank_list->find(bank_id);

      /* If bank not present add bank to the list */
      if (dram_bank_info_itr == bank_list->end())
      {
        per_bank_stats = new bank_stats;
        per_bank_stats->access = 1;

        per_row_stats = new row_stats;
        per_row_stats->access = 1;
        per_row_stats->blocks.insert(pair<ub8, ub8>(tmpPhysicalAddress, 1));

        per_bank_stats->rows.insert(pair<ub8, ub8>(ROW_ID(dram_info), (ub8)per_row_stats));

        bank_list->insert(pair<ub1, ub8>(bank_id, (ub8)per_bank_stats));
      }
      else
      {
        per_bank_stats = (bank_stats *)(dram_bank_info_itr->second);
        per_bank_stats->access += 1;
        dram_row_info_itr = per_bank_stats->rows.find(ROW_ID(dram_info));

        if (dram_row_info_itr == per_bank_stats->rows.end())
        {
          per_row_stats = new row_stats;
          per_row_stats->access = 1;
          per_row_stats->blocks.insert(pair<ub8, ub8>(tmpPhysicalAddress, 1));

          per_bank_stats->rows.insert(pair<ub8, ub8>(ROW_ID(dram_info), (ub8)per_row_stats));
        }
        else
        {
          per_row_stats = (row_stats *)dram_row_info_itr->second;
          assert(bank->rows[ROW_ID(dram_info)] > 1);
          per_row_stats->access += 1; 

          block_itr = per_row_stats->blocks.find(tmpPhysicalAddress);
          if (block_itr == per_row_stats->blocks.end())
          {
            per_row_stats->blocks.insert(pair<ub8, ub8>(tmpPhysicalAddress, 1));
          }
          else
          {
            block_itr->second += 1;
          }
        }
      }
    }
    
    /* Insert per bank info in global bank list */
    dram_bank_info_itr = dram_info->g_bank_list.find(BANK_ID(dram_info));
    
    if (dram_bank_info_itr == dram_info->g_bank_list.end())
    {
      per_bank_stats = new bank_stats;
      per_bank_stats->access = 1;
      dram_info->g_bank_list.insert(pair<ub1, ub8>(BANK_ID(dram_info), (ub8)per_bank_stats));

      per_row_stats = new row_stats;
      per_row_stats->access = 1;
      per_bank_stats->rows.insert(pair<ub8, ub8>(ROW_ID(dram_info), (ub8)per_row_stats));

      per_row_stats->blocks.insert(pair<ub8, ub8>(tmpPhysicalAddress, 1)); 
    }
    else
    {
      per_bank_stats = (bank_stats *)dram_bank_info_itr->second; 
      per_bank_stats->access += 1;

      dram_row_info_itr = per_bank_stats->rows.find(ROW_ID(dram_info));

      if (dram_row_info_itr == per_bank_stats->rows.end())
      {
        per_row_stats = new row_stats;
        per_row_stats->access = 1;
        per_bank_stats->rows.insert(pair<ub8, ub8>(ROW_ID(dram_info), (ub8)per_row_stats));

        per_row_stats->blocks.insert(pair<ub8, ub8>(tmpPhysicalAddress, 1));
      }
      else
      {
        per_row_stats = (row_stats *)(dram_row_info_itr->second);

        per_row_stats->access += 1;

        block_itr = per_row_stats->blocks.find(tmpPhysicalAddress);
        if (block_itr == per_row_stats->blocks.end())
        {
          per_row_stats->blocks.insert(pair<ub8, ub8>(tmpPhysicalAddress, 1));
        }
        else
        {
          block_itr->second += 1;
        }
      }
    }

    assert(bank->rows[ROW_ID(dram_info)] > 0);
  }

#undef BANKS_CHANS
#undef BANKS_RANKS
#undef CHAN_ID
#undef RANK_ID
#undef ROW_ID
#undef BANK_ID

  if (++(dram_info->trans_queue_entries) == dram_info->trans_queue_size)
  {
    exec_frfcfs(dram_info, bank);

#if 0
    /* Run the policy */
    while (!cs_qempty(dram_info->trans_inorder_queue))
    {
      if (dram_info->open_row_id >= 0)
      {
        next_request = attila_map_lookup(dram_info->trans_queue[TS], dram_info->open_row_id, ATTILA_MASTER_KEY);
        if (next_request)
        {
          /* Emulate DRAM */
          cs_qdelete(&(dram_info->trans_inorder_queue), next_request);
          attila_map_delete(&(dram_info->trans_queue[TS]), next_request->row_id, next_request->address);
          free(next_request);

          dram_info->trans_queue_entries--;
        }
      }
      else
      {
        /* Remove next FCFS request */
        next_request = cs_qdequeue(&(dram_info->trans_inorder_queue));
        if (next_request)
        {
          free(next_request);
          dram_info->trans_queue_entries--;

          dram_info->open_row_id = get_row_id(next_request->address);
        }
      }
    }

    assert(!dram_info->trans_queue_entries);
#endif
  }
}

#define CACHE_CTIME (.25 * 1000)

ub8 cs_init(cachesim_cache *cache, cache_params params, sb1 *trc_file_name)
{
  assert(cache);

  cs_qinit(&(cache->rdylist));
  cs_qinit(&(cache->plist));

  cache->total_mshr = MAX_MSHR;
  cache->free_mshr  = MAX_MSHR;

  /* Open trace file stream */
  cache->trc_file = gzopen(trc_file_name, "rb9");
  assert(cache->trc_file);

  /* Initialize policy specific part of cache */
  cache->cache = cache_init(&params);

  cache->dramsim_enable = params.dramsim_enable;
  cache->dramsim_trace  = params.dramsim_trace;
  
  /* Initialize page frame table to start address */
  (cache->i_page_table).next_cluster = 0;

  for (ub1 s = NN; s <= TST; s++)
  {
    (cache->i_page_table).stream_cluster[s] = 0;
  }

  return CACHE_CTIME;
}

#undef CACHE_CTIME

void cs_fini(cachesim_cache *cache)
{
  cache_free(cache->cache);

  /* Close statistics stream */
  gzclose(cache->trc_file);
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

#define LOG_BLOCK_SIZE  (6)
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

#undef LOG_BLOCK_SIZE
#undef LLC_BANK

  /* Initialize dram device */
  printf("Initializing DRAM controllers:\n");
  printf("\tNumber of DRAM controllers = %d\n", MC_COUNT);
  printf("\tDRAM controller transaction queue size = %d\n", trans_queue_size);
  printf("\tLLC Tag low bit width = %d\n\n", llcTagLowBitWidth);

  dram_tck = dramsim_init(simP->lcP.dramSimConfigFile, (unsigned long long) (dram_size >> 20),
      MC_COUNT, trans_queue_size, llcTagLowBitWidth);
   
  if (!strncmp(simP->lcP.dramPriorityStream, "TS", 2))
  {
    dramsim_set_priority_stream(TS);
  }
  else
  {
    if (!strncmp(simP->lcP.dramPriorityStream, "CS", 2))
    {
      dramsim_set_priority_stream(CS);
    }
    else
    {
      if (!strncmp(simP->lcP.dramPriorityStream, "ZS", 2))
      {
        dramsim_set_priority_stream(ZS);
      }
      else
      {
        if (!strncmp(simP->lcP.dramPriorityStream, "PS0", 3))
        {
          dramsim_set_priority_stream(PS);
        }
      }
    }
  }

  /* Calculate the tick for cpu and dram clocks */
  return dram_tck;
}

#undef MC_COUNT

void dram_fini()
{
  dramsim_done();
}

#define LOG_PG_SIZE         (12)
#define PG_SIZE             (1 << LOG_PG_SIZE)
#define LOG_PHY_ADDR_SIZE   (32)
#define MAX_PAGE            (1 << (LOG_PHY_ADDR_SIZE - LOG_PG_SIZE))
#define MAX_CLUSTER         ((1 << (LOG_PHY_ADDR_SIZE - LOG_PG_SIZE)) / PG_CLSTR_SIZE)
#define NEXT_CLUSTER(c)     (((c).next_cluster + 1) % MAX_CLUSTER)
#define NEXT_SCLUSTER(c, i) (((c).stream_cluster[(i)->stream] + 1) % MAX_CLUSTER)

static void cluster_page_allocate(page_cluster *cluster)
{
  ub1 free;

  free = cluster->free;

  /* First allocate one page to all streams. Then distribute rest of the pages 
   * to C, T, Z, P */
  
  for (ub1 s = NN + 1; s <= PS; s++)
  {
    cluster->stream_free[s] = 1;
    free--;
  }

  while (free)
  {
    if (free)
    {
      cluster->stream_free[CS]++;
      free--;
    }

    if (free)
    {
      cluster->stream_free[ZS]++;
      free--;
    }

    if (free)
    {
      cluster->stream_free[TS]++;
      free--;
    }

    if (free)
    {
      cluster->stream_free[PS]++;
      free--;
    }
  }

  for (ub1 s = NN + 1; s <= PS; s++)
  {
    assert(cluster->stream_free[s]);

    if (s == CS || s == TS || s == ZS || s == PS)
    {
      assert(cluster->stream_free[s] >= 2);
    }
  }
}

static page_cluster* get_cluster(cachesim_cache *cache, memory_trace *info)
{
  map <ub8, ub8>::iterator pg_itr;
  page_cluster  *cluster;
  ub8 cluster_id;

  /* Lookup inverted page table to assign new physical address */
#if 0
  pg_itr = cache->i_page_table.cluster.find(cache->i_page_table.next_cluster); 
#endif

  cluster_id = info->address >> (LOG_PG_SIZE + 5);
#if 0
  pg_itr = cache->i_page_table.cluster.find(cache->i_page_table.stream_cluster[info->stream]);
#endif
  pg_itr = cache->i_page_table.cluster.find(cluster_id);

  if (pg_itr == cache->i_page_table.cluster.end())
  {
    cluster = new page_cluster;
    assert(cluster);
#if 0
    cluster->id         = cache->i_page_table.stream_cluster[info->stream];
#endif
    cluster->id         = cluster_id;
    cluster->free       = PG_CLSTR_SIZE;
    cluster->allocated  = 0;

    for (ub1 i = 0; i < PG_CLSTR_SIZE; i++)
    {
      cluster->page_no[i] = 0;
      cluster->valid[i]   = FALSE;
    }

    for (ub1 i = NN; i <= TST; i++)
    {
      cluster->stream_access[i] = 0;
    }

    /* As per current distribution assign pages to each stream */
    cluster_page_allocate(cluster);

    ((cache->i_page_table).cluster).insert(pair<ub8, ub8>(cluster->id, (ub8)cluster));
  }
  else
  {
    cluster = (page_cluster *)(pg_itr->second);
    
    /* If cluster has no space allocate a new cluster */
#if 0
    if (!(cluster->free) || !(cluster->stream_free[info->stream]))
#endif
    if (!(cluster->free))
    {
      cache->i_page_table.next_cluster = NEXT_CLUSTER(cache->i_page_table);
      cache->i_page_table.stream_cluster[info->stream] = NEXT_SCLUSTER(cache->i_page_table, info);
#if 0
      cache->i_page_table.next_cluster = (random() % MAX_CLUSTER);
      cache->i_page_table.stream_cluster[info->stream] = (random() % MAX_CLUSTER);
#endif

      cluster = new page_cluster;
      assert(cluster);

#if 0
      cluster->id         = cache->i_page_table.stream_cluster[info->stream];
#endif
      cluster->id         = cache->i_page_table.next_cluster;
      cluster->free       = PG_CLSTR_SIZE;
      cluster->allocated  = 0;

      for (ub1 i = 0; i < PG_CLSTR_SIZE; i++)
      {
        cluster->page_no[i] = 0;
        cluster->valid[i]   = FALSE;
      }

      for (ub1 i = NN; i <= TST; i++)
      {
        cluster->stream_access[i] = 0;
      }

      /* As per current distribution assign pages to each stream */
      cluster_page_allocate(cluster);

      ((cache->i_page_table).cluster).insert(pair<ub8, ub8>(cluster->id, (ub8)cluster));
    }
  }
  
  assert(cluster);

  return cluster;
}

static page_entry* allocate_pte(cachesim_cache *cache, page_cluster *cluster, memory_trace *info)
{
  ub8 page_no;
  ub1 off;
  map <ub8, ub8>::iterator pg_itr;
  page_entry *pte;
  page_entry *old_pte;

  assert(cluster);
  assert(info);
  assert(info->stream > NN && info->stream <= PS);

  /* Assign a mapping to the page */   
  pte = new page_entry;
  assert(pte);
  memset(pte, 0, sizeof(page_entry));

  off = PG_CLSTR_SIZE;
 

  if (!cluster->free)
#if 0
  if (!(cluster->stream_free[info->stream]))
  if (cluster->id == PG_CLSTR_GEN)
#endif
  {
    off = random() % PG_CLSTR_SIZE;
    
    /* If entry was already allocated, deallocate and free the pte */
    if (cluster->valid[off])
    {
      cluster->valid[off] = FALSE;
      pg_itr = cache->page_table.find(cluster->page_no[off]);
    
      assert(pg_itr != cache->page_table.end());
      
      old_pte = (page_entry *)pg_itr->second;
      assert(old_pte); 

      cache->page_table.erase(cluster->page_no[off]);

      delete old_pte;

      cluster->free++;
    }
  }
  else
  {
#if 0
    assert(cluster->stream_free[info->stream]);
#endif

    /* Get next free page entry */
    for (off = 0; off < PG_CLSTR_SIZE; off++)
    {
      if (cluster->valid[off] == FALSE)
        break;
    }
  }

  assert(off != PG_CLSTR_SIZE);
  assert(cluster->valid[off] == FALSE); 

  cluster->valid[off]     = TRUE;
  cluster->page_no[off]   = info->address >> LOG_PG_SIZE;
  
  if (cluster->id != PG_CLSTR_GEN)
  {
    cluster->stream_free[info->stream]--;
  }
  
  assert(cluster->free);

  cluster->free--;
  cluster->allocated++;

#if 0
  page_no = cluster->id * PG_CLSTR_SIZE + off;
#endif
  page_no = info->address >> LOG_PG_SIZE;

  pte->frame_no     = page_no;
  pte->stream       = info->stream;
  pte->xstream      = FALSE;
  pte->cluster_id   = cluster->id;
  pte->cluster_off  = off;

  return pte;
}

ub8 mmu_translate(cachesim_cache *cache, memory_trace *info)
{
  map <ub8, ub8>::iterator pg_itr;
  ub8 phy_address;
  ub8 page_frame;
  page_cluster *cluster; 
  page_entry   *pte;

  page_frame = info->address >> LOG_PG_SIZE;

  /* Lookup page table if found in page table return the address */
  pg_itr = cache->page_table.find(page_frame);

  if (pg_itr != cache->page_table.end())
  {
    pte = (page_entry *)(pg_itr->second);
    assert(pte);
    
    if (pte->stream != info->stream)
    {
      pte->xstream = TRUE;
    }

    pg_itr = cache->i_page_table.cluster.find(pte->cluster_id);
    assert (pg_itr != cache->i_page_table.cluster.end());

    cluster = (page_cluster *)(pg_itr->second);

    goto end;
  }
  
  cluster = get_cluster(cache, info);
  assert(cluster);
  
  pte = allocate_pte(cache, cluster, info);   
  assert(pte);
  
  cache->i_page_table.stream_alloc[info->stream] += 1;

  cache->page_table.insert(pair<ub8, ub8>(page_frame, (ub8)pte));
  
end:
  
  cluster->stream_access[info->stream] += 1;

  assert(pte->frame_no <= MAX_PAGE);
  phy_address = (pte->frame_no << LOG_PG_SIZE) | (info->address & (PG_SIZE - 1));

  printf("Cluster %ld allocated to stream %s\n", info->address >> (LOG_PG_SIZE + 5), stream_names[info->stream]);

  return phy_address;
}

#undef LOG_PG_SIZE
#undef PG_SIZE
#undef LOG_PHY_ADDR_SIZE
#undef MAX_PAGE
#undef MAX_CLUSTER
#undef NEXT_CLUSTER
#undef NEXT_SCLUSTER

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

    assert(vctm_stream >= 0 && vctm_stream <= TST + MAX_CORES - 1);

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

      if (vctm_block.dirty && cache->dramsim_enable)
      {
        info_out = (memory_trace *)calloc(1, sizeof(memory_trace));
        assert(info_out);

        info_out->address     = vctm_block.tag;
        info_out->stream      = vctm_stream;
        info_out->spill       = TRUE;
        info_out->sap_stream  = vctm_block.sap_stream;

        /* As mshr have been freed for the completed fill. So, it must be available. */
        mshr_alloc = cs_alloc_mshr(cache, info_out);
        assert(mshr_alloc == CS_MSHR_ALLOC);

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

cache_access_status cachesim_incl_cache(cachesim_cache *cache, ub8 addr, 
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
    
    if (cache->dramsim_enable && info->fill == TRUE)
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
        info_out->stream      = info->stream;
        info_out->sap_stream  = info->sap_stream;
        info_out->fill        = TRUE;

        ret.fate = CACHE_ACCESS_MISS;

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
        ret = cachesim_fill_block(cache, info);
        get_address_map(cache, info->stream, info, &(cache->dram_info));
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

  if (strm == NN || strm > TST + MAX_CORES - 1)
  {
    cout << "Illegal stream " << (int)strm << endl;
  }

  assert(strm != NN && strm <= TST + MAX_CORES - 1);

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
      info_out->stream      = info->stream;
      info_out->sap_stream  = info->sap_stream;
      info_out->fill        = TRUE;

      ret.fate = CACHE_ACCESS_MISS;

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
  params->max_rrpv              = lcP->max_rrpv;
  params->spill_rrpv            = lcP->spill_rrpv;
  params->threshold             = lcP->threshold;
  params->ways                  = cache_way;
  params->stream                = lcP->stream;
  params->streams               = lcP->streams;
  params->bs_epoch              = lcP->useBs;
  params->speedup_enabled       = lcP->speedupEnabled;
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
}

void closeStatisticsStream(gzofstream &out_stream)
{
  out_stream.close();
}

void dumpOverallStats(cachesim_cache *cache, ub8 cycle, ub8 cache_cycle, ub8 dram_cycle)
{
  FILE *fout;
  ub1   i;

  fout = fopen("OverallCacheStats.log", "w+");
  assert(fout);

  fprintf(fout, "[LLC]\n");
  
  fprintf(fout, "Cycles = %ld\n", cycle);
  fprintf(fout, "CacheCycles = %ld\n", cache_cycle);
  fprintf(fout, "DramCycles = %ld\n", dram_cycle);
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

void update_access_stats(cachesim_cache *cache, memory_trace *info, cache_access_status ret)
{
  /* Update statistics only for fill */
  if (info->fill == TRUE)
  {
    if (ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
    {
      cache->miss[info->stream]++;

      if (info->stream >= PS && info->stream <= PS + MAX_CORES - 1)
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

    if (info->stream >= PS && info->stream <= PS + MAX_CORES - 1)
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
    if (info->stream >= PS && info->stream <= PS + MAX_CORES - 1)
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
        if (ret.stream >= PS && ret.stream <= PS + MAX_CORES - 1)
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
        if (info->stream >= PS && info->stream <= PS + MAX_CORES - 1)
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
        if (ret.stream >= PS && ret.stream <= PS + MAX_CORES - 1)
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
      if (info->stream >= PS && info->stream <= PS + MAX_CORES - 1)
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

            assert(info->spill == 0);

            switch (ret.stream)
            {
              case CS:
                (*t_cthit)++;

                if (ret.dirty)
                {
                  assert(info->spill == 0);
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
                  assert(info->spill == 0 && info->fill == 1);
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
            cout << "Illegal stream " << (int)info->stream << " for address ";
            cout << hex << info->address << " type : ";
            cout << dec << (int)info->fill << " spill: " << dec << (int)info->spill;
            assert(0);
        }
      }

#if 0
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

            assert(c_blocks->getValue(1) && info->stream != CS);
            (*c_blocks)--;
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
            cout << hex << info->address << " type : ";
            cout << dec << (int)info->fill << " spill: " << dec << (int)info->spill;
            assert(0);
        }
      }
#endif
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

  if (info->stream >= PS && info->stream <= PS + MAX_CORES - 1)
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

      default:
        cout << "Illegal stream " << (int)info->stream << " for address ";
        cout << hex << info->address << " type : ";
        cout << dec << (int)info->fill << " spill: " << dec << (int)info->spill;
        assert(0);
    }
  }
}

ub1 dram_cycle(cachesim_cache *cache)
{
  memory_trace        *info;
  cache_access_status  ret;
  
  assert(cache->dramsim_enable);

  dramsim_update();

  info = dram_response();

  if (info)
  {
    assert(pending_requests);

    cs_free_mshr(cache, info);

    pending_requests -= 1;

    if (info->fill)
    {
      ret = cachesim_fill_block(cache, info);

      update_access_stats(cache, info, ret);
    }

    free(info);
  }

  return (pending_requests == 0);
}

ub1 cache_cycle(cachesim_cache *cache, ub8 cachecycle)
{
  cs_qnode             *current_queue;  /* Queue for last request */
  cs_qnode             *current_node;   /* Last request node */
  memory_trace         *info;           /* Generic structure used for requests between units */
  memory_trace         *new_info;       /* Generic structure used for requests between units */
  cache_access_status   ret;            /* Cache access status */

  memset(&ret, 0, sizeof(cache_access_status));

  if (!gzeof(cache->trc_file) || !cs_qempty(&(cache->rdylist)) || !cs_qempty(&(cache->plist)))
  {
    info          = NULL;
    current_queue = NULL;
    current_node  = NULL;

    if (cs_qempty(&(cache->rdylist)) && !gzeof(cache->trc_file))
    {
      new_info = (memory_trace *)calloc(1, sizeof(memory_trace));
      assert(new_info);
  
      gzread(cache->trc_file, (char *)new_info, sizeof(memory_trace));

      if (!gzeof(cache->trc_file))
      {
        cs_qappend(&(cache->rdylist), (ub1 *)new_info);  
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
      if (!cs_qempty(&(cache->rdylist)))
      {
        current_queue = &(cache->rdylist);
        current_node  = (cs_qnode*)QUEUE_HEAD(&(cache->rdylist));

        info = (memory_trace *)QUEUE_HEAD_DATA(&(cache->rdylist));
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

    if (info && cachecycle >= info->cycle)
    {
      assert(current_queue);
      assert(current_node);

      info->vtl_addr  = info->address;
      info->address   = mmu_translate(cache, info);

      if (cache->dramsim_trace == FALSE)
      {
        /* Increment per-set access count */
        ret = cachesim_incl_cache(cache, info->address, 0, info->stream, info);

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
    }
  }

  return (gzeof(cache->trc_file));
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
  ub8  dram_ctime;
  ub8  cache_next_cycle;
  ub8  dram_next_cycle;
  ub8  current_cycle;
  ub8  total_cycle;
  ub8  dramcycle;
  ub8  cachecycle;
  ub1  dram_done;
  ub1  cache_done;

  gzofstream stats_stream;

  /* Cache configuration parser */
  SimParameters       sim_params;
  struct cache_params params;
  cachesim_cache      l3cache;
  ConfigLoader       *cfg_loader;
  StatisticsManager  *sm;

  /* Verify bitness */
  assert(sizeof(ub8) == 8 && sizeof(ub4) == 4);

  inscnt            = 0;
  trc_file_name     = NULL;
  config_file_name  = NULL;
  invalid_hits      = 0;
  pending_requests  = 0;
  total_cycle       = 0;
  dramcycle         = 0;
  cachecycle        = 0;

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

  if (sim_params.lcP.dramSimEnable == FALSE)
  {
    dram_config_init(&(l3cache.dram_info));
  }

  cout << "Running for " << cache_count << " iterations\n";

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

#define TOTAL_BANKS (NUM_CHANS * NUM_RANKS * NUM_BANKS)

      for (ub1 i = 0; i <= TST; i++)
      {
        for (ub1 j = 0 ; j < TOTAL_BANKS; j++)
        {
          l3cache.stream_access[i][j]   = 0;
          l3cache.bank_access[i][j]     = 0;
          l3cache.row_access[i][j]      = 0;
          l3cache.row_coverage[i][j]    = 0;
        }

        l3cache.bank_hit[i]         = 0;
        l3cache.bank_access_itr[i]  = 0;

      }

      for (ub1 j = 0 ; j <= TOTAL_BANKS; j++)
      {
        l3cache.g_bank_access[j]  = 0;
        l3cache.g_row_access[j]   = 0;
        l3cache.g_row_coverage[j] = 0;

        l3cache.ct_row_access[j]  = 0;
        l3cache.cz_row_access[j]  = 0;
        l3cache.cp_row_access[j]  = 0;
        l3cache.cb_row_access[j]  = 0;
        l3cache.zt_row_access[j]  = 0;
        l3cache.zp_row_access[j]  = 0;
        l3cache.zb_row_access[j]  = 0;
        l3cache.tp_row_access[j]  = 0;
        l3cache.tb_row_access[j]  = 0;
        l3cache.pb_row_access[j]  = 0;
      }

#undef TOTAL_BANKS

      l3cache.g_bank_access_itr = 0;

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

      printf("Cache parameters : Size %ld Ways %d Sets %d\n", 
          cache_sizes[c_cache], params.ways, params.num_sets);

      /* Initialize components and record cycle times */
      cache_ctime       = cs_init(&l3cache, params, trc_file_name);
      cache_next_cycle  = cache_ctime;
      
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
          current_cycle     = MINIMUM(cache_next_cycle, dram_next_cycle);
          cache_next_cycle  = cache_next_cycle - current_cycle;
          dram_next_cycle   = dram_next_cycle - current_cycle;
        }
        else
        {
          current_cycle     = cache_next_cycle;
          cache_next_cycle  = cache_next_cycle - current_cycle;
        }

        if (!cache_next_cycle)
        {
          cache_done = cache_cycle(&l3cache, cachecycle);
          cache_next_cycle = cache_ctime;
          cachecycle += 1;
        }

        if (l3cache.dramsim_enable && !dram_next_cycle)
        {
          dram_done = dram_cycle(&l3cache);
          dram_next_cycle = dram_ctime;
          dramcycle += 1;
        }

        total_cycle += 1;

        if (!inscnt)
        {
          sm->dumpNames("CacheSize", CH, stats_stream);
        }

        inscnt++;

        (*all_access)++;

        if (!(inscnt % 1000000))
        {
          sm->dumpValues(cache_sizes[c_cache], 0, CH, stats_stream);
          periodicReset();

          /* Dump dramsim bank info */
          dram_config *dram_info;

#define TOTAL_BANKS (NUM_CHANS * NUM_RANKS * NUM_BANKS)

          map<ub1, ub8>::iterator bank_itr;
          map<ub1, ub8>::iterator stream_itr;
          map<ub8, ub8>::iterator row_itr;
          ub1 stream_id;
          ub1 bank_id;
          map <ub1, ub8> *bank_list;
          bank_stats     *per_bank_stats;
          row_stats      *per_row_stats;
          map <ub8, ub8> *per_stream_rows[TST + 1][TOTAL_BANKS + 1];
          map <ub8, ub8> *per_stream_row_list;
          map <ub8, ub8> *cs_row_list;
          map <ub8, ub8> *zs_row_list;
          map <ub8, ub8> *ts_row_list;
          map <ub8, ub8> *ps_row_list;
          map <ub8, ub8> *bs_row_list;
          
          for (ub4 i = 0; i <= TST; i++)
          {
            for (ub4 j = 0; j <= TOTAL_BANKS; j++)
            {
              per_stream_rows[i][j] = NULL;
            }
          }

          dram_info = &(l3cache.dram_info);

          for (stream_itr = dram_info->bank_list.begin(); stream_itr != dram_info->bank_list.end(); stream_itr++)
          {
            stream_id =  stream_itr->first;
            bank_list = (map<ub1, ub8> *)(stream_itr->second);
            
            for (bank_itr = bank_list->begin(); bank_itr != bank_list->end(); bank_itr++)
            {
              bank_id         = bank_itr->first;
              per_bank_stats  = (bank_stats *)(bank_itr->second);
              
              per_stream_row_list = new map <ub8, ub8> ();

              l3cache.stream_access[stream_id][bank_id] += per_bank_stats->access;
              l3cache.bank_access[stream_id][bank_id]   += 1;
              l3cache.row_access[stream_id][bank_id]    += per_bank_stats->rows.size();
               
              for (row_itr = per_bank_stats->rows.begin(); row_itr != per_bank_stats->rows.end(); row_itr++)
              {
                per_row_stats = (row_stats *)(row_itr->second);

                l3cache.row_coverage[stream_id][bank_id] += per_row_stats->blocks.size();

                delete per_row_stats;

                per_stream_row_list->insert(pair<ub8, ub8>(row_itr->first, 1));
              }
              
              per_stream_rows[stream_id][bank_id] = per_stream_row_list;

              delete per_bank_stats;
            }
            
            /* If stream has accesses any bank */
            if (bank_list->size())
            {
              l3cache.bank_access_itr[stream_id]  += 1;
              l3cache.bank_hit[stream_id]         += bank_list->size();
            }

            delete bank_list;
          }
          
          /* Find overlapping rows */
          for (ub1 bank = 0; bank <= TOTAL_BANKS; bank++)
          {
            cs_row_list = per_stream_rows[CS][bank];
            zs_row_list = per_stream_rows[ZS][bank];
            ts_row_list = per_stream_rows[TS][bank];
            ps_row_list = per_stream_rows[PS][bank];
            bs_row_list = per_stream_rows[BS][bank];

            /* Find color overlap */
            if (cs_row_list)
            {
              for (row_itr = cs_row_list->begin(); row_itr != cs_row_list->end(); row_itr++)
              {
                if (zs_row_list && zs_row_list->find(row_itr->first) != zs_row_list->end())
                {
                  l3cache.cz_row_access[bank] += 1;
                }

                if (ts_row_list && ts_row_list->find(row_itr->first) != ts_row_list->end())
                {
                  l3cache.ct_row_access[bank] += 1;
                }

                if (ps_row_list && ps_row_list->find(row_itr->first) != ps_row_list->end())
                {
                  l3cache.cp_row_access[bank] += 1;
                }

                if (bs_row_list && bs_row_list->find(row_itr->first) != bs_row_list->end())
                {
                  l3cache.cb_row_access[bank] += 1;
                }
              }
            }

            /* Find depth overlap */
            if (zs_row_list)
            {
              for (row_itr = zs_row_list->begin(); row_itr != zs_row_list->end(); row_itr++)
              {
                if (ts_row_list && ts_row_list->find(row_itr->first) != ts_row_list->end())
                {
                  l3cache.zt_row_access[bank] += 1;
                }

                if (ps_row_list && ps_row_list->find(row_itr->first) != ps_row_list->end())
                {
                  l3cache.zp_row_access[bank] += 1;
                }

                if (bs_row_list && bs_row_list->find(row_itr->first) != bs_row_list->end())
                {
                  l3cache.zb_row_access[bank] += 1;
                }
              }
            }

            /* Find texture overlap */
            if (ts_row_list)
            {
              for (row_itr = ts_row_list->begin(); row_itr != ts_row_list->end(); row_itr++)
              {
                if (ps_row_list && ps_row_list->find(row_itr->first) != ps_row_list->end())
                {
                  l3cache.tp_row_access[bank] += 1;
                }

                if (bs_row_list && bs_row_list->find(row_itr->first) != bs_row_list->end())
                {
                  l3cache.tb_row_access[bank] += 1;
                }
              }
            }

            /* Find CPU overlap */
            if (ps_row_list)
            {
              for (row_itr = ps_row_list->begin(); row_itr != ps_row_list->end(); row_itr++)
              {
                if (bs_row_list && bs_row_list->find(row_itr->first) != bs_row_list->end())
                {
                  l3cache.pb_row_access[bank] += 1;
                }
              }
            }
          }

          for (ub1 stream = NN; stream <= TST; stream++)
          {
            for (ub1 bank = 0; bank <= TOTAL_BANKS; bank++)
            {
              if (per_stream_rows[stream][bank])
              {
                delete per_stream_rows[stream][bank];
              }
            }
          }

          dram_info->bank_list.clear();
          
          /* Reset global bank list and add values into accumulator counters */
          for (bank_itr = dram_info->g_bank_list.begin(); bank_itr != dram_info->g_bank_list.end(); bank_itr++)
          {
            bank_id = bank_itr->first;
            per_bank_stats = (bank_stats *)(bank_itr->second);

            l3cache.g_bank_access[bank_id]   += 1;
            l3cache.g_row_access[bank_id]    += per_bank_stats->rows.size();

            for (row_itr = per_bank_stats->rows.begin(); row_itr != per_bank_stats->rows.end(); row_itr++)
            {
              per_row_stats = (row_stats *)(row_itr->second);

              l3cache.g_row_coverage[bank_id] += per_row_stats->blocks.size();

              delete per_row_stats;
            }

            delete per_bank_stats;
          }

          /* If stream has accesses any bank */
          if (dram_info->g_bank_list.size())
          {
            l3cache.g_bank_access_itr  += 1;
          }

          dram_info->g_bank_list.clear();

          /* Add new values to total */
          l3cache.itr_count += 1;
        }
      }while(!cache_done || !dram_done);
      
      dram_config *dram_info;
      ub1 stream_id;
      ub1 bank_id;
      map <ub1, ub8> *bank_list;
      
      printf("Total iterations %ld\n", l3cache.itr_count);

      printf("DRAM Average bank access stats \n");

      printf("Stream; ");
      for (bank_id = 0; bank_id < NUM_CHANS * NUM_RANKS * NUM_BANKS; bank_id++)
      {
        printf("%d; ", bank_id);
      }

      printf("\n");

      for (stream_id = NN; stream_id <= TST; stream_id++)
      {
        printf("%d; ", stream_id);
        for (bank_id = 0; bank_id < NUM_CHANS * NUM_RANKS * NUM_BANKS; bank_id++)
        {
          if (l3cache.bank_access_itr[stream_id])
          {
            printf("%ld; ", l3cache.bank_access[stream_id][bank_id]);
          }
          else
          {
            printf("0; ");
          }
        }
        printf("\n");
      }

      printf("\n");

      printf("DRAM Average per-bank access stats \n");

      for (stream_id = NN; stream_id <= TST; stream_id++)
      {
        printf("%d; ", stream_id);
        for (bank_id = 0; bank_id < NUM_CHANS * NUM_RANKS * NUM_BANKS; bank_id++)
        {
          if (l3cache.bank_access_itr[stream_id])
          {
            printf("%ld; ", l3cache.stream_access[stream_id][bank_id] / l3cache.bank_access_itr[stream_id]);
          }
          else
          {
            printf("0; ");
          }
        }

        printf("\n");
      }
      
      printf("\n");
      
      printf("DRAM Average per-bank rows access stats \n");

      for (stream_id = NN; stream_id <= TST; stream_id++)
      {
        printf("%d; ", stream_id);
        for (bank_id = 0; bank_id < NUM_CHANS * NUM_RANKS * NUM_BANKS; bank_id++)
        {
          if (l3cache.bank_access_itr[stream_id])
          {
#if 0
            printf("%ld; ", l3cache.row_access[stream_id][bank_id] / l3cache.bank_access_itr[stream_id]);
#endif
            printf("%ld; ", l3cache.row_access[stream_id][bank_id]);
          }
          else
          {
            printf("0; ");
          }
        }

        printf("\n");
      }
      
      printf("\n");

      printf("DRAM Average per-bank rows coverage stats \n");

      for (stream_id = NN; stream_id <= TST; stream_id++)
      {
        printf("%d; ", stream_id);
        for (bank_id = 0; bank_id < NUM_CHANS * NUM_RANKS * NUM_BANKS; bank_id++)
        {
          if (l3cache.row_access[stream_id][bank_id])
          {
#if 0
            printf("%ld; ", l3cache.row_coverage[stream_id][bank_id] / l3cache.row_access[stream_id][bank_id]);
#endif
            printf("%ld; ", l3cache.row_coverage[stream_id][bank_id]);
          }
          else
          {
            printf("0; ");
          }
        }

        printf("\n");
      }
      
      printf("\n");

      printf("DRAM Average global per-bank rows access stats \n");

      for (bank_id = 0; bank_id < NUM_CHANS * NUM_RANKS * NUM_BANKS; bank_id++)
      {
        if (l3cache.g_bank_access_itr)
        {
#if 0
          printf("%ld; ", l3cache.g_row_access[bank_id] / l3cache.g_bank_access_itr);
#endif
          printf("%ld; ", l3cache.g_row_access[bank_id]);
        }
        else
        {
          printf("0; ");
        }
      }

      printf("\n");

      printf("DRAM Average global per-bank rows coverage stats \n");

     for (bank_id = 0; bank_id < NUM_CHANS * NUM_RANKS * NUM_BANKS; bank_id++)
     {
       if (l3cache.g_row_access[bank_id])
       {
#if 0
         printf("%ld; ", l3cache.g_row_coverage[bank_id] / l3cache.g_row_access[bank_id]);
#endif
         printf("%ld; ", l3cache.g_row_coverage[bank_id]);
       }
       else
       {
         printf("0; ");
       }
     }
      
      printf("\n");

      printf("Average banks accessed by each stream \n");
      for (stream_id = NN; stream_id <= TST; stream_id++)
      {
        if (l3cache.bank_access_itr[stream_id])
        {
          printf("%ld; ", l3cache.bank_hit[stream_id] / l3cache.bank_access_itr[stream_id]);
        }
        else
        {
          printf("0; ");
        }
      }

      printf("\n");
      
      printf("Average per bank overlapping stream \n");
      printf ("CZ; ");
      for (ub1 i = 0; i < TOTAL_BANKS; i++)
      {
        printf("%ld; ", l3cache.cz_row_access[i]); 
      }

      printf ("\nCT; ");
      for (ub1 i = 0; i < TOTAL_BANKS; i++)
      {
        printf("%ld; ", l3cache.ct_row_access[i]); 
      }

      printf ("\nCP; ");
      for (ub1 i = 0; i < TOTAL_BANKS; i++)
      {
        printf("%ld; ", l3cache.cp_row_access[i]); 
      }

      printf ("\nCB; ");
      for (ub1 i = 0; i < TOTAL_BANKS; i++)
      {
        printf("%ld; ", l3cache.cb_row_access[i]); 
      }

      printf ("\nZT; ");
      for (ub1 i = 0; i < TOTAL_BANKS; i++)
      {
        printf("%ld; ", l3cache.zt_row_access[i]); 
      }

      printf ("\nZP; ");
      for (ub1 i = 0; i < TOTAL_BANKS; i++)
      {
        printf("%ld; ", l3cache.zp_row_access[i]); 
      }

      printf ("\nZB; ");
      for (ub1 i = 0; i < TOTAL_BANKS; i++)
      {
        printf("%ld; ", l3cache.zb_row_access[i]); 
      }

      printf ("\nTP; ");
      for (ub1 i = 0; i < TOTAL_BANKS; i++)
      {
        printf("%ld; ", l3cache.tp_row_access[i]); 
      }

      printf ("\nTB; ");
      for (ub1 i = 0; i < TOTAL_BANKS; i++)
      {
        printf("%ld; ", l3cache.tb_row_access[i]); 
      }

      printf ("\nPB; ");
      for (ub1 i = 0; i < TOTAL_BANKS; i++)
      {
        printf("%ld; ", l3cache.pb_row_access[i]); 
      }
      
      printf("\n");

#undef TOTAL_BANKS

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

  if (l3cache.dramsim_enable == false)
  {
    printf("DRAM Color stats \n");
    print_dram_stats(CS, &(l3cache.dram_info));

    printf("DRAM Depth stats \n");
    print_dram_stats(ZS, &(l3cache.dram_info));

    printf("DRAM Texture stats \n");
    print_dram_stats(TS, &(l3cache.dram_info));

    printf("DRAM stats \n");
    printf("Row Access: %ld Row Hits: %ld\n", l3cache.dram_info.row_access, l3cache.dram_info.row_hits);
  }

  /* Close statistics stream */
  closeStatisticsStream(stats_stream);
  dumpOverallStats(&l3cache, total_cycle, cachecycle, dramcycle);

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
#undef MAX_MSHR
#undef TQ_SIZE
