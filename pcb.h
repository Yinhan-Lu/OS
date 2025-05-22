#pragma once
#include <stddef.h>
#define MAX_PAGES 10

struct PCB {
    int pid;
    int position;
    int length;
    int current_instruction;
    char *scriptName;
    int count;
    int age;
    int page_table[MAX_PAGES];
    int pages_max;
};

struct PCB createPCB (char *scriptName, int length, int position);
void freePCB (struct PCB *pcb);
void updatePCBPriorities (struct PCB *pcb, int count);
void updatePCB (struct PCB *pcb, int current_instruction);
