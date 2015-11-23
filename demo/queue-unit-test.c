#include <stdio.h>
#include "../common/List.h"

#define Q_SIZE  (100)
#define Q_SMALL (50)

int main()
{
  cs_fqueue *queue;
  ub4 i;

  queue = cs_fqinit(Q_SIZE);
  assert(queue);

  /* Insert n elements in the queue  */
  for (i = 0 ; i < Q_SIZE; i++)
  {
    cs_fqinsert(queue, (ub1 *)0xdead);
  }
  
  assert(cs_fqfull(queue));

  /* Delete m elements in the queue */
  for (i = 0 ; i < Q_SIZE; i++)
  {
    cs_fqdelete(queue);
  }

  assert(cs_fqempty(queue));
  
  /* Insert n elements in the queue  */
  for (i = 0 ; i < Q_SIZE; i++)
  {
    cs_fqinsert(queue, (ub1 *)0xdead);
  }
  
  assert(cs_fqfull(queue));

  /* Delete m elements in the queue */
  for (i = 0 ; i < Q_SMALL; i++)
  {
    cs_fqdelete(queue);
  }

  assert(!cs_fqfull(queue));
  assert(!cs_fqempty(queue));

  /* Delete m elements in the queue */
  for (i = 0 ; i < Q_SMALL; i++)
  {
    cs_fqdelete(queue);
  }

  assert(!cs_fqfull(queue));
  assert(cs_fqempty(queue));
  
  printf("Unit test passed!!\n");

  return 0;
}

#undef Q_SIZE
#undef Q_SMALL
