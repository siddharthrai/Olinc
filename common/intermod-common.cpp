#include "intermod-common.h"

/* ATTILA and M2S configuration */
struct attila_config attilaconf;
struct m2s_config    m2sconf;

/* Tracks number of pending requests send to Attila */
long int agent_req_pending = 0;
  
/* Names of units used in statistics collection */
char attilaUnitName[TUC][40] = 
  {"Input Cache", 
   "Color Cache", 
   "Depth Cache", 
   "Texture Cache", 
   "Instruction Cache", 
   "HZ Cache",
   "Blitter",
   "DAC",
   "Index",
   "Shared Input Cache", 
   "Shared Color Cache",
   "Shared Depth Cache", 
   "Shared Texture Cache",
   "Shared HZ Cache", 
   "Shared Instruction Cache"};

/* Names of streams seen by LLC, used in statistics collection */
ub1 attilaStreamName[TU][40] = 
  {
    "Input",
    "Color",
    "Depth",
    "Texture",
    "Instruction",
    "HiZ",
    "Blitter",
    "DAC",
    "Index"
  };

/* Unit names */
sb1 *stream_names[TST + 1] = {"U", "I", "C", "Z", "T", "N", "H", "B", "D", "X", "P0", "P1", "P2", "P3", "P4", "DCS", "DZS", "DBS", "DPS", "OS"};

int gcd_ab(int a,int b)
{
  int c;
  if(a < b)
  {
    c = a;
    a = b;
    b = c;
  }
  while (1)
  {
    c = a%b;
    if(c == 0)
      return b;
    a = b;
    b = c;
  }
}

