#ifndef CACHE_BLOCK_H
#define CACHE_BLOCK_H

#define CACHE_UPDATE_BLOCK_STATE(block, tag, state_in)        \
do                                                            \
{                                                             \
    (block)->tag   = (tag);                                   \
    (block)->state = (state_in);                              \
}while(0)

#define CACHE_GET_BLOCK_STREAM(block, strm)                   \
do                                                            \
{                                                             \
    strm = (block)->stream;                                   \
}while(0)

#define CACHE_UPDATE_BLOCK_STREAM(block, strm)                \
do                                                            \
{                                                             \
    (block)->stream = strm;                                   \
}while(0)

#define CACHE_UPDATE_BLOCK_PC(block, pc_in)                   \
do                                                            \
{                                                             \
    (block)->pc = (pc_in);                                    \
}while(0)

#define CACHE_UPDATE_BLOCK_EVENT(block, event, val)           \
do                                                            \
{                                                             \
    (block)->notify_event[event] = val;                       \
}while(0)

enum cache_block_state_t
{
  cache_block_invalid = 0,
  cache_block_modified,
  cache_block_exclusive,
  cache_block_shared
};

struct cache_block_t
{
  struct cache_block_t *next; /* Next link pointer */
  struct cache_block_t *prev; /* Prev link pointer */
  void                 *data; /* Metadata of a node */
  
  /* GSPC state, i.e., filled by Texture or accessed by Color etc. */
  unsigned int block_state;
  
  /* Core last accessed the block */
  unsigned int pid;

  /* Stream last accessed the block */
  unsigned int stream;

  /* Stream filled the block */
  unsigned int src_stream;

  /* Stream last accessed the block */
  unsigned int sap_stream;
  
  /* Flag set if block is produced by Blitter */
  unsigned char is_bt_block;

  /* Flag set if block is produced by color writer */
  unsigned char is_ct_block;

  /* Flag set if block is produced by depth unit */
  unsigned char is_zt_block;
  
  /* TRUE, if block is pinned */
  unsigned char is_block_pinned;

  /* Access made to this block, used for measuring maximum reuse */
  unsigned int access;
  
  /* TRUE, if block has been demoted */
  unsigned char demote;

  /* TRUE, if block has been demoted to head */
  unsigned char fill_at_head;

  /* TRUE, if block has been demoted to tail */
  unsigned char fill_at_tail;

  /* TRUE, if block has been demoted to head */
  unsigned char demote_at_head;

  /* TRUE, if block has been demoted to tail */
  unsigned char demote_at_tail;

  /* Head identifier  */
  unsigned int head : 1;
  
  /* Cache block state for coherence, tag and way-id. */
  enum cache_block_state_t state;

  long long tag;                /* Block tag */
  long long vtl_addr;           /* Block virtual address */
  long long tag_end;            /* Secondary tag for stride-based storage */
  int       way;                /* Physical way-id */
  long long replacements;       /* Tracks number of replacements for a block */
  long long hits;               /* Tracks number of hits for the block */
  long long false_hits;         /* Tracks number of hits for the block */
  long long unknown_hits;       /* Tracks number of hits for the block */
  long long max_stride;         /* Tracks number of hits for the block */
  long long max_use;            /* Tracks number of hits for the block */
  long long use;                /* Tracks number of uses for the block */
  unsigned char *long_use;      /* Full bit-vector */
  unsigned char *long_hit;      /* Full bit-vector */
  long long hit;                /* Tracks number of hits for the block */
  long long expand;             /* Tracks number of expansion for the block */
  unsigned int use_bitmap;      /* Bitmap for region usage */
  unsigned int max_use_bitmap;  /* Max-use-bitmap for region */
  unsigned int hit_bitmap;      /* Bitmap for hit regions */
  unsigned int max_hit_bitmap;  /* Max-bitmap for hit regions */
  unsigned int captured_use;    /* Uses captured by all lives of the blocks */
  unsigned int color_transition;/* Number of CT or TC transitions */
  long long    x_stream_use;    /* X stream use of block */ 
  long long    s_stream_use;    /* X stream use of block */ 
  int          last_rrpv;       /* Keeps track of last assigned RRPV */
  int          fill_rrpv;       /* Keeps track of fill RRPV */

  /* Cache block busy flag. When set this flag instructs the cache
   * replacement policy to not evict the block as there is a request
   * pending for it. This flag is only used by LLC module. The LLC module
   * sets the flag to 1, on sending an intervention request to the owner
   * of the block. The flag is cleared when the home module receives
   * intervention reply - sharing write-back or ownership transfer message. */
  unsigned int busy : 1;

  /* This flag is only used by L1 & L2 caches in a three level module
   * hierarchy. When set for a L1 cache block this flag indicates that
   * the block is cached in also cached in L2 cache and when set for
   * a L2 cache block this flag indicates that the block is cached in a
   * L1 cache. */
  unsigned int cached : 1;

  /* When this flag is set that the block was filled in the module by a
   * prefetch request. */
  unsigned int prefetch : 1;
  
  /* NRU bit  */
  unsigned int nru : 1;
  
  /* Flag for dirty block */
  unsigned int dirty : 1;
  
  unsigned int epoch;

  /* Recency of the block */
  unsigned long long recency;

  unsigned long long pc;
  
  /* Ship policy specific signature */
  unsigned long long ship_sign;
};

#endif
