#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_ARGS 64

// Function prototypes for built-in commands
static int fuzzy_echo(int argc, char **argv);
static int fuzzy_pwd(int argc, char **argv);
static int fuzzy_cd(int argc, char **argv);

// Command structure
struct command {
    const char *name;
    int (*func)(int argc, char **argv);
    const char *help;
};

// Command table
static struct command commands[] = {
    {"echo", fuzzy_echo, "Print text to stdout"},
    {"pwd", fuzzy_pwd, "Print working directory"},
    {"cd", fuzzy_cd, "Change directory"},
    {NULL, NULL, NULL}
};

// Built-in command implementations
static int fuzzy_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printf("%s%s", argv[i], (i < argc - 1) ? " " : "");
    }
    printf("\n");
    return 0;
}

static int fuzzy_pwd(int argc, char **argv) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    }
    return 1;
}

static int fuzzy_cd(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: cd <directory>\n");
        return 1;
    }
    if (chdir(argv[1]) != 0) {
        perror("cd");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("FuzzyBox - A simple BusyBox clone\n");
        printf("Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    char *cmd_name = argv[1];
    
    // Search for command in table
    for (struct command *cmd = commands; cmd->name != NULL; cmd++) {
        if (strcmp(cmd_name, cmd->name) == 0) {
            return cmd->func(argc - 1, argv + 1);
        }
    }

    fprintf(stderr, "Unknown command: %s\n", cmd_name);
    return 1;
}