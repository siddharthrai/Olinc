#ifndef INTER_STREAM_REUSE_H
#define INTER_STREAM_REUSE_H

#include "common/intermod-common.h"
#include "./demo/address-reuse.h"
#include "./demo/region-table.h"
#include <map>
#include <iostream>

using namespace std;

/* Cache access callback map */
class CacheAccessCbkMap
{
  ub1 start_callback;             /* Top level entry switch */
  ub1 begin_callback;             /* Begin switch */
  ub1 end_callback;               /* End switch */
  ub1 hit_callback;               /* Hit switch */
  ub1 miss_callback;              /* Miss switch */
  ub1 replace_callback;           /* Replacement switch */
  ub1 exit_callback;              /* Final exit switch */
  ub1 use_va;                     /* TRUE if virtual address is used */

  public:

  /*
   *
   * NAME
   *  
   *  Cache access callback map
   *
   * DESCRIPTION
   *  
   *  Constructor
   *
   * PARAMETERS
   *  
   *  start   (IN) - Switch to top-level entry callback
   *  begin   (IN) - Switch to enable cache access begin callback
   *  end     (IN) - Switch to enable cache access end callback
   *  hit     (IN) - Switch to enable cache access hit callback
   *  miss    (IN) - Switch to enable cache access miss callback
   *  replace (IN) - Switch to enable cache access replacement callback
   *  exit    (IN) - Switch to final exit callback
   *
   * RETURNS
   *
   *  Nothing 
   */

  CacheAccessCbkMap(ub1 start = 0, ub1 begin = 0, ub1 end = 0, ub1 hit = 0, 
      ub1 miss = 0, ub1 replace = 0, ub1 exit = 0, ub1 use_va_in = 0)
  {
    start_callback    = start;
    begin_callback    = begin;
    end_callback      = end;
    hit_callback      = hit;
    miss_callback     = miss;
    replace_callback  = replace;
    exit_callback     = exit;
    use_va            = use_va_in;
  }
};

/* Inter stream reuse statistics class */
class InterStreamReuse
{
  CacheAccessCbkMap callback_map;         /* Map of callbacks which are enable */
  map <ub8, ub8>    block_map;            /* Map of blocks */
  AddressMap        *address_map;         /* Inter stream reuse map */
  RegionTable       *region_table;        /* Region table */
  ub1               use_va;               /* True, if virtual address is used */
  ub1               src_stream;           /* Source stream */
  ub1               tgt_stream;           /* Target stream */

  public:

  /*
   *
   * NAME
   *  
   *  Inter stream Reuse initialization
   *
   * DESCRIPTION
   *  
   *  Constructor
   *
   * PARAMETERS
   *  
   *  map_in (IN) - Callback enable / disable map
   *
   * RETURNS
   *
   *  Nothing 
   */

  InterStreamReuse(ub1 src, ub1 dst, ub1 use_va_in = FALSE)
  {
    callback_map  = CacheAccessCbkMap(1, 1, 1, 1, 1, 1, 1, use_va_in);
    address_map   = new AddressMap(src, dst);
    region_table  = new RegionTable(src, dst);
    use_va        = use_va_in;
    src_stream    = src;
    tgt_stream    = dst;
  }
  
  /*
   *
   * NAME
   *  
   *  Get source stream 
   *
   * DESCRIPTION
   *  
   *  Return source stream 
   *
   * PARAMETERS
   *  
   *  Nothing
   *
   * RETURNS
   *
   *  Nothing 
   */

  ub1 GetSourceStream()
  {
    return src_stream;
  }

  /*
   *
   * NAME
   *  
   *  Get target stream 
   *
   * DESCRIPTION
   *  
   *  Returns target stream 
   *
   * PARAMETERS
   *  
   *  Nothing
   *
   * RETURNS
   *
   *  Nothing 
   */

  ub1 GetTargetStream()
  {
    return tgt_stream;
  }

  /*
   *
   * NAME
   *  
   *  Inter stream Reuse finish 
   *
   * DESCRIPTION
   *  
   *  Destructor
   *
   * PARAMETERS
   *  
   *  Nothing
   *
   * RETURNS
   *
   *  Nothing 
   */

  ~InterStreamReuse()
  {
    delete address_map;
    delete region_table;
  }
  
  /*
   *
   * NAME
   *  
   *  Start callback
   *
   * DESCRIPTION
   *  
   *  Called at the very beginning even before cache accesses start
   *
   * PARAMETERS
   *  
   *  Nothing 
   *
   * RETURNS
   *
   *  Nothing 
   */

  void StartCbk();

  /*
   *
   * NAME
   *  
   *  Cache access begin callback
   *
   * DESCRIPTION
   *  
   *  Called before every cache access
   *
   * PARAMETERS
   *  
   *  info (IN) - Access info
   *
   * RETURNS
   *
   *  Nothing 
   */

  void CacheAccessBeginCbk(memory_trace info, ub8 set_idx, ub8 access, ub8 rplc);

  /*
   *
   * NAME
   *  
   *  Cache access end callback
   *
   * DESCRIPTION
   *  
   *  Called after every cache access
   *
   * PARAMETERS
   *  
   *  info (IN) - Access info
   *
   * RETURNS
   *
   *  Nothing 
   */

  void CacheAccessEndCbk(memory_trace info, ub8 set_idx, ub8 rplc);

  /*
   *
   * NAME
   *  
   *  Cache access hit callback
   *
   * DESCRIPTION
   *  
   *  Called if access hits in the cache
   *
   * PARAMETERS
   *  
   *  info    (IN) - Access info
   *  status  (IN) - Cache access return data
   *
   * RETURNS
   *
   *  Nothing 
   */

  void CacheAccessHitCbk(memory_trace info, cache_access_status status, ub8 set_idx, ub8 access, ub8 rplc);

  /*
   *
   * NAME
   *  
   *  Cache access miss callback
   *
   * DESCRIPTION
   *  
   *  Called if cache access misses in the cache 
   *
   * PARAMETERS
   *  
   *  info    (IN) - Access info
   *
   * RETURNS
   *
   *  Nothing 
   */

  void CacheAccessMissCbk(memory_trace info, ub8 set_idx, ub8 rplc);

  /*
   *
   * NAME
   *  
   *  Cache access replace callback
   *
   * DESCRIPTION
   *  
   *  Called if cache access result in a replacement 
   *
   * PARAMETERS
   *  
   *  info    (IN) - Access info
   *  status  (IN) - Cache access return data
   *
   * RETURNS
   *
   *  Nothing 
   */

  void CacheAccessReplaceCbk(memory_trace info, cache_access_status status, ub8 set_idx, ub8 access, ub8 rplc);

  /*
   *
   * NAME
   *  
   *  Exit callback
   *
   * DESCRIPTION
   *  
   *  Called on final exit
   *
   * PARAMETERS
   *  
   * Nothing 
   *
   * RETURNS
   *
   *  Nothing 
   */

  void ExitCbk();
};

#endif
