#include "stream-region.h"
#include "region-cache.h"
#include <string.h>

extern region_cache *cc_regions;                  /* Color access regions */
extern region_cache *ct_regions;                  /* Color access regions */
extern region_cache *bt_regions;                  /* Blitter access regions */
extern region_cache *bb_regions;                  /* Blitter access regions */
extern region_cache *zz_regions;                  /* Depth access regions */
extern region_cache *zt_regions;                  /* Depth access regions */
extern region_cache *tt_regions;                  /* Depth access regions */

ub4 add_region_block(ub1 src_stream, ub1 stream, ub8 address)
{
  ub4 ret;

  switch (src_stream)
  {
    case CS:
      if (src_stream != stream)
      {
        ct_regions->tgt_access++;
      }
      else
      {
        ct_regions->src_access++;
      }
      
      if (src_stream != stream)
      {
        ret = add_region_cache_block(ct_regions, stream, address);
      }
      else
      {
        add_region_cache_block(ct_regions, stream, address);
        ret = add_region_cache_block(cc_regions, stream, address);
      }
      break;
      
    case BS:
      if (src_stream != stream)
      {
        bt_regions->tgt_access++;
      }
      else
      {
        bt_regions->src_access++;
      }
      
      if (src_stream != stream)
      {
        ret = add_region_cache_block(bt_regions, stream, address);
      }
      else
      {
        add_region_cache_block(bt_regions, stream, address);
        ret = add_region_cache_block(bb_regions, stream, address);
      }
      break;

    case ZS:
      if (src_stream != stream)
      {
        zt_regions->tgt_access++;
      }
      else
      {
        zt_regions->src_access++;
      }
      
      if (src_stream != stream)
      {
        ret = add_region_cache_block(zt_regions, stream, address);
      }
      else
      {
        add_region_cache_block(zt_regions, stream, address);
        ret = add_region_cache_block(zz_regions, stream, address);
      }

      break;

    case TS:
      ret = add_region_cache_block(tt_regions, stream, address);
      break;

    default:
      assert(1);
  }

  return ret;
}

ub4 get_xstream_reuse_ratio(ub1 src_stream, ub1 stream, ub8 address)
{
  ub4 ret;

  switch (src_stream)
  {
    case CS:
      if (src_stream != stream)
      {
        ret = get_region_reuse_ratio(ct_regions, stream, address);
      }
      else
      {
        ret = get_region_reuse_ratio(cc_regions, stream, address);
      }
      break;
      
    case BS:
      if (src_stream != stream)
      {
        ret = get_region_reuse_ratio(bt_regions, stream, address);
      }
      else
      {
        ret = get_region_reuse_ratio(bb_regions, stream, address);
      }
      break;

    case ZS:
      if (src_stream != stream)
      {
        ret = get_region_reuse_ratio(zt_regions, stream, address);
      }
      else
      {
        ret = get_region_reuse_ratio(zz_regions, stream, address);
      }

      break;

    case TS:
      ret = get_region_reuse_ratio(tt_regions, stream, address);
      break;

    default:
      assert(0);
  }

  return ret;
}
