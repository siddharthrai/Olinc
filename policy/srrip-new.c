#include "srrip-new.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void SRRIPClassInit (struct SRRIPClass *srripClass, unsigned numSets, unsigned numWays)
{
  int j, k;
  srripClass->_numSets = numSets;
  srripClass->_numWays = numWays;
  
  srripClass->_lruVector = (char **)malloc(sizeof(char *) * numSets);
  assert(srripClass->_lruVector); 

  /* */
  for (j = 0; j < srripClass->_numSets; j++) 
  {
    srripClass->_lruVector[j] = (char *)malloc(sizeof(char) * numWays);
    assert(srripClass->_lruVector[j] != NULL);

    for (k = 0; k < srripClass->_numWays; k++) 
    {
      srripClass->_lruVector[j][k] = 0;
    }
  }

  srripClass->_currentFillOrderArray = (unsigned **)malloc(sizeof(unsigned*) * numSets);   // For each set in the bank
  assert(srripClass->_currentFillOrderArray != NULL);

  for (j = 0; j < numSets; j++) 
  {
    srripClass->_currentFillOrderArray[j] =  (unsigned *)malloc(sizeof(unsigned) * numWays);   // For each way in the set
    assert(srripClass->_currentFillOrderArray[j] != NULL);

    for (k=0; k<srripClass->_numWays; k++) 
    {
      srripClass->_currentFillOrderArray[j][k] = srripClass->_numWays;   // Indicative of unfilled position
    }
  }
}

void UpdateOnHit (struct SRRIPClass *srripClass, unsigned set, unsigned way)
{
  srripClass->_lruVector[set][way] = 3;
}

void GetReplacementCandidate (struct SRRIPClass *srripClass, unsigned set, unsigned *way)
{
  int i;

  while (1) {
    for (i=0; i<srripClass->_numWays; i++) {
      if (!srripClass->_lruVector[set][i]) {
        (*way) = i;
        return;
      }
    }

    for (i=0; i<srripClass->_numWays; i++) {
      assert(srripClass->_lruVector[set][i] > 0);
      srripClass->_lruVector[set][i]--;
    }
  }

  assert(FALSE);
}

void UpdateOnFill (struct SRRIPClass *srripClass, unsigned set, unsigned way)
{
    srripClass->_lruVector[set][way] = 1;
  _UpdateCurrentFillOrderArray(srripClass, set, way);
}

unsigned GetCurrentFillOrderArray (struct SRRIPClass *srripClass, unsigned set, unsigned way)
{
  return srripClass->_currentFillOrderArray[set][way];
}

void _UpdateCurrentFillOrderArray (struct SRRIPClass *srripClass, unsigned set, unsigned way)
{
  int i;

  for (i = 0; i < srripClass->_numWays; i++) 
  {
    if (srripClass->_currentFillOrderArray[set][i] < srripClass->_currentFillOrderArray[set][way])
    {
      srripClass->_currentFillOrderArray[set][i]++;
      assert(srripClass->_currentFillOrderArray[set][i] < srripClass->_numWays);
    }
  }

  srripClass->_currentFillOrderArray[set][way] = 0;
}

