#include "zlib.h"
#include "access-phase.h"
#include "../common/intermod-common.h"

int main(int argc, char **argv)
{
  gzFile      *trc_file;
  AccessPhase *all_stream_phase;
  memory_trace trace;

  all_stream_phase = new AccessPhase[TST];
  assert(all_stream_phase);

  all_stream_phase[CS].addAccessPhase(CS);
  all_stream_phase[CS].setTargetStream(CS, TS);

  all_stream_phase[ZS].addAccessPhase(ZS);
  all_stream_phase[ZS].setTargetStream(ZS, TS);

  all_stream_phase[BS].addAccessPhase(BS);
  all_stream_phase[BS].setTargetStream(BS, TS);

  all_stream_phase[TS].addAccessPhase(TS);
  all_stream_phase[TS].setTargetStream(TS, CS);
  
  cout << "Opening tracefile " << argv[1] << endl;

  trc_file = (gzFile *)gzopen(argv[1], "rb9");
  assert(trc_file);
  
  while (!gzeof(trc_file))
  {
    assert(gzread(trc_file, &trace, sizeof(memory_trace)) != -1);
    if (!gzeof(trc_file))
    {
      all_stream_phase[trace.stream].addBlock(trace.vtl_addr, trace.stream); 
      if (trace.stream == TS)
      {
        all_stream_phase[CS].addBlock(trace.vtl_addr, CS, trace.stream); 
        all_stream_phase[ZS].addBlock(trace.vtl_addr, ZS, trace.stream); 
        all_stream_phase[BS].addBlock(trace.vtl_addr, BS, trace.stream); 
      }

      if (trace.stream == CS)
      {
        all_stream_phase[TS].addBlock(trace.vtl_addr, TS, trace.stream); 
      }
    }
  }
  
  delete [] all_stream_phase;

  gzclose(trc_file);
}
