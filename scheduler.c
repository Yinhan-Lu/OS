#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"
#include "interpreter.h"
#include "shell.h"
#include "shellmemory.h"
#include "scheduler.h"
#include "shellmemory.h"
#include <string.h>
#include <stdio.h>
pthread_t scheduler_threads[2];
pthread_mutex_t queue_lock;
bool scheduler_exit_flag = false;
#define PAGE_FAULT_OCCURRED -999



void handlePageFault (struct PCB *pcb, int page) {
    printf ("Page fault! ");

    int frame = find_free_frame ();

    if (frame == -1) {
        frame = find_lru_frame ();

        if (frame < 0 || frame >= FRAMESIZE / PAGE_SIZE) {
            fprintf (stderr, "Error: Invalid frame from LRU: %d\n", frame);
            return;
        }

        printf ("Victim page contents:\n\n");

        for (int i = 0; i < PAGE_SIZE; i++) {
            int idx = frame * PAGE_SIZE + i;
            if (idx < FRAMESIZE && !is_empty_slot (frame_store[idx].value)) {

                printf ("%s\n", frame_store[idx].value);
            }
        }

        printf ("\nEnd of victim page contents.\n");

        for (int i = 0; i < PAGE_SIZE; i++) {
            int idx = frame * PAGE_SIZE + i;
            if (idx < FRAMESIZE) {
                safe_free (&frame_store[idx].var);
                safe_free (&frame_store[idx].value);
                frame_store[idx].var = (char *) EMPTY_SLOT;
                frame_store[idx].value = (char *) EMPTY_SLOT;
            }
        }

        for (int i = 0; i < MAX_PAGES; i++) {
            if (pcb->page_table[i] == frame) {
                pcb->page_table[i] = -1;
            }
        }
    } else {
        printf ("\n");
    }

    FILE *fp = fopen (pcb->scriptName, "r");
    if (!fp) {
        printf ("Failed to open script file %s\n", pcb->scriptName);
        return;
    }

    char line[MAX_USER_INPUT];
    int skip = page * PAGE_SIZE;

    // Skip to the required page
    for (int i = 0; i < skip; i++) {
        if (!fgets (line, sizeof (line), fp)) {
            break;              // End of file
        }
    }

    // Load the page into the frame
    for (int i = 0; i < PAGE_SIZE; i++) {
        if (fgets (line, sizeof (line), fp)) {
            load_line_to_frame (frame, i, line, pcb->scriptName, page);
        } else {
            // End of file - load empty line
            load_line_to_frame (frame, i, "", pcb->scriptName, page);
        }
    }

    // Update PCB's page table
    pcb->page_table[page] = frame;
    fclose (fp);
}

int execute_instruction (struct PCB *pcb) {
    if (!pcb)
        return -1;

    int line_num = pcb->current_instruction;
    int page = line_num / PAGE_SIZE;
    int offset = line_num % PAGE_SIZE;

    int frame = pcb->page_table[page];

    if (frame == -1) {
        handlePageFault (pcb, page);
        return PAGE_FAULT_OCCURRED;
    }

    update_frame_access_time (frame);


    int idx = frame * PAGE_SIZE + offset;
    if (idx >= FRAMESIZE) {
        fprintf (stderr,
                 "Error: Frame index out of bounds in execute_instruction\n");
        return pcb->current_instruction + 1;
    }

    char *command = NULL;
    if (!is_empty_slot (frame_store[idx].value)) {
        command = strdup (frame_store[idx].value);
    }

    if (!command) {
        char key[MAX_USER_INPUT];
        sprintf (key, "%s%d", pcb->scriptName, line_num);
        command = mem_get_value (key);
    }

    if (!command || strcmp (command, "Variable does not exist") == 0
        || strcmp (command, "") == 0) {
        if (command && strcmp (command, "Variable does not exist") != 0) {
            free (command);
        }
        return pcb->current_instruction + 1;
    }

    processInput (command);
    free (command);
    return pcb->current_instruction + 1;
}

void scheduler () {
    int executed_queue = 0;
    if (!is_queue_empty (FCFS)) {
        executed_queue = FCFS;
    } else if (!is_queue_empty (SJF)) {
        executed_queue = SJF;
    }

    while (!is_queue_empty (executed_queue)) {
        // Dequeue the process from the queue   
        struct PCB current_process = dequeue (executed_queue);

        // Execute the process until it is finished
        while (current_process.current_instruction < current_process.length) {
            int next_instruction = execute_instruction (&current_process);

            if (next_instruction == PAGE_FAULT_OCCURRED) {
                enqueue (current_process, executed_queue);
                goto next_process;      // Skip to avoid memory corruption
            }

            updatePCB (&current_process, next_instruction);
        }

        // Free the PCB 
        freePCB (&current_process);

        // Remove the process from memory
        mem_remove_value (current_process.scriptName, current_process.length);

      next_process:
        continue;               // Go to next process
    }
}
void schedulerForAging () {
    while (!is_queue_empty (AGING)) {
        // Dequeue the process from the queue   
        struct PCB current_process = dequeue (AGING);

        // Execute the process until it is finished
        while (current_process.current_instruction < current_process.length) {
            int next_instruction = execute_instruction (&current_process);

            if (next_instruction == PAGE_FAULT_OCCURRED) {
                insert_sorted (current_process, AGING);
                goto next_process;      // Skip to avoid memory corruption
            }

            updatePCB (&current_process, next_instruction);
            aging ();

            if (current_process.current_instruction == current_process.length) {
                freePCB (&current_process);
                mem_remove_value (current_process.scriptName,
                                  current_process.length);
                break;
            }
        }

      next_process:
        continue;               // Go to next process
    }
}

void schedulerForRR (int time) {
    int t;

    while (!is_queue_empty (RR)) {
        struct PCB current_process = dequeue (RR);

        t = 0;
        while (current_process.current_instruction < current_process.length
               && t < time) {
            int next_instruction = execute_instruction (&current_process);

            if (next_instruction == PAGE_FAULT_OCCURRED) {
                enqueue (current_process, RR);
                goto next_process;
            }

            updatePCB (&current_process, next_instruction);
            t++;
        }

        if (current_process.current_instruction < current_process.length) {
            enqueue (current_process, RR);
        } else {
            freePCB (&current_process);
            mem_remove_value (current_process.scriptName,
                              current_process.length);
        }

      next_process:
        continue;               // Go to next process
    }
}
