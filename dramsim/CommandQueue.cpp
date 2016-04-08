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








//CommandQueue.cpp
//
//Class file for command queue object
//

#include "CommandQueue.h"
#include "MemoryController.h"
#include "../common/intermod-common.h"
#include <assert.h>

using namespace DRAMSim;

/* Returns TRUE if random number falls in range [lo, hi] */
static ub1 get_prob_in_range(uf8 lo, uf8 hi)
{
  uf8 val;
  ub1 ret;

  val = (uf8)random() / RAND_MAX;

  if (lo == 1.0F && hi == 1.0F)
  {
    ret = TRUE;
  }
  else
  {
    if (lo == 0.0F && hi == 0.0F)
    {
      ret = FALSE;
    }
    else
    {
      if (val >= lo && val < hi)
      {
        ret = TRUE;
      }
      else
      {
        ret = FALSE;
      }
    }
  }

  return ret;
}

CommandQueue::CommandQueue(vector< vector<BankState> > &states, ostream &dramsim_log_) :
dramsim_log(dramsim_log_),
bankStates(states),
nextBank(0),
nextRank(0),
nextBankPRE(0),
nextRankPRE(0),
refreshRank(0),
refreshWaiting(false),
priorityStream(NN),
sendAct(true) {
        //set here to avoid compile errors
        currentClockCycle = 0;

        //use numBankQueus below to create queue structure
        size_t numBankQueues;
        if (queuingStructure == PerRank)
        {
                numBankQueues = 1;
        }
        else if (queuingStructure == PerRankPerBank)
        {
                numBankQueues = NUM_BANKS;
        }
        else
        {
                ERROR("== Error - Unknown queuing structure");
                exit(0);
        }

        //vector of counters used to ensure rows don't stay open too long
        rowAccessCounters = vector< vector<unsigned> >(NUM_RANKS, vector<unsigned>(NUM_BANKS, 0));

        //create queue based on the structure we want
        BusPacket1D actualQueue;
        BusPacket2D perBankQueue = BusPacket2D();
        queues = BusPacket3D();
        for (size_t rank = 0; rank < NUM_RANKS; rank++)
        {
                //this loop will run only once for per-rank and NUM_BANKS times for per-rank-per-bank
                for (size_t bank = 0; bank < numBankQueues; bank++)
                {
                        actualQueue = BusPacket1D();
                        perBankQueue.push_back(actualQueue);
                }
                queues.push_back(perBankQueue);
        }

        perBankQueue = BusPacket2D();
        speedup_queues = BusPacket3D();
        for (size_t rank = 0; rank < NUM_RANKS; rank++)
        {
                //this loop will run only once for per-rank and NUM_BANKS times for per-rank-per-bank
                for (size_t bank = 0; bank < numBankQueues; bank++)
                {
                        actualQueue = BusPacket1D();
                        perBankQueue.push_back(actualQueue);
                }
                speedup_queues.push_back(perBankQueue);
        }

        //FOUR-bank activation window
        //	this will count the number of activations within a given window
        //	(decrementing counter)
        //
        //countdown vector will have decrementing counters starting at tFAW
        //  when the 0th element reaches 0, remove it
        tFAWCountdown.reserve(NUM_RANKS);
        for (size_t i = 0; i < NUM_RANKS; i++)
        {
                //init the empty vectors here so we don't seg fault later
                tFAWCountdown.push_back(vector<unsigned>());
        }
        
        init_sms(16, 16);
}


CommandQueue::~CommandQueue() {
        //ERROR("COMMAND QUEUE destructor");
        size_t bankMax = NUM_RANKS;
        if (queuingStructure == PerRank)
        {
                bankMax = 1;
        }

        for (size_t r = 0; r < NUM_RANKS; r++)
        {
                for (size_t b = 0; b < bankMax; b++)
                {
                        for (size_t i = 0; i < queues[r][b].size(); i++)
                        {
                                delete(queues[r][b][i]);
                        }
                        queues[r][b].clear();
                }
        }

        for (size_t r = 0; r < NUM_RANKS; r++)
        {
                for (size_t b = 0; b < bankMax; b++)
                {
                        for (size_t i = 0; i < speedup_queues[r][b].size(); i++)
                        {
                                delete(speedup_queues[r][b][i]);
                        }
                        speedup_queues[r][b].clear();
                }
        }
}
//Adds a command to appropriate queue


void CommandQueue::enqueue(BusPacket *newBusPacket) {
  unsigned rank = newBusPacket->rank;
  unsigned bank = newBusPacket->bank;
  
  if (schedulingPolicy == RankThenBankSMS || schedulingPolicy == BankThenRankSMS)
  {
    enqueue_sms(newBusPacket->stream, newBusPacket);
  }
  else
  {
    if (newBusPacket->stream_type == speedup_stream_x)
    {
      if (queuingStructure == PerRank)
      {
        speedup_queues[rank][0].push_back(newBusPacket);
        if (speedup_queues[rank][0].size() > CMD_QUEUE_DEPTH)
        {
          ERROR("== Error - Enqueued more than allowed in command queue");
          ERROR("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
          exit(0);
        }
      }
      else if (queuingStructure == PerRankPerBank)
      {
        speedup_queues[rank][bank].push_back(newBusPacket);
        if (speedup_queues[rank][bank].size() > CMD_QUEUE_DEPTH)
        {
          ERROR("== Error - Enqueued more than allowed in command queue");
          ERROR("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
          exit(0);
        }
      }
      else
      {
        ERROR("== Error - Unknown queuing structure");
        exit(0);
      }
    }
    else
    {
      if (queuingStructure == PerRank)
      {
        queues[rank][0].push_back(newBusPacket);
        if (queues[rank][0].size() > CMD_QUEUE_DEPTH)
        {
          ERROR("== Error - Enqueued more than allowed in command queue");
          ERROR("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
          exit(0);
        }
      }
      else if (queuingStructure == PerRankPerBank)
      {
        queues[rank][bank].push_back(newBusPacket);
        if (queues[rank][bank].size() > CMD_QUEUE_DEPTH)
        {
          ERROR("== Error - Enqueued more than allowed in command queue");
          ERROR("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
          exit(0);
        }
      }
      else
      {
        ERROR("== Error - Unknown queuing structure");
        exit(0);
      }
    }
  }
}

//Removes the next item from the command queue based on the system's
//command scheduling policy


bool CommandQueue::pop(BusPacket **busPacket) {
        //this can be done here because pop() is called every clock cycle by the parent MemoryController
        //	figures out the sliding window requirement for tFAW
        //
        //deal with tFAW book-keeping
        //	each rank has it's own counter since the restriction is on a device level
        for (size_t i = 0; i < NUM_RANKS; i++)
        {
                //decrement all the counters we have going
                for (size_t j = 0; j < tFAWCountdown[i].size(); j++)
                {
                        tFAWCountdown[i][j]--;
                }

                //the head will always be the smallest counter, so check if it has reached 0
                if (tFAWCountdown[i].size() > 0 && tFAWCountdown[i][0] == 0)
                {
                        tFAWCountdown[i].erase(tFAWCountdown[i].begin());
                }
        }

        /* Now we need to find a packet to issue. When the code picks a packet, it will set
         *busPacket = [some eligible packet]
		 
                 First the code looks if any refreshes need to go
                 Then it looks for data packets
                 Otherwise, it starts looking for rows to close (in open page)
         */


        if (rowBufferPolicy == ClosePage)
        {
                bool sendingREF = false;
                //if the memory controller set the flags signaling that we need to issue a refresh
                if (refreshWaiting)
                {
                  bool foundActiveOrTooEarly = false;
                  //look for an open bank
                  for (size_t b = 0; b < NUM_BANKS; b++)
                  {
                    vector<BusPacket *> &speedup_queue = getCommandQueue(refreshRank, b, TRUE);

                    //checks to make sure that all banks are idle
                    if (bankStates[refreshRank][b].currentBankState == RowActive)
                    {
                      foundActiveOrTooEarly = true;
                      bool packetFound = false;

                      //if the bank is open, make sure there is nothing else
                      // going there before we close it
                      for (size_t j = 0; j < speedup_queue.size(); j++)
                      {
                        BusPacket *packet = speedup_queue[j];
                        if (packet->row == bankStates[refreshRank][b].openRowAddress &&
                            packet->bank == b)
                        {
                          if (packet->busPacketType != ACTIVATE && isIssuable(packet))
                          {
                            *busPacket = packet;
                            speedup_queue.erase(speedup_queue.begin() + j);
                            sendingREF = true;
                          }

                          packetFound = true;
                          break;
                        }
                      }

                      if (!packetFound)
                      {
                        vector<BusPacket *> &queue = getCommandQueue(refreshRank, b, FALSE);

                        for (size_t j = 0; j < queue.size(); j++)
                        {
                          BusPacket *packet = queue[j];
                          if (packet->row == bankStates[refreshRank][b].openRowAddress &&
                              packet->bank == b)
                          {
                            if (packet->busPacketType != ACTIVATE && isIssuable(packet))
                            {
                              *busPacket = packet;
                              queue.erase(queue.begin() + j);
                              sendingREF = true;
                            }

                            break;
                          }
                        }
                      }

                      break;
                    }//	NOTE: checks nextActivate time for each bank to make sure tRP is being
                    //				satisfied.	the next ACT and next REF can be issued at the same
                    //				point in the future, so just use nextActivate field instead of
                    //				creating a nextRefresh field
                    else if (bankStates[refreshRank][b].nextActivate > currentClockCycle)
                    {
                      foundActiveOrTooEarly = true;
                      break;
                    }
                  }

                  //if there are no open banks and timing has been met, send out the refresh
                  //	reset flags and rank pointer
                  if (!foundActiveOrTooEarly && bankStates[refreshRank][0].currentBankState != PowerDown)
                  {
                    *busPacket = new BusPacket(REFRESH, 0, 0, speedup_stream_u, 0, 0, refreshRank, 0, 0, dramsim_log);
                    refreshRank = -1;
                    refreshWaiting = false;
                    sendingREF = true;
                  }
                } // refreshWaiting

                //if we're not sending a REF, proceed as normal
                if (!sendingREF)
                {
                  bool foundIssuable = false;
                  unsigned startingRank = nextRank;
                  unsigned startingBank = nextBank;
                  do
                  {
                    vector<BusPacket *> &speedup_queue = getCommandQueue(nextRank, nextBank, TRUE);
                    //make sure there is something in this queue first
                    //	also make sure a rank isn't waiting for a refresh
                    //	if a rank is waiting for a refesh, don't issue anything to it until the
                    //		refresh logic above has sent one out (ie, letting banks close)
                    if (!speedup_queue.empty() && !((nextRank == refreshRank) && refreshWaiting))
                    {
                      if (queuingStructure == PerRank)
                      {

                        //search from beginning to find first issuable bus packet
                        for (size_t i = 0; i < speedup_queue.size(); i++)
                        {
                          if (isIssuable(speedup_queue[i]))
                          {
                            //check to make sure we aren't removing a read/write that is paired with an activate
                            if (i > 0 && speedup_queue[i - 1]->busPacketType == ACTIVATE &&
                                speedup_queue[i - 1]->physicalAddress == speedup_queue[i]->physicalAddress)
                              continue;

                            *busPacket = speedup_queue[i];
                            speedup_queue.erase(speedup_queue.begin() + i);
                            foundIssuable = true;
                            break;
                          }
                        }
                      }
                      else
                      {
                        if (isIssuable(speedup_queue[0]))
                        {

                          //no need to search because if the front can't be sent,
                          // then no chance something behind it can go instead
                          *busPacket = speedup_queue[0];
                          speedup_queue.erase(speedup_queue.begin());
                          foundIssuable = true;
                        }
                      }

                    }

                    vector<BusPacket *> &queue = getCommandQueue(nextRank, nextBank, FALSE);

                    if (!foundIssuable && !queue.empty() && !((nextRank == refreshRank) && refreshWaiting))
                    {
                      if (queuingStructure == PerRank)
                      {

                        //search from beginning to find first issuable bus packet
                        for (size_t i = 0; i < queue.size(); i++)
                        {
                          if (isIssuable(queue[i]))
                          {
                            //check to make sure we aren't removing a read/write that is paired with an activate
                            if (i > 0 && queue[i - 1]->busPacketType == ACTIVATE &&
                                queue[i - 1]->physicalAddress == queue[i]->physicalAddress)
                              continue;

                            *busPacket = queue[i];
                            queue.erase(queue.begin() + i);
                            foundIssuable = true;
                            break;
                          }
                        }
                      }
                      else
                      {
                        if (isIssuable(queue[0]))
                        {

                          //no need to search because if the front can't be sent,
                          // then no chance something behind it can go instead
                          *busPacket = queue[0];
                          queue.erase(queue.begin());
                          foundIssuable = true;
                        }
                      }
                    }

                    //if we found something, break out of do-while
                    if (foundIssuable) break;

                    //rank round robin
                    if (queuingStructure == PerRank)
                    {
                      nextRank = (nextRank + 1) % NUM_RANKS;
                      if (startingRank == nextRank)
                      {
                        break;
                      }
                    }
                    else
                    {
                      nextRankAndBank(nextRank, nextBank);
                      if (startingRank == nextRank && startingBank == nextBank)
                      {
                        break;
                      }
                    }
                  }
                  while (true);

                  //if we couldn't find anything to send, return false
                  if (!foundIssuable) return false;
                }
        }
        else if (rowBufferPolicy == OpenPage)
        {
          bool sendingREForPRE = false;
          if (refreshWaiting)
          {
            bool sendREF = true;
            //make sure all banks idle and timing met for a REF
            for (size_t b = 0; b < NUM_BANKS; b++)
            {
              //if a bank is active we can't send a REF yet
              if (bankStates[refreshRank][b].currentBankState == RowActive)
              {
                sendREF = false;
                bool closeRow = true;
                bool packetFound = false;
                //search for commands going to an open row
                vector <BusPacket *> &speedup_refreshQueue = getCommandQueue(refreshRank, b, TRUE);

                for (size_t j = 0; j < speedup_refreshQueue.size(); j++)
                {
                  BusPacket *packet = speedup_refreshQueue[j];
                  //if a command in the queue is going to the same row . . .
                  if (bankStates[refreshRank][b].openRowAddress == packet->row &&
                      b == packet->bank)
                  {
                    // . . . and is not an activate . . .
                    if (packet->busPacketType != ACTIVATE)
                    {
                      closeRow = false;
                      // . . . and can be issued . . .
                      if (isIssuable(packet))
                      {
                        //send it out
                        *busPacket = packet;
                        speedup_refreshQueue.erase(speedup_refreshQueue.begin() + j);
                        sendingREForPRE = true;
                      }
                      break;
                    }
                    else //command is an activate
                    {
                      //if we've encountered another act, no other command will be of interest
                      break;
                    }

                    packetFound = true;
                  }
                }

                if (packetFound == false)
                {
                  vector <BusPacket *> &refreshQueue = getCommandQueue(refreshRank, b, FALSE);

                  for (size_t j = 0; j < refreshQueue.size(); j++)
                  {
                    BusPacket *packet = refreshQueue[j];
                    //if a command in the queue is going to the same row . . .
                    if (bankStates[refreshRank][b].openRowAddress == packet->row &&
                        b == packet->bank)
                    {
                      // . . . and is not an activate . . .
                      if (packet->busPacketType != ACTIVATE)
                      {
                        closeRow = false;
                        // . . . and can be issued . . .
                        if (isIssuable(packet))
                        {
                          //send it out
                          *busPacket = packet;
                          refreshQueue.erase(refreshQueue.begin() + j);
                          sendingREForPRE = true;
                        }
                        break;
                      }
                      else //command is an activate
                      {
                        //if we've encountered another act, no other command will be of interest
                        break;
                      }
                    }
                  }
                }

                //if the bank is open and we are allowed to close it, then send a PRE
                if (closeRow && currentClockCycle >= bankStates[refreshRank][b].nextPrecharge)
                {
                  rowAccessCounters[refreshRank][b] = 0;
                  *busPacket = new BusPacket(PRECHARGE, 0, 0, speedup_stream_u, 0, 0, refreshRank, b, 0, dramsim_log);
                  sendingREForPRE = true;
                }
                break;
              }//	NOTE: the next ACT and next REF can be issued at the same
              //				point in the future, so just use nextActivate field instead of
              //				creating a nextRefresh field
              else if (bankStates[refreshRank][b].nextActivate > currentClockCycle) //and this bank doesn't have an open row
              {
                sendREF = false;
                break;
              }
            }

            //if there are no open banks and timing has been met, send out the refresh
            //	reset flags and rank pointer
            if (sendREF && bankStates[refreshRank][0].currentBankState != PowerDown)
            {
              *busPacket = new BusPacket(REFRESH, 0, 0, speedup_stream_u, 0, 0, refreshRank, 0, 0, dramsim_log);
              refreshRank = -1;
              refreshWaiting = false;
              sendingREForPRE = true;
            }
          }

          if (!sendingREForPRE)
          {
            unsigned startingRank = nextRank;
            unsigned startingBank = nextBank;
            bool foundIssuable = false;
            do // round robin over queues
            {
              vector<BusPacket *> &speedup_queue = getCommandQueue(nextRank, nextBank, TRUE);

              //make sure there is something there first
              if (!speedup_queue.empty() && !((nextRank == refreshRank) && refreshWaiting))
              {
                //search from the beginning to find first issuable bus packet
                for (size_t i = 0; i < speedup_queue.size(); i++)
                {
                  BusPacket *packet = speedup_queue[i];
                  if (isIssuable(packet))
                  {
                    //check for dependencies
                    bool dependencyFound = false;
                    for (size_t j = 0; j < i; j++)
                    {
                      BusPacket *prevPacket = speedup_queue[j];
                      if (prevPacket->busPacketType != ACTIVATE &&
                          prevPacket->bank == packet->bank &&
                          prevPacket->row == packet->row)
                      {
                        dependencyFound = true;
                        break;
                      }
                    }
                    if (dependencyFound) continue;

                    *busPacket = packet;

                    //if the bus packet before is an activate, that is the act that was
                    //	paired with the column access we are removing, so we have to remove
                    //	that activate as well (check i>0 because if i==0 then theres nothing before it)
                    if (i > 0 && speedup_queue[i - 1]->busPacketType == ACTIVATE)
                    {
                      (*busPacket)->rowHit = true;
                      rowAccessCounters[(*busPacket)->rank][(*busPacket)->bank]++;
                      // i is being returned, but i-1 is being thrown away, so must delete it here 
                      delete (speedup_queue[i - 1]);

                      // remove both i-1 (the activate) and i (we've saved the pointer in *busPacket)
                      speedup_queue.erase(speedup_queue.begin() + i - 1, speedup_queue.begin() + i + 1);
                    }
                    else // there's no activate before this packet
                    {
                      //or just remove the one bus packet
                      speedup_queue.erase(speedup_queue.begin() + i);
                    }

                    foundIssuable = true;
                    break;
                  }
                }
              }

              vector<BusPacket *> &queue = getCommandQueue(nextRank, nextBank, FALSE);

              if (foundIssuable == false)
              {
                //make sure there is something there first
                if (!queue.empty() && !((nextRank == refreshRank) && refreshWaiting))
                {
                  //search from the beginning to find first issuable bus packet
                  for (size_t i = 0; i < queue.size(); i++)
                  {
                    BusPacket *packet = queue[i];
                    if (isIssuable(packet))
                    {
                      //check for dependencies
                      bool dependencyFound = false;
                      for (size_t j = 0; j < i; j++)
                      {
                        BusPacket *prevPacket = queue[j];
                        if (prevPacket->busPacketType != ACTIVATE &&
                            prevPacket->bank == packet->bank &&
                            prevPacket->row == packet->row)
                        {
                          dependencyFound = true;
                          break;
                        }
                      }
                      if (dependencyFound) continue;

                      *busPacket = packet;

                      //if the bus packet before is an activate, that is the act that was
                      //	paired with the column access we are removing, so we have to remove
                      //	that activate as well (check i>0 because if i==0 then theres nothing before it)
                      if (i > 0 && queue[i - 1]->busPacketType == ACTIVATE)
                      {
                        (*busPacket)->rowHit = true;
                        rowAccessCounters[(*busPacket)->rank][(*busPacket)->bank]++;
                        // i is being returned, but i-1 is being thrown away, so must delete it here 
                        delete (queue[i - 1]);

                        // remove both i-1 (the activate) and i (we've saved the pointer in *busPacket)
                        queue.erase(queue.begin() + i - 1, queue.begin() + i + 1);
                      }
                      else // there's no activate before this packet
                      {
                        //or just remove the one bus packet
                        queue.erase(queue.begin() + i);
                      }

                      foundIssuable = true;
                      break;
                    }
                  }
                }
              }


              //if we found something, break out of do-while
              if (foundIssuable) break;

              //rank round robin
              if (queuingStructure == PerRank)
              {
                nextRank = (nextRank + 1) % NUM_RANKS;
                if (startingRank == nextRank)
                {
                  break;
                }
              }
              else
              {
                nextRankAndBank(nextRank, nextBank);
                if (startingRank == nextRank && startingBank == nextBank)
                {
                  break;
                }
              }
            }
            while (true);

            //if nothing was issuable, see if we can issue a PRE to an open bank
            //	that has no other commands waiting
            if (!foundIssuable)
            {
              //search for banks to close
              bool sendingPRE = false;
              unsigned startingRank = nextRankPRE;
              unsigned startingBank = nextBankPRE;

              do // round robin over all ranks and banks
              {
                vector <BusPacket *> &queue = getCommandQueue(nextRankPRE, nextBankPRE, FALSE);
                vector <BusPacket *> &speedup_queue = getCommandQueue(nextRankPRE, nextBankPRE, TRUE);
                bool found = false;
                //check if bank is open
                if (bankStates[nextRankPRE][nextBankPRE].currentBankState == RowActive)
                {
                  for (size_t i = 0; i < speedup_queue.size(); i++)
                  {
                    //if there is something going to that bank and row, then we don't want to send a PRE
                    if (speedup_queue[i]->bank == nextBankPRE &&
                        speedup_queue[i]->row == bankStates[nextRankPRE][nextBankPRE].openRowAddress)
                    {
                      found = true;
                      break;
                    }
                  }

                  if (!found)
                  {
                    for (size_t i = 0; i < queue.size(); i++)
                    {
                      //if there is something going to that bank and row, then we don't want to send a PRE
                      if (queue[i]->bank == nextBankPRE &&
                          queue[i]->row == bankStates[nextRankPRE][nextBankPRE].openRowAddress)
                      {
                        found = true;
                        break;
                      }
                    }
                  }

                  // if nothing found going to that bank and row and there are requests to the same
                  // bank but different row, close it
                  if (!found && (queue.size() || speedup_queue.size()))
                  {
                    if (currentClockCycle >= bankStates[nextRankPRE][nextBankPRE].nextPrecharge)
                    {
                      sendingPRE = true;
                      rowAccessCounters[nextRankPRE][nextBankPRE] = 0;
                      *busPacket = new BusPacket(PRECHARGE, 0, 0, speedup_stream_u, 0, 0, nextRankPRE, nextBankPRE, 0, dramsim_log);
                      break;
                    }
                  }
                }
                nextRankAndBank(nextRankPRE, nextBankPRE);
              }
              while (!(startingRank == nextRankPRE && startingBank == nextBankPRE));

              //if no PREs could be sent, just return false
              if (!sendingPRE) return false;
            }
          }
        }

        //sendAct is flag used for posted-cas
        //  posted-cas is enabled when AL>0
        //  when sendAct is true, when don't want to increment our indexes
        //  so we send the column access that is paid with this act
        if (AL > 0 && sendAct)
        {
          sendAct = false;
        }
        else
        {
          sendAct = true;
          nextRankAndBank(nextRank, nextBank);
        }

        //if its an activate, add a tfaw counter
        if (*busPacket && (*busPacket)->busPacketType == ACTIVATE)
        {
                tFAWCountdown[(*busPacket)->rank].push_back(tFAW);
        }

        if (*busPacket)
        {
          return true;
        }
        else
        {
          return false;
        }
}

//check if a rank/bank queue has room for a certain number of bus packets


bool CommandQueue::hasRoomFor(unsigned numberToEnqueue, unsigned rank, unsigned bank, bool speedup, unsigned stream) {
  if (schedulingPolicy == RankThenBankSMS || schedulingPolicy == BankThenRankSMS)
  {
        return has_room_for_sms(&scheduler_sms, stream, numberToEnqueue);
  }
  else
  {
        if (speedup)
        {
              vector<BusPacket *> &queue = getCommandQueue(rank, bank, TRUE);
              return (CMD_QUEUE_DEPTH - queue.size() >= numberToEnqueue);
        }
        else
        {
              vector<BusPacket *> &queue = getCommandQueue(rank, bank, FALSE);
              return (CMD_QUEUE_DEPTH - queue.size() >= numberToEnqueue);
        }
  }
}

//prints the contents of the command queue


void CommandQueue::print() {
        if (queuingStructure == PerRank)
        {
                PRINT(endl << "== Printing Per Rank Queue");
                for (size_t i = 0; i < NUM_RANKS; i++)
                {
                        PRINT(" = Rank " << i << "  size : " << queues[i][0].size());
                        for (size_t j = 0; j < queues[i][0].size(); j++)
                        {
                                PRINTN("    " << j << "]");
                                queues[i][0][j]->print();
                        }
                }
        }
        else if (queuingStructure == PerRankPerBank)
        {
                PRINT("\n== Printing Per Rank, Per Bank Queue");

                for (size_t i = 0; i < NUM_RANKS; i++)
                {
                        PRINT(" = Rank " << i);
                        for (size_t j = 0; j < NUM_BANKS; j++)
                        {
                                PRINT("    Bank " << j << "   size : " << queues[i][j].size());

                                for (size_t k = 0; k < queues[i][j].size(); k++)
                                {
                                        PRINTN("       " << k << "]");
                                        queues[i][j][k]->print();
                                }
                        }
                }
        }
}


/** 
 * return a reference to the queue for a given rank, bank. Since we
 * don't always have a per bank queuing structure, sometimes the bank
 * argument is ignored (and the 0th index is returned 
 */
vector<BusPacket *> &CommandQueue::getCommandQueue(unsigned rank, unsigned bank, bool speedup) {

  if ((schedulingPolicy == RankThenBankSMS || schedulingPolicy == BankThenRankSMS) && !speedup)
  {
    sms *sched = &scheduler_sms;

    if (queuingStructure == PerRankPerBank)
    {
      return sched->batch_queue.queue[rank][bank];
    }
    else if (queuingStructure == PerRank)
    {
      return sched->batch_queue.queue[rank][0];
    }
    else
    {
      ERROR("Unknown queue structure");
      abort();
    }
  }
  else
  {
    if (speedup)
    {
      if (queuingStructure == PerRankPerBank)
      {
        return speedup_queues[rank][bank];
      }
      else if (queuingStructure == PerRank)
      {
        return speedup_queues[rank][0];
      }
      else
      {
        ERROR("Unknown queue structure");
        abort();
      }
    }
    else
    {
      if (queuingStructure == PerRankPerBank)
      {
        return queues[rank][bank];
      }
      else if (queuingStructure == PerRank)
      {
        return queues[rank][0];
      }
      else
      {
        ERROR("Unknown queue structure");
        abort();
      }
    }
  }
}

//checks if busPacket is allowed to be issued


bool CommandQueue::isIssuable(BusPacket *busPacket) {
        switch (busPacket->busPacketType)
        {
                case REFRESH:

                        break;
                case ACTIVATE:
                        if ((bankStates[busPacket->rank][busPacket->bank].currentBankState == Idle ||
                             bankStates[busPacket->rank][busPacket->bank].currentBankState == Refreshing) &&
                            currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextActivate &&
                            tFAWCountdown[busPacket->rank].size() < 4)
                        {
                                return true;
                        }
                        else
                        {
                                return false;
                        }
                        break;
                case WRITE:
                case WRITE_P:
                        if (bankStates[busPacket->rank][busPacket->bank].currentBankState == RowActive &&
                            currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextWrite &&
                            busPacket->row == bankStates[busPacket->rank][busPacket->bank].openRowAddress)
                        {
                                return true;
                        }
                        else
                        {
                                return false;
                        }
                        break;
                case READ_P:
                case READ:
                        if (bankStates[busPacket->rank][busPacket->bank].currentBankState == RowActive &&
                            currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextRead &&
                            busPacket->row == bankStates[busPacket->rank][busPacket->bank].openRowAddress)
                        {
                                return true;
                        }
                        else
                        {
                                return false;
                        }
                        break;
                case PRECHARGE:
                        if (bankStates[busPacket->rank][busPacket->bank].currentBankState == RowActive &&
                            currentClockCycle >= bankStates[busPacket->rank][busPacket->bank].nextPrecharge)
                        {
                                return true;
                        }
                        else
                        {
                                return false;
                        }
                        break;
                default:
                        ERROR("== Error - Trying to issue a crazy bus packet type : ");
                        busPacket->print();
                        exit(0);
        }
        return false;
}

//figures out if a rank's queue is empty


bool CommandQueue::isEmpty(unsigned rank) {
        if (queuingStructure == PerRank)
        {
                return queues[rank][0].empty();
        }
        else if (queuingStructure == PerRankPerBank)
        {
                for (size_t i = 0; i < NUM_BANKS; i++)
                {
                        if (!queues[rank][i].empty()) return false;
                }
                return true;
        }
        else
        {
                DEBUG("Invalid Queueing Stucture");
                abort();
        }
}

//tells the command queue that a particular rank is in need of a refresh


void CommandQueue::needRefresh(unsigned rank) {
        refreshWaiting = true;
        refreshRank = rank;
}


void CommandQueue::nextRankAndBank(unsigned &rank, unsigned &bank) {
        if (schedulingPolicy == RankThenBankRoundRobin || schedulingPolicy == RankThenBankSMS)
        {
                rank++;
                if (rank == NUM_RANKS)
                {
                        rank = 0;
                        bank++;
                        if (bank == NUM_BANKS)
                        {
                                bank = 0;
                        }
                }
        }//bank-then-rank round robin
        else if (schedulingPolicy == BankThenRankRoundRobin || schedulingPolicy == BankThenRankSMS)
        {
                bank++;
                if (bank == NUM_BANKS)
                {
                        bank = 0;
                        rank++;
                        if (rank == NUM_RANKS)
                        {
                                rank = 0;
                        }
                }
        }
        else
        {
                ERROR("== Error - Unknown scheduling policy");
                exit(0);
        }

}


void CommandQueue::update() {
        //do nothing since pop() is effectively update(),
        //needed for SimulatorObject
        //TODO: make CommandQueue not a SimulatorObject
}

/* Allocate and initialize per-bank queues */
void CommandQueue::init_sms(size_t bank_queue, size_t queue_size)
{
  sms *sched;
  BusPacket1D actualQueue;
  BusPacket2D perBankQueue;
  
  sched = &scheduler_sms;

#define INVALID_ROW (-1)    

  for (int u = NN; u <= TST; u++)
  {
    sched->source_queue[u].queue          = BusPacket1D();
    sched->source_queue[u].queue_size     = queue_size;
    sched->source_queue[u].queue_entries  = 0;
    sched->source_queue[u].current_row_id = INVALID_ROW;
    sched->source_queue[u].ready          = FALSE;
    sched->source_queue[u].age            = 0;
  }

  sched->batch_queue.mode       = batching_mode_pick;
  sched->batch_queue.src        = NN;
  sched->batch_queue.psjf       = .9;
  sched->batch_queue.rr_next    = NN;

  for (int u = NN; u <= TST; u++)
  {
    sched->batch_queue.src_req[u] = 0;
  }

  /* Allocate common batch queue for all sources */
  sched->batch_queue.queue = BusPacket3D();

  perBankQueue = BusPacket2D();

  for (size_t rank = 0; rank < NUM_RANKS; rank++)
  {
    //this loop will run only once for per-rank and NUM_BANKS times for per-rank-per-bank
    for (size_t bank = 0; bank < bank_queue; bank++)
    {
      actualQueue = BusPacket1D();
      perBankQueue.push_back(actualQueue);
    }

    sched->batch_queue.queue.push_back(perBankQueue);
  }
}

bool CommandQueue::has_room_for_sms(sms *sched, int stream_in, unsigned numberToEnqueue)
{
  int stream;

  assert(sched);
  assert(sched->source_queue);
  
  stream = SMS_STREAM(stream_in);

  assert(stream > NN && stream <= TST);

  if (sched->source_queue[stream].queue_entries < sched->source_queue[stream].queue_size)
  {
    return true;
  }

  return false;
}

/* First stage of SMS */
void CommandQueue::enqueue_sms(int stream_in, BusPacket *newBusPacket)
{
  int stream;
  sms *sched;

  unsigned rank = newBusPacket->rank;
  unsigned bank = newBusPacket->bank;

  assert(newBusPacket);
  assert(stream_in > NN && stream_in <= TST);
  
  sched   = &scheduler_sms;
  stream  = SMS_STREAM(stream_in);

  assert(sched->source_queue[stream].queue_entries < sched->source_queue[stream].queue_size);
  
  newBusPacket->age = sched->cycle;

  /* Enqueue new request into the source queue */
  sched->source_queue[stream].queue.push_back(newBusPacket);
  sched->source_queue[stream].queue_entries += 1;
  
  /* Update batch state based on the row-id of current request */
#define BATCH_READY(s, st)    ((s)->source_queue[st].ready == TRUE)
#define QUEUE_SIZE(s, st)     ((s)->source_queue[st].queue_size)
#define QUEUE_ENTRY(s, st)    ((s)->source_queue[st].queue_entries)
#define QUEUE_FULL(s, st)     (QUEUE_SIZE(s, st) == QUEUE_ENTRY(s, st))
#define CURRENT_ROW(s, st)    ((s)->source_queue[st].current_row_id)
#define BATCH_BEGIN(s, st)    (CURRENT_ROW(s, st) != INVALID_ROW)
#define ROW_CHANGE(s, st, p)  ((s)->source_queue[st].current_row_id != (p)->row)

  if (BATCH_BEGIN(sched, stream) && !BATCH_READY(sched, stream))
  {
    if (ROW_CHANGE(sched, stream, newBusPacket) || QUEUE_FULL(sched, stream))
    {
      sched->source_queue[stream].ready = TRUE;
      sched->source_queue[stream].age   = sched->cycle;
    }
  }
  else
  {
    if (!BATCH_BEGIN(sched, stream))
    {
      CURRENT_ROW(sched, stream) = newBusPacket->row;
    }
  }

#undef BATCH_READY
#undef QUEUE_SIZE
#undef QUEUE_ENTRY
#undef QUEUE_FULL
#undef CURRENT_ROW
#undef BATCH_BEGIN
#undef ROW_CHANGE

  if (sched->source_queue[stream].queue.size() > CMD_QUEUE_DEPTH)
  {
    ERROR("== Error - Enqueued more than allowed in command queue");
    ERROR("						Need to call .hasRoomFor(int numberToEnqueue, unsigned rank, unsigned bank) first");
    exit(0);
  }

#if 0
  sched->batch_queue.queue[rank][bank].push_back(newBusPacket);
#endif
}

void CommandQueue::execute_batching_sms()
{
  /* Find out sources with ready batches 
   * Select one batch based on the policy and enqueue request to batch queue 
   * */ 
  sms       *sched;
  int        min_batch_size;
  long int   min_age;
  int        tgt_queue;
  int        qcount;
  BusPacket *newBusPacket;  
  int        rank;
  int        bank;
 
  sched           = &scheduler_sms; 
  min_batch_size  = 0xffffff;
  min_age         = 0xffffffffffff;
  tgt_queue       = NN;

  /* Increment scheduler cycle */
  sched->cycle += 1;

#define BATCH_READY(s, st)  ((s)->source_queue[st].ready == TRUE)
#define SOURCE_HEAD(s, st)  ((s)->source_queue[st].queue[0])
#define SOURCE_PNDNG(s, st) ((s)->source_queue[st].queue_entries)
#define SOURCE_SIZE(s, st)  ((s)->source_queue[st].queue_entries + (s)->batch_queue.src_req[st])
#define BATCH_AGE(s, st)    ((s)->source_queue[st].age)

  if (sched->batch_queue.mode == batching_mode_pick)
  {
    for (int stream = NN; stream <= TST; stream++)
    {
      if (SOURCE_PNDNG(sched, stream))
      {
        newBusPacket = SOURCE_HEAD(sched, stream);

        if (sched->cycle - newBusPacket->age > 200)
        {
          sched->source_queue[stream].ready = TRUE;    
        }
      }
    }

    /* Chooses between SJF and Round Robin */
    if (get_prob_in_range(0.0, 0.9))
    {
      for (int stream = NN; stream <= TST; stream++)
      {
        if (BATCH_READY(sched, stream)) 
        {
#define CHK_TIE(sc, st, m, ma) (SOURCE_SIZE(sc, st) == m && BATCH_AGE(sc, st) < ma)

          /* Choose next ready batch */
          if (SOURCE_PNDNG(sched, stream))
          {
            if (SOURCE_SIZE(sched, stream) <= min_batch_size || CHK_TIE(sched, stream, min_batch_size, min_age))
            {
              tgt_queue       = stream;
              min_age         = BATCH_AGE(sched, stream); 
              min_batch_size  = SOURCE_SIZE(sched, stream);
            }
          }

#undef CHK_TIE
        }
      }
    }
    else
    {
      qcount    = 0;
      tgt_queue = (sched->batch_queue.rr_next + 1) % (TST + 1);
      
      /* Find next ready batch from the source in round-robin order */
      while (!BATCH_READY(sched, tgt_queue) && qcount++ <= TST)
      {
        tgt_queue = (sched->batch_queue.rr_next + 1) % (TST + 1);
      }

      if (!BATCH_READY(sched, tgt_queue))
      {
        tgt_queue = NN;
      }
    }

    sched->batch_queue.src = tgt_queue;

    if (tgt_queue != NN)
    {
      sched->batch_queue.mode = batching_mode_drain;
    }
  }
  else
  {
    assert(sched->batch_queue.src > NN && sched->batch_queue.src <= TST);

    /* Dequeue next request from the target queue and enqueue into the 
     * batch queue */ 
    assert(sched->source_queue[sched->batch_queue.src].queue.size());
    assert(sched->source_queue[sched->batch_queue.src].ready == TRUE);

    newBusPacket = sched->source_queue[sched->batch_queue.src].queue[0];

    assert(newBusPacket);
    assert(newBusPacket->row == sched->source_queue[sched->batch_queue.src].current_row_id);

    rank = newBusPacket->rank;
    bank = newBusPacket->bank;

    /* Move request from source queue to per-bank batch queue */
    sched->source_queue[sched->batch_queue.src].queue.erase(sched->source_queue[sched->batch_queue.src].queue.begin());
    assert(sched->source_queue[sched->batch_queue.src].queue_entries > 0);

    sched->source_queue[sched->batch_queue.src].queue_entries -= 1;

    sched->batch_queue.queue[rank][bank].push_back(newBusPacket);
    sched->batch_queue.src_req[sched->batch_queue.src] += 1;

    if (newBusPacket->busPacketType == ACTIVATE &&
      sched->source_queue[sched->batch_queue.src].queue[0]->physicalAddress == newBusPacket->physicalAddress)
    {
      newBusPacket = sched->source_queue[sched->batch_queue.src].queue[0];

      assert(newBusPacket);
      assert(newBusPacket->row == sched->source_queue[sched->batch_queue.src].current_row_id);

      rank = newBusPacket->rank;
      bank = newBusPacket->bank;

      /* Move request from source queue to per-bank batch queue */
      sched->source_queue[sched->batch_queue.src].queue.erase(sched->source_queue[sched->batch_queue.src].queue.begin());
      assert(sched->source_queue[sched->batch_queue.src].queue_entries > 0);

      sched->source_queue[sched->batch_queue.src].queue_entries -= 1;

      sched->batch_queue.queue[rank][bank].push_back(newBusPacket);
    }

    if (sched->source_queue[sched->batch_queue.src].queue_entries == 0)
    {
      sched->source_queue[sched->batch_queue.src].ready           = FALSE;
      sched->source_queue[sched->batch_queue.src].current_row_id  = INVALID_ROW;
      sched->source_queue[sched->batch_queue.src].age             = 0;
      sched->batch_queue.src                                      = NN;
      sched->batch_queue.mode                                     = batching_mode_pick;
    }
    else
    {
      /* If there are requests in the queue, find state of the batch */
      newBusPacket = sched->source_queue[sched->batch_queue.src].queue[0];
      
      if (newBusPacket->row != sched->source_queue[sched->batch_queue.src].current_row_id)
      {
        sched->source_queue[sched->batch_queue.src].current_row_id = newBusPacket->row;

        /* Iterate through all entries and if a request with different row 
         * is found, make batch ready  */
        for (size_t j = 0; j < sched->source_queue[sched->batch_queue.src].queue.size(); j++)
        {
          newBusPacket = sched->source_queue[sched->batch_queue.src].queue[j];

          if (newBusPacket->row != sched->source_queue[sched->batch_queue.src].current_row_id)
          {
            sched->source_queue[sched->batch_queue.src].ready = TRUE;
            sched->source_queue[sched->batch_queue.src].age = sched->cycle;
            break;
          }
        }
      }
    }
  }

#undef BATCH_READY
#undef SOURCE_PNDNG
#undef SOURCE_SIZE
#undef BATCH_AGE
}

vector<BusPacket *> &CommandQueue::get_command_queue_sms(sms *sched, unsigned rank, unsigned bank) 
{
  if (queuingStructure == PerRankPerBank)
  {
    return sched->batch_queue.queue[rank][bank];
  }
  else if (queuingStructure == PerRank)
  {
    return sched->batch_queue.queue[rank][0];
  }
  else
  {
    ERROR("Unknown queue structure");
    abort();
  }
}

/* This function is used to update total number of requests from a source once a request is 
 * dequeued from the command queue */
void CommandQueue::remove_request_sms(BusPacket *newBusPacket)
{
  sms *sched;
  int  stream;

  sched = &scheduler_sms;
  stream = SMS_STREAM(newBusPacket->stream);

#if 0
  assert(newBusPacket->stream != NN);    

  assert(sched->batch_queue.src_req[newBusPacket->stream]);
#endif
  
  if (sched->batch_queue.src_req[stream])
  {
    sched->batch_queue.src_req[stream] -= 1;
  }
}

#undef INVALID_ROW
