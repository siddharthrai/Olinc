#include "zlib.h"
#include "assert.h"
#include "../common/intermod-common.h"

/* Take address diff of two files */
int main(int argc, char **argv)
{
  gzFile *trc_file[2];
  sb4 i;
  ub8 address[2];
  ub8 pc[2];
  ub8 seq_no;

  memory_trace trace[2];

  assert(argc == 3);

  printf("Opening tracefile %s\n", argv[1]);
  trc_file[0] = gzopen(argv[1], "rb9");
  assert(trc_file[0]);
  
  printf("Opening tracefile %s\n", argv[2]);
  trc_file[1] = gzopen(argv[2], "rb9");
  assert(trc_file[1]);
  
  seq_no = 0;

  while (!gzeof(trc_file[0]) || !gzeof(trc_file[1]))
  {
    seq_no++;

    address[0] = 0;
    address[1] = 0;

    if (!gzeof(trc_file[0]))
    {
      assert(gzread(trc_file[0], &trace[0], sizeof(memory_trace)) != -1);
    }

    if (!gzeof(trc_file[1]))
    {
      assert(gzread(trc_file[1], &trace[1], sizeof(memory_trace)) != -1);
    }

    if (!gzeof(trc_file[0]))
    {
      if (trace[0].dbuf == FALSE && trace[0].stream >= PS)
      {
        address[0]  = trace[0].address;
        pc[0]       = trace[0].pc;
      }
    }

    if (!gzeof(trc_file[1]))
    {
      if (trace[1].dbuf == FALSE && trace[1].stream >= PS)
      {
        address[1]  = trace[1].address;
        pc[1]       = trace[1].pc;
      }
    }
    
    /* If addrsses differ print sequence number and address */
    if (address[0] != address[1])
    {
      printf("SeqNo: %ld PC1: %lx Address1 %lx PC2 %lx Address2 %lx\n", seq_no, pc[0], address[0], pc[1], address[1]);
    }
  }

  gzclose(trc_file);
}
