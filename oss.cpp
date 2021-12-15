/* 
    Shawn Brown
    Project 4 - CS4760

    oss.cpp
*/




#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <sys/sem.h>
#include <sys/shm.h>
#include "config.h"
#include <queue>


using namespace std;


/* queues to store active and terminated processes */
queue<int> readyQ;
queue<int> blockedQ;
queue<int> finishedQ;



/* 

	forward declarations for all of the functions to control
	scheduling and process generation

*/
void logReport();
void addToReport(int);
void signal(int);
void incrementTime(unsigned int *, int, int);
void createProc();
void clearPCB(int);
void scheduler();
void signalHandler(int);


// global variables for the various processes
bool activeProcesses;

int 	totalCompleted = 0,
	generatedProcesses = 0,
    	totalWait = 0,
    	numCPUBound = 0,
    	numIOBound = 0;

double  totalWaitSec = 0,
	totalWaitNano = 0,
	IOBlockedSec = 0,
	IOBlockedNano = 0,
	CPUBlockedSec = 0,
	CPUBlockedNano = 0,
	totalCPUSec = 0,
	totalCPUNano = 0;
	
	
int main(int argc, char * argv[])
{
	/* set up various signals  */
	signal(SIGINT, signalHandler);
	signal(SIGALRM, signalHandler);
	signal(SIGTERM, signalHandler);


	int opt;
	int maxTime = 20;
	unsigned int maxTimeBetweenNewProcsSecs = 1;
	unsigned int maxTimeBetweenNewProcsNS = 1;
	struct tm * time;
	time_t startTime;
	string logfile = "logfile";
	
	// key for shared memory
	key_t mutex;


	// parse different options
	while ((opt = getopt(argc, argv, ":hs:l:")) != -1)
	{
		switch(opt)
		{
			case 'h':
				printf("oss [-h] [-s t] [-l f]\n");
				printf("    -s t: Indicate how many maximum seconds before the system terminates\n");
				printf("    -l f: Specify a particular name for the log file\n");
				return 0;
			case 's':
				maxTime = atoi(optarg);
				if(!(maxTime > 0))
				{
					perror("ERROR: must be greater than zero:");
					exit(-1);
				}
				break;

			case 'l':
				logfile = optarg;
				//printf("name of logfile: %s\n", logfile);
				cout << "logfile: " << logfile << endl;
				break;
			case '?':
				printf("Uknown option\n");
				perror("ERROR: invalid option");
				exit(EXIT_FAILURE);
				
		}
	}

	//kills after max time is reached
	alarm(maxTime);

	// initialize semaphore
	mutex = ftok("makefile", 1);
	semID = semget(mutex, 1, 0666 | IPC_CREAT);
	cout << semID << endl;
	semctl(semID, 0, SETVAL, 0);
	sprintf(semIDstr, "%d", semID);
	//cout << semIDstr << endl;
	

	// set up clock in shared memory
	key_t sysClockKey = ftok("makefile", 1);
	sysClockID = shmget(sysClockKey, sizeof(int[2]), 0666 | IPC_CREAT);
	sysClock = (unsigned int*)shmat(sysClockID, (void*)0, 0);
	sysClock[0] = 0;
	sysClock[1] = 0;
	sprintf(sysClockstr, "%d", sysClockID);
	
	// shared memory for the process control block
	key_t pcbKey = ftok("makefile", 1);
	pcbID = shmget(pcbKey, sizeof(PCB[18]), 0666 | IPC_CREAT);
	pcb = (PCB *)shmat(pcbID, (void*)0, 0);
	sprintf(pcbStr, "%d", pcbID);


	sprintf(logName, logfile.c_str());
	FILE * out;
	out = fopen(string(logfile).c_str(), "a");


	

	int randDelay;
	int waitTime;
	int secsTarget = sysClock[0] + maxTimeBetweenNewProcsSecs;
	
	// flag for when processes still havent finished
	bool areProcesses;
	areProcesses = true;
	

	while(areProcesses)
	{
		if(activeProcesses < 18 && (sysClock[0] == secsTarget))
		{
			createProc();
		}
	
		scheduler();

		randDelay = (rand() % 1001);
		waitTime = (rand() % 3);

		maxTimeBetweenNewProcsNS += randDelay;
		maxTimeBetweenNewProcsNS += waitTime;

		secsTarget = sysClock[0] + maxTimeBetweenNewProcsSecs;

		if(finishedQ.size() == 18)
		{
			areProcesses = false;
			cout << "All processes finished\n";
		}
		incrementTime(sysClock, 1, randDelay);
	}

	int status;
	while(wait(&status) != -1){}


	fprintf(out, "\nOSS: Finished Normally %d:%d\n", sysClock[0], sysClock[1]);
	
	// print log to file 
	logReport();

	fclose(out);

	// cleans up shared memory and semaphore
	shmdt(out);
	shmdt(pcb);

	shmctl(sysClockID, IPC_RMID, NULL);
	shmctl(pcbID, IPC_RMID, NULL);
	semctl(semID, 0, IPC_RMID);

	return 0;
}


void logReport()
{
	fprintf(logfile, "Throughput: %ds:%dns\n",(sysClock[0]/totalCompleted),(sysClock[1]/totalCompleted));
	fprintf(logfile, "Avg System Time: %fs:%fns\n",((totalWaitSec + totalCPUSec)/totalCompleted),((totalWaitNano + totalCPUNano)/totalCompleted));
	fprintf(logfile, "Avg Wait Time: %fs:%fns\n",(totalWaitSec/totalCompleted),(totalWaitNano/totalCompleted));
	fprintf(logfile, "Avg Blocked Time (CPU): %fs:%fns\n",(CPUBlockedSec/numCPUBound),(CPUBlockedNano/numCPUBound));
	fprintf(logfile, "Avg Blocked Time (IO): %fs:%fns\n",(IOBlockedSec/numIOBound),(IOBlockedNano/numIOBound));
	fprintf(logfile, "Avg CPU Time: %fs:%fns\n",(totalCPUSec/totalCompleted),(totalCPUNano/totalCompleted));
	fprintf(logfile, "CPU Idle Time: %fs:%fns\n",(sysClock[0] - totalCPUSec),(sysClock[1] - totalCPUNano)); 
}


void signalHandler(int s)
{
	printf("SIGNAL: Terminating\n");
	fprintf(logfile, "\nOSS: terminated early: %d:%d\n", sysClock[0], sysClock[1]);

	logReport();
	kill(0,SIGTERM);
	int status;
	while(wait(&status) != -1){}
	
	fclose(logfile);
	shmdt(sysClock);
	shmdt(pcb);
	shmctl(sysClockID, IPC_RMID, NULL);
	shmctl(pcbID, IPC_RMID, NULL);
	shmctl(semID, 0, IPC_RMID);

	_exit(0);
}
void signal(int)
{
	struct sembuf sb;
	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = 0;
	if(semop(semID, &sb, 1) == -1)
	{
		perror("ERROR: semaphore error");
		exit(5);
	}
}


// increments the clock, both seconds and nanoseconds
void incrementTime(unsigned int *clock, int secs, int nano)
{
	clock[0] += secs;
	clock[1] += nano;
}

// creates new processes but stops at most 18
void createProc()
{
	int newProc = -1;
	
	if(generatedProcesses < 18)
	{
		newProc = generatedProcesses;
		generatedProcesses++;
	}
	else if(!finishedQ.empty())
	{
		int status;
		wait(&status);
		newProc = finishedQ.front();
		finishedQ.pop();
		generatedProcesses++;
		clearPCB(newProc);
	}
	if(newProc != -1)
	{
		readyQ.push(newProc);
		//fprintf(logfile, "OSS: created process %d, sent to ready queue. %d:%d\n", newProc, sysClock[0], sysClock[1]);
		printf("OSS: generated process %d", newProc);
		sprintf(pcbStr, "%d", newProc);
		
		pid_t pid = fork();
		if(pid == -1)
		{
			perror("ERROR: fork error");
			exit(-1);
		}
		else if( pid == 0 )
		{
			char *args[] = {"./child", sysClockstr, semIDstr, pcbStr, procIndex, NULL};
			execvp(args[0], args);
			exit(-1);

		}
	}

}

// clears the process control block stats
void clearPCB(int procID)
{
	pcb[procID].finished = false;
	pcb[procID].scheduled = false;
	pcb[procID].cpuTimeUsedSec = 0;
	pcb[procID].finished = false;
	
}

// schedules with RR algorithm
void scheduler()
{
	if(!readyQ.empty())
	{
		int procIndex = readyQ.front();
		readyQ.pop();

		pcb[procIndex].scheduled = true;
		fprintf(logfile, "OSS: CPU gives control to process %d. %d:%d\n", procIndex, sysClock[0], sysClock[1]);
		printf("OSS: CPU gives control to process %d. %d:%d\n", procIndex, sysClock[0], sysClock[1]);		
		
		//printPCB(procIndex);
		signal(semID);

		while(semctl(semID, 0, GETVAL) != 0){}

		incrementTime(sysClock, 0, pcb[procIndex].cpuBurstNano);
		incrementTime(sysClock, 0, 22);

		fprintf(logfile, "OSS: Cpu took control from process %d. %d:%d\n", procIndex, sysClock[0], sysClock[1]);
		printf("OSS: Cpu took control from process %d. %d:%d\n", procIndex, sysClock[0], sysClock[1]);		

		pcb[procIndex].scheduled = false;
		

		if(pcb[procIndex].waitingIO)
		{
			blockedQ.push(procIndex);
			fprintf(logfile, "OSS: process %d blocked. %d:%d\n",procIndex, sysClock[0],sysClock[1]);
			printf("OSS: Process %d blocked \n", procIndex);
			
		}
		else if (pcb[procIndex].finished)
		{
			fprintf(logfile, "OSS: proccess %d has finished. %d:%d\n", procIndex, sysClock[0], sysClock[1]);
			
			finishedQ.push(procIndex);
			addToReport(procIndex);
			totalCompleted++;
		} 
		else if(pcb[procIndex].terminated)
		{
			fprintf(logfile, "OSS: process %d terminated early. %d:%d\n", procIndex, sysClock[0], sysClock[1]);		


		}
		else
		{
			readyQ.push(procIndex);
			fprintf(logfile, "OSS: process %d moved to ready queue. %d:%d\n", procIndex, sysClock[0], sysClock[1]);		

		}

	}
	else
	{
		incrementTime(sysClock, 0, 500);
		if(!blockedQ.empty())
		{
			for(int i = 0; i <  blockedQ.size(); i++)
			{
				int procIndex = blockedQ.front();
				blockedQ.pop();
				if( pcb[procIndex].waitingIO)
				{
					blockedQ.push(procIndex);
				}
				else
				{
					readyQ.push(procIndex);
					fprintf(logfile, "OSS: process %d moved from blocked to ready queue. %d:%d\n", procIndex, sysClock[0], sysClock[1]);		

				}
			}
		}
	}
}

// increments process time for the log
void addToReport(int procIndex)
{
	totalWaitSec += pcb[procIndex].waitSec;
	totalWaitNano += pcb[procIndex].waitNano;

	if(pcb[procIndex].type == 0)
	{
		numCPUBound++;
		CPUBlockedSec += pcb[procIndex].blockedSec;
		CPUBlockedNano += pcb[procIndex].blockedNano;
	}
	else
	{
		numIOBound++;
		IOBlockedSec += pcb[procIndex].blockedSec;
		IOBlockedNano += pcb[procIndex].blockedNano;
	
	}
	totalCPUSec += pcb[procIndex].cpuTimeUsedSec;
	totalCPUNano += pcb[procIndex].cpuTimeUsedNano;


}
