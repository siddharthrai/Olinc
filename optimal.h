#ifndef OPTIMAL_H
#define OPTIMAL_H

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <map>
#include <stdlib.h>
#include <zlib.h>
#include "common.h"
#include "../common/intermod-common.h"
#include <iostream>
#include <fstream>
#include "zfstream.h"
#include "math.h"

using namespace std;

#define CACHE_BLCKOFFSET     (6)

#define CACHE_L1_BSIZE       ((ub8)((ub8)1 << 6))
#define CACHE_L1_SIZE        ((ub8)((ub8)1 << 15))
#define CACHE_L1_WAYS        ((ub8)((ub8)1 << 3))
#define CACHE_L1_INDX_MASK   ((ub8)(0x3f) << 6)
#define CACHE_L1_TAG_MASK    ((ub8)(~((ub8)0xfff)))
#define CACHE_L1_TAG_SHFT    (12)

#define CACHE_L1_CACHE_LINES (CACHE_L1_SIZE / (CACHE_L1_WAYS * (1 << CACHE_BLCKOFFSET)))

/* 2MB cache parameters */
#define CACHE_L2_SIZE        (1 << 21)
#define CACHE_L2_WAYS        (1 << 4)
#define CACHE_L2_INDX_MASK   (0x7ff << 6)
#define CACHE_L2_TAG_MASK    (~0x1ffff)
#define CACHE_L2_TAG_SHFT    (17)

#define CACHE_L2_CACHE_LINES (CACHE_L2_SIZE / (CACHE_L2_WAYS * (1 << CACHE_BLCKOFFSET)))

#define CACHE_I_STATE        (0)
#define CACHE_M_STATE        (1)
#define CACHE_E_STATE        (2)
#define CACHE_S_STATE        (3)

#define CACHE_AND(x, y) ((x) & (y)) 
#define CACHE_OR(x, y)  ((x) | (y))
#define CACHE_SHL(x, y) (((y) == 64) ? 0 : ((x) << (y)))
#define CACHE_SHR(x, y) (((y) == 64) ? 0 : ((x) >> (y)))

#define CACHE_LINE_VALID_MASK (0x1);

/* 0 stand for LRU and 1 stand for OPT */
#define FRAGS     (1)
#define TAG_SIZE  (7)

/*
 *
 * NAME
 *
 *  Bit set
 *
 * DESCRIPTION
 *
 *  Set y-th bit in a bit-vector
 *
 * PARAMETERS
 *
 *  x (OUT) - bit vector 
 *  y (IN)  - bit position to be modified
 *
 * RETURNS
 *  
 *  Modified bit vector 
 *
 * NOTES
 *
 *  Bit-vector is assumed to be of 64 bit in size
 *
 */

#define CACHE_BIS(x, y) ((x) = (CACHE_OR((x), CACHE_SHL((ub8)0x1, (y))))) 

/*
 *
 * NAME
 *
 *  Bit clear
 *
 * DESCRIPTION
 *
 *  Clear y-th bit in a bit-vector
 *
 * PARAMETERS
 *
 *  x (OUT) - bit vector 
 *  y (IN)  - bit position to be cleared
 *
 * RETURNS
 *  
 *  Modified bit vector 
 *
 * NOTE
 *
 * Bit-vector is assumed to be of 64 bits in size
 *
 */

#define CACHE_BIC(x, y) \
  ((x) = (CACHE_AND((x), CACHE_OR(CACHE_SHL((~(ub8)0), 64 - (y) + 1), CACHE_SHR((~(ub8)0), (y))))))

#define CLRSTRUCT(s) (memset((&s), 0, sizeof(s)))

/* Constants for last operation on block */
typedef enum block_op
{
  block_op_invalid = 1,
  block_op_fill,
  block_op_hit
}block_op;

#define CACHE_LINE_ENT(x) ((x)->cachesim_ecnt)
typedef struct
{
  ub8       cachesim_ecnt;
  ub1       valid;
  ub8       pc;
  ub8       fillpc;
  ub8       tag;
  ub8       vtl_addr;
  ub8       rdis;
  ub8       prdis;
  ub8       way;
  ub1       stream;
  ub1       dirty;
  ub1       epoch;
  ub1       is_bt_block;
  ub1       is_ct_block;
  ub8       access;
  ub8       rrpv;
  block_op  op;
}cachesim_cacheline;

#define CACHE_LOCK(x)     ((x)->cachesim_cache_lock)
#define CACHE_LINE(x)     ((x)->cachesim_cache_cacheline)
#define CACHE_SIZE(x)     ((x)->cachesim_cache_size)
#define CACHE_BSIZE(x)    ((x)->cachesim_block_size)
#define CACHE_WAYS(x)     ((x)->cachesim_cache_ways)
#define CACHE_LCNT(x)     ((x)->cachesim_cache_line_cnt)
#define CACHE_IMSK(x)     ((x)->cachesim_cache_indx_mask)
#define CACHE_TMSK(x)     ((x)->cachesim_cache_tag_mask)
#define CACHE_TSHF(x)     ((x)->cachesim_cache_tag_shft)
#define CACHE_MISS(x)     ((x)->cachesim_miss_cnt)
#define CACHE_NLVL(x)     ((x)->cachesim_cache_next_lvl)
#define CACHE_PLVL(x)     ((x)->cachesim_cache_prev_lvl)
#define CACHE_AFRQ(x)     ((x)->cachesim_afrq)
#define CACHE_EVCT(x)     ((x)->cachesim_evct)
#define CACHE_MAX_RRPV(x) ((x)->cachesim_max_rrpv)

#define CACHE_SNDCNT(x)   ((x)->cachesim_snd_cnt)
#define CACHE_RCVCNT(x)   ((x)->cachesim_rcv_cnt)
#define CACHE_GETCNT(x)   ((x)->cachesim_get_cnt)
#define CACHE_GETXCNT(x)  ((x)->cachesim_getx_cnt)
#define CACHE_INVALCNT(x) ((x)->cachesim_inval_cnt)
#define CACHE_UPGRDCNT(x) ((x)->cachesim_upgrd_cnt)
#define CACHE_INTERCNT(x) ((x)->cachesim_inter_cnt)
#define CACHE_WBCNT(x)    ((x)->cachesim_wb_cnt)

struct cachesim_cache;

struct cachesim_cache
{
  /* TODO: 
      lock per cache 
      invalidation routine
      update state 
  */
  //PIN_LOCK cachesim_cache_lock;
  ub8 cachesim_cache_size;
  ub2 cachesim_block_size;
  ub8 cachesim_cache_ways;
  ub8 cachesim_cache_line_cnt;
  ub8 cachesim_cache_indx_mask;
  ub8 cachesim_cache_tag_mask;
  ub8 cachesim_cache_tag_shft;
  ub8 cachesim_miss_cnt;
  ub8 cachesim_snd_cnt;
  ub8 cachesim_rcv_cnt;
  ub8 cachesim_get_cnt;
  ub8 cachesim_getx_cnt;
  ub8 cachesim_inval_cnt;
  ub8 cachesim_upgrd_cnt;
  ub8 cachesim_inter_cnt;
  ub8 cachesim_wb_cnt;
  ub4 cachesim_max_rrpv;

  /* Cache tag array */ 
  cachesim_cacheline *cachesim_cache_cacheline;

  /* Structure to track set usage */
  ub8 *cachesim_afrq;

  /* Structure to track evictions */
  ub8 *cachesim_evct;

  /* keeps pointer to next level of cache */
  struct cachesim_cache *cachesim_cache_next_lvl; 

  /* keeps pointer to previous level of cache */
  struct cachesim_cache *cachesim_cache_prev_lvl; 
};

typedef struct cachesim_cache cachesim_cache;

/* Statistics of simulation */
typedef struct cachesim_cache_stats
{
  ub1 cst_game[100];            /* Game */
  ub1 cst_rsln[100];            /* Resolution */
  ub1 cst_trcn[100];            /* Trace name */
  ub1 cst_sfrm;                 /* Start frame */
  ub1 cst_tfrm;                 /* Total frames */
  ub8 cst_uacc[4];              /* Vertex, color, depth, texture accesses */
  ub8 cst_tacc;                 /* Total accesses */
  ub8 cst_lumiss[4];            /* LRU vertex, color, depth, texture misses */
  ub8 cst_ltmiss;               /* LRU total misses */
  ub8 cst_oumiss[4];            /* OPT vertex, color, depth, texture misses */
  ub8 cst_otmiss;               /* OPT total misses */
  ub8 cst_lucmiss[4];           /* LRU vertex, color, depth, texture cold misses */
  ub8 cst_ltcmiss;              /* LRU total cold misses */
  ub8 cst_oucmiss[4];           /* OPT vertex, color, depth, texture cold misses */
  ub8 cst_otcmiss;              /* OPT total cold misses */
}cachesim_cache_stats;

/* Stores ordered sequence of keys */
typedef struct seq_map
{
  ub8           seq_id;         /* Next sequence id */
  map<ub8, ub8> map_seq;        /* Map of keys */
}seq_map;

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

void add_to_seq_map(seq_map *map, ub8 key);

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

ub8 get_id_from_seq_map(seq_map *map, ub8 key);

typedef struct cache_access_data
{
  ub8 access_count;             /* Block align memory address */
  ub8 hit_count;                /* Block align memory address */
  ub8 expected_blocks;          /* Records expected blocks if bitmap is used */
}cache_access_data;

/* Per-block data kept with per-pc data */
typedef struct block_data
{
  ub8 pc_sharing_bitmap;        /* Bit-map where each bit corresponds to a PC */
  ub8 block_address;            /* Block align memory address */
  ub8 overflow_count;           /* # PCs which overflow the bit-map */
  cache_access_data block_data; /* Cache access stats for the block */
}block_data;

/* A FIFO of bitmaps */
class bitmap_fifo
{
  private :

    ub1   fifo_size;
    ub8 **last_n_bitmaps;     /* Last 5 accessed bit maps */
    ub1   current_ptr;        /* Pointer in the FIFO list */
    ub1   bitmap_count;       /* Pointer in the FIFO list */
    ub8   last_access_sn;     /* Sequence number which last accessed the bitmap */
    ub8   sn_thres;           /* Threshold to age FIFO entries */

  public:

    bitmap_fifo(ub1 size_in)
    {
      last_n_bitmaps = new ub8*[size_in];
      assert(last_n_bitmaps);

      for (ub1 i = 0; i < size_in; i++)
      {
        last_n_bitmaps[i] = new ub8[2];
        memset(last_n_bitmaps[i], 0, sizeof(ub8) * 2);
      }

      fifo_size       = size_in;
      current_ptr     = 0;
      bitmap_count    = 0;
      sn_thres        = 10000;
      last_access_sn  = 0;
    }

    sb1 find_bitmap(ub8 bitmap)
    {
      ub1 i;

      assert(bitmap != 0);

      for (i = 0; i < fifo_size; i++)
      {
        if (this->last_n_bitmaps[i][0] == bitmap)
          break;
      }

      return (i == this->fifo_size) ? -1 : i;
    }

    void move_fifo_fwd(ub1 start, ub1 end)
    {
      assert(start < end && start >= 0 && end < this->fifo_size);

      for (ub1 i = start + 1; i <= end; i++)
      {
        this->last_n_bitmaps[i - 1][0] = this->last_n_bitmaps[i][0];
        this->last_n_bitmaps[i - 1][1] = this->last_n_bitmaps[i][1];
      }
    }

    void move_fifo_bwd(ub1 start, ub1 end)
    {
      assert(start < end && start >= 0 && end < this->fifo_size);

      for (ub1 i = end; i > start; i--)
      {
        this->last_n_bitmaps[i][0] = this->last_n_bitmaps[i - 1][0];
        this->last_n_bitmaps[i][1] = this->last_n_bitmaps[i - 1][1];
      }
    }

    void swap_fifo_element(ub1 ele1, ub1 ele2)
    {
      ub8 tmp;

      for (ub1 i = 0; i < 2; i++)
      {
        tmp = this->last_n_bitmaps[ele1][i];
        this->last_n_bitmaps[ele1][i] = this->last_n_bitmaps[ele2][i];
        this->last_n_bitmaps[ele2][i] = tmp;
      }
    }

    void add_bitmap_to_fifo(ub8 bitmap, ub8 sn)
    {
      sb1 indx;

      assert(bitmap != 0);
      assert(last_access_sn < sn);
      
      if ((sn - last_access_sn) <= sn_thres)
      {
        indx = this->find_bitmap(bitmap);

        if (indx != -1)
        {
          if (indx != 0)
          {
            ub8 old_bitmap;
            ub8 old_access;

            old_bitmap = this->last_n_bitmaps[indx][0];
            old_access = this->last_n_bitmaps[indx][1];

            this->move_fifo_bwd(0, indx);

            /* Move accessed to the head */
            this->last_n_bitmaps[0][0] = old_bitmap;
            this->last_n_bitmaps[0][1] = old_access + 1;
          }
          else
          {
            this->last_n_bitmaps[0][1] += 1;
          }
        }
        else
        {
          /* Update bitmap entry */
          if (this->bitmap_count < this->fifo_size)
          {
            this->last_n_bitmaps[this->current_ptr][0] = bitmap;
            this->last_n_bitmaps[this->current_ptr][1] = 1;

            /* Move pointer to next FIFO entry */
            this->current_ptr   = (this->current_ptr + 1) % this->fifo_size;
            this->bitmap_count  = this->bitmap_count + 1;
          }
          else
          {
            assert(this->bitmap_count == this->fifo_size);

            this->last_n_bitmaps[this->fifo_size - 1][0] = bitmap;
            this->last_n_bitmaps[this->fifo_size - 1][1] = 1;
          }
        }
      }
      else
      {
        this->bitmap_count = 0;
        this->current_ptr  = 0;
      }
      
      last_access_sn = sn;
    }

    ub8 find_max_bitmap()
    {
      ub8 max;
      ub8 max_indx;

      max       = 0; 
      max_indx  = 0; 

      if (this->bitmap_count > 0)
      {
        /* Scan entire FIFO to get the max bit-vector */
        for (ub1 i = 0; i < this->bitmap_count; i++) 
        {
          if (this->last_n_bitmaps[i][1] > max)    
          {
            max       = this->last_n_bitmaps[i][1];
            max_indx  = i;
          }
        }
      }

      assert(max_indx < this->fifo_size); 

      return (max > 0) ? this->last_n_bitmaps[0][0] : 0;
    }
};
/* Per PC data used for statistics collection */
typedef struct pc_data
{
  ub4 id;                       /* Id given to the PC data */
  ub8 pc;                       /* Program counter */
  ub8 pc_sharing_bitmap;        /* Bit-map where each bit corresponds to a PC */
  ub8 overflow_count;           /* # PCs which overflow the bit-map */
  ub8 access_count;             /* Times pc was at the head */
  ub8 stat_access_count;        /* Times pc was at the head */
  ub8 stat_miss_l1;             /* L1 miss count */
  ub8 stat_miss_l2;             /* L2 miss count */
  ub8 stat_miss_llc;            /* LLC miss count */
  ub8 stat_access_llc;          /* LLC miss count */
  ub8 fill_op;                  /* # times last operation was fill */
  ub8 hit_op;                   /* # times last operation was hit */
  map<ub8, ub8> blocks;         /* Blocks accessed by this PC */
  map<ub8, ub8> ppc;            /* PCs which last accessed the current block */
  map<ub8, ub8> bitmaps;        /* List of distinct bit-maps seen by the PC */
  bitmap_fifo  *bmp_fifo;       /* FIFO of last N bit maps */
  seq_map pc_map;               /* PCs accessing blocks brought by this PC */
}pc_data;

/* Returns true for cache hit */
#define ADDR_NDX(cache, addr) (CACHE_AND(addr, CACHE_IMSK(cache)) >> CACHE_BLCKOFFSET)
#define ADDR_TAG(cache, addr) (CACHE_AND(addr, CACHE_TMSK(cache)) >> CACHE_TSHF(cache))

cache_access_status cachesim_incl_cache(cachesim_cache *cache, ub8 addr, 
  memory_trace info, ub8 rdis);

cache_access_status cachesim_match (cachesim_cache *cache, ub8 addr);

void cachesim_index(cachesim_qnode *bhead, MtrcInfo info);

void set_cache_params(ub8 cache_size, ub2 block_size, ub8 ways, 
  struct cachesim_cache *cache);

void cachesim_fini();
#endif
