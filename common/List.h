#ifndef LIST
#define LIST

#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "intermod-common.h"

#ifdef __cplusplus
#define EXPORT_C extern "C" 
#else
#define EXPORT_C
#endif

struct cs_qnode;

typedef struct cs_qnode
{
  ub1      *data;         /* Node data */
  struct cs_qnode *next;  /* Next node */
  struct cs_qnode *prev;  /* Previous node */
}cs_qnode;
  
/* Fixed size queue */
typedef struct cs_fixed_queue
{
  ub4       size;         /* Queue size */
  sb4       head;         /* Head node */
  sb4       tail;         /* Tail node */
  ub4       ele;          /* Elements */
  cs_qnode *queue;        /* Queue head */
}cs_fqueue;

/* Node used in key based data structures */
typedef struct cs_knode
{
  ub8 key;      /* Key used for search */
  ub8 skey;     /* Secondary key for search */
  ub1 *data;    /* Data kept in the node */
}cs_knode;

EXPORT_C void      cs_qinit(cs_qnode *head);
EXPORT_C ub1       cs_qempty(cs_qnode *head);
EXPORT_C cs_qnode* cs_qappend(cs_qnode *head, ub1 *data);
EXPORT_C void      cs_qconcat(cs_qnode *dhead, cs_qnode *shead);
EXPORT_C ub8       cs_qsize(cs_qnode *head);
EXPORT_C void      cs_qdelete(cs_qnode *head, cs_qnode *node);
EXPORT_C ub1*      cs_qdequeue(cs_qnode *head);
EXPORT_C void      attila_map_insert(cs_qnode *htbl, ub8 key, ub8 skey, ub1* data);
EXPORT_C ub1*      attila_map_lookup(cs_qnode *htbl, ub8 key, ub8 skey);
EXPORT_C void      attila_map_delete(cs_qnode *htbl, ub8 key, ub8 skey);


/* Initialize a fixed size queue */
EXPORT_C cs_fqueue* cs_fqinit(ub4 size);

/* Insert a new element in the queue */
EXPORT_C void cs_fqinsert(cs_fqueue *queue, ub1 *data);

/* Delete an element from the queue */
EXPORT_C void cs_fqdelete(cs_fqueue *queue);

/* Check if queue is full */
EXPORT_C ub1 cs_fqfull(cs_fqueue *queue);

/* Check if queue is empty */
EXPORT_C ub1 cs_fqempty(cs_fqueue *queue);

#undef EXPORT_C

#endif
