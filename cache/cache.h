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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MEM_SYSTEM_CACHE_H
#define	MEM_SYSTEM_CACHE_H

#include "common/intermod-common.h"
#include "cache-set.h"
#include "cache-param.h"
#include "srrip-new.h"
#include "drrip-new.h"

#ifdef __cplusplus
# define EXPORT_C extern "C"
#else
# define EXPORT_C
#endif

/* Fast access macros */
#define MAP_STATE(state) str_map_value(&cache_block_state_map, state)

extern struct str_map_t cache_policy_map;
extern struct str_map_t cache_block_state_map;

#define CACHE_DIP_GDATA(cache)          (&(cache->policy_data.dip))
#define CACHE_SRRIP_GDATA(cache)        (&(cache->policy_data.srrip))
#define CACHE_SRRIPBS_GDATA(cache)      (&(cache->policy_data.srripbs))
#define CACHE_BRRIP_GDATA(cache)        (&(cache->policy_data.brrip))
#define CACHE_DRRIP_GDATA(cache)        (&(cache->policy_data.drrip))
#define CACHE_GSDRRIP_GDATA(cache)      (&(cache->policy_data.gsdrrip))
#define CACHE_GSPC_GDATA(cache)         (&(cache->policy_data.gspc))
#define CACHE_GSPCM_GDATA(cache)        (&(cache->policy_data.gspcm))
#define CACHE_GSPCT_GDATA(cache)        (&(cache->policy_data.gspct))
#define CACHE_GSHP_GDATA(cache)         (&(cache->policy_data.gshp))
#define CACHE_UCP_GDATA(cache)          (&(cache->policy_data.ucp))
#define CACHE_TAPUCP_GDATA(cache)       (&(cache->policy_data.tapucp))
#define CACHE_TAPDRRIP_GDATA(cache)     (&(cache->policy_data.tapdrrip))
#define CACHE_HELMDRRIP_GDATA(cache)    (&(cache->policy_data.helmdrrip))
#define CACHE_SAP_GDATA(cache)          (&(cache->policy_data.sap))
#define CACHE_SDP_GDATA(cache)          (&(cache->policy_data.sdp))
#define CACHE_PASRRIP_GDATA(cache)      (&(cache->policy_data.pasrrip))
#define CACHE_SRRIPDBP_GDATA(cache)     (&(cache->policy_data.srripdbp))
#define CACHE_SRRIPSAGE_GDATA(cache)    (&(cache->policy_data.srripsage))
#define CACHE_XSP_GDATA(cache)          (&(cache->policy_data.xsp))
#define CACHE_XSPPIN_GDATA(cache)       (&(cache->policy_data.xsppin))
#define CACHE_XSPDBP_GDATA(cache)       (&(cache->policy_data.xspdbp))
#define CACHE_SAPSIMPLE_GDATA(cache)    (&(cache->policy_data.sapsimple))
#define CACHE_SAPDBP_GDATA(cache)       (&(cache->policy_data.sapdbp))
#define CACHE_SAPPRIORITY_GDATA(cache)  (&(cache->policy_data.sappriority))
#define CACHE_SAPPRIDEPRI_GDATA(cache)  (&(cache->policy_data.sappridepri))
#define CACHE_CUSTOMSRRIP_GDATA(cache)  (&(cache->policy_data.customsrrip))

struct cache_t
{
  int assoc;
  int num_sets;

  /* Policy to be followed by the cache */
  enum cache_policy_t policy;

  /* Policy specific global data */
  struct 
  {
    dip_gdata         dip;
    srrip_gdata       srrip;
    srripbs_gdata     srripbs;
    brrip_gdata       brrip;
    drrip_gdata       drrip;
    gsdrrip_gdata     gsdrrip;
    gspc_gdata        gspc;
    srripdbp_gdata    srripdbp;
    srripsage_gdata   srripsage;
    gspcm_gdata       gspcm;
    gspct_gdata       gspct;
    gshp_gdata        gshp;
    ucp_gdata         ucp;
    tapucp_gdata      tapucp;
    tapdrrip_gdata    tapdrrip;
    helmdrrip_gdata   helmdrrip;
    sap_gdata         sap;
    sdp_gdata         sdp;
    pasrrip_gdata     pasrrip;
    xsp_gdata         xsp;
    xsppin_gdata      xsppin;
    xspdbp_gdata      xspdbp;
    sapsimple_gdata   sapsimple;
    sapdbp_gdata      sapdbp;
    customsrrip_gdata customsrrip;
    sappriority_gdata sappriority;
    sappridepri_gdata sappridepri;
  }policy_data;

  struct cache_set_t *sets; /* Cache sets */

  struct SRRIPClass srrip_policy;
  struct DRRIPClass drrip_policy;
};

EXPORT_C struct cache_t *cache_create(int num_sets, int assoc, 
  enum cache_policy_t policy);

EXPORT_C struct cache_t*  cache_init(struct cache_params *params);

EXPORT_C void   cache_free(struct cache_t *cache);

EXPORT_C void cache_access_block(struct cache_t *cache, int set, int way, 
  int stream, memory_trace *info);

EXPORT_C void cache_fill_block(struct cache_t *cache, int set, int way, 
  long long tag, enum cache_block_state_t state, int stream,
  memory_trace *info);

EXPORT_C struct cache_block_t* cache_find_block(struct cache_t *cache, int set,
  long long tag, memory_trace *info);

EXPORT_C void cache_set_block(struct cache_t *cache, int set, int way, 
  long long tag, enum cache_block_state_t state, ub1 stream, 
  memory_trace *info);

EXPORT_C struct cache_block_t cache_get_block(struct cache_t *cache, int set,
  int way, long long *tag_ptr, enum cache_block_state_t *state_ptr, 
  int *stream);

EXPORT_C int cache_replace_block(struct cache_t *cache, int set, memory_trace *info);

EXPORT_C int cache_stack_position(struct cache_t *cache, int set, int way);

EXPORT_C int cache_count_block(struct cache_t *cache, int strm);

EXPORT_C void cache_sdp_evaluate_policy(sdp_gdata *global_data);

#undef EXPORT_C
#endif	/* MEM_SYSTEM_CACHE_H */
