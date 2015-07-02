#include "cache-set.h"

struct cache_block_t* get_cache_blocks(struct cache_set_t *set, cache_policy_t policy)
{
  switch(policy)
  {
  case cache_policy_lru:
    return (LRU_DATA_BLOCKS(CACHE_SET_DATA_LRU(set)));
    break;

  case cache_policy_srrip:
    return (SRRIP_DATA_BLOCKS(CACHE_SET_DATA_SRRIP(set)));
    break;

  case cache_policy_srripbs:
    return (SRRIP_DATA_BLOCKS(CACHE_SET_DATA_SRRIPBS(set)));
    break;

  case cache_policy_brrip:
    return (SRRIP_DATA_BLOCKS(CACHE_SET_DATA_SRRIP(set)));
    break;

  case cache_policy_drrip:
    return (DRRIP_DATA_BLOCKS(CACHE_SET_DATA_DRRIP(set)));
    break;

  case cache_policy_lip:
    return (LIP_DATA_BLOCKS(CACHE_SET_DATA_LIP(set)));
    break;

  case cache_policy_bip:
    return (BIP_DATA_BLOCKS(CACHE_SET_DATA_BIP(set)));
    break;

  case cache_policy_dip:
    return (DIP_DATA_BLOCKS(CACHE_SET_DATA_DIP(set)));
    break;

  case cache_policy_gspc:
    return (GSPC_DATA_BLOCKS(CACHE_SET_DATA_GSPC(set)));
    break;

  default:
    panic("%s: invalid cache policy", __FUNCTION__);
    return NULL;
  }
}

struct cache_block_t* get_cache_block(struct cache_set_t *set,
  cache_policy_t policy, int way)
{
  switch(policy)
  {
  case cache_policy_lru:
    return &(LRU_DATA_BLOCKS(CACHE_SET_DATA_LRU(set))[way]);
    break;

  case cache_policy_srrip:
    return &(SRRIP_DATA_BLOCKS(CACHE_SET_DATA_SRRIP(set))[way]);
    break;

  case cache_policy_brrip:
    return &(SRRIP_DATA_BLOCKS(CACHE_SET_DATA_SRRIP(set))[way]);
    break;

  case cache_policy_drrip:
    return &(DRRIP_DATA_BLOCKS(CACHE_SET_DATA_DRRIP(set))[way]);
    break;

  case cache_policy_lip:
    return &(LIP_DATA_BLOCKS(CACHE_SET_DATA_LIP(set))[way]);
    break;

  case cache_policy_bip:
    return &(BIP_DATA_BLOCKS(CACHE_SET_DATA_BIP(set))[way]);
    break;

  case cache_policy_dip:
    return &(DIP_DATA_BLOCKS(CACHE_SET_DATA_DIP(set))[way]);
    break;

  case cache_policy_gspc:
    return &(GSPC_DATA_BLOCKS(CACHE_SET_DATA_GSPC(set))[way]);
    break;

  default:
    panic("%s: invalid cache policy", __FUNCTION__);
    return NULL;
  }
}

struct cache_block_t* get_valid_block_list(struct cache_set_t *set, cache_policy_t policy)
{
  switch(policy)
  {
    case cache_policy_lru:
      return (LRU_DATA_VALID_HEAD(CACHE_SET_DATA_LRU(set)));
      break;

    case cache_policy_srrip:
      return ((SRRIP_DATA_VALID_HEAD(CACHE_SET_DATA_SRRIP(set)))->head);
      break;

    case cache_policy_brrip:
      return ((SRRIP_DATA_VALID_HEAD(CACHE_SET_DATA_SRRIP(set)))->head);
      break;

    case cache_policy_drrip:
      return ((DRRIP_DATA_VALID_HEAD(CACHE_SET_DATA_DRRIP(set)))->head);
      break;

    case cache_policy_lip:
      return (LIP_DATA_VALID_HEAD(CACHE_SET_DATA_LIP(set)));
      break;

    case cache_policy_bip:
      return (BIP_DATA_VALID_HEAD(CACHE_SET_DATA_BIP(set)));
      break;

    case cache_policy_dip:
      return (DIP_DATA_VALID_HEAD(CACHE_SET_DATA_DIP(set)));
      break;

    case cache_policy_gspc:
      return ((GSPC_DATA_VALID_HEAD(CACHE_SET_DATA_GSPC(set)))->head);
      break;

    default:
      panic("%s: invalid cache policy", __FUNCTION__);
      return NULL;
  }
}
