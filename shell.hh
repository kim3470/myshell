#ifndef shell_hh
#define shell_hh

#include "command.hh"

struct Shell {

  static void prompt();

  static Command _currentCommand;

};

//source command flag
extern bool source;

//extern for shell.l to access $!, $?, $_, $SHELL
extern int retcode;
extern int backpid;
extern std::string lastarg;
extern std::string path;
#endif
