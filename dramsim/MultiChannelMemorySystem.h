/*********************************************************************************
 *  Copyright (c) 2010-2011, Elliott Cooper-Balis
 *                             Paul Rosenfeld
 *                             Bruce Jacob
 *                             University of Maryland 
 *                             dramninjas [at] gmail [dot] com
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  
 *     * Redistributions of source code must retain the above copyright notice,
 *        this list of conditions and the following disclaimer.
 *  
 *     * Redistributions in binary form must reproduce the above copyright notice,
 *        this list of conditions and the following disclaimer in the documentation
 *        and/or other materials provided with the distribution.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************************/
#include "SimulatorObject.h"
#include "Transaction.h"
#include "SystemConfiguration.h"
#include "MemorySystem.h"
#include "IniReader.h"
#include "CSVWriter.h"
#include "../common/intermod-common.h"


namespace DRAMSim
{


        class MultiChannelMemorySystem : public SimulatorObject
        {
        public:

                MultiChannelMemorySystem(const string &dev, const string &sys, const string &pwd, const string &trc, unsigned megsOfMemory, string *visFilename = NULL, const IniReader::OverrideMap *paramOverrides = NULL);
                virtual ~MultiChannelMemorySystem();
                void setPriorityStream(ub1 stream);
                bool addTransaction(Transaction *trans);
                bool addTransaction(bool isWrite, uint64_t addr, char stream, speedup_stream_type stream_type);
                bool willAcceptTransaction(uint64_t addr, bool isWrite, char stream);
                bool willAcceptTransaction(unsigned chan, bool isWrite, char stream);
                void update();
                void printStats(bool finalStats = false, char *file_name = NULL);
                ostream &getLogFile();
                void RegisterCallbacks(
                                       TransactionCompleteCB *readDone,
                                       TransactionCompleteCB *writeDone,
                                       void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower));

                void InitOutputFiles(string tracefilename);
                unsigned findChannelNumber(uint64_t addr, char stream);
                ub8 getOpenRow(ub8 address);

                bool isSpeedupHintEnable(bool is_gpu, speedup_stream_type gpu_stream);

                //output file
                std::ofstream visDataOut;
                ofstream dramsim_log;

        private:
                vector<MemorySystem*> channels;
                unsigned megsOfMemory;
                string deviceIniFilename;
                string systemIniFilename;
                string traceFilename;
                string pwd;
                string *visFilename;
                static void mkdirIfNotExist(string path);
                static bool fileExists(string path);
                CSVWriter *csvOut;

                bool gpu_x_speedup_hint_enable;
                bool gpu_y_speedup_hint_enable;
                bool cpu_speedup_hint_enable;

        };
}
