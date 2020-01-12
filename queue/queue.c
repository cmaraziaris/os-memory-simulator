/* queue.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

struct queue *queue_initialize(void)
{
  struct queue *q = malloc(sizeof(struct queue)); //?
  assert(q);
  q->size = 0;
  q->front = q->tail = NULL;
  return q;
}

int queue_is_empty(struct queue *q){
  return (q->size ? 0 : 1);
}

int queue_is_full(struct queue *q, size_t maxsize){
  return (q->size == maxsize ? 1 : 0);
}

queue_item_t queue_remove_first(struct queue *q)
{
  --q->size;
  struct queue_node *temp = q->front;
  queue_item_t item = q->front->data;

  q->front = q->front->next;
  free(temp);
  if (q->front == NULL) q->tail = NULL;
  return item;
}

static struct queue_node * create_node(queue_item_t value)
{
  struct queue_node *new_node = malloc(sizeof(struct queue_node));
  assert(new_node);

  new_node->data = value;
  new_node->next = NULL;

  return new_node;
}

// first node is linked after the last one, change value of first node
// requires a full queue => Minimizes allocs/frees done
void queue_emplace_last(struct queue *q, queue_item_t value)
{
  struct queue_node *curr = q->front;

  q->front = q->front->next;

  q->tail->next = curr;

  curr->next = NULL;
  curr->data = value;

  q->tail = curr;
}

void queue_insert_last(struct queue *q, queue_item_t value)
{
  struct queue_node *new_node = create_node(value);
  ++q->size;

  if (q->front == NULL){
    q->front = q->tail = new_node;
  }
  else 
  {
    q->tail->next = new_node;
    q->tail = new_node;    
  }
}

void queue_sorted_insert(struct queue *q, queue_item_t value)
{
  ++q->size;

  if (q->front == NULL){
    q->front = q->tail = create_node(value);
  }
  else
  {
    struct queue_node *temp = q->front;

    while (temp->next)
    {
      if (temp->next->data.addr == value.addr)
      { 
        --q->size;     /* If it already exists in the set, do nothing */
        return; 
      }

      if (temp->next->data.addr > value.addr) break;
      temp = temp->next;
    }

    struct queue_node *new_node = create_node(value);

    if (temp->next == NULL)
    {
      q->tail->next = new_node;
      q->tail = new_node;    
    }
    else
    {
      new_node->next = temp->next;
      temp->next = new_node;
    }
  }
}

// 0 if equal, else 1
static int compare_equal(struct vmem_entry A, struct vmem_entry B)
{
  return ((A.pid == B.pid && A.addr == B.addr) ? 0 : 1);
}

int queue_search(struct queue *q, queue_item_t value)
{
  struct queue_node *temp = q->front;

  while (temp)
  {
    if (compare_equal(temp->data, value) == 0) return 1;
    temp = temp->next;
  }

  return 0;
}


void queue_destroy(struct queue *q)
{
  struct queue_node *current = q->front;
  while (current){
    struct queue_node *temp = current->next;
    free(current);
    current = temp;
  }
  free(q);
}


void queue_print(struct queue *q)
{
  struct queue_node *current = q->front;

  while (current){
    printf("%d\n", current->data.addr);
    current = current->next;
  }
}