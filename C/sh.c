#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#define MAX_INPUT 1024
#define MAX_ARGS 64

void parse_input(char *input, char **args) {
    char *token = strtok(input, " \t\n");
    int i = 0;
    
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

int builtin_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return 1;
    }
    if (chdir(args[1]) != 0) {
        perror("cd");
        return 1;
    }
    return 0;
}

#ifdef _WIN32
int execute_command(char **args) {
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    char cmd[MAX_INPUT] = "";

    // Combine args into a single command string
    for (int i = 0; args[i] != NULL; i++) {
        strcat(cmd, args[i]);
        strcat(cmd, " ");
    }

    si.cb = sizeof(si);
    if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "Command execution failed\n");
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}
#endif

int main(void) {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    
    while (1) {
        printf("$ ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        parse_input(input, args);
        
        if (args[0] == NULL) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        if (strcmp(args[0], "cd") == 0) {
            builtin_cd(args);
            continue;
        }

#ifdef _WIN32
        execute_command(args);
#else
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                perror("sh");
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            // Parent process
            wait(NULL);
        } else {
            perror("fork");
        }
#endif
    }

    return EXIT_SUCCESS;
}