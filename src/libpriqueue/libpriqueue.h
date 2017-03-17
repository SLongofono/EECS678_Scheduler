/** @file libpriqueue.h
 */

#ifndef LIBPRIQUEUE_H_
#define LIBPRIQUEUE_H_
#include <assert.h>


/**
 * Node data structure
 */
typedef struct node_t{
	void *value;
	struct node_t *next;
} node_t;


/**
 * Priority queue data structure
 */
typedef struct _priqueue_t
{
	node_t *front;
	node_t * back;
	int size;
	int (*compare)(const void *, const void *);

} priqueue_t;

/**
 * @brief Initializer for a priority queue element, called once immediately
 * after creating it.
 *
 * @param q	A pointer to an instance of the priqueue_t type
 * @param comparer	A function pointer to be used when comparing this
 * pidqueue_t type.
 */
void   priqueue_init     (priqueue_t *q, int(*comparer)(const void *, const void *));


/**
 * @brief Inserts the value at the given address into this queue, and returns
 * the value of the index it was inserted at.
 */
int    priqueue_offer    (priqueue_t *q, void *ptr);


/**
 * @brief Returns the value of the frontmost element in the queue
 */
void * priqueue_peek     (priqueue_t *q);


/**
 * @brief Removes and returns the frontmost element in the queue, or returns
 * NULL if the queue is empty
 */
void * priqueue_poll     (priqueue_t *q);


/**
 * @brief Return the element at the specified index, or NULL if no such
 * element exists
 */
void * priqueue_at       (priqueue_t *q, int index);


/**
 * @brief Remove all elements with the given value at ptr
 */
int    priqueue_remove   (priqueue_t *q, void *ptr);

/**
 * @brief Remove the element at the given index, 
 */
void * priqueue_remove_at(priqueue_t *q, int index);

/**
 * @brief Get the number of elements in this queue
 */
int    priqueue_size     (priqueue_t *q);

/**
 * @brief Destructor for the priority queue
 */
void   priqueue_destroy  (priqueue_t *q);

#endif /* LIBPQUEUE_H_ */
