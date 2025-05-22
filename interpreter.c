#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"
#include "shell.h"
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include "scheduler.h"
#include "queue.h"
#include "pcb.h"
#include <pthread.h>

#define PAGE_SIZE  3
int MAX_ARGS_SIZE = 10;



int badcommand () {
    printf ("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist () {
    printf ("Bad command: File not found\n");
    return 3;
}

int badcommandMkdir () {
    printf ("Bad command: my_mkdir\n");
    return 1;
}

int badcommandCd () {
    printf ("Bad command: my_cd\n");
    return 1;
}

int help ();
int quit ();
int set (char *var, char *value);
int print (char *var);
int source (char *script);
int badcommandFileDoesNotExist ();
int echo (char *var);
int my_ls ();
int my_mkdir (char *dir);
int my_touch (char *filename);
int my_cd (char *dir);
int exec (char *args[], int args_size);
int loadScriptIntoMemory (char *script);
int comparePCBLength (const void *a, const void *b);


// Interpret commands and their arguments
int interpreter (char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand ();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn (command_args[i], "\r\n")] = 0;
    }



    if (strcmp (command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand ();
        return help ();

    } else if (strcmp (command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand ();
        return quit ();

    } else if (strcmp (command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand ();
        return set (command_args[1], command_args[2]);

    } else if (strcmp (command_args[0], "print") == 0) {
        if (args_size != 2)
            return badcommand ();
        return print (command_args[1]);

    } else if (strcmp (command_args[0], "source") == 0) {
        if (args_size != 2)
            return badcommand ();
        return source (command_args[1]);

    } else if (strcmp (command_args[0], "echo") == 0) {
        if (args_size != 2)
            return badcommand ();
        return echo (command_args[1]);

    } else if (strcmp (command_args[0], "my_ls") == 0) {
        if (args_size != 1)
            return badcommand ();
        return my_ls ();

    } else if (strcmp (command_args[0], "my_mkdir") == 0) {
        if (args_size != 2)
            return badcommand ();
        return my_mkdir (command_args[1]);

    } else if (strcmp (command_args[0], "my_touch") == 0) {
        if (args_size != 2)
            return badcommand ();
        return my_touch (command_args[1]);

    } else if (strcmp (command_args[0], "my_cd") == 0) {
        if (args_size != 2)
            return badcommand ();
        return my_cd (command_args[1]);
    } else if (strcmp (command_args[0], "exec") == 0) {
        // printf(">>> [debug][interpreter] exec command_args[0] = %s, args_size = %d in interpreter\n", command_args[0], args_size);
        // fflush(stdout);
        return exec (&command_args[1], args_size - 1);
    } else
        return badcommand ();
}

int help () {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n ";
    printf ("%s\n", help_string);
    return 0;
}

int quit () {
    printf ("Bye!\n");
    exit (0);
}

int set (char *var, char *value) {
    // printf("in set, var is %s, value is %s\n", var, value);
    // fflush(stdout);
    char *link = " ";

    // Hint: If "value" contains multiple tokens, you'll need to loop through them, 
    // concatenate each token to the buffer, and handle spacing appropriately. 
    // Investigate how `strcat` works and how you can use it effectively here.
    char buffer[MAX_USER_INPUT];
    // copy var into buffer
    strcpy (buffer, var);
    strcat (buffer, link);
    strcat (buffer, value);

    mem_set_value (var, value);
    return 0;
}

int echo (char *var) {

    char arg_prefix = var[0];
    if (arg_prefix == '$') {
        // printf(">>> [debug] echo script now\n");
        // fflush(stdout);
        char var_name[MAX_TOKEN_SIZE];
        strcpy (var_name, var + 1);
        char *value = mem_get_value (var_name);

        if (strcmp (value, "Variable does not exist") == 0) {
            printf ("\n");
        } else {
            printf ("%s\n", value);
        }
    } else {
        printf ("%s\n", var);
    }
    return 0;
}

int alnumCheck (char *name) {
    for (char c = *name; c != '\0'; c = *++name) {
        if (!isalnum (c))
            return 1;
    }
    return 0;
}

int compare (const void *a, const void *b) {
    return strcmp (*(const char **) a, *(const char **) b);
}

int my_ls () {
    DIR *d = opendir (".");
    struct dirent *dir;
    int count = 0;
    int capacity = 10;
    char **file_names = malloc (capacity * sizeof (char *));

    while ((dir = readdir (d)) != NULL) {
        if (count >= capacity) {
            capacity *= 2;
            file_names = realloc (file_names, capacity * sizeof (char *));
        }
        file_names[count] = strdup (dir->d_name);
        count++;
    }
    closedir (d);
    qsort (file_names, count, sizeof (char *), compare);
    for (int i = 0; i < count; i++) {
        printf ("%s\n", file_names[i]);
        free (file_names[i]);
    }
    free (file_names);
    return 0;
}


int my_mkdir (char *dir) {
    char arg_prefix = dir[0];
    if (arg_prefix == '$') {
        char dir_name[MAX_TOKEN_SIZE];
        strcpy (dir_name, dir + 1);
        char *value = mem_get_value (dir_name);
        // printf("<----- debug: info about value and dir_name ----->\n");
        // printf("Variable name: %s\n", dir_name);
        // printf("Value from memory: %s\n", value);
        // printf("__________\n");
        if (strcmp (value, "Variable does not exist") == 0) {
            return badcommandMkdir ();
        }
        if (alnumCheck (value)) {
            return badcommandMkdir ();
        }
        if (mkdir (value, 0777) == 0) {
            // printf("%s\n",value);
        } else {
            // printf("mkdir failed happen, current value is: %s\n",value);
            perror ("mkdir failed. ");
        }

    } else {
        if (mkdir (dir, 0777) != 0) {
            // printf("%s\n",dir);
            perror ("mkdir failed");
        }
    }
    return 0;
}

int my_touch (char *filename) {
    // assert(alnumCheck(filename));
    FILE *f = fopen (filename, "a");
    fclose (f);
    return 0;
}

int my_cd (char *dir) {
    // assert(alnumCheck(dir));
    if (alnumCheck (dir)) {
        return badcommandCd ();
    }
    int value = chdir (dir);
    if (value != 0) {
        return badcommandCd ();
    }
    return 0;
}

int print (char *var) {
    printf ("%s\n", mem_get_value (var));
    return 0;
}

int loadProgramAsPages (char *script, struct PCB *pcb) {
    if (!script || !pcb)
        return -1;

    FILE *fp = fopen (script, "r");
    if (!fp)
        return -1;

    int line_count = 0;
    char line_buffer[MAX_USER_INPUT];
    while (fgets (line_buffer, sizeof (line_buffer), fp)) {
        line_count++;
    }

    int page_count = (line_count + PAGE_SIZE - 1) / PAGE_SIZE;

    rewind (fp);

    pcb->scriptName = strdup (script);
    pcb->current_instruction = 0;
    pcb->pages_max = page_count;
    pcb->length = line_count;

    for (int i = 0; i < MAX_PAGES; i++) {
        pcb->page_table[i] = -1;
    }

    int frame_count = FRAMESIZE / PAGE_SIZE;
    int initial_page_limit = (page_count >= 2) ? 2 : 1;

    if (initial_page_limit > frame_count) {
        initial_page_limit = frame_count;
    }

    for (int page = 0; page < initial_page_limit && page < page_count; page++) {
        int frame = find_free_frame ();

        if (frame == -1) {
            frame = find_lru_frame ();

            if (frame < 0 || frame >= frame_count) {
                fprintf (stderr, "Error: Invalid frame from LRU: %d\n", frame);
                fclose (fp);
                return -1;
            }

            for (int i = 0; i < PAGE_SIZE; i++) {
                int idx = frame * PAGE_SIZE + i;
                if (idx < FRAMESIZE) {
                    safe_free (&frame_store[idx].var);
                    safe_free (&frame_store[idx].value);
                    frame_store[idx].var = (char *) EMPTY_SLOT;
                    frame_store[idx].value = (char *) EMPTY_SLOT;
                }
            }
        }

        pcb->page_table[page] = frame;

        rewind (fp);
        for (int i = 0; i < page * PAGE_SIZE; i++) {
            if (!fgets (line_buffer, sizeof (line_buffer), fp)) {
                break;
            }
        }

        for (int i = 0; i < PAGE_SIZE; i++) {
            if (fgets (line_buffer, sizeof (line_buffer), fp)) {
                line_buffer[strcspn (line_buffer, "\r\n")] = '\0';
                load_line_to_frame (frame, i, line_buffer, script, page);
            } else {
                load_line_to_frame (frame, i, "", script, page);
            }
        }
    }

    fclose (fp);
    return 0;
}


int source (char *script) {
    // printf("the script is %s\n", script);
    //int script_length = loadScriptIntoMemory (script);

    struct PCB new_pcb;
    if (loadProgramAsPages (script, &new_pcb) == -1) {
        return badcommandFileDoesNotExist ();
    }
    enqueue (new_pcb, FCFS);
    scheduler ();
    return 0;
}
int exec (char *args[], int args_size) {
    // printf(">>> [debug][exec] args = %s, args_size = %d in the beginning of exec\n", args[0], args_size);
    // fflush(stdout);

    bool is_background = false;
    int policy = FCFS;

    if (strcmp (args[args_size - 1], "#") == 0) {
        is_background = true;
        args_size--;
    }

    if (strcmp (args[args_size - 1], "FCFS") == 0) {
        policy = FCFS;
    } else if (strcmp (args[args_size - 1], "SJF") == 0) {
        policy = SJF;
    } else if (strcmp (args[args_size - 1], "RR") == 0) {
        policy = RR;
    } else if (strcmp (args[args_size - 1], "AGING") == 0) {
        policy = AGING;
    } else if (strcmp (args[args_size - 1], "RR30") == 0) {
        policy = RR30;
    } else {
        return badcommand ();
    }
    args_size--;

    struct PCB process_list[args_size];
    int count = 0;

    for (int i = 0; i < args_size; i++) {
        // printf(">>> [debug][exec] loading args[%d] = %s\n", i, args[i]);
        // fflush(stdout);

        struct PCB new_pcb;
        if (loadProgramAsPages (args[i], &new_pcb) == -1) {
            // printf(">>> [debug][exec] failed to load %s\n", args[i]);
            return badcommandFileDoesNotExist ();
        }
        // printf(">>> [debug][exec] new_pcb = %s before assign to process_list in exec\n", new_pcb.scriptName);
        // fflush(stdout);
        process_list[count++] = new_pcb;
    }
    // printf(">>> [debug][exec] process_list = %s before enqueue to ready queue in exec\n", process_list[0].scriptName);
    // fflush(stdout);
    if (policy == SJF || policy == AGING) {
        qsort (process_list, count, sizeof (struct PCB), comparePCBLength);
    }

    for (int i = 0; i < count; i++) {
        enqueue (process_list[i], policy);
    }

    // printf(">>> [debug][exec] policy = %d\n", policy);
    // fflush(stdout);

    if (policy == RR || policy == RR30) {
        // printf(">>> [debug][exec] policy = %d in exec\n", policy);
        // fflush(stdout);
        schedulerForRR (policy == RR ? 2 : 30);
    } else if (policy == AGING) {
        schedulerForAging ();
    } else {
        scheduler ();
    }

    return 0;
}

int comparePCBLength (const void *a, const void *b) {
    struct PCB *pcb_a = (struct PCB *) a;
    struct PCB *pcb_b = (struct PCB *) b;

    return pcb_a->length - pcb_b->length;
}



int processInput (char *current_command) {
    // printf("this is the command going to execute:\"%s\"\n",current_command);
    // fflush(stdout);

    int w = 0;
    int ix = 0;
    int wordlen = 0;
    char tmp[MAX_USER_INPUT];
    char *words[MAX_USER_INPUT];
    // printf("this is the command going to execute:\"%s\"\n",current_command);
    for (ix = 0; current_command[ix] == ' ' && ix < 1000; ix++);        // skip white spaces
    while (current_command[ix] != '\n' && current_command[ix] != '\0'
           && ix < 1000) {

        // extract a word
        for (wordlen = 0; !wordEnding (current_command[ix]) && ix < 1000;
             ix++, wordlen++) {
            tmp[wordlen] = current_command[ix];
        }


        tmp[wordlen] = '\0';
        words[w] = strdup (tmp);
        w++;
        if (current_command[ix] == '\0')
            break;
        else if (current_command[ix] == ';') {
            ix++;
            words[w] = NULL;
            // printf("w is %d\n",w);
            if (w > 0 && strcmp (words[0], "run") == 0) {
                if (w == 1) {
                    printf ("there is no command following run");
                    return 1;
                } else {
                    char **new_words = words + 1;
                    return execution_fork (words + 1);
                }
            }
            // for (int i = 0; i < w; i++) {
            //     printf("in processInput, words[%d] is %s\n", i, words[i]);
            // }

            int errorCode = interpreter (words, w);
            if (errorCode != 0) {
                return errorCode;
            }
            w = 0;
            for (int i = 0; words[i] != NULL; i++) {
                free (words[i]);
            }
            memset (words, 0, MAX_USER_INPUT * sizeof (char *));

        }
        ix++;
    }


    words[w] = NULL;
    // printf("w is %d\n",w);

    if (w > 0 && strcmp (words[0], "run") == 0) {
        if (w == 1) {
            printf ("there is no command following run");
            return 1;
        } else {
            char **new_words = words + 1;
            return execution_fork (words + 1);
        }
    }
    // for (int i = 0; i < w; i++) {
    //     printf("in processInput, words[%d] is %s\n", i, words[i]);
    // }

    int errorCode = interpreter (words, w);
    if (errorCode != 0) {
        return errorCode;       /* code */
    }


    return errorCode;
}
