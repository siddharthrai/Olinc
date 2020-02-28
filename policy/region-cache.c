#include "region-cache.h"
#include <string.h>

#define BLOCK_SIZE    (64)
#define PAGE_SIZE     (4096)

extern char *stream_names[TST + 1];

void region_cache_init(region_cache *region, ub1 src_stream_in, ub1 tgt_stream_in)
{
  region->src_stream      = src_stream_in;
  region->tgt_stream      = tgt_stream_in;
  region->src_access      = 0;
  region->tgt_access      = 0;
  region->real_hits       = 0;
  region->predicted_hits  = 0;

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
  params.rpl_on_miss  = FALSE;

  /* If region is for intra-stream reuse, enable block replacement */
  if (src_stream_in == tgt_stream_in)
  {
    params.rpl_on_miss = TRUE;
  }

  /* Initialize LRU table */
  region->s_region = cache_init(&params);
  assert(region->s_region);

  /* Allocate phases and set src and target stream */
  region->phases = (access_phase *)malloc(sizeof(access_phase) * WAY_FA);
  assert(region->phases);

  sb1 tracefile_name[100];

  /* Open output stream */
  assert(strlen("Phases-----stats.trace.csv.gz") + 1 < 100);
  sprintf(tracefile_name, "PhasesCounters-%s-%s-stats.trace.csv.gz", 
    stream_names[src_stream_in], stream_names[tgt_stream_in]);

  region->out_file = gzopen(tracefile_name, "wb9");
  assert(region->out_file);

  for (ub4 i = 0; i < WAY_FA; i++) 
  {
    access_phase_init(&(region->phases[i]), src_stream_in, WAY_FA, 
        region->out_file);

    set_phase_target_stream(&(region->phases[i]), tgt_stream_in);
  }

#undef SET_FA
#undef WAY_FA
}
 
#define ONLY_SET        (0)

void region_cache_fini(region_cache *region)
{
  struct cache_block_t *block;
  
  printf("Dumping region cache %d : %d\n", region->src_stream, region->tgt_stream);
  /* Dump region cache */
  for (ub4 i = 0; i < region->s_region->assoc; i++)
  {
    block = &(region->s_region->sets[ONLY_SET].policy_data.stride_lru.blocks[i]);
    printf("%ld - %ld - %d\n", block->tag, block->tag_end, block->replacements); 

    access_phase_fini(&(region->phases[i]));
  }
  
  /* Close region output file */
  gzclose(region->out_file);
}


ub4 add_region_cache_block(region_cache *region, ub1 stream_in, ub8 address)
{
  /* Lookup and update region lru-table */
  struct cache_block_t *current_block;
  memory_trace  info;
  ub1 region_miss;
  ub4 reuse_ratio; 

  memset(&info, 0, sizeof(memory_trace));

  info.address = (address / PAGE_SIZE);
  info.stream  = stream_in;
  
  region_miss = FALSE;
  reuse_ratio = 3;
  
  if (stream_in == region->src_stream || stream_in == region->tgt_stream)
  {
    /* Lookup region cache, on miss fill the block */
    current_block = cache_find_block(region->s_region, ONLY_SET, info.address, &info);
    if (!current_block)
    {
      sb4  vctm_way;
      sb4  vctm_stream;
      long long int vctm_tag;

      enum    cache_block_state_t vctm_state;
      struct  cache_block_t       vctm_block; 
      struct  cache_block_t      *current_block; 

      /* If source and target stream of a region are different, access never 
       * misses in the region cache */
      if((region->src_stream != region->tgt_stream && stream_in == region->src_stream) ||
          region->src_stream == region->tgt_stream)
      {
        /* Get replacement candidate */
        vctm_way = cache_replace_block(region->s_region, ONLY_SET, &info);

        /* There should always be a replacement */
        assert(vctm_way != -1);

        vctm_block = cache_get_block(region->s_region, ONLY_SET, vctm_way, &vctm_tag, 
            &vctm_state, &vctm_stream);

        if (vctm_state != cache_block_invalid)
        {
          /* Fill block in the cache */
          cache_set_block(region->s_region, ONLY_SET, vctm_way, vctm_tag, 
              cache_block_invalid, NN, &info);
        }

        /* Fill block in the cache */
        cache_fill_block(region->s_region, ONLY_SET, vctm_way, info.address, 
            cache_block_exclusive, info.stream, &info);
        
        /* First reset the phase table, then update phase table */
        reset_phase(&(region->phases[vctm_way]));

        add_phase_block(&(region->phases[vctm_way]), stream_in, TRUE, address);

        set_access_phase_start_address(&(region->phases[vctm_way]), info.address);
        set_access_phase_end_address(&(region->phases[vctm_way]), info.address);

        reuse_ratio = get_phase_reuse_ratio(&(region->phases[vctm_way]), 
            stream_in, address);
      }
    }
    else
    {
      /* If region is expanded */
      if (info.address < current_block->tag || info.address > current_block->tag_end)
      {
        region_miss = TRUE;

        if (info.address < current_block->tag)
        {
          set_access_phase_start_address(&(region->phases[current_block->way]), info.address);
        }

        if (info.address > current_block->tag_end)
        {
          set_access_phase_end_address(&(region->phases[current_block->way]), info.address);
        }

        reset_phase(&(region->phases[current_block->way]));
      }

      /* Update recency of the block and expand the region if required */
      cache_access_block(region->s_region, ONLY_SET, current_block->way, 
          info.stream, &info);

      add_phase_block(&(region->phases[current_block->way]), stream_in, 
          region_miss, address);

      reuse_ratio = get_phase_reuse_ratio(&(region->phases[current_block->way]),
          stream_in, address);
    }
  }

  return reuse_ratio;
}

ub4 get_region_reuse_ratio(region_cache *region, ub1 stream_in, ub8 address)
{
  struct cache_block_t *current_block;
  ub4    reuse_ratio;
  memory_trace info;

  memset(&info, 0, sizeof(memory_trace));

  info.address  = (address / PAGE_SIZE);
  info.stream   = stream_in;

  reuse_ratio   = 3;

  current_block = cache_find_block(region->s_region, ONLY_SET, info.address, &info);
  if (current_block)
  {
    reuse_ratio = get_phase_reuse_ratio(&(region->phases[current_block->way]),
      stream_in, address);
  }

  return reuse_ratio;
}

#undef ONLY_SET
#undef ACCESS_IS_MISS
#undef ACCESS_IS_HIT
#undef BLOCK_SIZE
#undef PAGE_SIZE
