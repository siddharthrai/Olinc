#include "zlib.h"
#include "assert.h"
#include "address-reuse.h"

int main(int argc, char **argv)
{
  gzFile *trc_file;

  memory_trace trace;
  AddressMap   bt_address_map(BS, TS);
  AddressMap   ct_address_map(CS, TS);
  AddressMap   zt_address_map(ZS, TS);

  cout << "Opening tracefile " << argv[1] << endl;

  trc_file = (gzFile *)gzopen(argv[1], "rb9");
  assert(trc_file);
  
  while (!gzeof(trc_file))
  {
    assert(gzread(trc_file, &trace, sizeof(memory_trace)) != -1);
    if (!gzeof(trc_file))
    {
      if (trace.stream != CS || (trace.stream == CS && trace.dbuf == FALSE))
      {
        ct_address_map.addAddress(trace.stream, trace.address);    
      }
      
      if (trace.stream != BS || (trace.stream == BS && trace.spill == TRUE))
      {
        bt_address_map.addAddress(trace.stream, trace.address);    
      }

      zt_address_map.addAddress(trace.stream, trace.address);    
    }
  }
  
  ct_address_map.printMap();
  bt_address_map.printMap();
  zt_address_map.printMap();

  gzclose(trc_file);
}
