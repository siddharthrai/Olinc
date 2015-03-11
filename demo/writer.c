#include "zlib.h"
#include "assert.h"
#include "../common/intermod-common.h"

int main()
{
  gzFile *trc_file;
  int i;

  memory_trace trace;

  trc_file = gzopen("mem-trace.gz", "wb9");
  assert(trc_file);
  
  for (i = 0; i < 10000; i++)
  {
    trace.address = 0x12345;
    trace.fill    = 1;
    trace.spill   = 0;
    trace.stream  = CS;

    gzwrite(trc_file, &trace, sizeof(memory_trace));
  }

  gzclose(trc_file);
}
