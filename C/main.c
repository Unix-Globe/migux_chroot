#ifdef __UNIX__
#include <curses.h>
#endif
#ifdef __WIN32__
#include <pdcurses.h>
#endif
#include <stdio.h>
#include <string.h>

public class unix()
{
  // main Unix class
  #define POSIX_BASESYS
  #define MAX_CHAR_SIZE 256
  #define MAX_ARGS 128
  #define UNIX_DISTRO_NAME "MIG/UX"

  void internal_libs()
  {}

  void cpu_controller()
  {
    // cpu controler
  }
  
}

int main()
{
  std::cout << "Welcome to MIG/UX! \n";
  std::cout << "internal build 01 pre-Beta\n"
  
  return 0;
}
