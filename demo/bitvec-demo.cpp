#include <iostream>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../common/intermod-common.h"

using namespace std;

#define STRIDE_BITS (6)
#define BSIZE       (8)
#define LBSIZE      (3)

#define CACHE_LONG_MASK(p)     ((ub4)(0x1) << (p))
#define CACHE_LONG_BIV(bv, p)  (((bv)[(p) >> LBSIZE] & CACHE_LONG_MASK((p) % BSIZE)) >> ((p) % BSIZE))
#define CACHE_LONG_BIS(bv, p)  ((bv)[(p) >> LBSIZE] |= CACHE_LONG_MASK((p) % BSIZE))
#define CACHE_LONG_BIC(bv, p)  ((bv)[(p) >> LBSIZE] &= ~CACHE_LONG_MASK((p) % BSIZE))

#define CACHE_LONG_SFL(bv, p)                                 \
do                                                            \
{                                                             \
  unsigned char tmp;                                          \
  unsigned int  end;                                          \
  end = pow(2, STRIDE_BITS - LBSIZE) - 1;                     \
  tmp = 0;                                                    \
  for (int i = pow(2, STRIDE_BITS) - (1 + p), j = end; i >= 0;)\
  {                                                           \
    if (i < BSIZE)                                            \
    {                                                         \
      tmp = get_from_source(bv, 0, i);                        \
      i = -1;                                                 \
    }                                                         \
    else                                                      \
    {                                                         \
      tmp = get_from_source(bv, i - BSIZE + 1, i);            \
      i -= BSIZE;                                             \
    }                                                         \
    cout << i << " O " << (int)bv[j] << " " << (int)tmp << endl;    \
    bv[j--] = tmp;                                            \
  }                                                           \
}while(0)

#define CACHE_LONG_RST(bv)                                    \
do                                                            \
{                                                             \
  for (int i = 0; i < pow(2, STRIDE_BITS - 3); i++)           \
  {                                                           \
    (bv)[i] = 0;                                              \
  }                                                           \
}while(0)

unsigned char get_from_source(ub1 *bv, ub4 start, ub4 end)
{
  ub4 start_idx;
  ub4 start_off;
  ub4 end_idx;
  ub4 end_off;
  ub1 stride;
  ub1 start_size;
  ub1 end_size;
  ub1 start_mask;
  ub1 end_mask;
  ub1 result;

  result = 0;
  stride = end - start + 1;

  assert(stride <= BSIZE);

  /* Obtain start and end index and offset of linear idex values */
  start_idx   = start / BSIZE;
  start_off   = start % BSIZE;
  end_idx     = end / BSIZE;
  end_off     = end % BSIZE;

  start_size  = BSIZE - start_off;
  start_mask  = ((int)(pow(2, start_size) - 1)) << start_off;

  if (start_idx == end_idx)
  {
    /* If start and end are in the same byte, update mask to include end 
     * offset */
    start_mask = (start_mask << (BSIZE - 1 - end_off)) >> (BSIZE - 1 - end_off);
    result    |= (bv[start_idx] & start_mask) << (BSIZE - 1 - end_off);

    /* Reset old values */
    bv[start_idx] &= ~start_mask;
  }
  else
  {
    /* Apply start mask */
    result |= (bv[start_idx] & start_mask) >> start_off;

    /* Reset old values in start byte */
    bv[start_idx] &= ~start_mask;

    /* Build end mask */
    end_size = end_off + 1;
    end_mask = pow(2, end_size) - 1;

    /* Apply end mask */
    result |= (bv[end_idx] & end_mask) << start_size;

    /* Reset old values in end byte */
    bv[end_idx] &= ~end_mask;
  }

  cout << "Shift " << start << " " << end << " " << (int)result << endl;

  return result;
}

int main()
{
  ub1 *bv;

  bv = (ub1 *)malloc(sizeof(char) * (pow(2, STRIDE_BITS - LBSIZE)));
  assert(bv);
  
  memset(bv, 0x0f, pow(2, STRIDE_BITS - LBSIZE));

  for (int i = pow(2, STRIDE_BITS - LBSIZE) - 1; i >=0; i--)
  {
    cout << (int)bv[i] << " ";
  }
  
  cout << endl;

  CACHE_LONG_SFL(bv, 7);
  
  for (int i = pow(2, STRIDE_BITS - LBSIZE) - 1; i >=0; i--)
  {
    cout << (int)bv[i] << " ";
  }
  
  return 0;
}

