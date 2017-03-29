/** @file queuetest.c
 */

#include <stdio.h>
#include <stdlib.h>

#include "libpriqueue/libpriqueue.h"

int compare1(const void * a, const void * b)
{
	return ( *(int*)a - *(int*)b );
}

int compare2(const void * a, const void * b)
{
	return ( *(int*)b - *(int*)a );
}

int compare3(const void *a, const void *b){
	return -1;	
}

int main()
{
	priqueue_t q3;//q, q2, q3;

//	priqueue_init(&q, compare1);
//	priqueue_init(&q2, compare2);
	priqueue_init(&q3, compare3);

	/* Pupulate some data... */
	int *values = malloc(100 * sizeof(int));

	int i;
	for (i = 0; i < 100; i++)
		values[i] = i;

#if 0	/* Add 5 values, 3 unique. */
	priqueue_offer(&q, &values[12]);
	priqueue_offer(&q, &values[13]);
	priqueue_offer(&q, &values[14]);
	priqueue_offer(&q, &values[12]);
	priqueue_offer(&q, &values[12]);
	printf("Total elements: %d (expected 5).\n", priqueue_size(&q));

	int val = *((int *)priqueue_poll(&q));
	printf("Top element: %d (expected 12).\n", val);
	printf("Total elements: %d (expected 4).\n", priqueue_size(&q));

	int values_removed = priqueue_remove(&q, &values[12]);
	printf("Elements removed: %d (expected 2).\n", values_removed);
	printf("Total elements: %d (expected 2).\n", priqueue_size(&q));

	priqueue_offer(&q, &values[10]);
	priqueue_offer(&q, &values[30]);
	priqueue_offer(&q, &values[20]);

	priqueue_offer(&q2, &values[10]);
	priqueue_offer(&q2, &values[30]);
	priqueue_offer(&q2, &values[20]);


	printf("Elements in order queue (expected 10 13 14 20 30): ");
	for (i = 0; i < priqueue_size(&q); i++)
		printf("%d ", *((int *)priqueue_at(&q, i)) );
	printf("\n");

	printf("Elements in reverse order queue (expected 30 20 10): ");
	for (i = 0; i < priqueue_size(&q2); i++)
		printf("%d ", *((int *)priqueue_at(&q2, i)) );
	printf("\n");


	/*	NEW TESTS	*/

	printf("\n\nBEGINNING NEW TESTS\n\n");
	for(int j = 0; j<10; ++j){
		priqueue_poll(&q);
	}
	for(int j = 0; j<10; ++j){
		priqueue_poll(&q2);
	}


	// Create queue of 1's and 9's
	for(int j = 0; j<11; ++j){
		if(j < 5){
			priqueue_offer(&q, &values[1]);
		}
		else{
			priqueue_offer(&q, &values[9]);	
		}
	}


	// Remove middle and replace by a 5
	priqueue_remove_at(&q, 5);
	priqueue_offer(&q, &values[5]);

	priqueue_peek(&q2);
	priqueue_poll(&q2);
	priqueue_remove_at(&q2, 16);
	priqueue_remove(&q2, &values[1]);

	printf("Size of first queue: %d\n", priqueue_size(&q));

	printf("Elements in order queue (expected 1 1 1 1 1 5 9 9 9 9 9): ");
	for (i = 0; i < priqueue_size(&q); i++)
		printf("%d ", *((int *)priqueue_at(&q, i)) );
	printf("\n");

	int * temp;

	temp = (int *)priqueue_remove_at(&q, 1);

	printf("Removed element %d from position %d\n", *temp, 1);

	temp = (int*)priqueue_remove_at(&q, priqueue_size(&q)-1);

	printf("Removed element %d from position %d\n", *temp, priqueue_size(&q));

	printf("Elements in order queue (expected 1 1 1 1 5 9 9 9 9): ");
	for (i = 0; i < priqueue_size(&q); i++)
		printf("%d ", *((int *)priqueue_at(&q, i)) );
	printf("\n");


	printf("The middle element is: %d\n", *(int*)priqueue_at(&q, 5));

	printf("Elements in second queue (expected none): ");
	for (i = 0; i < priqueue_size(&q2); i++)
		printf("%d ", *((int *)priqueue_at(&q2, i)) );
	printf("\n");
#endif
	int* temp2;

	printf("Case 1: remove on empty\n");
	assert(NULL == priqueue_remove_at(&q3, 0));

	priqueue_offer(&q3, &values[0]);
	
	printf("Case 2: remove single entry\n");
	
	priqueue_offer(&q3, &values[1]);
	assert(q3.front == q3.back);
	assert(priqueue_size(&q3) == 1);

	temp2 = (int*)priqueue_remove_at(&q3, 0);
	assert(priqueue_size(&q3)==0);
	assert(q3.front == NULL);
	assert(q3.back == NULL);
	assert(*temp2 == 1);


	printf("Case 3: remove front, 2+ elements\n");

	priqueue_offer(&q3, temp2);
	priqueue_offer(&q3, temp2);
	assert(priqueue_size(&q3)==2);
	temp2 = (int*)priqueue_remove_at(&q3, 0);
	assert(priqueue_size(&q3)==1);
	assert(q3.front == q3.back);
	assert(*temp2 == 1);

	temp2 = (int*)priqueue_remove_at(&q3, 0);
	assert(priqueue_size(&q3)==0);
	assert(q3.front == NULL);
	assert(q3.back == NULL);
	assert(*temp2 == 1);
	
	printf("Case 4: remove back, 2+ elements\n");
	priqueue_offer(&q3, temp2);
	priqueue_offer(&q3, temp2);
	assert(priqueue_size(&q3)==2);
	temp2 = (int*)priqueue_remove_at(&q3, 1);
	assert(priqueue_size(&q3)==1);
	assert(q3.front == q3.back);
	assert(*temp2 == 1);

	printf("Case 5: remove in middle\n");
	for(int i = 0; i<3; ++i){
		*temp2 = i;
		priqueue_offer(&q3, temp2);
	}

	temp2 = (int*)priqueue_remove_at(&q3, 1);
	assert(priqueue_size(&q3)==2);
	assert(*temp2 == 1);



	printf("Checking RR-style queue...\n");
	for(int i = 0; i<priqueue_size(&q3); ++i){
		printf("%d ", *(int*)priqueue_at(&q3, i));
	}


	printf("\n");

	//priqueue_destroy(&q2);
	//priqueue_destroy(&q);

	//free(values);

	return 0;
}
