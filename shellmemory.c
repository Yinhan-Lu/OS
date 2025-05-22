#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"
#include "shell.h"
#include <time.h>


const char *EMPTY_SLOT = "____EMPTY____";


struct memory_struct frame_store[FRAMESIZE];
struct memory_struct var_store[VARMEMSIZE];
struct FrameOwner frameOwners[FRAMESIZE / PAGE_SIZE];
int frame_access_time[FRAMESIZE / PAGE_SIZE];

int is_empty_slot (const char *str) {
    return str == EMPTY_SLOT || (str && strcmp (str, EMPTY_SLOT) == 0);
}

void safe_free (char **ptr) {
    if (ptr && *ptr && *ptr != EMPTY_SLOT) {
        free (*ptr);
        *ptr = (char *) EMPTY_SLOT;
    }
}

void initFrameOwners () {
    for (int i = 0; i < FRAMESIZE / PAGE_SIZE; i++) {
        frameOwners[i].scriptName = NULL;
        frameOwners[i].page = -1;
    }
}

void updateFrameOwner (int frame, char *scriptName, int page) {
    if (frame < 0 || frame >= FRAMESIZE / PAGE_SIZE) {
        return;
    }

    if (frameOwners[frame].scriptName) {
        free (frameOwners[frame].scriptName);
        frameOwners[frame].scriptName = NULL;
    }

    if (scriptName) {
        frameOwners[frame].scriptName = strdup (scriptName);
        frameOwners[frame].page = page;
    } else {
        frameOwners[frame].page = -1;
    }
}

extern unsigned int lru_counter;

unsigned int lru_counter = 0;

void update_frame_access_time (int frame) {
    if (frame >= 0 && frame < FRAMESIZE / PAGE_SIZE) {
        frame_access_time[frame] = lru_counter++;
    }
}

int find_lru_frame () {


    int oldest_time = frame_access_time[0];
    int oldest_index = 0;

    for (int i = 0; i < FRAMESIZE / PAGE_SIZE; i++) {
        if (frame_access_time[i] == -1) {
            return i;
        }
    }

    for (int i = 1; i < FRAMESIZE / PAGE_SIZE; i++) {
        if (frame_access_time[i] < oldest_time) {
            oldest_time = frame_access_time[i];
            oldest_index = i;
        }
    }
    return oldest_index;
}

void mem_init () {

    for (int i = 0; i < VARMEMSIZE; i++) {
        var_store[i].var = (char *) EMPTY_SLOT;
        var_store[i].value = (char *) EMPTY_SLOT;
    }

    for (int i = 0; i < FRAMESIZE; i++) {
        frame_store[i].var = (char *) EMPTY_SLOT;
        frame_store[i].value = (char *) EMPTY_SLOT;
    }

    for (int i = 0; i < FRAMESIZE / PAGE_SIZE; i++) {
        frame_access_time[i] = -1;
    }

    initFrameOwners ();
}


void mem_remove_value (char *script, int length) {
    if (!script)
        return;

    char key[MAX_USER_INPUT + 10];

    for (int i = 0; i < VARMEMSIZE; i++) {

        if (!is_empty_slot (var_store[i].var)) {
            sprintf (key, "%s%d", script, i);
            if (strcmp (var_store[i].var, key) == 0) {

                safe_free (&var_store[i].var);
                safe_free (&var_store[i].value);
                var_store[i].var = (char *) EMPTY_SLOT;
                var_store[i].value = (char *) EMPTY_SLOT;
            }
        }
    }

    for (int frame = 0; frame < FRAMESIZE / PAGE_SIZE; frame++) {
        if (frameOwners[frame].scriptName
            && strcmp (frameOwners[frame].scriptName, script) == 0) {

            for (int offset = 0; offset < PAGE_SIZE; offset++) {
                int idx = frame * PAGE_SIZE + offset;

                if (idx < FRAMESIZE) {

                    safe_free (&frame_store[idx].var);
                    safe_free (&frame_store[idx].value);
                    frame_store[idx].var = (char *) EMPTY_SLOT;
                    frame_store[idx].value = (char *) EMPTY_SLOT;
                }
            }
            updateFrameOwner (frame, NULL, -1);
            // if (frameOwners[frame].scriptName) {
            //     free(frameOwners[frame].scriptName);
            // }
            // frameOwners[frame].scriptName = NULL;
            // frameOwners[frame].page = -1;
        }
    }
}


void mem_set_value (char *var_in, char *value_in) {

    for (int i = 0; i < VARMEMSIZE; i++) {
        if (!is_empty_slot (var_store[i].var)
            && strcmp (var_store[i].var, var_in) == 0) {
            safe_free (&var_store[i].value);
            var_store[i].value = strdup (value_in);
            return;
        }
    }

    for (int i = 0; i < VARMEMSIZE; i++) {
        if (is_empty_slot (var_store[i].var)) {
            var_store[i].var = strdup (var_in);
            var_store[i].value = strdup (value_in);
            return;
        }
    }

    fprintf (stderr, "Error: Variable memory is full\n");
}


char *mem_get_value (char *var_in) {
    if (!var_in)
        return "Variable does not exist";

    for (int i = 0; i < VARMEMSIZE; i++) {
        if (!is_empty_slot (var_store[i].var)
            && strcmp (var_store[i].var, var_in) == 0) {
            return is_empty_slot (var_store[i].
                                  value) ? strdup ("") : strdup (var_store[i].
                                                                 value);
        }
    }

    char script[MAX_USER_INPUT];
    int line_num;

    if (sscanf (var_in, "%[^0-9]%d", script, &line_num) == 2) {
        int page = line_num / PAGE_SIZE;
        int offset = line_num % PAGE_SIZE;

        for (int f = 0; f < FRAMESIZE / PAGE_SIZE; f++) {
            if (frameOwners[f].scriptName &&
                strcmp (frameOwners[f].scriptName, script) == 0 &&
                frameOwners[f].page == page) {

                int idx = f * PAGE_SIZE + offset;
                if (idx < FRAMESIZE && !is_empty_slot (frame_store[idx].value)) {
                    return strdup (frame_store[idx].value);
                }
            }
        }
    }


    return "Variable does not exist";
}


int find_free_frame () {
    int frame_count = FRAMESIZE / PAGE_SIZE;

    for (int frame = 0; frame < frame_count; frame++) {
        int is_free = 1;

        for (int offset = 0; offset < PAGE_SIZE; offset++) {
            int idx = frame * PAGE_SIZE + offset;
            if (idx >= FRAMESIZE) {
                fprintf (stderr, "Error: Frame index out of bounds\n");
                return -1;
            }

            if (!is_empty_slot (frame_store[idx].var)
                || !is_empty_slot (frame_store[idx].value)) {
                is_free = 0;
                break;
            }
        }

        if (is_free) {
            return frame;
        }
    }

    return -1;
}

void load_line_to_frame (int frame, int offset, char *line,
                         const char *scriptName, int page) {
    if (!scriptName)
        return;

    if (frame < 0 || frame >= FRAMESIZE / PAGE_SIZE) {
        fprintf (stderr, "Error: Invalid frame number: %d\n", frame);
        return;
    }

    if (offset < 0 || offset >= PAGE_SIZE) {
        fprintf (stderr, "Error: Invalid offset: %d\n", offset);
        return;
    }

    int idx = frame * PAGE_SIZE + offset;
    if (idx >= FRAMESIZE) {
        fprintf (stderr, "Error: Frame store index out of bounds\n");
        return;
    }

    char clean_line[MAX_USER_INPUT] = "";
    if (line) {
        strncpy (clean_line, line, MAX_USER_INPUT - 1);
        clean_line[strcspn (clean_line, "\r\n")] = 0;
    }

    char key[MAX_USER_INPUT];
    sprintf (key, "%s%d", scriptName, page * PAGE_SIZE + offset);

    safe_free (&frame_store[idx].var);
    safe_free (&frame_store[idx].value);

    frame_store[idx].var = strdup (key);
    frame_store[idx].value = strdup (clean_line);
    updateFrameOwner (frame, (char *) scriptName, page);
    update_frame_access_time (frame);
}
