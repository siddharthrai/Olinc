#ifndef ACCESS_PHASE_H
#define ACCESS_PHASE_H

#include <map>
#include <iostream>
#include "../common/intermod-common.h"
#include "../common/sat-counter.h"
#include <iomanip>
#include <fstream>
#include <string.h>
#include <math.h>
#include "zfstream.h"
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

#define BLOCK_SIZE        (64)
#define PAGE_SIZE         (4096)
#define ACC_BUCKET_BITS   (5)
#define ACC_BUCKETS       (1 << ACC_BUCKET_BITS)
#define PHASE_BITS        (20)
#define PHASE_SIZE        (1 << PHASE_BITS)
#define LBSIZE            (3)
#define CTR_W             (20)
#define CTR_M             (0)
#define CTR_X             ((1 << CTR_W) - 1)


extern sb1 *stream_names[TST + 1];

typedef struct footprint_data
{
  ub4 *footprint;
  ub8  x_stream_reuse;
  ub8  access_count;
  ub8  min_x_stream_phase;
  ub8  max_x_stream_phase;
}footprint_data;

/* Map of all regions */
class PerStreamPhase
{
  ub1            src_stream;          /* Region stream */
  ub1            tgt_stream;          /* Target stream */
  sctr          *s_acc_table;         /* Region table indexed by hash */
  gzofstream     out_stream;          /* Stream to dump region map */
  gzofstream     x_out_stream;        /* Stream to dump x stream reuse */
  map<ub8, ub8>  all_phases;          /* Map of all phases of a stream */
  ub8            phase_count;         /* Total number of phases */
  ub8            access_count;        /* Total number of access */
  map<ub8, ub8>  src_blocks;          /* Source stream blocks */
  map<ub8, ub8>  tgt_blocks;          /* Source stream blocks */
  ub8            x_stream_reuse;      /* Number of cross stream reuse */
  ub8            min_x_stream_phase;  /* Number of cross stream reuse */
  ub8            max_x_stream_phase;  /* Number of cross stream reuse */
  ub4            entry_count;         /* Number of entries in the table */
  ub1            dump_phases;         /* If TRUE phases are dumped */

  public:

  PerStreamPhase(ub1 stream_in, ub4 entry_count_in = ACC_BUCKETS, ub1 dump_phases_in = FALSE)
  {
    src_stream          = stream_in;
    tgt_stream          = NN;
    phase_count         = 0;
    x_stream_reuse      = 0;
    min_x_stream_phase  = -1;
    max_x_stream_phase  = 0;
    entry_count         = entry_count_in;
    dump_phases         = dump_phases_in;
    
    /* Ensure entries are power of 2 */
    assert((entry_count_in & (entry_count_in - 1)) == 0);
    assert(PHASE_BITS >= ACC_BUCKET_BITS);
    assert((CTR_W - (PHASE_BITS - ACC_BUCKET_BITS)) > 0);
    
    /* Initialize accumulator table */
    s_acc_table = new sctr[entry_count];
    assert(s_acc_table);

    for (ub4 i = 0; i < entry_count; i++)
    {
      SAT_CTR_INI(s_acc_table[i], CTR_W, CTR_M, CTR_X);
    }
    
    if (dump_phases_in)
    {
      /* Open output stream */
      sb1 tracefile_name[100];

      assert(strlen("Phase---stats.trace.csv.gz") + 1 < 100);
      sprintf(tracefile_name, "Phase-%s-stats.trace.csv.gz", 
          stream_names[stream_in]);
      out_stream.open(tracefile_name, ios::out | ios::binary);

      if (!out_stream.is_open())
      {
        printf("Error opening output stream\n");
        exit(EXIT_FAILURE);
      }
    }
  } 

  void set_tgt_stream(ub1 stream_in)
  {
    /* Open x stream reuse file */
    sb1 tracefile_name[100];
    
    assert(tgt_stream == NN && stream_in != NN);

    tgt_stream = stream_in;
    
    if (dump_phases)
    {
      assert(strlen("XPhaseReuse-----stats.trace.csv.gz") + 1 < 100);
      sprintf(tracefile_name, "XPhase-%s-%s-stats.trace.csv.gz", 
          stream_names[src_stream], stream_names[tgt_stream]);
      x_out_stream.open(tracefile_name, ios::out | ios::binary);

      if (!x_out_stream.is_open())
      {
        printf("Error opening output stream\n");
        exit(EXIT_FAILURE);
      }
    }
  }

  ub4* get_foot_print()
  {
    ub4 *footprint;
    
    footprint = new ub4[entry_count + 1];
    assert(footprint);
    
    /* Ensure footprint fints in 64 bits */
    assert(s_acc_table); 
    assert(CTR_W - (PHASE_BITS - ACC_BUCKET_BITS) <= 32);
    
    for (ub4 i = 0; i < entry_count; i++)
    {
      footprint[i]  = (int)(SAT_CTR_VAL(s_acc_table[i]) >> (PHASE_BITS - ACC_BUCKET_BITS));
    }
    
    return footprint;
  }

  ~PerStreamPhase()
  {
    footprint_data *data;
    map<ub8, ub8>::iterator phase_itr;
    
    if (dump_phases)
    {
      out_stream << "Phase; Footprint" << endl;

      /* Dump region cache */
      for (phase_itr = all_phases.begin(); phase_itr != all_phases.end(); phase_itr++)
      {
        out_stream << phase_itr->first << ";";
        data = (footprint_data *)(phase_itr->second);

        for (ub4 i = 0; i < entry_count; i++)
        {
          out_stream << (int)(data->footprint[i]);
        } 

        out_stream << endl;
      }

      out_stream.close();

      x_out_stream << "Phase; Reuse; AccessCount; MinXusePhase; MaxXusePhase" << endl;

      /* Dump region cache */
      for (phase_itr = all_phases.begin(); phase_itr != all_phases.end(); phase_itr++)
      {
        x_out_stream << phase_itr->first << ";";
        data = (footprint_data *)(phase_itr->second);

        x_out_stream << data->x_stream_reuse << ";" << data->access_count << ";"
          << data->min_x_stream_phase << ";" << data->max_x_stream_phase;

        x_out_stream << endl;
      }

      /* Close output stream */
      x_out_stream.close();
    }
  }

  ub4 addBlock(ub1 stream, ub8 address)
  {
    ub4 ret;
    ub8 tag_hash;

    map<ub8, ub8>::iterator blk_itr;

    /* Insert blocks into accumulator table */
    ret       = 0;
    tag_hash  = (address / (BLOCK_SIZE << 4)) % entry_count;
      
    if (stream == src_stream)
    {
      access_count++;

      /* Lookup block in source stream */
      blk_itr = src_blocks.find(address);
      if (blk_itr == src_blocks.end())
      {
        src_blocks.insert(pair<ub8, ub8>(address, phase_count));
      }
      else
      {
        /* Update block phase on hit */
        blk_itr->second = phase_count; 
      }

      blk_itr = tgt_blocks.find(address);
      if (blk_itr != tgt_blocks.end())
      {

        x_stream_reuse++;

        if (blk_itr->second <= min_x_stream_phase)
        {
          min_x_stream_phase = blk_itr->second;
        }

        if (blk_itr->second >= max_x_stream_phase)
        {
          max_x_stream_phase = blk_itr->second;
        }
      }

      ret = SAT_CTR_VAL(s_acc_table[tag_hash]);
    }
    else
    {
      assert(stream == tgt_stream);

      /* Lookup block in target stream for inter-stream access */
      blk_itr = tgt_blocks.find(address);
      if (blk_itr == tgt_blocks.end())
      {
        tgt_blocks.insert(pair<ub8, ub8>(address, phase_count));
      }
      else
      {
        /* Update block phase on hit */
        blk_itr->second = phase_count;
      }

      blk_itr = src_blocks.find(address);
      if (blk_itr != src_blocks.end())
      {
        x_stream_reuse++;

        if (blk_itr->second <= min_x_stream_phase)
        {
          min_x_stream_phase = blk_itr->second;
        }

        if (blk_itr->second >= max_x_stream_phase)
        {
          max_x_stream_phase = blk_itr->second;
        }
      }

      SAT_CTR_INC(s_acc_table[tag_hash]);
    }

    return ret;
  }

  void endPhase(ub8 access_count)
  {
    footprint_data *data;

    data = new footprint_data;

    data->x_stream_reuse      = x_stream_reuse;
    data->access_count        = this->access_count;
    data->footprint           = get_foot_print();
    data->min_x_stream_phase  = (min_x_stream_phase == (ub8)(-1)) ? 0 : min_x_stream_phase;
    data->max_x_stream_phase  = max_x_stream_phase;

    all_phases.insert(pair<ub8, ub8>(phase_count++, (ub8)data));

    this->x_stream_reuse      = 0;
    this->min_x_stream_phase  = -1;
    this->max_x_stream_phase  = 0;
    this->access_count        = 0;

    for (ub4 i = 0; i < entry_count; i++)
    {
      SAT_CTR_HLF(s_acc_table[i]);
    }
  }
};

class AccessPhase
{
  ub8 access_count;
  ub8 total_access_count;
  ub1 new_phase;
  map <ub8, PerStreamPhase* > per_stream_phase;
  ub4 per_phase_entries;

  public:
  
  AccessPhase(ub4 entry_count = ACC_BUCKETS)
  {
    access_count        = 0;
    total_access_count  = 0;
    per_phase_entries   = entry_count;
    new_phase           = FALSE;
  }

  ~AccessPhase()
  {
    map<ub8, PerStreamPhase* >::iterator str_itr;

    for (str_itr = per_stream_phase.begin(); str_itr != per_stream_phase.end(); str_itr++)
    {
      delete (PerStreamPhase *)str_itr->second;
    }
  }

  /* Add new stream to per-stream phase */
  void setTargetStream(ub1 src_stream, ub1 stream)
  {
    map<ub8, PerStreamPhase* >::iterator str_itr;

    str_itr = per_stream_phase.find(src_stream);
    assert(str_itr != per_stream_phase.end());
    
    (str_itr->second)->set_tgt_stream(stream);
  }

  /* Add new stream to per-stream phase */
  void addAccessPhase(ub1 stream)
  {
    map<ub8, PerStreamPhase* >::iterator str_itr;

    str_itr = per_stream_phase.find(stream);
    if (str_itr == per_stream_phase.end())
    {
      per_stream_phase.insert(pair<ub8, PerStreamPhase*>(stream, new PerStreamPhase(stream, per_phase_entries)));
    }
  }
  
  /* Add new block to access stream */
  ub1 addBlock(ub8 address, ub1 src_stream, ub1 stream = NN)
  {
    ub1 ret;
    PerStreamPhase *phase;

    if (per_stream_phase.find(src_stream) != per_stream_phase.end())
    {
      phase = per_stream_phase.find(src_stream)->second;
      assert(phase);

      if (stream != NN)
      {
        ret = phase->addBlock(stream, address);
      }
      else
      {
        ret = phase->addBlock(src_stream, address);
      }

      new_phase = FALSE;
    }

    total_access_count++;

    if(++access_count == PHASE_SIZE)
    {
      map<ub8, PerStreamPhase*>::iterator str_itr;
    
      for (str_itr = per_stream_phase.begin(); str_itr != per_stream_phase.end(); str_itr++)
      {
        str_itr->second->endPhase(total_access_count);
      }

      access_count  = 0;
    }

    return ret;
  }
};

#undef BLOCK_SIZE
#undef PAGE_SIZE
#undef ACC_BUCKETS
#undef PHASE_BITS
#undef PHASE_SIZE
#undef CTR_W
#undef CTR_M
#undef CTR_X

#endif
