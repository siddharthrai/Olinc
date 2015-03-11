#ifndef POLICY_H
#define POLICY_H

#define BYPASS_WAY (0xffff)

/* Common list enqueue dequeue functions used by all policies */

/* Header queue enqueue dequeue function */
#define CACHE_REMOVE_FROM_QUEUE(block, head_ptr, tail_ptr)    \
do                                                            \
{                                                             \
  /* Block is at the head of the list */                      \
  if ((block)->next == NULL)                                  \
  {                                                           \
    (head_ptr).head = (block)->prev;                          \
    if ((block)->prev)                                        \
    {                                                         \
      (block)->prev->next = NULL;                             \
    }                                                         \
  }                                                           \
                                                              \
  /* Block is at the tail of the list */                      \
  if ((block)->prev == NULL)                                  \
  {                                                           \
    (tail_ptr).head = (block)->next;                          \
    if ((block)->next)                                        \
    {                                                         \
      (block)->next->prev = NULL;                             \
    }                                                         \
  }                                                           \
                                                              \
  /* Block in the middle of the list */                       \
  if ((block)->next != NULL && (block)->prev != NULL)         \
  {                                                           \
    ((block)->next)->prev = (block)->prev;                    \
    ((block)->prev)->next = (block)->next;                    \
  }                                                           \
                                                              \
  (block)->data = NULL;                                       \
  (block)->next = NULL;                                       \
  (block)->prev = NULL;                                       \
}while(0)

#define CACHE_APPEND_TO_QUEUE(block, head_ptr, tail_ptr)      \
do                                                            \
{                                                             \
  if ((head_ptr).head == NULL && (tail_ptr).head == NULL)     \
  {                                                           \
    (head_ptr).head = (block);                                \
    (tail_ptr).head = (block);                                \
    (block)->data   = (&head_ptr);                            \
    break;                                                    \
  }                                                           \
                                                              \
  assert((block->next == NULL) && (block)->prev == NULL);     \
                                                              \
  /* Insert block at tail of valid list */                    \
  ((tail_ptr).head)->prev  = (block);                         \
  (block)->next            = (tail_ptr).head;                 \
  (tail_ptr).head          = (block);                         \
  (block)->data            = (&head_ptr);                     \
}while(0)

#define CACHE_PREPEND_TO_QUEUE(block, head_ptr, tail_ptr)     \
do                                                            \
{                                                             \
  if ((head_ptr).head == NULL && (tail_ptr).head == NULL)     \
  {                                                           \
    (head_ptr).head = (block);                                \
    (tail_ptr).head = (block);                                \
    (block)->data   = (&head_ptr);                            \
    break;                                                    \
  }                                                           \
                                                              \
  assert((block->next == NULL) && (block)->prev == NULL);     \
                                                              \
  /* Insert block at head of valid list */                    \
  ((head_ptr).head)->next  = (block);                         \
  (block)->prev            = (head_ptr).head;                 \
  (head_ptr).head          = (block);                         \
  (block)->data            = (&head_ptr);                     \
}while(0)

/* Simple queue enqueue dequeue functions */
#define CACHE_REMOVE_FROM_SQUEUE(block, head_ptr, tail_ptr)   \
do                                                            \
{                                                             \
  if ((block)->next == NULL)                                  \
  {                                                           \
    (head_ptr) = (block)->prev;                               \
    if ((block)->prev)                                        \
    {                                                         \
      (block)->prev->next = NULL;                             \
    }                                                         \
  }                                                           \
                                                              \
  if ((block)->prev == NULL)                                  \
  {                                                           \
    (tail_ptr) = (block)->next;                               \
    if ((block)->next)                                        \
    {                                                         \
      (block)->next->prev = NULL;                             \
    }                                                         \
  }                                                           \
                                                              \
  if ((block)->next != NULL && (block)->prev != NULL)         \
  {                                                           \
    ((block)->next)->prev = (block)->prev;                    \
    ((block)->prev)->next = (block)->next;                    \
  }                                                           \
                                                              \
  (block)->next = NULL;                                       \
  (block)->prev = NULL;                                       \
}while(0)

#define CACHE_APPEND_TO_SQUEUE(block, head_ptr, tail_ptr)     \
do                                                            \
{                                                             \
  if ((head_ptr) == NULL && (tail_ptr) == NULL)               \
  {                                                           \
    (head_ptr) = (block);                                     \
    (tail_ptr) = (block);                                     \
    break;                                                    \
  }                                                           \
                                                              \
  assert((block->next == NULL) && (block)->prev == NULL);     \
                                                              \
  /* Insert block at tail of valid list */                    \
  ((tail_ptr))->prev  = (block);                              \
  (block)->next       = (tail_ptr);                           \
  (tail_ptr)          = (block);                              \
}while(0)

#define CACHE_PREPEND_TO_SQUEUE(block, head_ptr, tail_ptr)    \
do                                                            \
{                                                             \
  if ((head_ptr) == NULL && (tail_ptr) == NULL)               \
  {                                                           \
    (head_ptr) = (block);                                     \
    (tail_ptr) = (block);                                     \
    break;                                                    \
  }                                                           \
                                                              \
  assert((block->next == NULL) && (block)->prev == NULL);     \
                                                              \
  /* Insert block at tail of valid list */                    \
  ((head_ptr))->next  = (block);                              \
  (block)->prev       = (head_ptr);                           \
  (head_ptr)          = (block);                              \
}while(0)

typedef enum cache_policy_t
{
        cache_policy_invalid,

        /* NRU replacement  */
        cache_policy_bypass,

        /* NRU replacement  */
        cache_policy_opt,

        /* NRU replacement  */
        cache_policy_nru,

        /* LRU replacement, MRU insertion, on hit move block to MRU. */
        cache_policy_lru,

        /* LRU replacement, MRU insertion, on hit move block to MRU. */
        cache_policy_stridelru,

        /* LRU replacement, MRU insertion, no aging. */
        cache_policy_fifo,

        /* LRU insertion, LRU replacement */
        cache_policy_lip,
        
        /* Bimodal insertion, LRU replacement */
        cache_policy_bip,
        
        /* Dynamic policy between LIP and BIP */
        cache_policy_dip,
        
        /* Insertion at MAXRRPV - 1, eviction form MAXRRPV */
        cache_policy_srrip,
        
        /* Insertion at MAXRRPV - 1, eviction form MAXRRPV */
        cache_policy_srripm,
        
        /* Insertion at MAXRRPV - 1, eviction form MAXRRPV */
        cache_policy_srript,
        
        /* Bimodal insertion, eviction from MAXRRPV */
        cache_policy_brrip,
        
        /* Dynamic policy between SRRIP and BRRIP */
        cache_policy_drrip,

        /* Graphics stream aware insertion, replacement from MAXRRPV  */
        cache_policy_gspc,

        /* Graphics stream aware insertion, replacement from MAXRRPV  
         * with blitter stream identification */
        cache_policy_gspcm,

        /* Graphics stream aware insertion, replacement from MAXRRPV  
         * with blitter stream identification */
        cache_policy_gspct,

        /* Graphics + CPU stream aware insertion, replacement from MAXRRPV  */
        cache_policy_gshp,

        /* UCP, cache partitoning  */
        cache_policy_ucp,

        /* UCP, cache partitoning  */
        cache_policy_tapucp,

        /* UCP, cache partitoning  */
        cache_policy_tapdrrip,

        /* LRU with stream aware policy */
        cache_policy_salru,
        
        /* Replace streams below current stream first */
        cache_policy_cpulast,

        /* HELM DRRIP */
        cache_policy_helmdrrip,

        /* Stream aware policy */
        cache_policy_sap,

        /* Stall directed policy */
        cache_policy_sdp,

        /* Phase aware SRRIP */
        cache_policy_pasrrip,
        
}cache_policy_t;

typedef struct list_head_t
{
  int id;
  struct cache_block_t *head;
}list_head_t;

#endif
