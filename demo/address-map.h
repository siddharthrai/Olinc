#ifndef ADDRESS_MAP_H
#define ADDRESS_MAP_H

#include <map>
#include <iostream>
#include "../common/intermod-common.h"
#include "demo/region-map.h"
#include <iomanip>

using namespace std;

#define BLOCK_SIZE  (64)
#define PAGE_SIZE   (4096)

class Region
{
  ub1 stream;
  ub8 start;
  ub8 end;
  map <ub8, ub8> block_map;
  RegionMap *region_map;

  public:

  Region(ub1 stream_in, ub8 start_in, ub8 end_in)
  {
    stream      = stream_in;
    start       = start_in;
    end         = end_in;
    region_map  = new RegionMap(stream_in, stream_in, BLOCK_SIZE, PAGE_SIZE); 
    assert(region_map);
  }
  
  ~Region()
  {
    delete region_map;
  }

  void addBlock(ub8 address)
  {
    if (block_map.find(address) == block_map.end())
    {
      block_map.insert(pair<ub8, ub8>(address, 1));
    }

    region_map->addBlock(address, 1, 0);
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
  ub8            current_region; /* For sequentially numbered regions next key */
  map<ub1, ub8>  address_map;    /* Map of region, indexed by stream */
  ub8            ct_use;         /* # blocks used between color and texture */
  ub8            bt_use;         /* # blocks used between blitter and texture */
  ub8            cb_use;         /* # blocks used between blitter and color */

  public:

  AddressMap()
  {
    current_region  = 0;
    ct_use          = 0;
    bt_use          = 0;
    cb_use          = 0;
  }
  
  ~AddressMap()
  {
    map<ub1, ub8>::iterator map_itr;
    Region *current_region;

    for (map_itr = address_map.begin(); map_itr != address_map.end(); map_itr++)
    {
      current_region = (Region *)(map_itr->second);
      delete current_region;
    }
  }

  void addAddress(ub1 stream, ub8 address)
  {
    map<ub1, ub8>::iterator map_itr;
    
    /* Ensure address is block aligned */
    assert((address & (BLOCK_SIZE - 1)) == 0);

    map_itr = address_map.find(stream);
    if (map_itr == address_map.end())
    {
      Region *memory_region;
      
      /* TODO: Create a vector of regions */
      memory_region = new Region(stream, address, address + BLOCK_SIZE);

      /* Create a new region */
      address_map.insert(pair<ub1, ub8>(stream, (ub8)memory_region));

      /* Add block to block map */
      memory_region->addBlock(address);
    }
    else
    {
      /* Obtain the region */
      Region *memory_region = (Region *)(map_itr->second);

      assert(memory_region);
      assert(memory_region->getStream() == stream);
      
      /* Add block to block map */
      memory_region->addBlock(address);

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
      
      Region *color_memory_region;
      Region *blitter_memory_region;

      /* Check CT reuse */
      if (stream == TS)
      {
        map_itr = (address_map.find(CS));
        
        if (map_itr != address_map.end())
        {
          color_memory_region = (Region *)map_itr->second;
          assert(color_memory_region);

          if (color_memory_region->isBlockPresent(address))
          {
            ct_use += 1;
          }
        }

        map_itr = (address_map.find(BS));
        
        if (map_itr != address_map.end())
        {
          blitter_memory_region = (Region *)map_itr->second;
          assert(blitter_memory_region);

          if (blitter_memory_region->isBlockPresent(address))
          {
            bt_use += 1;
          }
        }
      }
      
      if (stream == BS)
      {
        map_itr = (address_map.find(CS));
        
        if (map_itr != address_map.end())
        {
          color_memory_region = (Region *)map_itr->second;
          assert(color_memory_region);

          if (color_memory_region->isBlockPresent(address))
          {
            cb_use += 1;
          }
        }
      }
    }
  }

  void printMap()
  {
    map<ub1, ub8>::iterator map_itr;
    Region  *current_region;

    cout << std::setw(10) << std::left;
    cout << "Stream";
    cout << std::setw(10) << std::left;
    cout << "Blocks";
    cout << std::setw(12) << std::left;
    cout << "Start";
    cout << "End";
    cout << endl;

    for (map_itr = address_map.begin(); map_itr != address_map.end(); map_itr++)
    {
      current_region = (Region *)(map_itr->second);
      assert(current_region);
      cout << std::setw(10) << std::left;
      cout << (int)(map_itr->first);
      cout << std::setw(10) << std::left;
      cout << current_region->getBlockCount();
      cout << std::setw(12) << std::left;
      cout << current_region->getStart() << " " << current_region->getEnd() << endl;
    }

    cout << "CT Reuse " << ct_use << endl;
    cout << "BT Reuse " << bt_use << endl;
    cout << "CB Reuse " << cb_use << endl;
  }
};

#undef BLOCK_SIZE
#undef PAGE_SIZE
#endif
