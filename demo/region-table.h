#ifndef REGION_TABLE_H
#define REGION_TABLE_H

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
#include "cache.h"
#include "cache-block.h"
#include "access-phase.h"

using namespace std;

#define BLOCK_SIZE    (64)
#define PAGE_SIZE     (4096)

extern sb1 *stream_names[TST + 1];


/* Map of all regions */
class StreamRegion
{
  ub1 src_stream;             /* Region stream */
  ub1 tgt_stream;             /* Region stream */
  ub8 real_hits;              /* Real hits seen by the region */
  ub8 predicted_hits;         /* Real hits seen by the region */
  map <ub8, ub8> all_blocks;  /* All blocks accessed by the stream */
  cache_t       *s_region;    /* Stream region LRU table */
  AccessPhase   *phases;      /* Phases for given region */
  gzofstream     out_stream;  /* Stream to dump region map */

  public:

  StreamRegion(ub1 src_stream_in, ub1 tgt_stream_in, ub8 start_in, ub8 end_in)
  {
    src_stream      = src_stream_in;
    tgt_stream      = tgt_stream_in;
    real_hits       = 0;
    predicted_hits  = 0;

    /* Build region lru-table parameters and initialize the cache */
    struct cache_params params;
    memset(&params, 0, sizeof(struct cache_params));

#define SET_FA  (1)
#define WAY_FA  (16)

    params.num_sets     = SET_FA;
    params.ways         = WAY_FA;
    params.policy       = cache_policy_stridelru;
    params.stl_stream   = src_stream_in;
    params.use_long_bv  = FALSE;

    /* Initialize LRU table */
    s_region = cache_init(&params);
    assert(s_region);

    phases = new AccessPhase(WAY_FA);
    assert(phases);

    phases->addAccessPhase(src_stream);
    phases->setTargetStream(src_stream, tgt_stream);

#undef SET_FA
#undef WAY_FA

    sb1 tracefile_name[100];

    /* Open output stream */
    assert(strlen("PhaseRegion-----stats.trace.csv.gz") + 1 < 100);
    sprintf(tracefile_name, "PhaseRegion-%s-%s-stats.trace.csv.gz", 
        stream_names[src_stream_in], stream_names[src_stream_in]);
    out_stream.open(tracefile_name, ios::out | ios::binary);

    if (!out_stream.is_open())
    {
      printf("Error opening output stream\n");
      exit(EXIT_FAILURE);
    }
  }

  ~StreamRegion()
  {
    struct cache_block_t *block;

    out_stream << "Region; Start; End; PageCount; Hits" << endl;

    /* Dump region cache */
    for (sb4 i = 0; i < s_region->assoc; i++)
    {
      block = &(STRIDELRU_DATA_BLOCKS(CACHE_SET_DATA_STRIDELRU(&(s_region->sets[0])))[i]);
      assert(block);

      out_stream << i << ";" << block->tag << ";" << block->tag_end << ";"
        << block->tag_end - block->tag << ";" << predicted_hits << endl;
    }

    /* Cleanup cache */
    cache_free(s_region);

    out_stream.close();
  }
  
  ub1 getStream()
  {
    return src_stream;
  }

  void addBlock(ub1 stream_in, ub8 address)
  {
    /* Lookup and update region lru-table */
    cache_block_t *current_block;
    memory_trace   info;

    memset(&info, 0, sizeof(memory_trace));

    info.address = (address / PAGE_SIZE);
    info.stream  = src_stream;

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
        
      if (stream_in == src_stream)
      {
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
    }
    else
    {
#define ACCESS_TH (0)

      /* If block was hit in region table, add block to region table */
      if (stream_in == src_stream)
      {
        if (phases->addBlock(address, src_stream, stream_in) > ACCESS_TH)
        {
          /* If block was hit phase tracking table */
          if (all_blocks.find(address) == all_blocks.end())
          {
            all_blocks.insert(pair<ub8, ub8>(address, 1));
          }
        }
      }
      else
      {
        assert(stream_in ==  tgt_stream);
        phases->addBlock(address, src_stream, stream_in);
      }

#undef ACCESS_TH

      /* Update recency of the block */
      cache_access_block(s_region, ONLY_SET, current_block->way, info.stream, &info);
    }

#undef ONLY_SET
  }
  
  void removeBlock(ub8 address)
  {
    if (all_blocks.find(address) != all_blocks.end())
    {
      all_blocks.erase(address);
    }
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
      if (all_blocks.find(address) != all_blocks.end())
      {
        predicted_hits++;
      }

      /* Update recency of the block */
      cache_access_block(s_region, ONLY_SET, current_block->way, info.stream, &info);
    }

#undef ONLY_SET
  }
};

/* Map of all regions */
class RegionTable
{
  map<ub1, ub8>   region_map;       /* Map of region, indexed by stream */
  ub1             src_stream;       /* Source stream */
  ub1             tgt_stream;       /* Target stream */

  public:

  RegionTable(ub1 src_stream_in, ub1 tgt_stream_in)
  {
    src_stream        = src_stream_in;  /* Source stream */
    tgt_stream        = tgt_stream_in;  /* Target stream */
  }

  ~RegionTable()
  {
    map<ub8, ub8>::iterator map_itr;
    map<ub1, ub8>::iterator region_itr;

    /* Clear all maps */
    for (region_itr = region_map.begin(); region_itr != region_map.end(); region_itr++)
    {
      StreamRegion *current_region;

      current_region = (StreamRegion *)(region_itr->second);

      delete current_region;
    }

    region_map.clear();
  }

  void addBlock(ub1 stream, ub8 address)
  {
    map<ub1, ub8>::iterator map_itr;  /* Map iterator */

    if (stream == src_stream)
    {
      map_itr = region_map.find(stream);
      if (map_itr == region_map.end())
      {
        StreamRegion *memory_region;

        memory_region = new StreamRegion(stream, tgt_stream, address, address + BLOCK_SIZE);
        assert(memory_region);

        /* Create a new region */
        region_map.insert(pair<ub1, ub8>(stream, (ub8)memory_region));

        /* Add block to block map */
        memory_region->addBlock(stream, address);
      }
      else
      {
        /* Obtain the region */
        StreamRegion *memory_region = (StreamRegion *)(map_itr->second);

        assert(memory_region);
        assert(memory_region->getStream() == stream);

        /* Add block to block map */
        memory_region->addBlock(stream, address);
      }
    }

    if (stream == tgt_stream)
    {
      StreamRegion *src_memory_region;

      /* Check x stream reuse */
      map_itr = (region_map.find(src_stream));

      /* If block in source stream map  */
      if (map_itr != region_map.end())
      {
        src_memory_region = (StreamRegion *)map_itr->second;
        assert(src_memory_region);

        src_memory_region->lookupXStrmBlock(address, stream, TRUE);

        src_memory_region->addBlock(stream, address);
      }
    }
  }

  void removeBlock(ub1 stream, ub8 address)
  {
    map<ub1, ub8>::iterator map_itr;  /* Map iterator */
    StreamRegion *memory_region;
    
    /* TODO: Block is not present in any other stream */
    if (stream == src_stream)
    {
      map_itr = region_map.find(stream);
      assert(map_itr != region_map.end());

      /* Obtain the region */
      memory_region = (StreamRegion *)(map_itr->second);

      assert(memory_region);
      assert(memory_region->getStream() == stream);

      /* Remove block from the memory region of  the corresponding stream */
      memory_region->removeBlock(address);
    }

    /* TODO: Block is not present in any other stream */
    if (stream == tgt_stream)
    {
      map_itr = region_map.find(stream);

      if (map_itr != region_map.end())
      {
        /* Obtain the region */
        memory_region = (StreamRegion *)(map_itr->second);

        assert(memory_region);
        assert(memory_region->getStream() == stream);

        /* Remove block from the memory region of  the corresponding stream */
        memory_region->removeBlock(address);
      } 
    }
  }
};

#undef BLOCK_SIZE
#undef PAGE_SIZE

#endif
