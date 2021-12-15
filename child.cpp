/*
	Shawn Brown
	project 4 - CS4760
	
	10/24/21
	
	child.cpp
*/

#include <unistd.h>
#include <iostream>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include "config.h"


using namespace std;

// forward declarations for semaphore and signals
void wait(int);
void signal(int);
void sigHandler(int);

struct sembuf sb;



int main(int argc, char * argv[])
{
	cout<< "child called" << endl;

	signal(SIGTERM, sigHandler);
	signal(SIGINT, sigHandler);
	
	sysClockID = atoi(argv[2]);
	semID = atoi(argv[3]);
	pcbID = atoi(argv[4]);
	int procIndex = atoi(argv[5]);

	sysClock = (unsigned int*) shmat(sysClockID, (void*)0, 0);
	pcb = (PCB*)shmat(pcbID, (void*)0,0);


	srand(getpid());
	
		
	pcb[procIndex].type = (int)(rand() % 2);
	
	int procTime;

	if(pcb[procIndex].type == 0)
	{
		procTime = (int)(rand()%3) + 1;
	
  	}
	else
	{
		procTime = (int)(rand()%5) + 1;
	}
	
	while(!pcb[procIndex].finished && !pcb[procIndex].terminated)
	{
		while ((semctl(semID, 0, GETVAL) != 1) || !pcb[procIndex].scheduled)
		{
			if((pcb[procIndex].notBlockedSec <= sysClock[0]) && (pcb[procIndex].notBlockedNano <= sysClock[1]))
			{
				pcb[procIndex].waitingIO = false;
			}
		}
		cout << "\nChild " << procIndex << ": Gained control\n";
		cout << "Blocked: " << pcb[procIndex].waitingIO << " Ready: " << pcb[procIndex].scheduled << endl;
		pcb[procIndex].waitSec += sysClock[0] - pcb[procIndex].timeCededSec;
		pcb[procIndex].waitNano += sysClock[0] - pcb[procIndex].timeCededNano;
		pcb[procIndex].cpuTimeUsedSec += 0.5;
		pcb[procIndex].cpuBurstNano = 0.5;

		
		int termProb = (int)(rand() % 10) + 1;
		cout << "probability of termination: " << termProb << endl;

		if(termProb == 1) 
		{
			pcb[procIndex].terminated = true;
		} 
		else
		{
			if(procTime <= pcb[procIndex].cpuTimeUsedSec)
			{
				pcb[procIndex].finished = true;
				cout << "Child process " << procIndex << "finished" << endl;
			}
			else
			{
				int blockProb = (int)(rand() % 10) + 1;
				if((pcb[procIndex].type == 1 && blockProb > 9) || (pcb[procIndex].type == 0 && blockProb > 4))
				{
					pcb[procIndex].waitingIO = true;
					pcb[procIndex].blockedSec = (int)(rand() % 6);
					pcb[procIndex].blockedNano = (int)(rand() % 1000);
					
					pcb[procIndex].notBlockedSec = pcb[procIndex].blockedSec + sysClock[0];
					pcb[procIndex].notBlockedNano = pcb[procIndex].blockedNano + sysClock[1];
				}
				else
				{
					pcb[procIndex].cpuBurstNano += 0.5;
					pcb[procIndex].cpuTimeUsedSec += 0.5;
	
				}

			}
		}
		pcb[procIndex].timeCededSec = sysClock[0];
		pcb[procIndex].timeCededNano == sysClock[1];


		cout << "Child process: " << procIndex << "cedes control\n";

		wait(semID); 
	}
	
	shmdt(sysClock);
	shmdt(pcb);
	
	return 0;

}

// sends wait signal to shared semaphore
void wait(int semID)
{
	while(semctl(semID, 0, GETVAL) <= 0)
	{
		sleep(1);
	}
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = 0;

	if(semop(semID, &sb, 1) == -1)
	{
		perror("ERROR: semaphore error");
		exit(5);
	}

}


void signal(int)
{
	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = 0;

	if(semop(semID, &sb, 1) == -1)
	{
		perror("ERROR: semaphore error");
		exit(5);
	}
	
}

// signal handler that detatches memory and exits
void sigHandler(int s)
{
	shmdt(sysClock);
	shmdt(pcb);
	_exit(0);

}
