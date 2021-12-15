/*
	Shawn Brown
	Project 4 - CS4760
	
	Process Control Block struct definition

*/



#ifndef PCB_H
#define PCB_H


struct PCB
{

	double cpuTimeUsedSec;
	double cpuTimeUsedNano;

	double cpuBurstSec;
	double cpuBurstNano;

	int blockedSec;
	int blockedNano;
	
	int notBlockedSec;
	int notBlockedNano;
	
	int waitSec;
	int waitNano;

	int timeCededSec;
	int timeCededNano;

	bool scheduled = false;
	bool finished = false;
	bool waitingIO = false;
	bool terminated = false;


	//enum Type {Io = 0, Cpu};
	//Type type;
	int type;
};


#endif
