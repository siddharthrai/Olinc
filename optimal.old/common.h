typedef signed   long   sb8;
typedef unsigned long   ub8;
typedef signed   int    sb4;
typedef unsigned int    ub4;
typedef signed   short  sb2;
typedef unsigned short  ub2;
typedef          char   sb1;
typedef unsigned char   ub1;

sb1 * cachesim_malloc(ub1 sz);

typedef struct MtrcInfo
{
  ub8 sn;    /* Sequence number */
  ub1 u[3];  /* Unit name (IC / ZC / CC / TC) */
  ub1 t[2];  /* Access type (read / write ) */
  ub8 addr;  /* Referenced address */
  ub8 rdis;  /* Reuse distance */
}MtrcInfo;

/* Address information the goes into hash table */
typedef struct cachesim_ainf
{
  ub8 sn;   /* Sequence number */
  ub8 addr; /* Address */
}cachesim_ainf;

/* Page descriptor used for allocating paged memory */
typedef struct cachesim_pd
{
  ub8 cur; /* Current address in page */
  ub8 end; /* End address in page */
}cachesim_pd;

/* Structure to hold recency data structure is almost same as trcinfo 
 * except one new field that stores file position */

typedef struct cachesim_md
{
  cachesim_ainf trcinfo; /* Memory trace info */
}cachesim_md;

struct cachesim_qnode;

typedef struct cachesim_qnode
{
  ub1            *data; /* Data */
  cachesim_qnode *next; /* Next node */
  cachesim_qnode *prev; /* Previous node */ 
}cachesim_qnode;

/* Initializes a head node */
void cachesim_qinit(cachesim_qnode *head)
{
  assert(head);
  head->data = (ub1 *)head;
  head->next = NULL;
  head->prev = NULL;
}

/* Returns true if list is empty */
ub1 cachesim_qempty(cachesim_qnode *head)
{
  assert(head);

  return (head->next == head->prev);
}

/* Appends a node to tail of the list */
cachesim_qnode * cachesim_qappend(cachesim_qnode *head, ub1 *data)
{
  cachesim_qnode *last;

  assert(head && data);
  /* Get tail of the list stored in list head */
  last = (cachesim_qnode*)head->data;
  cachesim_qnode *node = (cachesim_qnode *)malloc(sizeof(cachesim_qnode));

#if 0
  cachesim_qnode *node = (cachesim_qnode *)cachesim_malloc(sizeof(cachesim_qnode));
#endif
  assert(node);

  memset(node, 0, sizeof(cachesim_qnode));
  
  /* Add new node */
  node->data = data;
  node->next = last->next;
  node->prev = last;
  last->next = node;
  
  ((cachesim_qnode*)head)->data = (ub1 *)node;

  return node;
}

/* Given head node returns list sie */
ub8 cachesim_qsize(cachesim_qnode *head)
{
  cachesim_qnode *i;

  ub8 ncnt = 0;

  for (i = head->next; i != NULL; i = i->next)
  {
    ncnt++;
  }
  return ncnt;
}

/* Deletes a node from the list */
void cachesim_qdelete(cachesim_qnode *node)
{
  free(node->data);

  node->prev->next = node->next;

  if (node->next)
    node->next->prev = node->prev;
  free(node);
}
