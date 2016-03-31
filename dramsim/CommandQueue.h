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






#ifndef CMDQUEUE_H
#define CMDQUEUE_H

//CommandQueue.h
//
//Header
//

#include "BusPacket.h"
#include "BankState.h"
#include "Transaction.h"
#include "SystemConfiguration.h"
#include "SimulatorObject.h"
#include "../common/intermod-common.h"

using namespace std;

namespace DRAMSim
{


        typedef vector<BusPacket *> BusPacket1D;
        typedef vector<BusPacket1D> BusPacket2D;
        typedef vector<BusPacket2D> BusPacket3D;
        
        /* 
         * Staged memory scheduler
         *
         * Our implementation of SMS, maintains one queue for each source unit and one batch
         * queue for all sources combined. 
         *
         * 
         * */
        typedef enum sms_batching_mode
        {
          batching_mode_invalid = 0,
          batching_mode_pick    = 0,
          batching_mode_drain   = 0
        }sms_mode;
        
#define SMS_SRC_Q(s)   ((s)->queue)
#define SMS_SRC_SZ(s)  ((s)->queue_size)
#define SMS_SRC_ENT(s) ((s)->queue_entries)
#define SMS_SRC_RDY(s) ((s)->ready)
#define SMS_SRC_AGE(s) ((s)->age)
#define SMS_SRC_ROW(s) ((s)->current_row_id)

        /* Staged memory scheduler specific info */
        typedef struct sms_source_queue
        {
          BusPacket1D queue;          // 1D array of BusPacket pointers
          size_t      queue_size;     // Maximum queue size 
          size_t      queue_entries;  // Entries in queue
          int         ready;          // True, if batch is ready
          int         age;            // Cycle when batch became ready
          unsigned    current_row_id; // Current row id
        }sms_queue;

#define SMS_BCH_Q(s)        ((s)->queue)
#define SMS_BCH_MODE(s)     ((s)->mode)
#define SMS_BCH_SRC(s)      ((s)->src)
#define SMS_BCH_PRB(s)      ((s)->psjf)
#define SMS_BCH_REQ(s, st)  ((s)->src_req[st])

        /* Staged memory scheduler specific info */
        typedef struct sms_batch_queue
        {
          BusPacket3D queue;            // 3D array of BusPacket pointers
          sms_mode    mode;             // Mode of operation of batch scheduler
          int         src;              // Source stream of batch to be drained
          float       psjf;             // Probability of using SJF policy
          int         src_req[TST + 1]; // Probability of using SJF policy
          int         rr_next;          // Next source for RR policy
        }sms_bqueue;

#define SMS_SCHD_SRC_Q(s, st)  ((s)->source_queue[st])
#define SMS_SCHD_BCH_Q(s)      ((s)->batch_queue)

        typedef struct sms_scheduler
        {
          ub8         cycle;                  /* Current scheduler cycle */
          sms_queue   source_queue[TST + 1];  /* Queue per source */
          sms_bqueue  batch_queue;            /* Batch queue */
        }sms;
        
        void init_sms(sms *sched, size_t bank_queue, size_t queue_size);
        bool has_room_for_sms(sms *sched, int stream);
        void enqueue_sms(sms *sched, int stream, BusPacket *newBusPacket);
        void execute_batching_sms(sms *sched, unsigned rank, unsigned bank);
        vector<BusPacket *> get_command_queue_sms(sms *sched, unsigned rank, unsigned bank); 
        void remove_request_sms(sms *sched, BusPacket *newBusPacket);

        class CommandQueue : public SimulatorObject
        {
                CommandQueue();
                ostream &dramsim_log;
        public:
                //typedefs
                typedef vector<BusPacket *> BusPacket1D;
                typedef vector<BusPacket1D> BusPacket2D;
                typedef vector<BusPacket2D> BusPacket3D;

                //functions
                CommandQueue(vector< vector<BankState> > &states, ostream &dramsim_log);
                virtual ~CommandQueue();

                void enqueue(BusPacket *newBusPacket);
                bool pop(BusPacket **busPacket);
                bool hasRoomFor(unsigned numberToEnqueue, unsigned rank, unsigned bank, bool speedup);
                bool isIssuable(BusPacket *busPacket);
                bool isEmpty(unsigned rank);
                void needRefresh(unsigned rank);
                void print();
                void update(); //SimulatorObject requirement
                vector<BusPacket *> &getCommandQueue(unsigned rank, unsigned bank, bool speedup);

                //fields
                ub1 priorityStream;

                BusPacket3D queues; // 3D array of BusPacket pointers
                BusPacket3D speedup_queues; // 3D array of BusPacket pointers
                vector< vector<BankState> > &bankStates;
                
                sms sms_policy;

                void setPriorityStream(ub1 stream)
                {
                  priorityStream = stream;
                }
        private:
                void nextRankAndBank(unsigned &rank, unsigned &bank);
                //fields
                unsigned nextBank;
                unsigned nextRank;

                unsigned nextBankPRE;
                unsigned nextRankPRE;

                unsigned refreshRank;
                bool refreshWaiting;

                vector< vector<unsigned> > tFAWCountdown;
                vector< vector<unsigned> > rowAccessCounters;

                bool sendAct;

                int  scheduling_policy;
        };
}

#endif

