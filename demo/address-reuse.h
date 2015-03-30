#ifndef ADDRESS_REUSE_H
#define ADDRESS_REUSE_H

#include <map>
#include <vector>
#include <iostream>
#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include <iomanip>
#include <fstream>
#include <string.h>
#include <math.h>
#include "zfstream.h"
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include "region-map.h"
#include "cache.h"
#include "cache-block.h"

using namespace std;

#define BLOCK_SIZE    (64)
#define PAGE_SIZE     (4096)
#define ACC_BUCKETS   (32)
#define PHASE_ACCESS  (1 << 20)
#define MAX_REUSE     (4096)

extern sb1 *stream_names[TST + 1];

typedef enum block_operation
{
  block_operation_unknown = 0,
  block_operation_fill    = 1,
  block_operation_reuse   = 2
}block_operation;

class ReuseInfo
{
  ub8             address;              /* Block address */
  ub8             set_idx;              /* Cache set index of block */
  ub1             replacement;          /* Count of replacements */
  vector<pair<ub8, ub1> > eviction_seq; /* Replacement sequence and streams */
  block_operation last_operation;       /* Last operation on this block (Fill / reuse) */
  vector<ub8>     fill_seq;             /* Sequence numbers of reuses to this blocks */
  vector<pair<ub8, ub8> > access_seq;   /* Sequence numbers of reuses to this blocks */

  public:

  ReuseInfo(ub8 address_in, ub8 set_idx_in)
  {
    address         = address_in;
    replacement     = 0;
    last_operation  = block_operation_unknown;
    set_idx         = set_idx_in;
#if 0
    access_seq.push_back(pair<ub8, ub8>(block_seq_in, g_block_seq_in));
    fill_seq.push_back(g_block_seq_in);
#endif
  }

  /* Add a new sequence number */
  void addReuseSequence(ub8 access_seq_in, ub8 rplc_seq_in)
  {
    access_seq.push_back(pair<ub8, ub8>(access_seq_in, rplc_seq_in));
  }
  
  /* Add a new sequence number */
  void addFillSequence(ub8 rplc_seq_in)
  {
    fill_seq.push_back(rplc_seq_in);
  }
  
  /* Set block operation */
  void setOperation(block_operation operation)
  {
    last_operation = operation;
  }

  /* Get block operation */
  block_operation getOperation()
  {
    return last_operation;
  }

  /* Get number of reuses */
  ub8 getReuse()
  {
    return access_seq.size();
  }
  
  /* Increment replacement to the info */
  void addReplacement(ub1 stream, ub8 eviction_seq_in)
  {
    assert(replacement < MAX_REUSE);
    eviction_seq.push_back(pair<ub8, ub1>(eviction_seq_in, stream));

    replacement += 1;
  }

  /* Get number of replacements */
  ub8 getReplacements()
  {
    return replacement;
  }
  
  /* Get number of replacements */
  ub8 getSetIdx()
  {
    return set_idx;
  }
  
  /* Print all sequence number */
  void printFillSequence(gzofstream &out_stream)
  {
    vector<ub8>::iterator seq_itr;

    for (seq_itr = fill_seq.begin(); seq_itr != fill_seq.end(); seq_itr++)
    {
      out_stream << *seq_itr << " ";
    }
  }

  /* Print all sequence number */
  void printEvictionSequence(gzofstream &out_stream)
  {
    vector<pair<ub8, ub1> >::iterator seq_itr;

    for (seq_itr = eviction_seq.begin(); seq_itr != eviction_seq.end(); seq_itr++)
    {
      out_stream << seq_itr->first << ":" << (int)seq_itr->second << " ";
    }
  }

  /* Print all sequence number */
  void printSequence(gzofstream &out_stream, ub8 min_seq = 0)
  {
    vector<pair<ub8, ub8> >::iterator seq_itr;

    for (seq_itr = access_seq.begin(); seq_itr != access_seq.end(); seq_itr++)
    {
      out_stream << seq_itr->first - min_seq << " ";
    }
  }

  /* Print all global sequence number */
  void printGlobalSequence(gzofstream &out_stream, ub8 min_seq = 0)
  {
    vector<pair<ub8, ub8> >::iterator seq_itr;

    for (seq_itr = access_seq.begin(); seq_itr != access_seq.end(); seq_itr++)
    {
      out_stream << seq_itr->second - min_seq << " ";
    }
  }
  
  /* Get vector of sequences */
  vector<pair<ub8, ub8> > getSequence()
  {
    return access_seq;
  }
};


/* Map of all regions */
class Region
{
  ub1 stream;                 /* Region stream */
  ub8 start;                  /* Region start page address */
  ub8 end;                    /* Region end page address */
  ub8 real_hits;              /* Real hits seen by the region */
  map <ub8, ub8> block_map;   /* Blocks currently in the cache */
  map <ub8, ub8> all_blocks;  /* All blocks accessed by the stream */
  cache_t       *s_region;    /* Stream region LRU table */
  sctr          *s_acc_table; /* Region table indexed by hash */
  gzofstream     out_stream;  /* Stream to dump region map */
  ub8            access_count;/* Count of accesses seen by the region */

  public:

  Region(ub1 stream_in, ub8 start_in, ub8 end_in)
  {
    stream        = stream_in;
    start         = start_in;
    end           = end_in;
    real_hits     = 0;
    access_count  = 0;

    /* Build region lru-table parameters and initialize the cache */
    struct cache_params params;
    memset(&params, 0, sizeof(struct cache_params));

#define SET_FA  (1)
#define WAY_FA  (16)

    params.num_sets   = SET_FA;
    params.ways       = WAY_FA;
    params.policy     = cache_policy_stridelru;
    params.stl_stream = stream_in;

#undef SET_FA
#undef WAY_FA
    
    /* Initialize LRU table */
    s_region = cache_init(&params);
    assert(s_region);
    
    /* Initialize accumulator table */
    s_acc_table = new sctr[ACC_BUCKETS];
    assert(s_acc_table);

#define CTR_W (20)
#define CTR_M (0)
#define CTR_X ((1 << CTR_W) - 1)

    for (int i = 0; i < ACC_BUCKETS; i++)
    {
      SAT_CTR_INI(s_acc_table[i], CTR_W, CTR_M, CTR_X);
    }

#undef CTR_W
#undef CTR_M
#undef CTR_X

    sb1 tracefile_name[100];

    /* Open output stream */
    assert(strlen("Region-----stats.trace.csv.gz") + 1 < 100);
    sprintf(tracefile_name, "Region-%s-%s-stats.trace.csv.gz", 
        stream_names[stream_in], stream_names[stream_in]);
    out_stream.open(tracefile_name, ios::out | ios::binary);

    if (!out_stream.is_open())
    {
      printf("Error opening output stream\n");
      exit(EXIT_FAILURE);
    }
  }
  
  ~Region()
  {
    struct cache_block_t *block;
    ub8    total_hits;
    ub8    total_false_hits;
    ub8    total_unknown_hits;

#if 0
    out_stream << "Region; Start; End; PageCount; Replacements; Hits;";
    out_stream << "FalseHits; UnknownHits; MaxStride; MaxUse; MaxUseBitmap;";
    out_stream << "MaxHiBitmap; UseBitmap; HitBitmap; Use; Hit; Expand; Longuse' Longhit" << endl;
#endif

    out_stream << "Region; Start; End; PageCount; Replacements; Hits; Use; Captured; Longuse; Longhit; Trasition" << endl;

    total_hits          = 0;
    total_false_hits    = 0;
    total_unknown_hits  = 0;

    /* Dump region cache */
    for (sb4 i = 0; i < s_region->assoc; i++)
    {
      block = &(STRIDELRU_DATA_BLOCKS(CACHE_SET_DATA_STRIDELRU(&(s_region->sets[0])))[i]);
      assert(block);
      ub4 long_use;
      ub4 long_hit;
        
      long_use = (block->long_use) ? (int)count_bit_set(block->long_use) : -1;
      long_hit = (block->long_hit) ? (int)count_bit_set(block->long_hit) : -1;

      out_stream << i << ";" << block->tag << ";" << block->tag_end << ";"
        << block->tag_end - block->tag << ";" << block->replacements << ";"
        << block->hits << ";" << block->use << ";" << block->captured_use 
        << ";" << long_use << ";" << long_hit << ";" << block->color_transition << endl;

      total_hits         += block->hits;
      total_false_hits   += block->false_hits;
      total_unknown_hits += block->unknown_hits;
    }

    out_stream << "Total: " << total_hits << " False: " << total_false_hits 
      << " Real: " << this->real_hits << " Unknown: " << total_unknown_hits 
      << endl;

    /* Cleanup cache */
    cache_free(s_region);
    out_stream.close();
  }

  unsigned int count_bit_set(ub1 *bv)
  { 
#define STRIDE_BITS (13)
#define LBSIZE      (3)

    unsigned char mask;
    unsigned char tmp;
    unsigned int  bits;

    bits = 0;

    for (int i = 0; i < pow(2, STRIDE_BITS - LBSIZE); i++)
    {
      tmp   = bv[i];
      mask  = 0x1;

      while (mask)
      {
        bits += (tmp & mask) ? 1 : 0; 
        mask <<= 1;
      }
    }

    return bits;

#undef STRIDE_BITS
#undef LBSIZE
  }

  ub1 addBlock(ub8 address, ub8 set_idx)
  {
    ub1 ret = FALSE;

    if (block_map.find(address) == block_map.end())
    {
      block_map.insert(pair<ub8, ub8>(address, set_idx));

      ret = TRUE;
    }

    if (all_blocks.find(address) == all_blocks.end())
    {
      all_blocks.insert(pair<ub8, ub8>(address, set_idx));
    }
    
    /* Lookup and update region lru-table */
    cache_block_t *current_block;
    memory_trace   info;
    
    memset(&info, 0, sizeof(memory_trace));

    info.address = (address / PAGE_SIZE);
    info.stream  = stream;

#define ONLY_SET (0)

    /* Lookup region cache, on miss fill the block */
    current_block = cache_find_block(s_region, ONLY_SET, info.address, &info);
    if (!current_block)
    {
      sb4  vctm_way;
      sb4  vctm_stream;
      long long int vctm_tag;

      enum    cache_block_state_t vctm_state;
      struct  cache_block_t       vctm_block; 

      /* Get replacement candidate */
      vctm_way = cache_replace_block(s_region, ONLY_SET, &info);
      
      /* There should always be a replacement */
      assert(vctm_way != -1);
      
      vctm_block = cache_get_block(s_region, ONLY_SET, vctm_way, &vctm_tag, 
          &vctm_state, &vctm_stream);
      
      if (vctm_state != cache_block_invalid)
      {
        /* Fill block in the cache */
        cache_set_block(s_region, ONLY_SET, vctm_way, vctm_tag, 
            cache_block_invalid, NN, &info);
      }

      /* Fill block in the cache */
      cache_fill_block(s_region, ONLY_SET, vctm_way, info.address, 
          cache_block_exclusive, info.stream, &info);
    }
    else
    {
      /* Update recency of the block */
      cache_access_block(s_region, ONLY_SET, current_block->way, info.stream, &info);
    }
    
    /* Insert blocks into accumulator table */
    sb8 tag_hash = (address / BLOCK_SIZE) % ACC_BUCKETS;

    SAT_CTR_INC(s_acc_table[tag_hash]);

    if (++access_count == PHASE_ACCESS)
    {
      access_count = 0;
      
      for (int i = 0; i < ACC_BUCKETS; i++)
      {
        SAT_CTR_RST(s_acc_table[i]);
      }
    }

#undef ONLY_SET

    return ret;
  }
  
  void lookupXStrmBlock(ub8 address, ub1 stream, ub1 real_hit)
  {
    /* Lookup and update region LRU table */
    cache_block_t *current_block;
    memory_trace   info;
    
    memset(&info, 0, sizeof(memory_trace));

    info.address  = (address / PAGE_SIZE);
    info.stream   = stream;

#define ONLY_SET (0)

    /* Lookup region cache, on miss fill the block */
    current_block = cache_find_block(s_region, ONLY_SET, info.address, &info);
    if (current_block)
    {
      current_block->hits++;
      if (real_hit == FALSE)
      {
        current_block->false_hits++;
      }

      if (all_blocks.find(address) == all_blocks.end())
      {
        current_block->unknown_hits++;
      }

      /* Update recency of the block */
      cache_access_block(s_region, ONLY_SET, current_block->way, info.stream, &info);
    }
    
    if (real_hit)
    {
      this->real_hits += 1; 
    }

#undef ONLY_SET
  }

  ub1 isBlockPresent(ub8 address)
  {
    if (block_map.find(address) != block_map.end()) 
    {
      return TRUE;
    }
    else
    {
      return FALSE;
    }
  }
  
  void removeBlock(ub8 address)
  {
    block_map.erase(address);
  }

  void setStream(ub1 stream_in)
  {
    stream = stream_in;
  }

  void setStart(ub8 start_in)
  {
    start = start_in;
  }

  void setEnd(ub8 end_in)
  {
   end = end_in;
  }
  
  ub8 getBlockCount()
  {
    return block_map.size();
  }

  ub8 getAllBlockCount()
  {
    return all_blocks.size();
  }

  ub1 getStream()
  {
    return stream;
  }

  ub8 getStart()
  {
    return start;
  }

  ub8 getEnd()
  {
    return end;
  }
};

/* Map of all regions */
class AddressMap
{
  map<ub1, ub8>   region_map;       /* Map of region, indexed by stream */
  ub1             src_stream;       /* Source stream */
  ub1             tgt_stream;       /* Target stream */
  ub8             g_address_seq;    /* Sequence number for all accesses */
  ub8             x_address_seq;    /* Sequence number for CT access */
  ub8             xstrm_use;        /* #blocks used between src and tgt stream */
  ub8             xstrm_cache_use;  /* #blocks used between src and tgt stream from cache */
  ub4             max_reuse_size;   /* max reuse among all reused blocks */
  ub8             next_x_block;     /* Next seq of x stream reuse */
  ub8             min_x_reuse_seq;  /* Minimum seq of CT reuse */
  ub8             max_x_reuse_seq;  /* Maximum seq of CT reuse */
  map<ub8, ub8>   reuse_blocks;     /* blocks reused between src and tgt */
  map<ub8, ub8>   reuse_info;       /* Per-block CT reuse detail */
  gzofstream      out_stream;       /* Stream to dump reuse pattern */
  gzofstream      g_out_stream;     /* Stream to dump global reuse pattern */
  gzofstream      f_out_stream;     /* Stream to dump fills */
  gzofstream      e_out_stream;     /* Stream to dump replacements */
  RegionMap      *all_region_map;   /* Map of all regions in the reuse */

  public:

  AddressMap(ub1 src_stream_in, ub1 tgt_stream_in)
  {
    src_stream        = src_stream_in;  /* Source stream */
    tgt_stream        = tgt_stream_in;  /* Target stream */
    next_x_block      = 0;              /* Sequence number of x strem reuse */
    xstrm_use         = 0;              /* # CT reuse */
    xstrm_cache_use   = 0;              /* # CT reuse tracked in cache */
    max_reuse_size    = 0;              /* Max xstream reuse by a block */
    x_address_seq     = 0;              /* CT access sequence */
    g_address_seq     = 0;              /* Global CT access sequence */
    min_x_reuse_seq   = 0xffffffffff;   /* Min CT sequence number */
    max_x_reuse_seq   = 0;              /* Max CT sequence number */

    sb1 tracefile_name[100];

    /* Open output stream for global sequence numbers */
    assert(strlen("XStream-F-----stats.trace.csv.gz") + 1 < 100);
    sprintf(tracefile_name, "XStream-F-%s-%s-stats.trace.csv.gz", 
        stream_names[src_stream_in], stream_names[tgt_stream_in]);
    f_out_stream.open(tracefile_name, ios::out | ios::binary);

    if (!f_out_stream.is_open())
    {
      printf("Error opening output stream\n");
      exit(EXIT_FAILURE);
    }

    /* Open output stream */
    assert(strlen("XStream-A-----stats.trace.csv.gz") + 1 < 100);
    sprintf(tracefile_name, "XStream-A-%s-%s-stats.trace.csv.gz", 
        stream_names[src_stream_in], stream_names[tgt_stream_in]);
    out_stream.open(tracefile_name, ios::out | ios::binary);

    if (!out_stream.is_open())
    {
      printf("Error opening output stream\n");
      exit(EXIT_FAILURE);
    }

    /* Open output stream for global sequence numbers */
    assert(strlen("XStream-R-----stats.trace.csv.gz") + 1 < 100);
    sprintf(tracefile_name, "XStream-R-%s-%s-stats.trace.csv.gz", 
        stream_names[src_stream_in], stream_names[tgt_stream_in]);
    g_out_stream.open(tracefile_name, ios::out | ios::binary);

    if (!g_out_stream.is_open())
    {
      printf("Error opening output stream\n");
      exit(EXIT_FAILURE);
    }

    /* Open output stream for replacements */
    assert(strlen("XStream-E-----stats.trace.csv.gz") + 1 < 100);
    sprintf(tracefile_name, "XStream-E-%s-%s-stats.trace.csv.gz", 
        stream_names[src_stream_in], stream_names[tgt_stream_in]);
    e_out_stream.open(tracefile_name, ios::out | ios::binary);

    if (!e_out_stream.is_open())
    {
      printf("Error opening output stream\n");
      exit(EXIT_FAILURE);
    }

    all_region_map = new RegionMap(src_stream_in, tgt_stream_in, BLOCK_SIZE, PAGE_SIZE);
    assert(all_region_map);
  }

  ~AddressMap()
  {
    map<ub8, ub8>::iterator map_itr;
    map<ub1, ub8>::iterator region_itr;

    /* Clear all maps */
    for (region_itr = region_map.begin(); region_itr != region_map.end(); region_itr++)
    {
      Region *current_region;

      current_region = (Region *)(region_itr->second);

      delete current_region;
    }

    region_map.clear();
    reuse_blocks.clear();

    for (map_itr = reuse_info.begin(); map_itr != reuse_info.end(); map_itr++)
    {
      ReuseInfo *current_reuse_info;

      current_reuse_info = (ReuseInfo *)(map_itr->second);

      delete current_reuse_info;
    }

    reuse_info.clear();

    out_stream.close();

    f_out_stream.close();

    g_out_stream.close();

    e_out_stream.close();

    delete all_region_map;
  }
  
  /* Update reuse by source stream */
  void update_x_stream_fill(ub8 address)
  {
    map<ub8, ub8>::iterator reuse_itr;

    ReuseInfo *block_reuse_info;

    /* Add sequence number to x stream reuse map */
    reuse_itr = reuse_info.find(address);
    if(reuse_itr != reuse_info.end())
    {

      block_reuse_info = (ReuseInfo *)(reuse_itr->second);
      assert(block_reuse_info);

      block_reuse_info->setOperation(block_operation_fill);
    }
  }

  void update_stream_use(ub8 address, ub8 set_idx, ub8 access, ub8 rplc)
  {
    map<ub8, ub8>::iterator reuse_itr;

    ReuseInfo *block_reuse_info;

    /* Add sequence number to x stream reuse map */
    reuse_itr = reuse_info.find(address);
    if (reuse_itr == reuse_info.end())
    {
      block_reuse_info = new ReuseInfo(address, set_idx);
      reuse_info.insert(pair<ub8, ub8>(address, (ub8) (block_reuse_info)));
      block_reuse_info->addFillSequence(rplc);
    }
    else
    {
      block_reuse_info = (ReuseInfo *)(reuse_itr->second);
      assert(block_reuse_info);
      assert(block_reuse_info->getSetIdx() == set_idx);

      block_reuse_info->addFillSequence(rplc);
    }
  }

  void update_x_stream_reuse(ub8 address, ub8 set_idx, ub8 access, ub8 rplc)
  {
    map<ub8, ub8>::iterator reuse_itr;

    xstrm_use  += 1;
    reuse_itr   = reuse_blocks.find(address);

    if (reuse_itr == reuse_blocks.end())
    {
      /* Add block into CT block list */
      reuse_blocks.insert(pair<ub8, ub8>(address, next_x_block++));
    }

    ReuseInfo *block_reuse_info;

    /* Add sequence number to x stream reuse map */
    reuse_itr = reuse_info.find(address);
    if (reuse_itr == reuse_info.end())
    {
      block_reuse_info = new ReuseInfo(address, set_idx);
      reuse_info.insert(pair<ub8, ub8>(address, (ub8) (block_reuse_info)));

      block_reuse_info->addReuseSequence(access, rplc);
    }
    else
    {
      block_reuse_info = (ReuseInfo *)(reuse_itr->second);
      assert(block_reuse_info);
      assert(block_reuse_info->getSetIdx() == set_idx);

      block_reuse_info->addReuseSequence(access, rplc);
    }
    
    block_reuse_info->setOperation(block_operation_reuse);
      
    /* Update max reuse seen by any block */
    if (block_reuse_info->getReuse() > max_reuse_size)
    {
      max_reuse_size = block_reuse_info->getReuse();

      if (max_reuse_size > MAX_REUSE)
      {
        max_reuse_size = MAX_REUSE - 1;
      }
    }

    /* Update min max sequence seen by CT reuse */
    if (x_address_seq < min_x_reuse_seq)
    {
      min_x_reuse_seq = x_address_seq;
    }
    else
    {
      if (x_address_seq > max_x_reuse_seq)
      {
        max_x_reuse_seq = x_address_seq;
      }
    }

    x_address_seq++;
  }

  void removeAddress(ub1 stream, ub1 new_stream, ub8 address, ub8 set_idx, ub8 rplc)
  {
    map<ub1, ub8>::iterator map_itr;  /* Map iterator */
    Region *memory_region;

    /* TODO: Block is not present in any other stream */
    if (stream == src_stream)
    {
      map_itr = region_map.find(stream);
      assert(map_itr != region_map.end());

      /* Obtain the region */
      memory_region = (Region *)(map_itr->second);

      assert(memory_region);
      assert(memory_region->getStream() == stream);
      assert(memory_region->isBlockPresent(address));

      /* Remove block from the memory region of  the corresponding stream */
      memory_region->removeBlock(address);

      ReuseInfo *block_reuse_info;
      map<ub8, ub8>::iterator reuse_itr;

      /* Update reuse info */
      reuse_itr = reuse_info.find(address);
      if (reuse_itr != reuse_info.end())
      {
        block_reuse_info = (ReuseInfo *)(reuse_itr->second);
        assert(block_reuse_info);

        /* If block is reused, increment replacement count */
#if 0
        if (block_reuse_info->getOperation() == block_operation_reuse)
#endif
        {
          block_reuse_info->addReplacement(new_stream, rplc);
        }

        /* Reset last block operation */
        block_reuse_info->setOperation(block_operation_unknown);
      }
    }

    /* TODO: Block is not present in any other stream */
    if (stream == tgt_stream && src_stream != tgt_stream)
    {
      map_itr = region_map.find(stream);

      if (map_itr != region_map.end())
      {

        /* Obtain the region */
        memory_region = (Region *)(map_itr->second);

        assert(memory_region);
        assert(memory_region->getStream() == stream);
        assert(memory_region->isBlockPresent(address));

        /* Remove block from the memory region of  the corresponding stream */
        memory_region->removeBlock(address);
      } 
#if 0
      /* Remove */
      map_itr = region_map.find(src_stream);

      if(map_itr != region_map.end())
      {
        /* Obtain the region */
        memory_region = (Region *)(map_itr->second);

        assert(memory_region);
        assert(memory_region->getStream() == src_stream);

        if (memory_region->isBlockPresent(address))
        {
          /* Remove block from the memory region of the src stream */
          memory_region->removeBlock(address);
        }
      }
#endif
    }
  }

  ub1 isBlockPresent(ub1 stream, ub8 address)
  {
    map<ub1, ub8>::iterator map_itr;  /* Map iterator */
    Region *memory_region;
    
    if (stream == src_stream)
    {
      map_itr = region_map.find(stream);
      assert(map_itr != region_map.end());

      /* Obtain the region */
      memory_region = (Region *)(map_itr->second);

      assert(memory_region);
      assert(memory_region->getStream() == stream);
      assert(memory_region->isBlockPresent(address));
    }

    return TRUE;
  }

  void addAddress(ub1 stream, ub8 address, ub8 set_idx, ub8 access, ub8 rplc)
  {
    ub1 block_filled;                 /* TRUE is block is filled */
    map<ub1, ub8>::iterator map_itr;  /* Map iterator */
  
    block_filled = FALSE;

    if (stream == src_stream)
    {
      map_itr = region_map.find(stream);
      if (map_itr == region_map.end())
      {
        Region *memory_region;

        memory_region = new Region(stream, address, address + BLOCK_SIZE);
        assert(memory_region);

        /* Create a new region */
        region_map.insert(pair<ub1, ub8>(stream, (ub8)memory_region));

        /* Add block to block map */
        if (memory_region->addBlock(address, set_idx) == TRUE)
        {
          block_filled = TRUE;
          update_stream_use(address, set_idx, access, rplc);
        }
      }
      else
      {
        /* Obtain the region */
        Region *memory_region = (Region *)(map_itr->second);

        assert(memory_region);
        assert(memory_region->getStream() == stream);

        /* Add block to block map */
        if (memory_region->addBlock(address, set_idx) == TRUE)
        {
          block_filled = TRUE;
          update_stream_use(address, set_idx, access, rplc);
        }

        if (memory_region->getStart() > address)
        {
          memory_region->setStart(address);
        }
        else
        {
          if (memory_region->getEnd() < address)
          {
            memory_region->setEnd(address);
          }
        }
      }

      g_address_seq++;
    }

    if (stream == tgt_stream && block_filled == FALSE)
    {
      Region *src_memory_region;

      /* Check x stream reuse */
      map_itr = (region_map.find(src_stream));

      /* If block in source stream map  */
      if (map_itr != region_map.end())
      {
        src_memory_region = (Region *)map_itr->second;
        assert(src_memory_region);

        if (src_memory_region->isBlockPresent(address))
        {
          update_x_stream_reuse(address, set_idx, access, rplc);

          src_memory_region->lookupXStrmBlock(address, stream, TRUE);
        }
        else
        {
          src_memory_region->lookupXStrmBlock(address, stream, FALSE);
        }

        /* Lookup source stream to see if it could be a hit */
        xstrm_cache_use++;
      }

      g_address_seq++;
    }
  }

  void printFillMap()
  {
    map<ub1, ub8>::iterator map_itr;
    Region  *current_region;
    Region  *src_region;
    ub4     *reused_blocks;

    reused_blocks = new ub4[max_reuse_size + 1];
    assert(reused_blocks);

    memset(reused_blocks, 0, sizeof(ub4) * (max_reuse_size + 1));

    for (map_itr = region_map.begin(); map_itr != region_map.end(); map_itr++)
    {
      current_region = (Region *)(map_itr->second);
      assert(current_region);

      f_out_stream << std::setw(10) << std::left;
      f_out_stream << (int)(map_itr->first);
      f_out_stream << std::setw(10) << std::left;
      f_out_stream << current_region->getBlockCount();
      f_out_stream << std::setw(12) << std::left;
      f_out_stream << current_region->getStart() << " " << current_region->getEnd() << endl;
    }

    f_out_stream << "Src stream " << (ub4)src_stream << endl;

    map_itr = region_map.find(src_stream); 
    if (map_itr != region_map.end())
    {
      f_out_stream << "Src blocks " << ((Region *)(map_itr->second))->getAllBlockCount() << endl;
    }
    else
    {
      f_out_stream << "Src blocks " << 0 << endl;
    }

    f_out_stream << "Tgt stream " << (ub4)tgt_stream << endl;

    map_itr = region_map.find(tgt_stream); 
    if (map_itr != region_map.end())
    {
      f_out_stream << "Tgt blocks " << ((Region *)(map_itr->second))->getAllBlockCount() << endl;
    }
    else
    {
      f_out_stream << "Src blocks " << 0 << endl;
    }

    f_out_stream << "Reuse " << xstrm_use << endl;
    f_out_stream << "Reused blocks " << reuse_info.size() << endl;

    ReuseInfo *current_reuse_info;
    map<ub8, ub8>::iterator reuse_itr;

    for(reuse_itr = reuse_info.begin(); reuse_itr != reuse_info.end(); reuse_itr++)
    {
      current_reuse_info = (ReuseInfo *)reuse_itr->second;

      if (current_reuse_info->getReuse() < max_reuse_size)
      {
        reused_blocks[current_reuse_info->getReuse()] += 1;
      }
      else
      {
        reused_blocks[max_reuse_size] += 1;
      }
    }

    /* Printing xstream block reuse histogram */
    f_out_stream << "max reuse " << (ub4)max_reuse_size << endl;

    assert(max_reuse_size < MAX_REUSE);

    f_out_stream << "[ReuseHistogram]" << endl;

    for (ub4 i = 1; i <= max_reuse_size; i++)
    {
      f_out_stream << std::setw(10) << std::left; 
      f_out_stream << (ub4)i << reused_blocks[i] << endl;
    }

    f_out_stream << "Printing xstream resue blocks" << endl;
    f_out_stream << "Min sequence " << min_x_reuse_seq << endl;
    f_out_stream << "Max sequence " << max_x_reuse_seq << endl;

    /* Get the region of source stream */
    map_itr = region_map.find(src_stream);

    if (map_itr != region_map.end())
    {
      src_region = (Region *)map_itr->second;

      ub8 last_block = 0;

      /* For all blocks in source stream print reuse info for reused blocks */
      for(reuse_itr = reuse_info.begin(); reuse_itr != reuse_info.end(); reuse_itr++)
      {
        current_reuse_info = (ReuseInfo *)(reuse_itr->second);
        if (current_reuse_info->getReuse() > 0)
        {
          f_out_stream << reuse_itr->first << " | " << current_reuse_info->getSetIdx() << " | " << current_reuse_info->getReplacements() << " | ";
          current_reuse_info->printFillSequence(f_out_stream);
          f_out_stream << endl;
        }
      }
    }

    delete [] reused_blocks;
  }

  void printEvictionMap()
  {
    map<ub1, ub8>::iterator map_itr;
    Region  *current_region;
    Region  *src_region;
    ub4     *reused_blocks;

    reused_blocks = new ub4[max_reuse_size + 1];
    assert(reused_blocks);

    memset(reused_blocks, 0, sizeof(ub4) * (max_reuse_size + 1));

    for (map_itr = region_map.begin(); map_itr != region_map.end(); map_itr++)
    {
      current_region = (Region *)(map_itr->second);
      assert(current_region);

      e_out_stream << std::setw(10) << std::left;
      e_out_stream << (int)(map_itr->first);
      e_out_stream << std::setw(10) << std::left;
      e_out_stream << current_region->getBlockCount();
      e_out_stream << std::setw(12) << std::left;
      e_out_stream << current_region->getStart() << " " << current_region->getEnd() << endl;
    }

    e_out_stream << "Src stream " << (ub4)src_stream << endl;

    map_itr = region_map.find(src_stream); 
    if (map_itr != region_map.end())
    {
      e_out_stream << "Src blocks " << ((Region *)(map_itr->second))->getAllBlockCount() << endl;
    }
    else
    {
      e_out_stream << "Src blocks " << 0 << endl;
    }

    e_out_stream << "Tgt stream " << (ub4)tgt_stream << endl;

    map_itr = region_map.find(tgt_stream); 
    if (map_itr != region_map.end())
    {
      e_out_stream << "Tgt blocks " << ((Region *)(map_itr->second))->getAllBlockCount() << endl;
    }
    else
    {
      e_out_stream << "Src blocks " << 0 << endl;
    }

    e_out_stream << "Reuse " << xstrm_use << endl;
    e_out_stream << "Reused blocks " << reuse_info.size() << endl;

    ReuseInfo *current_reuse_info;
    map<ub8, ub8>::iterator reuse_itr;

    for(reuse_itr = reuse_info.begin(); reuse_itr != reuse_info.end(); reuse_itr++)
    {
      current_reuse_info = (ReuseInfo *)reuse_itr->second;

      if (current_reuse_info->getReuse() < max_reuse_size)
      {
        reused_blocks[current_reuse_info->getReuse()] += 1;
      }
      else
      {
        reused_blocks[max_reuse_size] += 1;
      }
    }

    /* Printing xstream block reuse histogram */
    e_out_stream << "max reuse " << (ub4)max_reuse_size << endl;

    assert(max_reuse_size < MAX_REUSE);

    e_out_stream << "[ReuseHistogram]" << endl;

    for (ub4 i = 1; i <= max_reuse_size; i++)
    {
      e_out_stream << std::setw(10) << std::left; 
      e_out_stream << (ub4)i << reused_blocks[i] << endl;
    }

    e_out_stream << "Printing xstream resue blocks" << endl;
    e_out_stream << "Min sequence " << min_x_reuse_seq << endl;
    e_out_stream << "Max sequence " << max_x_reuse_seq << endl;
    
    /* Get the region of source stream */
    map_itr = region_map.find(src_stream);

    if (map_itr != region_map.end())
    {
      src_region = (Region *)map_itr->second;

      ub8 last_block = 0;

      /* For all blocks in source stream print reuse info for reused blocks */
      for(reuse_itr = reuse_info.begin(); reuse_itr != reuse_info.end(); reuse_itr++)
      {
        current_reuse_info = (ReuseInfo *)(reuse_itr->second);
        if (current_reuse_info->getReuse() > 0)
        {
          e_out_stream << reuse_itr->first << " | " << current_reuse_info->getSetIdx() << " | " << current_reuse_info->getReplacements() << " | ";
          current_reuse_info->printEvictionSequence(e_out_stream);
          e_out_stream << endl;
        }
      }
    }

    delete [] reused_blocks;
  }

  void printGlobalMap()
  {
    map<ub1, ub8>::iterator map_itr;
    Region  *current_region;
    Region  *src_region;
    ub4     *reused_blocks;

    reused_blocks = new ub4[max_reuse_size + 1];
    assert(reused_blocks);

    memset(reused_blocks, 0, sizeof(ub4) * (max_reuse_size + 1));

    for (map_itr = region_map.begin(); map_itr != region_map.end(); map_itr++)
    {
      current_region = (Region *)(map_itr->second);
      assert(current_region);

      g_out_stream << std::setw(10) << std::left;
      g_out_stream << (int)(map_itr->first);
      g_out_stream << std::setw(10) << std::left;
      g_out_stream << current_region->getBlockCount();
      g_out_stream << std::setw(12) << std::left;
      g_out_stream << current_region->getStart() << " " << current_region->getEnd() << endl;
    }

    g_out_stream << "Src stream " << (ub4)src_stream << endl;

    map_itr = region_map.find(src_stream); 
    if (map_itr != region_map.end())
    {
      g_out_stream << "Src blocks " << ((Region *)(map_itr->second))->getAllBlockCount() << endl;
    }
    else
    {
      g_out_stream << "Src blocks " << 0 << endl;
    }

    g_out_stream << "Tgt stream " << (ub4)tgt_stream << endl;

    map_itr = region_map.find(tgt_stream); 
    if (map_itr != region_map.end())
    {
      g_out_stream << "Tgt blocks " << ((Region *)(map_itr->second))->getAllBlockCount() << endl;
    }
    else
    {
      g_out_stream << "Src blocks " << 0 << endl;
    }

    g_out_stream << "Reuse " << xstrm_use << endl;
    g_out_stream << "Reused blocks " << reuse_info.size() << endl;

    ReuseInfo *current_reuse_info;
    map<ub8, ub8>::iterator reuse_itr;

    for(reuse_itr = reuse_info.begin(); reuse_itr != reuse_info.end(); reuse_itr++)
    {
      current_reuse_info = (ReuseInfo *)reuse_itr->second;

      if (current_reuse_info->getReuse() < max_reuse_size)
      {
        reused_blocks[current_reuse_info->getReuse()] += 1;
      }
      else
      {
        reused_blocks[max_reuse_size] += 1;
      }
    }

    /* Printing xstream block reuse histogram */
    g_out_stream << "max reuse " << (ub4)max_reuse_size << endl;

    assert(max_reuse_size < MAX_REUSE);

    g_out_stream << "[ReuseHistogram]" << endl;

    for (ub4 i = 1; i <= max_reuse_size; i++)
    {
      g_out_stream << std::setw(10) << std::left; 
      g_out_stream << (ub4)i << reused_blocks[i] << endl;
    }

    g_out_stream << "Printing xstream resue blocks" << endl;
    g_out_stream << "Min sequence " << min_x_reuse_seq << endl;
    g_out_stream << "Max sequence " << max_x_reuse_seq << endl;
    
    /* Get the region of source stream */
    map_itr = region_map.find(src_stream);

    if (map_itr != region_map.end())
    {
      src_region = (Region *)map_itr->second;

      ub8 last_block = 0;

      /* For all blocks in source stream print reuse info for reused blocks */
      for(reuse_itr = reuse_info.begin(); reuse_itr != reuse_info.end(); reuse_itr++)
      {
        current_reuse_info = (ReuseInfo *)(reuse_itr->second);
        if (current_reuse_info->getReuse() > 0)
        {
          g_out_stream << reuse_itr->first << " | " << current_reuse_info->getSetIdx() << " | " << current_reuse_info->getReplacements() << " | ";
          current_reuse_info->printGlobalSequence(g_out_stream);
          g_out_stream << endl;
        }

        /* If this is not the first block */
        if (last_block != 0)
        {
          if ((reuse_itr->first - last_block) > BLOCK_SIZE)
          {
            ub8 hole_size   = 0;
            ub8 next_stride = last_block + BLOCK_SIZE;
            for (ub8 unused_block = last_block + BLOCK_SIZE; unused_block < reuse_itr->first; unused_block += BLOCK_SIZE)
            {
              if (src_region->isBlockPresent(unused_block) == FALSE)
              {
                if (hole_size != 0)
                {
                  g_out_stream << next_stride << " | " << " Hole " << hole_size << endl;
                  hole_size   = 0;
                }
                next_stride  = unused_block + BLOCK_SIZE;
              }
              else
              {
                hole_size += BLOCK_SIZE;
              }
            }

            if (hole_size != 0)
            {
              g_out_stream << next_stride << " | " << " Hole " << hole_size << endl;
            }
          }
        }

        last_block = reuse_itr->first;
      }
    }

    delete [] reused_blocks;
  }

  void printMap()
  {
    map<ub1, ub8>::iterator map_itr;
    Region  *current_region;
    ub4     *reused_blocks;

    reused_blocks = new ub4[max_reuse_size + 1];
    assert(reused_blocks);

    memset(reused_blocks, 0, sizeof(ub4) * (max_reuse_size + 1));

    for (map_itr = region_map.begin(); map_itr != region_map.end(); map_itr++)
    {
      current_region = (Region *)(map_itr->second);
      assert(current_region);

      out_stream << std::setw(10) << std::left;
      out_stream << (int)(map_itr->first);
      out_stream << std::setw(10) << std::left;
      out_stream << current_region->getBlockCount();
      out_stream << std::setw(12) << std::left;
      out_stream << current_region->getStart() << " " << current_region->getEnd() << endl;
    }

    out_stream << "Src stream " << (ub4)src_stream << endl;
    out_stream << "Tgt stream " << (ub4)tgt_stream << endl;
    out_stream << "Reuse " << xstrm_use << endl;

    ReuseInfo *current_reuse_info;
    map<ub8, ub8>::iterator reuse_itr;

    for(reuse_itr = reuse_info.begin(); reuse_itr != reuse_info.end(); reuse_itr++)
    {
      current_reuse_info = (ReuseInfo *)(reuse_itr->second);

      if (current_reuse_info->getReuse() < max_reuse_size)
      {
        reused_blocks[current_reuse_info->getReuse()] += 1;
      }
      else
      {
        reused_blocks[max_reuse_size] += 1;
      }
    }

    /* Printing xstream block reuse histogram */
    out_stream << "max reuse " << (ub4)max_reuse_size << endl;

    assert(max_reuse_size < MAX_REUSE);

    out_stream << "[ReuseHistogram]" << endl;

    for (ub4 i = 1; i <= max_reuse_size; i++)
    {
      out_stream << std::setw(10) << std::left; 
      out_stream << (ub4)i << reused_blocks[i] << endl;
    }

    out_stream << "Printing xstream resue blocks" << endl;
    out_stream << "Min sequence " << min_x_reuse_seq << endl;
    out_stream << "Max sequence " << max_x_reuse_seq << endl;

    for (reuse_itr = reuse_info.begin(); reuse_itr != reuse_info.end(); reuse_itr++)
    {
      current_reuse_info = (ReuseInfo *)reuse_itr->second;
      if (current_reuse_info->getReuse() > 0)
      {
        out_stream << reuse_itr->first << " | " << current_reuse_info->getSetIdx() << " | " 
          << current_reuse_info->getReplacements() << " | ";
        current_reuse_info->printSequence(out_stream);
        out_stream << endl;
      }
#if 0
      /* Collect region wise data */
      all_region_map->addBlock(reuse_itr->first, current_reuse_info->getReuse(), 
          current_reuse_info->getReplacements());
#endif
    }

    delete [] reused_blocks;

    printGlobalMap();
    printEvictionMap();
    printFillMap();
  }
};

#undef BLOCK_SIZE
#undef PAGE_SIZE
#undef ACC_BUCKETS
#undef PHASE_ACCESS
#undef MAX_REUSE

#endif
