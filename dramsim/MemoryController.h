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


#ifndef MEMORYCONTROLLER_H
#define MEMORYCONTROLLER_H

//MemoryController.h
//
//Header file for memory controller object
//

#include "SimulatorObject.h"
#include "Transaction.h"
#include "SystemConfiguration.h"
#include "CommandQueue.h"
#include "BusPacket.h"
#include "BankState.h"
#include "Rank.h"
#include "CSVWriter.h"
#include "../common/intermod-common.h"
#include <map>

using namespace std;

namespace DRAMSim
{
        class MemorySystem;


        class MemoryController : public SimulatorObject
        {
        public:
                //functions
                MemoryController(MemorySystem* ms, CSVWriter &csvOut_, ostream &dramsim_log_);
                virtual ~MemoryController();

                bool addTransaction(Transaction *trans);
                bool WillAcceptTransaction(bool isWrite, char stream);
                void returnReadData(const Transaction *trans);
                void receiveFromBus(BusPacket *bpacket);
                void attachRanks(vector<Rank *> *ranks);
                void update();
                void setPriorityStream(ub1 stream);
                void printStats(bool finalStats = false);
                void printShortStats(char *file_name);
                void printShortOverallStats(char *file_name);
                void printShortOverallStatsPerStream(char *file_name);
                void resetStats();
                ub8 getOpenRow(unsigned rank, unsigned bank);


                //fields
                vector<Transaction *> read_transactionQueue;
                vector<Transaction *> write_transactionQueue;
                vector<Transaction *> priority_transactionQueue;
                
                // If true, writes are drained 
                bool write_drain;
                ub1  priorityStream;

        private:
                ostream &dramsim_log;
                vector< vector <BankState> > bankStates;
                //functions
                void insertHistogram(unsigned latencyValue, char stream, unsigned rank, unsigned bank);

                //fields
                MemorySystem *parentMemorySystem;

                CommandQueue commandQueue;
                BusPacket *poppedBusPacket;
                vector<unsigned>refreshCountdown;
                vector<BusPacket *> writeDataToSend;
                vector<unsigned> writeDataCountdown;
                vector<Transaction *> returnTransaction;
                vector<Transaction *> pendingReadTransactions;
                map<unsigned, unsigned> latencies; // latencyValue -> latencyCount
                vector<bool> powerDown;

                vector<Rank *> *ranks;

                //output file
                CSVWriter &csvOut;

                // these packets are counting down waiting to be transmitted on the "bus"
                BusPacket *outgoingCmdPacket;
                unsigned cmdCyclesLeft;
                BusPacket *outgoingDataPacket;
                unsigned dataCyclesLeft;

                uint64_t totalTransactions;
                vector<uint64_t> grandTotalBankAccesses;
                vector<uint64_t> totalReadsPerBank;
                vector<uint64_t> totalWritesPerBank;
                vector<uint64_t> totalRowHitsPerBank;
                vector<uint64_t> totalReadRowHitsPerBank;

                vector<uint64_t> totalReadsPerRank;
                vector<uint64_t> totalWritesPerRank;
                vector<uint64_t> totalRowHitsPerRank;
                vector<uint64_t> totalReadRowHitsPerRank;

                vector<vector<uint64_t> *> totalReadsPerBankPerStream;
                vector<vector<uint64_t> *> totalWritesPerBankPerStream;
                vector<vector<uint64_t> *> totalRowHitsPerBankPerStream;
                vector<vector<uint64_t> *> totalReadRowHitsPerBankPerStream;

                vector< uint64_t > totalEpochLatency;
                vector< uint64_t > totalEpochLatencyPerStream;

                unsigned channelBitWidth;
                unsigned rankBitWidth;
                unsigned bankBitWidth;
                unsigned rowBitWidth;
                unsigned colBitWidth;
                unsigned byteOffsetWidth;


                unsigned refreshRank;

        public:
                // energy values are per rank -- SST uses these directly, so make these public 
                vector< uint64_t > backgroundEnergy;
                vector< uint64_t > burstEnergy;
                vector< uint64_t > actpreEnergy;
                vector< uint64_t > refreshEnergy;

        };
}

#endif

