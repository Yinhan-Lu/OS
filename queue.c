#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pcb.h"
#include "queue.h"
#include "scheduler.h"

#define MAXIMUM(a, b) ((a) > (b) ? (a) : (b))

typedef struct queue {
    queue_node *head;
} Queue;

static Queue fcfs_queue = { NULL };
static Queue rr_queue = { NULL };
static Queue sjf_queue = { NULL };
static Queue aging_queue = { NULL };

void aging () {
    if (!aging_queue.head) {
        return;
    }
    queue_node *temp = aging_queue.head;
    while (temp) {
        if (temp->process.age > 0) {
            temp->process.age--;
        }
        temp = temp->next;
    }
}

queue_node *get_aging_queue_head (void) {
    return aging_queue.head;
}

void insert_sorted (struct PCB process, int policy) {
    queue_node *new_node = (queue_node *) malloc (sizeof (queue_node));
    if (!new_node) {
        fprintf (stderr, "Error: Memory allocation failed in enqueue()\n");
        exit (EXIT_FAILURE);
    }
    // Copy the process into the new node
    new_node->process = process;
    new_node->next = NULL;
    Queue *selected_queue;
    switch (policy) {
        case FCFS:
            selected_queue = &fcfs_queue;
            break;
        case RR:

            selected_queue = &rr_queue;
            break;
        case RR30:
            selected_queue = &rr_queue;
            break;
        case SJF:

            selected_queue = &sjf_queue;
            break;
        case AGING:
            selected_queue = &aging_queue;
            break;
        default:
            fprintf (stderr, "Error: Invalid queue policy\n");
            return;

    }

    // If the queue is empty, set the new node as the head
    if (!selected_queue->head) {
        selected_queue->head = new_node;
    } else {
        bool inserted = false;
        // If the queue is not empty, add the new node to the end of the queue
        queue_node *temp = selected_queue->head;
        while (temp->next) {
            if (new_node->process.age < temp->process.age) {
                new_node->next = temp;
                selected_queue->head = new_node;
                inserted = true;
                break;
            }
            temp = temp->next;
        }
        if (!inserted) {
            temp->next = new_node;
        }
    }

}

void enqueue (struct PCB process, int policy) {
    //struct queue_node* new_node = (struct queue_node*)malloc(sizeof(struct queue_node));
    queue_node *new_node = (queue_node *) malloc (sizeof (queue_node));
    if (!new_node) {
        fprintf (stderr, "Error: Memory allocation failed in enqueue()\n");
        exit (EXIT_FAILURE);
    }
    // Copy the process into the new node
    new_node->process = process;
    new_node->next = NULL;

    // Select the appropriate queue based on the policy
    Queue *selected_queue;
    switch (policy) {
        case FCFS:
            selected_queue = &fcfs_queue;
            break;
        case RR:

            selected_queue = &rr_queue;
            break;
        case RR30:
            selected_queue = &rr_queue;
            break;
        case SJF:

            selected_queue = &sjf_queue;
            break;
        case AGING:
            selected_queue = &aging_queue;
            break;
        default:
            fprintf (stderr, "Error: Invalid queue policy\n");
            free (new_node);
            return;

    }

    // If the queue is empty, set the new node as the head
    if (!selected_queue->head) {
        selected_queue->head = new_node;
    } else {
        // If the queue is not empty, add the new node to the end of the queue
        queue_node *temp = selected_queue->head;
        while (temp->next) {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}

struct PCB dequeue (int policy) {
    Queue *selected_queue;
    switch (policy) {
        case FCFS:
            selected_queue = &fcfs_queue;
            break;
        case RR:
            selected_queue = &rr_queue;
            break;
        case RR30:
            selected_queue = &rr_queue;
            break;
        case SJF:
            selected_queue = &sjf_queue;
            break;
        case AGING:
            selected_queue = &aging_queue;
            break;
        default:
            fprintf (stderr, "Error: Invalid queue policy\n");
            struct PCB empty_process = { 0 };
            return empty_process;
    }
    if (!selected_queue->head) {
        struct PCB empty_process = { 0 };
        return empty_process;
    }

    queue_node *temp = selected_queue->head;
    struct PCB dequeue_process = temp->process;
    selected_queue->head = temp->next;
    free (temp);
    return dequeue_process;
}

int is_queue_empty (int policy) {
    switch (policy) {
        case FCFS:
            return fcfs_queue.head == NULL;
        case RR:
            return rr_queue.head == NULL;
        case RR30:
            return rr_queue.head == NULL;
        case SJF:
            return sjf_queue.head == NULL;
        case AGING:
            return aging_queue.head == NULL;
        default:
            return 1;
    }
}

queue_node *access_queue_head (int policy) {
    switch (policy) {
        case FCFS:
            return fcfs_queue.head;
        case RR:
            return rr_queue.head;
        case RR30:
            return rr_queue.head;
        case SJF:
            return sjf_queue.head;
        case AGING:
            return aging_queue.head;
        default:
            return NULL;
    }
}

void update_queue_head (int policy, queue_node * new_head) {
    switch (policy) {
        case FCFS:
            fcfs_queue.head = new_head;
            break;
        case RR:
            rr_queue.head = new_head;
            break;
        case RR30:
            rr_queue.head = new_head;
            break;
        case SJF:
            sjf_queue.head = new_head;
            break;
        case AGING:
            aging_queue.head = new_head;
            break;
    }
}


// void decay_job_priorities() {
//     queue_node* temp = aging_queue.head;
//     while (temp) {
//         temp->process.count = MAXIMUM(0, temp->process.count - 1);
//         temp = temp->next;
//     }
// }

// void elevate_priority(struct PCB current_process, bool is_complete) {
//     if (!aging_queue.head || !aging_queue.head->next) return; 

//     queue_node *prev = NULL, *current = aging_queue.head, *lowest_priority_node = aging_queue.head, *lowest_prev = NULL;

//     while (current) {
//         if (current->process.count < lowest_priority_node->process.count) {
//             lowest_priority_node = current;
//             lowest_prev = prev;
//         }
//         prev = current;
//         current = current->next;
//     }

//     if (lowest_priority_node == aging_queue.head) return;

//     if (lowest_prev) {
//         lowest_prev->next = lowest_priority_node->next;
//     }

//     lowest_priority_node->next = aging_queue.head;
//     aging_queue.head = lowest_priority_node;

//     if (!is_complete) {
//         queue_node *new_node = (queue_node*)malloc(sizeof(queue_node));
//         if (!new_node) {
//             fprintf(stderr, "Error: Memory allocation failed in elevate_priority()\n");
//             exit(EXIT_FAILURE);
//         }
//         new_node->process=current_process;
//         insert_sorted(new_node, AGING);
//     }
// }


// void insert_sorted(queue_node* new_node, int policy) {
//     Queue *selected_queue;
//     switch (policy) {
//         case FCFS:
//             selected_queue = &fcfs_queue;
//             break;
//         case RR:
//             selected_queue = &rr_queue;
//             break;
//         case SJF:
//             selected_queue = &sjf_queue;
//             break;
//         case AGING:
//             selected_queue = &aging_queue;
//             break;
//         default:
//             free(new_node);
//             return;
//     }

//     queue_node *temp = selected_queue->head, *prev = NULL;

//     while (temp != NULL &new_node->process.count> temp->process.count) {
//         prev = temp;
//         temp = temp->next;
//     }
//     if (!prev) {
//         new_node->next = selected_queue->head;
//         selected_queue->head = new_node;
//     }else {
//         prev->next=new_node;
//         new_node->next= temp;
//     }
// }

// void display_queue(int policy) {
//     Queue *selected_queue;
//     switch (policy) {
//         case FCFS:
//             selected_queue = &fcfs_queue;
//             break;
//         case RR:
//             selected_queue = &rr_queue;
//             break;
//         case SJF:
//             selected_queue = &sjf_queue;
//             break;
//         case AGING:
//             selected_queue = &aging_queue;
//             break;
//         default:
//             return;
//     }
//     queue_node *temp = selected_queue->head;
//     while (temp) {
//         printf("%s -> ", temp->process.scriptName);
//         temp = temp->next;
//     }
//     printf("NULL\n");
// }
