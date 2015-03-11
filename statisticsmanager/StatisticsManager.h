/**************************************************************************
 *
 * Copyright (c) 2002 - 2011 by Computer Architecture Department,
 * Universitat Politecnica de Catalunya.
 * All rights reserved.
 *
 * The contents of this file may not be disclosed to third parties,
 * copied or duplicated in any form, in whole or in part, without the
 * prior permission of the authors, Computer Architecture Department
 * and Universitat Politecnica de Catalunya.
 *
 */

#ifndef STATISTICSMANAGER_H
    #define STATISTICSMANAGER_H

#include "Statistic.h"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cstdio>

#define TCM     (6)
#define COMP_SH (0)
#define COMP_TX (1)
#define COMP_CW (2)
#define COMP_ZS (3)
#define COMP_CH (4)
#define COMP_CP (5)


static const ub4 FREQ_CYCLES = 0;
static const ub4 FREQ_FRAME = 1;
static const ub4 FREQ_BATCH = 2;
static const char  *cm_name[TCM] = {"SH", "TX", "CW", "ZS", "CH", "CP"};

class StatisticsManager
{
private:

    bool cyclesFlagNamesDumped[TCM];

    /* list of current stats */
    std::map<std::string,Statistic*> stats;

    /* Helper method to find a Statistic with name 'name' */
    Statistic* find(std::string name);

    /* dump scheduling */
    ub8 startCycle;
    ub8 skipCycle;
    ub8 nCycles;
    ub8 nextDump;
    ub8 nextReset;
    ub8 lastCycle;
    bool autoReset;

    /* current output per cycle stream */
    std::ostream* osCycle[TCM];

    /*  output streams for per batch and per frame statistics.  */
    std::ostream* osBatch[TCM];

    std::ostream* osFrame[TCM];

    /* singleton instance */
    // static StatisticsManager* sm;

    /* avoid copying */
    StatisticsManager();
    StatisticsManager(const StatisticsManager&);
    StatisticsManager& operator=(const StatisticsManager&);

public:

    static StatisticsManager& instance();


    template<typename T>
    NumericStatistic<T>& getNumericStatistic(const char* name, T initialValue, const char* owner = 0, const char* postfix=0)
    {
        char temp[256];
        if ( postfix != 0 )
        {
            sprintf(temp, "%s_%s", name, postfix);
            name = temp;
        }
        Statistic* st = find(name);
        NumericStatistic<T>* nst;
        if ( st )
        {
            /*
             * Warning (in windows):
             * Use this flag (-GR) in VS6 to achive RTTI in dynamic_cast
             *
             *  -GR    enable standard C++ RTTI (Run Time Type
             *         Identification)
             */
            nst = dynamic_cast<NumericStatistic<T>*>(st);

            if ( nst == 0 )
            {
                char temp[128];
                sprintf(temp, "Another Statistic exists with name '%s' but has different type", name);
                //panic("StatisticsManager","getNumericStatistic()", temp);
            }
            return *nst;
        }

        nst = new NumericStatistic<T>(name,initialValue);
        stats.insert(std::make_pair(name, nst));

        if ( owner != 0 )
            nst->setOwner(owner);

        return *nst;
    }

    Statistic* operator[](std::string statName);

    virtual void clock(ub8 cycle);

    virtual void frame(ub4 frame);

    virtual void batch();

    void setDumpScheduling(ub8 startCycle, ub8 skipCycles, ub8 nCycles, bool autoReset = true);

    void setOutputStream(ub1 cm, std::ostream& os);

    void setPerFrameStream(ub1 cm, std::ostream& os);

    void setPerBatchStream(ub1 cm, std::ostream& os);

    void reset(ub4 freq);

    void dumpNames(ub1 cm, std::ostream& os = std::cout);

    void dumpValues(ub1 cm, std::ostream& os = std::cout);

    void dumpNames(char *str, ub1 cm, std::ostream& os = std::cout);

    void dumpValues(ub4 n, ub4 freq, ub1 cm, std::ostream& os = std::cout);

    void dump(ub1 cm, std::ostream& os = std::cout);

    void dump(std::string boxName, ub1 cm, std::ostream& os = std::cout);

    void finish();

};


#endif // STATISTICSMANAGER_H
