/*
 *  Copyright (C) 2014  Siddharth Rai
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <assert.h>

#include "ship.h"
#include "libstruct/string.h"
#include "libstruct/misc.h"
#include "libmhandle/mhandle.h"
#include "sap.h"

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))
#define BYPASS_RRPV             (-1)

/* Update blocks for Color stream */
#define CACHE_UPDATE_BLOCK_ACCESS(b)  ((b)->access += 1)  


#define CACHE_SHIP_INCREMENT_RRPV(head_ptr, tail_ptr, rrpv)   \
do                                                            \
{                                                             \
    int dif = 0;                                              \
                                                              \
    for (int i = rrpv - 1; i >= 0; i--)                       \
    {                                                         \
      assert(i <= rrpv);                                      \
                                                              \
      if ((head_ptr)[i].head)                                 \
      {                                                       \
        if (!dif)                                             \
        {                                                     \
          dif = rrpv - i;                                     \
        }                                                     \
                                                              \
        assert((tail_ptr)[i].head);                           \
        (head_ptr)[i + dif].head  = (head_ptr)[i].head;       \
        (tail_ptr)[i + dif].head  = (tail_ptr)[i].head;       \
        (head_ptr)[i].head        = NULL;                     \
        (tail_ptr)[i].head        = NULL;                     \
                                                              \
        struct cache_block_t *node = (head_ptr)[i + dif].head;\
                                                              \
        /* point data to new RRPV head */                     \
        for (; node; node = node->prev)                       \
        {                                                     \
          node->data = &(head_ptr[i + dif]);                  \
        }                                                     \
      }                                                       \
      else                                                    \
      {                                                       \
        assert(!((tail_ptr)[i].head));                        \
      }                                                       \
    }                                                         \
}while(0)

/* Update blocks for Color stream */
#define CACHE_UPDATE_STREAM_BLOCKS(block, strm)               \
do                                                            \
{                                                             \
  if ((block)->stream == CS && strm != CS)                    \
  {                                                           \
    ship_cs_blocks--;                                         \
  }                                                           \
                                                              \
  if (strm == CS && (block)->stream != CS)                    \
  {                                                           \
    ship_cs_blocks++;                                         \
  }                                                           \
}while(0)


/* Macros to obtain signature */
#define SIGNSIZE(g)     ((g)->sign_size)
#define SHCTSIZE(g)     ((g)->shct_size)
#define SB(g)           ((g)->sign_source == USE_MEMPC)
#define SP(g)           ((g)->sign_source == USE_PC)
#define SM(g)           ((g)->sign_source == USE_MEM)
#define SIGMAX_VAL(g)   (((1 << SIGNSIZE(g)) - 1))
#define SIGNMASK(g)     (SIGMAX_VAL(g) << SHCTSIZE(g))
#define SIGNP(g, i)     ((((i)->pc) & SIGNMASK(g)) >> SHCTSIZE(g))
#define SIGNM(g, i)     ((((i)->address) & SIGNMASK(g)) >> SHCTSIZE(g))
#define GET_SSIGN(g, i) ((i)->stream < TST ? SIGNM(g, i) : SIGNP(g, i))
#define GET_SIGN(g, i)  (SM(g) ? SIGNM(g, i) : SIGNP(g, i))  
#define SHIPSIGN(g, i)  (SB(g) ? GET_SSIGN(g, i) : GET_SIGN(g, i))

/* Update blocks for Color stream */
#define CACHE_UPDATE_BLOCK_SIGN(b, g, i) ((b)->ship_sign = SHIPSIGN(g, i))

#define CACHE_UPDATE_SHCT(block, g)                           \
do                                                            \
{                                                             \
  assert(block->ship_sign <= SIGMAX_VAL(g));                  \
                                                              \
  if (block->access)                                          \
  {                                                           \
    SAT_CTR_INC((g)->shct[block->ship_sign]);                 \
  }                                                           \
  else                                                        \
  {                                                           \
    SAT_CTR_DEC((g)->shct[block->ship_sign]);                 \
  }                                                           \
}while(0)

/*
 * Public Variables
 */

/*
 * Private Functions
 */
#define free_list_remove_block(set, blk)                                                    \
do                                                                                          \
{                                                                                           \
        /* Check: free list must be non empty as it contains the current block. */          \
        assert(SHIP_DATA_FREE_HEAD(set) && SHIP_DATA_FREE_HEAD(set));                 \
                                                                                            \
        /* Check: current block must be in invalid state */                                 \
        assert((blk)->state == cache_block_invalid);                                        \
                                                                                            \
        CACHE_REMOVE_FROM_SQUEUE(blk, SHIP_DATA_FREE_HEAD(set), SHIP_DATA_FREE_TAIL(set));\
                                                                                            \
        (blk)->next = NULL;                                                                 \
        (blk)->prev = NULL;                                                                 \
                                                                                            \
        /* Reset block state */                                                             \
        (blk)->busy = 0;                                                                    \
        (blk)->cached = 0;                                                                  \
        (blk)->prefetch = 0;                                                                \
}while(0);


int ship_cs_blocks = 0;

static void cache_init_gdata(struct cache_params *params, ship_gdata *global_data)
{
  global_data->shct_size  = params->ship_shct_size;   /* Signature history table size */
  global_data->sign_size  = params->ship_sig_size;    /* Signature size */
  global_data->entry_size = params->ship_entry_size;  /* Size of a counter */

  if (params->ship_use_pc && params->ship_use_mem)
  {
    global_data->sign_source = USE_MEMPC;
  }
  else
  {
    if (params->ship_use_pc)
    {
      global_data->sign_source = USE_PC;
    }
    else
    {
      global_data->sign_source = USE_MEM;
    }
  }
  
  assert(global_data->shct_size);
  assert(global_data->sign_size);
  assert(global_data->entry_size);
  assert(global_data->sign_source != USE_NN);

  /* Allocate counter table */
  global_data->shct = (sctr *)xcalloc((1 << global_data->shct_size), sizeof(sctr));
  assert(global_data->shct);

#define CTR_MIN_VAL (0)  
#define CTR_MAX_VAL ((1 << global_data->entry_size) - 1)

  /* Initialize counter table */ 
  for (ub4 i = 0; i < global_data->shct_size; i++)
  {
    SAT_CTR_INI(global_data->shct[i], global_data->entry_size, CTR_MIN_VAL, CTR_MAX_VAL);
  }

#undef CTR_MIN_VAL
#undef CTR_MAX_VAL
}

void cache_init_ship(ub4 set_idx, struct cache_params *params, 
    ship_data *policy_data, ship_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);
  
  /* Initialize global data */
  cache_init_gdata(params, global_data);

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  SHIP_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SHIP_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC  

  assert(SHIP_DATA_VALID_HEAD(policy_data));
  assert(SHIP_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SHIP_DATA_MAX_RRPV(policy_data) = MAX_RRPV;

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SHIP_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    SHIP_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    SHIP_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  SHIP_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

  SHIP_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(SHIP_DATA_FREE_HLST(policy_data));

  SHIP_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(SHIP_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  SHIP_DATA_FREE_HEAD(policy_data) = &(SHIP_DATA_BLOCKS(policy_data)[0]);
  SHIP_DATA_FREE_TAIL(policy_data) = &(SHIP_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SHIP_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SHIP_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SHIP_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SHIP_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SHIP_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SHIP_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }
  
  /* Set current and default fill policy to SHIP */
  SHIP_DATA_CFPOLICY(policy_data) = cache_policy_ship;
  SHIP_DATA_DFPOLICY(policy_data) = cache_policy_ship;
  SHIP_DATA_CAPOLICY(policy_data) = cache_policy_ship;
  SHIP_DATA_DAPOLICY(policy_data) = cache_policy_ship;
  SHIP_DATA_CRPOLICY(policy_data) = cache_policy_ship;
  SHIP_DATA_DRPOLICY(policy_data) = cache_policy_ship;
  
  assert(SHIP_DATA_MAX_RRPV(policy_data) != 0);

#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_ship(ship_data *policy_data)
{
  /* Free all data blocks */
  free(SHIP_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (SHIP_DATA_VALID_HEAD(policy_data))
  {
    free(SHIP_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SHIP_DATA_VALID_TAIL(policy_data))
  {
    free(SHIP_DATA_VALID_TAIL(policy_data));
  }
}

struct cache_block_t * cache_find_block_ship(ship_data *policy_data, long long tag)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SHIP_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);

      if (node->tag == tag)
        goto end;
    }
  }

end:
  return node;
}

void cache_fill_block_ship(ship_data *policy_data, 
    ship_gdata *global_data, int way, long long tag, 
    enum cache_block_state_t state, int strm, memory_trace *info)
{
  struct cache_block_t *block;
  int                   rrpv;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);
  
  /* Obtain SHIP specific data */
  block = &(SHIP_DATA_BLOCKS(policy_data)[way]);
  
  assert(block->stream == 0);

  /* Get RRPV to be assigned to the new block */
  rrpv = cache_get_fill_rrpv_ship(policy_data, global_data, info);

  /* If block is not bypassed */
  if (rrpv != BYPASS_RRPV)
  {
    /* Ensure a valid RRPV */
    assert(rrpv >= 0 && rrpv <= policy_data->max_rrpv); 

    /* Remove block from free list */
    free_list_remove_block(policy_data, block);

    /* Update new block state and stream */
    CACHE_UPDATE_BLOCK_STATE(block, tag, state);
    CACHE_UPDATE_BLOCK_STREAM(block, strm);
    CACHE_UPDATE_STREAM_BLOCKS(block, strm);
    CACHE_UPDATE_BLOCK_SIGN(block, global_data, info);

    if (info)
    {
      CACHE_UPDATE_BLOCK_PC(block, info->pc);
    }

    block->dirty  = (info && info->dirty) ? TRUE : FALSE;
    block->epoch  = 0;

    /* Insert block in to the corresponding RRPV queue */
    CACHE_APPEND_TO_QUEUE(block, 
        SHIP_DATA_VALID_HEAD(policy_data)[rrpv], 
        SHIP_DATA_VALID_TAIL(policy_data)[rrpv]);
  }

  SHIP_DATA_CFPOLICY(policy_data) = SHIP_DATA_DFPOLICY(policy_data);
}

int cache_replace_block_ship(ship_data *policy_data, ship_gdata *global_data)
{
  struct cache_block_t *block;

  int rrpv;

  /* Try to find an invalid block always from head of the free list. */
  for (block = SHIP_DATA_FREE_HEAD(policy_data); block; block = block->prev)
    return block->way;

  /* Obtain RRPV from where to replace the block */
  rrpv = cache_get_replacement_rrpv_ship(policy_data);
  
  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= SHIP_DATA_MAX_RRPV(policy_data));
  
  /* If there is no block with required RRPV, increment RRPV of all the blocks
   * until we get one with the required RRPV */
  if (SHIP_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
  {
    CACHE_SHIP_INCREMENT_RRPV(SHIP_DATA_VALID_HEAD(policy_data), 
      SHIP_DATA_VALID_TAIL(policy_data), rrpv);
  }
 
  /* Remove a nonbusy block from the tail */
  unsigned int min_wayid = ~(0);
  
  switch (SHIP_DATA_CRPOLICY(policy_data))
  {
    case cache_policy_ship:
      for (block = SHIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
      {
        if (!block->busy && block->way < min_wayid)
          min_wayid = block->way;
      }
      break;

    case cache_policy_cpulast:
      /* First try to find a GPU block */
      for (block = SHIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
      {
        if (!block->busy && (block->way < min_wayid && block->stream < TST))
          min_wayid = block->way;
      }
      
      /* If there so no GPU replacement candidate, replace CPU block */
      if (min_wayid == ~(0))
      {
        for (block = SHIP_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->next)
        {
          if (!block->busy && (block->way < min_wayid))
            min_wayid = block->way;
        }
      }
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }
  
  /* If a replacement has happeded, update signature counter table  */
  if (min_wayid != ~(0))
  {
    block = &(policy_data->blocks[min_wayid]);

    CACHE_UPDATE_SHCT(block, global_data);
  }

  /* If no non busy block can be found, return -1 */
  return (min_wayid != ~(0)) ? min_wayid : -1;
}

struct cache_block_t cache_access_block_ship(ship_data *policy_data, int way, int strm,
  memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;
  struct cache_block_t  ret_block;

  int old_rrpv;
  int new_rrpv;
  
  blk  = &(SHIP_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);
  
  /* Copy block data into return block */
  memcpy(&ret_block, blk, sizeof(struct cache_block_t));

  switch (SHIP_DATA_CAPOLICY(policy_data))
  {
    case cache_policy_ship:
      /* Update RRPV and epoch only for read hits */
      if (info)
      {
        /* Get old RRPV from the block */
        old_rrpv = (((rrip_list *)(blk->data))->rrpv);

        /* Get new RRPV using policy specific function */
        new_rrpv = cache_get_new_rrpv_ship(old_rrpv);

        /* Update block queue if block got new RRPV */
        if (new_rrpv != old_rrpv)
        {
          CACHE_REMOVE_FROM_QUEUE(blk, SHIP_DATA_VALID_HEAD(policy_data)[old_rrpv],
              SHIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
          CACHE_APPEND_TO_QUEUE(blk, SHIP_DATA_VALID_HEAD(policy_data)[new_rrpv], 
              SHIP_DATA_VALID_TAIL(policy_data)[new_rrpv]);
        }

        if (blk->stream == info->stream)
        {
          blk->epoch  = (blk->epoch == 3) ? 3 : blk->epoch + 1;
        }
        else
        {
          blk->epoch = 0;
        }

        if (info->dirty)
        {
          assert(info->spill);
          blk->dirty  = TRUE;
        }

        CACHE_UPDATE_BLOCK_PC(blk, info->pc);
        CACHE_UPDATE_BLOCK_ACCESS(blk);
        CACHE_UPDATE_BLOCK_STREAM(blk, strm);
        CACHE_UPDATE_STREAM_BLOCKS(blk, strm);
      }

      break;

    case cache_policy_bypass:
      break;

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
  }

  return ret_block;
}

int cache_get_fill_rrpv_ship(ship_data *policy_data, 
    ship_gdata *global_data, memory_trace *info)
{
  switch (SHIP_DATA_CFPOLICY(policy_data))
  {
    case cache_policy_lru:
      return 0;

    case cache_policy_lip:
      return SHIP_DATA_MAX_RRPV(policy_data);

    case cache_policy_ship:
      /* Get RRPV based on Ship signature history counter value */
      if (SAT_CTR_VAL(global_data->shct[SHIPSIGN(global_data, info)]))
      {
        return SHIP_DATA_MAX_RRPV(policy_data) - 1;
      }
      else
      {
        return SHIP_DATA_MAX_RRPV(policy_data);
      }
    
    case cache_policy_bypass:
      /* Not to insert */
      return BYPASS_RRPV;  

    default:
      panic("%s: line no %d - invalid policy type", __FUNCTION__, __LINE__);
      return 0;
  }
}

int cache_get_replacement_rrpv_ship(ship_data *policy_data)
{
  return SHIP_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_ship(int old_rrpv)
{
  return 0;
}

/* Update state of block. */
void cache_set_block_ship(ship_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  block = &(SHIP_DATA_BLOCKS(policy_data))[way];

  /* Check: tag matches with the block's tag. */
  assert(block->tag == tag);

  /* Check: block must be in valid state. It is not possible to set
   * state for an invalid block.*/
  assert(block->state != cache_block_invalid);

  if (state != cache_block_invalid)
  {
    /* Assign access stream */
    CACHE_UPDATE_BLOCK_STATE(block, tag, state);
    CACHE_UPDATE_BLOCK_STREAM(block, stream);
    CACHE_UPDATE_STREAM_BLOCKS(block, stream);
    return;
  }

  /* Invalidate block */
  CACHE_UPDATE_BLOCK_STATE(block, tag, cache_block_invalid);
  CACHE_UPDATE_BLOCK_STREAM(block, NN);
  CACHE_UPDATE_STREAM_BLOCKS(block, stream);
  block->epoch  = 0;

  /* Get old RRPV from the block */
  int old_rrpv = (((rrip_list *)(block->data))->rrpv);

  /* Remove block from valid list and insert into free list */
  CACHE_REMOVE_FROM_QUEUE(block, SHIP_DATA_VALID_HEAD(policy_data)[old_rrpv],
      SHIP_DATA_VALID_TAIL(policy_data)[old_rrpv]);
  CACHE_APPEND_TO_SQUEUE(block, SHIP_DATA_FREE_HEAD(policy_data), 
      SHIP_DATA_FREE_TAIL(policy_data));

}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_ship(ship_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  assert(policy_data);
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (SHIP_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (SHIP_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (SHIP_DATA_BLOCKS(policy_data)[way]).stream);

  return SHIP_DATA_BLOCKS(policy_data)[way];
}

/* Get tag and state of a block. */
int cache_count_block_ship(ship_data *policy_data, ub1 strm)
{
  int     max_rrpv;
  int     count;
  struct  cache_block_t *head;
  struct  cache_block_t *node;
  
  assert(policy_data);

  max_rrpv  = policy_data->max_rrpv;
  node      = NULL;
  count     = 0;

  for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
  {
    head = SHIP_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);
      if (node->stream == strm)
        count++;
    }
  }

  return count;
}

/* Set policy for next fill */
void cache_set_current_fill_policy_ship(ship_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  SHIP_DATA_CFPOLICY(policy_data) = policy;
}

/* Set policy for next access */
void cache_set_current_access_policy_ship(ship_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_lru || policy == cache_policy_lip || 
         policy == cache_policy_bypass);

  SHIP_DATA_CAPOLICY(policy_data) = policy;
}

/* Set policy for next replacment */
void cache_set_current_replacement_policy_ship(ship_data *policy_data, cache_policy_t policy)
{
  assert(policy == cache_policy_cpulast);

  SHIP_DATA_CRPOLICY(policy_data) = policy;
}

#undef SIGNSIZE
#undef SB
#undef SP
#undef SM
#undef SIGNP
#undef SIGNM
#undef GET_SSIGN
#undef GET_SIGN
#undef SHIPSIGN
