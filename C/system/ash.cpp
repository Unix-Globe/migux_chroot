#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include "lib/fuzzylib.hpp"

#define MAX_ARGS 64
#define MAX_LINE 1024
#define MAX_PATH 256
#define PROMPT "# "

void split_command(char* line, char** args) {
    int i = 0;
    args[i] = strtok(line, " \t\n");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " \t\n");
    }
}

int execute_command(char** args) {
    if (args[0] == NULL) return 1;

    // Handle built-in commands
    if (strcmp(args[0], "exit") == 0) return 0;
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            chdir(getenv("HOME"));
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("ash");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("ash");
    } else {
        // Parent process
        int status;
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int main() {
    char line[MAX_LINE];
    char* args[MAX_ARGS];
    int status = 1;

    while (status) {
        printf(PROMPT);
        if (!fgets(line, MAX_LINE, stdin)) break;
        
        split_command(line, args);
        status = execute_command(args);
    }

    return EXIT_SUCCESS;
}
