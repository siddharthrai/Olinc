#include "zlib.h"
#include "assert.h"
#include "intermod-common.h"
#include <iostream>
#include <map> 

using namespace std;

int main(int argc, char **argv)
{
  gzFile *trc_file;
  ub8 ct_reuse_count;
  ub8 bt_reuse_count;
  memory_trace trace;
  map<ub8, ub8>::iterator reuse_itr;

  map<ub8, ub8> c_blocks;  /* blocks reused between src and tgt */
  map<ub8, ub8> b_blocks;  /* blocks reused between src and tgt */

  cout << "Opening tracefile " << argv[1] << endl;

  trc_file = (gzFile *)gzopen(argv[1], "rb9");
  assert(trc_file);
  
  ct_reuse_count = 0;
  bt_reuse_count = 0;

  while (!gzeof(trc_file))
  {
    assert(gzread(trc_file, &trace, sizeof(memory_trace)) != -1);
    if (!gzeof(trc_file))
    {
      if (trace.stream == TS)
      {
        reuse_itr = c_blocks.find(trace.address);
        if (reuse_itr != c_blocks.end())
        {
          /* Add block into CT block list */
          c_blocks.erase(trace.address);
          ct_reuse_count++;
        }

        reuse_itr = b_blocks.find(trace.address);
        if (reuse_itr != b_blocks.end())
        {
          /* Add block into CT block list */
          b_blocks.erase(trace.address);
          bt_reuse_count++;
        }
      }

      if (trace.stream == CS)
      {
        reuse_itr   = c_blocks.find(trace.address);
        if (reuse_itr == c_blocks.end())
        {
          /* Add block into CT block list */
          c_blocks.insert(pair<ub8, ub8>(trace.address, 0));
        }
        else
        {

        }
      }

      if (trace.stream == BS)
      {
        reuse_itr   = b_blocks.find(trace.address);
        if (reuse_itr == b_blocks.end())
        {
          /* Add block into CT block list */
          b_blocks.insert(pair<ub8, ub8>(trace.address, 0));
        }
      }
    }
  }
  
  gzclose(trc_file);

  cout << "Total CT count " << ct_reuse_count << endl;
  cout << "Total BT count " << bt_reuse_count << endl;
}
