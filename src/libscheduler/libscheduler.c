/** @file libscheduler.c
 *
 *  @author Stephen Longofono
 *  
 *  created  03/20/2017
 *
 *  modified 03/29/2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"

#define DEBUG 0


// Global ready queue
priqueue_t *ready_q;


// Track busy cores
int *active_core;
int NUM_CORES;


// Keep track of scheme
scheme_t policy;


/**
  Stores information making up a job to be scheduled  and statistics required
  for the scheduler and its helper functions.
*/
typedef struct _job_t{

	// Contains in order: uuid, arrival, burst, priority, runtime, end,
	// last active time, latency
	int value[8];

	// Negative if unassigned, otherwise the integer corresponding to the
	// core in active_core
	int core;

	// Flag for completion
	int finished;

	// Tracks special order for RR scheduling
	int RR_order;

} job_t;


void print_queue(){
	job_t * curr;
	for(int i=0; i<priqueue_size(ready_q); ++i){
		curr = (job_t*)priqueue_at(ready_q, i);
		if(curr->finished){
			printf("X%dX ", curr->value[0]);
		}
		else{
			printf("%d(%d) ", curr->value[0], curr->value[3]);
		}
	}
	printf("\n");
}


/** @brief Computes and updates time metrics for the job given
 *
 * @param job A pointer to the job to update
 * @param time An integer representing the current time unit
 */
void update_running_time(job_t * job, int time){

	int new_running_time = 0;
	
	// Additional running time is current time less last active
	// time, or 0 if this job has not run yet
	
	// If this job has already been active...
	if(0 <= job->value[6]){
		if(DEBUG){
			printf("Computing new run time as %d - %d...\n", time, job->value[6]);
		}
		new_running_time = time - job->value[6];
	}
	
	// Update last active time
	job->value[6] = time;
	if(DEBUG){
		printf("Job %d running time incremented from  %d to %d\n", job->value[0], job->value[4], job->value[4]+new_running_time);
		printf("Job %d last active time updated to %d\n", job->value[0], job->value[6]);
	}
	
	job->value[4] += new_running_time;
}


/**
 * @brief Computes and updates the latency of the given job
 *
 * @param job A pointer to the job to update
 * @param time An integer representing the current time unit
 */
void update_latency_time(job_t* job, int time){
	// If we haven't already set it...
	if(0 > job->value[7]){
		job->value[7] = time - job->value[1];
		if(DEBUG){
			printf("Job %d latency updated to %d\n", job->value[0], job->value[7]);
		}
	}
}


/**
 * @brief Computes and updates time metrics for the job given
 *
 * @param job A pointer to the job to update
 * @param time An integer representing the current time unit
 */
void update_time(int time){

	job_t *curr_job;

	for(int i = 0; i<priqueue_size(ready_q); ++i){
		curr_job = (job_t*)priqueue_at(ready_q, i);

		assert(NULL != curr_job);

		// if active or just ended
		if(0 <= curr_job->core){
			update_running_time(curr_job, time);
			update_latency_time(curr_job, time);
		}
		else if(time == curr_job->value[5]){
			update_running_time(curr_job, time);
			
		}
		if(DEBUG){
			printf("Time updated, job %d\n", curr_job->value[0]);
		}
	}
}


/**
 * @brief fetches the currently running job to be preempted by the job passed in, or NULL if none exist
 *
 * @param job A job_t to compare against existing jobs, the preempting job
 *
 * @return A job to be preempted, or NULL if none exists
 */
job_t* get_preempt_job(job_t *current_job){

	// To accurately judge which needs to be preempted, we need to look
	// from the back of the list forward
	job_t *running_job;

	for(int i = priqueue_size(ready_q)-1; i>=0; --i){
		
		running_job = (job_t*)priqueue_at(ready_q, i);

		// If the job is running
		if(0 <= running_job->core){
			//and the job passed in trumps it
			if(0 >= ready_q->compare(current_job, running_job)){
				return running_job;
			}
		}
	}

	return NULL;
}


/**
 * @brief Returns the integer corresponding to the lowest free core
 * 
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
 * @brief Given a job number, locates and returns the job with that number
 * from the ready queue
 *
 * @param job_number The job number of the job to fetch (guaranteed unique)
 *
 * @return a pointer the job with the job number passed in, or NULL
 * 	  if no such job exists.
 */
job_t * get_job(int job_number){
	
	if(DEBUG){
		printf("In get job, looking for %d...\n", job_number);
	}
	
	job_t* result = NULL;
	job_t *current_job;
	for(int i = 0; i<priqueue_size(ready_q); ++i){
		current_job = (job_t*)priqueue_at(ready_q, i);
		if(job_number == current_job->value[0]){
			result = current_job;
			if(DEBUG){
				printf("Found job %d!\n", current_job->value[0]);
			}
			break;
		}
		else{
			if(DEBUG){
				printf("Job %d does not match %d\n", current_job->value[0], job_number);
			}
		}
	}
	return result;
}


/**
 * @brief Associates a job with a core and vice versa
 *
 * @param core An integer representing the index into the active_core global
 * 	       array of cores
 * @param job A pointer to a job to be updated to the given core
 *
 *  Dies if anything is amiss
 */

void update_core(int core, job_t * job){

	if(DEBUG){
		printf("Updating core %d with job %d...\n", core, job->value[0]);
	}
	if(core > NUM_CORES){
		printf("[ Error ]\t\tTried to update nonexistent core...\n");
		assert(0);	
	}
	
	// Update job
	job->core = core;

	if(DEBUG){
		printf("Job %d core set to %d\n", job->value[0], core);
	}

	// Update cores and return success
	active_core[core] = job->value[0];

	return;
}


/*
 * Functions for comparison of jobs under different scheduling policies
 *
 * @param j1 A pointer to a job, given as a void *
 * @param j2 A pointer to a job, given as a void *
 *
 * @return An integer which is negative if the j2 takes precedence over j1,
 *         zero if equal, or positive if j1 takes precedence
 *
 * Note: The priqueue class behaves as such:
 * 	 compare(a,b):
 * 		return 1 if a < b
 * 		else -1
 *
 * 	 Thus all functions need to return accordingly, positive return
 * 	 if the first parameter takes precedence, negative otherwise
 */

// Round robin comparison
int comparison_RR(const void *j1, const void *j2){
	// Always return -1 so that this queue behaves like a FIFO
	return -1;
}


// First come, first serve comparison
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
		if(DEBUG){
			printf("Job %d arrived at time %d\n", this->value[0], this->value[1]);
			printf("job %d arrived at time %d\n", that->value[0], that->value[1]);
			printf("Job %d came %d seconds before job %d...\n", this->value[0], that->value[1]-this->value[1], that->value[0]);
		}
		return (this->value[1] - that->value[1]);
	}
}


// Shortest job first comparison
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


// Priority comparison
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
			if(DEBUG){
				printf("Job %d arrived at time %d\n", this->value[0], this->value[1]);
				printf("job %d arrived at time %d\n", that->value[0], that->value[1]);
				printf("Job %d came %d seconds before job %d...\n", this->value[0], that->value[1]-this->value[1], that->value[0]);
			}
			return (this->value[1] - that->value[1]);
		}
		else{
			if(DEBUG){
				printf("Job %d has priority %d\n", this->value[0], this->value[3]);
				printf("Job %d has priority %d\n", that->value[0], that->value[3]);
				printf("Returning %d...\n", this->value[3]-that->value[3]);
			}
			// Positive if the former is higher priority
			return (this->value[3] - that->value[3]);	
		}
	}
}


// Preemptive priority comparison
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
		if(DEBUG){
			printf("Job %d arrived at time %d\n", this->value[0], this->value[1]);
			printf("job %d arrived at time %d\n", that->value[0], that->value[1]);
			printf("Job %d came %d seconds before job %d...\n", this->value[0], that->value[1]-this->value[1], that->value[0]);
		}
		return (this->value[1] - that->value[1]);
	}
	else{
		if(DEBUG){
			printf("Job %d has priority %d\n", this->value[0], this->value[3]);
			printf("Job %d has priority %d\n", that->value[0], that->value[3]);
			printf("Returning %d...\n", this->value[3]-that->value[3]);
		}
		// Positive if the former is higher priority
		// Note that this is exactly backwards wrt what we discussed
		// in class and most Linux systems.
		return (this->value[3] - that->value[3]);	
	}
}


// Preemptive shortest job first comparion
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
		if(DEBUG){
			printf("Both jobs have %d time remaining\n", t1);
			printf("Job %d arrived at time %d\n", this->value[0], this->value[1]);
			printf("job %d arrived at time %d\n", that->value[0], that->value[1]);
			printf("Job %d came %d seconds before job %d...\n", this->value[0], that->value[1]-this->value[1], that->value[0]);
		}
				
		// This result will be positive if the latter job arrived first
		return (this->value[1] - that->value[1]);
	}
	else{
		if(DEBUG){
			printf("Job %d has remaining time %d\n", this->value[0], t1);
			printf("Job %d has remaining time %d\n", that->value[0], t2);
			printf("Returning %d...\n", t1-t2);
		}
			
	}
	// This result will be positive if this job has a longer running
	// time remaining.
	return (t1-t2);
}


/**
 * @brief Determines the next job to be scheduled for non-preemptive schemes
 * 
 * @param new_job An artifact from earlier versions that I was too lazy to
 * 		  remove
 *
 * @param time An integer representing the current time		  
 *
 * NOTE: This method schedules and updates everything blindly, plan
 * 	 accordingly.  Relies on the ready queue being sorted by precedence.
 */
void next_job_no_preempt(job_t* new_job, int time){

	job_t* next_job = NULL;
	int length = priqueue_size(ready_q);
	int found;
	if(0 < length){
	
		// If not empty, search through the queue
		// Everything should be sorted by arrival automatically by
		// priqueue_offer
		int idle = get_idle_core();

		while(9000 != idle){
			found = 0;
			for(int i = 0; i<length; ++i){
				next_job = (job_t*)priqueue_at(ready_q, i);
			
				// If the current job is not finished..
				if(next_job->finished != 1){
		
					if(DEBUG){
						printf("Job %d is not finished(%d)...\n", next_job->value[0], next_job->finished);
					}
				
					// and it is not already running...
					if(0 > next_job->core){
				
						if(DEBUG){
							printf("and not running, it will be the next job...\n");
						}
						found = 1;
						break;
					}
					else{
						if(DEBUG){
							printf("Job %d is running, moving on...\n", next_job->value[0]);
						}
					}
				}
				else{
					if(DEBUG){
						printf("Job %d is finished, moving on...\n", next_job->value[0]);
					}
				}
			}

			if(found){
				if(DEBUG){
					printf("Updating core %d, currently running: %d\n", idle, active_core[idle]);
				}
				update_core(idle, next_job);
				if(DEBUG){
					printf("Core %d is now running job %d\n", idle, active_core[idle]);
				}
				

				// Update last active time to next time cycle
				next_job->value[6] = time;


				idle = get_idle_core();
			}
			else{
				if(DEBUG){
					printf("No valid jobs found to schedule\n");
				}
				break;
			}
		}// End while
	}// End if(0 < length)
}


/**
 * @brief Determines the next job to be scheduled for preemptive schemes
 * 
 * @param new_job An artifact from earlier versions that I was too lazy to
 * 		  remove
 *
 * @param time An integer representing the current time		  
 *
 * NOTE: This method schedules and updates everything blindly, plan
 * 	 accordingly.  Relies on the ready queue being sorted by precedence.
 */
void next_job_preempt(job_t *new_job, int time){
	
	job_t *next_job;
	
	next_job = NULL;
	int length = priqueue_size(ready_q);
	if(length > 0){
	
		for(int i = 0; i<length; ++i){
			next_job = (job_t *)priqueue_at(ready_q, i);

			// If the current job is not finished..
			if(next_job->finished != 1){
		
				if(DEBUG){
					printf("Job %d is not finished...\n", next_job->value[0]);
				}
			
				// and it is not already running...

				if(0 > next_job->core){
					if(DEBUG){
						printf("and not running, checking for free cores...\n");
					}

					// If an idle core exists, assign this
					// job to that core
					int idle = get_idle_core();

					if(9000 != idle){
						if(DEBUG){
							printf("An idle core exists to be scheduled...\n");	
							printf("Updating core %d, currently running: %d\n", idle, active_core[idle]);
						}
						update_core(idle, next_job);
						if(DEBUG){
							printf("Core %d is now running job %d\n", idle, active_core[idle]);
						}
					}
					else{
						// Otherwise, check if any of the jobs
						// existing jobs should be preempted
	
						// printf("No idle cores available, checking for preemption...\n");

						// To preserve priority, find
						// the least important task

						job_t *old_job = get_preempt_job(next_job);
						if(NULL != old_job){
							// Preempt the job
   							int core = old_job->core;
							
							// update its time
							update_running_time(old_job, time);

							// Reset its core
							old_job->core = -1;
							
							// Reset its active
							// time to ensure
							// proper running time
							// accounting
							old_job->value[6] = -1;


							// If the job has yet
							// to run, reset its
							// latency
							if(0 == old_job->value[4]){
								old_job->value[7] = -1;	
							}

							if(DEBUG){
								printf("Job %d will preempt job %d on core %d...\n", next_job->value[0], old_job->value[0], core);
								printf("Updating core %d, currently running: %d\n", core, active_core[core]);
							}
							update_core(core, next_job);
							if(DEBUG){
								printf("Core %d is now running job %d\n", core, active_core[core]);
							}

						}
						else if(DEBUG){
							printf("No preemptable jobs found...\n");	
						}
					}// End else
				}// End if (0 < next_job->core)
			}
			else if(DEBUG){
				printf("Job %d is finished, moving on...\n", next_job->value[0]);	
			}
		}// End for
	} // End if(length > 0)
}


/**
 * @brief Special snowflake method to find the next job for a Round robin
 * scheme
 *
 * @param new_job An artifact from a previous version that I was too lazy to
 * remove.
 *
 * @param time An integer representing the current time
 *
 * See notes
 */
void next_job_RR(job_t *new_job, int time){

	// Main idea:
	//   Since the new jobs are never run right away, there is no need to
	//   make this more complicated.  New jobs are simply added to the
	//   back of the list.  The next job is always at the front, the first
	//   job which is not finished and not already running.
	//
	//   In the event of a quantum timer expiring, simply move that one to
	//   the back and update the time.  When a job finishes, mark it as
	//   such and leave it be.

	job_t* next_job = NULL;
	int length = priqueue_size(ready_q);
	int found;
	if(0 < length){
	
		// If not empty, search through the queue
		// Everything should be sorted by arrival automatically by
		// priqueue_offer
		int idle = get_idle_core();

		while(9000 != idle){
			found = 0;
			for(int i = 0; i<length; ++i){
				next_job = (job_t*)priqueue_at(ready_q, i);
		
				if(NULL == next_job){
					printf("SOMETHING IS VERY WRONG...\n");
					printf("currently inspecting index %d\n", i);
					printf("core %d is idle\n", idle);
					printf("Priqueue has %d elements\n", priqueue_size(ready_q));
					assert(0);
				}
				// If the current job is not finished..
				if(next_job->finished != 1){
		
					if(DEBUG){
						printf("Job %d is not finished(%d)...\n", next_job->value[0], next_job->finished);
					}
				
					// and it is not already running...
					if(0 > next_job->core){
				
						if(DEBUG){
							printf("and not running, it will be the next job...\n");
						}
						found = 1;
						break;
					}
					else if(DEBUG){
						printf("Job %d is running, moving on...\n", next_job->value[0]);
					}
				}
				else if(DEBUG){
					printf("Job %d is finished, moving on...\n", next_job->value[0]);	
				}
			}

			if(found){
				if(DEBUG){
					printf("Updating core %d, currently running: %d\n", idle, active_core[idle]);
				}
				update_core(idle, next_job);
				if(DEBUG){
					printf("Core %d is now running job %d\n", idle, active_core[idle]);
				}
				

				// Update last active time to next time cycle
				next_job->value[6] = time;


				idle = get_idle_core();
			}
			else{
				if(DEBUG){
					printf("No valid jobs found to schedule\n");
				}
				break;
			}
		}// End while
	}// End if(0 < length)
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
void scheduler_start_up(int cores, scheme_t scheme){

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
			priqueue_init(ready_q, comparison_RR);
			break;
	}
	if(DEBUG){
		printf("Created new queue with %d elements\n", priqueue_size(ready_q));
		printf("Cores:\t");
		for(int i = 0; i<NUM_CORES; i++){printf("%d ", active_core[i]);}
		printf("\n");
	}
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
int scheduler_new_job(int job_number, int time, int running_time, int priority){

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

	daJob->core	= -1;			// Active core
	daJob->finished = 0;			// Complete/Incomplete

	if(DEBUG){
		print_queue();
	}

	// Add the new Job to the back of the queue
	int status = priqueue_offer(ready_q, daJob);
	
	if(DEBUG){
		print_queue();
	
		printf("Inserted new job at position %d\n", status);

		printf("Current ready queue size is: %d\n", priqueue_size(ready_q));
	}
	
	// Determine and set up for the next round of jobs.  It is assumed
	// that by this point, all inactive jobs have been destroyed, and that
	// this method call is the last step before the next execution
	// interval, per the instructions.
	//
	// Each of the methods below acts accordingly, updating all known jobs
	// to their correct cores and statuses.

	switch(policy){
		case RR:
			next_job_RR(daJob, time);
			break;
		case PPRI:
		case PSJF:
			next_job_preempt(daJob, time);
			break;
		case SJF:
		case PRI:
		case FCFS:
			next_job_no_preempt(daJob, time);
			break;
	}

	// Update time for all jobs
	update_time(time);

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
int scheduler_job_finished(int core_id, int job_number, int time){

	job_t* curr_job;

	// Find and update the job in question
	curr_job = get_job(job_number);
	
	if(DEBUG){
		printf("Job finished: %d\n", curr_job->value[0]);
	}
	
	// Update ended time
	curr_job->value[5] = time;

	// Mark complete
	curr_job->finished = 1;

	// Hacky hack
	// Need to ensure that final jobs do not influence new job placement
	// in preemptive priority scheme.  By setting this to maximum
	// priority, the new job will always make it past the the finished
	// jobs and compare to at least one incomplete job.
	curr_job->value[3] = 0;

	if(DEBUG){
		printf("Freeing core %d, currently running job %d...\n", core_id, active_core[core_id]);
	}
	
	// Free the core for downstream helpers
	active_core[core_id] = -1;
	curr_job->core = -1;

	// Update everything
	switch(policy){
		case FCFS:
		case PRI:
		case SJF:
			next_job_no_preempt(NULL, time);
			break;
		case PSJF:
		case PPRI:
			next_job_preempt(NULL, time);
			break;
		default:
			next_job_RR(NULL, time);
		
	}


	// Hacky hack
	curr_job->finished = 0;

	// Update time for all jobs
	update_time(time);
	
	// end Hacky hack
	curr_job->finished = 1;


	// return the next item to run on the core in question, or -1 if idle
	return active_core[core_id];
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.
 
  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  NOTE - not in the rubric, but there is some ill-defined behavior:
  	If no other jobs exist, a job should not be preempted at all.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time){
	
	if(DEBUG){
		printf("Quantum time expired for core %d\n", core_id);
	}

	scheduler_show_queue();
	
	job_t * current_job;
	int position = -1;
	int no_other_jobs = 0;


	// Find the currently running job
	int current_job_number = active_core[core_id];
	assert(current_job_number >= 0);
	
	// Verify that it exists...
	current_job = (job_t*)get_job(current_job_number);
	assert(NULL!=current_job);

	// Review each job to find the position of the current job, 
	// and to determine if there are any jobs besides the currently running job
	for(int i = 0; i<priqueue_size(ready_q); ++i){
		current_job = priqueue_at(ready_q, i);
		if(active_core[core_id] == current_job->value[0]){
			// Mark position of currently running job
			position = i;
		}
		else{
			if(!current_job->finished){
				// and not already running...
				if(0 > current_job->core){
					no_other_jobs = 1;
				}
			}
		}
	}
	assert(position >= 0);
	
	// If there are no other active jobs, there is nothing to do
	if(!no_other_jobs){
		return current_job_number;	
	}

	if(DEBUG){
		printf("Removing running job %d at position %d, current size is %d...\n", active_core[core_id], position, priqueue_size(ready_q));
	}

	current_job = (job_t*)priqueue_remove_at(ready_q, position);

	if(DEBUG){
		printf("Job %d removed, current size is %d, updating metrics...\n", current_job->value[0], priqueue_size(ready_q));
	}

	if(current_job->value[0] != active_core[core_id]){
		printf("Sanity check failed in quantum expired...\n");
		printf("The job removed %d does not match the expected job %d\n", current_job->value[0], active_core[core_id]);
		assert(0);
	}

	active_core[core_id] = -1;

	// update its time
	update_running_time(current_job, time);

	if(DEBUG){
		printf("Time updated, checking job again...\n");
		printf("Old job %d is OK...\n", current_job->value[0]);
	}

	// Reset its core
	current_job->core = -1;
							
	// Reset its active time to ensure proper running time accounting
	current_job->value[6] = -1;


	// If the job has yet to run, reset its latency
	if(0 == current_job->value[4]){
		current_job->value[7] = -1;	
	}
	
	if(DEBUG){
		printf("Adding old job to back of queue...\n");
	}

	// Add the old job to the back of the queue
	priqueue_offer(ready_q, current_job);

	if(DEBUG){
		printf("Checking that the job is in fact in the queue...\n");
	}

	int sanity_check = current_job->value[0];

	current_job = (job_t *)get_job(sanity_check);

	if(NULL == current_job){
		printf("get_job returned NULL, The job was not properly inserted...\n");
		assert(0);
	}

	if(DEBUG){
		printf("The job is in the queue\n");
		printf("Scheduling next job...\n");
	}

	// Call to schedule the next job to run
	next_job_RR(NULL, time);


	if(DEBUG){
		printf("Updating time...\n");
	}

	// Update time
	update_time(time);

	if(DEBUG){
		printf("Done with quantum expired...\n");
		print_queue();
	}
	
	return active_core[core_id];
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all
      jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time(){
	job_t * current_job;
	int sum = 0;
	for(int i = 0; i<priqueue_size(ready_q); ++i){
		current_job = (job_t *)priqueue_at(ready_q, i);
		int total, waiting;
		total = current_job->value[5] - current_job->value[1];
		waiting = total - current_job->value[2];

		if(DEBUG){
			printf("Job %d waited %d time units\n", current_job->value[0], waiting);
		}
		
		// waiting time is total time less running time
		sum += waiting;
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
float scheduler_average_turnaround_time(){
	int time = 0;
	job_t *current_job;
	for(int i = 0; i<priqueue_size(ready_q); ++i){
		current_job = (job_t*)priqueue_at(ready_q, i);
		
		if(DEBUG){
			printf("Job %d took a total of %d time units\n", current_job->value[0], current_job->value[5]-current_job->value[1]);
		}
				
		// Add in total time (end-start)
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
float scheduler_average_response_time(){
	int time = 0;
	job_t *current_job;
	for(int i = 0; i<priqueue_size(ready_q); ++i){
		current_job = (job_t*)priqueue_at(ready_q, i);
		
		if(DEBUG){
			printf("Job %d had latency of %d time units\n", current_job->value[0], current_job->value[7]);
		}
		
		time += current_job->value[7];
	}
	return (1.0 * time)/priqueue_size(ready_q);
}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up(){
	job_t * curr;
	while(0 < priqueue_size(ready_q)){
		curr = (job_t*)priqueue_poll(ready_q);	
		free(curr->value);
	}
	curr = NULL;
	priqueue_destroy(ready_q);
	free(ready_q);
	free(active_core);
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

void scheduler_show_queue(priqueue_t* q){
	if(DEBUG){
		printf("\n");
		for(int i=0; i<priqueue_size(ready_q); ++i){
			job_t *daJob = (job_t*)priqueue_at(ready_q, i);
			printf("\tJob %d:\tArrived:\t%d\tBurst:\t\t%d\tPriority:\t%d\tCore:\t%d\tRunning:\t%d\tComplete:\t%d\n", daJob->value[0], daJob->value[1], daJob->value[2], daJob->value[3], daJob->core, (daJob->core>=0)?1:0, daJob->finished);
			printf("\t       \tLast active:\t%d\tRuntime:\t%d\n", daJob->value[6], daJob->value[4]);
		}
	}
}

