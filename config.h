#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"

using namespace std;


FILE * logfile;
unsigned int * sysClock;

PCB * pcb;

int sysClockID;
int pcbID;
int semID;
char sysClockstr[64];
char semIDstr[64];
char logName[20];
char procIndex[20];
char pcbStr[64];


#endif
