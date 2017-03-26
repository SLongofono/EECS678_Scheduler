/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"

// Global ready queue
priqueue_t *ready_q;//= (priqueue_t *)malloc(sizeof(priqueue_t));

// Global queue for tracking RR scheduling order
priqueue_t *rr_q;

// Track busy cores
int *active_core;
int NUM_CORES;

// Keep track of scheme
scheme_t policy;

/**
  Stores information making up a job to be scheduled  and statistics required
  for the scheduler and its helper functions.
*/
typedef struct _job_t
{
	// Contains in order: uuid, arrival, burst, priority, runtime, end,
	// last active time, latency, wait time
	int value[9];

	// Negative if unassigned, otherwise the integer corresponding to the
	// core in active_core
	int core;
	int finished;
} job_t;


/**
 * Stores information to force our queue type to behave as a normal FIFO queue
 */
typedef struct rr_job_t{
	// Real job number, order number
	int value[2];
} rr_job_t;


/**
 * Helper Functions
 */
void update_wait_time(job_t * job, int time){
	// Determine total time
	// subtract running time
}

void update_run_time(job_t *job, int time){
	// Determine total time
	// subtract waiting time
}


/**
 * @brief Returns the integer corresponding to the lowest free core
 * @return The lowest non-negative integer which corresponds to a free core,
 * or -1 if no cores are free.
 *
 */
int get_idle_core(){
	int lowest = 9000;
	for(int i = 0; i<NUM_CORES; ++i){
		if(0 > active_core[i] && i < lowest){
			lowest = i;	
		}	
	}
	return lowest;
}


/**
 * @brief Returns a pointer the job with the job number passed in, or NULL
 * 	  if no such job exists.
 */
job_t * get_job(int job_number){
	job_t* result = NULL;
	job_t *current_job;
	for(int i = 0; i<priqueue_size(ready_q); ++i){
		current_job = priqueue_at(ready_q, i);
		if(job_number == current_job->value[0]){
			result = current_job;
			break;
		}
	}
	return result;
}


/**
 * @brief Associates a job with a core and vice versa
 *
 * Dies if anything is amiss
 */

void update_core(int core, job_t * job){

	printf("Updating core %d with job %d...\n", core, job->value[0]);
	if(core > NUM_CORES){
		printf("[ Error ]\t\tTried to update nonexistent core...\n");
		assert(0);	
	}
	
	// Update job
	job->core = core;

	printf("Job %d core set to %d\n", job->value[0], core);

	// Update cores and return success
	active_core[core] = job->value[0];

	return;
}


/*
 * Functions for comparison of jobs under different scheduling policies
 *
 * Note: The priqueue class behaves as such:
 * 	compare(a,b):
 * 		return 1 if a < b
 * 		else -1
 *
 * 	Thus all functions need to return accordingly, positive return
 * 	if the first parameter takes precedence, negative otherwsie
 */


int comparison_RR(const void *j1, const void *j2){
	int *this;
	int *that;
	this = (int*)j1;
	that = (int*)j2;
	// Assuming this is properly updated, this should be positive when
	// this should run next
	return this-that;
}

int comparison_FCFS(const void *j1, const void *j2){
	job_t *this;
	job_t *that;
	this = (job_t*)j1;
	that = (job_t*)j2;
	
	// currently running jobs take precedence
	if(0 > this->core && 0 <= that->core){
		return 1;
	}
	else if(0 > that->core && 0 <= this->core ){
		return -1;
	}
	else{
		// Otherwise, evaluate them based on arrival
		// Since we are guaranteed unique arrival times, simply subtract them.
		// If the latter came sooner than the former, the result will be
		// positive.
		printf("Job %d arrived at time %d\n", this->value[0], this->value[1]);
		printf("job %d arrived at time %d\n", that->value[0], that->value[1]);
		printf("Job %d came %d seconds before job %d...\n", this->value[0], that->value[1]-this->value[1], that->value[0]);
		return (this->value[1] - that->value[1]);
	}
}


int comparison_SJF(const void *j1, const void *j2){

	job_t *this;
	job_t *that;
	int t1, t2;
	this = (job_t *)j1;
	that = (job_t *)j2;


	// currently running jobs take precedence
	if(0 > this->core && 0 <= that->core){
		return 1;
	}
	else if(0 > that->core && 0 <= this->core ){
		return -1;
	}
	else{
		// Compute running time remaining, defined as the burst time minus the
		// running time.
		t1 = this->value[2] - this->value[4];
		t2 = that->value[2] - that->value[4];

		// If the remaining time is equal, compare the arrival times per the
		// rubric 
		if(t2 == t1){
			// This result will be positive if the latter job arrived first
			return (this->value[1] - that->value[1]);
		}

		// This result will be positive if this job has a longer running
		// time remaining.
		return (t1-t2);
	}
}


int comparison_PRI(const void *j1, const void *j2){

	job_t *this;
	job_t *that;
	this = (job_t*)j1;
	that = (job_t*)j2;
	
	// currently running jobs take precedence
	if(0 > this->core && 0 <= that->core){
		return 1;
	}
	else if(0 > that->core && 0 <= this->core ){
		return -1;
	}
	else{
		// Otherwise, evaluate them based on priority, then arrival
		if(this->value[3] == that->value[3]){
			
			// Since we are guaranteed unique arrival times, simply subtract them.
			// If the latter came sooner than the former, the result will be
			// positive.
			printf("Job %d arrived at time %d\n", this->value[0], this->value[1]);
			printf("job %d arrived at time %d\n", that->value[0], that->value[1]);
			printf("Job %d came %d seconds before job %d...\n", this->value[0], that->value[1]-this->value[1], that->value[0]);
			return (this->value[1] - that->value[1]);
		}
		else{
			// Positive if the former is higher priority
			return (this->value[3] - that->value[3]);	
		}
	}
}


int comparison_PPRI(const void *j1, const void *j2){

	job_t *this;
	job_t *that;
	this = (job_t*)j1;
	that = (job_t*)j2;
	
	// Evaluate them based on priority, then arrival
	if(this->value[3] == that->value[3]){
		
		// Since we are guaranteed unique arrival times, simply subtract them.
		// If the latter came sooner than the former, the result will be
		// positive.
		printf("Job %d arrived at time %d\n", this->value[0], this->value[1]);
		printf("job %d arrived at time %d\n", that->value[0], that->value[1]);
		printf("Job %d came %d seconds before job %d...\n", this->value[0], that->value[1]-this->value[1], that->value[0]);
		return (this->value[1] - that->value[1]);
	}
	else{
		// Positive if the former is higher priority
		// Note that this is exactly backwards wrt what we discussed
		// in class.
		return (this->value[3] - that->value[3]);	
	}
}


int comparison_PSJF(const void *j1, const void *j2){
	
	job_t *this;
	job_t *that;
	int t1, t2;
	this = (job_t *)j1;
	that = (job_t *)j2;


	// Compute running time remaining, defined as the burst time minus the
	// running time.
	t1 = this->value[2] - this->value[4];
	t2 = that->value[2] - that->value[4];

	// If the remaining time is equal, compare the arrival times per the
	// rubric 
	if(t2 == t1){
		// This result will be positive if the latter job arrived first
		return (this->value[1] - that->value[1]);
	}

	// This result will be positive if this job has a longer running
	// time remaining.
	return (t1-t2);
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
void next_job_FCFS(job_t* new_job, int time){
	job_t* next_job = NULL;
	int length = priqueue_size(ready_q);
	int found = 0;
	if(length > 0){
	
		// If not empty, search through the queue
		// Everything should be sorted by arrival automatically by
		// priqueue_offer
		printf("Finding next job for FCFS...\n");

		for(int i = 0; i<length; ++i){
			next_job = (job_t*)priqueue_at(ready_q, i);
			
			// If the current job is not finished..
			if(next_job->finished != 1){
		
				printf("Job %d is not finished(%d)...\n", next_job->value[0], next_job->finished);
				
				// and it is not already running...
				if(0 >  next_job->core){
				
					printf("and not running, it will be the next job...\n");
					found = 1;
					break;
				}
				else{
					printf("Job %d is running, moving on...\n", next_job->value[0]);
				}
			}
			else{
				printf("Job %d is finished, moving on...\n", next_job->value[0]);	
			}
		}

		if(found){
			// If there is an idle core, update it with the next job
			
			int idle = get_idle_core();
			
			if(9000 != idle){
				printf("Updating core %d, currently running: %d\n", idle, active_core[idle]);
				update_core(get_idle_core(), next_job);
				printf("Core %d is now running job %d\n", idle, active_core[idle]);
				
				// Update last active time to next time cycle
				next_job->value[6] = time;

				// Update latency if not already set
				if(next_job->value[7] < 0){
					next_job->value[7] = (time - next_job->value[1]);	
				}
			}
			else{
				printf("No idle cores found, get_idle returned %d\n", idle);	
			}
		}
		else{
			printf("No valid jobs found to schedule\n");
		}
	}
}

void next_job_SJF(job_t *new_job, int time){
	job_t* next_job = NULL;
	int length = priqueue_size(ready_q);
	int found;
	if(0 < length){
	
		// If not empty, search through the queue
		// Everything should be sorted by arrival automatically by
		// priqueue_offer
		printf("Finding next job for SJF...\n");
		
		int idle = get_idle_core();

		while(9000 != idle){
			found = 0;
			for(int i = 0; i<length; ++i){
				next_job = (job_t*)priqueue_at(ready_q, i);
			
				// If the current job is not finished..
				if(next_job->finished != 1){
		
					printf("Job %d is not finished(%d)...\n", next_job->value[0], next_job->finished);
				
					// and it is not already running...
					if(0 > next_job->core){
				
						printf("and not running, it will be the next job...\n");
						found = 1;
						break;
					}
					else{
						printf("Job %d is running, moving on...\n", next_job->value[0]);
					}
				}
				else{
					printf("Job %d is finished, moving on...\n", next_job->value[0]);	
				}
			}

			if(found){
				printf("Updating core %d, currently running: %d\n", idle, active_core[idle]);
				update_core(idle, next_job);
				printf("Core %d is now running job %d\n", idle, active_core[idle]);
				
				// Update last active time to next time cycle
				next_job->value[6] = time;

				// Update latency if not already set
				// Needs to be done this way since there could
				// be multiple schedulings between each time
				// unit
				if(next_job->value[4] == 1 && next_job->value[6] < 0){
					// This job just ran 1 unit of time,
					// update its latency to the previous
					// time unit if it has not already
					// been set
					next_job->value[7] = (time - next_job->value[1]);	
				}
				idle = get_idle_core();
			}
			else{
				printf("No valid jobs found to schedule\n");
				break;
			}
		}// End while
	}// End if(0 < length)
}

void next_job_PSJF(job_t *new_job, int time){
	// Any preempted jobs should have their run time updated as they are
	// pulled off
	
	job_t *next_job;
	
	// Clear all cores and update jobs
	for(int j = 0; j<NUM_CORES; ++j){
		printf("Updating core %d, job %d...\n", j, active_core[j]);
		next_job = get_job(active_core[j]);

		if(NULL != next_job){

			active_core[j] = -1;
			next_job->core = -1;
		
			printf("Updating time, job %d...\n", next_job->value[0]);

			// update running time
			if(next_job->value[4] < 0){
				if(next_job->value[6] >= 0){
					next_job->value[4] = time-next_job->value[6];
				}
				else{
					next_job->value[4] = 0;	
				}
			}
			else{
				next_job->value[4] += time-next_job->value[6];
			}
			// update last active time
			next_job->value[6] = time;
		}
		else{
			printf("No job on core %d...\n", j);
		}
	}


	next_job = NULL;
	int length = priqueue_size(ready_q);
	int found = 0;
	if(length > 0){
	
		// If not empty, search through the queue
		// Everything should be sorted by arrival automatically by
		// priqueue_offer
		printf("Finding next job for PSJF...\n");
		
		int idle = get_idle_core();
		
		// While there are cores left to be scheduled
		while(9000 != idle){
			found = 0;
			for(int i = 0; i<length; ++i){
				next_job = (job_t*)priqueue_at(ready_q, i);
			
				// If the current job is not finished..
				if(next_job->finished != 1){
		
					printf("Job %d is not finished, it will be the next job...\n", next_job->value[0]);
						found = 1;
						break;
				}
				else{
					printf("Job %d is finished, moving on...\n", next_job->value[0]);	
				}
			}

			if(found){
				
				// Update the idle core with the next job		
				printf("Updating core %d, currently running: %d\n", idle, active_core[idle]);
				update_core(get_idle_core(), next_job);
				printf("Core %d is now running job %d\n", idle, active_core[idle]);
				
				


				// Update latency if not already set
				// Needs to be done this way since there could
				// be multiple schedulings between each time
				// unit
				if(next_job->value[4] > 0 && next_job->value[7] < 0){
					// This job just ran,
					// update its latency to the previous
					// time unit if it has not already
					// been set
					printf("Job %d has now run for %d units, updating latency to %d\n", next_job->value[0], next_job->value[4], time - next_job->value[4]-next_job->value[1]);
					next_job->value[7] = (time - next_job->value[4] - next_job->value[1]);	
				}
				
				// Update last active time to next time cycle
				next_job->value[6] = time;	
				
				// Get next core to be scheduled
				idle = get_idle_core();
			}
			else{
				printf("No valid jobs found to schedule\n");
				break;
			}
		}// End while
	} // End if(length > 0)
}

void next_job_PRI(job_t *new_job, int time){
	job_t* next_job = NULL;
	int length = priqueue_size(ready_q);
	int found;
	if(0 < length){
	
		// If not empty, search through the queue
		// Everything should be sorted by arrival automatically by
		// priqueue_offer
		printf("Finding next job for PRI...\n");
		
		int idle = get_idle_core();

		while(9000 != idle){
			found = 0;
			for(int i = 0; i<length; ++i){
				next_job = (job_t*)priqueue_at(ready_q, i);
			
				// If the current job is not finished..
				if(next_job->finished != 1){
		
					printf("Job %d is not finished(%d)...\n", next_job->value[0], next_job->finished);
				
					// and it is not already running...
					if(0 > next_job->core){
				
						printf("and not running, it will be the next job...\n");
						found = 1;
						break;
					}
					else{
						printf("Job %d is running, moving on...\n", next_job->value[0]);
					}
				}
				else{
					printf("Job %d is finished, moving on...\n", next_job->value[0]);	
				}
			}

			if(found){
				printf("Updating core %d, currently running: %d\n", idle, active_core[idle]);
				update_core(idle, next_job);
				printf("Core %d is now running job %d\n", idle, active_core[idle]);
				
				// Update last active time to next time cycle
				next_job->value[6] = time;

				// Update latency if not already set
				// Needs to be done this way since there could
				// be multiple schedulings between each time
				// unit
				if(next_job->value[4] == 1 && next_job->value[6] < 0){
					// This job just ran 1 unit of time,
					// update its latency to the previous
					// time unit if it has not already
					// been set
					next_job->value[7] = (time - next_job->value[1]);	
				}
				idle = get_idle_core();
			}
			else{
				printf("No valid jobs found to schedule\n");
				break;
			}
		}// End while
	}// End if(0 < length)
}

void next_job_PPRI(job_t *new_job, int time){
	
	job_t *next_job;
	
	// Clear all cores and update jobs
	for(int j = 0; j<NUM_CORES; ++j){
		printf("Updating core %d, job %d...\n", j, active_core[j]);
		next_job = get_job(active_core[j]);

		if(NULL != next_job){

			active_core[j] = -1;
			next_job->core = -1;
		
			printf("Updating time, job %d...\n", next_job->value[0]);

			// update running time
			if(next_job->value[4] < 0){
				if(next_job->value[6] >= 0){
					next_job->value[4] = time-next_job->value[6];
				}
				else{
					next_job->value[4] = 0;	
				}
			}
			else{
				next_job->value[4] += time-next_job->value[6];
			}
			// update last active time
			next_job->value[6] = time;
		}
		else{
			printf("No job on core %d...\n", j);
		}
	}


	next_job = NULL;
	int length = priqueue_size(ready_q);
	int found = 0;
	if(length > 0){
	
		// If not empty, search through the queue
		// Everything should be sorted by arrival automatically by
		// priqueue_offer
		printf("Finding next job for PPRI...\n");
		
		int idle = get_idle_core();
		
		// While there are cores left to be scheduled
		while(9000 != idle){

			for(int i = 0; i<length; ++i){
				next_job = (job_t*)priqueue_at(ready_q, i);
			
				// If the current job is not finished..
				if(next_job->finished != 1){
		
					printf("Job %d is not finished, it will be the next job...\n", next_job->value[0]);
						found = 1;
						break;
				}
				else{
					printf("Job %d is finished, moving on...\n", next_job->value[0]);	
				}
			}

			if(found){
				
				// Update the idle core with the next job		
				printf("Updating core %d, currently running: %d\n", idle, active_core[idle]);
				update_core(get_idle_core(), next_job);
				printf("Core %d is now running job %d\n", idle, active_core[idle]);
				
				


				// Update latency if not already set
				// Needs to be done this way since there could
				// be multiple schedulings between each time
				// unit
				if(next_job->value[4] > 0 && next_job->value[7] < 0){
					// This job just ran,
					// update its latency to the previous
					// time unit if it has not already
					// been set
					printf("Job %d has now run for %d units, updating latency to %d\n", next_job->value[0], next_job->value[4], time - next_job->value[4]-next_job->value[1]);
					next_job->value[7] = (time - next_job->value[4] - next_job->value[1]);	
				}
				
				// Update last active time to next time cycle
				next_job->value[6] = time;	
				
				// Get next core to be scheduled
				idle = get_idle_core();
			}
			else{
				printf("No valid jobs found to schedule\n");
				break;
			}
		}// End while
	} // End if(length > 0)
}

void next_job_RR(job_t *new_job, int time){
	// Identify the next job to be run
	
	// Update the current job's entry in the rr queue to have one more
	// than the last member's rank

	// Find the core to run it on
	
	// Update the run time for the preempted job

}



/**
  Initalizes the scheduler.
 
  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These
  	 cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will
  		 be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
	// Set up global core tracker, initialized to zeros
	active_core = (int *)malloc(cores*sizeof(int));
	for(int i = 0; i<cores; ++i){
		active_core[i] = -1;
	}

	NUM_CORES = cores;

	// Set up global ready queue on the heap
	ready_q = (priqueue_t *)malloc(sizeof(priqueue_t));

	policy = scheme;

	switch(policy){
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
			// Round robin
			priqueue_init(ready_q, comparison_FCFS);
			priqueue_init(rr_q, comparison_RR);
			break;
	}
	printf("Created new queue with %d elements\n", priqueue_size(ready_q));
	printf("Cores:\t");
	for(int i = 0; i<NUM_CORES; i++){printf("%d ", active_core[i]);}
	printf("\n");
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

  NOTES:  Here, we need to determine if we need to change anything, and update
  	  everything on this end for execution immediately.


  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before
  		      it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the
  		  priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made. 
 
 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{

	// Create and initialize job
	job_t* daJob 	= (job_t*)malloc(sizeof(job_t));
	daJob->value[0] = job_number;		// UUID
	daJob->value[1] = time;			// Arrival time
	daJob->value[2] = running_time;		// Burst
	daJob->value[3] = priority;		// Priority
	daJob->value[4] = 0;			// Cumulative running time
	daJob->value[5] = 0;			// End time
	daJob->value[6] = -1;			// Last active time
	daJob->value[7] = -1;			// Scheduling latency
	daJob->value[8] = -1;			// Cumulative waiting time
	daJob->core	= -1;			// Active core
	daJob->finished = 0;			// Complete/Incomplete

	// Add the new Job to the back of the queue
	int status = priqueue_offer(ready_q, daJob);

	printf("Inserted new job at position %d\n", status);

	printf("Current jobs in queue:\n\t");
	for(int j = 0; j<priqueue_size(ready_q); ++j){
		job_t* temp;
		temp = (job_t*)priqueue_at(ready_q, j);
		printf("%d\t", temp->value[0]);
	}
	printf("\n");

	printf("Current ready queue size is: %d\n", priqueue_size(ready_q));
	
	// Determine and set up for the next round of jobs.  It is assumed
	// that by this point, all inactive jobs have been destroyed, and that
	// this method call is the last step before the next execution
	// interval, per the instructions.
	//
	// Each of the methods below acts accordingly, updating all known jobs
	// to their correct cores and statuses.

	switch(policy){
		case RR:
			// Need an empty statement to avoid a gotcha, labels
			// must be followed by statements, but declarations do
			// not count as statements. Look up error message:
			// "error: a label can only be part of a statement and
			// a declaration is not a statement".  Thanks stackexchange.
			; // Do nothing

			int *rr_job_number = (int*)malloc(sizeof(int));
			
			job_t* last;
			last = (job_t*)priqueue_at(ready_q, priqueue_size(ready_q)-1);

			// One more than the last ensures FIFO order
			*rr_job_number = last->value[1] + 1;
			priqueue_offer(rr_q, rr_job_number);

			// Schedule all jobs
			next_job_RR(daJob, time);
			break;
		case PPRI:
			next_job_PPRI(daJob, time);
			break;
		case PRI:
			next_job_PRI(daJob, time);
			break;
		case PSJF:
			next_job_PSJF(daJob, time);
			break;
		case SJF:
			next_job_SJF(daJob, time);
			break;
		case FCFS:
			next_job_FCFS(daJob, time);
			break;
	}

	// The job in question will have its core already assigned
	return get_job(job_number)->core;
}


/**
  Called when a job has completed execution.
 
  The core_id, job_number and time parameters are provided for convenience. You
  may be able to calculate the values with your own data structure.
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
	job_t* curr_job;

	// Find and update the job in question
	curr_job = get_job(job_number);
	printf("Job finished: %d\n", curr_job->value[0]);

	// Mark complete
	curr_job->finished = 1;

	// Mark end time
	curr_job->value[5] = time;

	// Update running time, as current time less last
	// known active time
	curr_job->value[4] += (time - curr_job->value[6]);


	// If latency was never set, this job ran start to finished
	// uncontested
	if(0 > curr_job->value[7]){
		// Thus latency is current time less running time less arrival
		// time
		printf("Time %d, job %d has run for %d time units\n", time, curr_job->value[0], curr_job->value[4]);
		printf("Job %d latency update to %d\n", curr_job->value[0], time-curr_job->value[4]-curr_job->value[1]);
		curr_job->value[7] = time - curr_job->value[4] - curr_job->value[1];
	}
	
	printf("Freeing core %d, currently running job %d...\n", core_id, active_core[core_id]);
	// Free the core for downstream helpers
	active_core[core_id] = -1;

	// Update everything
	switch(policy){
		case FCFS:
			next_job_FCFS(NULL, time);
			break;
		case SJF:
			next_job_SJF(NULL, time);
			break;
		case PSJF:
			next_job_PSJF(NULL, time);
			break;
		case PRI:
			next_job_PRI(NULL, time);
			break;
		case PPRI:
			next_job_PPRI(NULL, time);
			break;
		default:
			next_job_RR(NULL, time);
		
	}

	printf("Wrapping up finished job, scheduling new job: %d\n", active_core[core_id]);

	// return the next item to run on the core in question, or -1 if idle
	return active_core[core_id];
	
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
	// Since this is only relevant to RR, we have it easy.
	
	// Find the currently running job and set it to idle
	int current_job_number = active_core[core_id];
	assert(current_job_number >= 0);
	job_t * current_job = get_job(current_job_number);
	current_job->core = -1;

	// Update time metrics
	if(current_job->value[6] > 0){
		// Update running time with the current time less the last
		// known active time.
		current_job->value[4] += (time - current_job->value[6]);	
	}
	else{
		// Something is terribly wrong
		printf("[ Error ]\tQuantum expired on job that was never active...\n");
		assert(0);	
	}

	// Call to scheduled and return the job number of the next job to run
	next_job_RR(NULL, time);

	return active_core[core_id];
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all
      jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	job_t * current_job;
	int sum = 0;
	for(int i = 0; i<priqueue_size(ready_q); ++i){
		current_job = (job_t *)priqueue_at(ready_q, i);

		printf("Job %d waited %d time units\n", current_job->value[0], current_job->value[5]-current_job->value[1] - current_job->value[4]);
		// waiting time is total time less running time
		sum += (current_job->value[5] - current_job->value[1] - current_job->value[4]);
	}
	return (1.0 * sum)/priqueue_size(ready_q);
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  "turnaround time" is the total amount of time required to complete the process.
  This is the sum of the waiting time and the running time, or equivalently the
  time from arrival to completion.

  Assumptions:
    - This function will only be called after all scheduling is complete (all
      jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	int time = 0;
	job_t *current_job;
	for(int i = 0; i<priqueue_size(ready_q); ++i){
		current_job = (job_t*)priqueue_at(ready_q, i);
		// Add in total time (end-start)
		printf("Job %d took a total of %d time units\n", current_job->value[0], current_job->value[5]-current_job->value[1]);
		time += (current_job->value[5] - current_job->value[1]);
	}
	return (1.0 * time)/priqueue_size(ready_q);
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

 "Response time" is what everyone else calls "scheduling latency"

  Assumptions:
    - This function will only be called after all scheduling is complete (all
      jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	int time = 0;
	job_t *current_job;
	for(int i = 0; i<priqueue_size(ready_q); ++i){
		current_job = (job_t*)priqueue_at(ready_q, i);
		printf("Job %d had latency of %d time units\n", current_job->value[0], current_job->value[7]);
		time += current_job->value[7];
	}
	return (1.0 * time)/priqueue_size(ready_q);
}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
	// Need to free all values members
	// Need to free ready_q members
	// Need to free rr_q members
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
		job_t *daJob = (job_t*)priqueue_at(ready_q, i);
		printf("\n\tJob %d:\tArrived:\t%d\tBurst:\t%d\tPriority:\t%d\tCore:\t%d\tRuntime:\t%d\n", daJob->value[0], daJob->value[1], daJob->value[2], daJob->value[3], daJob->core, daJob->value[4]);
	}
}
