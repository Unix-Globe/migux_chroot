#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(void) {
    FILE *fp;
    const char *script_content = 
        "#!/bin/sh\n"
        "# User initialization script\n"
        "export PATH=/bin:/usr/bin:/sbin:/usr/sbin\n"
        "export HOME=/home/$USER\n"
        "export SHELL=/bin/sh\n"
        "export USER=$(whoami)\n"
        "\n"
        "# Add custom user initialization here\n"
        "\n"
        "# Execute user's shell\n"
        "exec $SHELL\n";

    fp = fopen("user.z.sh", "w");
    if (fp == NULL) {
        perror("Error creating user.z.sh");
        return 1;
    }

    fprintf(fp, "%s", script_content);
    fclose(fp);

    // Set execute permissions (755)
    if (chmod("user.z.sh", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
        perror("Error setting permissions");
        return 1;
    }

    printf("user.z.sh created successfully\n");
    return 0;
}