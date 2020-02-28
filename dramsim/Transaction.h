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

#ifndef TRANSACTION_H
#define TRANSACTION_H

//Transaction.h
//
//Header file for transaction object

#include "SystemConfiguration.h"
#include "BusPacket.h"
#include "../common/intermod-common.h"

using std::ostream;

#define UNKNOWN_EVENT   (0)
#define TQ_START_EVENT  (1)
#define TQ_END_EVENT    (2)
#define CQ_START_EVENT  (3)
#define CQ_END_EVENT    (4)

namespace DRAMSim
{


        enum TransactionType
        {
                DATA_READ,
                DATA_WRITE,
                RETURN_DATA
        };


        class Transaction
        {
                Transaction();
        public:
                //fields
                TransactionType transactionType;
                uint64_t address;
                bool     rowHit;
                char     stream;
                speedup_stream_type stream_type;
                void *data;
                uint64_t timeAdded;
                uint64_t timeReturned;
                uint64_t tq_start;      /* Transaction queue start */
                uint64_t tq_end;        /* Transaction queue end */
                uint64_t cq_start;      /* Command queue start */
                uint64_t cq_end;        /* Command queue end */


                friend ostream &operator<<(ostream &os, const Transaction &t);
                //functions
                Transaction(TransactionType transType, uint64_t addr, bool rowHit, char stream, speedup_stream_type streamType, void *data);
                Transaction(const Transaction &t);


                BusPacketType getBusPacketType() {
                        switch (transactionType)
                        {
                                case DATA_READ:
                                        if (rowBufferPolicy == ClosePage)
                                        {
                                                return READ_P;
                                        }
                                        else if (rowBufferPolicy == OpenPage)
                                        {
                                                return READ;
                                        }
                                        else
                                        {
                                                ERROR("Unknown row buffer policy");
                                                abort();
                                        }
                                        break;
                                case DATA_WRITE:
                                        if (rowBufferPolicy == ClosePage)
                                        {
                                                return WRITE_P;
                                        }
                                        else if (rowBufferPolicy == OpenPage)
                                        {
                                                return WRITE;
                                        }
                                        else
                                        {
                                                ERROR("Unknown row buffer policy");
                                                abort();
                                        }
                                        break;
                                default:
                                        ERROR("This transaction type doesn't have a corresponding bus packet type");
                                        abort();
                        }
                }

                void setEventTime(uint8_t event, uint64_t cycle)
                {
                  switch (event)
                  {
                    case CQ_START_EVENT:
                      cq_start = cycle;
                      break;

                    case TQ_START_EVENT:
                      tq_start = cycle;
                      break;

                    case CQ_END_EVENT:
                      cq_end = cycle;
                      break;

                    case TQ_END_EVENT:
                      tq_end = cycle;
                      break;

                    default:
                      ERROR("This event is invalid");
                      abort();
                      break;
                  }
                }
        };

}

#endif

