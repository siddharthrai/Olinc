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
#include <errno.h> 
#include <sstream> //stringstream
#include <stdlib.h> // getenv()
// for directory operations 
#include <sys/stat.h>
#include <sys/types.h>

#include "MultiChannelMemorySystem.h"
#include "AddressMapping.h"
#include "IniReader.h"



using namespace DRAMSim;


MultiChannelMemorySystem::MultiChannelMemorySystem(const string &deviceIniFilename_, const string &systemIniFilename_, const string &pwd_, const string &traceFilename_, unsigned megsOfMemory_, string *visFilename_, const IniReader::OverrideMap *paramOverrides)
: megsOfMemory(megsOfMemory_), deviceIniFilename(deviceIniFilename_),
systemIniFilename(systemIniFilename_), traceFilename(traceFilename_),
pwd(pwd_), visFilename(visFilename_),
csvOut(new CSVWriter(visDataOut)) {
        currentClockCycle = 0;
        if (visFilename)
                printf("CC VISFILENAME=%s\n", visFilename->c_str());

        if (!isPowerOfTwo(megsOfMemory))
        {
                ERROR("Please specify a power of 2 memory size");
                abort();
        }

        if (pwd.length() > 0)
        {
                //ignore the pwd argument if the argument is an absolute path
                if (deviceIniFilename[0] != '/')
                {
                        deviceIniFilename = pwd + "/" + deviceIniFilename;
                }

                if (systemIniFilename[0] != '/')
                {
                        systemIniFilename = pwd + "/" + systemIniFilename;
                }
        }

        /* The device and system configuration files are not provided by Multi2sim
         * under some circumstances. Only when the device and system configurations
         * are provided use them to load the arguments. */
        if (deviceIniFilename.length())
        {
                DEBUG("== Loading device model file '" << deviceIniFilename << "' == ");
                IniReader::ReadIniFile(deviceIniFilename, false);
        }
        if (systemIniFilename.length())
        {
                DEBUG("== Loading system model file '" << systemIniFilename << "' == ");
                IniReader::ReadIniFile(systemIniFilename, true);
        }

        // If we have any overrides, set them now before creating all of the memory objects
        if (paramOverrides)
                IniReader::OverrideKeys(paramOverrides);

        IniReader::InitEnumsFromStrings();
        if (!IniReader::CheckIfAllSet())
        {
                exit(-1);
        }

        if (NUM_CHANS == 0)
        {
                ERROR("Zero channels");
                abort();
        }
        for (size_t i = 0; i < NUM_CHANS; i++)
        {
                MemorySystem *channel = new MemorySystem(i, megsOfMemory / NUM_CHANS, (*csvOut), dramsim_log);
                channels.push_back(channel);
        }

        speedup_hint_enable = SPEEDUP_HINT;

        printf("Speedup enable %d\n", speedup_hint_enable);
}


bool fileExists(string &path) {
        struct stat stat_buf;
        if (stat(path.c_str(), &stat_buf) != 0)
        {
                if (errno == ENOENT)
                {
                        return false;
                }
                ERROR("Warning: some other kind of error happened with stat(), should probably check that");
        }
        return true;
}


string FilenameWithNumberSuffix(const string &filename, const string &extension, unsigned maxNumber = 100) {
        string currentFilename = filename + extension;
        if (!fileExists(currentFilename))
        {
                return currentFilename;
        }

        // otherwise, add the suffixes and test them out until we find one that works
        stringstream tmpNum;
        tmpNum << "." << 1;
        for (unsigned i = 1; i < maxNumber; i++)
        {
                currentFilename = filename + tmpNum.str() + extension;
                if (fileExists(currentFilename))
                {
                        currentFilename = filename;
                        tmpNum.seekp(0);
                        tmpNum << "." << i;
                }
                else
                {
                        return currentFilename;
                }
        }
        // if we can't find one, just give up and return whatever is the current filename
        ERROR("Warning: Couldn't find a suitable suffix for " << filename);
        return currentFilename;
}


/**
 * This function creates up to 3 output files: 
 * 	- The .log file if LOG_OUTPUT is set
 * 	- the .vis file where csv data for each epoch will go
 * 	- the .tmp file if verification output is enabled
 * The results directory is setup to be in PWD/TRACEFILENAME.[SIM_DESC]/DRAM_PARTNAME/PARAMS.vis
 * The environment variable SIM_DESC is also appended to output files/directories
 *
 * TODO: verification info needs to be generated per channel so it has to be
 * moved back to MemorySystem
 **/
void MultiChannelMemorySystem::InitOutputFiles(string traceFilename) {
        size_t lastSlash;
        size_t deviceIniFilenameLength = deviceIniFilename.length();
        string sim_description_str;
        string deviceName;

        char *sim_description = getenv("SIM_DESC");
        if (sim_description)
        {
                sim_description_str = string(sim_description);
        }


        // create a properly named verification output file if need be and open it
        // as the stream 'cmd_verify_out'
        if (VERIFICATION_OUTPUT)
        {
                string basefilename = deviceIniFilename.substr(deviceIniFilename.find_last_of("/") + 1);
                string verify_filename = "sim_out_" + basefilename;
                if (sim_description != NULL)
                {
                        verify_filename += "." + sim_description_str;
                }
                verify_filename += ".tmp";
                cmd_verify_out.open(verify_filename.c_str());
                if (!cmd_verify_out)
                {
                        ERROR("Cannot open " << verify_filename);
                        abort();
                }
        }
        // This sets up the vis file output along with the creating the result
        // directory structure if it doesn't exist
        if (VIS_FILE_OUTPUT)
        {
                stringstream out, tmpNum;
                string path;
                string filename;

                if (!visFilename)
                {
                        path = "results/";
                        // chop off the .ini if it's there
                        if (deviceIniFilename.substr(deviceIniFilenameLength - 4) == ".ini")
                        {
                                deviceName = deviceIniFilename.substr(0, deviceIniFilenameLength - 4);
                                deviceIniFilenameLength -= 4;
                        }

                        // chop off everything past the last / (i.e. leave filename only)
                        if ((lastSlash = deviceName.find_last_of("/")) != string::npos)
                        {
                                deviceName = deviceName.substr(lastSlash + 1, deviceIniFilenameLength - lastSlash - 1);
                        }

                        string rest;
                        // working backwards, chop off the next piece of the directory
                        if ((lastSlash = traceFilename.find_last_of("/")) != string::npos)
                        {
                                traceFilename = traceFilename.substr(lastSlash + 1, traceFilename.length() - lastSlash - 1);
                        }
                        if (sim_description != NULL)
                        {
                                traceFilename += "." + sim_description_str;
                        }

                        if (pwd.length() > 0)
                        {
                                path = pwd + "/" + path;
                        }

                        // create the directories if they don't exist 
                        mkdirIfNotExist(path);
                        path = path + traceFilename + "/";
                        mkdirIfNotExist(path);
                        path = path + deviceName + "/";
                        mkdirIfNotExist(path);

                        // finally, figure out the filename
                        string sched = "BtR";
                        string queue = "pRank";
                        if (schedulingPolicy == RankThenBankRoundRobin)
                        {
                                sched = "RtB";
                        }
                        if (queuingStructure == PerRankPerBank)
                        {
                                queue = "pRankpBank";
                        }

                        /* I really don't see how "the C++ way" is better than snprintf()  */
                        out << (TOTAL_STORAGE >> 10) << "GB." << NUM_CHANS << "Ch." << NUM_RANKS << "R." << ADDRESS_MAPPING_SCHEME << "." << ROW_BUFFER_POLICY << "." << TRANS_QUEUE_DEPTH << "TQ." << CMD_QUEUE_DEPTH << "CQ." << sched << "." << queue;
                }
                else //visFilename given
                {
                        out << *visFilename;
                }
                if (sim_description)
                {
                        out << "." << sim_description;
                }

                //filename so far, without extension, see if it exists already
                filename = out.str();


                filename = FilenameWithNumberSuffix(filename, ".vis");
                path.append(filename);
                cerr << "writing vis file to " << path << endl;


                visDataOut.open(path.c_str());
                if (!visDataOut)
                {
                        ERROR("Cannot open '" << path << "'");
                        exit(-1);
                }
                //write out the ini config values for the visualizer tool
                IniReader::WriteValuesOut(visDataOut);

        }
        else
        {
                // cerr << "vis file output disabled\n";
        }
#ifdef LOG_OUTPUT
        string dramsimLogFilename("dramsim");
        if (sim_description != NULL)
        {
                dramsimLogFilename += "." + sim_description_str;
        }

        dramsimLogFilename = FilenameWithNumberSuffix(dramsimLogFilename, ".log");

        dramsim_log.open(dramsimLogFilename.c_str(), ios_base::out | ios_base::trunc);

        if (!dramsim_log)
        {
                ERROR("Cannot open " << dramsimLogFilename);
                //	exit(-1); 
        }
#endif

}


void MultiChannelMemorySystem::mkdirIfNotExist(string path) {
        struct stat stat_buf;
        // check if the directory exists
        if (stat(path.c_str(), &stat_buf) != 0) // nonzero return value on error, check errno
        {
                if (errno == ENOENT)
                {
                        //			DEBUG("\t directory doesn't exist, trying to create ...");

                        // set permissions dwxr-xr-x on the results directories
                        mode_t mode = (S_IXOTH | S_IXGRP | S_IXUSR | S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR);
                        if (mkdir(path.c_str(), mode) != 0)
                        {
                                perror("Error Has occurred while trying to make directory: ");
                                cerr << path << endl;
                                abort();
                        }
                }
                else
                {
                        perror("Something else when wrong: ");
                        abort();
                }
        }
        else // directory already exists
        {
                if (!S_ISDIR(stat_buf.st_mode))
                {
                        ERROR(path << "is not a directory");
                        abort();
                }
        }
}


MultiChannelMemorySystem::~MultiChannelMemorySystem() {
        for (size_t i = 0; i < NUM_CHANS; i++)
        {
                delete channels[i];
        }
        channels.clear();

        // flush our streams and close them up
#ifdef LOG_OUTPUT
        dramsim_log.flush();
        dramsim_log.close();
#endif
        if (VIS_FILE_OUTPUT)
        {
                visDataOut.flush();
                visDataOut.close();
        }

        delete csvOut;
}


void MultiChannelMemorySystem::update() {
        if (currentClockCycle == 0)
        {
                InitOutputFiles(traceFilename);
        }

        if (currentClockCycle % EPOCH_LENGTH == 0)
        {
                (*csvOut) << "ms" << currentClockCycle * tCK * 1E-6;
                for (size_t i = 0; i < NUM_CHANS; i++)
                {
                        channels[i]->printStats(false);
                }
                csvOut->finalize();
        }

        for (size_t i = 0; i < NUM_CHANS; i++)
        {
                channels[i]->update();
        }


        currentClockCycle++;
}


unsigned MultiChannelMemorySystem::findChannelNumber(uint64_t addr, char stream) {
        // Single channel case is a trivial shortcut case 
        if (NUM_CHANS == 1)
        {
                return 0;
        }

        if (!isPowerOfTwo(NUM_CHANS))
        {
                ERROR("We can only support power of two # of channels.\n" <<
                      "I don't know what Intel was thinking, but trying to address map half a bit is a neat trick that we're not sure how to do");
                abort();
        }

        // only chan is used from this set 
        unsigned channelNumber, rank, bank, row, col;
        addressMapping(addr, channelNumber, rank, bank, row, col, stream);
        if (channelNumber >= NUM_CHANS)
        {
                ERROR("Got channel index " << channelNumber << " but only " << NUM_CHANS << " exist");
                abort();
        }
        //DEBUG("Channel idx = "<<channelNumber<<" totalbits="<<totalBits<<" channelbits="<<channelBits); 

        return channelNumber;

}


ostream &MultiChannelMemorySystem::getLogFile() {
        return dramsim_log;
}


bool MultiChannelMemorySystem::addTransaction(Transaction *trans) {
        unsigned channelNumber = findChannelNumber(trans->address, trans->stream);
        return channels[channelNumber]->addTransaction(trans);
}


bool MultiChannelMemorySystem::addTransaction(bool isWrite, uint64_t addr, char stream, speedup_stream_type stream_type) {
        unsigned channelNumber = findChannelNumber(addr, stream);
        return channels[channelNumber]->addTransaction(isWrite, addr, stream, stream_type);
}

void MultiChannelMemorySystem::setPriorityStream(ub1 stream)
{
        for (size_t i = 0; i < NUM_CHANS; i++)
        {
          channels[i]->setPriorityStream(stream);
        }
}

/*
        This function has two flavors: one with and without the address. 
        If the simulator won't give us an address and we have multiple channels, 
        we have to assume the worst and return false if any channel won't accept. 

        However, if the address is given, we can just map the channel and check just
        that memory controller
 */
bool MultiChannelMemorySystem::willAcceptTransaction(uint64_t addr, bool isWrite, char stream) {
        unsigned chan, rank, bank, row, col;
        addressMapping(addr, chan, rank, bank, row, col, stream);
        return channels[chan]->WillAcceptTransaction(isWrite, stream);
}


bool MultiChannelMemorySystem::willAcceptTransaction(unsigned chan, bool isWrite, char stream) {
        return channels[chan]->WillAcceptTransaction(isWrite, stream);
}


void MultiChannelMemorySystem::printStats(bool finalStats, char *file_name) {


        (*csvOut) << "ms" << currentClockCycle * tCK * 1E-6;
        for (size_t i = 0; i < NUM_CHANS; i++)
        {
                /* Print short DRAM stats */
                channels[i]->printShortStats(file_name);
                channels[i]->printShortOverallStats(file_name);
                channels[i]->printShortOverallStatsPerStream(file_name);
#if 0
                PRINT("==== Channel [" << i << "] ====");
                channels[i]->printStats(finalStats);
                PRINT("//// Channel [" << i << "] ////");
#endif
        }
        
        csvOut->finalize();
}


void MultiChannelMemorySystem::RegisterCallbacks(
                                                 TransactionCompleteCB *readDone,
                                                 TransactionCompleteCB *writeDone,
                                                 void (*reportPower)(double bgpower, double burstpower, double refreshpower, double actprepower)) {
        for (size_t i = 0; i < NUM_CHANS; i++)
        {
                channels[i]->RegisterCallbacks(readDone, writeDone, reportPower);
        }
}

ub8 MultiChannelMemorySystem::getOpenRow(ub8 address)
{
        unsigned chan, rank, bank, row, col;
        addressMapping(address, chan, rank, bank, row, col, NN);

        return channels[chan]->getOpenRow(rank, bank);
}

bool MultiChannelMemorySystem::isSpeedupHintEnable()
{
  return speedup_hint_enable;
}
