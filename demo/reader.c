#include "zlib.h"
#include "assert.h"
#include "../common/intermod-common.h"

int main(int argc, char **argv)
{
  gzFile *trc_file;

  int   i;
  long  dbuf_access;      /* # default RT access */
  long  dbuf_write;       /* # default RT write */
  long  nondbuf_access;   /* # non-default RT access */
  long  nondbuf_write;    /* # non-default RT write */
  long  dbuf_c_access;    /* # default RT access */
  long  dbuf_c_write;     /* # default RT write */
  long  nondbuf_c_access; /* # non-default RT access */
  long  nondbuf_c_write;  /* # non-default RT write */
  long  dbuf_z_access;    /* # default RT access */
  long  dbuf_z_write;     /* # default RT write */
  long  nondbuf_z_access; /* # non-default RT access */
  long  nondbuf_z_write;  /* # non-default RT write */
  long  dbuf_b_access;    /* # default RT access */
  long  dbuf_b_write;     /* # default RT write */
  long  nondbuf_b_access; /* # non-default RT access */
  long  nondbuf_b_write;  /* # non-default RT write */
  long  prev_block;       /* Address of the previous block */

  memory_trace trace;
  
  printf("Opening tracefile %s\n", argv[1]);
  trc_file = gzopen(argv[1], "rb9");
  assert(trc_file);
  
  nondbuf_access    = 0;
  nondbuf_write     = 0;
  dbuf_access       = 0;
  dbuf_write        = 0;
  nondbuf_c_access  = 0;
  nondbuf_c_write   = 0;
  dbuf_c_access     = 0;
  dbuf_c_write      = 0;
  nondbuf_z_access  = 0;
  nondbuf_z_write   = 0;
  dbuf_z_access     = 0;
  dbuf_z_write      = 0;
  nondbuf_b_access  = 0;
  nondbuf_b_write   = 0;
  dbuf_b_access     = 0;
  dbuf_b_write      = 0;
  prev_block        = 0;

  while (!gzeof(trc_file))
  {
    assert(gzread(trc_file, &trace, sizeof(memory_trace)) != -1);
    if (!gzeof(trc_file))
    {
#if 0
      printf("VTL Address %lx\n", trace.vtl_addr);
#endif      
      if (trace.dbuf == FALSE)
      {
        if (trace.stream == CS)
        {
          nondbuf_c_access += 1;

          if (trace.spill)
          {
            nondbuf_c_write += 1;
          }
        }
        else
        {
          if (trace.stream == ZS)
          {
            nondbuf_z_access += 1;

            if (trace.spill)
            {
              nondbuf_z_write += 1;
            }
          }
          else
          {
            if (trace.stream == BS)
            {
              nondbuf_b_access += 1;

              if (trace.spill)
              {
                nondbuf_b_write += 1;
              }
            }
            else
            {
              nondbuf_access += 1;

              if (trace.spill)
              {
                nondbuf_write += 1;
              }
            }
          }
        }
      }
      else
      {
        if (trace.stream == CS)
        {
          dbuf_c_access += 1;

          if (trace.spill)
          {
            dbuf_c_write += 1;
          }
        }
        else
        {
          if(trace.stream == ZS)
          {
            dbuf_z_access += 1;

            if (trace.spill)
            {
              dbuf_z_write += 1;
            }
          }
          else
          {
            if(trace.stream == BS)
            {
              dbuf_b_access += 1;

              if (trace.spill)
              {
                dbuf_b_write += 1;
              }
            }
            else
            {
              dbuf_access += 1;

              if (trace.spill)
              {
                dbuf_write += 1;
              }
            }
          }
        }
      }

      if (trace.spill && trace.stream == ZS)
      {
        printf("zS spill \n"); 
      }
#if 0
      if (trace.stream == BS)
      {
        printf("BS write \n"); 
      }
#endif
      prev_block = trace.vtl_addr;
    }
  }

  gzclose(trc_file);

  printf("Color: DBuf access: %ld | NonDBuf access: %ld\n", dbuf_c_access, nondbuf_c_access);
  printf("Color: DBuf write: %ld | NonDBuf write: %ld\n", dbuf_c_write, nondbuf_c_write);
  printf("Depth: DBuf access: %ld | NonDBuf access: %ld\n", dbuf_z_access, nondbuf_z_access);
  printf("Depth: DBuf write: %ld | NonDBuf write: %ld\n", dbuf_z_write, nondbuf_z_write);
  printf("Blitter: DBuf access: %ld | NonDBuf access: %ld\n", dbuf_b_access, nondbuf_b_access);
  printf("Blitter: DBuf write: %ld | NonDBuf write: %ld\n", dbuf_b_write, nondbuf_b_write);
  printf("Other: DBuf access: %ld | NonDBuf access: %ld\n", dbuf_access, nondbuf_access);
  printf("Other: DBuf write: %ld | NonDBuf write: %ld\n", dbuf_write, nondbuf_write);
}
