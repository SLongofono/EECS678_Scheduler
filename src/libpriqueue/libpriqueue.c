/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"

#define DEBUG 0


void print_q(priqueue_t *q){
	node_t* temp = q->front;
	printf("Queue contents: ");
	while(temp != NULL){
		printf("%d ", *(int*)temp->value);
		temp = temp->next;
	}
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
	int insertion_point = 0;
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
	}
	else{
		node_t *temp = q->front;
		node_t *prev = NULL;
		if(DEBUG){
			printf("Searching for correct position for new value %d...\n", *(int*)ptr);
		}

		// Two compares in use: 
		// 	c1(a,b) -> a-b (lowest takes priority)
		// 	c2(a,b) -> b-a (highest takes priority)
		while(NULL != temp && 0 >= q->compare(temp->value, ptr)){
			// proceed until we reach something that has lower
			// priority or the end
			if(DEBUG){
				printf("%d has higher priority than %d...\n", *(int*)temp->value, *(int*)ptr);	
			}
			prev = temp;
			temp = temp->next;
			insertion_point++;
		}
	
		if(NULL == temp){
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
			if(q->front == temp){
				q->front = temp2;
				temp2->next = temp;
			}
			else{
				temp2->next = temp;
				prev->next = temp2;
			}
			q->size++;
			if(DEBUG){
				printf("Added %d to position %d\n", *(int*)temp2->value, insertion_point);
			}
		}
		if(DEBUG){
			print_q(q);	
		}
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
		//node_t* temp = malloc(sizeof(node_t));
		//temp->next = NULL;
		//temp->value = q->front->value;
		return q->front->value;
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
		//node_t* temp = q->front;
		void *ret = q->front->value;
		free(q->front);
		q->front = NULL;
		q->back = NULL;
		q->size--;
		if(DEBUG){
			print_q(q);
			//printf("\n\nPlease remember to free this...\n\n");
		}
		return ret;
	}
	else{
		if(DEBUG){
			printf("Removing %d from front of queue...\n", *(int*)q->front->value);
			print_q(q);
		}
		node_t* temp = q->front;
		void *ret = q->front->value;
		q->front = q->front->next;
		free(temp);
		q->size--;
		if(DEBUG){
			print_q(q);
			//printf("\n\nPlease remember to free this...\n\n");
		}
		return ret;
	}
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.
 
  @note The item remains in the queue - this behavior was not explicitly stated,
  	but through trial and error it appears that this is the intended behavior

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{

	// Check size
	if(index > q->size){
		printf("INVALID SIZE\n");
	}
	else{
		
		// Locate the element
		int count = 0;
		node_t * temp = q->front;

		while(count < index && NULL != temp ){
			temp = temp->next;
			count++;
		}

		if(NULL != temp){

			if(DEBUG){
				printf("Found element %d at position %d\n", *(int*)temp->value, count);
			}

			return (void *)temp->value;
		}
	}

	return NULL;
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
				printf("\n\nFREEING!\n\n");
			}
			if(1 == q->size){
				free(temp);
				q->front = NULL;
				q->back = NULL;
			}
			else if(temp == q->front){
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
				q->back->next=NULL;
			}
			else{
				node_t* destroy = temp;
				temp = temp->next;
				free(destroy);
				prev->next = temp;
			}
			removed++;
			q->size--;
			if(DEBUG){
				print_q(q);
			}
		}
		else{
			if(DEBUG){
				printf("%d does not match pattern %d, moving on...\n", *(int*)temp->value, *(int*)ptr);	
			}
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
	if(q->size < index || index < 0){
		return NULL;	
	}
	int count = 0;
	void *ret = NULL;
	node_t *temp = q->front;
	node_t * prev = NULL;
	while(count != index){
		prev = temp;
		temp = temp->next;
		count++;
	}
	if(temp != NULL){
		if(prev != NULL){
			prev->next = temp->next;
		}
		else{
			q->front = temp->next;	
		}
		ret = (void *)temp->value;
		free(temp);
		q->size--;
	}
	return ret;

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
	node_t *temp;
	while(q->front != NULL){
		temp = q->front->next;
		free(q->front);
		q->front = temp;
		q->size--;
	}
	assert(0 == q->size);
	q->front = NULL;
	q->back = NULL;
}
