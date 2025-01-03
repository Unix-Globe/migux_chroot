#ifdef __UNIX__
#include <curses.h>
#include <sys/utsname.h>
#endif
#ifdef __WIN32__
#include <pdcurses.h>
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// main Unix structure
#define POSIX_BASESYS
#define MAX_CHAR_SIZE 256
#define MAX_ARGS 128
#define UNIX_DISTRO_NAME "MIG/UX"

struct unix {
    char dummy; // placeholder member to make struct non-empty
};

void initialize()
{
	// initialize the system
  long total_memory_kb;
  FILE *meminfo = fopen("/proc/meminfo", "r");
  if (meminfo) {
    char line[256];
    while (fgets(line, sizeof(line), meminfo)) {
      if (strncmp(line, "MemTotal:", 9) == 0) {
        sscanf(line, "MemTotal: %ld kB", &total_memory_kb);
        break;
      }
    }
    fclose(meminfo);
    
    if (total_memory_kb < 524288) { // 512MB = 524288KB
      printf("Error: System requires at least 512MB RAM\n");
      exit(1);
    }
  } else {
    printf("Warning: Cannot determine system memory\n");
  }
  	// check for system requirements
	// check for system updates
	// check for system errors
	// check for system warnings
}

int main()
{
  printf("Welcome to MIG/UX!\n");
  printf("internal build 01 pre-Beta\n");
  initialize();
  
  return 0;
}
