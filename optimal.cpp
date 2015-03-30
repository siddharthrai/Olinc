#include "optimal.h"
#include "configloader/ConfigLoader.h"
#include "statisticsmanager/StatisticsManager.h"
#include "cache/cache-param.h"
#include "support/btree.h"
#include "inter-stream-reuse.h"
#include <map>
#include <vector>
#include <algorithm>
#include <iomanip>

using namespace std;

ub8 htblused = 0; /* entries in hash table */

/* get state of cache line */
#define USE_INTER_STREAM_CALLBACK (TRUE)
#define BLCKALIGN(addr)           ((addr >> CACHE_BLCKOFFSET) << CACHE_BLCKOFFSET)
#define MAX_NAME                  (100)
#define PHT_SIZE                  (1 << 20)
#define MAX_RDIS                  (2048)
#define MAX_REUSE                 (64)
#define MAX_TRANS                 (64)
#define PHASE_BITS                (12)
#define PHASE_SIZE                (1 << PHASE_BITS)
#define BMP_ENTRIES               (4)
#define IS_SPILL_ALLOCATED(s)     ((s) == CS || (s) == BS || (s) == PS)

/* Data structures for reuse distances */
map<ub8, ub8>   *htmap;                     /* Hash-table for reuse distance */
ub8             *hthead_info;               /* Per hash-table statistics */
map<ub8, ub8>::iterator htmap_itr;          /* Iterator for set */

map<ub8, ub8>   *cache_set;                 /* Cache set that keeps maps */
map<ub8, ub8>::iterator set_itr;            /* Iterator for set */

map<ub8, ub8>   pc_table;                   /* map of PCs */
map<ub8, ub8>::iterator pc_table_itr;       /* Iterator for pc_table */

ub8 *sn_set;                                /* Sequence number per set */
ub8  last_rdis;                             /* Used to track last reuse of a block */
ub8  sn_glbl;                               /* Global sequence number for all accesses */

ub8 rdis_hist[TST + 1][MAX_RDIS + 1][2];    /* Reuse distance and reuse count histogram */
ub8 reuse_hist[TST + 1][MAX_REUSE + 1];     /* Reuse distance histogram */
ub8 rrpv_trans_hist[TST + 1][MAX_TRANS + 1];/* RRPV transition histogram */

/* Per block info. A max heap is build on top of this vector for finding max 
 * reuse distance block */
vector<cachesim_cacheline*> *phy_way;      

InterStreamReuse *reuse_ct_cbk;      /* Callback for CT reuse */
InterStreamReuse *reuse_cc_cbk;      /* Callback for CC reuse */
InterStreamReuse *reuse_bt_cbk;      /* Callback for BT reuse */
InterStreamReuse *reuse_bb_cbk;      /* Callback for BB reuse */
InterStreamReuse *reuse_zt_cbk;      /* Callback for ZT reuse */
InterStreamReuse *reuse_zz_cbk;      /* Callback for ZZ reuse */
InterStreamReuse *reuse_cb_cbk;      /* Callback for CB reuse */
InterStreamReuse *reuse_tt_cbk;      /* Callback for II reuse */

NumericStatistic <ub8> *all_access;  /* Input stream access */

NumericStatistic <ub8> *i_access;    /* Input stream access */
NumericStatistic <ub8> *i_miss;      /* Input stream miss  */
NumericStatistic <ub8> *i_raccess;   /* Input stream access */
NumericStatistic <ub8> *i_rmiss;     /* Input stream miss  */
NumericStatistic <ub8> *i_blocks;    /* Input stream blocks  */
NumericStatistic <ub8> *i_xevct;     /* Input inter stream evict */
NumericStatistic <ub8> *i_sevct;     /* Input intra stream evict */
NumericStatistic <ub8> *i_zevct;     /* Input intra stream evict */
NumericStatistic <ub8> *i_xcevct;    /* Input cross stream evict */
NumericStatistic <ub8> *i_xzevct;    /* Input cross stream evict */
NumericStatistic <ub8> *i_xtevct;    /* Input cross stream evict */
NumericStatistic <ub8> *i_xhit;      /* Input inter stream hit */
NumericStatistic <ub8> *i_rdis;      /* Input stream reuse distance */

NumericStatistic <ub8> *c_access;    /* Color stream access */
NumericStatistic <ub8> *c_dwrite;    /* Color stream write to default RT */
NumericStatistic <ub8> *c_miss;      /* Color stream miss */
NumericStatistic <ub8> *c_raccess;   /* Color stream access */
NumericStatistic <ub8> *c_rmiss;     /* Color stream miss */
NumericStatistic <ub8> *c_blocks;    /* Color stream blocks */
NumericStatistic <ub8> *c_cprod;     /* Color Write back */
NumericStatistic <ub8> *c_ccons;     /* Text C stream consumption */
NumericStatistic <ub8> *c_xevct;     /* Color inter stream evict */
NumericStatistic <ub8> *c_sevct;     /* Color intra stream evict */
NumericStatistic <ub8> *c_zevct;     /* Color intra stream evict */
NumericStatistic <ub8> *c_xzevct;    /* Input cross stream evict */
NumericStatistic <ub8> *c_xtevct;    /* Input cross stream evict */
NumericStatistic <ub8> *c_xhit;      /* Color inter stream hit */
NumericStatistic <ub8> *c_rdis;      /* Color stream reuse distances */
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
NumericStatistic <ub8> *c_re0fill;   /* Epoch 0 fill */
NumericStatistic <ub8> *c_re0hit;    /* Epoch 0 hit */
NumericStatistic <ub8> *c_re1fill;   /* Epoch 1 fill */
NumericStatistic <ub8> *c_re1hit;    /* Epoch 1 hit */
NumericStatistic <ub8> *c_re2fill;   /* Epoch 2 fill */
NumericStatistic <ub8> *c_re2hit;    /* Epoch 2 hit */
NumericStatistic <ub8> *c_re3fill;   /* Epoch 3 fill */
NumericStatistic <ub8> *c_re3hit;    /* Epoch 3 hit */
NumericStatistic <ub8> *c_re0evt;    /* Epoch 0 eviction */
NumericStatistic <ub8> *c_re1evt;    /* Epoch 1 eviction */
NumericStatistic <ub8> *c_re2evt;    /* Epoch 2 eviction */
NumericStatistic <ub8> *c_re3evt;    /* Epoch 3 eviction */

NumericStatistic <ub8> *z_access;    /* Depth stream access */
NumericStatistic <ub8> *z_dwrite;    /* Color stream write to default RT */
NumericStatistic <ub8> *z_miss;      /* Depth stream miss */
NumericStatistic <ub8> *z_raccess;   /* Depth stream access */
NumericStatistic <ub8> *z_rmiss;     /* Depth stream miss */
NumericStatistic <ub8> *z_blocks;    /* Depth stream blocks */
NumericStatistic <ub8> *z_xevct;     /* Depth inter stream evict */
NumericStatistic <ub8> *z_sevct;     /* Depth intra stream evict */
NumericStatistic <ub8> *z_zevct;     /* Depth intra stream evict */
NumericStatistic <ub8> *z_xcevct;    /* Input cross stream evict */
NumericStatistic <ub8> *z_xtevct;    /* Input cross stream evict */
NumericStatistic <ub8> *z_xhit;      /* Depth intra stream evict */
NumericStatistic <ub8> *z_rdis;      /* Depth stream reuse distance */
NumericStatistic <ub8> *z_czhit;     /* Depth C stream hit */
NumericStatistic <ub8> *z_tzhit;     /* Depth T stream hit */
NumericStatistic <ub8> *z_bzhit;     /* Depth B stream hit */
NumericStatistic <ub8> *z_e0fill;    /* Epoch 0 fill*/
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
NumericStatistic <ub8> *z_re0fill;   /* Epoch 0 fill*/
NumericStatistic <ub8> *z_re0hit;    /* Epoch 0 hit */
NumericStatistic <ub8> *z_re1fill;   /* Epoch 1 fill */
NumericStatistic <ub8> *z_re1hit;    /* Epoch 1 hit */
NumericStatistic <ub8> *z_re2fill;   /* Epoch 2 fill */
NumericStatistic <ub8> *z_re2hit;    /* Epoch 2 hit */
NumericStatistic <ub8> *z_re3fill;   /* Epoch 3 fill */
NumericStatistic <ub8> *z_re3hit;    /* Epoch 3 hit */
NumericStatistic <ub8> *z_re0evt;    /* Epoch 0 eviction */
NumericStatistic <ub8> *z_re1evt;    /* Epoch 1 eviction */
NumericStatistic <ub8> *z_re2evt;    /* Epoch 2 eviction */
NumericStatistic <ub8> *z_re3evt;    /* Epoch 3 eviction */

NumericStatistic <ub8> *t_access;    /* Text stream access */
NumericStatistic <ub8> *t_miss;      /* Text stream miss */
NumericStatistic <ub8> *t_raccess;   /* Text stream access */
NumericStatistic <ub8> *t_rmiss;     /* Text stream miss */
NumericStatistic <ub8> *t_blocks;    /* Text stream blocks */
NumericStatistic <ub8> *t_xevct;     /* Text inter stream evict */
NumericStatistic <ub8> *t_sevct;     /* Text intra stream evict */
NumericStatistic <ub8> *t_zevct;     /* Text intra stream evict */
NumericStatistic <ub8> *t_xcevct;    /* Input cross stream evict */
NumericStatistic <ub8> *t_xzevct;    /* Input cross stream evict */
NumericStatistic <ub8> *t_xhit;      /* Text inter stream hit */
NumericStatistic <ub8> *t_rdis;      /* Text stream reuse distance */
NumericStatistic <ub8> *t_cthit;     /* Text C stream hit */
NumericStatistic <ub8> *t_ctdhit;    /* Text C stream hit */
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
NumericStatistic <ub8> *t_re0fill;   /* Epoch 0 fill */
NumericStatistic <ub8> *t_re0hit;    /* Epoch 0 hit */
NumericStatistic <ub8> *t_re1fill;   /* Epoch 1 fill */
NumericStatistic <ub8> *t_re1hit;    /* Epoch 1 hit */
NumericStatistic <ub8> *t_re2fill;   /* Epoch 2 fill */
NumericStatistic <ub8> *t_re2hit;    /* Epoch 2 hit */
NumericStatistic <ub8> *t_re3fill;   /* Epoch 3 fill */
NumericStatistic <ub8> *t_re3hit;    /* Epoch 3 hit */
NumericStatistic <ub8> *t_re0evt;    /* Epoch 0 eviction */
NumericStatistic <ub8> *t_re1evt;    /* Epoch 1 eviction */
NumericStatistic <ub8> *t_re2evt;    /* Epoch 2 eviction */
NumericStatistic <ub8> *t_re3evt;    /* Epoch 3 eviction */

NumericStatistic <ub8> *t_btrdist;   /* BT reuse distance */
NumericStatistic <ub8> *t_mxbtrdist; /* Max BT reuse distance */
NumericStatistic <ub8> *t_mnbtrdist; /* Min BT reuse distance */
NumericStatistic <ub8> *t_btblocks;  /* BT blocks */
NumericStatistic <ub8> *t_btuse;     /* BT blocks */
NumericStatistic <ub8> *t_btruse;    /* BT reuse */
NumericStatistic <ub8> *t_mxbtruse;  /* BT max reuse */
NumericStatistic <ub8> *t_mnbtruse;  /* BT min reuse */

NumericStatistic <ub8> *t_ctrdist;   /* CT reuse distance */
NumericStatistic <ub8> *t_mxctrdist; /* Max CT reuse distance */
NumericStatistic <ub8> *t_mnctrdist; /* Min CT reuse distance */
NumericStatistic <ub8> *t_ctblocks;  /* CT blocks */
NumericStatistic <ub8> *t_ctuse;     /* CT blocks */
NumericStatistic <ub8> *t_ctruse;    /* CT reuse */
NumericStatistic <ub8> *t_mxctruse;  /* CT max reuse */
NumericStatistic <ub8> *t_mnctruse;  /* CT min reuse */

NumericStatistic <ub8> *n_access;    /* Instruction stream access */
NumericStatistic <ub8> *n_miss;      /* Instruction stream miss */
NumericStatistic <ub8> *n_raccess;   /* Instruction stream access */
NumericStatistic <ub8> *n_rmiss;     /* Instruction stream miss */
NumericStatistic <ub8> *n_blocks;    /* Instruction stream blocks */
NumericStatistic <ub8> *n_xevct;     /* Instruction inter stream evict */
NumericStatistic <ub8> *n_sevct;     /* Instruction intra stream evict */
NumericStatistic <ub8> *n_zevct;     /* Instruction intra stream evict */
NumericStatistic <ub8> *n_xcevct;    /* Input cross stream evict */
NumericStatistic <ub8> *n_xzevct;    /* Input cross stream evict */
NumericStatistic <ub8> *n_xtevct;    /* Input cross stream evict */
NumericStatistic <ub8> *n_xhit;      /* Instruction inter stream hit */
NumericStatistic <ub8> *n_rdis;      /* Instruction stream reuse distance */

NumericStatistic <ub8> *h_access;    /* HZ stream access */
NumericStatistic <ub8> *h_miss;      /* HZ stream miss */
NumericStatistic <ub8> *h_raccess;   /* HZ stream access */
NumericStatistic <ub8> *h_rmiss;     /* HZ stream miss */
NumericStatistic <ub8> *h_blocks;    /* HZ stream blocks */
NumericStatistic <ub8> *h_xevct;     /* HZ inter stream evict */
NumericStatistic <ub8> *h_sevct;     /* HZ intra stream evict */
NumericStatistic <ub8> *h_zevct;     /* HZ intra stream evict */
NumericStatistic <ub8> *h_xcevct;    /* Input cross stream evict */
NumericStatistic <ub8> *h_xzevct;    /* Input cross stream evict */
NumericStatistic <ub8> *h_xtevct;    /* Input cross stream evict */
NumericStatistic <ub8> *h_xhit;      /* HZ inter stream hit */
NumericStatistic <ub8> *h_rdis;      /* HZ stream reuse distance */

NumericStatistic <ub8> *b_access;    /* Blitter stream access */
NumericStatistic <ub8> *b_miss;      /* Blitter stream miss */
NumericStatistic <ub8> *b_raccess;   /* Blitter stream access */
NumericStatistic <ub8> *b_rmiss;     /* Blitter stream miss */
NumericStatistic <ub8> *b_blocks;    /* Blitter stream blocks */
NumericStatistic <ub8> *b_xevct;     /* Blitter inter stream evict */
NumericStatistic <ub8> *b_sevct;     /* Blitter intra stream evict */
NumericStatistic <ub8> *b_zevct;     /* Blitter intra stream evict */
NumericStatistic <ub8> *b_xcevct;    /* Input cross stream evict */
NumericStatistic <ub8> *b_xzevct;    /* Input cross stream evict */
NumericStatistic <ub8> *b_xtevct;    /* Input cross stream evict */
NumericStatistic <ub8> *b_xhit;      /* Blitter inter stream hit */
NumericStatistic <ub8> *b_bprod;     /* Blitter writeback */
NumericStatistic <ub8> *b_bcons;     /* Texture B stream consumption */
NumericStatistic <ub8> *b_rdis;      /* Blitter stream reuse distance */
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
NumericStatistic <ub8> *d_zevct;     /* DAC intra stream evict */
NumericStatistic <ub8> *d_xcevct;    /* Input cross stream evict */
NumericStatistic <ub8> *d_xzevct;    /* Input cross stream evict */
NumericStatistic <ub8> *d_xtevct;    /* Input cross stream evict */
NumericStatistic <ub8> *d_xhit;      /* DAC inter stream hit */
NumericStatistic <ub8> *d_rdis;      /* DAC stream reuse distance */
NumericStatistic <ub8> *d_cdhit;     /* DAC inter C hit */
NumericStatistic <ub8> *d_zdhit;     /* DAC inter Z hit */
NumericStatistic <ub8> *d_tdhit;     /* DAC inter T hit */
NumericStatistic <ub8> *d_bdhit;     /* DAC inter B hit */

NumericStatistic <ub8> *x_access;    /* Index stream access */
NumericStatistic <ub8> *x_miss;      /* Index stream miss */
NumericStatistic <ub8> *x_raccess;   /* Index stream access */
NumericStatistic <ub8> *x_rmiss;     /* Index stream miss */
NumericStatistic <ub8> *x_blocks;    /* Index stream blocks */
NumericStatistic <ub8> *x_xevct;     /* Index inter stream evict */
NumericStatistic <ub8> *x_sevct;     /* Index intra stream evict */
NumericStatistic <ub8> *x_zevct;     /* Index intra stream evict */
NumericStatistic <ub8> *x_xcevct;    /* Input cross stream evict */
NumericStatistic <ub8> *x_xzevct;    /* Input cross stream evict */
NumericStatistic <ub8> *x_xtevct;    /* Input cross stream evict */
NumericStatistic <ub8> *x_xhit;      /* Index inter stream hit */
NumericStatistic <ub8> *x_rdis;      /* Index stream reuse distance */

NumericStatistic <ub8> *p_access;    /* CPU stream access */ 
NumericStatistic <ub8> *p_miss;      /* CPU stream miss */
NumericStatistic <ub8> *p_raccess;   /* CPU stream access */ 
NumericStatistic <ub8> *p_rmiss;     /* CPU stream miss */
NumericStatistic <ub8> *p_blocks;    /* CPU stream blocks */
NumericStatistic <ub8> *p_xevct;     /* CPU inter stream evict */
NumericStatistic <ub8> *p_sevct;     /* CPU intra stream evict */
NumericStatistic <ub8> *p_zevct;     /* CPU intra stream evict */
NumericStatistic <ub8> *p_xcevct;    /* Input cross stream evict */
NumericStatistic <ub8> *p_xzevct;    /* Input cross stream evict */
NumericStatistic <ub8> *p_xtevct;    /* Input cross stream evict */
NumericStatistic <ub8> *p_xhit;      /* CPU inter stream hit */
NumericStatistic <ub8> *p_rdis;      /* CPU stream reuse distance */

/* Moves a node up the heap */
void percolate_up(vector<cachesim_cacheline *>&vec, ub8 node_id)
{
  sb8 parent;
  sb8 child;
  ub8 tmp;

  cachesim_cacheline *parent_line;
  cachesim_cacheline *child_line;
  
  /* Obtain child and parent indexes */
  parent = (node_id) ? (node_id - 1) / 2 : 0;
  child  = node_id;
  
  /* Until we have reached root */
  while(child > 0)
  {
    /* Obtain parent and child cache line stored in the vector */
    parent_line = (cachesim_cacheline *)vec[parent];
    child_line  = (cachesim_cacheline *)vec[child];
    
    /* If swap is required */
    if (parent_line->rdis < child_line->rdis)
    {
      /* As way index is kept inside cache line, we need to first swap their
       * ids so that way ids remain intact, after that swap data elements */
      tmp = parent_line->way;
      parent_line->way = child_line->way;
      child_line->way  = tmp;
      
      /* Swap elements in the vector */
      tmp         = (ub8)vec[parent];
      vec[parent] = vec[child];
      vec[child]  = (cachesim_cacheline *)tmp;
    }
    
    /* Obtain new parent and child */
    child   = parent;
    parent  = (child) ? (child - 1) / 2 : 0;
  }
}

/* Moves a node down the heap */
void percolate_down(vector<cachesim_cacheline *>&vec, ub8 node_id)
{
  sb8 parent;
  ub8 tmp;
  ub8 child1;
  ub8 child2;
  ub8 child;

  cachesim_cacheline *parent_line;
  cachesim_cacheline *child_line;
  
  /* Obtain index to parent and both the childs */
  parent = node_id;
  
  child1 = 2 * parent + 1;
  child2 = 2 * parent + 2; 
  
#define REUSE_DISTANCE(vec, indx) (((cachesim_cacheline *)vec[indx])->rdis)

  /* Choose the one with max value of reuse distance */
  if (child2 < vec.size())
  {
    if (REUSE_DISTANCE(vec, child1) < REUSE_DISTANCE(vec, child2))
    {
      child = child2;
    }
    else
    {
      child = child1;
    }
  }
  else
  {
    child = child1;
  }
  
  /* While we have reached to the leaf */
  while(child < vec.size())
  {
    /* Obtain parent and child vector stored in the vector */
    parent_line = (cachesim_cacheline *)vec[parent];
    child_line  = (cachesim_cacheline *)vec[child];
    
    /* If swap is required */
    if (parent_line->rdis < child_line->rdis)
    {
      /* As way index is kept inside cache line, we need to first swap their
       * ids so that way ids remain inteact, after that swap data elements */
      tmp = parent_line->way;
      parent_line->way = child_line->way;
      child_line->way  = tmp;
      
      /* Swap elements in the vector */
      tmp         = (ub8)vec[parent];
      vec[parent] = vec[child];
      vec[child]  = (cachesim_cacheline *)tmp;
    }
    

    /* Obtain new parent and child */
    parent  = child;

    child1 = 2 * parent + 1;
    child2 = 2 * parent + 2; 
    
    if (child2 < vec.size())
    {
      if (REUSE_DISTANCE(vec, child1) < REUSE_DISTANCE(vec, child2))
      {
        child = child2;
      }
      else
      {
        child = child1;
      }
    }
    else
    {
      child = child1;
    }
  }

#undef REUSE_DISTANCE

}

/* Routine to maintain max heap property */
void heapify(vector<cachesim_cacheline *>&vec, ub8 node_id)
{
  sb8 parent;
  ub8 child1;
  ub8 child2;

  parent = (node_id) ? (node_id - 1) / 2 : 0;
  child1 = node_id * 2 + 1; 
  child2 = node_id * 2 + 2; 

#define REUSE_DISTANCE(vec, indx) (((cachesim_cacheline *)vec[indx])->rdis)
  
  /* If parent is valid index */
  if (parent > 0)
  {
    /* Parent has smaller reuse distance */
    if (REUSE_DISTANCE(vec, node_id) > REUSE_DISTANCE(vec, parent))
    {
      percolate_up(vec, node_id);
      return;
    }
  }
  
  /* If parent is smaller than any of the children move this node down */
  if (child1 < vec.size() && 
      REUSE_DISTANCE(vec, node_id) < REUSE_DISTANCE(vec, child1))
  {
    percolate_down(vec, node_id); 
    return;
  }

  if (child2 < vec.size() && 
    (REUSE_DISTANCE(vec, node_id) < REUSE_DISTANCE(vec, child2)))
  {
    percolate_down(vec, node_id);
    return;
  }

#undef REUSE_DISTANCE

}

cache_access_info* cachesim_get_access_info(ub8 address)
{
  map<ub8, ub8>::iterator map_itr;

  /* Find block in the per address hash table */
  map_itr = htmap->find(BLCKALIGN(address));
  if (map_itr != htmap->end()) 
  {
    return (cache_access_info *)(map_itr->second);
  }
  else
  {
    return NULL;
  }
}

/*
 *
 * NAME
 *
 *  Add a new key into map and assign it next sequence number
 *
 * DESCRIPTION
 *  
 *  Adds a new key to the map, assigns it the next sequence number 
 *  and increment sequence number.
 *
 * PARAMETERS
 *
 *  map (OUT) - sequence map
 *  key (IN)  - key to be added
 *
 * RETURNS
 *
 *  Sequence id start with 1
 *
 */

void add_to_seq_map(seq_map *pc_map, ub8 key)
{
  map<ub8, ub8>::iterator map_seq_itr;

  assert(pc_map); 
  
  /* Ensure map sequence id is initialized */
  if ((pc_map->map_seq).size() == 0)
  {
    pc_map->seq_id = 1;
  }
  
  /* Lookup map to ensure key is not a duplicate */
  map_seq_itr = (pc_map->map_seq).find(key);

  if (map_seq_itr == (pc_map->map_seq).end())
  {
    /* Insert key with next sequence id */
    (pc_map->map_seq).insert(pair<ub8, ub8>(key, pc_map->seq_id++));
  }
}


/*
 *
 * NAME
 *  
 *  Get if from a sequence map 
 *
 * DESCRIPTION
 *  
 *  Give a key finds out associated sequence id 
 *
 * PARAMETERS
 *  
 *  map (IN)  - sequence map
 *  key (IN)  - key to be looked up 
 *
 * RETURNS
 *  
 *  associated sequence id
 *
 */

ub8 get_id_from_seq_map(seq_map *pc_map, ub8 key)
{
  map<ub8, ub8>::iterator map_seq_itr;
  
  /* Obtain value for key */
  map_seq_itr = (pc_map->map_seq).find(key);

  assert(map_seq_itr != (pc_map->map_seq).end());
  assert(map_seq_itr->second != 0);

  return map_seq_itr->second;
}

/* Obtain next use distance of an access */
ub8 cachesim_getreusedistance(cachesim_qnode *head, ub8 addr)
{
  cachesim_qnode *node, *last_node;
  cachesim_ainf  *block;
  ub8 rdis;
  ub1 found;

  found     = 0;
  last_node = NULL;

  /* Find consecutive use of an address */
  for (node = head->next; node != NULL; node = node->next)
  {
    block = (cachesim_ainf *)(node->data);

    if (BLCKALIGN(block->addr) == BLCKALIGN(addr))
    {
      /* Add one more found */
      found++;

      if (found == 2)
      {
        /* Returning sequence number of next node */
        rdis = block->sn;

        /* Statistics collection */
        htblused--;
        
        /* Remove node corresponding to current access */
        if (last_node)
        {
          cachesim_qdelete(last_node);
        }
        
        assert(rdis != 0);

        break;
      }
      else
      {
        last_node = node;
        last_rdis = block->sn;
      }
    }
  }

  if (found < 2)
  {
    /* If there was only one access set reuse distance infinity */
    rdis = ~((ub8)0); 
  }

  return rdis;
}

void cachesim_createmap(cachesim_cache *cache, char *trc, cache_params *params)
{
  gzifstream  fstr;    /* Input stream */
  MtrcInfo  info;    /* Memory info structure */
  memory_trace data;
  memory_trace_in_file trace_info;
  map<ub8, ub8>::iterator map_itr;

  ub4 ctr = 0;  /* Temporary counter */
  ub8 addr;     /* Block aligned address */
  ub1 i;        /* Temporary index */
  ub8 set_indx; /* Set index */
  ub8 sn = 0;   /* global sequence number */

  /* Loop through entire trace files to build the reuse map */
  for (i = 0; i < FRAGS; i++)
  {
    fstr.open(trc, ios::in | ios::binary);
    assert(fstr.is_open());
  
    while(!fstr.eof())
    {
      fstr.read((char *)&data, sizeof(memory_trace));
      
      /* TODO: Make insertion stream aware */
      if (!fstr.eof())
      {
        cachesim_qnode    *node_list;
        cache_access_info *access_info;

        if (params->stream == NN || (data.stream == params->stream))
        {
          addr      = BLCKALIGN(data.address);
          set_indx  = ADDR_NDX(cache, data.address);
          info.addr = addr;
          info.sn   = sn_set[set_indx]++;

          sn++; 

          node_list  = NULL;

          map_itr = htmap->find(BLCKALIGN(addr));
          if (map_itr == htmap->end())
          {
            node_list = new cachesim_qnode;
            assert(node_list);
            cachesim_qinit(node_list);
            
            /* Allocate access info structure and initialize it */
            access_info = new cache_access_info;
            assert(access_info);
            memset(access_info, 0, sizeof(cache_access_info));

            /* Set access info stream as unknown */
            access_info->stream     = NN;
            access_info->list_head  = (ub8)node_list;

            /* Insert access info into htmap */
            htmap->insert(pair<ub8, ub8>(BLCKALIGN(addr), (ub8)access_info));
          }
          else
          {
            node_list = (cachesim_qnode *)(((cache_access_info *)(map_itr->second))->list_head);
          }

          /* Add info to per node list  */
          cachesim_index(node_list, info);

          /* Insert list into the map */
          if (sn % 1000000 == 0)
          {
            printf(".");
            std::cout.flush();
          }
        }
      }

      ctr++;
    }
    
    cout << "\nLoading of reuse distances done..\n";
    fstr.close();
  }
}

/* Creates hash table */
void cachesim_index(cachesim_qnode *head, MtrcInfo info)
{
  cachesim_ainf   *ndata; /* Data that goes into linked list node */

  ub8 indx;     /* Index into hash table */
  ub8 addr;     /* Block aligned address */

  assert(head);

  addr = BLCKALIGN(info.addr);
  indx = (addr & HASHTINDXMASK) >> 6;

  ndata  = (cachesim_ainf *)malloc(sizeof(cachesim_md));
  assert(ndata);

  ndata->sn     = info.sn;
  ndata->addr   = info.addr;

  cachesim_qappend(head, (ub1 *)ndata);

  /* Update entries in this bucket */
  hthead_info[indx]++;

  /* Number of entries in hash table */
  htblused++;
}

cachesim_cacheline* cachesim_get_block(cachesim_cache *cache, ub8 addr)
{
  assert(cache);
  
  set_itr = cache_set[ADDR_NDX(cache, addr)].find(BLCKALIGN(addr));
  
  assert (set_itr != cache_set[ADDR_NDX(cache, addr)].end());

  return (cachesim_cacheline *)(set_itr->second);
}

/* Do associative tag match */
cache_access_status cachesim_match(cachesim_cache *cache, ub8 addr)
{
  ub8 indx;

  cachesim_cacheline *line;
  cache_access_status ret;

  struct timeval st;
  struct timeval et;

  assert(cache);
  memset(&ret, 0, sizeof(cache_access_status));  

  /* Obtain set index */
  indx = ADDR_NDX(cache, addr);

  /* Initialize return data */
  ret.way     = -1;
  ret.fate    = CACHE_ACCESS_UNK;
  ret.stream  = NN;
  
  /* Note down request start time */
  gettimeofday(&st, NULL);

  set_itr = cache_set[indx].find(BLCKALIGN(addr));
  
  /* Note down request end time */
  gettimeofday(&et, NULL);

  if (set_itr != cache_set[indx].end())
  {
    line = (cachesim_cacheline *)(set_itr->second);

    /* Update request status */
    ret.fate        = CACHE_ACCESS_HIT;
    ret.tag         = line->tag;
    ret.vtl_addr    = line->vtl_addr;
    ret.stream      = line->stream;
    ret.way         = line->way;
    ret.epoch       = line->epoch;
    ret.dirty       = line->dirty;
    ret.access      = line->access;
    ret.rdis        = line->rdis;
    ret.prdis       = line->prdis;
    ret.pc          = line->pc;
    ret.fillpc      = line->fillpc;
    ret.is_bt_block = line->is_bt_block;
    ret.is_ct_block = line->is_ct_block;
    ret.op          = line->op;

    assert(ret.rdis >= ret.prdis);
    assert(ret.tag == addr);
    return ret; 
  }
  
  ret.fate  = CACHE_ACCESS_MISS;

  return ret;
}

/* Fill the block in the cache */
cache_access_status cachesim_opt_replace_or_add(
  cachesim_cache *cache,
  ub8             addr, 
  memory_trace    info, 
  ub8             rdis)
{
  ub8 indx;

  cachesim_cacheline *rpl_line;
  cachesim_cacheline *line;
  cache_access_status ret;
  
  /* Sanity check */
  assert(cache);

  /* Initialize return data */
  memset(&ret, 0, sizeof(cache_access_status));
  
  /* Get set index */
  indx = ADDR_NDX(cache, addr);

  ret.way     = -1;
  ret.fate    = CACHE_ACCESS_UNK;
  ret.stream  = NN;
  ret.epoch   = 0;

  /* Get root of the max heap */ 
  line    = phy_way[indx][0];
  set_itr = cache_set[indx].find(line->tag);
  
  /* If replacement candidate is found */
  if (line->valid) 
  {
    if (info.spill == FALSE || (info.spill == TRUE && IS_SPILL_ALLOCATED(info.stream)))
    {
      /* Find cache line in cache set */
      rpl_line = (cachesim_cacheline *)(set_itr->second);
      assert(rpl_line); 

      /* Ensure replacement candidate has largest reuse distance */
      assert(rpl_line == line);

      ret.fate        = CACHE_ACCESS_RPLC;
      ret.tag         = rpl_line->tag;
      ret.vtl_addr    = rpl_line->vtl_addr;
      ret.way         = rpl_line->way;
      ret.stream      = rpl_line->stream;
      ret.pc          = rpl_line->pc;
      ret.fillpc      = rpl_line->fillpc;
      ret.dirty       = rpl_line->dirty;
      ret.epoch       = rpl_line->epoch;
      ret.access      = rpl_line->access;
      ret.is_bt_block = rpl_line->is_bt_block;
      ret.is_ct_block = rpl_line->is_ct_block;
      ret.op          = rpl_line->op;

      /* Replace the block  */ 
      cache_set[indx].erase(set_itr);
    }
    else
    {
      ret.fate  = CACHE_ACCESS_MISS;
    }
  }
  else
  {
    /* Ensure invalid block has max reuse distance */
    assert(line->rdis == ~((ub8)0));     

    ret.fate  = CACHE_ACCESS_MISS;
  }
  
  bool is_max_rrpv = false;

  /* Check if there is a block with max RRPV */
  for (ub8 k = 0; k < phy_way[indx].size(); k++)
  {
    if (((cachesim_cacheline *)(phy_way[indx][k]))->rrpv == CACHE_MAX_RRPV(cache))
    {
      is_max_rrpv = true;
      break;
    }
  }
  
  if (!is_max_rrpv)
  {
    ub4 old_rrpv;
    cache_access_info *access_info;

    while (!is_max_rrpv)
    {
      for (ub8 k = 0; k < phy_way[indx].size(); k++)
      {
        old_rrpv = ((cachesim_cacheline *)(phy_way[indx][k]))->rrpv;
        assert(old_rrpv < CACHE_MAX_RRPV(cache));

        if (old_rrpv == CACHE_MAX_RRPV(cache) - 1)
        {
          is_max_rrpv = true;
          /* If new RRPV has reached max make it 0 and note down transition */
          ((cachesim_cacheline *)phy_way[indx][k])->rrpv = 0;

          access_info = cachesim_get_access_info(((cachesim_cacheline *)phy_way[indx][k])->tag);
          if (access_info)
          {
            access_info->rrpv_trans += 1;
          }
        }
        else
        {
          ((cachesim_cacheline *)phy_way[indx][k])->rrpv = old_rrpv + 1 ;
        }
      }
    }
  }

  /* Reinstall the new block if it is not bypassed */
  if (info.fill == TRUE || (info.spill == TRUE && IS_SPILL_ALLOCATED(info.stream)))
  {
    /* Insert new block into cache set */
    std::pair<std::map<ub8, ub8>::iterator, bool> block_itr;
    block_itr = cache_set[indx].insert(pair<ub8, ub8>(BLCKALIGN(addr), (ub8)line));

    assert(block_itr.second == true);

    line->valid       = TRUE;
    line->rdis        = rdis;
    line->tag         = BLCKALIGN(addr);
    line->vtl_addr    = BLCKALIGN(info.vtl_addr);
    line->stream      = info.stream;
    line->pc          = info.pc;
    line->fillpc      = info.pc;
    line->dirty       = (info.spill) ? 1 : 0;
    line->epoch       = 0;
    line->access      = 0;
    line->rrpv        = CACHE_MAX_RRPV(cache) - 1;
    line->is_bt_block = FALSE;
    line->is_ct_block = FALSE;
    line->op          = block_op_fill;
  }

  /* Update expected block count based on bit vector */
  if (info.pc)
  {
    pc_data *stall_data;
    ub8      tgt_bitmap;

    pc_table_itr = pc_table.find(info.pc);

    /* For old PC, if block is not present in the block list, add block to 
     * the list  */
    if (pc_table_itr != pc_table.end());
    {
      stall_data = (pc_data *)(pc_table_itr->second);
      assert(stall_data);
      assert(stall_data->bmp_fifo);

      tgt_bitmap = stall_data->bmp_fifo->find_max_bitmap();

      /* If there is a bitmap with non-zero accesses */
      if (tgt_bitmap != 0)
      {
        map<ub8, ub8>::iterator bitmap_itr;
        cache_access_data *cache_data;

        /* Get bitmap from bitmaps map and increment expected blocks */
        bitmap_itr = stall_data->bitmaps.find(tgt_bitmap);
        assert(bitmap_itr != stall_data->bitmaps.end());

        cache_data = (cache_access_data *)(bitmap_itr->second);
        cache_data->expected_blocks += 1;
      }
    }
  }

  /* Update CT/BT flags */
  if (info.spill == TRUE || info.fill == TRUE)
  {
    if (info.stream == BS)
    {
      line->is_bt_block = TRUE;
    }
    else
    {    
      if (info.stream == CS && info.dbuf == FALSE)
      {
        line->is_ct_block = TRUE;
      }
    }
  }

  heapify(phy_way[indx], line->way);
  
  return ret;
}

/* Simulate a cache access */
cache_access_status cachesim_incl_cache(
  cachesim_cache *cache,
  ub8             addr,
  memory_trace    info,
  ub8             rdis)
{
  /* If address is hit in L2 we are done else we match in L3, if we hit 
   * update data in L2 else update data in L3 and L2 and if a block is 
   * evected from L3 invalidate it in L2 */ 

  sb4     indx;
  struct  timeval st, et;

  pc_data *stall_data;
  map<ub8, ub8>::iterator block_itr;

  cache_access_status ret;
  cachesim_cacheline  *block;

  assert(cache);

  indx = ADDR_NDX(cache, addr);
  addr = BLCKALIGN(addr); 

  ret.way     = -1;
  ret.fate    = CACHE_ACCESS_UNK;
  ret.stream  = NN;

  /* Increase access frequency for this set */
  CACHE_AFRQ(cache)[indx]++;

  gettimeofday(&st, NULL);
  
  /* Issue callback for reuse */
  if (USE_INTER_STREAM_CALLBACK)
  {
    reuse_ct_cbk->CacheAccessBeginCbk(info, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
    reuse_cc_cbk->CacheAccessBeginCbk(info, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
    reuse_bt_cbk->CacheAccessBeginCbk(info, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
    reuse_bb_cbk->CacheAccessBeginCbk(info, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
    reuse_zt_cbk->CacheAccessBeginCbk(info, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
    reuse_zz_cbk->CacheAccessBeginCbk(info, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
    reuse_cb_cbk->CacheAccessBeginCbk(info, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
    reuse_tt_cbk->CacheAccessBeginCbk(info, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
  }

  /* Do the tag matching */
  ret = cachesim_match(cache, addr);

  /* If data access (Only data access have pc set) */
  if (info.pc)
  {
    pc_table_itr = pc_table.find(info.pc);

    /* For old PC, if block is not present in the block list, add block to 
     * the list  */
    if (pc_table_itr != pc_table.end())
    {
      stall_data = (pc_data *)(pc_table_itr->second);
      assert(stall_data);
      assert(stall_data->pc == info.pc);
    }
    else
    {
      /* If pc is not in the pc-list, insert pc into pc-list and block-data 
       * in pc-block-list */
      stall_data = new pc_data;
      assert(stall_data);

      stall_data->pc                      = info.pc;
      stall_data->stat_access_llc         = 0;
      stall_data->stat_miss_llc           = 0;
      stall_data->access_count            = 0;
      stall_data->fill_op                 = 0;
      stall_data->hit_op                  = 0;
      stall_data->pc_sharing_bitmap       = 0;
      stall_data->overflow_count          = 0;
      stall_data->bmp_fifo                = new bitmap_fifo(BMP_ENTRIES);
      
      assert(stall_data->bmp_fifo);

      /* Insert stall into pc table */
      pc_table.insert(pair<ub8, ub8>(info.pc, (ub8)stall_data));
    }

    block_data *current_block_data;

    /* Find block in the block list of the PC */
    block_itr = (stall_data->blocks).find(info.address);

    /* If a new block, insert into the pc-block-list */
    if (block_itr == (stall_data->blocks).end())
    {
      /* Allocate and initialize block-data */
      current_block_data = new block_data;
      assert(current_block_data);

      /* Initialize block data */
      current_block_data->pc_sharing_bitmap       = 0;
      current_block_data->overflow_count          = 0;
      current_block_data->block_address           = info.address;
      current_block_data->block_data.access_count = 0;
      current_block_data->block_data.hit_count    = 0;

      /* Insert into pc-block-list */
      (stall_data->blocks).insert(pair<ub8, ub8>(info.address, (ub8)current_block_data));
    }
    else
    {
      current_block_data = (block_data *)(block_itr->second);
      assert(current_block_data->block_address == info.address); 
    }

    stall_data->stat_access_llc += 1;

    if (ret.fate == CACHE_ACCESS_MISS)
    {
      stall_data->stat_miss_llc += 1;
    }
    else
    {
      /* Lookup previous PC list, if current PC is not in the list add it 
       * to the list. */
      pc_data *data;

      if (ret.pc != info.pc)
      {
        /* Update previous operation on the PC */
        stall_data->access_count += 1;

        switch (ret.op)
        {
          case block_op_fill:
            stall_data->fill_op += 1;
            break;

          case block_op_hit:
            stall_data->hit_op += 1;
            break;

          default:
            printf("Invalid operation\n");
        }

        block_itr = (stall_data->ppc).find(ret.pc);
        if (block_itr == (stall_data->ppc).end())
        {
          data = new pc_data;
          assert(data);

          data->access_count = 0;
          data->fill_op      = 0;
          data->hit_op       = 0;

          (stall_data->ppc).insert(pair<ub8, ub8>(ret.pc, (ub8)data));
        }
        else
        {
          data = (pc_data *)(block_itr->second);
        }

        /* Update previous operation on the block */
        data->access_count += 1;
        switch (ret.op)
        {
          case block_op_fill:
            data->fill_op += 1;
            break;

          case block_op_hit:
            data->hit_op += 1;
            break;

          default:
            printf("Invalid operation\n");
        }
      }
    }
  }

  if (ret.fate == CACHE_ACCESS_MISS)
  {
    CACHE_MISS(cache)++;

    /* Fill block into cache */
    ret = cachesim_opt_replace_or_add(cache, addr, info, rdis);

    block = (cachesim_cacheline *)(phy_way[indx][ret.way]);
    assert(block);

    /* If fill caused a replacement */
    if (ret.fate == CACHE_ACCESS_RPLC)
    {
      block_data *current_block_data;
      
      CACHE_EVCT(cache)[indx]++;

      /* Issue callback for reuse */
      if (USE_INTER_STREAM_CALLBACK)
      {
        reuse_ct_cbk->CacheAccessReplaceCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
        reuse_cc_cbk->CacheAccessReplaceCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
        reuse_bt_cbk->CacheAccessReplaceCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
        reuse_bb_cbk->CacheAccessReplaceCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
        reuse_zt_cbk->CacheAccessReplaceCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
        reuse_zz_cbk->CacheAccessReplaceCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
        reuse_cb_cbk->CacheAccessReplaceCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
        reuse_tt_cbk->CacheAccessReplaceCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
      }
    
      if (ret.fillpc)
      {
        /* Get stall data from pc_table */
        pc_table_itr = pc_table.find(ret.fillpc);

        /* Ensure pc is present */
        assert(pc_table_itr != pc_table.end());

        /* Obtain PC data */
        stall_data = (pc_data *)(pc_table_itr->second);

        /* Obtain block data */
        block_itr = (stall_data->blocks).find(ret.tag);
        assert(block_itr != (stall_data->blocks).end());

        current_block_data = (block_data *)(block_itr->second);
        assert(current_block_data);

        map<ub8, ub8>::iterator bitmap_itr;
        if (current_block_data->pc_sharing_bitmap > 0)
        {
          bitmap_itr = stall_data->bitmaps.find(current_block_data->pc_sharing_bitmap);

          cache_access_data *cache_data;

          if (bitmap_itr == stall_data->bitmaps.end())
          {
            cache_data = new cache_access_data;
            assert(cache_data);
            memset(cache_data, 0, sizeof(cache_access_data)); 

            (stall_data->bitmaps).insert(pair<ub8, ub8>(current_block_data->pc_sharing_bitmap, (ub8)cache_data));

            /* Count of blocks with this bitmap */
            cache_data->access_count = 1;
          }
          else
          {
            cache_data = (cache_access_data *)(bitmap_itr->second);

            /* Count of blocks with this bitmap */
            cache_data->access_count += 1;
          }
          
          /* Add bitmap to bitmap FIFO */
          (stall_data->bmp_fifo)->add_bitmap_to_fifo(current_block_data->pc_sharing_bitmap, sn_glbl);

          /* Add access and hit count of replaced block to */
          cache_data->hit_count    += current_block_data->block_data.hit_count;

          current_block_data->pc_sharing_bitmap       = 0;
          current_block_data->overflow_count          = 0;
          current_block_data->block_data.access_count = 0;
          current_block_data->block_data.hit_count    = 0;

          stall_data->pc_sharing_bitmap   = 0;
          stall_data->overflow_count      = 0;
        }
      }
    }
  }
  else
  {
    /* Issue callback for reuse */
    if (USE_INTER_STREAM_CALLBACK)
    {
      reuse_ct_cbk->CacheAccessHitCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
      reuse_cc_cbk->CacheAccessHitCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
      reuse_bt_cbk->CacheAccessHitCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
      reuse_bb_cbk->CacheAccessHitCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
      reuse_zt_cbk->CacheAccessHitCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
      reuse_zz_cbk->CacheAccessHitCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
      reuse_cb_cbk->CacheAccessHitCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
      reuse_tt_cbk->CacheAccessHitCbk(info, ret, indx, CACHE_AFRQ(cache)[indx], CACHE_EVCT(cache)[indx]);
    }

    block = (cachesim_cacheline *)(phy_way[indx][ret.way]);
    assert(block);

    /* Update epoch counter as defined in GSPC */
    if (info.fill == TRUE)
    {
#define EPOCH(phy_way_in, indx_in, way_in) \
      (((cachesim_cacheline *)(phy_way_in)[(indx_in)][(way_in)])->epoch)
#define CURRENT_STREAM(phy_way_in, indx_in, way_in) \
      (((cachesim_cacheline *)(phy_way_in)[(indx_in)][(way_in)])->stream)

      if (CURRENT_STREAM(phy_way, indx, ret.way) == info.stream)
      {
        EPOCH(phy_way, indx, ret.way) = EPOCH(phy_way, indx, ret.way) >= 3 ? 3 :
          EPOCH(phy_way, indx, ret.way) + 1;
      }
      else
      {
        EPOCH(phy_way, indx, ret.way) = 0;
      }

#undef EPOCH
#undef CURRENT_STREAM
    }

    assert(rdis >= last_rdis);

    /* Update accessed block BT status */
    ret.is_bt_block = block->is_bt_block;
    ret.is_ct_block = block->is_ct_block;

    ((cachesim_cacheline *)phy_way[indx][ret.way])->rdis    = rdis;
    ((cachesim_cacheline *)phy_way[indx][ret.way])->prdis   = last_rdis;
    ((cachesim_cacheline *)phy_way[indx][ret.way])->stream  = info.stream;
    ((cachesim_cacheline *)phy_way[indx][ret.way])->pc      = info.pc;
    ((cachesim_cacheline *)phy_way[indx][ret.way])->dirty   = (info.spill) ? 1 : 0;
    ((cachesim_cacheline *)phy_way[indx][ret.way])->rrpv    = CACHE_MAX_RRPV(cache) - CACHE_MAX_RRPV(cache);
    ((cachesim_cacheline *)phy_way[indx][ret.way])->op      = block_op_hit;
    ((cachesim_cacheline *)phy_way[indx][ret.way])->access += 1;

    /* If BT block is spilled update access and bt flag */
    if (info.spill == TRUE)
    {
      if (info.stream == BS)
      {
        block->is_bt_block = TRUE;
      }
      else
      {
        if (info.stream == CS && info.dbuf == FALSE)
        {
          block->is_ct_block = TRUE;
        }
      }
    }

    heapify(phy_way[indx], ret.way);

    /* Set per pc bit map */
    ub8         pc_seq_id;
    block_data *current_block_data;
    
    if (ret.fillpc)
    {
      /* Get stall data from pc_table */
      pc_table_itr = pc_table.find(ret.fillpc);

      /* Ensure pc is present */
      assert(pc_table_itr != pc_table.end());

      /* Obtain PC data */
      stall_data = (pc_data *)(pc_table_itr->second);

      /* Obtain block data */
      block_itr = (stall_data->blocks).find(ret.tag);
      assert(block_itr != (stall_data->blocks).end());

      current_block_data = (block_data *)(block_itr->second);
      assert(current_block_data);
      
      if (ret.fillpc != info.pc)
      {
        /* Insert new pc into pc sequence map */
        add_to_seq_map(&(stall_data->pc_map), info.pc);

        /* Set the bit corresponding to current PC in per-block bit-vector */
        pc_seq_id = get_id_from_seq_map(&(stall_data->pc_map), info.pc);

        if (pc_seq_id >= 64)
        {
          stall_data->overflow_count          += 1;
          current_block_data->overflow_count  += 1;
        }
        else
        {
          CACHE_BIS(current_block_data->pc_sharing_bitmap, pc_seq_id);
          CACHE_BIS(stall_data->pc_sharing_bitmap, pc_seq_id);
        }

        current_block_data->block_data.hit_count += 1;
      }
    }
  }

  gettimeofday(&et, NULL);

  return ret;
}

/* Given cache capacity and associativity obtain other cache parameters */
void set_cache_params(SimParameters *simp, struct cachesim_cache *cache)
{
  /*
    1. Cache lines  : capacity >> (bocksize in bits  * associativity)
    2. Index mask   : log(cachelines) 1s << block offset in number of bits
    3. Tag mask     : !(log(cachelines) 1s +  block offset in number of bits 1s)
    4. Tag shift    : log(cachelines + blockoffset)
  */
  ub8 cache_size;
  ub4 block_size;
  ub4 ways;
  ub4 max_rrpv;

  assert(cache);
  assert(simp);

  cache_size = simp->lcP.minCacheSize;
  block_size = simp->lcP.cacheBlockSize;
  ways       = simp->lcP.minCacheWays;
  max_rrpv   = simp->lcP.max_rrpv;

 #define CS_BYTE(cache_size) (cache_size * 1024)

  cout << "size "  << CS_BYTE(cache_size) << " block size " << block_size << " ways " << ways << "\n";

#define LOG_BASE_TWO(a) (log(a) / log(2))

  ub8 size_bits   = LOG_BASE_TWO(CS_BYTE(cache_size));
  ub8 block_bits  = LOG_BASE_TWO(block_size);
  ub8 way_bits    = (ways == 0) ? size_bits - block_bits : LOG_BASE_TWO(ways);
  ub8 line_count  = (((ub8)1) << (size_bits - way_bits - block_bits));
  ub8 index_bits  = (ways == 0) ? 0 : LOG_BASE_TWO(line_count);

#undef LOG_BASE_TWO

  /* Ensure cache size and block size if power of 2 */
  assert((((ub8)1) << size_bits) == CS_BYTE(cache_size));
  assert((((ub8)1) << block_bits) == block_size);
  
  /* Initialize cache parameters */
  CACHE_SIZE(cache)     = CS_BYTE(cache_size);
  CACHE_WAYS(cache)     = (ways > 0) ? ways : (1 << (size_bits - block_bits));
  CACHE_MAX_RRPV(cache) = max_rrpv;
  CACHE_BSIZE(cache)    = block_size;
  CACHE_LCNT(cache)     = line_count;
  CACHE_IMSK(cache)     = (ways == 0) ? 0 : (((ub8)(CACHE_LCNT(cache) - 1))) << block_bits;
  CACHE_TMSK(cache)     = ~((CACHE_IMSK(cache)) | (block_size - 1));
  CACHE_TSHF(cache)     = index_bits + block_bits;

#undef CS_BYTE

  printf("lines %d ways %d imask %lx tmask %lx tshft %ld %lx\n", CACHE_LCNT(cache), 
    CACHE_WAYS(cache), CACHE_IMSK(cache), CACHE_TMSK(cache), 
    CACHE_TSHF(cache), ((ub8)(CACHE_LCNT(cache) - 1)) << block_bits);
}

/* Given cache capacity and associativity obtain other cache parameters */
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

/* Close stats stream */
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

#define  updateEpochEvictCountersForRead(ret_in)              \
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

/* Update counter for all access */
#define updateEpochFillCounters(stream_in, epoch_in)         \
{                                                            \
  switch (stream_in)                                         \
  {                                                          \
    case ZS:                                                 \
                                                             \
      switch (epoch_in)                                      \
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
      switch (epoch_in)                                      \
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
      switch (epoch_in)                                      \
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

#define  updateEpochEvictCounters(ret_in)                     \
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

#define updateReuseDistance(info, rdis)                             \
{                                                                   \
  if ((info).stream >= PS && (info).stream <= PS + MAX_CORES - 1)   \
  {                                                                 \
    (p_rdis)->inc(rdis);                                            \
  }                                                                 \
  else                                                              \
  {                                                                 \
    switch ((info).stream)                                          \
    {                                                               \
      case IS:                                                      \
        (i_rdis)->inc(rdis);                                        \
        break;                                                      \
                                                                    \
      case CS:                                                      \
        (c_rdis)->inc(rdis);                                        \
        break;                                                      \
                                                                    \
      case ZS:                                                      \
        (z_rdis)->inc(rdis);                                        \
        break;                                                      \
                                                                    \
      case TS:                                                      \
        (t_rdis)->inc(rdis);                                        \
        break;                                                      \
                                                                    \
      case NS:                                                      \
        (n_rdis)->inc(rdis);                                        \
        break;                                                      \
                                                                    \
      case HS:                                                      \
        (h_rdis)->inc(rdis);                                        \
        break;                                                      \
                                                                    \
      case BS:                                                      \
        (b_rdis)->inc(rdis);                                        \
        break;                                                      \
                                                                    \
      case DS:                                                      \
        (d_rdis)->inc(rdis);                                        \
        break;                                                      \
                                                                    \
      case XS:                                                      \
        (x_rdis)->inc(rdis);                                        \
        break;                                                      \
                                                                    \
      default:                                                      \
        cout << "Illegal stream " << (int)((info).stream) << " for address ";     \
        cout << hex << (info).address << " type : ";                              \
        cout << dec << (int)(info).fill << " spill: " << dec << (int)(info).spill;\
        assert(0);                                                                \
    }                                                                             \
  }                                                                               \
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
  i_xcevct->setValue(0);
  i_xzevct->setValue(0);
  i_xtevct->setValue(0);
  i_xhit->setValue(0);
  i_rdis->setValue(0);

  c_access->setValue(0);
  c_dwrite->setValue(0);
  c_miss->setValue(0);
  c_raccess->setValue(0);
  c_rmiss->setValue(0);
  c_blocks->setValue(0);
  c_cprod->setValue(0);
  c_ccons->setValue(0);
  c_xevct->setValue(0);
  c_sevct->setValue(0);
  c_zevct->setValue(0);
  c_xzevct->setValue(0);
  c_xtevct->setValue(0);
  c_xhit->setValue(0);
  c_rdis->setValue(0);
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
  z_dwrite->setValue(0);
  z_raccess->setValue(0);
  z_rmiss->setValue(0);
  z_blocks->setValue(0);
  z_xevct->setValue(0);
  z_sevct->setValue(0);
  z_zevct->setValue(0);
  z_xcevct->setValue(0);
  z_xtevct->setValue(0);
  z_xhit->setValue(0);
  z_rdis->setValue(0);
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
  t_xcevct->setValue(0);
  t_xzevct->setValue(0);
  t_xhit->setValue(0);
  t_rdis->setValue(0);
  t_cthit->setValue(0);
  t_ctdhit->setValue(0);
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
  n_xcevct->setValue(0);
  n_xzevct->setValue(0);
  n_xtevct->setValue(0);
  n_xhit->setValue(0);
  n_rdis->setValue(0);

  h_access->setValue(0);
  h_miss->setValue(0);
  h_raccess->setValue(0);
  h_rmiss->setValue(0);
  h_blocks->setValue(0);
  h_xevct->setValue(0);
  h_sevct->setValue(0);
  h_zevct->setValue(0);
  h_xcevct->setValue(0);
  h_xzevct->setValue(0);
  h_xtevct->setValue(0);
  h_xhit->setValue(0);
  h_rdis->setValue(0);

  d_access->setValue(0);
  d_miss->setValue(0);
  d_raccess->setValue(0);
  d_rmiss->setValue(0);
  d_blocks->setValue(0);
  d_xevct->setValue(0);
  d_sevct->setValue(0);
  d_zevct->setValue(0);
  d_xcevct->setValue(0);
  d_xzevct->setValue(0);
  d_xtevct->setValue(0);
  d_xhit->setValue(0);
  d_rdis->setValue(0);
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
  b_xcevct->setValue(0);
  b_xzevct->setValue(0);
  b_xtevct->setValue(0);
  b_xhit->setValue(0);
  b_bprod->setValue(0);
  b_bcons->setValue(0);
  b_rdis->setValue(0);
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
  x_xcevct->setValue(0);
  x_xzevct->setValue(0);
  x_xtevct->setValue(0);
  x_xhit->setValue(0);
  x_rdis->setValue(0);

  p_access->setValue(0);
  p_miss->setValue(0);
  p_raccess->setValue(0);
  p_rmiss->setValue(0);
  p_blocks->setValue(0);
  p_xevct->setValue(0);
  p_sevct->setValue(0);
  p_zevct->setValue(0);
  p_xcevct->setValue(0);
  p_xzevct->setValue(0);
  p_xtevct->setValue(0);
  p_xhit->setValue(0);
  p_rdis->setValue(0);
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
  i_xcevct->setValue(0);
  i_xzevct->setValue(0);
  i_xtevct->setValue(0);
  i_xhit->setValue(0);
  i_rdis->setValue(0);

  c_access->setValue(0);
  c_dwrite->setValue(0);
  c_miss->setValue(0);
  c_raccess->setValue(0);
  c_rmiss->setValue(0);
  c_xevct->setValue(0);
  c_sevct->setValue(0);
  c_zevct->setValue(0);
  c_xzevct->setValue(0);
  c_xtevct->setValue(0);
  c_xhit->setValue(0);
  c_rdis->setValue(0);
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
  z_dwrite->setValue(0);
  z_raccess->setValue(0);
  z_rmiss->setValue(0);
  z_xevct->setValue(0);
  z_sevct->setValue(0);
  z_zevct->setValue(0);
  z_xcevct->setValue(0);
  z_xtevct->setValue(0);
  z_xhit->setValue(0);
  z_rdis->setValue(0);
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
  t_xcevct->setValue(0);
  t_xzevct->setValue(0);
  t_xhit->setValue(0);
  t_rdis->setValue(0);
  t_cthit->setValue(0);
  t_ctdhit->setValue(0);
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
  n_xcevct->setValue(0);
  n_xzevct->setValue(0);
  n_xtevct->setValue(0);
  n_xhit->setValue(0);
  n_rdis->setValue(0);

  h_access->setValue(0);
  h_miss->setValue(0);
  h_raccess->setValue(0);
  h_rmiss->setValue(0);
  h_xevct->setValue(0);
  h_sevct->setValue(0);
  h_zevct->setValue(0);
  h_xcevct->setValue(0);
  h_xzevct->setValue(0);
  h_xtevct->setValue(0);
  h_xhit->setValue(0);
  h_rdis->setValue(0);

  d_access->setValue(0);
  d_miss->setValue(0);
  d_raccess->setValue(0);
  d_rmiss->setValue(0);
  d_xevct->setValue(0);
  d_sevct->setValue(0);
  d_zevct->setValue(0);
  d_xcevct->setValue(0);
  d_xzevct->setValue(0);
  d_xtevct->setValue(0);
  d_xhit->setValue(0);
  d_rdis->setValue(0);
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
  b_xcevct->setValue(0);
  b_xzevct->setValue(0);
  b_xtevct->setValue(0);
  b_xhit->setValue(0);
  b_rdis->setValue(0);
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
  x_xcevct->setValue(0);
  x_xzevct->setValue(0);
  x_xtevct->setValue(0);
  x_xhit->setValue(0);
  x_rdis->setValue(0);

  p_access->setValue(0);
  p_miss->setValue(0);
  p_raccess->setValue(0);
  p_rmiss->setValue(0);
  p_xevct->setValue(0);
  p_sevct->setValue(0);
  p_zevct->setValue(0);
  p_xcevct->setValue(0);
  p_xzevct->setValue(0);
  p_xtevct->setValue(0);
  p_xhit->setValue(0);
  p_rdis->setValue(0);
}

void initStatistics()
{
  StatisticsManager *sm;

  /* Add statistics counter */
  sm = &StatisticsManager::instance(); 

  all_access  = &sm->getNumericStatistic<ub8>("CH_TotalAccess", ub8(0), "UC", NULL);  /* Total access */

  i_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "I");  /* Input stream access */
  i_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "I");    /* Input stream access */
  i_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "I"); /* Input stream access */
  i_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "I");   /* Input stream access */
  i_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "I");  /* Input stream access */
  i_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "I");  /* Input stream access */
  i_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "I");  /* Input stream access */
  i_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "I");  /* Input stream access */
  i_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "I"); /* Input stream access */
  i_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "I"); /* Input stream access */
  i_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "I"); /* Input stream access */
  i_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "I");    /* Input stream access */
  i_rdis      = &sm->getNumericStatistic<ub8>("CH_Rdis", ub8(0), "UC", "I");    /* Input stream access */

  c_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "C");  /* Input stream access */
  c_dwrite    = &sm->getNumericStatistic<ub8>("CH_DWrite", ub8(0), "UC", "C");  /* Input stream access */
  c_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "C");    /* Input stream access */
  c_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "C"); /* Input stream access */
  c_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "C");   /* Input stream access */
  c_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "C");  /* Input stream access */
  c_cprod     = &sm->getNumericStatistic<ub8>("CH_CProd", ub8(0), "UC", "C");   /* Input stream access */
  c_ccons     = &sm->getNumericStatistic<ub8>("CH_CCons", ub8(0), "UC", "C");   /* Input stream access */
  c_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "C");  /* Input stream access */
  c_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "C");  /* Input stream access */
  c_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "C");  /* Input stream access */
  c_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "C"); /* Input stream access */
  c_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "C"); /* Input stream access */
  c_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "C");    /* Input stream access */
  c_rdis      = &sm->getNumericStatistic<ub8>("CH_Rdis", ub8(0), "UC", "C");    /* Input stream access */
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
  c_re0fill   = &sm->getNumericStatistic<ub8>("CH_RE0fill", ub8(0), "UC", "C");/* Input stream access */
  c_re0hit    = &sm->getNumericStatistic<ub8>("CH_RE0hit", ub8(0), "UC", "C"); /* Input stream access */
  c_re1fill   = &sm->getNumericStatistic<ub8>("CH_RE1fill", ub8(0), "UC", "C");/* Input stream access */
  c_re1hit    = &sm->getNumericStatistic<ub8>("CH_RE1hit", ub8(0), "UC", "C"); /* Input stream access */
  c_re2fill   = &sm->getNumericStatistic<ub8>("CH_RE2fill", ub8(0), "UC", "C");/* Input stream access */
  c_re2hit    = &sm->getNumericStatistic<ub8>("CH_RE2hit", ub8(0), "UC", "C"); /* Input stream access */
  c_re3fill   = &sm->getNumericStatistic<ub8>("CH_RE3fill", ub8(0), "UC", "C");/* Input stream access */
  c_re3hit    = &sm->getNumericStatistic<ub8>("CH_RE3hit", ub8(0), "UC", "C"); /* Input stream access */
  c_re0evt    = &sm->getNumericStatistic<ub8>("CH_RE0evt", ub8(0), "UC", "C"); /* Input stream access */
  c_re1evt    = &sm->getNumericStatistic<ub8>("CH_RE1evt", ub8(0), "UC", "C"); /* Input stream access */
  c_re2evt    = &sm->getNumericStatistic<ub8>("CH_RE2evt", ub8(0), "UC", "C"); /* Input stream access */
  c_re3evt    = &sm->getNumericStatistic<ub8>("CH_RE3evt", ub8(0), "UC", "C"); /* Input stream access */

  z_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "Z");  /* Input stream access */
  z_dwrite    = &sm->getNumericStatistic<ub8>("CH_DWrite", ub8(0), "UC", "Z");  /* Input stream access */
  z_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "Z");    /* Input stream access */
  z_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "Z"); /* Input stream access */
  z_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "Z");   /* Input stream access */
  z_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "Z");  /* Input stream access */
  z_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "Z");  /* Input stream access */
  z_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "Z");  /* Input stream access */
  z_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "Z");  /* Input stream access */
  z_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "Z"); /* Input stream access */
  z_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "Z"); /* Input stream access */
  z_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "Z");    /* Input stream access */
  z_rdis      = &sm->getNumericStatistic<ub8>("CH_Rdis", ub8(0), "UC", "Z");    /* Input stream access */
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
  z_re0fill   = &sm->getNumericStatistic<ub8>("CH_RE0fill", ub8(0), "UC", "Z");/* Input stream access */
  z_re0hit    = &sm->getNumericStatistic<ub8>("CH_RE0hit", ub8(0), "UC", "Z"); /* Input stream access */
  z_re1fill   = &sm->getNumericStatistic<ub8>("CH_RE1fill", ub8(0), "UC", "Z");/* Input stream access */
  z_re1hit    = &sm->getNumericStatistic<ub8>("CH_RE1hit", ub8(0), "UC", "Z"); /* Input stream access */
  z_re2fill   = &sm->getNumericStatistic<ub8>("CH_RE2fill", ub8(0), "UC", "Z");/* Input stream access */
  z_re2hit    = &sm->getNumericStatistic<ub8>("CH_RE2hit", ub8(0), "UC", "Z"); /* Input stream access */
  z_re3fill   = &sm->getNumericStatistic<ub8>("CH_RE3fill", ub8(0), "UC", "Z");/* Input stream access */
  z_re3hit    = &sm->getNumericStatistic<ub8>("CH_RE3hit", ub8(0), "UC", "Z"); /* Input stream access */
  z_re0evt    = &sm->getNumericStatistic<ub8>("CH_RE0evt", ub8(0), "UC", "Z"); /* Input stream access */
  z_re1evt    = &sm->getNumericStatistic<ub8>("CH_RE1evt", ub8(0), "UC", "Z"); /* Input stream access */
  z_re2evt    = &sm->getNumericStatistic<ub8>("CH_RE2evt", ub8(0), "UC", "Z"); /* Input stream access */
  z_re3evt    = &sm->getNumericStatistic<ub8>("CH_RE3evt", ub8(0), "UC", "Z"); /* Input stream access */

  t_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "T");  /* Input stream access */
  t_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "T");    /* Input stream access */
  t_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "T"); /* Input stream access */
  t_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "T");   /* Input stream access */
  t_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "T");  /* Input stream access */
  t_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "T");  /* Input stream access */
  t_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "T");  /* Input stream access */
  t_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "T");  /* Input stream access */
  t_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "T"); /* Input stream access */
  t_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "T"); /* Input stream access */
  t_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "T");    /* Input stream access */
  t_rdis      = &sm->getNumericStatistic<ub8>("CH_Rdis", ub8(0), "UC", "T");    /* Input stream access */
  t_cthit     = &sm->getNumericStatistic<ub8>("CH_CThit", ub8(0), "UC", "T");   /* Input stream access */
  t_ctdhit    = &sm->getNumericStatistic<ub8>("CH_CTdhit", ub8(0), "UC", "T");  /* Input stream access */
  t_zthit     = &sm->getNumericStatistic<ub8>("CH_ZThit", ub8(0), "UC", "T");   /* Input stream access */
  t_bthit     = &sm->getNumericStatistic<ub8>("CH_BThit", ub8(0), "UC", "T");   /* Input stream access */
  t_e0fill    = &sm->getNumericStatistic<ub8>("CH_E0fill", ub8(0), "UC", "T");  /* Input stream access */
  t_e0hit     = &sm->getNumericStatistic<ub8>("CH_E0hit", ub8(0), "UC", "T");   /* Input stream access */
  t_e1fill    = &sm->getNumericStatistic<ub8>("CH_E1fill", ub8(0), "UC", "T");  /* Input stream access */
  t_e1hit     = &sm->getNumericStatistic<ub8>("CH_E1hit", ub8(0), "UC", "T");   /* Input stream access */
  t_e2fill    = &sm->getNumericStatistic<ub8>("CH_E2fill", ub8(0), "UC", "T");  /* Input stream access */
  t_e2hit     = &sm->getNumericStatistic<ub8>("CH_E2hit", ub8(0), "UC", "T");   /* Input stream access */
  t_e3fill    = &sm->getNumericStatistic<ub8>("CH_E3fill", ub8(0), "UC", "T");  /* Input stream access */
  t_e3hit     = &sm->getNumericStatistic<ub8>("CH_E3hit", ub8(0), "UC", "T");   /* Input stream access */
  t_e0evt     = &sm->getNumericStatistic<ub8>("CH_E0evt", ub8(0), "UC", "T");   /* Input stream access */
  t_e1evt     = &sm->getNumericStatistic<ub8>("CH_E1evt", ub8(0), "UC", "T");   /* Input stream access */
  t_e2evt     = &sm->getNumericStatistic<ub8>("CH_E2evt", ub8(0), "UC", "T");   /* Input stream access */
  t_e3evt     = &sm->getNumericStatistic<ub8>("CH_E3evt", ub8(0), "UC", "T");   /* Input stream access */
  t_re0fill   = &sm->getNumericStatistic<ub8>("CH_RE0fill", ub8(0), "UC", "T");/* Input stream access */
  t_re0hit    = &sm->getNumericStatistic<ub8>("CH_RE0hit", ub8(0), "UC", "T"); /* Input stream access */
  t_re1fill   = &sm->getNumericStatistic<ub8>("CH_RE1fill", ub8(0), "UC", "T");/* Input stream access */
  t_re1hit    = &sm->getNumericStatistic<ub8>("CH_RE1hit", ub8(0), "UC", "T"); /* Input stream access */
  t_re2fill   = &sm->getNumericStatistic<ub8>("CH_RE2fill", ub8(0), "UC", "T");/* Input stream access */
  t_re2hit    = &sm->getNumericStatistic<ub8>("CH_RE2hit", ub8(0), "UC", "T"); /* Input stream access */
  t_re3fill   = &sm->getNumericStatistic<ub8>("CH_RE3fill", ub8(0), "UC", "T");/* Input stream access */
  t_re3hit    = &sm->getNumericStatistic<ub8>("CH_RE3hit", ub8(0), "UC", "T"); /* Input stream access */
  t_re0evt    = &sm->getNumericStatistic<ub8>("CH_RE0evt", ub8(0), "UC", "T"); /* Input stream access */
  t_re1evt    = &sm->getNumericStatistic<ub8>("CH_RE1evt", ub8(0), "UC", "T"); /* Input stream access */
  t_re2evt    = &sm->getNumericStatistic<ub8>("CH_RE2evt", ub8(0), "UC", "T"); /* Input stream access */
  t_re3evt    = &sm->getNumericStatistic<ub8>("CH_RE3evt", ub8(0), "UC", "T"); /* Input stream access */
  t_btrdist   = &sm->getNumericStatistic<ub8>("CH_BTRdis", ub8(0), "UC", "T"); /* Input stream access */
  t_mxbtrdist = &sm->getNumericStatistic<ub8>("CH_BTMxrd", ub8(0), "UC", "T"); /* Input stream access */
  t_mnbtrdist = &sm->getNumericStatistic<ub8>("CH_BTMnrd", ub8(0), "UC", "T"); /* Input stream access */
  t_btblocks  = &sm->getNumericStatistic<ub8>("CH_BTBlocks", ub8(0), "UC", "T"); /* Input stream access */
  t_btuse     = &sm->getNumericStatistic<ub8>("CH_BTUse", ub8(0), "UC", "T"); /* Input stream access */
  t_btruse    = &sm->getNumericStatistic<ub8>("CH_BTRuse", ub8(0), "UC", "T"); /* Input stream access */
  t_mxbtruse  = &sm->getNumericStatistic<ub8>("CH_BTMaxruse", ub8(0), "UC", "T"); /* Input stream access */
  t_mnbtruse  = &sm->getNumericStatistic<ub8>("CH_BTMinruse", ub8(0), "UC", "T"); /* Input stream access */
  t_ctrdist   = &sm->getNumericStatistic<ub8>("CH_CTRdis", ub8(0), "UC", "T"); /* Input stream access */
  t_mxctrdist = &sm->getNumericStatistic<ub8>("CH_CTMxrd", ub8(0), "UC", "T"); /* Input stream access */
  t_mnctrdist = &sm->getNumericStatistic<ub8>("CH_CTMnrd", ub8(0), "UC", "T"); /* Input stream access */
  t_ctblocks  = &sm->getNumericStatistic<ub8>("CH_CTBlocks", ub8(0), "UC", "T"); /* Input stream access */
  t_ctuse     = &sm->getNumericStatistic<ub8>("CH_CTUse", ub8(0), "UC", "T"); /* Input stream access */
  t_ctruse    = &sm->getNumericStatistic<ub8>("CH_CTRuse", ub8(0), "UC", "T"); /* Input stream access */
  t_mxctruse  = &sm->getNumericStatistic<ub8>("CH_CTMaxruse", ub8(0), "UC", "T"); /* Input stream access */
  t_mnctruse  = &sm->getNumericStatistic<ub8>("CH_CTMinruse", ub8(0), "UC", "T"); /* Input stream access */

  n_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "N");  /* Input stream access */
  n_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "N");    /* Input stream access */
  n_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "N"); /* Input stream access */
  n_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "N");   /* Input stream access */
  n_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "N");  /* Input stream access */
  n_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "N");  /* Input stream access */
  n_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "N");  /* Input stream access */
  n_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "N");  /* Input stream access */
  n_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "N"); /* Input stream access */
  n_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "N"); /* Input stream access */
  n_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "N"); /* Input stream access */
  n_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "N");    /* Input stream access */
  n_rdis      = &sm->getNumericStatistic<ub8>("CH_Rdis", ub8(0), "UC", "N");    /* Input stream access */

  h_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "H");  /* Input stream access */
  h_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "H");    /* Input stream access */
  h_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "H"); /* Input stream access */
  h_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "H");   /* Input stream access */
  h_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "H");  /* Input stream access */
  h_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "H");  /* Input stream access */
  h_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "H");  /* Input stream access */
  h_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "H");  /* Input stream access */
  h_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "H"); /* Input stream access */
  h_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "H"); /* Input stream access */
  h_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "H"); /* Input stream access */
  h_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "H");    /* Input stream access */
  h_rdis      = &sm->getNumericStatistic<ub8>("CH_Rdis", ub8(0), "UC", "H");    /* Input stream access */

  d_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "D");  /* Input stream access */
  d_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "D");    /* Input stream access */
  d_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "D"); /* Input stream access */
  d_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "D");   /* Input stream access */
  d_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "D");  /* Input stream access */
  d_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "D");  /* Input stream access */
  d_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "D");  /* Input stream access */
  d_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "D");  /* Input stream access */
  d_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "D"); /* Input stream access */
  d_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "D"); /* Input stream access */
  d_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "D"); /* Input stream access */
  d_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "D");    /* Input stream access */
  d_rdis      = &sm->getNumericStatistic<ub8>("CH_Rdis", ub8(0), "UC", "D");    /* Input stream access */
  d_cdhit     = &sm->getNumericStatistic<ub8>("CH_CDhit", ub8(0), "UC", "D");   /* Input stream access */
  d_zdhit     = &sm->getNumericStatistic<ub8>("CH_ZDhit", ub8(0), "UC", "D");   /* Input stream access */
  d_tdhit     = &sm->getNumericStatistic<ub8>("CH_TDhit", ub8(0), "UC", "D");   /* Input stream access */
  d_bdhit     = &sm->getNumericStatistic<ub8>("CH_BDhit", ub8(0), "UC", "D");   /* Input stream access */

  b_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "B");  /* Input stream access */
  b_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "B");    /* Input stream access */
  b_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "B"); /* Input stream access */
  b_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "B");   /* Input stream access */
  b_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "B");  /* Input stream access */
  b_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "B");  /* Input stream access */
  b_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "B");  /* Input stream access */
  b_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "B");  /* Input stream access */
  b_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "B"); /* Input stream access */
  b_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "B"); /* Input stream access */
  b_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "B"); /* Input stream access */
  b_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "B");    /* Input stream access */
  b_bprod     = &sm->getNumericStatistic<ub8>("CH_BProd", ub8(0), "UC", "B");   /* Input stream access */
  b_bcons     = &sm->getNumericStatistic<ub8>("CH_BCons", ub8(0), "UC", "B");   /* Input stream access */
  b_rdis      = &sm->getNumericStatistic<ub8>("CH_Rdis", ub8(0), "UC", "B");    /* Input stream access */
  b_cbhit     = &sm->getNumericStatistic<ub8>("CH_CBhit", ub8(0), "UC", "B");   /* Input stream access */
  b_zbhit     = &sm->getNumericStatistic<ub8>("CH_ZBhit", ub8(0), "UC", "B");   /* Input stream access */
  b_tbhit     = &sm->getNumericStatistic<ub8>("CH_TBhit", ub8(0), "UC", "B");   /* Input stream access */

  x_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "X");  /* Input stream access */
  x_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "X");    /* Input stream access */
  x_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "X"); /* Input stream access */
  x_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "X");   /* Input stream access */
  x_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "X");  /* Input stream access */
  x_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "X");  /* Input stream access */
  x_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "X");  /* Input stream access */
  x_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "X");  /* Input stream access */
  x_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "X"); /* Input stream access */
  x_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "X"); /* Input stream access */
  x_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "X"); /* Input stream access */
  x_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "X");    /* Input stream access */
  x_rdis      = &sm->getNumericStatistic<ub8>("CH_Rdis", ub8(0), "UC", "X");    /* Input stream access */

  p_access    = &sm->getNumericStatistic<ub8>("CH_Access", ub8(0), "UC", "P");  /* Input stream access */
  p_miss      = &sm->getNumericStatistic<ub8>("CH_Miss", ub8(0), "UC", "P");    /* Input stream access */
  p_raccess   = &sm->getNumericStatistic<ub8>("CH_RAccess", ub8(0), "UC", "P"); /* Input stream access */
  p_rmiss     = &sm->getNumericStatistic<ub8>("CH_RMiss", ub8(0), "UC", "P");   /* Input stream access */
  p_blocks    = &sm->getNumericStatistic<ub8>("CH_Blocks", ub8(0), "UC", "P");  /* Input stream access */
  p_xevct     = &sm->getNumericStatistic<ub8>("CH_Xevict", ub8(0), "UC", "P");  /* Input stream access */
  p_sevct     = &sm->getNumericStatistic<ub8>("CH_Sevict", ub8(0), "UC", "P");  /* Input stream access */
  p_zevct     = &sm->getNumericStatistic<ub8>("CH_Zevict", ub8(0), "UC", "P");  /* Input stream access */
  p_xcevct    = &sm->getNumericStatistic<ub8>("CH_XCevict", ub8(0), "UC", "P"); /* Input stream access */
  p_xzevct    = &sm->getNumericStatistic<ub8>("CH_XZevict", ub8(0), "UC", "P"); /* Input stream access */
  p_xtevct    = &sm->getNumericStatistic<ub8>("CH_XTevict", ub8(0), "UC", "P"); /* Input stream access */
  p_xhit      = &sm->getNumericStatistic<ub8>("CH_Xhit", ub8(0), "UC", "P");    /* Input stream access */
  p_rdis      = &sm->getNumericStatistic<ub8>("CH_Rdis", ub8(0), "UC", "P");    /* Input stream access */

  resetStatistics();
}


/* Simulation start */
int initParams(int argc, char **argv, SimParameters *sim_params, sb1 **trc_file_name)
{
  /* Cache configuration parser */
  sb1 *config_file_name;
  sb1  opt;

  ConfigLoader  *cfg_loader;

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
        *trc_file_name = (sb1 *) malloc(strlen(optarg) + 1);
        strncpy(*trc_file_name, optarg, strlen(optarg));
        printf("t:%s\n", *trc_file_name);
        break;

      default:
        printf("Invalid option %c\n", opt);
        exit(EXIT_FAILURE);
    }
  }

  /* Instantiate configuration loader and read cache params */
  cfg_loader = new ConfigLoader(config_file_name);
  cfg_loader->getParameters(sim_params);

  return 0;
}


/* Simulation start */
int main(int argc, char **argv)
{
  ub8  rdis;
  ub8  inscnt;
  ub8  address;
  ub8  bt_blocks;
  ub8  bt_access;
  ub8  fills[TST];
  ub8  blocks[TST];
  ub8  hits[TST];
  ub8  replacement[TST];
  ub8  total_access;
  sb1  *trc_file_name;

  gzifstream    trc_strm;
  gzofstream    stats_stream;
  memory_trace  info;

  memory_trace_in_file  trace_info;

  struct timeval      st;
  struct timeval      et;
  struct cache_params params;
  SimParameters       sim_params;
  StatisticsManager  *sm;

  cachesim_cache l2cache, l3cache;

  /* Verify bit-ness */
  assert(sizeof(ub8) == 8 && sizeof(ub4) == 4);

  /* Clear per-stream fill and hit arrays */
  CLRSTRUCT(fills); 
  CLRSTRUCT(hits); 
  CLRSTRUCT(blocks); 
  CLRSTRUCT(replacement); 

  /* Initialize simulation params */
  initParams(argc, argv, &sim_params, &trc_file_name);

  /* Initialize statistics */
  initStatistics();

  sm = &StatisticsManager::instance(); 

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
  }

  /* Open statistics stream */
  openStatisticsStream(&sim_params, stats_stream);

  CLRSTRUCT(l2cache); 
  CLRSTRUCT(l3cache); 

  inscnt        = 0;
  bt_blocks     = 0;
  bt_access     = 0;
  inscnt        = 0;
  total_access  = 0;

  assert(sim_params.lcP.minCacheSize == sim_params.lcP.maxCacheSize);
  assert(sim_params.lcP.minCacheWays == sim_params.lcP.maxCacheWays);

  set_cache_params(&sim_params, &l3cache);

  params.stream = sim_params.lcP.stream;

  printf("L3 ways %d L3 size %d L3 cache lines %d\n", 
      CACHE_WAYS(&l3cache), CACHE_SIZE(&l3cache) >> 10, CACHE_LCNT(&l3cache));

  phy_way = new vector <cachesim_cacheline *>[CACHE_LCNT(&l3cache)];

  for (ub8 i = 0; i < CACHE_LCNT(&l3cache); i++) 
  {
    phy_way[i].resize(CACHE_WAYS(&l3cache), NULL);
  }

  /* Create lookup map for each set of the cache */
  cache_set = new map <ub8, ub8>[CACHE_LCNT(&l3cache)];
  assert(cache_set);

  /* Create sequence number array for each set */
  sn_set    = new ub8[CACHE_LCNT(&l3cache)];
  assert(sn_set);

  for (ub8 i = 0; i < CACHE_LCNT(&l3cache); i++) 
  {
    for (ub8 k = 0; k < CACHE_WAYS(&l3cache); k++) 
    {
      cachesim_cacheline *cache_line;

      cache_line = new cachesim_cacheline;
      assert(cache_line);
      CLRSTRUCT(*cache_line);

      cache_line->rdis  = ~((ub8)0);
      cache_line->way   = k;
      phy_way[i][k]     = cache_line;
    }
  }

  /* Set sequence number to 0 */
  memset(sn_set, 0, sizeof(ub8) * CACHE_LCNT(&l3cache));

  /* Initialize global sequence number counter */
  sn_glbl = 0;

  /* Create ways to keep reuse distance */
  CACHE_AFRQ(&l3cache) = new ub8[CACHE_LCNT(&l3cache)];
  CLRSTRUCT(*CACHE_AFRQ(&l3cache));

  /* Create ways to keep reuse distance */
  CACHE_EVCT(&l3cache) = new ub8[CACHE_LCNT(&l3cache)];
  CLRSTRUCT(*CACHE_EVCT(&l3cache));

  /* Set next and previous level of hierarchy */
  CACHE_PLVL(&l3cache) = NULL;
  CACHE_NLVL(&l3cache) = NULL;

  /* Creates hash table / map */
  hthead_info = (ub8 *)malloc(sizeof(ub8) * HTBLSIZE);
  if (! hthead_info)
  {
    perror("allocating hthead_info failed ");
    assert(hthead_info);
  }

  /* Initialize info */
  memset(hthead_info, 0, sizeof(sizeof(ub8) * HTBLSIZE));

  htmap = new map<ub8, ub8>[HTBLSIZE];
  assert(htmap);

  /* Build map to get reuse distance */
  cachesim_createmap(&l3cache, trc_file_name, &params);

  for (ub8 i = 0; i < FRAGS; i++)
  {
    /* Open trace file stream */
    trc_strm.open(trc_file_name, ios::in | ios::binary);
    assert (trc_strm.is_open());

    /* Start callback */
    if (USE_INTER_STREAM_CALLBACK)
    {
      reuse_ct_cbk->StartCbk();
      reuse_cc_cbk->StartCbk();
      reuse_bt_cbk->StartCbk();
      reuse_bb_cbk->StartCbk();
      reuse_zt_cbk->StartCbk();
      reuse_zz_cbk->StartCbk();
      reuse_cb_cbk->StartCbk();
      reuse_tt_cbk->StartCbk();
    }

    /* Read entire trace file and run the simulation */
    while (!trc_strm.eof())
    {
#if 0
      trc_strm.read((char *)&trace_info, sizeof(memory_trace_in_file));
#endif
      trc_strm.read((char *)&info, sizeof(memory_trace));
      if (!trc_strm.eof())
      {
#if 0
        info.address  = trace_info.address;
        info.stream   = trace_info.stream;
        info.fill     = trace_info.fill;
        info.spill    = trace_info.spill;
#endif

#define MAX_CORES (128)

        if (params.stream == NN || ((params.stream == OS) && 
              (info.stream == XS || info.stream == DS ||
               info.stream == BS || info.stream == NS ||
               info.stream == HS)) || params.stream == info.stream)
        {
          cachesim_qnode    *list_head;
          cache_access_info *access_info;

          address = info.address;

          access_info = cachesim_get_access_info(info.address);
          assert(access_info);

          /* Get address list head out of access info */
          list_head = (cachesim_qnode *)(access_info->list_head);
          assert(list_head);

          /* Get reuse distance */
          rdis = cachesim_getreusedistance(list_head, address);

          /* Make a cache access */
          cache_access_status ret = 
            cachesim_incl_cache(&l3cache, address, info, rdis);

          /* Update hit counter */
          if (ret.fate == CACHE_ACCESS_HIT)
          {
            hits[info.stream]++;
          }

          /* For blitter color and depth measure reuse distance on fill */
          if (info.stream == BS || info.stream == CS || info.stream == ZS ||
              (info.stream >= PS && info.stream <= PS + MAX_CORES - 1))
          {
#define CT_BLOCK(info)  (info.stream == CS && info.dbuf == FALSE)
#define BT_BLOCK(info)  (info.stream == BS)

            /* For all CPU cores use PS as stream index */
            ub1 stream_indx = (info.stream >= PS) ? PS : info.stream;

            /* If not the first access */
            if (access_info->first_seq != 0 || access_info->last_seq != 0)
            {
              assert(last_rdis != (ub8)(~(0)) && access_info->last_seq >= access_info->first_seq);

              /* Get difference of sequence numbers (intervening accesses) */
              ub8 index = access_info->last_seq - access_info->first_seq;

              /* Clamp index to MAX_RDIS */
              index = (index < MAX_RDIS) ? index : MAX_RDIS; 
              rdis_hist[stream_indx][index][0] += 1;
              rdis_hist[stream_indx][index][1] += access_info->access_count;

              /* Clamp reuse count to max reuse tracked */
              index = (access_info->access_count < MAX_REUSE) ?  access_info->access_count : MAX_REUSE;
              reuse_hist[stream_indx][index] += 1;
            }
            else
            {
#define NOREUSE (0)

              access_info->stream       = info.stream;
              access_info->access_count = NOREUSE;
              access_info->first_seq    = last_rdis;
              access_info->last_seq     = last_rdis;

#undef NOREUSE
            }

            if (access_info->is_ct_block == FALSE && info.stream == CS && 
                info.dbuf == FALSE)
            {
              (*t_ctblocks)++;
            }

            if (access_info->is_bt_block == FALSE && info.stream == BS)
            {
              (*t_btblocks)++;
            }

            access_info->is_ct_block  = CT_BLOCK(info) ? TRUE : FALSE;
            access_info->is_bt_block  = (info.stream == BS) ? TRUE : FALSE;

#undef CT_BLOCK
          }

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
                    assert(0);
                }
              }

              if (ret.fate == CACHE_ACCESS_RPLC)
              { 
                /* Update C, Z, T epoch counters */
                if (ret.stream == CS || ret.stream == ZS || ret.stream == TS)
                {
                  updateEpochEvictCountersForRead(ret);
                }
#if 0
                /* For blitter color and depth measure reuse distance on fill */
                if (info.stream == BS || info.stream == CS || info.stream == ZS
                    || info.stream == TS || (info.stream >= PS && info.stream <= PS + MAX_CORES - 1))
                {
                  cache_access_info *access_info = cachesim_get_access_info(info.address);
                  assert(access_info);

                  access_info->stream       = info.stream;
                  access_info->is_bt_block  = FALSE;
                  access_info->is_ct_block  = FALSE;
                  access_info->access_count = 0;
                  access_info->first_seq    = 0;
                  access_info->last_seq     = 0;
                }
#endif
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

              /* Update reuse distance statistics */
              if (info.stream == TS && 
                  (ret.is_bt_block == TRUE || ret.is_ct_block == TRUE))
              {
                ub8 rdis_diff;

                assert(ret.rdis != ~(0) && ret.prdis != ~(0));

                rdis_diff = ret.rdis - ret.prdis;
                updateReuseDistance(info, rdis_diff);

                /* If this is a BT transition */
                if (ret.stream == BS)
                {
                  access_info->btuse = TRUE;
#if 0
                  printf("Set BT rdis to %d\n", rdis_diff);
#endif

                  /* For BT transition increment total reuse distance */
                  t_btrdist->inc(rdis_diff);

                  /* Update max reuse distance */
                  if (t_mxbtrdist->getValue(1) < rdis_diff)
                  {
                    t_mxbtrdist->setValue(rdis_diff);
                  }

                  /* Update min reuse distance */
                  if (t_mnbtrdist->getValue(1) > rdis_diff)
                  {
                    t_mnbtrdist->setValue(rdis_diff);
                  }
                }

                /* If this is a BT transition */
                if (ret.stream == CS)
                {
                  access_info->btuse = TRUE;
#if 0
                  printf("Set CT rdis to %d\n", rdis_diff);
#endif

                  /* For BT transition increment total reuse distance */
                  t_ctrdist->inc(rdis_diff);

                  /* Update max reuse distance */
                  if (t_mxctrdist->getValue(1) < rdis_diff)
                  {
                    t_mxctrdist->setValue(rdis_diff);
                  }

                  /* Update min reuse distance */
                  if (t_mnctrdist->getValue(1) > rdis_diff)
                  {
                    t_mnctrdist->setValue(rdis_diff);
                  }
                }
              }

              /* Update access info of the block */
              access_info->last_seq = ret.rdis;
            }

            if (info.stream == CS || info.stream == ZS || info.stream == TS)
            {
              /* If epoch has changed in this access */
              if(ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
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
            blocks[info.stream]++;

            /* Update fill counter */
            fills[info.stream]++;

            if (info.stream >= PS && info.stream <= PS + MAX_CORES - 1)
            {
              (*p_miss)++;
              (*p_blocks)++;
            }
            else
            {
              switch (info.stream)
              {
                case IS:
                  (*i_miss)++;
                  (*i_blocks)++;
                  break;

                case CS:
                  (*c_miss)++;
                  (*c_blocks)++;
                  break;

                case ZS:
                  (*z_miss)++;
                  (*z_blocks)++;
                  break;

                case TS:
                  (*t_miss)++;
                  (*t_blocks)++;
                  break;

                case NS:
                  (*n_miss)++;
                  (*n_blocks)++;
                  break;

                case HS:
                  (*h_miss)++;
                  (*h_blocks)++;
                  break;

                case BS:
                  (*b_miss)++;
                  (*b_blocks)++;
                  break;

                case DS:
                  (*d_miss)++;
                  (*d_blocks)++;

                  break;

                case XS:
                  (*x_miss)++;
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
              assert(blocks[ret.stream] > 0);
              blocks[ret.stream]--;

              /* Update replacement counter */
              replacement[ret.stream]++;

              /* Update C, Z, T epoch counters */
              if (ret.stream == CS || ret.stream == ZS || ret.stream == TS)
              {
                updateEpochEvictCounters(ret);
              }

              /* Current stream has a block allocated */
              if (ret.stream != NN && ret.stream != info.stream)
              {
                if (ret.stream >= PS && ret.stream <= PS + MAX_CORES - 1)
                {
                  (*p_xevct)++;

                  (*p_blocks)--;
                }
                else
                {
                  switch(ret.stream)
                  {
                    case IS:
                      (*i_xevct)++;
                      (*i_blocks)--;

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
                        else
                        {
                          if (info.stream == TS) 
                          {
                            (*i_xtevct)++;
                          }
                        }
                      }

                      break;

                    case CS:
                      (*c_xevct)++;
                      (*c_blocks)--;

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
                      }

                      break;

                    case ZS:
                      (*z_xevct)++;
                      (*z_blocks)--;

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
                      }

                      break;

                    case TS:
                      (*t_xevct)++;
                      (*t_blocks)--;

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
                      }

                      break;

                    case NS:
                      (*n_xevct)++;
                      (*n_blocks)--;

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
                        }
                      }

                      break;

                    case HS:
                      (*h_xevct)++;
                      (*h_blocks)--;

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
                        }
                      }

                      break;

                    case BS:
                      (*b_xevct)++;
                      (*b_blocks)--;

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
                        }
                      }

                      break;

                    case DS:
                      (*d_xevct)++;
                      (*d_blocks)--;

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
                        }
                      }

                      break;

                    case XS:
                      (*x_xevct)++;
                      (*x_blocks)--;

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
                        }
                      }

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
                  (*p_blocks)--;
                }
                else
                {
                  switch(info.stream)
                  {
                    case IS:
                      (*i_sevct)++;
                      (*i_blocks)--;
                      break;

                    case CS:
                      (*c_sevct)++;
                      (*c_blocks)--;
                      break;

                    case ZS:
                      (*z_sevct)++;
                      (*z_blocks)--;
                      break;

                    case TS:
                      (*t_sevct)++;
                      (*t_blocks)--;
                      break;

                    case NS:
                      (*n_sevct)++;
                      (*n_blocks)--;
                      break;

                    case HS:
                      (*h_sevct)++;
                      (*h_blocks)--;
                      break;

                    case BS:
                      (*b_sevct)++;
                      (*b_blocks)--;
                      break;

                    case DS:
                      (*d_sevct)++;
                      (*d_blocks)--;
                      break;

                    case XS:
                      (*x_sevct)++;
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

              /* If return block was never accessed */
              if (ret.access == 0)
              {
                switch (ret.stream)
                {
                  case IS:
                    (*i_zevct)++;
                    break;

                  case CS:
                    (*c_zevct)++;
                    break;

                  case ZS:
                    (*z_zevct)++;
                    break;

                  case TS:
                    (*t_zevct)++;
                    break;

                  case NS:
                    (*n_zevct)++;
                    break;

                  case HS:
                    (*h_zevct)++;
                    break;

                  case BS:
                    (*b_zevct)++;
                    break;

                  case DS:
                    (*d_zevct)++;
                    break;

                  case XS:
                    (*x_zevct)++;
                    break;

                  default:
                    cout << "Illegal stream " << (int)(ret.stream);
                    assert(1);
                }
              }

              if ((ret.is_bt_block == TRUE  || ret.is_ct_block == TRUE) &&
                  (ret.stream == TS || ret.stream == CS))
              {

                if (ret.is_bt_block)
                {
                  bt_blocks += 1;
                  bt_access += ret.access;

                  /* Update BT reuse statistics */
                  t_btruse->inc(ret.access);

                  if (access_info->btuse == FALSE)
                  {
                    (*t_btuse)++;
                    access_info->btuse = TRUE;
                  }
#if 0
                  printf("Set BT reuse to %d\n", ret.access);
#endif
                  if (t_mxbtruse->getValue(1) < ret.access)
                  {
                    t_mxbtruse->setValue(ret.access);
                  }

                  if (t_mnbtruse->getValue(1) > ret.access)
                  {
                    t_mnbtruse->setValue(ret.access);
                  }
                }
                else
                {
                  if (ret.is_ct_block)
                  {
                    /* Update CT reuse statistics */
                    t_ctruse->inc(ret.access);

                    if (access_info->ctuse == FALSE)
                    {
                      (*t_ctuse)++;
                      access_info->ctuse = TRUE;
                    }
#if 0
                    printf("Set CT reuse to %d\n", ret.access);
#endif

                    if (t_mxctruse->getValue(1) < ret.access)
                    {
                      t_mxctruse->setValue(ret.access);
                    }

                    if (t_mnctruse->getValue(1) > ret.access)
                    {
                      t_mnctruse->setValue(ret.access);
                    }
                  }
                }
              }

              /* For blitter, color and depth measure reuse distance on fill */
#if 0
              if (ret.stream == BS || ret.stream == CS || ret.stream == ZS || ret.stream == TS || 
                  (ret.stream == PS && ret.stream <= PS + MAX_CORES - 1))
#endif
              {
                ub8 index;         /* Histogram bucket index */
                ub1 stream_indx;   /* Stream number */

                cache_access_info *ret_access_info = cachesim_get_access_info(ret.tag);
                assert(ret_access_info);
                assert(ret.access == ret_access_info->access_count);
                assert(last_rdis != (ub8)(~(0)) && ret_access_info->last_seq >= ret_access_info->first_seq);

                index       = ret_access_info->last_seq - ret_access_info->first_seq;
                index       = (index < MAX_RDIS) ? index : MAX_RDIS; 
                stream_indx = (ret.stream >= PS) ? PS : ret.stream;

                /* Update Reuse distance histogram */
                rdis_hist[stream_indx][index][0] += 1;
                rdis_hist[stream_indx][index][1] += ret_access_info->access_count;

                /* Update reuse count histogram */
                index = (ret_access_info->access_count < MAX_REUSE) ? ret_access_info->access_count : MAX_REUSE;
                reuse_hist[stream_indx][index] += 1;

#define NOACCESS  (0)
#define NOSEQ     (0)
                ret_access_info->stream       = NN;
                ret_access_info->is_bt_block  = FALSE;
                ret_access_info->is_ct_block  = FALSE;
                ret_access_info->access_count = NOACCESS;
                ret_access_info->first_seq    = NOSEQ;
                ret_access_info->last_seq     = NOSEQ;
#undef NOACCESS
#undef NOSEQ
              }
            }
          }
          else
          {
            assert(ret.fate == CACHE_ACCESS_HIT);

            (access_info->access_count)++;

            /* Update C, Z, T epoch counters */
            if (info.stream == ret.stream && 
                (ret.stream == CS || ret.stream == ZS || ret.stream == TS))
            {
              updateEpochHitCounters(ret);
            }

            /* Update stats for incoming stream */
            if (info.stream != ret.stream)
            {
              assert(blocks[ret.stream] > 0);
              blocks[ret.stream]--;
              blocks[info.stream]++;

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

                        if (ret.is_ct_block == TRUE)
                        {
                          (*t_ctdhit)++;
                        }

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

                      case ZS:
                        (*b_zbhit)++;
                        break;

                      case TS:
                        (*b_tbhit)++;
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

                      case ZS:
                        (*d_zdhit)++;
                        break;

                      case TS:
                        (*d_tdhit)++;
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
                (*p_blocks)--;
              }
              else
              {
                switch (ret.stream)
                {
                  case IS:
                    (*i_blocks)--;
                    break;

                  case CS:
                    (*c_blocks)--;
                    break;

                  case ZS:
                    (*z_blocks)--;
                    break;

                  case TS:
                    (*t_blocks)--;
                    break;

                  case NS:
                    (*n_blocks)--;
                    break;

                  case HS:
                    (*h_blocks)--;
                    break;

                  case BS:
                    (*b_blocks)--;
                    break;

                  case DS:
                    (*d_blocks)--;
                    break;

                  case XS:
                    (*x_blocks)--;
                    break;

                  default:
                    cout << "Illegal stream " << (int)info.stream << " for address ";
                    cout << hex << info.address << " type : ";
                    cout << dec << (int)info.fill << " spill: " << dec << (int)info.spill;
                    assert(0);
                }
              }
            }

            /* Update transition info */
            ub8 rrpv_trans_indx;
            ub1 stream_indx;

            access_info = cachesim_get_access_info(BLCKALIGN(info.address));
            assert(access_info);

            rrpv_trans_indx = (access_info->rrpv_trans < MAX_TRANS) ? 
              access_info->rrpv_trans : MAX_TRANS;
            stream_indx     = (info.stream >= PS) ? PS : info.stream;

            rrpv_trans_hist[stream_indx][rrpv_trans_indx] += 1;
            access_info->rrpv_trans = 0;
          }

          if (info.stream == CS || info.stream == ZS || info.stream == TS)
          {
            /* If epoch has changed in this access */
            if(ret.fate == CACHE_ACCESS_MISS || ret.fate == CACHE_ACCESS_RPLC)
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

                if (info.spill == TRUE && info.dbuf == FALSE)
                {
                  (*c_dwrite)++;
                }
                break;

              case ZS:
                (*z_access)++;
                if (info.spill == TRUE && info.dbuf == FALSE)
                {
                  (*z_dwrite)++;
                }
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
#define CS_KBYTES(size) (size >> 10)

          /* Dump names only once */
          if (!inscnt)
            sm->dumpNames("CacheSize", CH, stats_stream);

          (*all_access)++;
          inscnt++;

          if (!(inscnt % 10000))
          {
            cout << ".";
            std::cout.flush();

            /* Dump values every 1M instruction */
            sm->dumpValues(CS_KBYTES(CACHE_SIZE(&l3cache)), 0, CH, stats_stream);

            periodicReset();
          }

#undef CS_KBYTES
#undef CH
          if (++total_access >= PHASE_SIZE)
          {
#if 0
            printf("CF:%ld ZF:%ld TF:%ld IF:%ld BF:%ld\n", fills[CS], 
                fills[ZS], fills[TS], fills[IS], fills[BS]);
            printf("CH:%ld ZH:%ld TH:%ld IH:%ld BH:%ld\n", hits[CS], 
                hits[ZS], hits[TS], hits[IS], hits[BS]);
            printf("CR:%ld ZR:%ld TR:%ld IR:%ld BR:%ld\n", replacement[CS],
                replacement[ZS], replacement[TS], replacement[IS], replacement[BS]);
            printf("CB:%ld ZB:%ld TB:%ld IB:%ld BB:%ld\n", blocks[CS], 
                blocks[ZS], blocks[TS], blocks[IS], blocks[BS]);
#endif
            total_access = 0;  
          }
        }
      }
      else
      {
        cout << "\n Simulation done!" << endl;
      }

      sn_glbl++;
    }

#define CH (4)
#define CS_KBYTES(size) (size >> 10)

    sm->dumpValues(CS_KBYTES(CACHE_SIZE(&l3cache)), 0, CH, stats_stream);

#undef CS_KBYTES
#undef CH

    trc_strm.close();

    /* Start callback */
    if (USE_INTER_STREAM_CALLBACK)
    {
      reuse_ct_cbk->ExitCbk();
      reuse_cc_cbk->ExitCbk();
      reuse_bt_cbk->ExitCbk();
      reuse_bb_cbk->ExitCbk();
      reuse_zt_cbk->ExitCbk();
      reuse_zz_cbk->ExitCbk();
      reuse_cb_cbk->ExitCbk();
      reuse_tt_cbk->ExitCbk();
    }

    /* Free InterStreamReuse object */
    if (USE_INTER_STREAM_CALLBACK)
    {
      delete reuse_ct_cbk;
      delete reuse_cc_cbk;
      delete reuse_bt_cbk;
      delete reuse_bb_cbk;
      delete reuse_zt_cbk;
      delete reuse_zz_cbk;
      delete reuse_cb_cbk;
      delete reuse_tt_cbk;
    }

    /* Close statistics stream */
    closeStatisticsStream(stats_stream);
  }

  ub8 access_count_avg;
  ub8 total_bt_access;
  ub8 total_rdis;
  ub8 min_bt_access;
  ub8 max_bt_access;
  ub8 rrpv_trans_indx;

  access_count_avg  = 0;
  total_bt_access   = 0;
  total_rdis        = 0;
  min_bt_access     = 0xffffffff;
  max_bt_access     = 0;

  /* Deallocate all cache_access_info */
  for (htmap_itr = htmap->begin(); htmap_itr != htmap->end(); htmap_itr++)
  {
    cache_access_info *access_info = (cache_access_info *)(htmap_itr->second);

    if (access_info->is_bt_block == TRUE && access_info->access_count != 0)
    {
      access_count_avg += access_info->access_count;
      total_bt_access  += 1;
      total_rdis       += access_info->last_seq - access_info->first_seq;

      if (access_info->access_count < min_bt_access)
      {
        min_bt_access = access_info->access_count;
      }

      if (access_info->access_count > max_bt_access)
      {
        max_bt_access = access_info->access_count;
      }
    }

#if 0    
    rrpv_trans_indx = (access_info->rrpv_trans < MAX_TRANS) ? access_info->rrpv_trans : MAX_TRANS;
    rrpv_trans_hist[rrpv_trans_indx] += 1;
#endif

    delete (cachesim_qnode *)(access_info->list_head);
    delete access_info;
  }

  if (total_bt_access)
  {
    cout << "Avg accesses / address " << access_count_avg / total_bt_access << endl;
  }

  cout << "Min accesses / address " << min_bt_access << endl;
  cout << "Max accesses / address " << max_bt_access << endl;

  if (total_bt_access)
  {
    cout << "Avg rdis" << total_rdis / total_bt_access << endl;
  }

  /* Deallocate cache ways */
  for (ub8 i = 0; i < CACHE_LCNT(&l3cache); i++)
  {
    cache_set[i].clear();
  }

  /* Free all memory allocated for cache blocks */
  for (ub8 i = 0; i < CACHE_LCNT(&l3cache); i++)
  {
    for (ub8 k = 0; k < phy_way[i].size(); k++)
    {
      delete phy_way[i][k];
    }
  }

  /* Delete cache tag array */ 
  delete []phy_way;

  cachesim_fini();

  /* Deallocate all pc_table data */
  for (pc_table_itr = pc_table.begin(); pc_table_itr != pc_table.end(); pc_table_itr++)
  {
    pc_data *stall_data = (pc_data *)(pc_table_itr->second);
    delete stall_data;
  }

  return 0;
}

void dump_pc_table()
{
  /* Dump Reuse stats in file */
  gzofstream   out_stream;
  char         tracefile_name[100];

  assert(strlen("PC-L2-stats.trace.csv.gz") + 1 < 100);
  sprintf(tracefile_name, "PC-L2-stats.trace.csv.gz");
  out_stream.open(tracefile_name, ios::out | ios::binary);

  if (!out_stream.is_open())
  {
    printf("Error opening output stream\n");
    exit(EXIT_FAILURE);
  }
  
  printf("Dumping PC Table %s \n", tracefile_name);

  out_stream << "PC; LLCACCESS; LLCMISS; BLOCKS; SHARINGPCS; UNIQUEBITMAPS; PCBITMAP; OVERFLOW; FILLOP; HITOP" << endl;

  pc_data *stall_data;

  for (pc_table_itr = pc_table.begin(); pc_table_itr != pc_table.end(); pc_table_itr++)
  {
    stall_data = (pc_data *)(pc_table_itr->second);
    out_stream << stall_data->pc << ";" <<  stall_data->stat_access_llc << ";" << stall_data->stat_miss_llc 
      << ";" << (stall_data->blocks).size() << ";" << (stall_data->pc_map.map_seq).size() << ";" 
      << (stall_data->bitmaps).size() << ";" << stall_data->pc_sharing_bitmap << ";" 
      << stall_data->overflow_count << ";" << stall_data->fill_op << ";" << stall_data->hit_op << endl;
  }

  out_stream.close();

  assert(strlen("PC-LASTPC-stats.trace.csv.gz") + 1 < 100);
  sprintf(tracefile_name, "PC-LASTPC-stats.trace.csv.gz");
  out_stream.open(tracefile_name, ios::out | ios::binary);

  if (!out_stream.is_open())
  {
    printf("Error opening output stream\n");
    exit(EXIT_FAILURE);
  }
  
  printf("Dumping LASTPC Table %s \n", tracefile_name);

  out_stream << "PC; LASTPC; ACCESS; FILLOP; HITOP" << endl;
  
  map<ub8, ub8>::iterator block_itr;

  for (pc_table_itr = pc_table.begin(); pc_table_itr != pc_table.end(); pc_table_itr++)
  {
    stall_data = (pc_data *)(pc_table_itr->second);
    for (block_itr = stall_data->ppc.begin(); block_itr != stall_data->ppc.end(); block_itr++)
    {
      pc_data *data = (pc_data *)(block_itr->second);
      out_stream << stall_data->pc << ";" <<  block_itr->first << ";" << data->access_count
        << ";" << data->fill_op << ";" << data->hit_op << endl;
    }
  }

  out_stream.close();

  assert(strlen("PC-BITMAP-stats.trace.csv.gz") + 1 < 100);
  sprintf(tracefile_name, "PC-BITMAP-stats.trace.csv.gz");
  out_stream.open(tracefile_name, ios::out | ios::binary);

  if (!out_stream.is_open())
  {
    printf("Error opening output stream\n");
    exit(EXIT_FAILURE);
  }
  
  printf("Dumping BITMAP Table %s \n", tracefile_name);

  out_stream << "PC; BITMAP; BLOCKS; HITS; EXPECTED_BLOCKS" << endl;
  
  map<ub8, ub8>::iterator bitmap_itr;

  cache_access_data *cache_data;

  for (pc_table_itr = pc_table.begin(); pc_table_itr != pc_table.end(); pc_table_itr++)
  {
    stall_data = (pc_data *)(pc_table_itr->second);
    for (bitmap_itr = stall_data->bitmaps.begin(); bitmap_itr != stall_data->bitmaps.end(); bitmap_itr++)
    {
      cache_data = (cache_access_data *)(bitmap_itr->second);

      out_stream << stall_data->pc << ";";
      out_stream << std::setbase(16) << bitmap_itr->first << ";";
      out_stream << std::setbase(10) << cache_data->access_count << ";";
      out_stream << cache_data->hit_count << ";" << cache_data->expected_blocks << endl;
    }
    
    if (stall_data->pc_sharing_bitmap)
    {
      out_stream << stall_data->pc << ";";
      out_stream << std::setbase(16) << stall_data->pc_sharing_bitmap << ";";
      out_stream << std::setbase(10) << 1 << ";" << 0 << ";" << endl;
    }
  }

  out_stream.close();
}

void dump_hist_for_stream(ub1 stream)
{
  /* Dump Reuse stats in file */
  gzofstream   out_stream;
  char         tracefile_name[100];
  extern  sb1 *stream_names[TST + 1];  

  assert(stream >= 0 && stream <= TST + 1);
  assert(strlen("CHRD--stats.trace.csv.gz") + 1 < 100);
  sprintf(tracefile_name, "CHRD-%s-stats.trace.csv.gz", stream_names[stream]);
  out_stream.open(tracefile_name, ios::out | ios::binary);

  if (!out_stream.is_open())
  {
    printf("Error opening output stream\n");
    exit(EXIT_FAILURE);
  }
  
  printf("Dumping reuse distance histogram %s \n", tracefile_name);

  out_stream << "Distance; Blocks; Reuse" << endl;

  for (ub8 i = 0; i <= MAX_RDIS; i++)
  {
    out_stream << std::dec << i << ";" << rdis_hist[stream][i][0] << ";" << rdis_hist[stream][i][1] << endl;
  }

  out_stream.close();

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

  /* Dump Reuse stats in file */
  assert(strlen("CHRRPVTRANS--stats.trace.csv.gz") + 1 < 100);
  sprintf(tracefile_name, "CHRRPVTRANS-%s-stats.trace.csv.gz", stream_names[stream]);
  out_stream.open(tracefile_name, ios::out | ios::binary);

  if (!out_stream.is_open())
  {
    printf("Error opening output stream\n");
    exit(EXIT_FAILURE);
  }
  
  printf("Dumping RRPV transitions %s \n", tracefile_name);

  out_stream << "Transitions; Blocks" << endl;

  for (ub8 i = 0; i <= MAX_TRANS; i++)
  {
    out_stream << std::dec << i << ";" << rrpv_trans_hist[stream][i] << endl;
  }

  out_stream.close();
}

void dump_hist_for_rrpv_trans()
{
  /* Dump Reuse stats in file */
  gzofstream   out_stream;
  char         tracefile_name[100];

  /* Dump Reuse stats in file */
  assert(strlen("CHRRPVTRANS-stats.trace.csv.gz") + 1 < 100);
  sprintf(tracefile_name, "CHRRPVTRANS-stats.trace.csv.gz");
  out_stream.open(tracefile_name, ios::out | ios::binary);

  if (!out_stream.is_open())
  {
    printf("Error opening output stream\n");
    exit(EXIT_FAILURE);
  }
  
  printf("Dumping RRPV transitions %s \n", tracefile_name);

  out_stream << "Transitions; Blocks" << endl;

  for (ub8 i = 0; i <= MAX_TRANS; i++)
  {
    out_stream << std::dec << i << ";" << rrpv_trans_hist[i] << endl;
  }

  out_stream.close();
}

void cachesim_fini()
{
  dump_hist_for_stream(BS);
  dump_hist_for_stream(TS);
  dump_hist_for_stream(ZS);
  dump_hist_for_stream(CS);
  dump_hist_for_stream(PS);
  dump_pc_table();
}

#undef BLCKALIGN
#undef HASHTINDXMASK
#undef INVALIDATE_SHARERS
#undef PHASE_BITS
#undef PHASE_SIZE
#undef IS_SPILL_ALLOCATED
