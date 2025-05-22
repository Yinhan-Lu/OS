#pragma once
#include "pcb.h"

#define FCFS 0
#define SJF 1
#define RR 2
#define RR30 3
#define AGING 4

void scheduler ();
void schedulerForRR (int time);
void schedulerForAging ();
void handlePageFault (struct PCB *pcb, int page);
