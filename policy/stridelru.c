/*
 *  Copyright (C) 2011  Rafael Ubal (ubal@ece.neu.edu)
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
#include <math.h>

#include "libstruct/misc.h"
#include "libstruct/string.h"
#include "libmhandle/mhandle.h"
#include "stridelru.h"

#define PAGE_SIZE   (4096)  
#define STRIDE_BITS (10)
#define BSIZE       (8)
#define LBSIZE      (3)

#define CACHE_SET(cache, set)  (&((cache)->sets[set]))
#define CACHE_BLOCK(set, way)  (&((set)->blocks[way]))

#define CACHE_LONG_MASK(p)     ((ub4)(0x1) << (p))
#define CACHE_LONG_BIV(bv, p)  (((bv)[(p - 1) >> LBSIZE] & CACHE_LONG_MASK((p - 1) % BSIZE)) >> ((p - 1) % BSIZE))
#define CACHE_LONG_BIS(bv, p)  ((bv)[(p - 1) >> LBSIZE] |= CACHE_LONG_MASK((p - 1) % BSIZE))
#define CACHE_LONG_BIC(bv, p)  ((bv)[(p - 1) >> LBSIZE] &= ~CACHE_LONG_MASK((p - 1) % BSIZE))

#define CACHE_LONG_SFL(bv, p)                                   \
do                                                              \
{                                                               \
  unsigned char tmp;                                            \
  unsigned int  end;                                            \
  tmp = 0;                                                      \
  end = pow(2, STRIDE_BITS - LBSIZE) - 1;                       \
  for (int i = pow(2, STRIDE_BITS) - (1 + p), j = end; i >= 0;) \
  {                                                             \
    if (i < BSIZE)                                              \
    {                                                           \
      tmp = get_from_source(bv, 0, i);                          \
      i = -1;                                                   \
    }                                                           \
    else                                                        \
    {                                                           \
      tmp = get_from_source(bv, i - BSIZE + 1, i);              \
      i -= BSIZE;                                               \
    }                                                           \
                                                                \
    bv[j--] = tmp;                                              \
  }                                                             \
}while(0)

#define CACHE_LONG_RST(bv)                                      \
do                                                              \
{                                                               \
  for (int i = 0; i < pow(2, STRIDE_BITS - LBSIZE); i++)        \
  {                                                             \
    (bv)[i] = 0;                                                \
  }                                                             \
}while(0)

unsigned int count_bit_set(ub1 *bv)
{ 
  unsigned char mask;
  unsigned char tmp;
  unsigned int  bits;

  bits = 0;

  for (int i = 0; i < pow(2, STRIDE_BITS - LBSIZE); i++)
  {
    tmp   = bv[i];
    mask  = 0x1;

    while (mask)
    {
      bits += (tmp & mask) ? 1 : 0; 
      mask <<= 1;
    }
  }

  return bits;
}

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
    
    /* Obtain the value and make it aligned towards MSB */
    result |= (bv[start_idx] & start_mask) << (BSIZE - 1 - end_off);

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

  return result;
}

#define CACHE_UPDATE_BLOCK_STATE(block, tag, state)           \
do                                                            \
{                                                             \
  (block)->tag      = (tag);                                  \
  (block)->tag_end  = (tag + 1);                              \
  (block)->state    = (state);                                \
}while(0)

#define CACHE_GET_BLOCK_STREAM(block, strm)                   \
do                                                            \
{                                                             \
  strm = (block)->stream;                                     \
}while(0)

#define CACHE_UPDATE_BLOCK_STREAM(block, strm)                \
do                                                            \
{                                                             \
  (block)->stream = strm;                                     \
}while(0)

/*
 * Public Variables
 */

/*
 * Private Functions
 */
#define free_list_remove_block(set, blk)                                                \
do                                                                                      \
{                                                                                       \
        /* Check: free list must be non empty as it contains the current block. */      \
        assert(STRIDELRU_DATA_FREE_HEAD(set) && STRIDELRU_DATA_FREE_TAIL(set));         \
                                                                                        \
        /* Check: current block must be in invalid state */                             \
        assert((blk)->state == cache_block_invalid);                                    \
                                                                                        \
        CACHE_REMOVE_FROM_SQUEUE(blk, STRIDELRU_DATA_FREE_HEAD(set), STRIDELRU_DATA_FREE_TAIL(set));\
                                                                                        \
        (blk)->next = NULL;                                                             \
        (blk)->prev = NULL;                                                             \
                                                                                        \
        /* Reset block state */                                                         \
        (blk)->busy = 0;                                                                \
        (blk)->cached = 0;                                                              \
        (blk)->prefetch = 0;                                                            \
}while(0);

static unsigned int get_next_power_of_2(unsigned int val)
{
  if (val > 0)
  {
    val--;

    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;

    val++;
  }

  return val;
}

void cache_init_stridelru(struct cache_params *params, stridelru_data *policy_data)
{
  /* Ensure valid arguments */
  assert(params);
  assert(policy_data);

#define MEM_ALLOC(size) (xcalloc(size, sizeof(stridelru_list)))
  
  /* Allocate list heads */
  STRIDELRU_DATA_VALID_HLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(STRIDELRU_DATA_VALID_HLST(policy_data));

  STRIDELRU_DATA_VALID_TLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(STRIDELRU_DATA_VALID_TLST(policy_data));

  STRIDELRU_DATA_FREE_HLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(STRIDELRU_DATA_FREE_HLST(policy_data));

  STRIDELRU_DATA_FREE_TLST(policy_data) = (list_head_t *)xcalloc(1, sizeof(list_head_t));
  assert(STRIDELRU_DATA_FREE_TLST(policy_data));

  /* Initialize valid head and tail pointers */
  STRIDELRU_DATA_VALID_HEAD(policy_data) = NULL;
  STRIDELRU_DATA_VALID_TAIL(policy_data) = NULL;

  STRIDELRU_DATA_BLOCKS(policy_data) =
    (struct cache_block_t *)xcalloc(params->ways, sizeof (struct cache_block_t));
  assert(STRIDELRU_DATA_BLOCKS(policy_data));

  /* Initialize array of blocks */
  STRIDELRU_DATA_FREE_HEAD(policy_data) = &(STRIDELRU_DATA_BLOCKS(policy_data)[0]);
  STRIDELRU_DATA_FREE_TAIL(policy_data) = &(STRIDELRU_DATA_BLOCKS(policy_data)[params->ways - 1]);

  for (int way = 0; way < params->ways; way++)
  {
    (&STRIDELRU_DATA_BLOCKS(policy_data)[way])->way   = way;
    (&STRIDELRU_DATA_BLOCKS(policy_data)[way])->state = cache_block_invalid;
    (&STRIDELRU_DATA_BLOCKS(policy_data)[way])->next  = way ? 
      (&STRIDELRU_DATA_BLOCKS(policy_data)[way - 1]) : NULL;
    (&STRIDELRU_DATA_BLOCKS(policy_data)[way])->prev  = way < params->ways - 1 ? 
      (&STRIDELRU_DATA_BLOCKS(policy_data)[way + 1]) : NULL;
    
    /* Allocate long bit vectors if requested */
    if (params->use_long_bv)
    {
      /* Allocate bit vector for page map */
      int bit_vector_bytes = pow(2, STRIDE_BITS - 3);
      (&STRIDELRU_DATA_BLOCKS(policy_data)[way])->long_use = xcalloc(bit_vector_bytes, sizeof(unsigned char));
      assert((&STRIDELRU_DATA_BLOCKS(policy_data)[way])->long_use);

      /* Allocate bit vector for page map */
      bit_vector_bytes = pow(2, STRIDE_BITS - 3);
      (&STRIDELRU_DATA_BLOCKS(policy_data)[way])->long_hit = xcalloc(bit_vector_bytes, sizeof(unsigned char));
      assert((&STRIDELRU_DATA_BLOCKS(policy_data)[way])->long_hit);
    }
    else
    {
      (&STRIDELRU_DATA_BLOCKS(policy_data)[way])->long_use = NULL;
      (&STRIDELRU_DATA_BLOCKS(policy_data)[way])->long_hit = NULL;
    }
  }

  policy_data->ways             = params->ways;       /* Number of ways in a set */
  policy_data->stream           = params->stl_stream; /* Source stream for the cache */
  policy_data->out_of_reach     = 0;                  /* Initialize access to out of range regions */
  policy_data->recycle_blocks   = FALSE;              /* Blocks recycled */
  policy_data->rpl_on_miss      = params->rpl_on_miss;/* True, if block is replaced on miss */

#undef MEM_ALLOC
}

/* Free all blocks, sets, head and tail buckets */
void cache_free_stridelru(stridelru_data *policy_data)
{
  /* Ensure valid arguments */
  assert(policy_data);

  /* Free all data blocks */
  free(STRIDELRU_DATA_BLOCKS(policy_data));
}


struct cache_block_t* cache_find_block_stridelru(stridelru_data *policy_data, 
    long long tag, memory_trace *info)
{
  struct cache_block_t *head;
  struct cache_block_t *tail;
  struct cache_block_t *node;
  struct cache_block_t *free_head;
  struct cache_block_t *nearest_node;
  
  ub4 distance;
  ub4 valid_blocks;

  /* Ensure valid arguments */
  assert(policy_data);

  distance          = 0xffffffff;
  nearest_node      = NULL;
  valid_blocks      = 0; 

  head      = STRIDELRU_DATA_VALID_HEAD(policy_data);
  tail      = STRIDELRU_DATA_VALID_TAIL(policy_data);
  free_head = STRIDELRU_DATA_FREE_HEAD(policy_data);
  
  for (node = head; node; node = node->prev)
  {
    assert(node->state != cache_block_invalid);

    if (distance > abs(node->tag - tag))
    {
      distance      = abs(node->tag - tag);
      nearest_node  = node;
    }

    if (distance > abs(node->tag_end - tag))
    {
      distance      = abs(node->tag_end - tag);
      nearest_node  = node;
    }
    
    if (tag >= node->tag && tag <= node->tag_end)
    {
      break;
    }

    valid_blocks++;
  }
  
  /* If all ways have valid block, there is miss and a nearest node is found */
  if (free_head == NULL && node == NULL && nearest_node && info->stream == policy_data->stream)
  {
    /* If access has not been out of reach for w times and access is not in 
     * replacement mode */
    if (policy_data->out_of_reach < policy_data->ways && 
        policy_data->recycle_blocks == FALSE)
    {
      /* If distance between current bock and any region is above 4K pages consider 
       * access as out of reach */
      if (distance > 4096)
      {
        /* Increment out of reach counter if block is either above min way or 
         * is below max way */
        if (nearest_node->way == head->way || nearest_node->way == tail->way)
        {
          (policy_data->out_of_reach)++;
        }
        else
        {
          (policy_data->out_of_reach)--;
        }

        if ((policy_data->out_of_reach) == policy_data->ways)
        {
          policy_data->recycle_blocks = TRUE;
        }
      }

#define STRIDE_THRES ((ub4)0x1 << STRIDE_BITS)

      if (policy_data->rpl_on_miss)
      {
        if ((nearest_node->tag_end - nearest_node->tag + distance) < STRIDE_THRES)
        {
          node = nearest_node;
        }
      }
      else
      {
        node = nearest_node;
      }

#undef STRIDE_THRES

    }
    else
    {
      if (policy_data->out_of_reach)
      {
        (policy_data->out_of_reach)--;
      }

      if (policy_data->out_of_reach == 0)
      {
        policy_data->recycle_blocks = FALSE; 
      }
    }
  }

  ub4 region_offset;
  ub4 bit_pos;

#define BIT_MASK(p)   ((ub4)(0x1) << (p))
#define BIT_VAL(v, p) (((v) & BIT_MASK(p)) >> (p))
#define BIT_BIS(v, p) ((v) = (v) | BIT_MASK(p))
#define BIT_BIC(v, p) ((v) = (v) & ~BIT_MASK(p))
  
  /* If, access was hit and accessing stream was not the source 
   * stream lookup bit vector to decide hit / miss */
  if (node && info->stream != policy_data->stream)
  {
    assert(info->address >= node->tag);

#if 0
    region_offset = info->address - node->tag;
    bit_pos       = (ub4)log2(get_next_power_of_2(region_offset));

    BIT_BIS(node->hit_bitmap, bit_pos);
    
    if (node->long_hit)
    {
      if (!CACHE_LONG_BIV(node->long_hit, region_offset + 1))
      {
        CACHE_LONG_BIS(node->long_hit, region_offset + 1);
      }
    }
    /* If bit is not set consider access as miss */
    if (!BIT_VAL(node->use_bitmap, bit_pos))
    {
      node = NULL;
    }
#endif
  }


#undef BIT_MASK
#undef BIT_VAL
#undef BIT_BIC
#undef BIT_BIS

  if (node)
  {
    if (info->stream != policy_data->stream)
    {
      (node->x_stream_use)++;
    }
    else
    {
      (node->s_stream_use)++;
    }
  }

  return node;
}

void cache_fill_block_stridelru(stridelru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, int strm, memory_trace *info)
{
  struct cache_block_t *block;

  /* Check: tag, state and insertion_position are valid */
  assert(policy_data);
  assert(tag >= 0);
  assert(state != cache_block_invalid);

  /* Obtain block from STRIDELRU specific data */
  block = &(STRIDELRU_DATA_BLOCKS(policy_data)[way]);

  /* Remove block from free list */
  free_list_remove_block(policy_data, block);
  
  /* Update new block state and stream */
  CACHE_UPDATE_BLOCK_STATE(block, tag, state);
  CACHE_UPDATE_BLOCK_STREAM(block, strm);

  block->dirty = (info && info->spill) ? 1 : 0;

  block->epoch        = 0;
  block->use          = 0;
  block->use_bitmap   = get_next_power_of_2(1);

  if (block->use > block->max_use)
  {
    block->max_use  = 1;
  }

  if ((block->tag_end - block->tag) > block->max_stride)
  {
    block->max_stride = 1;
  }
  
  if (block->long_use)
  {
    CACHE_LONG_BIS(block->long_use, 1);
  }

  /* Insert block to the head of the queue */
  CACHE_PREPEND_TO_SQUEUE(block, STRIDELRU_DATA_VALID_HEAD(policy_data), 
    STRIDELRU_DATA_VALID_TAIL(policy_data));
}

int cache_replace_block_stridelru(stridelru_data *policy_data)
{
  struct cache_block_t *block;
  
  /* Ensure valid argument */
  assert(policy_data);

  /* Try to find an invalid block. */
  for (block = STRIDELRU_DATA_FREE_HEAD(policy_data); block; block = block->next)
    return block->way;

  /* Remove a nonbusy block from the tail */
  for (block = STRIDELRU_DATA_VALID_TAIL(policy_data); block; block = block->prev)
  {
    if (!block->busy)
    {
      if ((block->tag_end - block->tag) > block->max_stride)
      {
        block->max_stride     = block->tag_end - block->tag + 1;
        block->max_use_bitmap = block->use_bitmap;
      }

      if (block->use > block->max_use)
      {
        block->max_use = block->use;
      }

      if (block->hit_bitmap > block->max_hit_bitmap)
      {
        block->max_hit_bitmap = block->hit_bitmap;
      }
      
      if (block->long_use)
      {
        assert(count_bit_set(block->long_use) <= block->tag_end - block->tag + 1);

        /* Reset long bit vector */
        CACHE_LONG_RST(block->long_use);
      }

      if (block->long_hit)
      {
        assert(count_bit_set(block->long_hit) <= block->tag_end - block->tag + 1);

        /* Reset long bit vector */
        CACHE_LONG_RST(block->long_hit);
      }

      block->use_bitmap    = 0;
      block->hit_bitmap    = 0;
      block->use           = 0;
      block->replacements += 1;
      
      return block->way;
    }
  }

  /* If no non busy block can be found, return -1 */
  return -1;
}

void cache_access_block_stridelru(stridelru_data *policy_data, int way, int strm,
  memory_trace *info)
{
  struct cache_block_t *blk   = NULL;
  struct cache_block_t *next  = NULL;
  struct cache_block_t *prev  = NULL;

  blk  = &(STRIDELRU_DATA_BLOCKS(policy_data)[way]);
  prev = blk->prev;
  next = blk->next;

  /* Check: block's tag and state are valid */
  assert(policy_data);
  assert(blk->tag >= 0);
  assert(blk->state != cache_block_invalid);

  /* Update epoch only for demand reads */
  if (info && info->fill == TRUE)
  {
    if (blk->stream == info->stream)
    {
      blk->epoch = (blk->epoch == 3) ? 3 : blk->epoch + 1;
    }
    else
    {
      blk->epoch = 0;
    }
  }

  ub4 region_size; /* Expansion in the region */
  ub4 next_power;  /* Next power of 2 of region size */
  
  if (info->stream == policy_data->stream)
  {
    /* Based on new tag and old tag, decide how to extend the region */
    if (info->address < blk->tag)
    {
      /* Region is extended in backward direction. Get next power of 2 of the 
       * added size, shift bitmap left by those many bits and set lsb. */
      region_size = blk->tag - info->address;

#define BIT_LENGTH    (STRIDE_BITS)
#define BIT_MASK(p)   ((ub4)(0x1) << (p))
#define BIT_VAL(v, p) (((v) & BIT_MASK(p)) >> (p))
#define BIT_BIS(v, p) ((v) = (v) | BIT_MASK(p))
#define BIT_BIC(v, p) ((v) = (v) & ~BIT_MASK(p))

      ub1 if_bit_set;
      ub4 org_bitmap; 
      ub4 start_val;
      ub4 end_val;
      ub4 current_val;

      /* Make a copy of original bitmap */
      org_bitmap = blk->use_bitmap;

      for (ub4 i = 0; i <= BIT_LENGTH; i++)
      {
        /* Get bit state (set / reset) */
        if_bit_set = BIT_VAL(org_bitmap, i);

        /* If bit is set find its value, add expand size to it and
         * set bit of the new region */
        if (if_bit_set)
        {
          start_val = (ub4)pow(2, (i) ? i - 1 : 0);
          end_val   = (ub4)pow(2, i);

          /* Clear i_th bit */
          BIT_BIC(blk->use_bitmap, i); 

          for (ub4 val = start_val + 1; val <= end_val; val++)
          {
#if 0        
            if ((blk->tag_end - info->address + 1) > get_next_power_of_2(val + region_size))
#endif
            {
              /* Find new region location */
              current_val = val + region_size;

              /* Get next power of 2 for the new region */
              next_power = (ub4)log2(get_next_power_of_2(current_val));
              assert(next_power > 0);

              if (next_power <= BIT_LENGTH)
              {
                /* Set corresponding bit */
                BIT_BIS(blk->use_bitmap, next_power);
              }
            }
          }
        }
      }
      
      /* Shift and set long bit-vector */
      if (blk->long_use)
      {
        CACHE_LONG_SFL(blk->long_use, region_size);
        CACHE_LONG_BIS(blk->long_use, 1);
      }

      /* Finally, set LSB */
      blk->use_bitmap |= get_next_power_of_2(1);
      
#undef BIT_LENGTH
#undef BIT_MASK
#undef BIT_VAL
#undef BIT_BIS
#undef BIT_BIC

      blk->tag     = info->address;
      blk->expand += 1;
      
      if (blk->long_use)
      {
        assert(count_bit_set(blk->long_use) <= blk->tag_end - blk->tag + 1);
      }
    }
    else
    {
      if (info->address > blk->tag_end)
      {
        /* Region is extended in backward direction. Get next power of 2 of the 
         * added size, shift bitmap left by those many bits and set lsb. */
        region_size = info->address - blk->tag;

        next_power  = get_next_power_of_2(region_size);
        assert(next_power > 0);

        /* Shift bitmap left to accomodate increased size */
        blk->use_bitmap |= next_power;

        blk->tag_end  = info->address;
        blk->expand  += 1;
        
        if (blk->long_use)
        {
          assert(count_bit_set(blk->long_use) <= blk->tag_end - blk->tag + 1);

          /* Set bit corresponding to region size in long bitmap */
          if (!CACHE_LONG_BIV(blk->long_use, region_size + 1))
          {
            CACHE_LONG_BIS(blk->long_use, region_size + 1);
          }

          assert(count_bit_set(blk->long_use) <= blk->tag_end - blk->tag + 1);
        }
      }
      else
      {
        assert(info->address >= blk->tag && info->address <= blk->tag_end);
        
        /* If acess is to a new page */
        if (info->address != blk->tag)
        {
          region_size = info->address - blk->tag;

          next_power  = get_next_power_of_2(region_size);
          assert(next_power > 0);

          /* Shift bitmap left to accomodate increased size */
          blk->use_bitmap |= next_power;
          
          if (blk->long_use)
          {
            assert(count_bit_set(blk->long_use) <= blk->tag_end - blk->tag + 1);

            if (!CACHE_LONG_BIV(blk->long_use, region_size + 1))
            {
              /* Set bit corresponding to region size in long bitmap */
              CACHE_LONG_BIS(blk->long_use, region_size + 1);
            }

            assert(count_bit_set(blk->long_use) <= blk->tag_end - blk->tag + 1);
          }
        }

        blk->hit  += 1;
      }
    }

    if ((blk->tag_end - blk->tag + 1) > blk->max_stride)
    {
      blk->max_stride     = blk->tag_end - blk->tag + 1;
      blk->max_use_bitmap = blk->use_bitmap;
    }

    if (blk->use > blk->max_use)
    {
      blk->max_use = blk->use;
    }

#if 0
    assert(blk->max_use_bitmap < (get_next_power_of_2(blk->max_stride) << 1));
    assert(blk->use_bitmap < (get_next_power_of_2(blk->tag_end - blk->tag + 1) << 1));
#endif
    
    if (blk->long_hit)
    {
      if (blk->stream != info->stream && count_bit_set(blk->long_hit) > 0)
      {
        blk->color_transition += 1;
      }
    }
  }
  else
  {
    blk->use          += 1;
    blk->captured_use += 1;
  }

  CACHE_UPDATE_BLOCK_STREAM(blk, strm);
  blk->dirty = (info && info->spill) ? 1 : 0;

  /* If block is not at the head of the valid list */
  if (blk->next)
  {
    /* Update block queue if block got new RRPV */
    CACHE_REMOVE_FROM_SQUEUE(blk, STRIDELRU_DATA_VALID_HEAD(policy_data),
        STRIDELRU_DATA_VALID_TAIL(policy_data));
    CACHE_PREPEND_TO_SQUEUE(blk, STRIDELRU_DATA_VALID_HEAD(policy_data), 
        STRIDELRU_DATA_VALID_TAIL(policy_data));
  }
}

/* Update state of block. */
void cache_set_block_stridelru(stridelru_data *policy_data, int way, long long tag,
  enum cache_block_state_t state, ub1 stream, memory_trace *info)
{
        struct cache_block_t *block;
        
        /* Ensure valid arguments */
        assert(policy_data);
        assert(tag >= 0);

        block = &(STRIDELRU_DATA_BLOCKS(policy_data))[way];

        /* Check: tag matches with the block's tag. */
        assert(block->tag == tag);

        /* Check: block must be in valid state. It is not possible to set
         * state for an invalid block.*/
        assert(block->state != cache_block_invalid);
        
        /* Assign access stream */
        block->stream = stream;

        if (state != cache_block_invalid)
        {
          block->state = state;
          return;
        }
          
        assert(block->state != cache_block_invalid);

        /* Invalidate block */
        block->state = cache_block_invalid;
        block->epoch = 0;

        /* Remove block from valid list and insert into free list */
        CACHE_REMOVE_FROM_SQUEUE(block, STRIDELRU_DATA_VALID_HEAD(policy_data),
          STRIDELRU_DATA_VALID_TAIL(policy_data));
        CACHE_APPEND_TO_SQUEUE(block, STRIDELRU_DATA_FREE_HEAD(policy_data), 
          STRIDELRU_DATA_FREE_TAIL(policy_data));
}

/* Get tag and state of a block. */
struct cache_block_t cache_get_block_stridelru(stridelru_data *policy_data, int way,
  long long *tag_ptr, enum cache_block_state_t *state_ptr, int *stream_ptr)
{
  /* Ensure valid arguments */
  assert(policy_data);      
  assert(tag_ptr);
  assert(state_ptr);
  assert(stream_ptr);

  PTR_ASSIGN(tag_ptr, (STRIDELRU_DATA_BLOCKS(policy_data)[way]).tag);
  PTR_ASSIGN(state_ptr, (STRIDELRU_DATA_BLOCKS(policy_data)[way]).state);
  PTR_ASSIGN(stream_ptr, (STRIDELRU_DATA_BLOCKS(policy_data)[way]).stream);

  return STRIDELRU_DATA_BLOCKS(policy_data)[way];
}

#undef PAGE_SIZE
#undef CACHE_BIT_MASK
#undef CACHE_LONG_BIV
#undef CACHE_LONG_BIS
#undef CACHE_LONG_BIC
