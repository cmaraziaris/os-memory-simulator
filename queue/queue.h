/* queue.h */
#ifndef QUEUE_MODULE
#define QUEUE_MODULE

#include "memory.h"

#define QUEUE_TYPE struct vmem_entry

typedef QUEUE_TYPE queue_item_t;

struct queue_node 
{
  queue_item_t data;
  struct queue_node *next;
};

struct queue
{
  size_t size;
  struct queue_node *front;
  struct queue_node *tail;  
};

struct queue * queue_initialize(void);

int is_queue_empty(struct queue *);
int is_queue_full (struct queue *, size_t);
int queue_search(struct queue *q, queue_item_t value);

queue_item_t queue_remove_first(struct queue *);

void queue_insert_last(struct queue *, queue_item_t);
void queue_sorted_insert(struct queue *q, queue_item_t value);

void queue_destroy(struct queue *);

void queue_print(struct queue *q);



#endif