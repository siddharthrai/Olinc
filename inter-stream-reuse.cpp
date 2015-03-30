#include "inter-stream-reuse.h"
#include <iostream>

using namespace std;

/* Entry callback */
void InterStreamReuse::StartCbk()
{
}

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
