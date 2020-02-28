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

#include "StatisticsManager.h"
#include <assert.h>
#include <algorithm>

using namespace std;

// StatisticsManager* StatisticsManager::sm = 0;

StatisticsManager::StatisticsManager():
startCycle(0), nextReset(999), nCycles(1000), nextDump(999), lastCycle(-1), autoReset(true)
{
  for (ub4 i = 0; i < TCM; i++)
  {
    osCycle[i] = NULL;
    osFrame[i] = NULL;
    osBatch[i] = NULL;
    cyclesFlagNamesDumped[i] = false;
  }
}

StatisticsManager& StatisticsManager::instance()
{
    static StatisticsManager* sm = new StatisticsManager();
    return *sm;
}

Statistic* StatisticsManager::find(std::string name)
{
    map<string,Statistic*>::iterator it = stats.find(name);
    if ( it == stats.end() )
        return 0;
    return it->second;
}

void StatisticsManager::clock(ub8 cycle)
{
    // static bool flag = false;
    lastCycle = cycle;

    if ( cycle >= startCycle )
        Statistic::enable();
    else
        Statistic::disable();

    if ( cycle >= nextDump )
    {
      for (ub4 i = 0; i < TCM; i++)
      {
        if (!osCycle[i])
          continue;

        if ( !cyclesFlagNamesDumped[i] )
        {
          cyclesFlagNamesDumped[i] = true;

          if (osCycle[i] != NULL)
            dumpNames(i, *osCycle[i]);
        }

        if (osCycle != NULL)
          dumpValues(i, *osCycle[i]);
      }

      if ( autoReset )
      {
        startCycle = cycle + 1;
        reset(FREQ_CYCLES);
      }

      /* Update next start of statistics collection */
      nextReset = cycle + skipCycle;

      /* Update next dump cycle */
      nextDump = nextReset + nCycles;
    }
    
    if (cycle == nextReset)
    {
      reset(FREQ_CYCLES); 
    }
}

void StatisticsManager::frame(ub4 frame)
{
    static bool namesOut[TCM];
    static ub4 sframe = 0;

    for (ub4 i = 0; i < TCM && sframe == 0; i++)
    {
      namesOut[i] = false;
    }

    //  Check if output stream for per frame statistics is defined.
    if (osFrame != NULL)
    {
      for (ub4 i = 0; i < TCM; i++)
      {
        if (!osFrame[i])
          continue;

        if (namesOut[i] == false)
        {
            namesOut[i] = true;
            dumpNames("Frame", i, *osFrame[i]);
        }

        dumpValues(frame, FREQ_FRAME, i, *osFrame[i]);
      }
      
        sframe++;

        reset(FREQ_FRAME);
    }
}

void StatisticsManager::batch()
{
    static bool namesOut[TCM];
    static ub4 batch = 0;

    for (ub4 i = 0; i < TCM && batch == 0; i++) 
    {
      namesOut[i] = false;
    }

    //  Check if the output stream for per batch statistics is defined
    if (osBatch != NULL)
    {
      for (ub4 i = 0; i < TCM; i++)
      {
        if (!osBatch[i])
          continue;

        if (namesOut[i] == false)
        {
            namesOut[i] = true;
            dumpNames("Batch", i, *osBatch[i]);
        }

        dumpValues(batch, FREQ_BATCH, i, *osBatch[i]);
      }

        batch++;

        reset(FREQ_BATCH);
    }
}

Statistic* StatisticsManager::operator[](std::string statName)
{
    return find(statName);
}


void StatisticsManager::setDumpScheduling(ub8 startCycle, ub8 skipCycle, ub8 nCycles, bool autoReset)
{
    this->startCycle = startCycle;
    this->skipCycle  = skipCycle;
    this->nextDump   = nextReset + nCycles - 1;
    this->nCycles    = nCycles; /* dump every 'nCycles' cycles */
    this->autoReset  = autoReset;
}

void StatisticsManager::setOutputStream(ub1 cm, ostream& os)
{
  assert(cm <= TCM);
  osCycle[cm] = &os;
}

void StatisticsManager::setPerFrameStream(ub1 cm, ostream& os)
{
    assert(cm <= TCM);
    osFrame[cm] = &os;
}

void StatisticsManager::setPerBatchStream(ub1 cm, ostream& os)
{
    assert(cm <= TCM);
    osBatch[cm] = &os;
}

void StatisticsManager::reset(ub4 freq)
{
    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++ )
        (it->second)->clear(freq);
}

void StatisticsManager::dumpValues(ub1 cm, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    if (nextReset != lastCycle) 
    {
      os << nextReset << ".." << lastCycle;
      for ( ; it != stats.end(); it++ )
      {
        (*(it->second)).setCurrentFreq(FREQ_CYCLES);

        if (!((*(it->second)).getName()).compare(0,2, cm_name[cm]) && 
            (*(it->second)).isStatisticEnabled())
        {
          os << ";" << *(it->second);
        }
      }
      os << endl;
    }
}

void StatisticsManager::dumpValues(ub4 n, ub4 freq, ub1 cm, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << n;
    for ( ; it != stats.end(); it++ )
    {
        (*(it->second)).setCurrentFreq(freq);

        if (!((*(it->second)).getName()).compare(0,2, cm_name[cm]) &&
            (*(it->second)).isStatisticEnabled())
        {
          os << ";" << *(it->second);
        }
    }

    os << endl;
}


void StatisticsManager::dumpNames(ub1 cm, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << "Cycles";
    for ( ; it != stats.end(); it++ )
    {
        if (!((*(it->second)).getName()).compare(0,2, cm_name[cm]) && 
            (*(it->second)).isStatisticEnabled())
        {
          os << ";" << (it->first).substr(3);
        }
    }

    os << endl;
}

void StatisticsManager::dumpNames(char *str, ub1 cm, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << str;
    for ( ; it != stats.end(); it++ )
    {
        if (!((*(it->second)).getName()).compare(0,2, cm_name[cm]) &&
            (*(it->second)).isStatisticEnabled())
        {
          os << ";" << (it->first).substr(3);
        }
    }
    os << endl;
}

void StatisticsManager::dump(ub1 cm, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++ )
    {
        if (!((*(it->second)).getName()).compare(0,2, cm_name[cm]) &&
            (*(it->second)).isStatisticEnabled())
        {
          os << it->second->getName() << " = " << *(it->second) << endl;
        }
    }
}


void StatisticsManager::dump(string boxName, ub1 cm, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++ )
    {
        if ( it->second->getOwner() == boxName )
          if (!((*(it->second)).getName()).compare(0,2, cm_name[cm]) &&
              (*(it->second)).isStatisticEnabled())
          {
            os << it->second->getName() << " = " << *(it->second) << endl;
          }
    }

}

void StatisticsManager::finish()
{
    ub8 prevDump = nextDump + 1 - nCycles;
    if ( lastCycle > prevDump ) {
        if ( osCycle ) 
        {
          for (ub4 i = 0; i < TCM; i++)
          {
            if (!osCycle[i])
              continue;

            if ( !cyclesFlagNamesDumped[i] )
            {
              cyclesFlagNamesDumped[i] = true;
              dumpNames(i, *osCycle[i]);
            }
            dumpValues(i, *osCycle[i]);
          }
        }    
    }
}
