#include "zlib.h"
#include "assert.h"
#include "address-map.h"

int main(int argc, char **argv)
{
  gzFile *trc_file;
  int     i;

  memory_trace trace;
  AddressMap   address_map;

  cout << "Opening tracefile " << argv[1] << endl;

  trc_file = (gzFile *)gzopen(argv[1], "rb9");
  assert(trc_file);
  
  while (!gzeof(trc_file))
  {
    assert(gzread(trc_file, &trace, sizeof(memory_trace)) != -1);
    if (!gzeof(trc_file))
    {
      address_map.addAddress(trace.stream, trace.address);    
    }
  }
  
  address_map.printMap();

  gzclose(trc_file);
}
