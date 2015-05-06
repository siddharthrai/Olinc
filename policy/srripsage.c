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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 */

#include <stdlib.h>
#include <assert.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "srripsage.h"
#include "sap.h"

#define SAMPLED_SET             (0)
#define THR_SAMPLED_SET         (0)
#define LRU_SAMPLED_SET         (1)
#define MRU_SAMPLED_SET         (2)
#define FOLLOWER_SET            (3)
#define CS_LRU_SAMPLED_SET      (4)
#define CS_MRU_SAMPLED_SET      (5)
#define ZS_LRU_SAMPLED_SET      (6)
#define ZS_MRU_SAMPLED_SET      (7)
#define TS_LRU_SAMPLED_SET      (8)
#define TS_MRU_SAMPLED_SET      (9)
#define BS_LRU_SAMPLED_SET      (10)
#define BS_MRU_SAMPLED_SET      (11)
#define PS_LRU_SAMPLED_SET      (12)
#define PS_MRU_SAMPLED_SET      (13)

#define CACHE_SET(cache, set)   (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)   (&((set)->blocks[way]))

#define BYPASS_RRPV             (-1)

#define PHASE_BITS              (15)

#define REUSE_TH                (1)
#define MIN_TH                  (1)
#define MAX_TH                  (16)

#define OP_HIT                  (0)
#define OP_FILL                 (1)
#define OP_DEMOTE               (2)

#define PSEL_WIDTH              (6)
#define PSEL_MIN_VAL            (0x00)
#define PSEL_MAX_VAL            (0x3f)
#define PSEL_MID_VAL            (32)

#define SET_BLCOKS(p)           ((p)->rrpv_blocks[0])

#define PHASE_SIZE              (5000)

#define IS_SAMPLED_SET(p)       ((p)->set_type == THR_SAMPLED_SET)
#define RCY_STHR(g, s)          ((g)->rcy_thr[SAMPLED_SET][(s)])
#define RCY_FTHR(g, s)          ((g)->rcy_thr[FOLLOWER_SET][(s)])
#define RCY_THR(p, g, s)        (IS_SAMPLED_SET(p) ? RCY_STHR(g, s) : RCY_FTHR(g, s))
#define OCC_STHR(g, s)          ((g)->occ_thr[SAMPLED_SET][(s)])
#define OCC_FTHR(g, s)          ((g)->occ_thr[FOLLOWER_SET][(s)])
#define OCC_THR(p, g, s)        (IS_SAMPLED_SET(p) ? OCC_STHR(g, s) : OCC_FTHR(g, s))

#define CACHE_UPDATE_BLOCK_STATE(block, tag, va, state_in)              \
do                                                                      \
{                                                                       \
  (block)->tag      = (tag);                                            \
  (block)->vtl_addr = (va);                                             \
  (block)->state    = (state_in);                                       \
}while(0)

/* Age block based on their LRU position for follower sets */
#define CACHE_SRRIPSAGE_AGEBLOCK_LRU(head_ptr, tail_ptr, rrpv,  g)    \
do                                                                    \
{                                                                     \
  int dif = 0;                                                        \
  int min_wayid = 0xff;                                               \
  struct cache_block_t *rpl_node = NULL;                              \
                                                                      \
  for (int i = rrpv - 1; i >= 0; i--)                                 \
  {                                                                   \
    assert(i <= rrpv);                                                \
                                                                      \
    if ((head_ptr)[i].head)                                           \
    {                                                                 \
      if (!dif)                                                       \
      {                                                               \
        dif = rrpv - i;                                               \
      }                                                               \
                                                                      \
      /* Move the block from current rrpv to new RRPV */              \
      if (i == 0)                                                     \
      {                                                               \
        struct cache_block_t *node = (head_ptr)[i].head;              \
                                                                      \
        if (node)                                                     \
        {                                                             \
            rpl_node = node;                                          \
                                                                      \
            assert(rpl_node);                                         \
                                                                      \
            CACHE_REMOVE_FROM_QUEUE(rpl_node, head_ptr[i], tail_ptr[i]);  \
                                                                      \
            rpl_node->data = &(head_ptr[i + dif]);                    \
            rpl_node->demote = TRUE;                                  \
                                                                      \
            cache_add_reuse_blocks((g), OP_DEMOTE, rpl_node->stream); \
                                                                      \
            CACHE_PREPEND_TO_QUEUE(rpl_node, head_ptr[i + dif], tail_ptr[i + dif]);\
        }                                                             \
      }                                                               \
      else                                                            \
      {                                                               \
        struct cache_block_t *node = (tail_ptr)[i].head;              \
                                                                      \
        if (node)                                                     \
        {                                                             \
            rpl_node = node;                                          \
                                                                      \
            assert(rpl_node);                                         \
                                                                      \
            CACHE_REMOVE_FROM_QUEUE(rpl_node, (head_ptr)[i], (tail_ptr)[i]);\
                                                                      \
            rpl_node->data = &(head_ptr[i + dif]);                    \
            rpl_node->demote = TRUE;                                  \
                                                                      \
            cache_add_reuse_blocks((g), OP_DEMOTE, rpl_node->stream); \
                                                                      \
            CACHE_PREPEND_TO_QUEUE(rpl_node, (head_ptr)[i + dif], (tail_ptr)[i + dif]);\
        }                                                             \
      }                                                               \
    }                                                                 \
    else                                                              \
    {                                                                 \
      assert(!((tail_ptr)[i].head));                                  \
    }                                                                 \
  }                                                                   \
}while(0)

/* Age block based on their LRU position for follower sets */
#define CACHE_SRRIPSAGE_AGEBLOCK_MRU(head_ptr, tail_ptr, rrpv, g)     \
do                                                                    \
{                                                                     \
  int dif = 0;                                                        \
  int min_wayid = 0xff;                                               \
  struct cache_block_t *rpl_node = NULL;                              \
                                                                      \
  for (int i = rrpv - 1; i >= 0; i--)                                 \
  {                                                                   \
    assert(i <= rrpv);                                                \
                                                                      \
    if ((head_ptr)[i].head)                                           \
    {                                                                 \
      if (!dif)                                                       \
      {                                                               \
        dif = rrpv - i;                                               \
      }                                                               \
                                                                      \
      /* Move the block from current rrpv to new RRPV */              \
      if (i == 0)                                                     \
      {                                                               \
        struct cache_block_t *node = (head_ptr)[i].head;              \
                                                                      \
        if (node)                                                     \
        {                                                             \
            rpl_node = node;                                          \
                                                                      \
            assert(rpl_node);                                         \
                                                                      \
            CACHE_REMOVE_FROM_QUEUE(rpl_node, head_ptr[i], tail_ptr[i]);  \
                                                                      \
            rpl_node->data = &(head_ptr[i + dif]);                    \
            rpl_node->demote = TRUE;                                  \
                                                                      \
            cache_add_reuse_blocks((g), OP_DEMOTE, rpl_node->stream); \
                                                                      \
            CACHE_APPEND_TO_QUEUE(rpl_node, head_ptr[i + dif], tail_ptr[i + dif]);\
        }                                                             \
      }                                                               \
      else                                                            \
      {                                                               \
        struct cache_block_t *node = (tail_ptr)[i].head;              \
                                                                      \
        if (node)                                                     \
        {                                                             \
            rpl_node = node;                                          \
                                                                      \
            assert(rpl_node);                                         \
                                                                      \
            CACHE_REMOVE_FROM_QUEUE(rpl_node, (head_ptr)[i], (tail_ptr)[i]);\
                                                                      \
            rpl_node->data = &(head_ptr[i + dif]);                    \
            rpl_node->demote = TRUE;                                  \
                                                                      \
            cache_add_reuse_blocks((g), OP_DEMOTE, rpl_node->stream); \
                                                                      \
            CACHE_APPEND_TO_QUEUE(rpl_node, (head_ptr)[i + dif], (tail_ptr)[i + dif]);\
        }                                                             \
      }                                                               \
    }                                                                 \
    else                                                              \
    {                                                                 \
      assert(!((tail_ptr)[i].head));                                  \
    }                                                                 \
  }                                                                   \
}while(0)

/* Age block based on their LRU position for follower sets */
#define CACHE_SRRIPSAGE_AGEBLOCK_FOLLOWER(head_ptr, tail_ptr, rrpv, p, g)\
do                                                                    \
{                                                                     \
  int dif = 0;                                                        \
  int min_wayid = 0xff;                                               \
  struct cache_block_t *rpl_node = NULL;                              \
                                                                      \
  for (int i = rrpv - 1; i >= 0; i--)                                 \
  {                                                                   \
    assert(i <= rrpv);                                                \
                                                                      \
    if ((head_ptr)[i].head)                                           \
    {                                                                 \
      if (!dif)                                                       \
      {                                                               \
        dif = rrpv - i;                                               \
      }                                                               \
                                                                      \
      /* Move the block from current rrpv to new RRPV */              \
      if (i == 0)                                                     \
      {                                                               \
        struct cache_block_t *node = (head_ptr)[i].head;              \
                                                                      \
        if (node)                                                     \
        {                                                             \
            rpl_node = node;                                          \
                                                                      \
            assert(rpl_node);                                         \
                                                                      \
            CACHE_REMOVE_FROM_QUEUE(rpl_node, head_ptr[i], tail_ptr[i]);  \
                                                                      \
            rpl_node->data = &(head_ptr[i + dif]);                    \
            rpl_node->demote = TRUE;                                  \
                                                                      \
            CACHE_APPEND_TO_QUEUE(rpl_node, head_ptr[i + dif], tail_ptr[i + dif]);\
        }                                                             \
      }                                                               \
      else                                                            \
      {                                                               \
        struct cache_block_t *node = (tail_ptr)[i].head;              \
                                                                      \
        if (node)                                                     \
        {                                                             \
          rpl_node = node;                                            \
                                                                      \
          assert(rpl_node);                                           \
                                                                      \
          CACHE_REMOVE_FROM_QUEUE(rpl_node, (head_ptr)[i], (tail_ptr)[i]);\
                                                                      \
          rpl_node->data    = &(head_ptr[i + dif]);                   \
          rpl_node->demote  = TRUE;                                   \
                                                                      \
          if ((g)->demote_at_head[rpl_node->stream] == FALSE)         \
          {                                                           \
            CACHE_APPEND_TO_QUEUE(rpl_node, (head_ptr)[i + dif], (tail_ptr)[i + dif]);\
          }                                                           \
          else                                                        \
          {                                                           \
            CACHE_PREPEND_TO_QUEUE(rpl_node, (head_ptr)[i + dif], (tail_ptr)[i + dif]);\
          }                                                           \
        }                                                             \
      }                                                               \
    }                                                                 \
    else                                                              \
    {                                                                 \
      assert(!((tail_ptr)[i].head));                                  \
    }                                                                 \
  }                                                                   \
}while(0)

#define CACHE_GET_BLOCK_STREAM(block, strm)                       \
do                                                                \
{                                                                 \
  strm = (block)->stream;                                         \
}while(0)

#define CACHE_UPDATE_BLOCK_STREAM(block, strm)                    \
do                                                                \
{                                                                 \
  if ((block)->stream == CS && strm != CS)                        \
  {                                                               \
    srripsage_blocks--;                                           \
  }                                                               \
                                                                  \
  if (strm == CS && (block)->stream != CS)                        \
  {                                                               \
    srripsage_blocks++;                                           \
  }                                                               \
                                                                  \
  (block)->stream = strm;                                         \
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
        assert(SRRIPSAGE_DATA_FREE_HEAD(set) && SRRIPSAGE_DATA_FREE_HEAD(set));             \
                                                                                            \
        /* Check: current block must be in invalid state */                                 \
        assert((blk)->state == cache_block_invalid);                                        \
                                                                                            \
        CACHE_REMOVE_FROM_SQUEUE(blk, SRRIPSAGE_DATA_FREE_HEAD(set), SRRIPSAGE_DATA_FREE_TAIL(set));\
                                                                                            \
        (blk)->next = NULL;                                                                 \
        (blk)->prev = NULL;                                                                 \
                                                                                            \
        /* Reset block state */                                                             \
        (blk)->busy = 0;                                                                    \
        (blk)->cached = 0;                                                                  \
        (blk)->prefetch = 0;                                                                \
}while(0);

int srripsage_blocks = 0;

static ub1 get_set_type_srripsage(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x07;
  msb_bits = (indx >> 7) & 0x07;
  mid_bits = (indx >> 3) & 0x0f;

  if (lsb_bits == msb_bits && mid_bits == 0x00)
  {
    return PS_LRU_SAMPLED_SET;
  }

  if ((~lsb_bits & 0x07) == msb_bits && mid_bits == 0x00)
  {
    return PS_MRU_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 0x01)
  {
    return THR_SAMPLED_SET;
  }

  if ((~lsb_bits & 0x07) == msb_bits && mid_bits == 0x01)
  {
    return CS_LRU_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 0x02)
  {
    return CS_MRU_SAMPLED_SET;
  }

  if ((~lsb_bits & 0x07) == msb_bits && mid_bits == 0x02)
  {
    return ZS_LRU_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 0x03)
  {
    return ZS_MRU_SAMPLED_SET;
  }

  if ((~lsb_bits & 0x07) == msb_bits && mid_bits == 0x03)
  {
    return TS_LRU_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 0x04)
  {
    return TS_MRU_SAMPLED_SET;
  }

  if ((~lsb_bits & 0x07) == msb_bits && mid_bits == 0x04)
  {
    return BS_LRU_SAMPLED_SET;
  }

  if (lsb_bits == msb_bits && mid_bits == 0x05)
  {
    return BS_MRU_SAMPLED_SET;
  }

  return FOLLOWER_SET;
}

#if 0
static ub1 get_set_type_srripsage(long long int indx)
{
  int lsb_bits;
  int msb_bits;
  int mid_bits;

  lsb_bits = indx & 0x0f;
  msb_bits = (indx >> 6) & 0x0f;
  mid_bits = (indx >> 4) & 0x03;

  if (lsb_bits == msb_bits && mid_bits == 0x00)
  {
    return PS_LRU_SAMPLED_SET;
  }
  else
  {
    if ((~lsb_bits & 0x0f) == msb_bits && mid_bits == 0x00)
    {
      return PS_MRU_SAMPLED_SET;
    }
    else
    {
      if (lsb_bits == msb_bits && mid_bits == 0x01)
      {
        return THR_SAMPLED_SET;
      }
      else
      {
        if ((~lsb_bits & 0x0f) == msb_bits && mid_bits == 0x01)
        {
          return CS_LRU_SAMPLED_SET;
        }
        else
        {
          if (lsb_bits == msb_bits && mid_bits == 0x02)
          {
            return CS_MRU_SAMPLED_SET;
          }
          else
          {
            if ((~lsb_bits & 0x0f) == msb_bits && mid_bits == 0x02)
            {
              return ZS_LRU_SAMPLED_SET;
            }
            else
            {
              if (lsb_bits == msb_bits && mid_bits == 0x03)
              {
                return ZS_MRU_SAMPLED_SET;
              }
              else
              {
                if ((~lsb_bits & 0x0f) == msb_bits && mid_bits == 0x03)
                {
                  return TS_LRU_SAMPLED_SET;
                }
                else
                {
                  if ((lsb_bits ^ 0xa) == msb_bits && mid_bits == 0x00)
                  {
                    return TS_MRU_SAMPLED_SET;
                  }
                  else
                  {
                    if ((lsb_bits ^ 0xa) == msb_bits && mid_bits == 0x01)
                    {
                      return BS_LRU_SAMPLED_SET;
                    }
                    else
                    {
                      if ((lsb_bits ^ 0xa) == msb_bits && mid_bits == 0x02)
                      {
                        return BS_MRU_SAMPLED_SET;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return FOLLOWER_SET;
}
#endif

/* Update reused blocks at RRPV 0 */
void cache_add_reuse_blocks(srripsage_gdata *global_data, ub1 op,
    ub1 stream)
{
  assert(op == OP_FILL || op == OP_HIT || op == OP_DEMOTE);

  switch (op)
  {
    case OP_HIT:
      global_data->per_stream_reuse_blocks[stream]++;
      break;

    case OP_FILL:
      global_data->per_stream_oreuse_blocks[stream]++;
      break;

    case OP_DEMOTE:
      global_data->per_stream_dreuse_blocks[stream]++;
      break;

    default:
      printf("Unknown op %s : %d\n", __FUNCTION__, __LINE__);
      assert(0);
  }
}

/* Update reused blocks at RRPV 0 */
void cache_remove_reuse_blocks(srripsage_gdata *global_data, ub1 op,
    ub1 stream)
{
  assert(op == OP_FILL || op == OP_HIT || op == OP_DEMOTE);
  
  switch (op)
  {
    case OP_HIT:
      if(global_data->per_stream_reuse_blocks[stream])
      {
        global_data->per_stream_reuse_blocks[stream]--;
      }
      break;

    case OP_FILL:
      if(global_data->per_stream_oreuse_blocks[stream])
      {
        global_data->per_stream_oreuse_blocks[stream]--;
      }
      break;

    case OP_DEMOTE:
      if(global_data->per_stream_dreuse_blocks[stream])
      {
        global_data->per_stream_dreuse_blocks[stream]--;
      }
      break;

    default:
      printf("Unknown op %s : %d\n", __FUNCTION__, __LINE__);
      assert(0);
  }
}

/* Update reuse count of block at RRPV 0 */
static void cache_update_reuse(srripsage_gdata *global_data, ub1 op,
    ub1 stream)
{
  assert(op == OP_FILL || op == OP_HIT || op == OP_DEMOTE);
  
  switch (op)
  {
    case OP_HIT:
      global_data->per_stream_reuse[stream]++;
      break;

    case OP_FILL:
      global_data->per_stream_oreuse[stream]++;
      break;

    case OP_DEMOTE:
      global_data->per_stream_dreuse[stream]++;
      break;

    default:
      printf("Unknown op %s : %d\n", __FUNCTION__, __LINE__);
      assert(0);
  }
}

void cache_init_srripsage(int set_indx, struct cache_params *params, 
    srripsage_data *policy_data, srripsage_gdata *global_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

  /* For RRIP blocks are organized in per RRPV list */
#define MAX_RRPV        (params->max_rrpv)
#define MEM_ALLOC(size) ((rrip_list *)xcalloc(size, sizeof(rrip_list)))

  /* Create per rrpv buckets */
  SRRIPSAGE_DATA_VALID_HEAD(policy_data) = MEM_ALLOC(MAX_RRPV + 1);
  SRRIPSAGE_DATA_VALID_TAIL(policy_data) = MEM_ALLOC(MAX_RRPV + 1);

#undef MEM_ALLOC  

  assert(SRRIPSAGE_DATA_VALID_HEAD(policy_data));
  assert(SRRIPSAGE_DATA_VALID_TAIL(policy_data));

  /* Set max RRPV for the set */
  SRRIPSAGE_DATA_MAX_RRPV(policy_data)    = MAX_RRPV;
  SRRIPSAGE_DATA_SPILL_RRPV(policy_data)  = params->spill_rrpv;

  assert(params->spill_rrpv <= MAX_RRPV);

  /* Initialize head nodes */
  for (int i = 0; i <= MAX_RRPV; i++)
  {
    SRRIPSAGE_DATA_VALID_HEAD(policy_data)[i].rrpv = i;
    SRRIPSAGE_DATA_VALID_HEAD(policy_data)[i].head = NULL;
    SRRIPSAGE_DATA_VALID_TAIL(policy_data)[i].head = NULL;
  }

  /* Create array of blocks */
  SRRIPSAGE_DATA_BLOCKS(policy_data) = 
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));

#define MEM_ALLOC(size) ((list_head_t *)xcalloc(size, sizeof(list_head_t)))

  SRRIPSAGE_DATA_FREE_HLST(policy_data) = MEM_ALLOC(1);
  assert(SRRIPSAGE_DATA_FREE_HLST(policy_data));

  SRRIPSAGE_DATA_FREE_TLST(policy_data) = MEM_ALLOC(1);
  assert(SRRIPSAGE_DATA_FREE_TLST(policy_data));

#undef MEM_ALLOC

  /* Initialize array of blocks */
  SRRIPSAGE_DATA_FREE_HEAD(policy_data) = &(SRRIPSAGE_DATA_BLOCKS(policy_data)[0]);
  SRRIPSAGE_DATA_FREE_TAIL(policy_data) = &(SRRIPSAGE_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&SRRIPSAGE_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&SRRIPSAGE_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&SRRIPSAGE_DATA_BLOCKS(policy_data)[way])->next  = way ?
      (&SRRIPSAGE_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&SRRIPSAGE_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&SRRIPSAGE_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
  }

  if (set_indx == 0)
  {
    /* Fill and hit counter to get reuse probability */
    SAT_CTR_INI(global_data->tex_e0_fill_ctr, 8, 0, 255);  /* Texture epoch 0 fill */
    SAT_CTR_INI(global_data->tex_e0_hit_ctr, 8, 0, 255);   /* Texture epoch 0 hits */

    SAT_CTR_INI(global_data->tex_e1_fill_ctr, 8, 0, 255);  /* Texture epoch 1 fill */
    SAT_CTR_INI(global_data->tex_e1_hit_ctr, 8, 0, 255);   /* Texture epoch 1 hits */

    global_data->bm_ctr         = 0;
    global_data->bm_thr         = params->threshold;
    global_data->access_count   = 0;
    global_data->active_stream  = NN;

    /* Set default value for each stream recency threshold */
    for (int strm = 0; strm <= TST; strm++)
    {
      global_data->rcy_thr[SAMPLED_SET][strm]   = 1;
      global_data->rcy_thr[FOLLOWER_SET][strm]  = 1;
      global_data->occ_thr[SAMPLED_SET][strm]   = 1;
      global_data->occ_thr[FOLLOWER_SET][strm]  = 1;
      global_data->per_stream_cur_thr[strm]     = 1;
      global_data->per_stream_occ_thr[strm]     = 1;
      global_data->fill_at_head[strm]           = FALSE;
      global_data->demote_on_hit[strm]          = FALSE;
    }

    /* Override value for each Color and Depth */
    global_data->rcy_thr[SAMPLED_SET][CS]   = global_data->per_stream_cur_thr[CS];
    global_data->rcy_thr[SAMPLED_SET][ZS]   = global_data->per_stream_cur_thr[ZS];
    global_data->rcy_thr[SAMPLED_SET][TS]   = global_data->per_stream_cur_thr[TS];
    global_data->rcy_thr[SAMPLED_SET][BS]   = global_data->per_stream_cur_thr[BS];
    global_data->rcy_thr[SAMPLED_SET][PS]   = global_data->per_stream_cur_thr[PS];

    /* Override value for each Color and Depth */
    global_data->rcy_thr[FOLLOWER_SET][CS]  = 1;
    global_data->rcy_thr[FOLLOWER_SET][ZS]  = 1;
    global_data->rcy_thr[FOLLOWER_SET][TS]  = 1;
    global_data->rcy_thr[FOLLOWER_SET][BS]  = 1;
    global_data->rcy_thr[FOLLOWER_SET][PS]  = 1;

    /* Override value of occupancy threshold */
    global_data->occ_thr[SAMPLED_SET][CS]   = global_data->per_stream_occ_thr[CS];
    global_data->occ_thr[SAMPLED_SET][ZS]   = global_data->per_stream_occ_thr[ZS];
    global_data->occ_thr[SAMPLED_SET][TS]   = global_data->per_stream_occ_thr[TS];
    global_data->occ_thr[SAMPLED_SET][BS]   = global_data->per_stream_occ_thr[BS];
    global_data->occ_thr[SAMPLED_SET][PS]   = global_data->per_stream_occ_thr[PS];

    /* Override value of occupancy threshold */
    global_data->occ_thr[FOLLOWER_SET][CS]  = 1;
    global_data->occ_thr[FOLLOWER_SET][ZS]  = 1;
    global_data->occ_thr[FOLLOWER_SET][TS]  = 1;
    global_data->occ_thr[FOLLOWER_SET][BS]  = 1;
    global_data->occ_thr[FOLLOWER_SET][PS]  = 1;

    memset(global_data->per_stream_fill, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_hit, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_sevct, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_xevct, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_rrpv_hit, 0, sizeof(ub8) * (TST + 1) * 4);
    memset(global_data->per_stream_demote, 0, sizeof(ub8) * (TST + 1) * 4);
    memset(global_data->fills_at_head, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->dems_at_head, 0, sizeof(ub8) * (TST + 1));

    /* Reset reuse data used for learning for reuses at RRPV 0 */
    memset(global_data->per_stream_reuse_blocks, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_reuse, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_max_reuse, 0, sizeof(ub8) * (TST + 1));

    /* Reset reuse data used for learning for reuses at RRPV 2 */
    memset(global_data->per_stream_oreuse_blocks, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_oreuse, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_max_oreuse, 0, sizeof(ub8) * (TST + 1));

    /* Reset reuse data used for learning for reuses of demote blocks */
    memset(global_data->per_stream_dreuse_blocks, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_dreuse, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_max_dreuse, 0, sizeof(ub8) * (TST + 1));

    /* Alocate per RRPV block count table */
    global_data->rrpv_blocks = xcalloc(SRRIPSAGE_DATA_MAX_RRPV(policy_data) + 1, sizeof(ub8));
    assert(global_data->rrpv_blocks);

    memset(global_data->rrpv_blocks, 0, sizeof(ub8) * SRRIPSAGE_DATA_MAX_RRPV(policy_data) + 1);

    /* Initialize fill policy saturating counter */
    for (ub1 i = 0; i <= TST; i++)
    {
      SAT_CTR_INI(global_data->fath_ctr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_SET(global_data->fath_ctr[i], PSEL_MID_VAL);

      SAT_CTR_INI(global_data->fathm_ctr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_SET(global_data->fathm_ctr[i], PSEL_MID_VAL);

      SAT_CTR_INI(global_data->dem_ctr[i], PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
      SAT_CTR_SET(global_data->dem_ctr[i], PSEL_MID_VAL);

      memset(&(global_data->lru_sample_access[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->lru_sample_hit[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->lru_sample_dem[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->mru_sample_access[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->mru_sample_hit[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->mru_sample_dem[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->thr_sample_access[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->thr_sample_hit[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->thr_sample_dem[i]), 0, sizeof(ub8) * (TST + 1));
    }

    SAT_CTR_INI(global_data->gfath_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->gfath_ctr, PSEL_MID_VAL);

    SAT_CTR_INI(global_data->gdem_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->gdem_ctr, PSEL_MID_VAL);

    SAT_CTR_INI(global_data->gfathm_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_SET(global_data->gfathm_ctr, PSEL_MID_VAL);

    SAT_CTR_INI(global_data->cb_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_RST(global_data->cb_ctr);

    SAT_CTR_INI(global_data->bc_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_RST(global_data->bc_ctr);

    SAT_CTR_INI(global_data->ct_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_RST(global_data->ct_ctr);

    SAT_CTR_INI(global_data->bt_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_RST(global_data->bt_ctr);

    SAT_CTR_INI(global_data->tb_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_RST(global_data->tb_ctr);

    SAT_CTR_INI(global_data->zt_ctr, PSEL_WIDTH, PSEL_MIN_VAL, PSEL_MAX_VAL);
    SAT_CTR_RST(global_data->zt_ctr);

    global_data->lru_sample_set = 0;
    global_data->mru_sample_set = 0;
  }

  /* Alocate per RRPV block count table */
  SRRIPSAGE_DATA_RRPV_BLCKS(policy_data) = xcalloc(SRRIPSAGE_DATA_MAX_RRPV(policy_data) + 1, sizeof(ub1));
  assert(SRRIPSAGE_DATA_RRPV_BLCKS(policy_data));

  memset(SRRIPSAGE_DATA_RRPV_BLCKS(policy_data), 0, sizeof(ub1) * SRRIPSAGE_DATA_MAX_RRPV(policy_data) + 1);
  
  ub1 set_type;

  /* Set policy based on chosen sets */
  set_type = get_set_type_srripsage(set_indx);

  if (set_type == CS_LRU_SAMPLED_SET || set_type == TS_LRU_SAMPLED_SET || 
      set_type == ZS_LRU_SAMPLED_SET || set_type == BS_LRU_SAMPLED_SET ||
      set_type == PS_LRU_SAMPLED_SET )
  {
    /* Initialize srrip policy for the set */
    cache_init_srrip(params, &(policy_data->srrip_policy_data));

    policy_data->following  = cache_policy_srrip;

    global_data->lru_sample_set +=1;
  }
  else
  {
    policy_data->following = cache_policy_srripsage;

    if (set_type == CS_MRU_SAMPLED_SET || set_type == TS_MRU_SAMPLED_SET ||
        set_type == ZS_MRU_SAMPLED_SET || set_type == BS_MRU_SAMPLED_SET || 
        set_type == PS_MRU_SAMPLED_SET)
    {
      global_data->mru_sample_set +=1;
    }
    else
    {
      if (set_type == THR_SAMPLED_SET)
      {
        global_data->thr_sample_set += 1;
      }
    }
  }

  /* Initialize LRU fill and demote vector. By default nothing goes to 
   * LRU. Based on sample type, selected stream is set, so that it fills
   * or demotes to head. */ 

  memset(policy_data->fill_at_lru, 0, sizeof(ub1) * (TST + 1));
  memset(policy_data->dem_to_lru, 0, sizeof(ub1) * (TST + 1));
  
  switch (set_type)
  {
    case CS_LRU_SAMPLED_SET:
      policy_data->set_type         = LRU_SAMPLED_SET;
      policy_data->fill_at_lru[CS]  = TRUE;
      break;

    case CS_MRU_SAMPLED_SET:
      policy_data->set_type         = MRU_SAMPLED_SET;
      policy_data->dem_to_lru[CS]   = TRUE;
      break;

    case ZS_LRU_SAMPLED_SET:
      policy_data->set_type         = LRU_SAMPLED_SET;
      policy_data->fill_at_lru[ZS]  = TRUE;
      break;

    case ZS_MRU_SAMPLED_SET:
      policy_data->set_type         = MRU_SAMPLED_SET;
      policy_data->dem_to_lru[ZS]   = TRUE;
      break;

    case TS_LRU_SAMPLED_SET:
      policy_data->set_type         = LRU_SAMPLED_SET;
      policy_data->fill_at_lru[TS]  = TRUE;
      break;

    case TS_MRU_SAMPLED_SET:
      policy_data->set_type         = MRU_SAMPLED_SET;
      policy_data->dem_to_lru[TS]   = TRUE;
      break;

    case BS_LRU_SAMPLED_SET:
      policy_data->set_type         = LRU_SAMPLED_SET;
      policy_data->fill_at_lru[BS]  = TRUE;
      break;

    case BS_MRU_SAMPLED_SET:
      policy_data->set_type         = MRU_SAMPLED_SET;
      policy_data->dem_to_lru[BS]   = TRUE;
      break;

    case PS_LRU_SAMPLED_SET:
      policy_data->set_type         = LRU_SAMPLED_SET;
      policy_data->fill_at_lru[PS]  = TRUE;
      break;

    case PS_MRU_SAMPLED_SET:
      policy_data->set_type         = MRU_SAMPLED_SET;
      policy_data->dem_to_lru[PS]   = TRUE;
      break;

    case FOLLOWER_SET:
      policy_data->set_type = FOLLOWER_SET;
      break;

    case THR_SAMPLED_SET:
      policy_data->set_type = THR_SAMPLED_SET;
      break;

    default:
      printf("Invalid set type %d - %s : %d\n", set_type, __FUNCTION__, __LINE__);  
  }

  /* Reset per-set fill counter */
  memset(policy_data->per_stream_fill, 0, sizeof(ub8) * (TST + 1));

  /* Reset all hit_post_fill bits */
  memset(policy_data->hit_post_fill, 0, sizeof(ub1) * (TST + 1));

  assert(SRRIPSAGE_DATA_MAX_RRPV(policy_data) != 0);

#undef MAX_RRPV
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_srripsage(srripsage_data *policy_data)
{
  /* Free all data blocks */
  free(SRRIPSAGE_DATA_BLOCKS(policy_data));

  /* Free valid head buckets */
  if (SRRIPSAGE_DATA_VALID_HEAD(policy_data))
  {
    free(SRRIPSAGE_DATA_VALID_HEAD(policy_data));
  }

  /* Free valid tail buckets */
  if (SRRIPSAGE_DATA_VALID_TAIL(policy_data))
  {
    free(SRRIPSAGE_DATA_VALID_TAIL(policy_data));
  }

  /* Free SRRIP specific information */
  if (policy_data->following == cache_policy_srrip)
  {
    cache_free_srrip(&(policy_data->srrip_policy_data));
  }
}

struct cache_block_t* cache_find_block_srripsage(srripsage_data *policy_data, 
    srripsage_gdata *global_data, long long tag, int strm, memory_trace *info)
{
  int     max_rrpv;
  struct  cache_block_t *head;
  struct  cache_block_t *node;
  enum    cache_block_state_t state_ptr;
  
  int stream_ptr;
  int min_wayid;
  int i;

  long long tag_ptr;

  ub8 reuse;
  ub8 reuse_blocks;

  /* Increment per-stream fills */
  global_data->per_stream_fill[strm]++;
  policy_data->per_stream_fill[strm]++;
  
  if ((global_data->bm_ctr)++ == global_data->bm_thr)
  {
    global_data->bm_ctr = 0;
  }
  
  if (policy_data->set_type == LRU_SAMPLED_SET)
  {
    global_data->lru_sample_access[strm]++;
  }
  else
  {
    if (policy_data->set_type == MRU_SAMPLED_SET)
    {
      global_data->mru_sample_access[strm]++;
    }
    else
    {
      if (policy_data->set_type == THR_SAMPLED_SET)
      {
        global_data->thr_sample_access[strm]++;
      }
    }
  }

  if (policy_data->following == cache_policy_srrip)
  {
    node = cache_find_block_srrip(&(policy_data->srrip_policy_data), tag);
    if (node)
    {
      cache_get_block_srrip(&(policy_data->srrip_policy_data), node->way, 
          &tag_ptr, &state_ptr, &stream_ptr);

      /* Follow SRRIP policy */
      cache_access_block_srrip(&(policy_data->srrip_policy_data), node->way, strm, 
          info);
    }
    else
    {
      min_wayid = cache_replace_block_srrip(&(policy_data->srrip_policy_data));

      assert(min_wayid != -1);

      cache_get_block_srrip(&(policy_data->srrip_policy_data), min_wayid, 
          &tag_ptr, &state_ptr, &stream_ptr);

      if (state_ptr != cache_block_invalid)
      {
        cache_set_block_srrip(&(policy_data->srrip_policy_data), min_wayid,
            tag_ptr, cache_block_invalid, stream_ptr, info);
      }
      
      if (policy_data->blocks[min_wayid].demote)
      {
        cache_remove_reuse_blocks(global_data, OP_DEMOTE, info->stream);  
      }

      /* Follow RRIP policy */
      cache_fill_block_srrip(&(policy_data->srrip_policy_data), min_wayid, tag, 
          cache_block_exclusive, strm, info);

    }
  }

  {
    max_rrpv  = policy_data->max_rrpv;
    node      = NULL;

    for (int rrpv = 0; rrpv <= max_rrpv; rrpv++)
    {
      head = SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv].head;

      for (node = head; node; node = node->prev)
      {
        assert(node->state != cache_block_invalid);

        if (node->tag == tag)
          goto end;
      }
    }
  }

end:

  if (++(global_data->access_count) == PHASE_SIZE)
  {
    /* Update fill at occupancy list head flag */
    printf("CB:%d BC:%d CT:%d BT: %d TB:%d ZT: %d\n", SAT_CTR_VAL(global_data->cb_ctr),
        SAT_CTR_VAL(global_data->cb_ctr), SAT_CTR_VAL(global_data->ct_ctr), 
        SAT_CTR_VAL(global_data->bt_ctr), SAT_CTR_VAL(global_data->tb_ctr), 
        SAT_CTR_VAL(global_data->zt_ctr));

    /* Update fill at occupancy list head flag */
    printf("LRUSS:%d MRUSS: %d THRSS: %d\n", global_data->lru_sample_set, 
        global_data->mru_sample_set, global_data->thr_sample_set);

    printf("LACC-C:%d LACC-Z: %d LACC-T: %d LACC-B: %d LACC-P: %d\n", global_data->lru_sample_access[CS], 
        global_data->lru_sample_access[ZS], global_data->lru_sample_access[TS], 
        global_data->lru_sample_access[BS], global_data->lru_sample_access[PS]);

    printf("MACC-C:%ld MACC-Z: %ld MACC-T: %ld MACC-B: %ld MACC-P: %ld\n", global_data->mru_sample_access[CS], 
        global_data->mru_sample_access[ZS], global_data->mru_sample_access[TS], 
        global_data->mru_sample_access[BS], global_data->mru_sample_access[PS]);

    printf("LHIT-C:%d LHIT-Z: %d LHIT-T: %d LHIT-B: %d LHIT-P: %d\n", global_data->lru_sample_hit[CS], 
        global_data->lru_sample_hit[ZS], global_data->lru_sample_hit[TS], 
        global_data->lru_sample_hit[BS], global_data->lru_sample_hit[PS]);

    printf("MHIT-C:%d MHIT-Z: %d MHIT-T: %d MHIT-B: %d MHIT-P: %d\n", global_data->mru_sample_hit[CS], 
        global_data->mru_sample_hit[ZS], global_data->mru_sample_hit[TS], 
        global_data->mru_sample_hit[BS], global_data->mru_sample_hit[PS]);
    
    ub4 lru_hr[TST + 1];
    ub4 mru_hr[TST + 1];

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        reuse_blocks  = global_data->lru_sample_access[i];
        reuse         = global_data->lru_sample_hit[i];
        lru_hr[i]     = 0;

        if (reuse_blocks)
        {
          lru_hr[i] = (reuse * 100) / reuse_blocks;
        }

        printf("LHR:%d:%ld ", i, lru_hr[i]);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        reuse_blocks  = global_data->mru_sample_access[i];
        reuse         = global_data->mru_sample_hit[i];
        mru_hr[i]     = 0; 

        if (reuse_blocks)
        {
          mru_hr[i] = (reuse * 100) / reuse_blocks;
        }

        printf("MHR:%d:%ld ", i, mru_hr[i]);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {

        printf("FATH:%d:%ld ", i, global_data->fill_at_head[i]);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {

        printf("DONH:%d:%ld ", i, global_data->demote_on_hit[i]);
      }
    }

    printf("\n");

    ub8 demoted;

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        demoted = global_data->per_stream_dreuse_blocks[i];

        printf("DBLKS:%d:%ld ", i, demoted);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        reuse = global_data->per_stream_dreuse[i];

        printf("DUSE:%d:%ld ", i, reuse);
      }
    }

    printf("\n");

    printf("LDEM-C:%d LDEM-Z: %d LDEM-T: %d LDEM-B: %d LDEM-P: %d\n", global_data->lru_sample_dem[CS], 
        global_data->lru_sample_dem[ZS], global_data->lru_sample_dem[TS], 
        global_data->lru_sample_dem[BS], global_data->lru_sample_dem[PS]);

    printf("MDEM-C:%d MDEM-Z: %d MDEM-T: %d MDEM-B: %d MDEM-P: %d\n", global_data->mru_sample_dem[CS], 
        global_data->mru_sample_dem[ZS], global_data->mru_sample_dem[TS], 
        global_data->mru_sample_dem[BS], global_data->mru_sample_dem[PS]);

    printf("CF:%9ld ZF:%9ld TF:%9ld BF:%9ld PF:%9ld\n", global_data->per_stream_fill[CS], 
        global_data->per_stream_fill[ZS], global_data->per_stream_fill[TS], 
        global_data->per_stream_fill[BS], global_data->per_stream_fill[PS]);

    printf("CH:%8ld ZH:%8ld TH:%8ld BH:%8ld\n", global_data->per_stream_hit[CS], 
        global_data->per_stream_hit[ZS], global_data->per_stream_hit[TS],
        global_data->per_stream_hit[BS]);

    printf("CSE:%8ld ZSE:%8ld TSE:%8ld BSE:%8ld\n", global_data->per_stream_sevct[CS], 
        global_data->per_stream_sevct[ZS], global_data->per_stream_sevct[TS],
        global_data->per_stream_sevct[BS]);

    printf("CXE:%8ld ZXE:%8ld TXE:%8ld BXE:%8ld\n", global_data->per_stream_xevct[CS], 
        global_data->per_stream_xevct[ZS], global_data->per_stream_xevct[TS],
        global_data->per_stream_xevct[BS]);

    printf("CD0:%8ld ZD0:%8ld TD0:%8ld BD0:%8ld\n", global_data->per_stream_demote[0][CS], 
        global_data->per_stream_demote[0][ZS], global_data->per_stream_demote[0][TS],
        global_data->per_stream_demote[0][BS]);

    printf("CD1:%8ld ZD1:%8ld TD1:%8ld BD1:%8ld\n", global_data->per_stream_demote[1][CS], 
        global_data->per_stream_demote[1][ZS], global_data->per_stream_demote[1][TS],
        global_data->per_stream_demote[1][BS]);

    printf("CD2:%8ld ZD2:%8ld TD2:%8ld BD2:%8ld\n", global_data->per_stream_demote[2][CS], 
        global_data->per_stream_demote[2][ZS], global_data->per_stream_demote[2][TS],
        global_data->per_stream_demote[2][BS]);

    printf("CH0:%8ld ZH0:%8ld TH0:%8ld BH0:%8ld\n", global_data->per_stream_rrpv_hit[0][CS], 
        global_data->per_stream_rrpv_hit[0][ZS], global_data->per_stream_rrpv_hit[0][TS],
        global_data->per_stream_rrpv_hit[0][BS]);

    printf("CH1:%8ld ZH1:%8ld TH1:%8ld BH1:%8ld\n", global_data->per_stream_rrpv_hit[1][CS], 
        global_data->per_stream_rrpv_hit[1][ZS], global_data->per_stream_rrpv_hit[1][TS],
        global_data->per_stream_rrpv_hit[1][BS]);

    printf("CH2:%8ld ZH2:%8ld TH2:%8ld BH2:%8ld\n", global_data->per_stream_rrpv_hit[2][CS], 
        global_data->per_stream_rrpv_hit[2][ZS], global_data->per_stream_rrpv_hit[2][TS],
        global_data->per_stream_rrpv_hit[2][BS]);

    printf("CH3:%8ld ZH3:%8ld TH3:%8ld BH3:%8ld\n", global_data->per_stream_rrpv_hit[3][CS], 
        global_data->per_stream_rrpv_hit[3][ZS], global_data->per_stream_rrpv_hit[3][TS],
        global_data->per_stream_rrpv_hit[3][BS]);

    printf("R0:%9ld R1:%9ld R2:%8ld R3:%9ld\n", global_data->rrpv_blocks[0], 
        global_data->rrpv_blocks[1], global_data->rrpv_blocks[2],
        global_data->rrpv_blocks[3]);

    printf("FHC:%8ld FHZ:%8ld FHT:%8ld FHB:%8ld FHP:%8ld\n", global_data->fills_at_head[CS], 
        global_data->fills_at_head[ZS], global_data->fills_at_head[TS],
        global_data->fills_at_head[BS], global_data->fills_at_head[PS]);

    printf("DMC:%8ld DMZ:%8ld DMT:%8ld DMB:%8ld DMP:%8ld\n", global_data->dems_at_head[CS], 
        global_data->dems_at_head[ZS], global_data->dems_at_head[TS],
        global_data->dems_at_head[BS], global_data->dems_at_head[PS]);

    printf("DCTRC:%6d DCTRZ:%6d DCTRT:%6d DCTRB:%6d DCTRP:%6d\n", 
        SAT_CTR_VAL(global_data->dem_ctr[CS]), 
        SAT_CTR_VAL(global_data->dem_ctr[ZS]), 
        SAT_CTR_VAL(global_data->dem_ctr[TS]), 
        SAT_CTR_VAL(global_data->dem_ctr[BS]), 
        SAT_CTR_VAL(global_data->dem_ctr[PS]));

    printf("FCTRC:%6d FCTRZ:%6d FCTRT:%6d FCTRB:%6d FCTRP:%6d\n", 
        SAT_CTR_VAL(global_data->fath_ctr[CS]), 
        SAT_CTR_VAL(global_data->fath_ctr[ZS]), 
        SAT_CTR_VAL(global_data->fath_ctr[TS]), 
        SAT_CTR_VAL(global_data->fath_ctr[BS]), 
        SAT_CTR_VAL(global_data->fath_ctr[PS]));

    printf("FMCTRC:%5d FMCTRZ:%5d FMCTRT:%5d FMCTRB:%5d FMCTRP:%5d\n", 
        SAT_CTR_VAL(global_data->fathm_ctr[CS]), 
        SAT_CTR_VAL(global_data->fathm_ctr[ZS]), 
        SAT_CTR_VAL(global_data->fathm_ctr[TS]), 
        SAT_CTR_VAL(global_data->fathm_ctr[BS]), 
        SAT_CTR_VAL(global_data->fathm_ctr[PS]));

    printf("GFCTR:%6d GDCTR:%6d GFMCTR:%5d\n", SAT_CTR_VAL(global_data->gfath_ctr), 
        SAT_CTR_VAL(global_data->gdem_ctr), SAT_CTR_VAL(global_data->gfathm_ctr));

    ub8 cur_thr;

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        cur_thr = global_data->per_stream_cur_thr[i];

        printf("DTH%d:%7ld ", i, cur_thr);
      }
    }

    printf("DST%d ", global_data->active_stream);

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        cur_thr = global_data->rcy_thr[FOLLOWER_SET][i];

        printf("TH%d:%8ld ", i, cur_thr);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        cur_thr = global_data->occ_thr[FOLLOWER_SET][i];

        printf("OTH%d:%7ld ", i, cur_thr);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        reuse_blocks  = global_data->per_stream_reuse_blocks[i];

        printf("SB%d:%8ld ", i, reuse_blocks);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        reuse = global_data->per_stream_reuse[i];

        printf("SU%d:%8ld ", i, reuse);
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        reuse         = global_data->per_stream_reuse[i];
        reuse_blocks  = global_data->per_stream_reuse_blocks[i];
#if 0
        if (reuse_blocks)
#endif
          if (reuse_blocks && global_data->active_stream == i)
          {
            printf("S%d:%9ld ", i, reuse);
#if 0
            if (global_data->per_stream_max_reuse[i] < (reuse / reuse_blocks))
#endif
              if (global_data->per_stream_max_reuse[i] < reuse)
              {
#if 0
                global_data->per_stream_max_reuse[i]  = (reuse / reuse_blocks); 
#endif
                global_data->per_stream_max_reuse[i]  = reuse; 
#if 0
                if (global_data->per_stream_cur_thr[i] > global_data->rcy_thr[FOLLOWER_SET][i])
#endif
                {
                  global_data->rcy_thr[FOLLOWER_SET][i] = global_data->per_stream_cur_thr[i]; 
                }
              }
          }
          else
          {
            printf("S%d:%9ld ", i, 0);
          }
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        /* Update occupancy threshold */
        reuse         = global_data->per_stream_oreuse[i];
        reuse_blocks  = global_data->per_stream_oreuse_blocks[i];

        if (reuse_blocks)
        {
          printf("O%d:%9ld ", i, reuse);
#if 0
          if (global_data->per_stream_max_reuse[i] < (reuse / reuse_blocks))
#endif
            if (global_data->per_stream_max_oreuse[i] < reuse)
            {
#if 0
              global_data->per_stream_max_reuse[i]  = (reuse / reuse_blocks); 
#endif
              global_data->per_stream_max_oreuse[i]  = reuse; 
              global_data->occ_thr[FOLLOWER_SET][i] = global_data->per_stream_cur_thr[i]; 
            }
        }
        else
        {
          printf("O%d:%9ld ", i, 0);
        }
      }
    }

    printf("\n");

    for (ub4 i = 0; i <= TST; i++)
    {
      if (i == CS || i == TS || i == ZS || i == BS || i == PS)   
      {
        /* Update occupancy threshold */
        reuse         = global_data->per_stream_dreuse[i];
        reuse_blocks  = global_data->per_stream_dreuse_blocks[i];

        if (reuse_blocks)
        {
          printf("D%d:%9ld ", i, reuse / reuse_blocks);
#if 0
          if (global_data->per_stream_max_dreuse[i] < (reuse / reuse_blocks))
#endif
            if (global_data->per_stream_max_dreuse[i] < reuse)
            {
#if 0
              global_data->per_stream_max_dreuse[i] = (reuse / reuse_blocks); 
#endif
              global_data->per_stream_max_dreuse[i]  = reuse; 
              global_data->dem_thr[FOLLOWER_SET][i] = global_data->per_stream_cur_thr[i]; 
            }
        }
        else
        {
          printf("D%d:%9ld:0", i, reuse);
        }
      }
    }

    printf("\n");

#if 0
    for (ub4 i = 0; i <= TST; i++)
    {
      if (global_data->per_stream_hit[i] < 512)  
      {
        global_data->rcy_thr[SAMPLED_SET][i]   = 1;
        global_data->rcy_thr[FOLLOWER_SET][i] = 1;
      }
    }
#endif

    /* Reset global counter */
#if 0
    memset(global_data->per_stream_fill, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_hit, 0, sizeof(ub8) * (TST + 1));

    /* Reset per-set counter */
    memset(policy_data->per_stream_fill, 0, sizeof(ub8) * (TST + 1));
#endif

#if 0
    memset(global_data->per_stream_sevct, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->rrpv_blocks, 0, sizeof(ub8) * SRRIPSAGE_DATA_MAX_RRPV(policy_data));
    memset(global_data->per_stream_demote, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_rrpv_hit, 0, sizeof(ub8) * (TST + 1) * 4);
#endif

    global_data->access_count = 0;

    memset(global_data->per_stream_reuse_blocks, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_reuse, 0, sizeof(ub8) * (TST + 1));
#if 0
    memset(global_data->per_stream_max_reuse, 0, sizeof(ub8) * (TST + 1));
#endif

    /* Reset per stream occupancy blocks */
    memset(global_data->per_stream_oreuse_blocks, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_oreuse, 0, sizeof(ub8) * (TST + 1));
#if 0
    memset(global_data->per_stream_max_oreuse, 0, sizeof(ub8) * (TST + 1));
#endif

#if 0    
    /* Reset per stream demote */
    memset(global_data->per_stream_dreuse_blocks, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_dreuse, 0, sizeof(ub8) * (TST + 1));
    memset(global_data->per_stream_max_dreuse, 0, sizeof(ub8) * (TST + 1));
#endif

    for (i = 0; i <= TST; i++)
    {
      global_data->per_stream_dreuse_blocks[i]  >>= 1;
      global_data->per_stream_dreuse[i]         >>= 1;
      global_data->per_stream_max_dreuse[i]     >>= 1;
    }

    if (global_data->per_stream_cur_thr[CS] == MIN_TH &&
        global_data->per_stream_cur_thr[ZS] == MIN_TH &&
        global_data->per_stream_cur_thr[TS] == MIN_TH &&
        global_data->per_stream_cur_thr[BS] == MIN_TH &&
        global_data->per_stream_cur_thr[PS] == MIN_TH)
    {
      global_data->per_stream_cur_thr[CS] <<= 1;
      global_data->active_stream = CS;
    }
    else
    {
      if (global_data->per_stream_cur_thr[CS] != 1 && 
          global_data->per_stream_cur_thr[CS] != MAX_TH)
      {
        /* Other streams must be using 1 as threshold */
        assert(global_data->per_stream_cur_thr[ZS] == 1 &&
            global_data->per_stream_cur_thr[TS] == 1 &&
            global_data->per_stream_cur_thr[BS] == 1 && 
            global_data->per_stream_cur_thr[PS] == 1);

        global_data->per_stream_cur_thr[CS] <<= 1;
        global_data->active_stream = CS;
      }
      else
      {
        if (global_data->per_stream_cur_thr[ZS] != 1 && 
            global_data->per_stream_cur_thr[ZS] != MAX_TH)
        {
          /* Other streams must be using 1 as threshold */
          assert(global_data->per_stream_cur_thr[CS] == 1 &&
              global_data->per_stream_cur_thr[TS] == 1 &&
              global_data->per_stream_cur_thr[BS] == 1 &&
              global_data->per_stream_cur_thr[PS] == 1);

          global_data->per_stream_cur_thr[ZS] <<= 1;
          global_data->active_stream = ZS;
        }
        else
        {
          if (global_data->per_stream_cur_thr[TS] != 1 && 
              global_data->per_stream_cur_thr[TS] != MAX_TH)
          {
            /* Other streams must be using 1 as threshold */
            assert(global_data->per_stream_cur_thr[CS] == 1 &&
                global_data->per_stream_cur_thr[ZS] == 1 &&
                global_data->per_stream_cur_thr[BS] == 1 &&
                global_data->per_stream_cur_thr[PS] == 1);

            global_data->per_stream_cur_thr[TS] <<= 1;
            global_data->active_stream = TS;
          }
          else
          {
            if (global_data->per_stream_cur_thr[BS] != 1 && 
                global_data->per_stream_cur_thr[BS] != MAX_TH)
            {
              /* Other streams must be using 1 as threshold */
              assert(global_data->per_stream_cur_thr[CS] == 1 &&
                  global_data->per_stream_cur_thr[ZS] == 1 &&
                  global_data->per_stream_cur_thr[TS] == 1 &&
                  global_data->per_stream_cur_thr[PS] == 1);

              global_data->per_stream_cur_thr[BS] <<= 1;
              global_data->active_stream = BS;
            }
            else
            {
              if (global_data->per_stream_cur_thr[PS] != 1 && 
                  global_data->per_stream_cur_thr[PS] != MAX_TH)
              {
                /* Other streams must be using 1 as threshold */
                assert(global_data->per_stream_cur_thr[CS] == 1 &&
                    global_data->per_stream_cur_thr[ZS] == 1 &&
                    global_data->per_stream_cur_thr[TS] == 1 &&
                    global_data->per_stream_cur_thr[BS] == 1);

                global_data->per_stream_cur_thr[PS] <<= 1;
                global_data->active_stream = PS;
              }
              else
              {
                /* If none have any intermediate value, check MAX_TH stream */
                if (global_data->per_stream_cur_thr[CS] == MAX_TH)
                {
                  global_data->per_stream_cur_thr[CS] = 1;
                  global_data->per_stream_cur_thr[ZS] <<= 1;

                  global_data->active_stream            = ZS;
                  global_data->per_stream_max_reuse[ZS] = 0;
                }

                if (global_data->per_stream_cur_thr[ZS] == MAX_TH)
                {
                  global_data->per_stream_cur_thr[ZS] = 1;
                  global_data->per_stream_cur_thr[TS] <<= 1;

                  global_data->active_stream            = TS;
                  global_data->per_stream_max_reuse[TS] = 0;
                }

                if (global_data->per_stream_cur_thr[TS] == MAX_TH)
                {
                  global_data->per_stream_cur_thr[TS] = 1;
                  global_data->per_stream_cur_thr[BS] <<= 1;

                  global_data->active_stream            = BS;
                  global_data->per_stream_max_reuse[BS] = 0;
                }

                if (global_data->per_stream_cur_thr[BS] == MAX_TH)
                {
                  global_data->per_stream_cur_thr[BS] = 1;
                  global_data->per_stream_cur_thr[PS] <<= 1;

                  global_data->active_stream            = PS;
                  global_data->per_stream_max_reuse[PS] = 0;
                }

                if (global_data->per_stream_cur_thr[PS] == MAX_TH)
                {
                  global_data->per_stream_cur_thr[PS] = 1;
                }
              }
            }
          }
        }
      }
    }

    /* Override value for each Color and Depth */
#if 0
    global_data->rcy_thr[SAMPLED_SET][CS]   = global_data->per_stream_cur_thr[CS];
    global_data->rcy_thr[SAMPLED_SET][ZS]   = global_data->per_stream_cur_thr[ZS];
    global_data->rcy_thr[SAMPLED_SET][TS]   = global_data->per_stream_cur_thr[TS];
    global_data->rcy_thr[SAMPLED_SET][BS]   = global_data->per_stream_cur_thr[BS];
    global_data->rcy_thr[SAMPLED_SET][PS]   = global_data->per_stream_cur_thr[PS];
#endif

    if (global_data->active_stream == CS)
    {
      global_data->rcy_thr[SAMPLED_SET][CS]   = global_data->per_stream_cur_thr[CS];
    }
    else
    {
      global_data->rcy_thr[SAMPLED_SET][CS]   = global_data->rcy_thr[FOLLOWER_SET][CS];
    }


    if (global_data->active_stream == ZS)
    {
      global_data->rcy_thr[SAMPLED_SET][ZS]   = global_data->per_stream_cur_thr[ZS];
    }
    else
    {
      global_data->rcy_thr[SAMPLED_SET][ZS]   = global_data->rcy_thr[FOLLOWER_SET][ZS];
    }

    if (global_data->active_stream == TS)
    {
      global_data->rcy_thr[SAMPLED_SET][TS]   = global_data->per_stream_cur_thr[TS];
    }
    else
    {
      global_data->rcy_thr[SAMPLED_SET][TS]   = global_data->rcy_thr[FOLLOWER_SET][TS];
    }

    if (global_data->active_stream == BS)
    {
      global_data->rcy_thr[SAMPLED_SET][BS]   = global_data->per_stream_cur_thr[BS];
    }
    else
    {
      global_data->rcy_thr[SAMPLED_SET][BS]   = global_data->rcy_thr[FOLLOWER_SET][BS];
    }

    if (global_data->active_stream == PS)
    {
      global_data->rcy_thr[SAMPLED_SET][PS]   = global_data->per_stream_cur_thr[PS];
    }
    else
    {
      global_data->rcy_thr[SAMPLED_SET][PS]   = global_data->rcy_thr[FOLLOWER_SET][PS];
    }

    /* Override value for each Color and Depth */
    global_data->occ_thr[SAMPLED_SET][CS]   = global_data->per_stream_cur_thr[CS];
    global_data->occ_thr[SAMPLED_SET][ZS]   = global_data->per_stream_cur_thr[ZS];
    global_data->occ_thr[SAMPLED_SET][TS]   = global_data->per_stream_cur_thr[TS];
    global_data->occ_thr[SAMPLED_SET][BS]   = global_data->per_stream_cur_thr[BS];
    global_data->occ_thr[SAMPLED_SET][PS]   = global_data->per_stream_cur_thr[PS];
    
    /* Generate Fill order and demote on hit vector.
     *
     * Based on the counters updated on LRU and MRU sample access fill order 
     * is decided as follows. 
     *
     * Any stream which does better with LRU sample is filled at LRU
     * Any stream which does better with MRU sampe is filled at MRU
     * If texture does better with LRU and has any inter-stream reuse
     * Source stream is filled at LRU and texture is demoted on hit.
     * 
     * */
    
    ub1 pstreams[TST + 1];
     
    memset(pstreams, 0, sizeof(ub1) * (TST + 1));
    
    /* Filling a block at head is decided using following algorithm
     *
     * There are three counters 
     *
     * 1. Global miss counter     (C0)
     * 2. Per stream miss counter (C1)
     * 3. Per stream hit counter  (C2)
     *
     *  if C0 indicate fill at head
     *    Fill all the streams for which C1 says fill at head
     *
     *    If there is no such stream, fill streams for which C2 says to fill at head
     *
     *  if C0 indicate fill at tail
     *    fill all streams at tail 
     *   
     * */
    
    /* Fill at head is global best */
    for (ub1 i = NN; i <= TST; i++)
    {
      if (global_data->per_stream_fill[i])
      {
        global_data->fill_at_head[i] = FALSE;
#if 0
        if (SAT_CTR_VAL(global_data->gfathm_ctr) < PSEL_MID_VAL)
#endif
        {
          if (SAT_CTR_VAL(global_data->fathm_ctr[i]) < PSEL_MID_VAL)
          {
            global_data->fill_at_head[i] = TRUE;
          }
        }
#if 0
        else
        {
          if (SAT_CTR_VAL(global_data->fath_ctr[i]) > PSEL_MID_VAL)
          {
            global_data->fill_at_head[i] = TRUE;
          }
        }
#endif
      }

      if (SAT_CTR_VAL(global_data->dem_ctr[i]) < PSEL_MID_VAL)
      {
        global_data->demote_at_head[i] = TRUE;
      }
      else
      {
        global_data->demote_at_head[i] = FALSE;
      }

#define DONH_THR (8)

      if (SAT_CTR_VAL(global_data->fathm_ctr[i]) > (PSEL_MID_VAL + DONH_THR)&& 
          SAT_CTR_VAL(global_data->fath_ctr[i]) > (PSEL_MID_VAL + DONH_THR))
      {
        switch (i)
        {
          case CS:
            if (SAT_CTR_VAL(global_data->ct_ctr) || 
                SAT_CTR_VAL(global_data->cb_ctr))
            {
              global_data->demote_on_hit[i]  = FALSE;
            }
            else
            {
              global_data->demote_on_hit[i]  = TRUE;
            }
            break;

          case BS:
            if (SAT_CTR_VAL(global_data->bt_ctr) || 
                SAT_CTR_VAL(global_data->bc_ctr))
            {
              global_data->demote_on_hit[i]  = FALSE;
            }
            else
            {
              global_data->demote_on_hit[i]  = TRUE;
            }
            break;

          case ZS:
            if (SAT_CTR_VAL(global_data->zt_ctr))
            {
              global_data->demote_on_hit[i]  = FALSE;
            }
            else
            {
              global_data->demote_on_hit[i]  = TRUE;
            }
            break;

          case TS:
            if (SAT_CTR_VAL(global_data->tb_ctr))
            {
              global_data->demote_on_hit[i]  = FALSE;
            }
            else
            {
              global_data->demote_on_hit[i]  = TRUE;
            }

          default:
            global_data->demote_on_hit[i]  = TRUE;
        }
      }
      else
      {
        global_data->demote_on_hit[i]  = FALSE;
      }
      
      if (SAT_CTR_VAL(global_data->dem_ctr[i]) > PSEL_MID_VAL)
      {
        global_data->demote_on_hit[i]  = TRUE;
      }
      else
      {
        global_data->demote_on_hit[i]  = FALSE;
      }

#if 0
      switch (i)
      {
        case CS:
          if (SAT_CTR_VAL(global_data->ct_ctr) || 
              SAT_CTR_VAL(global_data->cb_ctr))
          {
            global_data->demote_on_hit[i]  = FALSE;
          }
          else
          {
            global_data->demote_on_hit[i]  = TRUE;
          }
          break;

        case BS:
          if (SAT_CTR_VAL(global_data->bt_ctr) || 
              SAT_CTR_VAL(global_data->bc_ctr))
          {
            global_data->demote_on_hit[i]  = FALSE;
          }
          else
          {
            global_data->demote_on_hit[i]  = TRUE;
          }
          break;

        case ZS:
          if (SAT_CTR_VAL(global_data->zt_ctr))
          {
            global_data->demote_on_hit[i]  = FALSE;
          }
          else
          {
            global_data->demote_on_hit[i]  = TRUE;
          }
          break;

        case TS:
          if (SAT_CTR_VAL(global_data->tb_ctr))
          {
            global_data->demote_on_hit[i]  = FALSE;
          }
          else
          {
            global_data->demote_on_hit[i]  = TRUE;
          }
      }
#endif
      }

#undef DONH_THR

#if 0
    /* Set fill and demote flags for inter-stream use */
    if (SAT_CTR_VAL(global_data->fathm_ctr[TS]) < PSEL_MID_VAL)
    {
      if (SAT_CTR_VAL(global_data->ct_ctr) > PSEL_MID_VAL / 4)
      {
        global_data->fill_at_head[CS]   = TRUE;
        global_data->demote_on_hit[TS]  = TRUE;
      }

      if (SAT_CTR_VAL(global_data->bt_ctr) > PSEL_MID_VAL / 4)
      {
        global_data->fill_at_head[BS]   = TRUE;
        global_data->demote_on_hit[TS]  = TRUE;
      }

      if (SAT_CTR_VAL(global_data->zt_ctr) > PSEL_MID_VAL / 4)
      {
        global_data->fill_at_head[ZS]   = TRUE;
        global_data->demote_on_hit[TS]  = TRUE;
      }
    }
#endif

    global_data->fill_at_head[PS]  = FALSE;

#if 0
    if (global_data->fill_at_head[CS] || global_data->fill_at_head[ZS])
#endif

    if (global_data->fill_at_head[CS])
    {
      global_data->fill_at_head[TS]  = FALSE;
    }

    if (global_data->fill_at_head[TS] == TRUE)
    {
      global_data->fill_at_head[ZS]  = FALSE;
    }

#if 0    
    global_data->fill_at_head[CS]  = TRUE;
    global_data->fill_at_head[TS]  = FALSE;
    global_data->fill_at_head[ZS]  = FALSE;
    global_data->fill_at_head[BS]  = FALSE;

    global_data->fill_at_head[CS]  = TRUE;
    global_data->fill_at_head[TS]  = FALSE;

    global_data->fill_at_head[CS]  = TRUE;
    global_data->fill_at_head[ZS]  = FALSE;
    global_data->fill_at_head[TS]  = TRUE;
    global_data->fill_at_head[BS]  = TRUE;

    global_data->fill_at_head[CS]  = TRUE;
    global_data->fill_at_head[ZS]  = FALSE;
    global_data->fill_at_head[TS]  = FALSE;
    global_data->fill_at_head[BS]  = TRUE;

    global_data->fill_at_head[CS]  = TRUE;
    global_data->fill_at_head[ZS]  = FALSE;
    global_data->fill_at_head[TS]  = FALSE;
    global_data->fill_at_head[BS]  = FALSE;
    global_data->fill_at_head[PS]  = TRUE;

    global_data->demote_on_hit[TS]  = TRUE;
    global_data->demote_on_hit[BS]  = TRUE;

    global_data->demote_on_hit[CS]  = FALSE;
    global_data->demote_on_hit[ZS]  = FALSE;
    global_data->demote_on_hit[TS]  = TRUE;
    global_data->demote_on_hit[BS]  = FALSE;
    global_data->demote_on_hit[PS]  = TRUE;

    global_data->demote_on_hit[TS]  = TRUE;

    global_data->demote_on_hit[CS]  = FALSE;
    global_data->demote_on_hit[ZS]  = FALSE;
    global_data->demote_on_hit[TS]  = TRUE;
    global_data->demote_on_hit[PS]  = FALSE;
    global_data->demote_on_hit[BS]  = FALSE;
    global_data->demote_on_hit[TS]  = TRUE;
    global_data->demote_on_hit[ZS]  = TRUE;
    global_data->demote_on_hit[TS]  = TRUE;
#endif

    /* Reset various counters */
    for (ub1 i = 0; i <= TST; i++)
    {
      memset(&(global_data->lru_sample_access[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->lru_sample_hit[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->lru_sample_dem[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->mru_sample_access[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->mru_sample_hit[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->mru_sample_dem[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->thr_sample_access[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->thr_sample_hit[i]), 0, sizeof(ub8) * (TST + 1));
      memset(&(global_data->thr_sample_dem[i]), 0, sizeof(ub8) * (TST + 1));
    }
    
    SAT_CTR_RST(global_data->cb_ctr);
    SAT_CTR_RST(global_data->bc_ctr);
    SAT_CTR_RST(global_data->ct_ctr);
    SAT_CTR_RST(global_data->bt_ctr);
    SAT_CTR_RST(global_data->tb_ctr);
    SAT_CTR_RST(global_data->zt_ctr);

    for (ub1 i = 0; i <= TST; i++)
    {
      SAT_CTR_SET(global_data->fath_ctr[i], PSEL_MID_VAL);
#if 0
      SAT_CTR_SET(global_data->dem_ctr[i], PSEL_MID_VAL);
#endif
    }
  }

  if (!node)
  {
    if (policy_data->set_type == LRU_SAMPLED_SET)
    {
      /* Update per-stream counter */
      if (policy_data->fill_at_lru[info->stream])
      {
        SAT_CTR_INC(global_data->fathm_ctr[info->stream]);
      }

      SAT_CTR_INC(global_data->gfathm_ctr);
    }
    else
    {
      if (policy_data->set_type == MRU_SAMPLED_SET)
      {
        /* Update per-stream counter */
        if (policy_data->dem_to_lru[info->stream])
        {
          SAT_CTR_DEC(global_data->fathm_ctr[info->stream]);
        }

        SAT_CTR_DEC(global_data->gfathm_ctr);
      }
    }
  }

  return node;
}

void cache_fill_block_srripsage(srripsage_data *policy_data, 
  srripsage_gdata *global_data, int way, long long tag, 
  enum cache_block_state_t state, int strm, memory_trace *info)
{
  struct cache_block_t *block;
  struct cache_block_t *rrpv_block;
  int                   rrpv;
  int                   min_wayid;
  long long             tag_ptr;
  int                   stream_ptr;
  enum cache_block_state_t state_ptr;

  /* Check: tag, state and insertion_position are valid */
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain SRRIPSAGE specific data */
  block = &(SRRIPSAGE_DATA_BLOCKS(policy_data)[way]);

  assert(block->stream == 0);

  assert(policy_data->following == cache_policy_srripsage || 
    policy_data->following == cache_policy_srrip);

  /* Get RRPV to be assigned to the new block */
  rrpv = cache_get_fill_rrpv_srripsage(policy_data, global_data, info);
  
  assert(rrpv <= SRRIPSAGE_DATA_MAX_RRPV(policy_data));

  /* If block is not bypassed */
  if (rrpv != BYPASS_RRPV)
  {
    /* Ensure a valid RRPV */
    assert(rrpv >= 0 && rrpv <= policy_data->max_rrpv); 

    /* Remove block from free list */
    free_list_remove_block(policy_data, block);

    /* Update new block state and stream */
    CACHE_UPDATE_BLOCK_STATE(block, tag, info->vtl_addr, state);
    CACHE_UPDATE_BLOCK_STREAM(block, strm);

    block->dirty      = (info && info->spill) ? 1 : 0;
    block->recency    = policy_data->evictions;
    block->last_rrpv  = rrpv;
    block->access     = 0;
    block->demote     = FALSE;

    switch (strm)
    {
      case TS:   
        block->epoch = 0;
        break;

      default:
        block->epoch = 3;
        break;
    }

    /* Update RRPV 2 block count */
    if (policy_data->set_type == THR_SAMPLED_SET)
    {
      if (rrpv == SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1)
      {
        cache_add_reuse_blocks(global_data, OP_FILL, info->stream);
      }
    }

    /* Insert block in to the corresponding RRPV queue */
    if (policy_data->set_type == LRU_SAMPLED_SET && 
        rrpv == SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1)
    {
      /* Fill block at LRU */
      if (policy_data->fill_at_lru[info->stream])
      {
        CACHE_PREPEND_TO_QUEUE(block, 
            SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv], 
            SRRIPSAGE_DATA_VALID_TAIL(policy_data)[rrpv]);
      }
      else
      {
        CACHE_APPEND_TO_QUEUE(block, 
            SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv], 
            SRRIPSAGE_DATA_VALID_TAIL(policy_data)[rrpv]);
      }
    }
    else
    {
      if (policy_data->set_type == MRU_SAMPLED_SET && 
          rrpv == SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1)
      {
        if (global_data->bm_ctr == 0)
        {
          /* Fill block at LRU */
          CACHE_PREPEND_TO_QUEUE(block, 
              SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv], 
              SRRIPSAGE_DATA_VALID_TAIL(policy_data)[rrpv]);
        }
        else
        {
          /* Fill block at MRU */
          CACHE_APPEND_TO_QUEUE(block, 
              SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv], 
              SRRIPSAGE_DATA_VALID_TAIL(policy_data)[rrpv]);
        }
      }
      else
      {
        if (rrpv == SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1) 
        {
          assert(policy_data->set_type == FOLLOWER_SET || 
              policy_data->set_type == THR_SAMPLED_SET);

          if (global_data->fill_at_head[info->stream] == TRUE)
          {
            CACHE_PREPEND_TO_QUEUE(block, 
                SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv], 
                SRRIPSAGE_DATA_VALID_TAIL(policy_data)[rrpv]);
          }
          else
          {
            if (global_data->bm_ctr == 0)
            {
              CACHE_PREPEND_TO_QUEUE(block, 
                  SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv], 
                  SRRIPSAGE_DATA_VALID_TAIL(policy_data)[rrpv]);
            }
            else
            {
              CACHE_APPEND_TO_QUEUE(block, 
                  SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv], 
                  SRRIPSAGE_DATA_VALID_TAIL(policy_data)[rrpv]);
            }
          }
        }
        else
        {
          CACHE_APPEND_TO_QUEUE(block, 
              SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv], 
              SRRIPSAGE_DATA_VALID_TAIL(policy_data)[rrpv]);
        }
      }
    }

    /* TODO: Get set associativity dynamically */
    assert(SRRIPSAGE_DATA_RRPV_BLCKS(policy_data)[rrpv] < 16);
    SRRIPSAGE_DATA_RRPV_BLCKS(policy_data)[rrpv]++;
    global_data->rrpv_blocks[rrpv]++;
  }

  /* Reset post fill hit bit */
  policy_data->hit_post_fill[info->stream] = FALSE;
}

int cache_replace_block_srripsage(srripsage_data *policy_data, 
    srripsage_gdata *global_data, memory_trace *info)
{
  struct cache_block_t *block;
  struct cache_block_t *lrublock;
  int    rrpv;
  int    old_rrpv;

  /* Remove a nonbusy block from the tail */
  unsigned int min_wayid = ~(0);
  lrublock = NULL;

  assert(policy_data->following == cache_policy_srripsage || 
      policy_data->following == cache_policy_srrip);

  /* Try to find an invalid block always from head of the free list. */
  for (block = SRRIPSAGE_DATA_FREE_HEAD(policy_data); block; block = block->prev)
  {
    if (block->way < min_wayid)
      min_wayid = block->way;
  }

  /* Obtain RRPV from where to replace the block */
  rrpv = cache_get_replacement_rrpv_srripsage(policy_data);

  /* Ensure rrpv is with in max_rrpv bound */
  assert(rrpv >= 0 && rrpv <= SRRIPSAGE_DATA_MAX_RRPV(policy_data));

  if (min_wayid == ~(0))
  {
    /* If there is no block with required RRPV, increment RRPV of all the blocks
     * until we get one with the required RRPV */
    if (SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv].head == NULL)
    {
      assert(SRRIPSAGE_DATA_VALID_TAIL(policy_data)[rrpv].head == NULL);

      /* To collect sample reuse block data */
      if (policy_data->set_type == LRU_SAMPLED_SET)
      {
        /* Age MRU in LRU set */
        CACHE_SRRIPSAGE_AGEBLOCK_MRU(SRRIPSAGE_DATA_VALID_HEAD(policy_data), 
            SRRIPSAGE_DATA_VALID_TAIL(policy_data), rrpv, global_data);
      }
      else
      {
        if (policy_data->set_type == MRU_SAMPLED_SET)
        {
          /* Demote sample stream at LRU and rest at MRU */
          if (policy_data->dem_to_lru[info->stream])
          {
            CACHE_SRRIPSAGE_AGEBLOCK_LRU(SRRIPSAGE_DATA_VALID_HEAD(policy_data), 
                SRRIPSAGE_DATA_VALID_TAIL(policy_data), rrpv, global_data);
          }
          else
          {
            CACHE_SRRIPSAGE_AGEBLOCK_MRU(SRRIPSAGE_DATA_VALID_HEAD(policy_data), 
                SRRIPSAGE_DATA_VALID_TAIL(policy_data), rrpv, global_data);
          }
        }
        else
        {
          assert(policy_data->set_type == FOLLOWER_SET || 
              policy_data->set_type == THR_SAMPLED_SET);


          CACHE_SRRIPSAGE_AGEBLOCK_FOLLOWER(SRRIPSAGE_DATA_VALID_HEAD(policy_data), 
              SRRIPSAGE_DATA_VALID_TAIL(policy_data), rrpv, policy_data, global_data);

          /* Increment demotion counter */
          if (global_data->per_stream_dreuse[info->stream] > 4)
          {
            global_data->dems_at_head[info->stream]++;
          }
        }
      }
    }

    for (block = SRRIPSAGE_DATA_VALID_TAIL(policy_data)[rrpv].head; block; block = block->prev)
    {
      if (!block->busy && block->way < min_wayid)
        min_wayid = block->way;
    }

    /* Get last touch RRPV of the block */
    old_rrpv = (policy_data->blocks)[min_wayid].last_rrpv;

    /* Increment eviction in the set */
    policy_data->evictions += 1;

    /* Update block count at old RRPV */
    assert(SRRIPSAGE_DATA_RRPV_BLCKS(policy_data)[old_rrpv] > 0);
    SRRIPSAGE_DATA_RRPV_BLCKS(policy_data)[old_rrpv]--;

    global_data->rrpv_blocks[old_rrpv]--;

    if (policy_data->blocks[min_wayid].last_rrpv == 2)
    {
      ub1 src_stream = policy_data->blocks[min_wayid].stream;

      if (src_stream == info->stream)
      {
        global_data->per_stream_sevct[src_stream] += 1;
      }
      else
      {
        global_data->per_stream_xevct[src_stream] += 1;
      }
    }
  }

  /* If no non busy block can be found, return -1 */
  return (min_wayid != ~(0)) ? min_wayid : -1;
}

void cache_access_block_srripsage(srripsage_data *policy_data, 
  srripsage_gdata *global_data, int way, int strm, memory_trace *info)
{
  enum cache_block_state_t state_ptr; 
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  int min_wayid;
  int stream_ptr;
  long long tag_ptr;

  int old_rrpv;
  int new_rrpv;

  /* Update per-stream hits */
  global_data->per_stream_hit[strm]++;

  assert(policy_data->following == cache_policy_srripsage || 
      policy_data->following == cache_policy_srrip);

  blk  = &(SRRIPSAGE_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);
  
  /* Increment inter stream counter */
  if (policy_data->set_type == LRU_SAMPLED_SET)
  {
    if (info->stream == TS)
    {
      if (blk->stream == CS)
      {
        SAT_CTR_INC(global_data->ct_ctr);
      }

      if (blk->stream == BS)
      {
        SAT_CTR_INC(global_data->bt_ctr);
      }

      if (blk->stream == ZS)
      {
        SAT_CTR_INC(global_data->zt_ctr);
      }
    }

    if (info->stream == BS)
    {
      if (blk->stream == CS)
      {
        SAT_CTR_INC(global_data->cb_ctr);
      }

      if (blk->stream == TS)
      {
        SAT_CTR_INC(global_data->tb_ctr);
      }
    }

    if (info->stream == CS)
    {
      if (blk->stream == BS)
      {
        SAT_CTR_INC(global_data->bc_ctr);
      }
    }
  }

  /* Get old RRPV from the block */
  old_rrpv = (((rrip_list *)(blk->data))->rrpv);
  new_rrpv = old_rrpv;

  /* Get new RRPV using policy specific function */
  new_rrpv = cache_get_new_rrpv_srripsage(policy_data, global_data, info, way, 
      old_rrpv, blk->recency);
  
  /* Update counters for sampled set */
  if (policy_data->set_type == THR_SAMPLED_SET)
  {
    /* Update hit counter at RRPV 0 */
    if (new_rrpv == !SRRIPSAGE_DATA_MAX_RRPV(policy_data))
    {
      if (old_rrpv != new_rrpv)
      {
        /* Update blocks ate RRPV 0 */
        cache_add_reuse_blocks(global_data, OP_HIT, blk->stream);

        if (old_rrpv == SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1)
        {
          cache_remove_reuse_blocks(global_data, OP_FILL, blk->stream);  
        }
      }
      else
      {
        /* Update blocks ate RRPV 0 */
        cache_update_reuse(global_data, OP_HIT, blk->stream);
      }
    }

    if (old_rrpv == SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1)
    {
      /* Update blocks ate RRPV 0 */
      cache_update_reuse(global_data, OP_FILL, blk->stream);
    }

    if (blk->demote) 
    {
      cache_update_reuse(global_data, OP_DEMOTE, blk->stream);
    }
  }
  
  /* Update demotion counter */
  if (new_rrpv == SRRIPSAGE_DATA_MAX_RRPV(policy_data))
  {
    global_data->per_stream_demote[old_rrpv][info->stream]++;
  }

  if (old_rrpv == SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1 && 
      blk->demote == FALSE)
  {
    if (policy_data->set_type == LRU_SAMPLED_SET)
    {
      /* Update per-stream counter */
      SAT_CTR_INC(global_data->fath_ctr[blk->stream]);

      global_data->lru_sample_hit[blk->stream]++;
      
      /* Update global counter */
      SAT_CTR_INC(global_data->gfath_ctr);
    }
    else
    {
      if (policy_data->set_type == MRU_SAMPLED_SET)
      {
        /* Update per-stream counter */
        SAT_CTR_DEC(global_data->fath_ctr[blk->stream]);
        global_data->mru_sample_hit[blk->stream]++;

        /* Update global counter */
        SAT_CTR_DEC(global_data->gfath_ctr);
      }
    }
  }
  else
  {
    if (old_rrpv == SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1 &&
        blk->demote == TRUE)
    {
      if (policy_data->set_type == LRU_SAMPLED_SET)
      {
        if (policy_data->fill_at_lru[blk->stream])
        {
          /* Update per-stream counter */
          SAT_CTR_INC(global_data->dem_ctr[blk->stream]);

          global_data->lru_sample_dem[blk->stream]++;

          /* Update global counter */
          SAT_CTR_INC(global_data->gdem_ctr);
        }
      }
      else
      {
        if (policy_data->set_type == MRU_SAMPLED_SET)
        {
          /* Update per-stream counter */
          if (policy_data->dem_to_lru[blk->stream])
          {
            SAT_CTR_DEC(global_data->dem_ctr[blk->stream]);

            global_data->mru_sample_dem[blk->stream]++;

            /* Update global counter */
            SAT_CTR_DEC(global_data->gdem_ctr);
          }
        }
      }
    }
  }

  /* Update block queue if block got new RRPV */
  if (new_rrpv != old_rrpv)
  {
    /* Move block at the tail of the list */
    CACHE_REMOVE_FROM_QUEUE(blk, SRRIPSAGE_DATA_VALID_HEAD(policy_data)[old_rrpv],
        SRRIPSAGE_DATA_VALID_TAIL(policy_data)[old_rrpv]);

    CACHE_APPEND_TO_QUEUE(blk, SRRIPSAGE_DATA_VALID_HEAD(policy_data)[new_rrpv], 
        SRRIPSAGE_DATA_VALID_TAIL(policy_data)[new_rrpv]);
  }
  else
  {
    /* Move block to the tail of the list */
    if (new_rrpv == !SRRIPSAGE_DATA_MAX_RRPV(policy_data))
    {
      CACHE_REMOVE_FROM_QUEUE(blk, SRRIPSAGE_DATA_VALID_HEAD(policy_data)[old_rrpv],
          SRRIPSAGE_DATA_VALID_TAIL(policy_data)[old_rrpv]);

      CACHE_APPEND_TO_QUEUE(blk, SRRIPSAGE_DATA_VALID_HEAD(policy_data)[new_rrpv], 
          SRRIPSAGE_DATA_VALID_TAIL(policy_data)[new_rrpv]);
    }
  }

  if (strm == TS)
  {
    if (info && blk->stream == info->stream)
    {
      blk->epoch  = (blk->epoch < 2) ? blk->epoch + 1 : 2;
    }
    else
    {
      blk->epoch = 0;
    }
  }
  else
  {
    if (blk->stream == TS)
    {
      blk->epoch = 3;
    }
  }

  if (blk->last_rrpv != new_rrpv)
  {
    /* Update RRPV block count */
    assert(SRRIPSAGE_DATA_RRPV_BLCKS(policy_data)[blk->last_rrpv] > 0);

    SRRIPSAGE_DATA_RRPV_BLCKS(policy_data)[blk->last_rrpv]--;
    global_data->rrpv_blocks[blk->last_rrpv]--;

    SRRIPSAGE_DATA_RRPV_BLCKS(policy_data)[new_rrpv]++;
    global_data->rrpv_blocks[new_rrpv]++;
  }

  global_data->per_stream_rrpv_hit[old_rrpv][strm]++;

  CACHE_UPDATE_BLOCK_STREAM(blk, strm);
  blk->dirty      = (info && info->dirty) ? 1 : 0;
  blk->recency    = policy_data->evictions;
  blk->last_rrpv  = new_rrpv;
  blk->access    += 1;

  policy_data->last_eviction = policy_data->evictions;

  /* Reset post fill hit bit */
  policy_data->hit_post_fill[info->stream] = TRUE;
}

int cache_get_fill_rrpv_srripsage(srripsage_data *policy_data, 
    srripsage_gdata *global_data, memory_trace *info)
{
  int ret_rrpv;
  int tex_alloc;
  struct cache_block_t *block;

  tex_alloc = FALSE;

  block = NULL;

  block = SRRIPSAGE_DATA_VALID_HEAD(policy_data)[0].head;

  switch (info->stream)
  {
    case TS:
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1;
      break;

    case BS:
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data);
      break;

    case CS:
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1;
      break;

    case ZS:
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1;
      break;

    case PS:
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data);
      break;

    default:
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data);
      break;
  }

  if (block && policy_data->evictions)
  {
    /* If recency of LRU block is below a threshold, fill new block at RRPV 3 */
    assert(block->recency <= policy_data->evictions);

    if ((policy_data->evictions - block->recency) < RCY_THR(policy_data, global_data, block->stream))
    {
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data);
    }
  }

  return ret_rrpv;
}

int cache_get_replacement_rrpv_srripsage(srripsage_data *policy_data)
{
  return SRRIPSAGE_DATA_MAX_RRPV(policy_data);
}

int cache_get_new_rrpv_srripsage(srripsage_data *policy_data, 
    srripsage_gdata *global_data, memory_trace *info, int way, int old_rrpv,
    unsigned long long old_recency)
{
  /* Current SRRIPSAGE specific block state */
  unsigned int state;
  int ret_rrpv;
  struct cache_block_t *block;
  
  ret_rrpv = 0;

  /* Get the LRU block */
  block = SRRIPSAGE_DATA_VALID_HEAD(policy_data)[ret_rrpv].head;

  /* Choose new rrpv based on LRU block at RRPV 0 */
  if (block && policy_data->evictions)
  {
    /* If recency of LRU block is below a threshold, fill new block at 
     * RRPV 3 */
    assert(block->recency <= policy_data->evictions);

    if ((old_rrpv == SRRIPSAGE_DATA_MAX_RRPV(policy_data)) &&
      (policy_data->evictions - block->recency) < RCY_THR(policy_data, global_data, block->stream))
    {
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data); 
    }
  }

#if 0
  if (policy_data->set_type == FOLLOWER_SET && (info->stream != CS))
  {
    if (old_rrpv != !SRRIPSAGE_DATA_MAX_RRPV(policy_data))
    {
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data); 
    }
  }
#endif

  if (policy_data->set_type == FOLLOWER_SET && 
      global_data->demote_on_hit[info->stream] == TRUE)
  {
    if (old_rrpv != SRRIPSAGE_DATA_MAX_RRPV(policy_data))
    {
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data) - 1; 
    }
#if 0
    else
    {
      ret_rrpv = SRRIPSAGE_DATA_MAX_RRPV(policy_data); 
    }
#endif
  }

  return ret_rrpv;
}

/* Update state of block. */
void cache_set_block_srripsage(srripsage_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
  struct cache_block_t *block;

  {
    block = &(SRRIPSAGE_DATA_BLOCKS(policy_data))[way];

    /* Check: tag matches with the block's tag. */
    assert(block->tag == tag);

    /* Check: block must be in valid state. It is not possible to set
     * state for an invalid block.*/
    assert(block->state != cache_block_invalid);

    if (state != cache_block_invalid)
    {
      /* Assign access stream */
      CACHE_UPDATE_BLOCK_STATE(block, tag, info->vtl_addr, state);
      CACHE_UPDATE_BLOCK_STREAM(block, stream);
      return;
    }

    /* Invalidate block */
    CACHE_UPDATE_BLOCK_STATE(block, tag, info->vtl_addr, cache_block_invalid);
    CACHE_UPDATE_BLOCK_STREAM(block, NN);
    block->epoch  = 0;

    /* Get old RRPV from the block */
    int old_rrpv = (((rrip_list *)(block->data))->rrpv);

    /* Remove block from valid list and insert into free list */
    CACHE_REMOVE_FROM_QUEUE(block, SRRIPSAGE_DATA_VALID_HEAD(policy_data)[old_rrpv],
        SRRIPSAGE_DATA_VALID_TAIL(policy_data)[old_rrpv]);
    CACHE_APPEND_TO_SQUEUE(block, SRRIPSAGE_DATA_FREE_HEAD(policy_data), 
        SRRIPSAGE_DATA_FREE_TAIL(policy_data));
  }
}


/* Get tag and state of a block. */
struct cache_block_t cache_get_block_srripsage(srripsage_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  assert(policy_data);
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  {
    PTR_ASSIGN(tag_ptr, (SRRIPSAGE_DATA_BLOCKS(policy_data)[way]).tag);
    PTR_ASSIGN(state_ptr, (SRRIPSAGE_DATA_BLOCKS(policy_data)[way]).state);
    PTR_ASSIGN(stream_ptr, (SRRIPSAGE_DATA_BLOCKS(policy_data)[way]).stream);

    return SRRIPSAGE_DATA_BLOCKS(policy_data)[way];
  }
}

/* Get tag and state of a block. */
int cache_count_block_srripsage(srripsage_data *policy_data, ub1 strm)
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
    head = SRRIPSAGE_DATA_VALID_HEAD(policy_data)[rrpv].head;

    for (node = head; node; node = node->prev)
    {
      assert(node->state != cache_block_invalid);
      if (node->stream == strm)
        count++;
    }
  }

  return count;
}

#undef SET_BLCOKS
#undef IS_SAMPLED_SET
#undef RCY_THR
#undef OP_HIT
#undef OP_FILL
#undef OP_DEMOTE
#undef PSEL_WIDTH
#undef PSEL_MIN_VAL
#undef PSEL_MAX_VAL
#undef PSEL_MID_VAL

