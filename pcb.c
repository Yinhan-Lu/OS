#include <stdint.h>
#include <stdlib.h>
#include "pcb.h"
#include <string.h>


static int next_pid = 1;

struct PCB createPCB (char *scriptName, int length, int position) {
    struct PCB new_pcb;
    new_pcb.position = position;
    new_pcb.length = length;
    new_pcb.pid = next_pid++;
    new_pcb.current_instruction = 0;
    new_pcb.scriptName = strdup (scriptName);
    new_pcb.count = length;
    new_pcb.age = length;
    new_pcb.pages_max = 0;

    for (int i = 0; i < MAX_PAGES; i++) {
        new_pcb.page_table[i] = -1;
    }
    return new_pcb;
}

void freePCB (struct PCB *pcb) {
    if (pcb && pcb->scriptName) {
        free (pcb->scriptName);
        pcb->scriptName = NULL;
    }
}

void updatePCB (struct PCB *pcb, int current_instruction) {
    if (pcb) {
        pcb->current_instruction = current_instruction;
    }
}

void updatePCBPriorities (struct PCB *pcb, int count) {
    if (pcb) {
        pcb->count = count;
    }
}
