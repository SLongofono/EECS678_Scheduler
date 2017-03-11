/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"

#define DEBUG 1


void print_q(priqueue_t *q){
	node_t* temp = q->front;
	printf("Queue contents: ");
	while(temp != q->back){
		printf("%d, ", *(int*)temp->value);
		temp = temp->next;
	}
	printf("%d\n", *(int*)temp->value);
}



/**
  Initializes the priqueue_t data structure.
  
  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
	if(DEBUG){
		printf("Init queue...\n");	
	}
	q->size = 0;
	q->front = NULL;
	q->back = NULL;
	q->compare = comparer;
	if(DEBUG){
		printf("Done init queue...\n");	
	}
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
	if(DEBUG){
		printf("Adding a value...\n");	
	}
	int insertion_point = -1;
	if(0 == q->size){
		if(DEBUG){
			printf("No previous vals, adding to front...\n");	
		}

		// Allocate some space on heap
		node_t *temp = malloc(sizeof(node_t));
		q->front = temp;

		// Update front/back ptrs
		q->back = q->front;

		// Fill in given data for the node
		q->front->value = ptr;
		q->front->next = NULL;

		// Update size
		q->size++;

		// Update insertion point
		insertion_point = 0;
	}
	else{
		if(DEBUG){
			printf("Searching for correct position...\n");	
		}

		node_t *temp = q->front;
		node_t *prev = NULL;
		insertion_point = 0;
		while(0 < q->compare(temp, ptr)){
			// proceed until we reach something that has lower
			// priority or the end
			if(DEBUG){
				printf("Position %d has higher priority...\n", insertion_point);	
			}
			if(temp->next != NULL){
				prev = temp;
				temp = temp->next;
				insertion_point++;
			}
			else{
				if(DEBUG){
					printf("Reached end of  queue, adding to back...\n");	
				}
				insertion_point++;
				break;
			}
		}
		// temp now points to what should come after the new value,
		// and insertion point where the new value will reside
		
		
		if(NULL == temp->next){
			// case we hit the end
			temp = malloc(sizeof(node_t));
			q->back->next = temp;
			temp->next = NULL;
			temp->value = ptr;
			q->size++;
			q->back = temp;
			if(DEBUG){
				printf("Added %d to back\n", *(int*)q->back->value);	
			}
		}
		else{
			// case insert a new value
			node_t *temp2 = malloc(sizeof(node_t));
			temp2->value = ptr;
			prev->next = temp2;
			temp2->next = temp;
			q->size++;
			if(DEBUG){
				printf("Added %d to position %d\n", *(int*)temp->value, insertion_point);
			}
		}
	}
	if(DEBUG){
		printf("Inserted at position %d\n", insertion_point);
		print_q(q);
	}
	return insertion_point;
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.

  @note The returned value must be freed
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	if(0 == q->size){
		return NULL;
	}
	else{
		node_t* temp = malloc(sizeof(node_t));
		temp->next = NULL;
		temp->value = q->front->value;
		return temp;
	}
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.

  @note The returned element must be freed properly
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
	if(0 == q->size){
		if(DEBUG){
			printf("No elements to remove...\n");	
		}
		return NULL;
	}
	else if(1 == q->size){
		if(DEBUG){
			printf("Removing %d from front of queue...\n", *(int*)q->front->value);	
			print_q(q);
		}
		// I know this is terrible, but we can't assume the type of
		// this value with the given prompt.
		node_t* temp = q->front;
		q->front = NULL;
		q->back = NULL;
		q->size--;
		if(DEBUG){
			print_q(q);
		}
		return temp->value;
	}
	else{
		if(DEBUG){
			printf("Removing %d from front of queue...\n", *(int*)q->front->value);
			print_q(q);
		}
		node_t* temp = q->front;
		q->front = q->front->next;
		q->size--;
		if(DEBUG){
			print_q(q);
		}
		return temp->value;
	}
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.
 
  @note The returned item must be freed

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
	if(index > q->size){
		return NULL;
	}
	else{
		// Locate the element
		int count =0;
		node_t * temp = q->front;
		node_t * prev = NULL;
		while(count < index){
			prev = temp;
			temp = temp->next;
			count++;
		}

		if(DEBUG){
			printf("Removing element %d from position %d\n", *(int*)temp->value, count);
		}

		// Remove and clean up
		prev->next = temp->next;
		q->size--;

		// Return result
		return temp->value;
	}
}


/**
  Removes all instances of ptr from the queue. 
  
  This function should not use the comparer function, but check if the
  data contained in each element of the queue is equal (==) to ptr.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
	node_t* temp = q->front;
	node_t* prev = NULL;
	int removed = 0;
	while(NULL != temp){
		if(temp->value == ptr){
			// Remove
			if(DEBUG){
				printf("Destroying match for %d\n", *(int*)ptr);
				print_q(q);
			}
			if(temp == q->front){
				prev = temp;
				temp = temp->next;
				q->front = temp;
				free(prev);
				prev = NULL;
			}
			else if(temp == q->back){
				free(temp);
				temp = NULL;
				q->back = prev;
			}
			else{
				node_t* destroy = temp;
				temp = temp->next;
				free(destroy);
			}
			removed++;
			q->size--;
			if(DEBUG){
				print_q(q);
			}
		}
		else{
			prev = temp;
			temp = temp->next;
		}
	}
	return removed;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
	return 0;
}


/**
  Returns the number of elements in the queue.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	return q->size;
}


/**
  Destroys and frees all the memory associated with q.
  
  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
	node_t* temp = q->front;
	while(q->front != q->back){
		temp = priqueue_poll(q);
		free(temp);
	}
	assert(0 == q->size);
	q->front = NULL;
	q->back = NULL;
}
