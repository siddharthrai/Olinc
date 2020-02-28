#include "drrip-new.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// private function
// works only if x is a power of 2
static int logbase2 (unsigned long long x)
{
  int i = 0;
  unsigned long long y;

  y = 1;
  if (y == x) return i;

  for (i=1; x > y; i++) {
    y <<= 1;
    if (y == x) return i;
    if (y == 0) {
      printf ("You're insane\n");
      exit (1);
    }
  }
  return -1;
}

void DRRIPClassInit (DRRIPClass *drripClass, unsigned numSets, unsigned numWays)
{
   int j, k;
   drripClass->_numSets = numSets;
   drripClass->_numWays = numWays;
   drripClass->_numBanks = numSets >> 10;

   drripClass->_lruVector = (char **)calloc(1, sizeof(char *) * numSets);
   assert(drripClass->_lruVector != NULL);
   for (j=0; j<numSets; j++) {
      drripClass->_lruVector[j] = (char *)calloc(1, sizeof(char) * numWays);
      assert(drripClass->_lruVector[j] != NULL);
      for (k=0; k<drripClass->_numWays; k++) {
         drripClass->_lruVector[j][k] = 0;
      }
   }

   assert(DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS >= numSets/(drripClass->_numBanks*DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS));
   drripClass->_BIPcounter = 0;
   drripClass->_DIPchooser = DIP_CHOOSER_MAX/2;
   drripClass->_logSetsPerConstituency = logbase2(drripClass->_numSets/(drripClass->_numBanks*DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS));

   drripClass->_srripmiss       = 0;
   drripClass->_brripmiss       = 0;
   drripClass->_followed_srrip  = 0;
   drripClass->_followed_brrip  = 0;

   drripClass->_currentFillOrderArray = (unsigned **)calloc(1, sizeof(unsigned*) * drripClass->_numSets);   // For each set in the bank
   assert(drripClass->_currentFillOrderArray != NULL);
   for (j=0; j<drripClass->_numSets; j++) {
      drripClass->_currentFillOrderArray[j] = (unsigned *)calloc(1, sizeof(unsigned) * drripClass->_numWays);   // For each way in the set
      assert(drripClass->_currentFillOrderArray[j] != NULL);
      for (k=0; k<drripClass->_numWays; k++) {
         drripClass->_currentFillOrderArray[j][k] = drripClass->_numWays;   // Indicative of unfilled position
      }
   }
}


void
DRRIPUpdateOnHit (DRRIPClass *drripClass, unsigned set, unsigned way)
{
   drripClass->_lruVector[set][way] = 3;
}

void DRRIPUpdateOnMiss (DRRIPClass *drripClass, unsigned set)
{
   if ((set & (drripClass->_numSets/(drripClass->_numBanks*DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS)-1)) == ((set >> drripClass->_logSetsPerConstituency)
& (drripClass->_numSets/(drripClass->_numBanks*DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS)-1))) {
      // SRRIP sample
      if (drripClass->_DIPchooser < DIP_CHOOSER_MAX-1) {
         drripClass->_DIPchooser++;
      }
      drripClass->_srripmiss++;
   }
   else if ((~set & (drripClass->_numSets/(drripClass->_numBanks*DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS)-1)) == ((set >>
drripClass->_logSetsPerConstituency) & (drripClass->_numSets/(drripClass->_numBanks*DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS)-1))) {
      // BRRIP sample
      if (drripClass->_DIPchooser > 0) {
         drripClass->_DIPchooser--;
      }
      drripClass->_brripmiss++;
   }
}

void DRRIPGetReplacementCandidate (DRRIPClass *drripClass, unsigned set, unsigned *way)
{
   int i;

   while (1) {
      for (i=0; i<drripClass->_numWays; i++) {
         if (!drripClass->_lruVector[set][i]) {
            (*way) = i;
            return;
         }
      }

      for (i=0; i<drripClass->_numWays; i++) {
         assert(drripClass->_lruVector[set][i] > 0);
         drripClass->_lruVector[set][i]--;
      }
   }

   assert(0);
}

void DRRIPUpdateOnReplacement (DRRIPClass *drripClass, unsigned set, unsigned way)
{
}

void DRRIPUpdateOnFill (DRRIPClass *drripClass, unsigned set, unsigned way)
{
   if ((set & (drripClass->_numSets/(drripClass->_numBanks*DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS)-1)) == ((set >> drripClass->_logSetsPerConstituency)
& (drripClass->_numSets/(drripClass->_numBanks*DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS)-1))) {
      // SRRIP sample
      drripClass->_lruVector[set][way] = 1;
   }
   else if ((~set & (drripClass->_numSets/(drripClass->_numBanks*DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS)-1)) == ((set >>
drripClass->_logSetsPerConstituency) & (drripClass->_numSets/(drripClass->_numBanks*DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS)-1))) {
      // BRRIP sample
      if (drripClass->_BIPcounter == 0) {
         drripClass->_lruVector[set][way] = 1;
      }
      else {
         drripClass->_lruVector[set][way] = 0;
      }
   }
   else {
      // Follower
      if (drripClass->_DIPchooser >= DIP_CHOOSER_MAX/2) {
        drripClass->_followed_brrip += 1;
         // BRRIP
         if (drripClass->_BIPcounter == 0) {
            drripClass->_lruVector[set][way] = 1;
         }
         else {
            drripClass->_lruVector[set][way] = 0;
         }
      }
      else {
        drripClass->_followed_srrip += 1;
         // SRRIP
         drripClass->_lruVector[set][way] = 1;
      }
   }

   if (drripClass->_BIPcounter == BIP_EPSILON_RECIPROCAL-1) {
      drripClass->_BIPcounter = 0;
   }
   else {
      drripClass->_BIPcounter++;
   }

   DRRIP_UpdateCurrentFillOrderArray (drripClass, set, way);
}

unsigned DRRIPGetCurrentFillOrderArray (DRRIPClass *drripClass, unsigned set, unsigned way)
{
   return drripClass->_currentFillOrderArray[set][way];
}

void DRRIP_UpdateCurrentFillOrderArray (DRRIPClass *drripClass, unsigned set, unsigned way)
{
   int i;

   for (i=0; i<drripClass->_numWays; i++) {
      if (drripClass->_currentFillOrderArray[set][i] < drripClass->_currentFillOrderArray[set][way]) {
         drripClass->_currentFillOrderArray[set][i]++;
         assert(drripClass->_currentFillOrderArray[set][i] < drripClass->_numWays);
      }
   }
   drripClass->_currentFillOrderArray[set][way] = 0;
}

unsigned DRRIPGetBRRIPmiss (DRRIPClass *drripClass)
{
   return drripClass->_brripmiss;
}

unsigned DRRIPGetSRRIPmiss (DRRIPClass *drripClass)
{
   return drripClass->_srripmiss;
}
