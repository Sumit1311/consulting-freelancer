/* Fill in your Name and GNumber in the following two comment fields
 * Name:
 * GNumber:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "structs.h"
#include "constants.h"
#include "scheduler.h"
void freeQueue(Process *process);

/* Initialize the Schedule struct
 * Follow the specification for this function.
 * Returns a pointer to the new Schedule or NULL on any error.
 */
Schedule *scheduler_init() {
	Schedule *schedule = (Schedule *) malloc(sizeof(Schedule));
	//Initializing the ready queue
	schedule->ready_queue = (Queue *) malloc(sizeof(Queue));
	schedule->ready_queue->head = NULL;
	schedule->ready_queue->count = 0;
	//Initializing the stopped queue
	schedule->stopped_queue = (Queue *) malloc(sizeof(Queue));
	schedule->stopped_queue->head = NULL;
	schedule->stopped_queue->count = 0;
	//Initializing the defunct queue
	schedule->defunct_queue = (Queue *) malloc(sizeof(Queue));
	schedule->defunct_queue->head = NULL;
	schedule->defunct_queue->count = 0;

	return schedule;
}

/* Add the process into the appropriate linked list.
 * Follow the specification for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int scheduler_add(Schedule *schedule, Process *process) {
	//Add created Process to ready queue
	Queue * tempQueue;
	if (process->flags == CREATED) {
		process->flags = READY;
		tempQueue = schedule->ready_queue;
		//Add process to front of ready
		process->next = tempQueue->head;
		tempQueue->head = process;
		return 0;

	} else if (process->flags == READY) {
		//check time remaining
		if (process->time_remaining > 0) {
			tempQueue = schedule->ready_queue;
			//Add process to front of ready
			process->next = tempQueue->head;
			tempQueue->head = process;
			return 0;
		} else {
			if (process->time_remaining == 0) {
				process->flags = DEFUNCT;
				tempQueue = schedule->defunct_queue;
				//Add process to front of defunct queue
				process->next = tempQueue->head;
				tempQueue->head = process;
				return 0;
			}

		}
	} else if (process->flags == DEFUNCT) {
		tempQueue = schedule->defunct_queue;
		//Add process to front of defunct queue
		process->next = tempQueue->head;
		tempQueue->head = process;
		return 0;

	}

	return -1;
}

/* Move the process with matching pid from Ready to Stopped.
 * Change its State to Stopped.
 * Follow the specification for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int scheduler_stop(Schedule *schedule, int pid) {
	Process *tmp = schedule->ready_queue->head;
	Process *priv = NULL;
	Queue * tempQueue;
	while (tmp != NULL) {
		if (tmp->pid == pid) {
			//One process in ready
			if (priv == NULL) {
				schedule->ready_queue->head = priv;

			} else {
				//Remove From Ready
				priv->next = tmp->next;
			}

			//Add to stopped queue
			tmp->flags = STOPPED;
			tempQueue = schedule->stopped_queue;
			//Add process to front of stopped queue
			tmp->next = tempQueue->head;
			tempQueue->head = tmp;
			return 0;

		}
		priv = tmp;
		tmp = tmp->next;

	}
	return -1;
}

/* Move the process with matching pid from Stopped to Ready.
 * Change its State to Ready.
 * Follow the specification for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int scheduler_continue(Schedule *schedule, int pid) {
	Process *tmp = schedule->stopped_queue->head;
	Process *priv = NULL;
	Queue * tempQueue;
	while (tmp != NULL) {
		if (tmp->pid == pid) {
			//One process in stopped
			if (priv == NULL) {
				schedule->stopped_queue->head = priv;

			} else {
				//Remove From stopped
				priv->next = tmp->next;
			}

			//Add to ready queue
			tmp->flags = READY;
			tempQueue = schedule->ready_queue;
			//Add process to front of ready queue
			tmp->next = tempQueue->head;
			tempQueue->head = tmp;
			return 0;

		}
		priv = tmp;
		tmp = tmp->next;

	}
	return -1;
}

/* Remove the process with matching pid from Defunct.
 * Follow the specification for this function.
 * Returns its exit code (from flags) on success or a -1 on any error.
 */
int scheduler_reap(Schedule *schedule, int pid) {
	Process *tmp = schedule->defunct_queue->head;
	Process *priv = NULL;
	while (tmp != NULL) {
		if (tmp->pid == pid) {
			//One process in defunct
			if (priv == NULL) {
				schedule->ready_queue->head = priv;

			} else {
				//Remove From defunct
				priv->next = tmp->next;
			}

			//Add to stopped queue
			tmp->flags = TERMINATED;
			//free the process
			free(tmp);
			return 0;

		}
		priv = tmp;
		tmp = tmp->next;

	}
	return -1;
}

/* Create a new Process with the given information.
 * - Malloc and copy the command string, don't just assign it!
 * Set the STATE_CREATED flag.
 * If is_sudo, also set the PF_SUPERPRIV flag.
 * Follow the specification for this function.
 * Returns the Process on success or a NULL on any error.
 */
Process *scheduler_generate(char *command, int pid, int base_priority,
		int time_remaining, int is_sudo) {
//Creating a new Process and assigning values
	Process *newProcess = (Process *) malloc(sizeof(Process));
	newProcess->command = (char *) malloc(sizeof(char));
	strcpy(newProcess->command, command);
//Set it to created state
	newProcess->flags = CREATED;
//Check if SUDO
	if (is_sudo) {
		newProcess->flags = SUDO;
	}
	newProcess->next = NULL;
	if (newProcess)
		return newProcess;
	else
		return NULL;
}

/* Select the next process to run from Ready Queue.
 * Follow the specification for this function.
 * Returns the process selected or NULL if none available or on any errors.
 */
Process *scheduler_select(Schedule *schedule) {
	Process *readyProcess;
	Process * prev;
	Queue *readyQueue = schedule->ready_queue;
	Process *tmpProcess = readyQueue->head;
	readyProcess = tmpProcess;
	prev = NULL;
//Check for max priority process
	prev = tmpProcess;
	while (tmpProcess->next != NULL) {
		if (readyProcess->cur_priority < tmpProcess->cur_priority) {

			readyProcess = tmpProcess;
			prev = tmpProcess;
		}

		tmpProcess = tmpProcess->next;

	}
	if (prev != NULL) {
		tmpProcess->next = readyProcess->next;
		readyProcess->next = NULL;
	}
//Reducing the priority of all by 1
	tmpProcess = schedule->ready_queue->head;
	while (tmpProcess != NULL) {
		tmpProcess->cur_priority--;
		tmpProcess = tmpProcess->next;

	}

	return tmpProcess;
}

/* Returns the number of items in a given Linked List (Queue) (Queue)
 * Follow the specification for this function.
 * Returns the count of the Linked List, or -1 on any errors.
 */
int scheduler_count(Queue *ll) {
	if (ll->head != NULL)
		return ll->count;
	return -1;
}
/**
 * Function used to free Queues in Scheduler
 */
void freeQueue(Process *head) {
	Process* tmp = NULL;

	while (head != NULL) {
		tmp = head;
		head = head->next;
		free(tmp);
	}

}

/* Completely frees all allocated memory in the scheduler
 * Follow the specification for this function.
 */
void scheduler_free(Schedule *scheduler) {
//Free ready queue
	Process * head = scheduler->ready_queue->head;
	freeQueue(head);
//Free defunct queue
	freeQueue(scheduler->defunct_queue->head);
//Free stopped queue
	freeQueue(scheduler->stopped_queue->head);
	return;
}
