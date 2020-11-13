/**
 * CS2106 AY 20/21 Semester 1 - Lab 2
 *
 * This file contains function definitions. Your implementation should go in
 * this file.
 */

#include "sm.h"
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

typedef struct services
{
	sm_status_t *processList;
	int count;
	int logFileFd;
} serviceLog_t;

serviceLog_t serviceList[SM_MAX_SERVICES];
#define INITIAL_ARG_SIZE 100
#define INITIAL_NO_PROCESS 100
void sm_start_internal(const char *processes[], int needLogFile);
int serviceCount = 0;

// Use this function to any initialisation if you need to.
void sm_init(void)
{
}

// Use this function to do any cleanup of resources.
void sm_free(void)
{
	int i = 0;
	for (; i < SM_MAX_SERVICES; i++)
	{
		free(serviceList[i].processList);
	}
}

// Exercise 1a/2: start services
void sm_start(const char *processes[])
{
	sm_start_internal(processes, false);
}

// Exercise 1b: print service status
size_t sm_status(sm_status_t statuses[])
{
	int i, j = 0;
	for (i = 0; i < serviceCount; i++)
	{
		j = 0;

		//traverse to last process
		while (j < serviceList[i].count)
		{
			if (serviceList[i].processList[j].pid == 0)
			{
				break;
			}
			j++;
		}

		statuses[i].path = strdup(serviceList[i].processList[j - 1].path);
		statuses[i].pid = serviceList[i].processList[j - 1].pid;
		statuses[i].running = waitpid(serviceList[i].processList[j - 1].pid, NULL, WNOHANG) ? 0 : 1;
	}
	return serviceCount;
}

// Exercise 3: stop service, wait on service, and shutdown
void sm_stop(size_t index)
{
	serviceLog_t service = serviceList[index];
	int i = 0;
	for (; (i < service.count && service.processList[i].pid != 0); i++)
	{
		if (!waitpid(service.processList[i].pid, NULL, WNOHANG))
		{
			kill(service.processList[i].pid, SIGTERM);
		}
	}

	for (; (i < service.count && service.processList[i].pid != 0); i++)
	{
		waitpid(service.processList[i].pid, NULL, 0);
		if (service.logFileFd)
			close(service.logFileFd);
	}
}

void sm_wait(size_t index)
{
	serviceLog_t service = serviceList[index];
	int i = 0;
	for (; (i < service.count && service.processList[i].pid != 0); i++)
	{
		waitpid(service.processList[i].pid, NULL, 0);
		if (service.logFileFd)
			close(service.logFileFd);
	}
}

void sm_shutdown(void)
{
	int i = 0;
	for (; i < SM_MAX_SERVICES; i++)
	{
		sm_stop(i);
	}
}

// Exercise 4: start with output redirection
void sm_startlog(const char *processes[])
{
	sm_start_internal(processes, true);
}

// Exercise 5: show serviceLog file
void sm_showlog(size_t index)
{
	char *logFile = (char *)calloc(30, sizeof(char));
	char **process;
	pid_t pid;
	struct stat s;
	sprintf(logFile, "./service%d.log", (int)index);
	if (stat(logFile, &s) != 0)
	{
		printf("service has no log file\n");
		goto cleanup;
	}

	process = (char **)calloc(3, sizeof(char *));
	process[0] = "/bin/cat";
	process[1] = logFile;
	process[2] = NULL;

	pid = fork();
	if (pid == -1)
	{
		printf("Error starting service!\n");
		goto cleanup;
	}
	if (pid == 0)
	{
		execv(process[0], process);
	}
	else
	{
		waitpid(pid, NULL, 0);
	}

cleanup:
	free(logFile);
}

void sm_start_internal(const char *processes[], int needLogFile)
{
	int pNo = 0, start = 0;
	pid_t pid = -1;
	char **process = NULL;
	char *logFile = NULL;
	int prevPipe[2] = {0}, curPipe[2] = {0}, isLast = 0;
	int processCount = 0, logFileFd = -1;
	process = (char **)calloc(INITIAL_ARG_SIZE, sizeof(char));
	serviceList[serviceCount].processList = (sm_status_t *)calloc(INITIAL_NO_PROCESS, sizeof(sm_status_t));
	serviceList[serviceCount].count = INITIAL_NO_PROCESS;

	for (pNo = 0;; pNo++)
	{
		if (processes[pNo] == NULL)
		{
			process[start] = (char *)processes[pNo];
			pipe(curPipe);

			if (processes[pNo + 1] == NULL)
				isLast = 1;

			pid = fork();
			if (pid == -1)
			{
				printf("Error starting service!\n");
				return;
			}

			if (pid == 0)
			{ // Child Process Execute
				if (!isLast)
				{
					dup2(curPipe[1], fileno(stdout));
				}

				if (needLogFile && isLast)
				{
					logFile = (char *)calloc(30, sizeof(char));
					sprintf(logFile, "./service%d.log", serviceCount);
					logFileFd = open(logFile,
									 O_RDWR | O_APPEND | O_CREAT | O_TRUNC,
									 S_IRWXU | S_IRWXO | S_IRWXG);
					if (logFileFd == -1)
					{
						printf("Can't open file %s\n", strerror(errno));
					}
					free((void *)logFile);
					dup2(logFileFd, fileno(stdout));
					dup2(logFileFd, fileno(stderr));
					serviceList[serviceCount].logFileFd = logFileFd;
				} else if(isLast) {
					close(fileno(stdout));
					close(fileno(stderr));
				}
				if (processCount >= 1)
				{
					dup2(prevPipe[0], fileno(stdin));
					close(prevPipe[0]);
					close(prevPipe[1]);
				}
				else
				{
					close(fileno(stdin));
				}

				execv(process[0], process);
			}
			else
			{
				if (processCount >= 1)
				{
					close(prevPipe[0]);
					close(prevPipe[1]);
				}

				if (processCount >= (serviceList[serviceCount].count - 1))
				{
					serviceList[serviceCount].processList = realloc(serviceList[serviceCount].processList, INITIAL_NO_PROCESS * 2 * sizeof(sm_status_t));
					serviceList[serviceCount].count = INITIAL_NO_PROCESS * 2;
				}

				serviceList[serviceCount].processList[processCount].pid = pid;
				serviceList[serviceCount].processList[processCount].running = 1;
				serviceList[serviceCount].processList[processCount].path = strdup(process[0]);
				free(process);
				if (isLast)
				{
					serviceCount++;
					break;
				}
				else
				{
					process = (char **)calloc(INITIAL_ARG_SIZE, sizeof(char));
					start = 0;
					processCount++;
					prevPipe[0] = curPipe[0];
					prevPipe[1] = curPipe[1];
				}
			}
		}
		else
		{
			//Need one extra for NULL which is assigned in above if
			if (start >= (INITIAL_ARG_SIZE - 1))
			{
				process = realloc(process, INITIAL_ARG_SIZE * 2);
			}
			process[start] = (char *)processes[pNo];
			start++;
		}
	}
}