#ifndef REGION_MAP_H
#define REGION_MAP_H

#include <map>
#include <vector>
#include <iostream>
#include "../common/intermod-common.h"
#include <iomanip>
#include <fstream>
#include <string.h>
#include "zfstream.h"
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

#define UNDEF_ADDRESS (0xffffffffffff)
#define REGION_START  (0)

using namespace std;

extern sb1 *stream_names[TST + 1];

/* Description of one region */
class OneRegion
{
  ub8           start;            /* Start address of a region */
  ub8           end;              /* End address of a region */
  map<ub4, ub4> reuses;           /* Various reuses seen in this region */
  map<ub4, ub4> replacement;      /* Various reuses seen in this region */
  ub8           all_reuse;        /* Total reuse in the region */
  ub8           all_replacement;  /* Total replacement in the region */

  public:

  OneRegion()
  {
    all_reuse       = 0;              /* Total reuse in the region */
    all_replacement = 0;              /* Total replacement in the region */
    start           = UNDEF_ADDRESS;  /* Total replacement in the region */
    end             = UNDEF_ADDRESS;  /* Total replacement in the region */
  }

  ~OneRegion()
  {
  }

  /*
   *
   * NAME - Add Reuse
   *
   * DESCRIPTION
   *  
   *  Adds a reuse for the region 
   *
   * PARAMETERS
   *  
   *  reuse_count (IN) - Reuse count of a block in the region 
   *
   * RETURNS
   *
   *  Nothing 
   */

  void addReuse(ub4 reuse_count)
  {
    map<ub4, ub4>::iterator reuse_itr;
    
    /* Add reuse to number of distinct reuse seen by the region */
    reuse_itr = reuses.find(reuse_count);
    if (reuse_itr != reuses.end())
    {
      reuse_itr->second += 1;
    }
    else
    {
      reuses.insert(pair<ub4, ub4>(reuse_count, 1));
    }

    /* Increment total reuse count */
    all_reuse += reuse_count;
  }

  /*
   *
   * NAME - Add replacement for a block 
   *
   * DESCRIPTION
   *  
   *  Adds replacement count for a block in a region 
   *
   * PARAMETERS
   *  
   *  replacement_count (IN) - Number of repacement for a block 
   *
   * RETURNS
   *  
   *  Nothing
   */

  void addReplacement(ub4 replacement_count)
  {
    map<ub4, ub4>::iterator reuse_itr;

    /* Add reuse to number of distinct reuse seen by the region */
    reuse_itr = replacement.find(replacement_count);
    if (reuse_itr != replacement.end())
    {
      reuse_itr->second += 1;
    }
    else
    {
      replacement.insert(pair<ub4, ub4>(replacement_count, 1));
    }

    /* Increment total replacement count */
    all_replacement += replacement_count;
  }

  /*
   *
   * NAME - Set start of a region
   *
   * DESCRIPTION
   *  
   *  Sets starting address of a region
   *
   * PARAMETERS
   *  
   *  address (IN) - start address
   *
   * RETURNS
   *  
   *  Nothing 
   */

  void setStart(ub8 address)
  {
    start = address;
  }

  /*
   *
   * NAME - Set end address of a region
   *
   * DESCRIPTION
   *  
   *  Set last block address in a region 
   *
   * PARAMETERS
   *  
   *  address (IN) - End address
   *
   * RETURNS
   *  
   *  Nothing
   */

  void setEnd(ub8 address)
  {
    end = address;
    assert(end != start);
  }

  /*
   *
   * NAME - Get start of the region
   *
   * DESCRIPTION
   *  
   *  Get start of the region 
   *
   * PARAMETERS
   *
   *  Nothing 
   *
   * RETURNS
   *  
   *  Start address
   */

  ub8 getStart()
  {
    return start;
  }

  /*
   *
   * NAME - Get end of the region
   *
   * DESCRIPTION
   *  
   *  Get end of the region 
   *
   * PARAMETERS
   *
   *  Nothing 
   *
   * RETURNS
   *  
   *  End address
   */

  ub8 getEnd()
  {
    return end;
  }

  /*
   *
   * NAME - Get reuse count
   *
   * DESCRIPTION
   *  
   *  Return distinct reuse seen by blocks in the region
   *
   * PARAMETERS
   *
   *  Nothing 
   *
   * RETURNS
   *  
   * Reuse count
   */

  ub4 getReuse()
  {
    return all_reuse;
  }

  /*
   *
   * NAME - Get replacement
   *
   * DESCRIPTION
   *  
   *  Get distinct replacement seen by blocks in the region
   *
   * PARAMETERS
   *
   *  Nothing 
   *
   * RETURNS
   *  
   *  Distinct replacement
   */

  ub4 getReplacement()
  {
    return all_replacement;
  }
};

/* Container for all regions */
class RegionMap
{
  ub8         region_id;          /* IDs given to regions */
  ub1         block_size;         /* Size of a cache block */
  ub4         page_size;          /* Size of a page */
  ub8         last_block;         /* Address of the last block added */
  ub8         last_page;          /* Last accessed page number */
  OneRegion  *last_region;        /* Last accessed region */ 
  gzofstream  out_stream;         /* Stream to dump region data */

  map <ub8, ub8> all_region_map;  /* Map of different regions */

  public:

  RegionMap(ub1 src_stream_in, ub1 tgt_stream_in, 
      ub1 block_size_in, ub4 page_size_in)
  {
    /* Ensure page size and block size are power of 2 */
    assert((block_size_in & (block_size_in - 1)) == 0);
    assert((page_size_in & (page_size_in - 1)) == 0);

    last_block  = UNDEF_ADDRESS;  /* Block address of previous access */
    last_page   = UNDEF_ADDRESS;  /* Page number of previous access */
    block_size  = block_size_in;  /* Cache block size */
    page_size   = page_size_in;   /* Page size */
    region_id   = REGION_START;   /* Region start id */
    last_region = NULL;           /* Last region */

    sb1 tracefile_name[100];

    /* Open output stream */
    assert(strlen("Region-----stats.trace.csv.gz") + 1 < 100);
    sprintf(tracefile_name, "Region-%s-%s-stats.trace.csv.gz", 
        stream_names[src_stream_in], stream_names[tgt_stream_in]);
    out_stream.open(tracefile_name, ios::out | ios::binary);

    if (!out_stream.is_open())
    {
      printf("Error opening output stream\n");
      exit(EXIT_FAILURE);
    }
  }
  
  ~RegionMap()
  {
    map<ub8, ub8>::iterator region_itr;

    printRegions();

    for (region_itr = all_region_map.begin(); region_itr != all_region_map.end(); region_itr++)
    {
      OneRegion *current_region = (OneRegion *)region_itr->second;
      delete current_region;
    }

    out_stream.close();
  }

  /*
   *
   * NAME
   *
   * DESCRIPTION
   *
   * PARAMETERS
   *
   * RETURNS
   *
   * EXCEPTIONS
   *
   * NOTES
   */

  void addBlock(ub8 address, ub1 reuse_count, ub1 replace_count)
  {
    ub8 page_number;

    assert(address);

    /* Ensure address is block aligned */
    assert((address & ((ub8)(block_size - 1))) == 0);

    /* Obtain page number */
    page_number = address & ~((ub8)(page_size - 1));

    /* If next block is consecutive to previous block or it
     * falls in the last accessed page, add it to the last 
     * region. */
    if ((address != last_block)) 
    {
      if (last_page == UNDEF_ADDRESS || 
          (page_number != last_page && page_number != last_page + 1 && 
          (last_page > 0 && page_number != last_page - 1)))
      {
        if (last_region)
        {
          last_region->setEnd(last_block);
        }

        /* Create a new region add a block it to it */ 
        last_region = new OneRegion();
        assert(last_region);

        /* Set start address of the new region */
        last_region->setStart(page_number);

        all_region_map.insert(pair<ub8, ub8>(region_id, (ub8)last_region));

        /* Increment region id */
        region_id++;
      }
    }
    
    assert(last_region);

    /* Set start address of the new region */
    last_region->setEnd(address + block_size);
    
    assert(address + block_size > page_number);

    /* Update reuse count */
    last_region->addReuse(reuse_count);
    last_region->addReplacement(replace_count);

    /* Update last addresses */
    last_block  = address + block_size;
    last_page   = page_number;  /* Update last page number */
  }

  ub8 getRegionCount()
  {
    return all_region_map.size();
  }

  void printRegions()
  {
    map<ub8, ub8>::iterator region_itr;
    ub8 start_page;
    ub8 end_page;

    out_stream << "Region;" << "Start;" << "End;" << "PageCount;" << "TotalReuse;" << " TotalReplacement;";
    out_stream << "Size(Bytes);" << "Blocks" << endl;

    OneRegion *current_region;  
    for (region_itr = all_region_map.begin(); region_itr != all_region_map.end(); region_itr++)
    {
      current_region = (OneRegion *)(region_itr->second);
      assert(current_region->getEnd() > current_region->getStart());

      start_page  = (current_region->getStart() / page_size);
      end_page    = (current_region->getEnd() / page_size);

      out_stream << region_itr->first << ";" << start_page << ";" << end_page << ";" << (end_page - start_page);
      out_stream << ";" << current_region->getReuse() << ";" << current_region->getReplacement() << ";";
      out_stream << current_region->getEnd() - current_region->getStart() << ";";
      out_stream << (current_region->getEnd() - current_region->getStart()) / block_size << endl;
    }
  }
};

#endif
