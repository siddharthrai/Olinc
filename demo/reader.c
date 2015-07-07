#include "zlib.h"
#include "assert.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "stdlib.h"
#include "../export-trace/export.h"

int main(int argc, char **argv)
{
  sb1*          trc_file_name;/* Trace file name */
  gzFile*       trc_file;     /* Trace file */
  memory_trace  trace;        /* Memory trace */
  sb4           opt;          /* Current option value */
  ub1           opts;         /* # options */
  
  opts = 0;
  
  /* Parse command line arguments */
  while ((opt = getopt(argc, argv, "f:h:")) != -1)
  {
    opts++;

    switch (opt)
    {
      case 'f':
        trc_file_name = (sb1 *) malloc(strlen(optarg) + 1);
        strncpy(trc_file_name, optarg, strlen(optarg));
        printf("Opening trace file f:%s\n", trc_file_name);
        break;

      case 'h':
        printf("Syntax: reader -f file_name\n", opt);
        exit(EXIT_FAILURE);
        break;

      default:
        printf("Invalid option %c\n", opt);
        printf("Syntax: reader -f file_name\n", opt);
        exit(EXIT_FAILURE);
    }
  }
  
  /* Ensure there is only one argument */
  if (opts != 1)
  {
    printf("Syntax: reader -f file_name\n", opt);
    exit(EXIT_FAILURE);
  }

  trc_file = gzopen(trc_file_name, "rb9");
  assert(trc_file);
  
  /* Read entire trace file */
  while (!gzeof(trc_file))
  {
    assert(gzread(trc_file, &trace, sizeof(memory_trace)) != -1);
    if (!gzeof(trc_file))
    {
      if (trace.fill == TRUE && trace.stream != PS)
      {
        printf("Original Stream %2s ", stream_name[trace.stream]);
        printf("Phy Address %10ld ", trace.address);
        printf("Vtl Address %10ld ", trace.vtl_addr);
        printf("Classified stream  %s\n", sap_stream_name[trace.sap_stream]); 
      }
    }
  }

  gzclose(trc_file);
}
