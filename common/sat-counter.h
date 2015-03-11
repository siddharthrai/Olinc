#ifndef SAT_COUNTER_H
#define SAT_COUNTER_H

#define SAT_CTR_INI(ctr, w, mi, mx) \
do                                  \
{                                   \
  (ctr).val   = 0;                  \
  (ctr).width = w;                  \
  (ctr).min   = mi;                 \
  (ctr).max   = mx;                 \
}while(0)

#define SAT_CTR_VAL(ctr)    ((ctr).val)
#define IS_CTR_SAT(ctr)     ((ctr).val == (ctr).max)
#define SAT_CTR_SET(ctr, v) ((ctr).val = (v))
#define SAT_CTR_RST(ctr)    ((ctr).val = 0)
#define SAT_CTR_HLF(ctr)    ((ctr).val = ((ctr).val > 1) ? ((ctr).val / 2) : (ctr).val)
#define SAT_CTR_INC(ctr)    ((ctr).val = ((ctr).val < (ctr).max) ? (ctr).val + 1 : (ctr).val)
#define SAT_CTR_DEC(ctr)    ((ctr).val = ((ctr).val > (ctr).min) ? (ctr).val - 1 : (ctr).val)

typedef struct saturating_counter_t 
{
  ub1 width;
  sb4 min;
  sb4 max;
  sb4 val;
}sctr;

#endif
