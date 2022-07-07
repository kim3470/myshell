#include <cstdio>
#include <cstdlib>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <limits.h>

#include "command.hh"

#include "shell.hh"

int yyparse(void);

bool source;
int retcode;
int backpid = 0;
std::string lastarg;
std::string path;
//handle for ctrl-c sigint
extern "C" void disp( int sig )
{
	fprintf(stderr, "\n");
	pid_t pid = waitpid(-1, NULL, WSTOPPED);
	if(pid < 0)
	{
		if(isatty(0))
		{
			Shell::prompt();
		}
	}
	sig++;
}

//handle for sigchld zombie processes
void endzombie( int sig )
{
	pid_t pid = waitpid(-1, NULL, WSTOPPED);
	while(waitpid(-1, NULL, WNOHANG) > 0); 
	if(pid != -1)
	{
		fprintf(stderr, "[%d] exited\n", pid);
	}
	sig++;
}

void Shell::prompt() {
	char * prompt = (char *)"myshell>";

	if(getenv("PROMPT"))
	{
		prompt = getenv("PROMPT");
	}
	printf("%s ", prompt);
	fflush(stdout);
}

int main(int argc, char ** argv) {
	//interrupt process if ctrl+c pressed
	struct sigaction sigint_action;
	sigint_action.sa_handler = disp;
	sigemptyset(&sigint_action.sa_mask);
	sigint_action.sa_flags = SA_RESTART;


	if(sigaction(SIGINT, &sigint_action, NULL)){
		perror("sigaction");
		exit(2);
	}

	char buf[1000];
	realpath(argv[0], buf);
	path = buf;

	if(isatty(0))
	{
		Shell::prompt();
	}

	yyparse();
} 
Command Shell::_currentCommand;
