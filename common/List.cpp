#include "List.h"

/* Initializes a head node */
void cs_qinit(cs_qnode *head)
{
  assert(head);
  head->data = (ub1 *)head;
  head->next = head;
  head->prev = head;
}

/* Initializes a fixed size queue */
cs_fqueue* cs_fqinit(ub4 size)
{
  cs_fqueue *queue;

  assert(size);

  queue = (cs_fqueue *)calloc(1, sizeof(cs_fqueue));
  assert(queue);

  queue->queue = (cs_qnode *)calloc(size, sizeof(cs_qnode));
  assert(queue->queue);

  queue->size = size;
  queue->head = -1;
  queue->tail = -1;
  queue->ele  = 0;

  for (ub4 i = 0; i < size; i++)
  {
    queue->queue[i].data = (ub1*)queue;
  }

  return queue;
}

void cs_fqinsert(cs_fqueue *queue, ub1 *data)
{
  assert(queue);
  assert(queue->ele < queue->size);

  queue->tail = (queue->tail + 1) % queue->size;
  
  if (queue->ele == 0)
  {
    queue->head = queue->tail;
  }

  queue->queue[queue->tail].data = data;
  queue->ele  += 1;
  
  if (queue->ele > 1)
  {
    assert(queue->head != queue->tail);
  }
  else
  {
    assert(queue->head == queue->tail);
  }
}

void cs_fqdelete(cs_fqueue *queue)
{
  assert(queue);
  assert(queue->ele > 0);

  if (queue->ele > 1)
  {
    assert(queue->head != queue->tail);
  }

  /* Delete from the head of the queue */     
  queue->head = (queue->head + 1) % queue->size;
  queue->ele -= 1;

  if (queue->ele <= 1)
  {
    if (queue->ele == 0)
    {
      queue->tail = -1;
      queue->head = -1;
    }

    assert(queue->head == queue->tail);
  }
}

ub1 cs_fqfull(cs_fqueue *queue)
{
  if (queue->ele == queue->size)
  {
    assert((queue->tail + 1) % queue->size == queue->head);
  }

  return (queue->ele == queue->size);
}

ub1 cs_fqempty(cs_fqueue *queue)
{
  if (queue->ele == 0) 
  {
    assert(queue->head == queue->tail);
  }

  return (queue->ele == 0);
}

/* Returns true if list is empty */
ub1 cs_qempty(cs_qnode *head)
{
  assert(head);

  return (head->next == head->prev);
}

/* Appends a node to tail of the list */
cs_qnode * cs_qappend(cs_qnode *head, ub1 *data)
{
  cs_qnode *last;

  assert(head && data);

  /* Get tail of the list from the list head */
  last = (cs_qnode*)(head->data);
  cs_qnode *node = (cs_qnode *)malloc(sizeof(cs_qnode));

#if 0
  cs_qnode *node = (cs_qnode *)cs_malloc(sizeof(cs_qnode));
#endif
  assert(node);

  memset(node, 0, sizeof(cs_qnode));
  
  /* Add new node */
  node->data = data;
  node->next = last->next;
  node->prev = last;
  last->next = node;
  
  head->data = (ub1 *)node;
#if 0
  printf("%s head %lx last %lx node %lx\n", __func__, head, head->data, node);
#endif
  return node;
}

void cs_qconcat(cs_qnode *dhead, cs_qnode *shead)
{
  cs_qnode *dlast;
  cs_qnode *slast;  
  if (!cs_qempty(shead))
  {
    dlast = (cs_qnode *)(dhead->data);
    slast = (cs_qnode *)(shead->data);

    dhead->data = shead->data;  /* Fix last element of new list */
    dlast->next = shead->next;  /* Fix next of the last element of old list */

    (shead->next)->prev = dlast; /* Fix the previous pointer of concat list */
    slast->next = dhead;         /* Fix the last element of concat list */

    cs_qinit(shead);            /* Reinitialize source queue */
  }
}

/* Given head node returns list size */
ub8 cs_qsize(cs_qnode *head)
{
  cs_qnode *i;

  ub8 ncnt = 0;
   
  assert(head);

  for (i = head->next; i != head; i = i->next)
  {
    ncnt++;
  }
  return ncnt;
}

/* Return first node from the list */
ub1* cs_qdequeue(cs_qnode *head)
{
  cs_qnode *node;
  ub1      *data;

  assert(head);
  assert(!cs_qempty(head));

  node = (cs_qnode *)(head->next);
  data = node->data;

  head->next = node->next;
  node->next->prev = head;

  if ((ub1 *)node == head->data)
  {
    head->data = (ub1 *)head;
  }
  
  free(node);

  return data;
}

/* Deletes a node from the list */
void cs_qdelete(cs_qnode *head, cs_qnode *node)
{
  assert(node && head);
#if 0
  free(node->data);
#endif
  node->prev->next = node->next;

  if (node->next)
    node->next->prev = node->prev;

  if ((head->data) == (ub1 *)node)
  {
    head->data = (ub1 *) node->prev;
  }

  free(node);
#if 0
  printf("%s : head %lx last %lx node %lx\n", __func__, head, head->data, node);
#endif
}

/* Key based hash table */

/* Inserts data and an associated key in the hash table */
void attila_map_insert(cs_qnode *htbl, ub8 key, ub8 skey, ub1* data)
{
  cs_knode *knode;
  cs_qnode *head;
  ub8       indx;

  knode = (struct cs_knode *)malloc(sizeof(cs_knode));
  assert(knode);

  knode->key  = key;
  knode->skey = skey;
  knode->data = data;

  indx = ((key & HASHTINDXMASK) >> 6);
  head = &htbl[indx];
#if 0
  printf("%s : inserted into htbl %lx key %lx skey %lx\n", __func__, 
         htbl, key, skey);
#endif
  cs_qappend(head, (ub1 *)knode);
#if 0
  list_all_nodes(htbl, key);
#endif
}

ub1* attila_map_lookup(cs_qnode *htbl, ub8 key, ub8 skey)
{
  ub8       indx;
  ub1      *data; 
  cs_knode *knode;
  cs_qnode *head;
  cs_qnode *i;
  
  assert(htbl);

  data = NULL;
  indx = ((key & HASHTINDXMASK) >> 6);
  head = &htbl[indx];
#if 0  
  if (head->next == head)
  {
    printf("%s : queue empty! lookup for htbl %lx key %lx\n", __func__, htbl, key);
  }
#endif
  for (i = head->next; i != head; i = i->next)
  {
    knode = (cs_knode *)(i->data);
    if (knode->key == key && (knode->skey == skey || skey == ATTILA_MASTER_KEY))
    {
#if 0
      printf("%s : found into %lx key %lx\n", __func__, htbl, key);
#endif
      data = knode->data;
      break;
    }
    else
    {
#if 0
      printf("%s : looked into %lx key %lx knode->key %lx skey %lx knode->skey %lx\n", __func__, 
             htbl, key, knode->key, skey, knode->skey);
#endif
    }
  }

  return data;
}

void attila_map_delete(cs_qnode *htbl, ub8 key, ub8 skey)
{
  ub8       indx;
  cs_knode *knode;
  cs_qnode *i;
  cs_qnode *head;
  ub1       found = 0;
  
  assert(htbl);

  indx = ((key & HASHTINDXMASK) >> 6);
  head = &htbl[indx];

  for (i = head->next; i != head; i = i->next)
  {
    knode = (cs_knode *)(i->data);
    if (knode->key == key && knode->skey == skey)
    {
       cs_qdelete(head, i);
#if 0
       printf("%s : deleted from  %lx key %lx skey %lx\n", __func__, 
        htbl, key, skey);
#endif
       free(knode);
       found = 1;
       break;
    }
  }
#if 0
  printf("\n");
#endif
  assert(found == 1);
}

