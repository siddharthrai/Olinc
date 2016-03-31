#include <cassert>
#include <cstdio>
#include <map>
#include <list>
#include <string>

#include "DRAMSim.h"
#include "AddressMapping.h"
#include "MultiChannelMemorySystem.h"


using namespace std;
using namespace DRAMSim;

extern long long agent_req_pending;

int SHOW_SIM_OUTPUT = 1;

typedef pair<int, void*> RequestPair;
typedef map<uint64_t, list<memory_trace*> > Requests;
typedef map<unsigned, RequestPair > WaitingRequests;

void fatal(const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  fprintf(stderr, "fatal: ");
  vfprintf(stderr, fmt, va);
  fprintf(stderr, "\n");
  fflush(NULL);
  exit(1);
}

class Receiver
{
private:
        int num_pending;
        Requests pendingReadRequests;
        Requests pendingWriteRequests;
        list<memory_trace *> compQueue;          /* Request completion queue */

public:
      
        memory_trace* getNextCompleted()
        {
          memory_trace *ret;

          if (!compQueue.empty()) 
          {
            ret = compQueue.front(); 
            compQueue.pop_front(); 
          }
          else
          {
            ret = NULL;
          }

          return ret;
        }
        
        void requestCompleted(memory_trace *info)
        {
          compQueue.push_back(info);
        }

        Receiver() : num_pending(0) {
        }


        int requests_pending() {
                return num_pending;
        }

        void add_pending(int isWrite, uint64_t address, memory_trace *info)
        {
          /* To maintain the correct order always push to the back and
           * remove at the front. */
          if (isWrite)
            pendingWriteRequests[address].push_back(info);
          else
            pendingReadRequests[address].push_back(info);

          num_pending++;
        }

        void read_complete(unsigned id, uint64_t address, bool rowHit, uint64_t done_cycle) 
        {
          memory_trace *info;
          Requests::iterator it;
          dram_queue_time *time;
          
          time = (dram_queue_time *)done_cycle;

          it = pendingReadRequests.find(address);
          if (it == pendingReadRequests.end() || it->second.size() == 0)
            fatal("DRAMSim : Cant find a pending read");

          info = pendingReadRequests[address].front();
          
          info->prefetch = rowHit;
          
          if (info->policy_data)
          {
            dram_queue_time *dram_data = (dram_queue_time*)(info->policy_data);
            
            assert(dram_data->tq_time == 0 && dram_data->cq_time == 0);

            assert(time->tq_end >= time->tq_start);  
            dram_data->tq_time = time->tq_end - time->tq_start;

            assert(time->cq_end >= time->cq_start);  
            dram_data->cq_time = time->cq_end - time->cq_start;
          }

          pendingReadRequests[address].pop_front();

          requestCompleted(info);

          num_pending--;
        }


        void write_complete(unsigned id, uint64_t address, bool rowHit, uint64_t done_cycle) 
        {
          memory_trace *info;
          Requests::iterator it;

          it = pendingWriteRequests.find(address);
          if (it == pendingWriteRequests.end() || it->second.size() == 0)
            fatal("DRAMSim : Cant find a pending write");

          info = pendingWriteRequests[address].front();

          pendingWriteRequests[address].pop_front();

          requestCompleted(info);

          num_pending--;
        }
};

static MultiChannelMemorySystem *memSystem;
static Receiver *receiver;
static Callback_t *read_cb;
static Callback_t *write_cb;

static WaitingRequests waitingRequests;
static unsigned numControllers;


/* Initializes the DRAMSim and returns DRAM tick
 * A tick is equal to the cycle time in pico seconds. */
extern "C"
int dramsim_init(const char *deviceIniFilename, unsigned int megsOfMemory, unsigned int num_controllers,
                 int trans_queue_depth, int _llcTagLowBitWidth) {
        typedef Callback<Receiver, void, unsigned, uint64_t, bool, uint64_t> ReceiverCallback;
        char buf[200];

        /* SystemIniFile variables are passed through OverrideMap.
         * Create OverrideMap for system ini file variables */
        IniReader::OverrideMap kv_map;

        /* NUM_CHANS : number of logically independent channels (each with a separate 
         * memory controller), should be a power of 2 */
        snprintf(buf, sizeof buf, "%u", num_controllers);
        (kv_map)[string("NUM_CHANS")] = string(buf);

        numControllers = num_controllers;

        /* JEDEC_DATA_BUS_BITS : Always 64 for DDRx, if you want multiple ganged
         *  channels, set this to N*64*/
        (kv_map)[string("JEDEC_DATA_BUS_BITS")] = string("64");

        /* TRANS_QUEUE_DEPTH : transaction queue, i.e. CPU-level commands such as:  READ 0xbeef */
        snprintf(buf, sizeof buf, "%d", trans_queue_depth);
        (kv_map)[string("TRANS_QUEUE_DEPTH")] = string(buf);

        /* CMD_QUEUE_DEPTH : command queue, i.e. DRAM-level commands such as: CAS 544, RAS 4 */
        snprintf(buf, sizeof buf, "%d", (trans_queue_depth * 2));
        (kv_map)[string("CMD_QUEUE_DEPTH")] = string(buf);

        /* EPOCH_LENGTH : length of an epoch in cycles (granularity of simulation) */
        (kv_map)[string("EPOCH_LENGTH")] = string("100000000000");

        /* ROW_BUFFER_POLICY : close_page or open_page */
        (kv_map)[string("ROW_BUFFER_POLICY")] = string("open_page");

        /* ADDRESS_MAPPING_SCHEME : valid schemes 1-8; For multiple independent 
         * channels, use scheme7 since it has the most parallelism  */
        //(kv_map)[string("ADDRESS_MAPPING_SCHEME")] = string("scheme8");

        /* SCHEDULING_POLICY : bank_then_rank_round_robin or rank_then_bank_round_robin */
        (kv_map)[string("SCHEDULING_POLICY")] = string("bank_then_rank_round_robin");

        /* QUEUING_STRUCTURE : per_rank or per_rank_per_bank */
        (kv_map)[string("QUEUING_STRUCTURE")] = string("per_rank_per_bank");

        /* Debug parameters */
        (kv_map)[string("DEBUG_TRANS_Q")] = string("false");
        (kv_map)[string("DEBUG_CMD_Q")] = string("false");
        (kv_map)[string("DEBUG_ADDR_MAP")] = string("false");
        (kv_map)[string("DEBUG_BUS")] = string("false");
        (kv_map)[string("DEBUG_BANKSTATE")] = string("false");
        (kv_map)[string("DEBUG_BANKS")] = string("false");
        (kv_map)[string("DEBUG_POWER")] = string("false");
        (kv_map)[string("VIS_FILE_OUTPUT")] = string("false");
        (kv_map)[string("VERIFICATION_OUTPUT")] = string("false");

        /* USE_LOW_POWER : go into low power mode when idle */
        (kv_map)[string("USE_LOW_POWER")] = string("false");

        if (!*deviceIniFilename)
        {
                /* DDR3 2Gb x8 SDRAM device config*/
                (kv_map)[string("NUM_BANKS")] = string("8");
                (kv_map)[string("NUM_ROWS")] = string("32768");
                (kv_map)[string("NUM_COLS")] = string("1024");
                (kv_map)[string("DEVICE_WIDTH")] = string("8");

                (kv_map)[string("REFRESH_PERIOD")] = string("7800");
                (kv_map)[string("tCK")] = string("0.938");

                (kv_map)[string("CL")] = string("12");
                (kv_map)[string("AL")] = string("0");

                (kv_map)[string("BL")] = string("8");
                (kv_map)[string("tRAS")] = string("36");
                (kv_map)[string("tRCD")] = string("12");
                (kv_map)[string("tRRD")] = string("4");
                (kv_map)[string("tRC")] = string("48");
                (kv_map)[string("tRP")] = string("12");
                (kv_map)[string("tCCD")] = string("4");
                (kv_map)[string("tRTP")] = string("4");
                (kv_map)[string("tWTR")] = string("4");
                (kv_map)[string("tWR")] = string("16");
                (kv_map)[string("tRTRS")] = string("1");
                (kv_map)[string("tRFC")] = string("172");
                (kv_map)[string("tFAW")] = string("27");
                (kv_map)[string("tCKE")] = string("3");
                (kv_map)[string("tXP")] = string("3");

                (kv_map)[string("tCMD")] = string("1");

                (kv_map)[string("IDD0")] = string("100");
                (kv_map)[string("IDD1")] = string("130");
                (kv_map)[string("IDD2P")] = string("10");
                (kv_map)[string("IDD2Q")] = string("70");
                (kv_map)[string("IDD2N")] = string("70");
                (kv_map)[string("IDD3Pf")] = string("60");
                (kv_map)[string("IDD3Ps")] = string("60");
                (kv_map)[string("IDD3N")] = string("90");
                (kv_map)[string("IDD4W")] = string("255");
                (kv_map)[string("IDD4R")] = string("230");
                (kv_map)[string("IDD5")] = string("305");
                (kv_map)[string("IDD6")] = string("9");
                (kv_map)[string("IDD6L")] = string("12");
                (kv_map)[string("IDD7")] = string("415");

                (kv_map)[string("Vdd")] = string("1.5");

                (kv_map)[string("SPEEDUP_HINT")] = string("false");
                (kv_map)[string("CPU_SPEEDUP_HINT")] = string("false");
        }

        /* Initialize Receiver */
        receiver = new (nothrow) Receiver();
        if (!receiver)
                fatal("%s: out of memory", __FUNCTION__);

        /* Initialize Read/Write Callback functions */
        read_cb = new (nothrow) ReceiverCallback(receiver, &Receiver::read_complete);
        if (!read_cb)
                fatal("%s: out of memory", __FUNCTION__);

        write_cb = new (nothrow) ReceiverCallback(receiver, &Receiver::write_complete);
        if (!write_cb)
                fatal("%s: out of memory", __FUNCTION__);

        /* Initialize Memory System */
        memSystem = new (nothrow) MultiChannelMemorySystem(string(deviceIniFilename),
                                                           string(""), string(""), string(""), megsOfMemory, NULL, &kv_map);
        if (!memSystem)
                fatal("%s: out of memory", __FUNCTION__);

        memSystem->RegisterCallbacks(read_cb, write_cb, NULL);

        /* Set the llcTagLowBitWith for scheme 8 address mapping */
        llcTagLowBitWidth = _llcTagLowBitWidth;

        /* Return tick : tick is cycle time in picoseconds. tCK represents the
         * cycle time in nano seconds, thus tick for DRAM is (tCK * 1000) */
        return (int) (tCK * 1000);
}


extern "C"
void dramsim_done() {
        dramsim_stats((char *)"DRAM-stats.log");
        delete memSystem;
        delete receiver;
        delete read_cb;
        delete write_cb;
}


extern "C"
void dramsim_update() {
        WaitingRequests::iterator it;

        /* Update memory system */
        memSystem->update();
}


extern "C"
int dramsim_pending() {
        return receiver->requests_pending();
}


extern "C"
void dramsim_stats(char *file_name) {
        memSystem->printStats(true, file_name);
}


extern "C"
unsigned int dramsim_controller(uint64_t addr, char stream) {
        return memSystem->findChannelNumber(addr, stream);
}


extern "C"
int dramsim_will_accept_request(uint64_t addr, bool isWrite, char stream) {
        return memSystem->willAcceptTransaction(addr, isWrite, stream) ? 1 : 0;
}

extern "C"
void dramsim_request(int isWrite, uint64_t addr, char stream, memory_trace *info) {
        bool result;
        speedup_stream_type  new_stream;
        speedup_stream_type  stream_type;
         
        stream_type = (speedup_stream_type)(info->sap_stream);

#define GPU_HINT    (true)
#define CPU_HINT    (false)
#define CPU_PSET(s) ((s) == speedup_stream_p)
#define CPU_QSET(s) ((s) == speedup_stream_q)
#define CPU_RSET(s) ((s) == speedup_stream_r)
#define CPU_PQ(s)   (CPU_PSET(s) || CPU_QSET(s))
#define CPU_PQR(s)  (CPU_PSET(s) || CPU_QSET(s) || CPU_RSET(s))

        if (memSystem->isSpeedupHintEnable(CPU_HINT))
        {
          new_stream = CPU_PQ(stream_type) ? speedup_stream_x : speedup_stream_u;
        }
        else
        {
          new_stream = memSystem->isSpeedupHintEnable(GPU_HINT) ? stream_type : speedup_stream_u;
#if 0
          new_stream = memSystem->isSpeedupHintEnable(GPU_HINT) ? speedup_stream_x : speedup_stream_u;
#endif
        }

#undef GPU_HINT
#undef CPU_HINT
#undef CPU_PSET
#undef CPU_QSET
#undef CPU_RSET
#undef CPU_PQ
#undef CPU_PQR

        result = memSystem->addTransaction(isWrite ? true : false, addr, stream, new_stream);
        assert(result);

        receiver->add_pending(isWrite, addr, info);
}

extern "C" memory_trace* dram_response()
{
        return receiver->getNextCompleted();
}

extern "C"
void dramsim_set_priority_stream(ub1 stream)
{
  memSystem->setPriorityStream(stream);
}

extern "C"
ub8 dramsim_get_open_row(memory_trace *info)
{
  return memSystem->getOpenRow(info->address);
}
