/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"

// Global ready queue
priqueue_t *ready_q;//= (priqueue_t *)malloc(sizeof(priqueue_t));

// Track busy cores
int *active_core;
int num_cores;

// Keep track of scheme
scheme_t policy;

/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
*/
typedef struct _job_t
{
	// Contains in order: uuid, arrival, burst, priority
	int value[4];

	// Negative if unassigned, otherwise the integer corresponding to the
	// core in active_core
	int core;
} job_t;

/*
 * Functions for comparison pointer
 */

int comparison_FCFS(const void *j1, const void *j2){
	job_t *this;
	job_t *that;
	this = (job_t*)j1;
	that = (job_t*)j2;
	return (this->value[1] - that->value[1]);
}

int comparison_SJF(const void *j1, const void *j2){
	return 0;
}

int comparison_PSJF(const void *j1, const void* j2){
	return 0;
}

int comparison_PRI(const void *j1, const void *j2){
	return 0;
}

int comparison_PPRI(const void *j1, const void *j2){
	return 0;
}

int comparison_RR(const void *j1, const void *j2){
	return 0;	
}


/**
 * Functions for determining and setting up the next job in the queue
 *
 * All operate under the assumption that the parameter is the new job or null
 * if no new jobs are present.
 * 
 * Will return the next job to be scheduled, having already properly scheduled
 * it.  That is, the return value is solely for the simulator's sake.
 *
 * Everything must be ready to go, in case these are called from
 * scheduler_new_job().
 */

int next_job_FCFS(job_t* new_job){
	if(NULL != new_job){
		
	}
	else{
		// Find the earliest arrival among jobs which are not
		// currently running
	}
	job_t * head = priqueue_peek(ready_q);
	return head->value[0];
}

int next_job_SJF(job_t *new_job){
	return -1;
}

int next_job_PSJF(job_t *new_job){
	return -1;
}

int next_job_PRI(job_t *new_job){
	return -1;
}

int next_job_PPRI(job_t *new_job){
	return -1;
}

int next_job_RR(job_t *new_job){
	return -1;
}

/**
 * Helper Functions
 */

/**
 * @brief Determines the next job to be run in the queue
 */


/**
 * @brief Returns the integer corresponding to the lowest free core
 * @param numCores The number of cores in use, the size of active_core
 * @return The lowest non-negative integer which corresponds to a free core,
 * or -1 if no cores are free.
 *
 */
int find_idle_core(int numCores){
	int lowest = 9000;  // It's over 9000!
	for(int i = 0; i<numCores; ++i){
		if(0 == active_core[i] && i < lowest){
				lowest = i;	
		}	
	}
	return lowest;
}


/**
  Initalizes the scheduler.
 
  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
	// Set up global core tracker, initialized to zeros
	active_core = (int *)calloc(cores, sizeof(int));

	num_cores = cores;

	// Set up global ready queue on the heap
	ready_q = (priqueue_t *)malloc(sizeof(priqueue_t));

	policy = scheme;

	switch(scheme){
		case FCFS:
			priqueue_init(ready_q, comparison_FCFS);
			break;
		case SJF:
			priqueue_init(ready_q, comparison_SJF);	
			break;
		case PSJF:
			priqueue_init(ready_q, comparison_PSJF);
			break;
		case PRI:
			priqueue_init(ready_q, comparison_PRI);
			break;
		case PPRI:
			priqueue_init(ready_q, comparison_PPRI);
			break;
		default:
			priqueue_init(ready_q, comparison_RR);
			break;
	}
	printf("Created new queue with %d elements\n", priqueue_size(ready_q));
}


/**
  Called when a new job arrives.
 
  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  NOTES:  Here, we need to determine if we need to change anything, and update everything
  	on this end for execution immediately.


  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made. 
 
 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{

	int core_scheduled = -1;

	// Create and initialize job
	job_t* daJob 	= (job_t*)malloc(sizeof(job_t));
	daJob->value[0] = job_number;
	daJob->value[1] = running_time;
	daJob->value[2] = time;
	daJob->value[3] = priority;
	daJob->core	= -1;

	// Add the new Job to the back of the queue
	int status = priqueue_offer(ready_q, daJob);

	printf("Inserted new job at position %d\n", status);


	printf("Current ready queue size is: %d\n", priqueue_size(ready_q));
	
	// Determine and set up for the next round of jobs.  It is assumed
	// that by this point, all inactive jobs have been destroyed, and that
	// this method call is the last step before the next execution
	// interval, per the instructions.
	//
	// Each of the methods below acts accordingly, updating all known jobs
	// to their correct cores and statuses.
	switch(policy){
		case FCFS:
			core_scheduled = next_job_FCFS(daJob);
			break;
		case SJF:
			core_scheduled = next_job_SJF(daJob);
			break;
		case PSJF:
			core_scheduled = next_job_PSJF(daJob);
			break;
		case PRI:
			core_scheduled = next_job_PRI(daJob);
			break;
		case PPRI:
			core_scheduled = next_job_PPRI(daJob);
			break;
		default:
			core_scheduled = next_job_RR(daJob);
	}
	
	return core_scheduled;
}


/**
  Called when a job has completed execution.
 
  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.
 
  NOTES:	Here, we need to find the enxt job, and commit the changes on
  		this end.  There may be several such calls due to the way the
		simulator is written.

  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{	
	int next_scheduled = -1;
	job_t* curr_job;
	// Find and destroy the job in question
	for(int i = 0; i<priqueue_size(ready_q); ++i){
		curr_job = priqueue_peek(ready_q);
		if(job_number == curr_job->value[0]){
			priqueue_remove(ready_q, (void*)curr_job);
			break;	
		}
	}

	// Update everything
	switch(policy){
		case FCFS:
			next_job_FCFS(NULL);
			break;
		case SJF:
			next_job_SJF(NULL);
			break;
		case PSJF:
			next_job_PSJF(NULL);
			break;
		case PRI:
			next_job_PRI(NULL);
			break;
		case PPRI:
			next_job_PPRI(NULL);
			break;
		default:
			next_job_RR(NULL);
		
	}
	
	// return the next item to run on the core in question, or -1 if idle
	if(active_core[core_id]){
		return active_core[core_id];
	}
	return -1;
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.
 
  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
	return -1;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	return 0.0;
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	return 0.0;
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	return 0.0;
}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{

}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in
  the order they are to be scheduled. Furthermore, we have also listed the
  current state of the job (either running on a given core or idle).
  For example, if we have a non-preemptive algorithm and job(id=4) has began
  running, job(id=2) arrives with a higher priority, and job(id=1) arrives with
  a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)  
  
  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.

  #### NOTE NOTE NOTE ####
  This function is also not called with arguments, even though the *included* signature
  clearly takes arguments.  This means if you try to use the parameter, assuming it
  is there, you are going to have a bad time.  I will seriously punch the original
  authors of this code in the face if I ever meet them.  I shouldn't have to go
  through their code to look for intentionally inserted errors.
 */
void scheduler_show_queue(priqueue_t* q)
{
	for(int i=0; i<priqueue_size(ready_q); ++i){
		job_t *daJob = (job_t*)priqueue_at(ready_q, 0);
		printf("\n\tJob %d:\n\tArrived:\t%d\n\tBurst:\t\t%d\n\tPriority:\t%d", daJob->value[0], daJob->value[1], daJob->value[2], daJob->value[3]);	
	}
}
