#include <stdio.h>

int main() {
    const char *arch = "Unknown";

    #if defined(_M_X64) || defined(__amd64__)
        arch = "x64";
    #elif defined(_M_IX86) || defined(__i386__)
        arch = "x86";
    #elif defined(_M_ARM) || defined(__arm__)
        arch = "ARMv7";
    #elif defined(_M_ARM64) || defined(__aarch64__)
        arch = "aarch64";
    #elif defined(__riscv)
        arch = "Unknown RISC";
    #elif defined(__16BIT__)
        arch = "16-bit";
    #endif

    FILE *file = fopen("../version", "w");
    if (file == NULL) {
        perror("Error opening version file");
        return 1;
    }

    fprintf(file, "Processor Architecture: %s\n", arch);
    fclose(file);

    return 0;
}