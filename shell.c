#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"
#include <sys/types.h>

#include <sys/wait.h>
#include <sys/stat.h>
int parseInput (char ui[]);
int execution_fork (char *args[]);
// Start of everything
int main (int argc, char *argv[]) {
    printf ("Frame Store Size = %d; Variable Store Size = %d\n", FRAMESIZE,
            VARMEMSIZE);

    char prompt = '$';          // Shell prompt
    char userInput[MAX_USER_INPUT];     // user's input stored here
    int errorCode = 0;          // zero means no error, default
    int batch = !isatty (STDIN_FILENO);

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }

    //init shell memory
    mem_init ();

    while (1) {
        if (!batch) {
            printf ("%c ", prompt);
        }
        // here you should check the unistd library 
        // so that you can find a way to not display $ in the batch mode
        fgets (userInput, MAX_USER_INPUT - 1, stdin);
        errorCode = parseInput (userInput);
        if (errorCode == -1)
            exit (99);          // ignore all other errors

        if (feof (stdin)) {
            stdin = fopen ("/dev/tty", "r");
            batch = 0;
        }
        memset (userInput, 0, sizeof (userInput));
    }

    return 0;
}

int wordEnding (char c) {
    // You may want to add ';' to this at some point,
    // or you may want to find a different way to implement chains.
    return c == '\0' || c == '\n' || c == ' ' || c == ';';
}

int parseInput (char inp[]) {
    char tmp[200], *words[100];
    int ix = 0, w = 0;
    int wordlen;
    int errorCode;

    char *commands[10];
    char *token = strtok (inp, ";");
    int numC = 0;


    while (token != NULL && numC < 10) {
        if (*token == ' ') {
            // printf("empty space is found\n");
            token += 1;
        }
        // printf("%s\n",token);
        commands[numC] = strdup (token);
        numC++;
        token = strtok (NULL, ";");
    }
    commands[numC] = NULL;

    char *current_command;
    for (int j = 0; commands[j] != NULL; j++) {
        w = 0;
        current_command = strdup (commands[j]);
        // printf("this is the command going to execute:\"%s\"\n",current_command);
        for (ix = 0; current_command[ix] == ' ' && ix < 1000; ix++);    // skip white spaces
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
        // printf(">>> [debug][parseInput] words = %s, w = %d in parseInput\n", words[0], w);
        // fflush(stdout);
        errorCode = interpreter (words, w);
        if (errorCode != 0) {
            return errorCode;   /* code */
        }

    }
    return errorCode;
}
int execution_fork (char *args[]) {
    pid_t pid;
    fflush (stdout);
    if (pid = fork ()) {
        int s;
        waitpid (pid, &s, 0);
        return s;
    } else {
        execvp (args[0], args);
    }
    return 0;
}
