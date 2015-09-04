#include "inter-stream-reuse.h"
#include <iostream>

using namespace std;

/* Entry callback */
void InterStreamReuse::StartCbk()
{
}

#define SIGN_SIZE           (14)
#define REGION_SIZE         (14)
#define SIGMAX_VAL          (((1 << SIGN_SIZE) - 1))
#define SIGNMASK            (SIGMAX_VAL << REGION_SIZE)
#define SIGNATURE(i)        ((((i)->vtl_addr) & SIGNMASK) >> REGION_SIZE)
#define C_BIT(t, id)        ((t)[id].c_bit)
#define B_BIT(t, id)        ((t)[id].b_bit)
#define T_BIT(t, id)        ((t)[id].t_bit)
#define CT_BIT(t, id)       ((t)[id].ct_bit)
#define BT_BIT(t, id)       ((t)[id].bt_bit)
#define IS_CT_USE(t, id)    (C_BIT(t, id) == TRUE && T_BIT(t, id) == TRUE)
#define IS_BT_USE(t, id)    (B_BIT(t, id) == TRUE && T_BIT(t, id) == TRUE)
#define USE_COUNTED(t, id)  ((t)[id].counted)
#define C_USE(t)            ((t)->c_count)
#define B_USE(t)            ((t)->b_count)
#define T_USE(t)            ((t)->t_count)
#define CT_USE(t)           ((t)->ct_count)
#define BT_USE(t)           ((t)->bt_count)
#define CT_RTABLE(t)        ((t)->ctreuse_table)
#define BT_RTABLE(t)        ((t)->btreuse_table)

void InterStreamReuse::update_ct_reuse(memory_trace *info)
{
  ub8 index;

  index = SIGNATURE(info);
  
  if (info->stream == CS && C_BIT(CT_RTABLE(this), index) == FALSE)
  {
    C_BIT(CT_RTABLE(this), index) = TRUE;

    C_USE(this) += 1;
  }

  if (info->stream == TS && T_BIT(CT_RTABLE(this), index) == FALSE)
  {
    T_BIT(CT_RTABLE(this), index) = TRUE;

    T_USE(this) += 1;
  }

  if (IS_CT_USE(CT_RTABLE(this), index))
  {
    if (USE_COUNTED(CT_RTABLE(this), index) == FALSE)
    {
      assert(C_USE(this) > 0 && T_USE(this) > 0); 

      CT_BIT(CT_RTABLE(this), index) = TRUE;

      C_USE(this) -= 1;
      T_USE(this) -= 1;

      USE_COUNTED(CT_RTABLE(this), index) = TRUE;
    }

    CT_USE(this) += 1;
  }
}

ub1 InterStreamReuse::is_ct_block(memory_trace *info)
{
  ub8 index;
  ub1 ret;

  ret   = FALSE;
  index = SIGNATURE(info);

  if (CT_BIT(CT_RTABLE(this), index) == TRUE)
  {
    ret = TRUE;
  }

  return ret;
}

void InterStreamReuse::update_bt_reuse(memory_trace *info)
{
  ub8 index;

  index = SIGNATURE(info);
  
  if (info->stream == BS && B_BIT(BT_RTABLE(this), index) == FALSE)
  {
    B_BIT(BT_RTABLE(this), index) = TRUE;

    B_USE(this) += 1;
  }

  if (info->stream == TS && T_BIT(BT_RTABLE(this), index) == FALSE)
  {
    T_BIT(BT_RTABLE(this), index) = TRUE;

    T_USE(this) += 1;
  }

  if (IS_BT_USE(BT_RTABLE(this), index))
  {
    if (USE_COUNTED(BT_RTABLE(this), index) == FALSE)
    {
      assert(B_USE(this) > 0 && T_USE(this) > 0); 

      BT_BIT(BT_RTABLE(this), index) = TRUE;

      B_USE(this) -= 1;
      T_USE(this) -= 1;

      USE_COUNTED(BT_RTABLE(this), index) = TRUE;
    }

    BT_USE(this) += 1;
  }
}

ub1 InterStreamReuse::is_bt_block(memory_trace *info)
{
  ub8 index;
  ub1 ret;

  ret   = FALSE;
  index = SIGNATURE(info);

  if (BT_BIT(BT_RTABLE(this), index) == TRUE)
  {
    ret = TRUE;
  }

  return ret;
}

#undef SIGMAX_VAL
#undef SIGNMASK
#undef SIGNATURE
#undef C_BIT
#undef B_BIT
#undef T_BIT
#undef CT_BIT
#undef BT_BIT
#undef IS_CT_USE
#undef IS_BT_USE
#undef USE_COUNTED
#undef C_USE
#undef B_USE
#undef T_USE
#undef CT_USE
#undef BT_USE
#undef CT_RTABLE

/* Cache access begin callback */
void InterStreamReuse::CacheAccessBeginCbk(memory_trace info, ub8 set_idx, ub8 access, ub8 rplc)
{
  if (use_va == FALSE)
  {
    if (info.address)
    {
      address_map->addAddress(info.stream, info.address, set_idx, access, rplc);
      region_table->addBlock(info.stream, info.address);
    }
  }
  else
  {
    if (info.vtl_addr)
    {
      address_map->addAddress(info.stream, info.vtl_addr, set_idx, access, rplc);
      region_table->addBlock(info.stream, info.vtl_addr);
    }
  }

#if 0
  if (info.stream == TS || (info.stream == CS && info.spill == TRUE))
#endif
  if (info.stream == TS || info.stream == CS)
  {
    update_ct_reuse(&info);
  }

#if 0
  if (info.stream == TS || (info.stream == BS && info.spill == TRUE))
#endif
  if (info.stream == TS || info.stream == BS)
  {
    update_bt_reuse(&info);
  }
}

/* Cache access end callback */
void InterStreamReuse::CacheAccessEndCbk(memory_trace info, ub8 set_idx, ub8 rplc)
{
  if (use_va == FALSE)
  {
    if (info.address)
    {
      address_map->isBlockPresent(info.stream, info.address);
    }
  }
  else
  {
    if (info.vtl_addr)
    {
      address_map->isBlockPresent(info.stream, info.vtl_addr);
    }
  }
}

/* Cache access hit callback */
void InterStreamReuse::CacheAccessHitCbk(memory_trace info, 
    cache_access_status status, ub8 set_idx, ub8 access, ub8 rplc)
{
  if (info.stream != status.stream)
  {
    assert(status.tag == info.address);
    if (use_va == FALSE)
    {
      if (info.address)
      {
        address_map->addAddress(info.stream, info.address, set_idx, access, rplc);
        address_map->removeAddress(status.stream, info.stream, info.address, set_idx, rplc);

        region_table->addBlock(info.stream, info.address);
      }
    }
    else
    {
      if (info.vtl_addr)
      {
        address_map->addAddress(info.stream, info.vtl_addr, set_idx, access, rplc);
        address_map->removeAddress(status.stream, info.stream, info.vtl_addr, set_idx, rplc);

        region_table->addBlock(info.stream, info.vtl_addr);
      }
    }
  }
  else
  {
    if (use_va == FALSE)
    {
      if (info.address)
      {
        address_map->isBlockPresent(info.stream, info.address);
      }
    }
    else
    {
      if (info.vtl_addr)
      {
        address_map->isBlockPresent(info.stream, info.vtl_addr);
      }
    }
  }
}

/* Cache access miss callback */
void InterStreamReuse::CacheAccessMissCbk(memory_trace info, ub8 set_idx, ub8 rplc)
{
}

/* Cache access replace callback */
void InterStreamReuse::CacheAccessReplaceCbk(memory_trace info, 
    cache_access_status status, ub8 set_idx, ub8 access, ub8 rplc)
{
  if (use_va == FALSE)
  {
    if (status.tag)
    {
      address_map->removeAddress(status.stream, info.stream, status.tag, set_idx, rplc);
      region_table->removeBlock(status.stream, status.tag);
    }
  }
  else
  {
    if (status.vtl_addr)
    {
      address_map->removeAddress(status.stream, info.stream, status.vtl_addr, set_idx, rplc);
      region_table->removeBlock(status.stream, status.vtl_addr);
    }
  }
}

/* Final exit callback */
void InterStreamReuse::ExitCbk()
{
  address_map->printMap();
}
