#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_SERVICES 10
#define MAX_NAME_LEN 64

typedef struct {
    char name[MAX_NAME_LEN];
    pid_t pid;
    int running;
} Service;

Service services[MAX_SERVICES];
int service_count = 0;

void init_services() {
    memset(services, 0, sizeof(services));
}

int start_service(const char* name, char* const args[]) {
    if (service_count >= MAX_SERVICES) {
        fprintf(stderr, "Maximum services reached\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execvp(args[0], args);
        exit(1);
    } else if (pid > 0) {
        // Parent process
        strncpy(services[service_count].name, name, MAX_NAME_LEN-1);
        services[service_count].pid = pid;
        services[service_count].running = 1;
        service_count++;
        printf("Started service %s with PID %d\n", name, pid);
    }
    return pid;
}

void list_services() {
    printf("Running services:\n");
    for (int i = 0; i < service_count; i++) {
        if (services[i].running) {
            printf("%s (PID: %d)\n", services[i].name, services[i].pid);
        }
    }
}

int main() {
    printf("systemdc - Simple service manager\n");
    init_services();

    char command[256];
    while (1) {
        printf("systemdc> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "list") == 0) {
            list_services();
        } else if (strncmp(command, "start ", 6) == 0) {
            char *args[] = {command + 6, NULL};
            start_service(command + 6, args);
        } else if (strcmp(command, "exit") == 0) {
            break;
        } else {
            printf("Unknown command. Available commands: start, list, exit\n");
        }
    }

    return 0;
}