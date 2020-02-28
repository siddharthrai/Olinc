#ifndef __DRRIP_H__
#define __DRRIP_H__

#define DIP_DEDICATED_SET_COUNT_PER_THOUSAND_SETS 32
#define BIP_EPSILON_RECIPROCAL 32
#define DIP_CHOOSER_MAX 1024

typedef struct DRRIPClass 
{
  unsigned _numSets;
  unsigned _numWays;
  unsigned _numBanks;
  unsigned _BIPcounter;
  unsigned _DIPchooser;
  unsigned _logSetsPerConstituency;
  char **_lruVector;

  unsigned _srripmiss, _brripmiss;
  unsigned _followed_brrip, _followed_srrip;

  unsigned **_currentFillOrderArray;
}DRRIPClass;

void DRRIPClassInit (DRRIPClass *drripClass, unsigned numSets, unsigned numWay);

void DRRIP_UpdateCurrentFillOrderArray (DRRIPClass *drripClass, unsigned set, unsigned way);
void DRRIPUpdateOnHit (DRRIPClass *drripClass, unsigned set, unsigned way);
void DRRIPUpdateOnMiss (DRRIPClass *drripClass, unsigned set);
void DRRIPGetReplacementCandidate (DRRIPClass *drripClass, unsigned set, unsigned *way);
void DRRIPUpdateOnReplacement (DRRIPClass *drripClass, unsigned set, unsigned way);
void DRRIPUpdateOnFill (DRRIPClass *drripClass, unsigned set, unsigned way);

unsigned DRRIPGetCurrentFillOrderArray (DRRIPClass *drripClass, unsigned set, unsigned way);

unsigned DRRIPGetBRRIPmiss (DRRIPClass *drripClass);
unsigned DRRIPGetSRRIPmiss (DRRIPClass *drripClass);
#endif

