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

int is_queue_empty(struct queue *q){
  return (q->size ? 0 : 1);
}

int is_queue_full(struct queue *q, size_t maxsize){
  return (q->size == maxsize ? 1 : 0);
}

queue_item_t queue_remove_first(struct queue *q)
{

// MAKE SURE QUEUE IS NEVER EMPTY
  if (is_queue_empty(q)) exit(EXIT_FAILURE);

  --q->size;
  struct queue_node *temp = q->front;
  queue_item_t item = q->front->data;

  q->front = q->front->next;
  free(temp);
  if (q->front == NULL) q->tail = NULL;
  return item;
}

void queue_insert_last(struct queue *q, queue_item_t value)
{
  struct queue_node *new_node = malloc(sizeof(struct queue_node));
  assert(new_node);

  ++q->size;
  new_node->data = value;
  new_node->next = NULL;
  if (q->front == NULL){
    q->front = q->tail = new_node;
  }
  else 
  {
    q->tail->next = new_node;
    q->tail = new_node;    
  }
}

void queue_sorted_insert(struct queue *q, queue_item_t value) // TODO: add compare
{
  struct queue_node *new_node = malloc(sizeof(struct queue_node));
  assert(new_node);

  ++q->size;
  new_node->data = value;
  new_node->next = NULL;

  if (q->front == NULL){
    q->front = q->tail = new_node;
  }
  else
  {
    struct queue_node *temp = q->front;

    while (temp->next)
    {
      if (temp->next->data.addr == value.addr) { free(new_node); --q->size; return; }// no duplicates
      if (temp->next->data.addr > value.addr) break;
      temp = temp->next;
    }

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

//change according to your data types
static void gen_free(struct queue_node *node){
  free(node); // efoson dn exoume array san data type
}

void queue_destroy(struct queue *q)
{
  struct queue_node *current = q->front;
  while (current){
    struct queue_node *temp = current->next;
    gen_free(current);
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