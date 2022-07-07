/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>

#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <wait.h>

#include "command.hh"
#include "shell.hh"

extern int errno;
using namespace std;

//local boolean for backgrounds for signal handling
bool bg = false;

Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
	_append = false;
    _background = false;
}

//handle for sigchld zombie processes
void endzombie_c( int sig )
{
	pid_t pid = waitpid(-1, NULL, WSTOPPED);
	while(waitpid(-1, NULL, WNOHANG) > 0); 
	if(bg && pid != -1)
	{
		printf("[%d] exited\n", pid);
	}
	sig++;
}


void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _inFile ) {
        delete _inFile;
    }


	if (_outFile && _errFile && _outFile == _errFile) {
		delete _outFile;
	}
	else
	{
		if ( _outFile ) {
			delete _outFile;
		}

		if ( _errFile ) {
			delete _errFile;
		}
	}
    _inFile = NULL;
    _outFile = NULL;
    _errFile = NULL;

	_append = false;
    _background = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}


void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        if(isatty(0) && !source){Shell::prompt();}
        return;
    }

    // Print contents of Command data structure
	//delete this when you're done with everything
    //print();

    // Add execution here
    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec

	//saving stdin/out
	int stin = dup(0);
	int stout = dup(1);	
	int sterr = dup(2);

	//retcode = 0;

	//initial input
	int fdin;
	if (_inFile) {
		fdin = open(_inFile->c_str(), O_RDONLY, 0);
		if(fdin < 0)
		{
			perror("error: myshell: ");
			retcode = 2;
			clear();
			close(fdin);
			if(isatty(0) && !source) {Shell::prompt();}
			close(stin);
			close(stout);
			close(sterr);
			return;
		}
	}
	else {
		// Use default input
		fdin=dup(stin);
		close(fdin);
	}

	//number of simple commands in line
	int num_simple_commands = _simpleCommands.size();

	bg = _background;

	//fd for outfile
	int fdout;
	//fd for errfile
	int fderr;
	//pointer for fork return
	int ret;

	//convert string vector to array of const char *
	//const char * vector for arguments
	vector<const char *> argsvec;
	//simple command count
	int count = 0;

	//handle interrupt for zombie processes
	struct sigaction sigchld_action;
	sigchld_action.sa_handler = endzombie_c;
	sigemptyset(&sigchld_action.sa_mask);
	sigchld_action.sa_flags = SA_RESTART;

	if(sigaction(SIGCHLD, &sigchld_action, NULL)){
		perror("sigaction");
		exit(0);
	}
	//iterate through simple commands
	//marker
	for(auto s : _simpleCommands)
	{
		
		//adding each argument in this simplecommand to the const char * vector
		for(auto arg : s->_arguments)
		{
			argsvec.push_back(arg->c_str());
		}
		//constant for value of command name
		const char* scname = argsvec[0];

		// //set lastarg val
		lastarg = argsvec.back();

		//char* array with all of the arguments in this simplecommand 
		//size is 1 larger to accomodate for NULL at end
		char* args[argsvec.size() + 1];
		for(int i = 0; i < (int)argsvec.size(); i++)
		{
			//convert to chars
			args[i] = (char *)argsvec[i];
		}
		//final index is set to NULL
		args[argsvec.size()] = NULL;

		///////////////
		//exit:
		if(strcmp(scname, (const char *)"exit") == 0)
		{
			//only print prompt if stdin is terminal
			if(isatty(0))
			{
				printf("%s\n", "bye bye");
			}
			close(stin);
			close(stout);
			close(sterr);
			exit(0);
		}

		////////////////

		//setenv:

		if(strcmp(scname, (const char*)"setenv") == 0)
		{
			if(setenv(argsvec[1], argsvec[2], 1) == -1) {perror("myshell"); retcode = 2;}
			// Clear to prepare for next command
			clear();
			// Print new prompt
			if(isatty(0) && !source) {Shell::prompt();}
			close(stin);
			close(stout);
			close(sterr);
			return;
		}

		////////////////

		//unsetenv:
		if(strcmp(scname, (const char*)"unsetenv") == 0)
		{
			if(unsetenv(argsvec[1]) == -1) {perror("myshell"); retcode = 2;}
			// Clear to prepare for next command
			clear();
			// Print new prompt
			if(isatty(0) && !source) {Shell::prompt();}
			close(stin);
			close(stout);
			close(sterr);
			return;
		}

		////////////////

		//cd:
		if(strcmp(scname, (const char*)"cd") == 0)
		{
			if(!argsvec[1])
			{
				if(chdir(getenv("HOME")) == -1)
				{
					fprintf(stderr, "cd: can't cd to home");
					retcode = 2;
				}
			}
			else
			{
				if(chdir(argsvec[1]) == -1) 
				{
					fprintf(stderr, "cd: can't cd to %s\n", argsvec[1]);
					retcode = 2;
				}
			}
			// Clear to prepare for next command
			clear();
			// Print new prompt
			if(isatty(0) && !source) {Shell::prompt();}
			close(stin);
			close(stout);
			close(sterr);
			return;
		}

		////////////////

		//redirect input
		dup2(fdin, 0);
		close(fdin);
		//setup redirected output
		//if on final simple command in vector
		if(count == num_simple_commands - 1)
		{
			//if outfile specified
			if (_outFile)
			{
				//if append mode
				if(_append)
				{
					_append = false;
					fdout = open(_outFile->c_str(), O_CREAT|O_WRONLY|O_APPEND, 0664);
				}
				//otherwise
				else
				{
					//set fdout to outfile
					fdout = open(_outFile->c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0664);
				}
			}
			//else 
			else
			{
				//use default stdout
				fdout = dup(stout);
			}
			//if errfile specified
			if(_errFile)
			{
				//if err and out are the same, set err to out
				if(_errFile == _outFile)
				{
					fderr = fdout;
				}
				//otherwise
				else
				{
					//if append mode
					if(_append)
					{
						_append = false;
						fderr = open(_errFile->c_str(), O_CREAT|O_WRONLY|O_APPEND, 0664);
					}
					//otherwise
					else
					{
						//set fderr to errfile
						fderr = open(_errFile->c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0664);
					}
				}
			}
			else
			{
				//use default stderr
				fderr = dup(sterr);
			}
		}
		else
		{
			//if not last simple command, create pipe
			int fdpipe[2];
			pipe(fdpipe);
			fdout=fdpipe[1];
			fdin=fdpipe[0];
		}
		//redirect output
		dup2(fdout, 1);
		//redirect error
		dup2(fderr, 2);
		//close both
		close(fdout);
		close(fderr);
		
		//fork and execvp call
		ret = fork();
		if(ret == 0)
		{
			//printenv:
			if(strcmp(scname, (const char*)"printenv") == 0)
			{
				char **p = environ;
				while(*p != NULL) 
				{
					printf("%s\n", *p);
					p++;
				}
			}
			else
			{
				execvp(scname, args);
				perror("myshell: execvp");
				retcode = 2;
			}
			exit(0);
		}
		count++;
		argsvec.clear();
	}
	//restore default in/out
	dup2(stin, 0);
	dup2(stout, 1);
	dup2(sterr, 2);
	close(stin);
	close(stout);
	close(sterr);

	//if not background, wait for process to finish
	if(!_background) 
	{
		waitpid(ret, &retcode, 0);
	}
	else /*if(_background)*/ 
	{ 
		backpid = ret; 
	}
	
	if(WIFEXITED(retcode))
	{
		retcode = WEXITSTATUS(retcode);
		if(retcode != 0 && getenv("ON_ERROR"))
		{
			printf("%s\n", getenv("ON_ERROR"));
		}
	}

    // Clear to prepare for next command
    clear();

    // Print new prompt
	if(isatty(0) && !source) {Shell::prompt();}
}

SimpleCommand * Command::_currentSimpleCommand;
