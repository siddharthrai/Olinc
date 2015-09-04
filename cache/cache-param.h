/* 
 * File:   cache.h
 * Author: sudhan
 *
 * Created on 25 December, 2012, 11:53 AM
 */

#ifndef CACHE_PARAM_H
#define	CACHE_PARAM_H

struct cache_params
{
  enum    cache_policy_t policy;  /* Cache Policy */
  int     num_sets;               /* # Sets */
  int     ways;                   /* Associativity */
  int     max_rrpv;               /* Maximum RRPV for RRIP based policies */
  int     spill_rrpv;             /* Spill RRPV to be used by policies */
  double  threshold;              /* Bimodal threshold */
  int     stream;                 /* Stream to be simulated. For selective simulation */
  int     streams;                /* Total number of streams */
  int     core_policy1;           /* Core id following policy 1 */
  int     core_policy2;           /* Core id following policy 2 */
  int     txs;                    /* xsratio threshold */
  int     talpha;                 /* Performance threshold */
  long    highthr;                /* High threshold */
  long    lowthr;                 /* High threshold */
  long    tlpthr;                 /* TLP threshold */
  long    pthr;                   /* GPU threshold */
  long    mthr;                   /* CPU threshold */
  ub4     sdp_samples;            /* SDP samples for corrective path */
  ub4     sdp_gpu_cores;          /* Stream aware policy GPU cores */
  ub4     sdp_cpu_cores;          /* Stream aware policy CPU cores */
  ub4     sdp_streams;            /* Stream aware policy streams (X, Y, P, Q, R) */
  ub1     sdp_cpu_tpset;          /* SDP CPU threshold for P set */
  ub1     sdp_cpu_tqset;          /* SDP CPU threshold for Q set */
  ub8     sdp_tactivate;          /* SDP CPU threshold for classification activate */
  ub1     sdp_psetinstp;          /* If set, promote pset blocks to RRPV 0 on hit */
  ub1     sdp_greplace;           /* If set, choose min wayid block as victim */
  ub1     sdp_psetbmi;            /* If set, use bimodal insertion for P set */
  ub1     sdp_psetrecat;          /* If set, recategorize block on hit */
  ub1     sdp_psetbse;            /* If set, LRU table is updated only for base samples */
  ub1     sdp_psetmrt;            /* If set, use miss rate for P set */
  ub1     sdp_psethrt;            /* If set, use hit rate for P set */
  ub1     stl_stream;             /* Src stream for stride LRU */
  ub1     rpl_on_miss;            /* Src stream for stride LRU */
  ub1     use_long_bv;            /* Src stream for stride LRU */
  ub4     gsdrrip_streams;        /* Streams for graphics stream aware DRRIP */
  ub1     bs_epoch;               /* TRUE, if only baseline samples are used for epoch */
  ub1     use_step;               /* TRUE, if step function is to be used in sappridepri */
  ub4     ship_shct_size;         /* Ship signature history table size */
  ub4     ship_sig_size;          /* Ship signature size */
  ub4     ship_entry_size;        /* Ship counter width */
  ub4     ship_use_mem;           /* Ship-mem flag */
  ub4     ship_use_pc;            /* Ship-pc flag */
  ub4     sampler_sets;           /* SARP sampler sets */
  ub4     sampler_ways;           /* SARP sampler ways */
};

#endif	/* MEM_SYSTEM_CACHE_H */
