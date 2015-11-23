/*
 *  Copyright (C) 2011  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 */

#include <stdlib.h>
#include <assert.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"

#include "cache.h"
#include "region-cache.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))
#define EVENT_SCHEDULE_CYCLE    (500 * 1024)

#define CACHE_GET_BLOCK_STREAM(block, strm)                   \
do                                                            \
{                                                             \
  strm = (block)->stream;                                     \
}while(0)

#define CACHE_UPDATE_BLOCK_STREAM(block, strm)                \
do                                                            \
{                                                             \
  (block)->stream = strm;                                     \
}while(0)

/*
 * Public Variables
 */

struct str_map_t cache_policy_map = {
        35,
        {
                { "BYPASS", cache_policy_bypass},
                { "LRU", cache_policy_lru},
                { "NRU", cache_policy_nru},
                { "UCP", cache_policy_ucp},
                { "TAPUCP", cache_policy_tapucp},
                { "TAPDRRIP", cache_policy_tapdrrip},
                { "HELMDRRIP", cache_policy_helmdrrip},
                { "FIFO", cache_policy_fifo},
                { "SRRIP", cache_policy_srrip},
                { "SRRIPBS", cache_policy_srripbs},
                { "SRRIPDBP", cache_policy_srripdbp},
                { "SRRIPSAGE", cache_policy_srripsage},
                { "PASRRIP", cache_policy_pasrrip},
                { "XSP", cache_policy_xsp},
                { "XSPPIN", cache_policy_xsppin},
                { "STREAMPIN", cache_policy_streampin},
                { "BRRIP", cache_policy_brrip},
                { "DRRIP", cache_policy_drrip},
                { "GSDRRIP", cache_policy_gsdrrip},
                { "LIP", cache_policy_lip},
                { "BIP", cache_policy_bip},
                { "DIP", cache_policy_dip},
                { "GSPC", cache_policy_gspc},
                { "GSPCM", cache_policy_gspcm},
                { "GSHP", cache_policy_gshp},
                { "SDP", cache_policy_sdp},
                { "STRIDELRU", cache_policy_stridelru},
                { "SAPSIMPLE", cache_policy_sapsimple},
                { "SAPDBP", cache_policy_sapdbp},
                { "SAPPRIORITY", cache_policy_sappriority},
                { "SAPPRIDEPRI", cache_policy_sappridepri},
                { "CUSOMSRRIP", cache_policy_customsrrip},
                { "SHIP", cache_policy_ship},
                { "SARP", cache_policy_sarp},
                { "SRRIPBYPASS", cache_policy_srripbypass},
                { "SRRIPHINT", cache_policy_srriphint}
        }
};

struct str_map_t cache_block_state_map = {
        4,
        {
                { "I", cache_block_invalid},
                { "M", cache_block_modified},
                { "E", cache_block_exclusive},
                { "S", cache_block_shared}
        }
};

region_cache *cc_regions;  /* Color only access regions */
region_cache *ct_regions;  /* Color only access regions */
region_cache *bb_regions;  /* Blitter access regions */
region_cache *bt_regions;  /* Blitter access regions */
region_cache *zt_regions;  /* Depth access regions */
region_cache *zz_regions;  /* Depth access regions */
region_cache *tt_regions;  /* Texture access regions */

/*
 * Private Functions
 */
#define free_list_remove_block(set, blk)                                                \
do                                                                                      \
{                                                                                       \
        /* Check: free list must be non empty as it contains the current block. */      \
        assert((set)->free_head && (set)->free_tail);                                   \
                                                                                        \
        /* Check: current block must be in invalid state */                             \
        assert((blk)->state == cache_block_invalid);                                    \
                                                                                        \
        /* Remove block from free list */                                               \
        if (!(blk)->prev && !(blk)->next) /* Block is the only element in free list */  \
        {                                                                               \
                assert((set)->free_head == (blk) && (set)->free_tail == (blk));         \
                (set)->free_head = NULL;                                                \
                (set)->free_tail = NULL;                                                \
        }                                                                               \
        else if (!(blk)->prev) /* Block is at the head of free list */                  \
        {                                                                               \
                assert((set)->free_head == (blk) && (set)->free_tail != (blk));         \
                (set)->free_head = (blk)->next;                                         \
                (blk)->next->prev = NULL;                                               \
        }                                                                               \
        else if (!(blk)->next) /* Block is at the tail of free list */                  \
        {                                                                               \
                assert((set)->free_head != (blk) && (set)->free_tail == (blk));         \
                (set)->free_tail = (blk)->prev;                                         \
                (blk)->prev->next = NULL;                                               \
        }                                                                               \
        else /* Block some where in middle of free list */                              \
        {                                                                               \
                assert((set)->free_head != (blk) && (set)->free_tail != (blk));         \
                (blk)->prev->next = (blk)->next;                                        \
                (blk)->next->prev = (blk)->prev;                                        \
        }                                                                               \
                                                                                        \
        (blk)->next = NULL;                                                             \
        (blk)->prev = NULL;                                                             \
                                                                                        \
        /* Reset block state */                                                         \
        (blk)->busy = 0;                                                                \
        (blk)->cached = 0;                                                              \
        (blk)->prefetch = 0;                                                            \
}while(0);

extern struct cpu_t *cpu;

struct cache_t* cache_init(struct cache_params *params)
{
  struct cache_t      *cache;
  struct cache_set_t  *cache_set; 

  /* Create cache */
  cache = (struct cache_t *)xcalloc(1, sizeof (struct cache_t));
  assert(cache);

  /* Initialize */
  cache->num_sets = params->num_sets;
  cache->assoc    = params->ways;
  cache->policy   = params->policy;
  
  /* Create array of sets */
  cache->sets = (struct cache_set_t *)xcalloc(cache->num_sets, sizeof (struct cache_set_t));
  assert(cache->num_sets);

  /* Initialize array of sets */
  for (int set = 0; set < cache->num_sets; set++)
  {
    cache_set = CACHE_SET(cache, set);

    /* Initialize policy specific part of a set */
    switch (params->policy)
    {
      case cache_policy_lru:
        cache_init_lru(params, CACHE_SET_DATA_LRU(cache_set));
        break;

      case cache_policy_stridelru:
        cache_init_stridelru(params, CACHE_SET_DATA_STRIDELRU(cache_set));
        break;

      case cache_policy_nru:
        cache_init_nru(params, CACHE_SET_DATA_NRU(cache_set));
        break;

      case cache_policy_ucp:
        cache_init_ucp(set, params, CACHE_SET_DATA_UCP(cache_set), 
            CACHE_UCP_GDATA(cache));
        break;

      case cache_policy_tapucp:
        cache_init_tapucp(set, params, CACHE_SET_DATA_TAPUCP(cache_set), 
            CACHE_TAPUCP_GDATA(cache));
        break;

      case cache_policy_tapdrrip:
        cache_init_tapdrrip(set, params, CACHE_SET_DATA_TAPDRRIP(cache_set), 
            CACHE_TAPDRRIP_GDATA(cache));
        break;

      case cache_policy_helmdrrip:
        cache_init_helmdrrip(set, params, CACHE_SET_DATA_HELMDRRIP(cache_set), 
            CACHE_HELMDRRIP_GDATA(cache));
        break;

      case cache_policy_salru:
        cache_init_salru(params, CACHE_SET_DATA_SALRU(cache_set));
        break;

      case cache_policy_fifo:
        cache_init_fifo(params, CACHE_SET_DATA_FIFO(cache_set));
        break;

      case cache_policy_srrip:
        cache_init_srrip(set, params, CACHE_SET_DATA_SRRIP(cache_set), 
            CACHE_SRRIP_GDATA(cache));
        break;

      case cache_policy_srriphint:
        cache_init_srriphint(set, params, CACHE_SET_DATA_SRRIPHINT(cache_set), 
            CACHE_SRRIPHINT_GDATA(cache));
        break;

      case cache_policy_srripbypass:
        cache_init_srripbypass(set, params, CACHE_SET_DATA_SRRIPBYPASS(cache_set), 
            CACHE_SRRIPBYPASS_GDATA(cache));
        break;

      case cache_policy_customsrrip:
        cache_init_customsrrip(set, params, CACHE_SET_DATA_CUSTOMSRRIP(cache_set), 
            CACHE_CUSTOMSRRIP_GDATA(cache));
        break;

      case cache_policy_srripbs:
        cache_init_srripbs(set, params, CACHE_SET_DATA_SRRIPBS(cache_set), 
            CACHE_SRRIPBS_GDATA(cache));
        break;

      case cache_policy_pasrrip:
        cache_init_pasrrip(set, params, CACHE_SET_DATA_PASRRIP(cache_set),
            CACHE_PASRRIP_GDATA(cache));
        break;

      case cache_policy_xsp:
        cache_init_xsp(set, params, CACHE_SET_DATA_XSP(cache_set),
            CACHE_XSP_GDATA(cache));
        break;

      case cache_policy_xsppin:
        cache_init_xsppin(set, params, CACHE_SET_DATA_XSPPIN(cache_set),
            CACHE_XSPPIN_GDATA(cache));
        break;

      case cache_policy_streampin:
        cache_init_streampin(set, params, CACHE_SET_DATA_STREAMPIN(cache_set),
            CACHE_STREAMPIN_GDATA(cache));
        break;

      case cache_policy_xspdbp:
        cache_init_xspdbp(set, params, CACHE_SET_DATA_XSPDBP(cache_set), 
            CACHE_XSPDBP_GDATA(cache));
        break;

      case cache_policy_srripm:
        cache_init_srripm(params, CACHE_SET_DATA_SRRIPM(cache_set));
        break;

      case cache_policy_srript:
        cache_init_srript(params, CACHE_SET_DATA_SRRIPT(cache_set));
        break;

      case cache_policy_srripdbp:
        cache_init_srripdbp(set, params, CACHE_SET_DATA_SRRIPDBP(cache_set), 
            CACHE_SRRIPDBP_GDATA(cache));
        break;

      case cache_policy_srripsage:
        cache_init_srripsage(set, params, CACHE_SET_DATA_SRRIPSAGE(cache_set), 
            CACHE_SRRIPSAGE_GDATA(cache));
        break;

      case cache_policy_brrip:
        cache_init_brrip(params, CACHE_SET_DATA_BRRIP(cache_set),
            CACHE_BRRIP_GDATA(cache));
        break;

      case cache_policy_drrip:
        cache_init_drrip(set, params, CACHE_SET_DATA_DRRIP(cache_set),
            &((cache->policy_data).drrip));
        break;

      case cache_policy_sapsimple:
        cache_init_sapsimple(set, params, CACHE_SET_DATA_SAPSIMPLE(cache_set),
            &((cache->policy_data).sapsimple));
        break;

      case cache_policy_sapdbp:
        cache_init_sapdbp(set, params, CACHE_SET_DATA_SAPDBP(cache_set),
            &((cache->policy_data).sapdbp));
        break;

      case cache_policy_sappriority:
        cache_init_sappriority(set, params, CACHE_SET_DATA_SAPPRIORITY(cache_set),
            &((cache->policy_data).sappriority));
        break;

      case cache_policy_sarp:
        cache_init_sarp(set, params, CACHE_SET_DATA_SARP(cache_set),
            &((cache->policy_data).sarp));
        break;

      case cache_policy_sappridepri:
        cache_init_sappridepri(set, params, CACHE_SET_DATA_SAPPRIDEPRI(cache_set),
            &((cache->policy_data).sappridepri));
        break;

      case cache_policy_gsdrrip:
        cache_init_gsdrrip(set, params, CACHE_SET_DATA_GSDRRIP(cache_set),
            CACHE_GSDRRIP_GDATA(cache));
        break;

      case cache_policy_lip:
        cache_init_lip(params, CACHE_SET_DATA_LIP(cache_set));
        break;

      case cache_policy_bip:
        cache_init_bip(params, CACHE_SET_DATA_BIP(cache_set));
        break;

      case cache_policy_dip:
        cache_init_dip(set, params, CACHE_SET_DATA_DIP(cache_set),
            CACHE_DIP_GDATA(cache));
        break;

      case cache_policy_gspc:
        cache_init_gspc(set, params, CACHE_SET_DATA_GSPC(cache_set), 
            CACHE_GSPC_GDATA(cache));
        break;

      case cache_policy_gspcm:
        cache_init_gspcm(set, params, CACHE_SET_DATA_GSPCM(cache_set), 
            CACHE_GSPCM_GDATA(cache));
        break;

      case cache_policy_gspct:
        cache_init_gspct(set, params, CACHE_SET_DATA_GSPCT(cache_set), 
            CACHE_GSPCT_GDATA(cache));
        break;

      case cache_policy_gshp:
        cache_init_gshp(set, params, CACHE_SET_DATA_GSHP(cache_set), 
            CACHE_GSHP_GDATA(cache));
        break;

      case cache_policy_sap:
        cache_init_sap(set, params, CACHE_SET_DATA_SAP(cache_set), 
            CACHE_SAP_GDATA(cache));
        break;

      case cache_policy_sdp:
        cache_init_sdp(set, params, CACHE_SET_DATA_SDP(cache_set), 
            CACHE_SDP_GDATA(cache));

        /* Set SDP event schedule cycle */
        CACHE_SDP_GDATA(cache)->next_schedule = EVENT_SCHEDULE_CYCLE; 
        break;

      case cache_policy_ship:
        cache_init_ship(set, params, CACHE_SET_DATA_SHIP(cache_set), 
            CACHE_SHIP_GDATA(cache));

        /* Set SDP event schedule cycle */
        CACHE_SDP_GDATA(cache)->next_schedule = EVENT_SCHEDULE_CYCLE; 
        break;

      case cache_policy_bypass:
      case cache_policy_cpulast:
        /* Nothing to do */
        break;

      case cache_policy_opt:
      case cache_policy_invalid:
        panic("%s: line no %d - invalid polict type", __FUNCTION__, __LINE__);
    }
  }
  
  /* Temporary  set type collection for sampling based policies */
  long following_srrip = 0;
  long following_brrip = 0;

  for (int set = 0; set < cache->num_sets; set++)
  {
    cache_set = CACHE_SET(cache, set);

    if (CACHE_SET_DATA_DRRIP(cache_set)->following == cache_policy_srrip ||
        CACHE_SET_DATA_GSPC(cache_set)->following == cache_policy_srrip||
        CACHE_SET_DATA_GSPCM(cache_set)->following == cache_policy_srrip)    
    {
      following_srrip += 1;
    }
    else
    {
      if (CACHE_SET_DATA_DRRIP(cache_set)->following == cache_policy_brrip || 
          CACHE_SET_DATA_GSPC(cache_set)->following == cache_policy_gspc ||
          CACHE_SET_DATA_GSPCM(cache_set)->following == cache_policy_gspcm)    
      {
        following_brrip += 1;
      }
    }
  }
  
  /* Initialize policy to be used for verification. */
  if (params->policy == cache_policy_srrip || params->policy == cache_policy_drrip)
  {
    DRRIPClassInit(&(cache->drrip_policy), params->num_sets, params->ways);
    SRRIPClassInit(&(cache->srrip_policy), params->num_sets, params->ways);
  }

  if (params->policy == cache_policy_pasrrip || params->policy == cache_policy_xsp || 
      params->policy == cache_policy_xsppin || params->policy == cache_policy_xspdbp)
  {
    /* Allocate color, blitter, depth access region */
    cc_regions = (region_cache *)xcalloc(1, sizeof(region_cache));
    assert(cc_regions);

    region_cache_init(cc_regions, CS, CS);

    ct_regions = (region_cache *)xcalloc(1, sizeof(region_cache));
    assert(ct_regions);

    region_cache_init(ct_regions, CS, TS);

    bb_regions = (region_cache *)xcalloc(1, sizeof(region_cache));
    assert(bb_regions);

    region_cache_init(bb_regions, BS, BS);

    bt_regions = (region_cache *)xcalloc(1, sizeof(region_cache));
    assert(bt_regions);

    region_cache_init(bt_regions, BS, TS);

    zz_regions = (region_cache *)xcalloc(1, sizeof(region_cache));
    assert(zz_regions);

    region_cache_init(zz_regions, ZS, ZS);

    zt_regions = (region_cache *)xcalloc(1, sizeof(region_cache));
    assert(zt_regions);

    region_cache_init(zt_regions, ZS, TS);

    tt_regions = (region_cache *)xcalloc(1, sizeof(region_cache));
    assert(tt_regions);

    region_cache_init(tt_regions, TS, TS);
  }

  printf("Following srrip %ld brrip %ld \n", following_srrip, following_brrip);

  return cache;
}

void cache_free(struct cache_t *cache)
{
  struct cache_set_t *cache_set;

  /* Initialize array of sets */
  for (int set = 0; set < cache->num_sets; set++)
  {
    cache_set = CACHE_SET(cache, set);
    
    /* Initialize policy specific part of a set */
    switch (cache->policy)
    {
      case cache_policy_lru:
        cache_free_lru(CACHE_SET_DATA_LRU(cache_set));
        break;

      case cache_policy_stridelru:
        cache_free_stridelru(CACHE_SET_DATA_STRIDELRU(cache_set));
        break;

      case cache_policy_nru:
        cache_free_nru(CACHE_SET_DATA_NRU(cache_set));
        break;

      case cache_policy_ucp:
        cache_free_ucp(set, CACHE_SET_DATA_UCP(cache_set), CACHE_UCP_GDATA(cache));
        break;

      case cache_policy_tapucp:
        cache_free_tapucp(set, CACHE_SET_DATA_TAPUCP(cache_set), 
          CACHE_TAPUCP_GDATA(cache));
        break;

      case cache_policy_tapdrrip:
        cache_free_tapdrrip(CACHE_SET_DATA_TAPDRRIP(cache_set));
        break;

      case cache_policy_helmdrrip:
        cache_free_helmdrrip(CACHE_SET_DATA_HELMDRRIP(cache_set));
        break;

      case cache_policy_salru:
        cache_free_salru(CACHE_SET_DATA_SALRU(cache_set));
        break;

      case cache_policy_fifo:
        cache_free_fifo(CACHE_SET_DATA_FIFO(cache_set));
        break;

      case cache_policy_srrip:
        cache_free_srrip(CACHE_SET_DATA_SRRIP(cache_set));
        break;

      case cache_policy_srriphint:
        cache_free_srriphint(CACHE_SET_DATA_SRRIPHINT(cache_set));
        break;

      case cache_policy_srripbypass:
        cache_free_srripbypass(CACHE_SET_DATA_SRRIPBYPASS(cache_set));
        break;

      case cache_policy_customsrrip:
        cache_free_customsrrip(CACHE_SET_DATA_CUSTOMSRRIP(cache_set));
        break;

      case cache_policy_srripbs:
        cache_free_srripbs(CACHE_SET_DATA_SRRIPBS(cache_set));
        break;

      case cache_policy_pasrrip:
        cache_free_pasrrip(CACHE_SET_DATA_PASRRIP(cache_set));
        break;

      case cache_policy_xsp:
        cache_free_xsp(CACHE_SET_DATA_XSP(cache_set));
        break;

      case cache_policy_xsppin:
        cache_free_xsppin(CACHE_SET_DATA_XSPPIN(cache_set));
        break;

      case cache_policy_streampin:
        cache_free_streampin(CACHE_SET_DATA_STREAMPIN(cache_set));
        break;

      case cache_policy_xspdbp:
        cache_free_xspdbp(CACHE_SET_DATA_XSPDBP(cache_set), 
          CACHE_XSPDBP_GDATA(cache));
        break;

      case cache_policy_srripm:
        cache_free_srripm(CACHE_SET_DATA_SRRIPM(cache_set));
        break;

      case cache_policy_srript:
        cache_free_srript(CACHE_SET_DATA_SRRIPT(cache_set));
        break;

      case cache_policy_srripdbp:
        cache_free_srripdbp(CACHE_SET_DATA_SRRIPDBP(cache_set), 
          CACHE_SRRIPDBP_GDATA(cache));
        break;

      case cache_policy_srripsage:
        cache_free_srripsage(CACHE_SET_DATA_SRRIPSAGE(cache_set));
        break;

      case cache_policy_brrip:
        cache_free_brrip(CACHE_SET_DATA_BRRIP(cache_set));
        break;

      case cache_policy_drrip:
        cache_free_drrip(set, CACHE_SET_DATA_DRRIP(cache_set), 
          CACHE_DRRIP_GDATA(cache));
        break;

      case cache_policy_sapsimple:
        cache_free_sapsimple(set, CACHE_SET_DATA_SAPSIMPLE(cache_set), 
          CACHE_SAPSIMPLE_GDATA(cache));
        break;

      case cache_policy_sapdbp:
        cache_free_sapdbp(set, CACHE_SET_DATA_SAPDBP(cache_set), 
          CACHE_SAPDBP_GDATA(cache));
        break;

      case cache_policy_sappriority:
        cache_free_sappriority(set, CACHE_SET_DATA_SAPPRIORITY(cache_set), 
          CACHE_SAPPRIORITY_GDATA(cache));
        break;

      case cache_policy_sarp:
        cache_free_sarp(set, CACHE_SET_DATA_SARP(cache_set), 
          CACHE_SARP_GDATA(cache));
        break;

      case cache_policy_sappridepri:
        cache_free_sappridepri(set, CACHE_SET_DATA_SAPPRIDEPRI(cache_set), 
          CACHE_SAPPRIDEPRI_GDATA(cache));
        break;

      case cache_policy_gsdrrip:
        cache_free_gsdrrip(set, CACHE_SET_DATA_GSDRRIP(cache_set), 
          CACHE_GSDRRIP_GDATA(cache));
        break;

      case cache_policy_lip:
        cache_free_lip(CACHE_SET_DATA_LIP(cache_set));
        break;

      case cache_policy_bip:
        cache_free_bip(CACHE_SET_DATA_BIP(cache_set));
        break;

      case cache_policy_dip:
        cache_free_dip(CACHE_SET_DATA_DIP(cache_set));
        break;

      case cache_policy_gspc:
        cache_free_gspc(CACHE_SET_DATA_GSPC(cache_set), 
          CACHE_GSPC_GDATA(cache));
        break;

      case cache_policy_gspcm:
        cache_free_gspcm(CACHE_SET_DATA_GSPCM(cache_set), 
          CACHE_GSPCM_GDATA(cache));
        break;

      case cache_policy_gspct:
        cache_free_gspct(CACHE_SET_DATA_GSPCT(cache_set), 
          CACHE_GSPCT_GDATA(cache));
        break;

      case cache_policy_gshp:
        cache_free_gshp(CACHE_SET_DATA_GSHP(cache_set));
        break;

      case cache_policy_sap:
        cache_free_sap(set, CACHE_SET_DATA_SAP(cache_set), CACHE_SAP_GDATA(cache));
        break;

      case cache_policy_sdp:
        cache_free_sdp(set, CACHE_SET_DATA_SDP(cache_set), CACHE_SDP_GDATA(cache));
        break;

      case cache_policy_ship:
        cache_free_ship(CACHE_SET_DATA_SHIP(cache_set));
        break;

      case cache_policy_bypass:
      case cache_policy_cpulast:
        /* Nothing to do */
        break;

      case cache_policy_opt:
      case cache_policy_invalid:
        panic("%s: line no %d - invalid polict type", __FUNCTION__, __LINE__);
    }
  }
    
  if (cache->policy == cache_policy_pasrrip || cache->policy == cache_policy_xsp ||
      cache->policy == cache_policy_xsppin || cache->policy == cache_policy_xspdbp)
  {
    region_cache_fini(cc_regions);
    region_cache_fini(ct_regions);
    region_cache_fini(bb_regions);
    region_cache_fini(bt_regions);
    region_cache_fini(zz_regions);
    region_cache_fini(zt_regions);
    region_cache_fini(tt_regions);
  }
}

struct cache_block_t * cache_find_block(struct cache_t *cache, int set,
  long long tag, memory_trace *info)
{

  switch (cache->policy)
  {
    case cache_policy_lru:
      return cache_find_block_lru(CACHE_SET_DATA_LRU(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_stridelru:
      return cache_find_block_stridelru(CACHE_SET_DATA_STRIDELRU(CACHE_SET(cache, set)),
          tag, info);
      break;

    case cache_policy_nru:
      return cache_find_block_nru(CACHE_SET_DATA_NRU(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_ucp:
      return cache_find_block_ucp(CACHE_SET_DATA_UCP(CACHE_SET(cache, set)),
        CACHE_UCP_GDATA(cache), tag, info);
      break;

    case cache_policy_tapucp:
      return cache_find_block_tapucp(CACHE_SET_DATA_TAPUCP(CACHE_SET(cache, set)),
        CACHE_TAPUCP_GDATA(cache), tag, info);
      break;

    case cache_policy_tapdrrip:
      return cache_find_block_tapdrrip(CACHE_TAPDRRIP_GDATA(cache), 
          CACHE_SET_DATA_TAPDRRIP(CACHE_SET(cache, set)), tag, info);
      break;

    case cache_policy_helmdrrip:
      return cache_find_block_helmdrrip(CACHE_HELMDRRIP_GDATA(cache), 
          CACHE_SET_DATA_HELMDRRIP(CACHE_SET(cache, set)), tag, info);
      break;

    case cache_policy_salru:
      return cache_find_block_salru(CACHE_SET_DATA_SALRU(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_fifo:
      return cache_find_block_fifo(CACHE_SET_DATA_FIFO(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_srrip:
      return cache_find_block_srrip(CACHE_SET_DATA_SRRIP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_srriphint:
      return cache_find_block_srriphint(CACHE_SET_DATA_SRRIPHINT(CACHE_SET(cache, set)), 
          CACHE_SRRIPHINT_GDATA(cache), tag);
      break;

    case cache_policy_srripbypass:
      return cache_find_block_srripbypass(CACHE_SET_DATA_SRRIPBYPASS(CACHE_SET(cache, set)),
          CACHE_SRRIPBYPASS_GDATA(cache), info, tag);
      break;

    case cache_policy_customsrrip:
      return cache_find_block_customsrrip(CACHE_SET_DATA_CUSTOMSRRIP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_srripbs:
      return cache_find_block_srripbs(CACHE_SET_DATA_SRRIPBS(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_srripm:
      return cache_find_block_srripm(CACHE_SET_DATA_SRRIPM(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_srript:
      return cache_find_block_srript(CACHE_SET_DATA_SRRIPT(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_srripdbp:
      return cache_find_block_srripdbp(CACHE_SET_DATA_SRRIPDBP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_srripsage:
      return cache_find_block_srripsage(CACHE_SET_DATA_SRRIPSAGE(CACHE_SET(cache, set)), 
          CACHE_SRRIPSAGE_GDATA(cache), tag, info->stream, info);
      break;

    case cache_policy_pasrrip:
      return cache_find_block_pasrrip(CACHE_SET_DATA_PASRRIP(CACHE_SET(cache, set)), 
          CACHE_PASRRIP_GDATA(cache), tag, info);
      break;

    case cache_policy_xsp:
      return cache_find_block_xsp(CACHE_SET_DATA_XSP(CACHE_SET(cache, set)), 
          CACHE_XSP_GDATA(cache), tag, info);
      break;

    case cache_policy_xsppin:
      return cache_find_block_xsppin(CACHE_SET_DATA_XSPPIN(CACHE_SET(cache, set)), 
          CACHE_XSPPIN_GDATA(cache), tag, info);
      break;

    case cache_policy_streampin:
      return cache_find_block_streampin(CACHE_SET_DATA_STREAMPIN(CACHE_SET(cache, set)), 
          CACHE_STREAMPIN_GDATA(cache), tag, info);
      break;

    case cache_policy_xspdbp:
      return cache_find_block_xspdbp(CACHE_SET_DATA_XSPDBP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_brrip:
      return cache_find_block_brrip(CACHE_SET_DATA_BRRIP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_drrip:
      return cache_find_block_drrip(CACHE_SET_DATA_DRRIP(CACHE_SET(cache, set)), 
          CACHE_DRRIP_GDATA(cache), tag, info);
      break;

    case cache_policy_sapsimple:
      return cache_find_block_sapsimple(CACHE_SET_DATA_SAPSIMPLE(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_sapdbp:
      return cache_find_block_sapdbp(CACHE_SET_DATA_SAPDBP(CACHE_SET(cache, set)),
          CACHE_SAPDBP_GDATA(cache), info, tag);
      break;

    case cache_policy_sappriority:
      return cache_find_block_sappriority(CACHE_SET_DATA_SAPPRIORITY(CACHE_SET(cache, set)),
          CACHE_SAPPRIORITY_GDATA(cache), info, tag);
      break;

    case cache_policy_sarp:
      return cache_find_block_sarp(CACHE_SET_DATA_SARP(CACHE_SET(cache, set)),
          CACHE_SARP_GDATA(cache), tag, info);
      break;

    case cache_policy_sappridepri:
      return cache_find_block_sappridepri(CACHE_SET_DATA_SAPPRIDEPRI(CACHE_SET(cache, set)),
          CACHE_SAPPRIDEPRI_GDATA(cache), info, tag);
      break;

    case cache_policy_gsdrrip:
      return cache_find_block_gsdrrip(CACHE_SET_DATA_GSDRRIP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_lip:
      return cache_find_block_lip(CACHE_SET_DATA_LIP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_bip:
      return cache_find_block_bip(CACHE_SET_DATA_BIP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_dip:
      return cache_find_block_dip(CACHE_SET_DATA_DIP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_gspc:
      return cache_find_block_gspc(CACHE_SET_DATA_GSPC(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_gspcm:
      return cache_find_block_gspcm(CACHE_SET_DATA_GSPCM(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_gspct:
      return cache_find_block_gspct(CACHE_SET_DATA_GSPCT(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_gshp:
      return cache_find_block_gshp(CACHE_SET_DATA_GSHP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_sap:
      return cache_find_block_sap(CACHE_SET_DATA_SAP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_sdp:
      return cache_find_block_sdp(CACHE_SET_DATA_SDP(CACHE_SET(cache, set)), tag, info);
      break;

    case cache_policy_ship:
      return cache_find_block_ship(CACHE_SET_DATA_SHIP(CACHE_SET(cache, set)), tag);
      break;

    case cache_policy_bypass:
    case cache_policy_cpulast:
      /* Nothing to do */
      break;

    case cache_policy_opt:
    case cache_policy_invalid:
      panic("%s: line no %d - invalid polict type", __FUNCTION__, __LINE__);
  }

  return NULL;
}


void cache_fill_block( struct cache_t *cache, int set, int way, 
  long long tag, enum cache_block_state_t state, int stream, memory_trace *info)
{
  switch(cache->policy)
  {
    case cache_policy_lru:
      cache_fill_block_lru(CACHE_SET_DATA_LRU(CACHE_SET(cache, set)), way,
          tag, state, stream, info);
      break;

    case cache_policy_stridelru:
      cache_fill_block_stridelru(CACHE_SET_DATA_STRIDELRU(CACHE_SET(cache, set)), way,
          tag, state, stream, info);
      break;

    case cache_policy_nru:
      cache_fill_block_nru(CACHE_SET_DATA_NRU(CACHE_SET(cache, set)), way,
          tag, state, stream, info);
      break;

    case cache_policy_ucp:
      cache_fill_block_ucp(CACHE_SET_DATA_UCP(CACHE_SET(cache, set)),
          CACHE_UCP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_tapucp:
      cache_fill_block_tapucp(CACHE_SET_DATA_TAPUCP(CACHE_SET(cache, set)),
          CACHE_TAPUCP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_tapdrrip:
      cache_fill_block_tapdrrip(CACHE_SET_DATA_TAPDRRIP(CACHE_SET(cache, set)),
          CACHE_TAPDRRIP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_helmdrrip:
      cache_fill_block_helmdrrip(CACHE_SET_DATA_HELMDRRIP(CACHE_SET(cache, set)),
          CACHE_HELMDRRIP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_fifo:
      cache_fill_block_fifo(CACHE_SET_DATA_FIFO(CACHE_SET(cache, set)), way,
          tag, state, stream, info);
      break;

    case cache_policy_salru:
      cache_fill_block_salru(CACHE_SET_DATA_SALRU(CACHE_SET(cache, set)), way,
          tag, state, stream, info);
      break;

    case cache_policy_srrip:
      cache_fill_block_srrip(CACHE_SET_DATA_SRRIP(CACHE_SET(cache, set)), 
          CACHE_SRRIP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_srriphint:
      cache_fill_block_srriphint(CACHE_SET_DATA_SRRIPHINT(CACHE_SET(cache, set)), 
          CACHE_SRRIPHINT_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_srripbypass:
      cache_fill_block_srripbypass(CACHE_SET_DATA_SRRIPBYPASS(CACHE_SET(cache, set)), 
          CACHE_SRRIPBYPASS_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_customsrrip:
      cache_fill_block_customsrrip(CACHE_SET_DATA_CUSTOMSRRIP(CACHE_SET(cache, set)), 
          CACHE_CUSTOMSRRIP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_srripbs:
      cache_fill_block_srripbs(CACHE_SET_DATA_SRRIPBS(CACHE_SET(cache, set)), 
          CACHE_SRRIPBS_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_pasrrip:
      cache_fill_block_pasrrip(CACHE_SET_DATA_PASRRIP(CACHE_SET(cache, set)), 
          CACHE_PASRRIP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_xsp:
      cache_fill_block_xsp(CACHE_SET_DATA_XSP(CACHE_SET(cache, set)), 
          CACHE_XSP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_xsppin:
      cache_fill_block_xsppin(CACHE_SET_DATA_XSPPIN(CACHE_SET(cache, set)), 
          CACHE_XSPPIN_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_streampin:
      cache_fill_block_streampin(CACHE_SET_DATA_STREAMPIN(CACHE_SET(cache, set)), 
          CACHE_STREAMPIN_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_xspdbp:
      cache_fill_block_xspdbp(CACHE_SET_DATA_XSPDBP(CACHE_SET(cache, set)),
          CACHE_XSPDBP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_srripm:
      cache_fill_block_srripm(CACHE_SET_DATA_SRRIPM(CACHE_SET(cache, set)), way,
          tag, state, stream, info);
      break;

    case cache_policy_srript:
      cache_fill_block_srript(CACHE_SET_DATA_SRRIPT(CACHE_SET(cache, set)), way,
          tag, state, stream, info);
      break;

    case cache_policy_srripdbp:
      cache_fill_block_srripdbp(CACHE_SET_DATA_SRRIPDBP(CACHE_SET(cache, set)),
          CACHE_SRRIPDBP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_srripsage:
      cache_fill_block_srripsage(CACHE_SET_DATA_SRRIPSAGE(CACHE_SET(cache, set)),
          CACHE_SRRIPSAGE_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_brrip:
      cache_fill_block_brrip(CACHE_SET_DATA_BRRIP(CACHE_SET(cache, set)), 
        CACHE_BRRIP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_drrip:
      cache_fill_block_drrip(CACHE_SET_DATA_DRRIP(CACHE_SET(cache, set)), 
          CACHE_DRRIP_GDATA(cache), way, tag, state, stream, info);

#if 0
      /* For verification */
      DRRIPUpdateOnMiss(&(cache->drrip_policy), set);
      DRRIPUpdateOnFill(&(cache->drrip_policy), set, way);
      assert(SAT_CTR_VAL(CACHE_DRRIP_GDATA(cache)->psel) == cache->drrip_policy._DIPchooser);
      assert(SAT_CTR_VAL(CACHE_DRRIP_GDATA(cache)->brrip.access_ctr) == cache->drrip_policy._BIPcounter);
#endif
      break;

    case cache_policy_sapsimple:
      cache_fill_block_sapsimple(CACHE_SET_DATA_SAPSIMPLE(CACHE_SET(cache, set)), 
          CACHE_SAPSIMPLE_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_sapdbp:
      cache_fill_block_sapdbp(CACHE_SET_DATA_SAPDBP(CACHE_SET(cache, set)), 
          CACHE_SAPDBP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_sappriority:
      cache_fill_block_sappriority(CACHE_SET_DATA_SAPPRIORITY(CACHE_SET(cache, set)), 
          CACHE_SAPPRIORITY_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_sarp:
      cache_fill_block_sarp(CACHE_SET_DATA_SARP(CACHE_SET(cache, set)), 
          CACHE_SARP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_sappridepri:
      cache_fill_block_sappridepri(CACHE_SET_DATA_SAPPRIDEPRI(CACHE_SET(cache, set)), 
          CACHE_SAPPRIDEPRI_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_gsdrrip:
      cache_fill_block_gsdrrip(CACHE_SET_DATA_GSDRRIP(CACHE_SET(cache, set)), 
          CACHE_GSDRRIP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_lip:
      cache_fill_block_lip(CACHE_SET_DATA_LIP(CACHE_SET(cache, set)), way,
          tag, state, stream, info);
      break;

    case cache_policy_bip:
      cache_fill_block_bip(CACHE_SET_DATA_BIP(CACHE_SET(cache, set)), way, 
          tag, state, stream, info);
      break;

    case cache_policy_dip:
      cache_fill_block_dip(CACHE_SET_DATA_DIP(CACHE_SET(cache, set)), 
          CACHE_DIP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_gspc:
      cache_fill_block_gspc(CACHE_SET_DATA_GSPC(CACHE_SET(cache, set)),
          CACHE_GSPC_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_gspcm:
      cache_fill_block_gspcm(CACHE_SET_DATA_GSPCM(CACHE_SET(cache, set)),
          CACHE_GSPCM_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_gspct:
      cache_fill_block_gspct(CACHE_SET_DATA_GSPCT(CACHE_SET(cache, set)),
          CACHE_GSPCT_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_gshp:
      cache_fill_block_gshp(CACHE_SET_DATA_GSHP(CACHE_SET(cache, set)),
          CACHE_GSHP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_sap:
      cache_fill_block_sap(CACHE_SET_DATA_SAP(CACHE_SET(cache, set)),
           CACHE_SAP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_sdp:
      cache_fill_block_sdp(CACHE_SET_DATA_SDP(CACHE_SET(cache, set)),
          CACHE_SDP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_ship:
      cache_fill_block_ship(CACHE_SET_DATA_SHIP(CACHE_SET(cache, set)),
          CACHE_SHIP_GDATA(cache), way, tag, state, stream, info);
      break;

    case cache_policy_bypass:
    case cache_policy_cpulast:
      /* Nothing to do */
      break;

    case cache_policy_opt:
    case cache_policy_invalid:
      panic("%s: line no %d - invalid polict type", __FUNCTION__, __LINE__);
  }
}

void cache_access_block(struct cache_t *cache, int set, int way, int stream, 
  memory_trace *info)
{
  switch(cache->policy)
  {
    case cache_policy_lru:
      cache_access_block_lru(CACHE_SET_DATA_LRU(CACHE_SET(cache, set)), way,
        stream, info);
      break;

    case cache_policy_stridelru:
      cache_access_block_stridelru(CACHE_SET_DATA_STRIDELRU(CACHE_SET(cache, set)), way,
        stream, info);
      break;

    case cache_policy_nru:
      cache_access_block_nru(CACHE_SET_DATA_NRU(CACHE_SET(cache, set)), way,
        stream, info);
      break;

    case cache_policy_ucp:
      cache_access_block_ucp(CACHE_SET_DATA_UCP(CACHE_SET(cache, set)),
          CACHE_UCP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_tapucp:
      cache_access_block_tapucp(CACHE_SET_DATA_TAPUCP(CACHE_SET(cache, set)),
          CACHE_TAPUCP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_tapdrrip:
      cache_access_block_tapdrrip(CACHE_SET_DATA_TAPDRRIP(CACHE_SET(cache, set)),
          CACHE_TAPDRRIP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_helmdrrip:
      cache_access_block_helmdrrip(CACHE_SET_DATA_HELMDRRIP(CACHE_SET(cache, set)),
          CACHE_HELMDRRIP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_salru:
      cache_access_block_salru(CACHE_SET_DATA_SALRU(CACHE_SET(cache, set)), way,
        stream, info);
      break;

    case cache_policy_fifo:
      cache_access_block_fifo(CACHE_SET_DATA_FIFO(CACHE_SET(cache, set)), way,
        stream, info);
      break;

    case cache_policy_srrip:
      cache_access_block_srrip(CACHE_SET_DATA_SRRIP(CACHE_SET(cache, set)), 
          CACHE_SRRIP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_srriphint:
      cache_access_block_srriphint(CACHE_SET_DATA_SRRIPHINT(CACHE_SET(cache, set)), 
          CACHE_SRRIPHINT_GDATA(cache), way, stream, info);
      break;

    case cache_policy_srripbypass:
      cache_access_block_srripbypass(CACHE_SET_DATA_SRRIPBYPASS(CACHE_SET(cache, set)), 
          CACHE_SRRIPBYPASS_GDATA(cache), way, stream, info);
      break;

    case cache_policy_customsrrip:
      cache_access_block_customsrrip(CACHE_SET_DATA_CUSTOMSRRIP(CACHE_SET(cache, set)), 
          CACHE_CUSTOMSRRIP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_srripbs:
      cache_access_block_srripbs(CACHE_SET_DATA_SRRIPBS(CACHE_SET(cache, set)), 
          CACHE_SRRIPBS_GDATA(cache), way, stream, info);
      break;

    case cache_policy_pasrrip:
      cache_access_block_pasrrip(CACHE_SET_DATA_PASRRIP(CACHE_SET(cache, set)), 
          CACHE_PASRRIP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_xsp:
      cache_access_block_xsp(CACHE_SET_DATA_XSP(CACHE_SET(cache, set)), 
          CACHE_XSP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_xsppin:
      cache_access_block_xsppin(CACHE_SET_DATA_XSPPIN(CACHE_SET(cache, set)), 
          CACHE_XSPPIN_GDATA(cache), way, stream, info);
      break;

    case cache_policy_streampin:
      cache_access_block_streampin(CACHE_SET_DATA_STREAMPIN(CACHE_SET(cache, set)), 
          CACHE_STREAMPIN_GDATA(cache), way, stream, info);
      break;

    case cache_policy_xspdbp:
      cache_access_block_xspdbp(CACHE_SET_DATA_XSPDBP(CACHE_SET(cache, set)), 
        CACHE_XSPDBP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_srripm:
      cache_access_block_srripm(CACHE_SET_DATA_SRRIPM(CACHE_SET(cache, set)), way,
        stream, info);
      break;

    case cache_policy_srript:
      cache_access_block_srript(CACHE_SET_DATA_SRRIPT(CACHE_SET(cache, set)), way,
        stream, info);
      break;

    case cache_policy_srripdbp:
      cache_access_block_srripdbp(CACHE_SET_DATA_SRRIPDBP(CACHE_SET(cache, set)), 
        CACHE_SRRIPDBP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_srripsage:
      cache_access_block_srripsage(CACHE_SET_DATA_SRRIPSAGE(CACHE_SET(cache, set)), 
         CACHE_SRRIPSAGE_GDATA(cache), way, stream, info);
      break;

    case cache_policy_brrip:
      cache_access_block_brrip(CACHE_SET_DATA_BRRIP(CACHE_SET(cache, set)), 
        CACHE_BRRIP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_drrip:
      cache_access_block_drrip(CACHE_SET_DATA_DRRIP(CACHE_SET(cache, set)), 
        CACHE_DRRIP_GDATA(cache), way, stream, info);
#if 0
      /* For verification */
      DRRIPUpdateOnHit(&(cache->drrip_policy), set, way);
#endif
      break;

    case cache_policy_sapsimple:
      cache_access_block_sapsimple(CACHE_SET_DATA_SAPSIMPLE(CACHE_SET(cache, set)), 
        CACHE_SAPSIMPLE_GDATA(cache), way, stream, info);
      break;

    case cache_policy_sapdbp:
      cache_access_block_sapdbp(CACHE_SET_DATA_SAPDBP(CACHE_SET(cache, set)), 
        CACHE_SAPDBP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_sappriority:
      cache_access_block_sappriority(CACHE_SET_DATA_SAPPRIORITY(CACHE_SET(cache, set)), 
        CACHE_SAPPRIORITY_GDATA(cache), way, stream, info);
      break;

    case cache_policy_sarp:
      cache_access_block_sarp(CACHE_SET_DATA_SARP(CACHE_SET(cache, set)), 
        CACHE_SARP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_sappridepri:
      cache_access_block_sappridepri(CACHE_SET_DATA_SAPPRIDEPRI(CACHE_SET(cache, set)), 
        CACHE_SAPPRIDEPRI_GDATA(cache), way, stream, info);
      break;

    case cache_policy_gsdrrip:
      cache_access_block_gsdrrip(CACHE_SET_DATA_GSDRRIP(CACHE_SET(cache, set)), 
        CACHE_GSDRRIP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_lip:
      cache_access_block_lip(CACHE_SET_DATA_LIP(CACHE_SET(cache, set)), way,
        stream, info);
      break;

    case cache_policy_bip:
      cache_access_block_bip(CACHE_SET_DATA_BIP(CACHE_SET(cache, set)), way, 
        stream, info);
      break;

    case cache_policy_dip:
      cache_access_block_dip(CACHE_SET_DATA_DIP(CACHE_SET(cache, set)), 
        CACHE_DIP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_gspc:
      cache_access_block_gspc(CACHE_SET_DATA_GSPC(CACHE_SET(cache, set)), 
        CACHE_GSPC_GDATA(cache), way, stream, info);
      break;

    case cache_policy_gspcm:
      cache_access_block_gspcm(CACHE_SET_DATA_GSPCM(CACHE_SET(cache, set)), 
        CACHE_GSPCM_GDATA(cache), way, stream, info);
      break;

    case cache_policy_gspct:
      cache_access_block_gspct(CACHE_SET_DATA_GSPCT(CACHE_SET(cache, set)), 
        CACHE_GSPCT_GDATA(cache), way, stream, info);
      break;

    case cache_policy_gshp:
      cache_access_block_gshp(CACHE_SET_DATA_GSHP(CACHE_SET(cache, set)), 
        CACHE_GSHP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_sap:
      cache_access_block_sap(CACHE_SET_DATA_SAP(CACHE_SET(cache, set)), 
        CACHE_SAP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_sdp:
      cache_access_block_sdp(CACHE_SET_DATA_SDP(CACHE_SET(cache, set)), 
        CACHE_SDP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_ship:
      cache_access_block_ship(CACHE_SET_DATA_SHIP(CACHE_SET(cache, set)), 
        CACHE_SHIP_GDATA(cache), way, stream, info);
      break;

    case cache_policy_bypass:
    case cache_policy_cpulast:
      /* Nothing to do */
      break;

    case cache_policy_opt:
    case cache_policy_invalid:
      panic("%s: line no %d - invalid polict type", __FUNCTION__, __LINE__);
  }
}

int cache_replace_block(struct cache_t *cache, int set, memory_trace *info)
{
  ub4 verified_way;
  ub4 new_way;

  switch(cache->policy)
  {
    case cache_policy_lru:
      return cache_replace_block_lru(CACHE_SET_DATA_LRU(CACHE_SET(cache, set)), info);

    case cache_policy_stridelru:
      return cache_replace_block_stridelru(CACHE_SET_DATA_STRIDELRU(CACHE_SET(cache, set)));

    case cache_policy_nru:
      return cache_replace_block_nru(CACHE_SET_DATA_NRU(CACHE_SET(cache, set)));

    case cache_policy_ucp:
      return cache_replace_block_ucp(CACHE_SET_DATA_UCP(CACHE_SET(cache, set)),
        CACHE_UCP_GDATA(cache), info);

    case cache_policy_tapucp:
      return cache_replace_block_tapucp(CACHE_SET_DATA_TAPUCP(CACHE_SET(cache, set)),
        CACHE_TAPUCP_GDATA(cache), info);

    case cache_policy_tapdrrip:
      return cache_replace_block_tapdrrip(CACHE_SET_DATA_TAPDRRIP(CACHE_SET(cache, set)),
        CACHE_TAPDRRIP_GDATA(cache), info);

    case cache_policy_helmdrrip:
      return cache_replace_block_helmdrrip(CACHE_SET_DATA_HELMDRRIP(CACHE_SET(cache, set)),
        CACHE_HELMDRRIP_GDATA(cache), info);

    case cache_policy_salru:
      return cache_replace_block_salru(
        CACHE_SET_DATA_SALRU(CACHE_SET(cache, set)), NN, info);

    case cache_policy_fifo:
      return cache_replace_block_fifo(CACHE_SET_DATA_FIFO(CACHE_SET(cache, set)));

    case cache_policy_srrip:
      new_way = cache_replace_block_srrip(CACHE_SET_DATA_SRRIP(CACHE_SET(cache, set)), 
          CACHE_SRRIP_GDATA(cache), info);
      return new_way;

    case cache_policy_srriphint:
      new_way = cache_replace_block_srriphint(CACHE_SET_DATA_SRRIPHINT(CACHE_SET(cache, set)), 
          CACHE_SRRIPHINT_GDATA(cache), info);
      return new_way;

    case cache_policy_srripbypass:
      return cache_replace_block_srripbypass(CACHE_SET_DATA_SRRIPBYPASS(CACHE_SET(cache, set)), 
          CACHE_SRRIPBYPASS_GDATA(cache));

    case cache_policy_customsrrip:
      new_way = cache_replace_block_customsrrip(CACHE_SET_DATA_CUSTOMSRRIP(CACHE_SET(cache, set)), 
          CACHE_CUSTOMSRRIP_GDATA(cache), NN);
      return new_way;

    case cache_policy_srripbs:
      return cache_replace_block_srripbs(CACHE_SET_DATA_SRRIPBS(CACHE_SET(cache, set)), 
          CACHE_SRRIPBS_GDATA(cache));

    case cache_policy_pasrrip:
      new_way = cache_replace_block_pasrrip(CACHE_SET_DATA_PASRRIP(CACHE_SET(cache, set)),
          CACHE_PASRRIP_GDATA(cache));
      return new_way;

    case cache_policy_xsp:
      new_way = cache_replace_block_xsp(CACHE_SET_DATA_XSP(CACHE_SET(cache, set)),
          CACHE_XSP_GDATA(cache));
      return new_way;

    case cache_policy_xsppin:
      new_way = cache_replace_block_xsppin(CACHE_SET_DATA_XSPPIN(CACHE_SET(cache, set)),
          CACHE_XSPPIN_GDATA(cache));
      return new_way;

    case cache_policy_streampin:
      new_way = cache_replace_block_streampin(CACHE_SET_DATA_STREAMPIN(CACHE_SET(cache, set)),
          CACHE_STREAMPIN_GDATA(cache));
      return new_way;

    case cache_policy_xspdbp:
      return cache_replace_block_xspdbp(CACHE_SET_DATA_XSPDBP(CACHE_SET(cache, set)),
        CACHE_XSPDBP_GDATA(cache), info);

    case cache_policy_srripm:
      return cache_replace_block_srripm(CACHE_SET_DATA_SRRIPM(CACHE_SET(cache, set)));

    case cache_policy_srript:
      return cache_replace_block_srript(CACHE_SET_DATA_SRRIPT(CACHE_SET(cache, set)));

    case cache_policy_srripdbp:
      return cache_replace_block_srripdbp(CACHE_SET_DATA_SRRIPDBP(CACHE_SET(cache, set)),
        CACHE_SRRIPDBP_GDATA(cache), info);

    case cache_policy_srripsage:
      return cache_replace_block_srripsage(CACHE_SET_DATA_SRRIPSAGE(CACHE_SET(cache, set)),
          CACHE_SRRIPSAGE_GDATA(cache), info);

    case cache_policy_brrip:
      return cache_replace_block_brrip(CACHE_SET_DATA_BRRIP(CACHE_SET(cache, set)), info);

    case cache_policy_drrip:
#if 0
      DRRIPGetReplacementCandidate(&(cache->drrip_policy), set, &verified_way);
#endif
      new_way = cache_replace_block_drrip(CACHE_SET_DATA_DRRIP(CACHE_SET(cache, set)),
        CACHE_DRRIP_GDATA(cache), info);
#if 0
      assert(new_way == verified_way);
#endif
      return new_way;

    case cache_policy_sapsimple:
      return cache_replace_block_sapsimple(CACHE_SET_DATA_SAPSIMPLE(CACHE_SET(cache, set)),
        CACHE_SAPSIMPLE_GDATA(cache), info);
      break;

    case cache_policy_sapdbp:
      return cache_replace_block_sapdbp(CACHE_SET_DATA_SAPDBP(CACHE_SET(cache, set)),
        CACHE_SAPDBP_GDATA(cache), info);
      break;

    case cache_policy_sappriority:
      return cache_replace_block_sappriority(CACHE_SET_DATA_SAPPRIORITY(CACHE_SET(cache, set)),
        CACHE_SAPPRIORITY_GDATA(cache), info);
      break;

    case cache_policy_sarp:
      return cache_replace_block_sarp(CACHE_SET_DATA_SARP(CACHE_SET(cache, set)),
        CACHE_SARP_GDATA(cache), info);
      break;

    case cache_policy_sappridepri:
      return cache_replace_block_sappridepri(CACHE_SET_DATA_SAPPRIDEPRI(CACHE_SET(cache, set)),
        CACHE_SAPPRIDEPRI_GDATA(cache), info);
      break;

    case cache_policy_gsdrrip:
      return cache_replace_block_gsdrrip(CACHE_SET_DATA_GSDRRIP(CACHE_SET(cache, set)),
        CACHE_GSDRRIP_GDATA(cache), info);
      
    case cache_policy_lip:
      return cache_replace_block_lip(CACHE_SET_DATA_LIP(CACHE_SET(cache, set)));

    case cache_policy_bip:
      return cache_replace_block_bip(CACHE_SET_DATA_BIP(CACHE_SET(cache, set)));

    case cache_policy_dip:
      return cache_replace_block_dip(CACHE_SET_DATA_DIP(CACHE_SET(cache, set)),
        CACHE_DIP_GDATA(cache));

    case cache_policy_gspc:
      return cache_replace_block_gspc(CACHE_SET_DATA_GSPC(CACHE_SET(cache, set)),
        CACHE_GSPC_GDATA(cache), info);

    case cache_policy_gspcm:
      return cache_replace_block_gspcm(CACHE_SET_DATA_GSPCM(CACHE_SET(cache, set)),
        CACHE_GSPCM_GDATA(cache), info);

    case cache_policy_gspct:
      return cache_replace_block_gspct(CACHE_SET_DATA_GSPCT(CACHE_SET(cache, set)),
        CACHE_GSPCT_GDATA(cache));

    case cache_policy_gshp:
      cache_replace_block_gshp(CACHE_SET_DATA_GSHP(CACHE_SET(cache, set)), 
        CACHE_GSHP_GDATA(cache), info);
      break;

    case cache_policy_sap:
      return cache_replace_block_sap(CACHE_SET_DATA_SAP(CACHE_SET(cache, set)), 
        CACHE_SAP_GDATA(cache), info);

    case cache_policy_sdp:
      return cache_replace_block_sdp(CACHE_SET_DATA_SDP(CACHE_SET(cache, set)), 
        CACHE_SDP_GDATA(cache), info);

    case cache_policy_ship:
      return cache_replace_block_ship(CACHE_SET_DATA_SHIP(CACHE_SET(cache, set)), 
        CACHE_SHIP_GDATA(cache), info);

    case cache_policy_bypass:
    case cache_policy_cpulast:
      /* Nothing to do */
      break;

    case cache_policy_opt:
    case cache_policy_invalid:
      panic("%s: line no %d - invalid polict type", __FUNCTION__, __LINE__);
  }

  return -1;
}


void cache_set_block(struct cache_t *cache, int set, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  switch(cache->policy)
  {
    case cache_policy_lru:
      return cache_set_block_lru(CACHE_SET_DATA_LRU(CACHE_SET(cache, set)), 
              way, tag, state, stream, info);

    case cache_policy_stridelru:
      return cache_set_block_stridelru(CACHE_SET_DATA_STRIDELRU(CACHE_SET(cache, set)), 
              way, tag, state, stream, info);

    case cache_policy_nru:
      return cache_set_block_nru(CACHE_SET_DATA_NRU(CACHE_SET(cache, set)), 
              way, tag, state, stream, info);

    case cache_policy_ucp:
      return cache_set_block_ucp(CACHE_SET_DATA_UCP(CACHE_SET(cache, set)), 
              way, tag, state, stream, info);

    case cache_policy_tapucp:
      return cache_set_block_tapucp(CACHE_SET_DATA_TAPUCP(CACHE_SET(cache, set)), 
              way, tag, state, stream, info);

    case cache_policy_tapdrrip:
      return cache_set_block_tapdrrip(CACHE_SET_DATA_TAPDRRIP(CACHE_SET(cache, set)), 
              way, tag, state, stream, info);

    case cache_policy_helmdrrip:
      return cache_set_block_helmdrrip(CACHE_SET_DATA_HELMDRRIP(CACHE_SET(cache, set)), 
              way, tag, state, stream, info);

    case cache_policy_fifo:
      return cache_set_block_fifo(CACHE_SET_DATA_FIFO(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_salru:
      return cache_set_block_salru(CACHE_SET_DATA_SALRU(CACHE_SET(cache, set)), 
              way, tag, state, stream, info);

    case cache_policy_srrip:
      return cache_set_block_srrip(CACHE_SET_DATA_SRRIP(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_srriphint:
      return cache_set_block_srriphint(CACHE_SET_DATA_SRRIPHINT(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_srripbypass:
      return cache_set_block_srripbypass(CACHE_SET_DATA_SRRIPBYPASS(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_customsrrip:
      return cache_set_block_customsrrip(CACHE_SET_DATA_CUSTOMSRRIP(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_srripbs:
      return cache_set_block_srripbs(CACHE_SET_DATA_SRRIPBS(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_pasrrip:
      return cache_set_block_pasrrip(CACHE_SET_DATA_PASRRIP(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_xsp:
      return cache_set_block_xsp(CACHE_SET_DATA_XSP(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_xsppin:
      return cache_set_block_xsppin(CACHE_SET_DATA_XSPPIN(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_streampin:
      return cache_set_block_streampin(CACHE_SET_DATA_STREAMPIN(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_xspdbp:
      return cache_set_block_xspdbp(CACHE_SET_DATA_XSPDBP(CACHE_SET(cache, set)),
        CACHE_XSPDBP_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_srripm:
      return cache_set_block_srripm(CACHE_SET_DATA_SRRIPM(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_srript:
      return cache_set_block_srript(CACHE_SET_DATA_SRRIPT(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_srripdbp:
      return cache_set_block_srripdbp(CACHE_SET_DATA_SRRIPDBP(CACHE_SET(cache, set)),
        CACHE_SRRIPDBP_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_srripsage:
      return cache_set_block_srripsage(CACHE_SET_DATA_SRRIPSAGE(CACHE_SET(cache, set)),
        way, tag, state, stream, info);

    case cache_policy_brrip:
      return cache_set_block_brrip(CACHE_SET_DATA_BRRIP(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_drrip:
      return cache_set_block_drrip(CACHE_SET_DATA_DRRIP(CACHE_SET(cache, set)),
              CACHE_DRRIP_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_sapsimple:
      return cache_set_block_sapsimple(CACHE_SET_DATA_SAPSIMPLE(CACHE_SET(cache, set)),
              CACHE_SAPSIMPLE_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_sapdbp:
      return cache_set_block_sapdbp(CACHE_SET_DATA_SAPDBP(CACHE_SET(cache, set)),
              CACHE_SAPDBP_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_sappriority:
      return cache_set_block_sappriority(CACHE_SET_DATA_SAPPRIORITY(CACHE_SET(cache, set)),
              CACHE_SAPPRIORITY_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_sarp:
      return cache_set_block_sarp(CACHE_SET_DATA_SARP(CACHE_SET(cache, set)),
              CACHE_SARP_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_sappridepri:
      return cache_set_block_sappridepri(CACHE_SET_DATA_SAPPRIDEPRI(CACHE_SET(cache, set)),
              CACHE_SAPPRIDEPRI_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_gsdrrip:
      return cache_set_block_gsdrrip(CACHE_SET_DATA_GSDRRIP(CACHE_SET(cache, set)),
              CACHE_GSDRRIP_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_lip:
      return cache_set_block_lip(CACHE_SET_DATA_LIP(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_bip:
      return cache_set_block_bip(CACHE_SET_DATA_BIP(CACHE_SET(cache, set)),
              way, tag, state, stream, info);

    case cache_policy_dip:
      return cache_set_block_dip(CACHE_SET_DATA_DIP(CACHE_SET(cache, set)),
              CACHE_DIP_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_gspc:
      return cache_set_block_gspc(CACHE_SET_DATA_GSPC(CACHE_SET(cache, set)),
        CACHE_GSPC_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_gspcm:
      return cache_set_block_gspcm(CACHE_SET_DATA_GSPCM(CACHE_SET(cache, set)),
        CACHE_GSPCM_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_gspct:
      return cache_set_block_gspct(CACHE_SET_DATA_GSPCT(CACHE_SET(cache, set)),
        CACHE_GSPCT_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_gshp:
      return cache_set_block_gshp(CACHE_SET_DATA_GSHP(CACHE_SET(cache, set)),
        CACHE_GSHP_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_sap:
      return cache_set_block_sap(CACHE_SET_DATA_SAP(CACHE_SET(cache, set)),
        CACHE_SAP_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_sdp:
      return cache_set_block_sdp(CACHE_SET_DATA_SDP(CACHE_SET(cache, set)),
        CACHE_SDP_GDATA(cache), way, tag, state, stream, info);

    case cache_policy_ship:
      return cache_set_block_ship(CACHE_SET_DATA_SHIP(CACHE_SET(cache, set)),
        way, tag, state, stream, info);

    case cache_policy_bypass:
    case cache_policy_cpulast:
      /* Nothing to do */
      break;

    case cache_policy_opt:
    case cache_policy_invalid:
      panic("%s: line no %d - invalid polict type", __FUNCTION__, __LINE__);
  }
}

struct cache_block_t cache_get_block(struct cache_t *cache, int set, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  struct cache_block_t ret_block;

  switch(cache->policy)
  {
    case cache_policy_lru:
      return cache_get_block_lru(CACHE_SET_DATA_LRU(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_stridelru:
      return cache_get_block_stridelru(CACHE_SET_DATA_STRIDELRU(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_nru:
      return cache_get_block_nru(CACHE_SET_DATA_NRU(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_ucp:
      return cache_get_block_ucp(CACHE_SET_DATA_UCP(CACHE_SET(cache, set)),
        CACHE_UCP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_tapucp:
      return cache_get_block_tapucp(CACHE_SET_DATA_TAPUCP(CACHE_SET(cache, set)),
        CACHE_TAPUCP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_tapdrrip:
      return cache_get_block_tapdrrip(CACHE_SET_DATA_TAPDRRIP(CACHE_SET(cache, set)),
        CACHE_TAPDRRIP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_helmdrrip:
      return cache_get_block_helmdrrip(CACHE_SET_DATA_HELMDRRIP(CACHE_SET(cache, set)),
        CACHE_HELMDRRIP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_salru:
      return cache_get_block_salru(CACHE_SET_DATA_SALRU(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_fifo:
      return cache_get_block_fifo(CACHE_SET_DATA_FIFO(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_srrip:
      return cache_get_block_srrip(CACHE_SET_DATA_SRRIP(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_srriphint:
      return cache_get_block_srriphint(CACHE_SET_DATA_SRRIPHINT(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_srripbypass:
      return cache_get_block_srripbypass(CACHE_SET_DATA_SRRIPBYPASS(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_customsrrip:
      return cache_get_block_customsrrip(CACHE_SET_DATA_CUSTOMSRRIP(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_srripbs:
      return cache_get_block_srripbs(CACHE_SET_DATA_SRRIPBS(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_pasrrip:
      return cache_get_block_pasrrip(CACHE_SET_DATA_PASRRIP(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_xsp:
      return cache_get_block_xsp(CACHE_SET_DATA_XSP(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_xsppin:
      return cache_get_block_xsppin(CACHE_SET_DATA_XSPPIN(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_streampin:
      return cache_get_block_streampin(CACHE_SET_DATA_STREAMPIN(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_xspdbp:
      return cache_get_block_xspdbp(CACHE_SET_DATA_XSPDBP(CACHE_SET(cache, set)),
        CACHE_XSPDBP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_srripm:
      return cache_get_block_srripm(CACHE_SET_DATA_SRRIPM(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_srript:
      return cache_get_block_srript(CACHE_SET_DATA_SRRIPT(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_srripdbp:
      return cache_get_block_srripdbp(CACHE_SET_DATA_SRRIPDBP(CACHE_SET(cache, set)),
        CACHE_SRRIPDBP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_srripsage:
      return cache_get_block_srripsage(CACHE_SET_DATA_SRRIPSAGE(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_brrip:
      return cache_get_block_brrip(CACHE_SET_DATA_BRRIP(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_drrip:
      return cache_get_block_drrip(CACHE_SET_DATA_DRRIP(CACHE_SET(cache, set)),
        CACHE_DRRIP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_sapsimple:
      return cache_get_block_sapsimple(CACHE_SET_DATA_SAPSIMPLE(CACHE_SET(cache, set)),
        CACHE_SAPSIMPLE_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_sapdbp:
      return cache_get_block_sapdbp(CACHE_SET_DATA_SAPDBP(CACHE_SET(cache, set)),
        CACHE_SAPDBP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_sappriority:
      return cache_get_block_sappriority(CACHE_SET_DATA_SAPPRIORITY(CACHE_SET(cache, set)),
        CACHE_SAPPRIORITY_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_sarp:
      return cache_get_block_sarp(CACHE_SET_DATA_SARP(CACHE_SET(cache, set)),
        CACHE_SARP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_sappridepri:
      return cache_get_block_sappridepri(CACHE_SET_DATA_SAPPRIDEPRI(CACHE_SET(cache, set)),
        CACHE_SAPPRIDEPRI_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_gsdrrip:
      return cache_get_block_gsdrrip(CACHE_SET_DATA_GSDRRIP(CACHE_SET(cache, set)),
        CACHE_GSDRRIP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_lip:
      return cache_get_block_lip(CACHE_SET_DATA_LIP(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_bip:
      return cache_get_block_bip(CACHE_SET_DATA_BIP(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_dip:
      return cache_get_block_dip(CACHE_SET_DATA_DIP(CACHE_SET(cache, set)),
        CACHE_DIP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_gspc:
      return cache_get_block_gspc(CACHE_SET_DATA_GSPC(CACHE_SET(cache, set)),
        CACHE_GSPC_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_gspcm:
      return cache_get_block_gspcm(CACHE_SET_DATA_GSPCM(CACHE_SET(cache, set)),
        CACHE_GSPCM_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_gspct:
      return cache_get_block_gspct(CACHE_SET_DATA_GSPCT(CACHE_SET(cache, set)),
        CACHE_GSPCT_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_gshp:
      return cache_get_block_gshp(CACHE_SET_DATA_GSHP(CACHE_SET(cache, set)),
        CACHE_GSHP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_sap:
      return cache_get_block_sap(CACHE_SET_DATA_SAP(CACHE_SET(cache, set)),
        CACHE_SAP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_sdp:
      return cache_get_block_sdp(CACHE_SET_DATA_SDP(CACHE_SET(cache, set)),
        CACHE_SDP_GDATA(cache), way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_ship:
      return cache_get_block_ship(CACHE_SET_DATA_SHIP(CACHE_SET(cache, set)),
        way, tag_ptr, state_ptr, stream_ptr);

    case cache_policy_bypass:
    case cache_policy_cpulast:
      /* Nothing to do */
      return ret_block;

    case cache_policy_opt:
    case cache_policy_invalid:
    default:
      panic("%s: line no %d - invalid polict type", __FUNCTION__, __LINE__);
      return ret_block;
  }
}

int cache_count_block(struct cache_t *cache, int strm)
{
  int total_blocks = 0;

  for (int set = 0; set < cache->num_sets; set++)
  {
    switch(cache->policy)
    {
      case cache_policy_srrip:
        total_blocks += cache_count_block_srrip(CACHE_SET_DATA_SRRIP(CACHE_SET(cache, set)), strm);
        break;

      case cache_policy_invalid:
      default:
        panic("%s: line no %d - invalid polict type", __FUNCTION__, __LINE__);
    }
  }

  return total_blocks;
}

#define GFX_CORE (0)
#define STALL_TABLE(icore)  (cpu->core[(icore)].core_stall_table[3])
#define PC_LRU_LIST(icore)  (STALL_TABLE((icore)).pc_lru_list)

extern struct cpu_t *cpu;

static void penalize_cpu_core(sdp_gdata *global_data, ub8 max_bsmps_core)
{
  assert(1);
}

/* Penalize a graphics core by removing one of the stream from speedup set. */
static void penalize_gfx_core()
{
  assert(1);
}

void cache_sdp_evaluate_policy(sdp_gdata *global_data)
{
  assert(1);
}

void cache_sdp_reset_stall_counters(sdp_gdata *global_data)
{
  assert(1);
}
