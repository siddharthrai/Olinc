#include "zlib.h"
#include "assert.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "../export-trace/export.h"

#define BL                  (8)
#define JEDEC_DATA_BUS_BITS (64)
#define NUM_CHANS           (1)
#define NUM_RANKS           (2)
#define NUM_BANKS           (8)
#define NUM_ROWS            (32768)
#define NUM_COLS            (1024)
#define LOG_BLOCK_SIZE      (6)

#define CHAN(i)             ((i)->newTransactionChan)
#define COLM(i)             ((i)->newTransactionColumn)
#define BANK(i)             ((i)->newTransactionBank)
#define RANK(i)             ((i)->newTransactionRank)
#define ROW(i)              ((i)->newTransactionRow)

typedef struct dram_config
{
  ub8 newTransactionChan;
  ub8 newTransactionColumn;
  ub8 newTransactionBank;
  ub8 newTransactionRank;
  ub8 newTransactionRow;
  ub8 ****access[TST + 1];
}dram_config;

void dram_config_init(dram_config *dram_info)
{
  ub4 i;
  ub4 j;
  ub4 k;
  ub1 s;

  for (s = 0; s < TST; s++)
  {
    dram_info->access[s] = (ub8 ****)calloc(NUM_CHANS, sizeof(ub8 ***));
    assert(dram_info);

    for (i = 0; i < NUM_CHANS; i++)
    {
      dram_info->access[s][i] = (ub8 ***)calloc(NUM_RANKS, sizeof(ub8 **));
      assert(dram_info->access[s][i]);

      for (j = 0; j < NUM_RANKS; j++)
      {
        dram_info->access[s][i][j] = (ub8 **)calloc(NUM_BANKS, sizeof(ub8 *));
        assert(dram_info->access[s][i][j]);

        for (k = 0; k < NUM_BANKS; k++)
        {
          dram_info->access[s][i][j][k] = (ub8 *)calloc(NUM_ROWS, sizeof(ub8));
          assert(dram_info->access[s][i][j][k]);
        }
      }
    }
  }
}

void print_dram_stats(ub1 stream, dram_config *dram_info)
{
  ub8 total_rows;
  ub8 rows_access;
  ub8 min_access;
  ub8 max_access;
  ub8 i;
  ub8 j;
  ub8 k;
  ub8 l;

  total_rows  = 0;
  rows_access = 0;
  min_access  = 0xffffff;
  max_access  = 0;
  
  for (i = 0; i < NUM_CHANS; i++)
  {
    for (j = 0; j < NUM_RANKS; j++)  
    {
      for (k = 0; k < NUM_BANKS; k++)
      {
        for (l = 0; l < NUM_ROWS; l++)
        {
          if (dram_info->access[stream][i][j][k][l])
          {
            total_rows  += 1;     
            if (dram_info->access[stream][i][j][k][l] < min_access)
            {
              min_access = dram_info->access[stream][i][j][k][l];
            }

            if (dram_info->access[stream][i][j][k][l] > max_access)
            {
              max_access = dram_info->access[stream][i][j][k][l];
            }
          }
          
          rows_access += dram_info->access[stream][i][j][k][l];     
        }
      }
    }
  }

  printf(" ROW: %ld ACC: %ld AVG: %ld MIN: %ld MAX:%ld\n", total_rows,
      rows_access, (ub8)(rows_access / total_rows), min_access, max_access);
}

void get_address_map(ub1 stream, memory_trace *info, dram_config *dram_info)
{
  ub8 physicalAddress;
  ub8 tmpPhysicalAddress;
  ub8 tempA;
  ub8 tempB;
  ub8 transactionMask;

  ub4 byteOffsetWidth;
  ub4 transactionSize;
  ub4 channelBitWidth;
  ub4 rankBitWidth;
  ub4 bankBitWidth;
  ub4 rowBitWidth;
  ub4 colBitWidth;
  ub4 colHighBitWidth;
  ub4 colLowBitWidth;
  ub4 llcTagLowBitWidth;

  llcTagLowBitWidth = LOG_BLOCK_SIZE;
  transactionSize   = (JEDEC_DATA_BUS_BITS / 8) * BL;
  transactionMask   = transactionSize - 1;
  channelBitWidth   = log2(NUM_CHANS);
  rankBitWidth      = log2(NUM_RANKS);
  bankBitWidth      = log2(NUM_BANKS);
  rowBitWidth       = log2(NUM_ROWS);
  colBitWidth       = log2(NUM_COLS);

  byteOffsetWidth = log2((JEDEC_DATA_BUS_BITS / 8)); 
  
  tmpPhysicalAddress  = info->address;
  physicalAddress     = info->address >> byteOffsetWidth;
  colLowBitWidth      = log2(transactionSize) - byteOffsetWidth;
  colHighBitWidth     = colBitWidth - colLowBitWidth;

  //row:rank:bank:col:chan
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> channelBitWidth;
  tempB = physicalAddress << channelBitWidth;
  dram_info->newTransactionChan = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> colHighBitWidth;
  tempB = physicalAddress << colHighBitWidth;
  dram_info->newTransactionColumn = tempA ^ tempB;

  // bank id
  tempA = physicalAddress;
  physicalAddress = physicalAddress >> bankBitWidth;
  tempB = physicalAddress << bankBitWidth;
  dram_info->newTransactionBank = tempA ^ tempB;

  /* XOR with lower LLC tag bits */
  tempA = tmpPhysicalAddress >> llcTagLowBitWidth;
  tempB = (tempA >> bankBitWidth) << bankBitWidth;
  dram_info->newTransactionBank = dram_info->newTransactionBank ^ (tempA ^ tempB);

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rankBitWidth;
  tempB = physicalAddress << rankBitWidth;
  dram_info->newTransactionRank = tempA ^ tempB;

  tempA = physicalAddress;
  physicalAddress = physicalAddress >> rowBitWidth;
  tempB = physicalAddress << rowBitWidth;
  dram_info->newTransactionRow = tempA ^ tempB;

  assert(dram_info->access);

  dram_info->access[stream][CHAN(dram_info)][RANK(dram_info)][BANK(dram_info)][ROW(dram_info)] += 1;
}

int main(int argc, char **argv)
{
  sb1*          trc_file_name;/* Trace file name */
  gzFile*       trc_file;     /* Trace file */
  memory_trace  trace;        /* Memory trace */
  sb4           opt;          /* Current option value */
  ub1           opts;         /* # options */
  dram_config   dram_info;    /* DRAM info */ 

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
  
  dram_config_init(&dram_info);

  trc_file = gzopen(trc_file_name, "rb9");
  assert(trc_file);
  
  ub8 old_cycle;
  
  old_cycle = 0;
  /* Read entire trace file */
  while (!gzeof(trc_file))
  {
    assert(gzread(trc_file, &trace, sizeof(memory_trace)) != -1);
    if (!gzeof(trc_file))
    {
      assert(trace.stream > NN && trace.stream < TST + 5);
#if 0
      if (trace.stream == TS && trace.spill == TRUE)
      if (trace.spill == FALSE && trace.sap_stream == sappriority_stream_q)
      if (trace.sap_stream == sappriority_stream_x)
      if (trace.spill == TRUE)
#endif
      if (trace.sap_stream == sappriority_stream_p || trace.sap_stream == sappriority_stream_q || 
          trace.sap_stream == sappriority_stream_r)
      {
#if 0
        if (old_cycle > trace.cycle)
        {
          printf("O:%ld N:%ld\n", old_cycle, trace.cycle); 
          assert(0);
        }

        old_cycle = trace.cycle;
#endif

        printf("Original Stream %2s ", stream_name[trace.stream]);
        printf("core %2d ", trace.core);
#if 0
        printf("cycle %2ld ", trace.cycle);
#endif
        printf("pid %2d ", trace.pid);
        printf("Phy Address %10ld ", trace.address);
        printf("Vtl Address %10ld ", trace.vtl_addr);
        printf("Fill %2d ", trace.fill);
        printf("Spill %2d ", trace.spill);
        printf("Dirty %2d ", trace.dirty);
        printf("Classified stream  %s\n", sap_stream_name[trace.sap_stream]); 
      }

#if 0
      if (trace.stream == CS)
      {
        get_address_map(CS, &trace, &dram_info);
      }

      if (trace.stream == ZS)
      {
        get_address_map(ZS, &trace, &dram_info);
      }

      if (trace.stream == TS)
      {
        get_address_map(TS, &trace, &dram_info);
      }
#endif
    }
  }

#if 0  
  printf("Color:\n");
  print_dram_stats(CS, &dram_info);

  printf("\nDepth:\n");
  print_dram_stats(ZS, &dram_info);

  printf("\nTexture:\n");
  print_dram_stats(TS, &dram_info);
#endif

  gzclose(trc_file);
}

#undef CHAN
#undef COLM
#undef BANK
#undef RANK
#undef ROW

