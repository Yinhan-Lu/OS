#pragma once
#include <stdbool.h>
#include "pcb.h"

typedef struct queue_node {
    struct PCB process;
    struct queue_node *next;
} queue_node;

void enqueue (struct PCB new_pcb, int policy);
struct PCB dequeue (int policy);
int is_queue_empty (int policy);
struct queue_node *access_queue_head (int policy);
void update_queue_head (int policy, struct queue_node *new_head);
void decay_job_priorities ();
void elevate_priority (struct PCB current_process, bool is_complete);
void display_queue ();
queue_node *get_aging_queue_head (void);
void aging ();
void insert_sorted (struct PCB process, int policy);
