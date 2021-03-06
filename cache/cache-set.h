#ifndef CACHE_SET_H
#define CACHE_SET_H

#include <stdio.h>
#include "policy.h"
#include "lru.h"
#include "nru.h"
#include "fifo.h"
#include "lip.h"
#include "bip.h"
#include "dip.h"
#include "srrip.h"
#include "srripbs.h"
#include "srripm.h"
#include "srript.h"
#include "srripdbp.h"
#include "srripsage.h"
#include "pasrrip.h"
#include "xsp.h"
#include "xsppin.h"
#include "streampin.h"
#include "xspdbp.h"
#include "brrip.h"
#include "drrip.h"
#include "gsdrrip.h"
#include "gspc.h"
#include "gspcm.h"
#include "gspct.h"
#include "gshp.h"
#include "ucp.h"
#include "tapucp.h"
#include "tapdrrip.h"
#include "helmdrrip.h"
#include "sap.h"
#include "sdp.h"
#include "stridelru.h"
#include "sapsimple.h"
#include "sapdbp.h"
#include "customsrrip.h"
#include "sappriority.h"
#include "sappridepri.h"
#include "srripbypass.h"
#include "srriphint.h"
#include "ship.h"
#include "sarp.h"
#include "cache-block.h"

#define CACHE_SET_POLICY(cache_set)           ((cache_set)->policy)
#define CACHE_SET_DATA_LRU(cache_set)         (&(((cache_set)->policy_data).lru))
#define CACHE_SET_DATA_NRU(cache_set)         (&(((cache_set)->policy_data).nru))
#define CACHE_SET_DATA_UCP(cache_set)         (&(((cache_set)->policy_data).ucp))
#define CACHE_SET_DATA_FIFO(cache_set)        (&(((cache_set)->policy_data).fifo))
#define CACHE_SET_DATA_SALRU(cache_set)       (&(((cache_set)->policy_data).salru))
#define CACHE_SET_DATA_SRRIP(cache_set)       (&(((cache_set)->policy_data).srrip))
#define CACHE_SET_DATA_SRRIPBS(cache_set)     (&(((cache_set)->policy_data).srripbs))
#define CACHE_SET_DATA_SRRIPM(cache_set)      (&(((cache_set)->policy_data).srripm))
#define CACHE_SET_DATA_SRRIPT(cache_set)      (&(((cache_set)->policy_data).srript))
#define CACHE_SET_DATA_SRRIPDBP(cache_set)    (&(((cache_set)->policy_data).srripdbp))
#define CACHE_SET_DATA_SRRIPSAGE(cache_set)   (&(((cache_set)->policy_data).srripsage))
#define CACHE_SET_DATA_PASRRIP(cache_set)     (&(((cache_set)->policy_data).pasrrip))
#define CACHE_SET_DATA_XSP(cache_set)         (&(((cache_set)->policy_data).xsp))
#define CACHE_SET_DATA_XSPPIN(cache_set)      (&(((cache_set)->policy_data).xsppin))
#define CACHE_SET_DATA_STREAMPIN(cache_set)   (&(((cache_set)->policy_data).streampin))
#define CACHE_SET_DATA_XSPDBP(cache_set)      (&(((cache_set)->policy_data).xspdbp))
#define CACHE_SET_DATA_BRRIP(cache_set)       (&(((cache_set)->policy_data).brrip))
#define CACHE_SET_DATA_DRRIP(cache_set)       (&(((cache_set)->policy_data).drrip))
#define CACHE_SET_DATA_GSDRRIP(cache_set)     (&(((cache_set)->policy_data).gsdrrip))
#define CACHE_SET_DATA_LIP(cache_set)         (&(((cache_set)->policy_data).lip))
#define CACHE_SET_DATA_BIP(cache_set)         (&(((cache_set)->policy_data).bip))
#define CACHE_SET_DATA_DIP(cache_set)         (&(((cache_set)->policy_data).dip))
#define CACHE_SET_DATA_GSPC(cache_set)        (&(((cache_set)->policy_data).gspc))
#define CACHE_SET_DATA_GSPCM(cache_set)       (&(((cache_set)->policy_data).gspcm))
#define CACHE_SET_DATA_GSPCT(cache_set)       (&(((cache_set)->policy_data).gspct))
#define CACHE_SET_DATA_GSHP(cache_set)        (&(((cache_set)->policy_data).gshp))
#define CACHE_SET_DATA_TAPUCP(cache_set)      (&(((cache_set)->policy_data).tapucp))
#define CACHE_SET_DATA_TAPDRRIP(cache_set)    (&(((cache_set)->policy_data).tapdrrip))
#define CACHE_SET_DATA_HELMDRRIP(cache_set)   (&(((cache_set)->policy_data).helmdrrip))
#define CACHE_SET_DATA_SAP(cache_set)         (&(((cache_set)->policy_data).sap))
#define CACHE_SET_DATA_SDP(cache_set)         (&(((cache_set)->policy_data).sdp))
#define CACHE_SET_DATA_STRIDELRU(cache_set)   (&(((cache_set)->policy_data).stride_lru))
#define CACHE_SET_DATA_SAPSIMPLE(cache_set)   (&(((cache_set)->policy_data).sapsimple))
#define CACHE_SET_DATA_SAPDBP(cache_set)      (&(((cache_set)->policy_data).sapdbp))
#define CACHE_SET_DATA_SAPPRIORITY(cache_set) (&(((cache_set)->policy_data).sappriority))
#define CACHE_SET_DATA_SAPPRIDEPRI(cache_set) (&(((cache_set)->policy_data).sappridepri))
#define CACHE_SET_DATA_CUSTOMSRRIP(cache_set) (&(((cache_set)->policy_data).customsrrip))
#define CACHE_SET_DATA_SHIP(cache_set)        (&(((cache_set)->policy_data).ship))
#define CACHE_SET_DATA_SARP(cache_set)        (&(((cache_set)->policy_data).sarp))
#define CACHE_SET_DATA_SRRIPBYPASS(cache_set) (&(((cache_set)->policy_data).srripbypass))
#define CACHE_SET_DATA_SRRIPHINT(cache_set)   (&(((cache_set)->policy_data).srriphint))

struct cache_set_t
{
  /* Cache management policy specific data */
  union 
  {
    lru_data          lru;
    nru_data          nru;
    ucp_data          ucp;
    fifo_data         fifo;
    salru_data        salru;
    srrip_data        srrip;
    srripbs_data      srripbs;
    srripm_data       srripm;
    srript_data       srript;
    srripdbp_data     srripdbp;
    srripsage_data    srripsage;
    pasrrip_data      pasrrip;
    xsp_data          xsp;
    xsppin_data       xsppin;
    streampin_data    streampin;
    xspdbp_data       xspdbp;
    brrip_data        brrip;
    drrip_data        drrip;
    gsdrrip_data      gsdrrip;
    lip_data          lip;
    bip_data          bip;
    dip_data          dip;
    gspc_data         gspc;
    gspcm_data        gspcm;
    gspct_data        gspct;
    gshp_data         gshp;
    tapucp_data       tapucp;
    tapdrrip_data     tapdrrip;
    helmdrrip_data    helmdrrip;
    sap_data          sap;
    sdp_data          sdp;
    stridelru_data    stride_lru;
    sapsimple_data    sapsimple;
    sapdbp_data       sapdbp;
    sappriority_data  sappriority;
    sappridepri_data  sappridepri;
    customsrrip_data  customsrrip;
    ship_data         ship;
    sarp_data         sarp;
    srripbypass_data  srripbypass;
    srriphint_data    srriphint;
  }policy_data;
};

/* Set type */
typedef enum cache_set_type
{
  cache_set_sampled,                /* Set is used for sampling a policy */
  cache_set_following               /* Set follows the winning policy */
}set_type;

struct cache_block_t* get_cache_blocks(struct cache_set_t *set, cache_policy_t policy);

struct cache_block_t* get_cache_block(struct cache_set_t *set, cache_policy_t policy, int way);

struct cache_block_t* get_valid_block_list(struct cache_set_t *set, cache_policy_t policy);

#endif
