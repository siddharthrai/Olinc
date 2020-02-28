#ifndef __SRRIP_H__
#define __SRRIP_H__

#include "common/intermod-common.h"

struct SRRIPClass
{
  ub4   _numSets;
  ub4   _numWays;
  sb1 **_lruVector;
  ub4 **_currentFillOrderArray;
};

void SRRIPClassInit (struct SRRIPClass *srripClass, ub4 numSets, ub4 numWay);

void _UpdateCurrentFillOrderArray (struct SRRIPClass* srripClass, ub4 set, ub4 way);
void UpdateOnHit (struct SRRIPClass* srripClass, ub4 set, ub4 way);
void GetReplacementCandidate (struct SRRIPClass* srripClass, ub4 set, ub4 *way);
void UpdateOnFill (struct SRRIPClass* srripClass, ub4 set, ub4 way);
ub4 GetCurrentFillOrderArray (struct SRRIPClass* srripClass, ub4 set, ub4 way);

#endif
