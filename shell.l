
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <limits.h>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <wait.h>
#include "y.tab.hh"
#include "shell.hh"

////////////  Start added code ///////////


extern "C" char * read_line();

int mygetc(FILE * f) {
	static char *p;
	char ch;

	if (!isatty(0)) {
		// stdin is not a tty. Call real getc
    	return getc(f);
	}

	// stdin is a tty. Call our read_line.
	if (p==NULL || *p == 0) {
		char * s = read_line();	
		p = s;
	}

	ch = *p;
	p++;

	return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////


static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
	unput(c);
}

extern char ** environ;
using namespace std;
%}

%option noyywrap

%%

\n {
	return NEWLINE;
}

[ \t] {
	/* Discard spaces and tabs */
}

"|" {
	return PIPE;
}


"<" {
	return LESS;
}

">>&" {
	return GREATGREATAMPERSAND;
}

">>" {
	return GREATGREAT;
}

">&" {
	return GREATAMPERSAND;
}


"2>" {
	return TWOGREAT;
}

">" {
	return GREAT;
}

"&" {
	return AMPERSAND;
}

\~[^ \n\t\\\/]*[^ \n\t]* {
	//tilde expansion
	string text(yytext);
	string name, suff;
	char * temp;
	int start, end;
	//string to hold value
	string val;

	if(text.length() > 1)
	{
		//case for different tilde
		if(text.find("/") != string::npos)
		{
			end = text.find("/");
			name = text.substr(1, end - 1);
			suff = text.substr(end);
		}
		else
		{
			name = text.substr(1, text.size() - 1);
			suff = "";
		}
		//find directory for name
		if(getpwnam(name.c_str()))
		{
			temp = getpwnam(name.c_str())->pw_dir;	
			name = temp;
		}
		text = name + suff;
	}
	else
	{
		//case for home tilde
		text = getenv("HOME");
	}

	//set cpp_string to text and return
	const char *ret = text.c_str();
	yylval.cpp_string = new string(ret);
	return WORD;
}

[^ \n\t]*\$\{[^}]+\}[^ \n\t]* {
	//env variable expansion
	string text(yytext);
	string pref, temp, suff, name;
	int start, stop;
	//string to hold value
	string val;

	while(text.find("${") != -1)
	{
		start = text.find("${");
		stop = text.find("}");
		pref = text.substr(0, start);
		name = text.substr(start + 2, stop - start - 2);
		suff = text.substr(stop + 1);
		//special cases: $, ?, !, _, SHELL
		if(name == "$")
		{
			val = to_string((int)getpid());
		}
		else if(name == "?")
		{
			val = to_string(retcode);
		}
		else if(name == "!")
		{
			if(!backpid)
			{
				val = "";
			}
			else
			{
				val = to_string(backpid);
			}
		}
		else if(name == "_")
		{
			if(lastarg == "")
			{
				val = ""; 
			}
			else
			{
				val = lastarg; 
			}
		}
		else if(name == "SHELL")
		{
			val = path;
		}
		else
		{
			//find variable value in environ
			if(getenv(name.c_str()))
			{
				val = string(getenv(name.c_str()));
			}
			else
			{
				perror("getenv");
			}
		}
		//insert expanded string (val) into text
		text = pref + val + suff;
	}
	//set cpp_string to text and return
	const char *ret = text.c_str();
	yylval.cpp_string = new string(ret);
	return WORD;
}

(\$\([^\n]*\))|(\`[^\n]*\`) {
	//subshell:
	//substring command line from parentheses/backticks
	yylval.string_val = strdup(yytext);
	//dollar sign case
	if(yytext[0] == '$')
	{
		strncpy(yylval.string_val, yytext + 2, yyleng - 3);
		yylval.string_val[yyleng - 3] = '\0';
	}
	//backtick case
	if(yytext[0] == '`')
	{
		strncpy(yylval.string_val, yytext + 1, yyleng - 2);
		yylval.string_val[yyleng - 2] = '\0';
	}
	//concat "\nexit\n" to command
	strcat(yylval.string_val, "\nexit\n");
	//create pipes:
	//pipes for subshell input, output
	
	//buffer for output
	char buffer[4096];

	//for /proc/self/exe
	char * args[] = {(char *)"/proc/self/exe", NULL};

	int tempin, tempout;
	tempin = dup(0);
	tempout = dup(1);
	int pin[2], pout[2];
	//call pipe for both pipes
	pipe(pin);
	pipe(pout);
	dup2(pin[0], 0);
	dup2(pout[1], 1);
	close(pin[0]);
	close(pout[1]);

	//from parent: write to pin[1]: "command_line \n exit \n"
	write(pin[1], yylval.string_val, strlen(yylval.string_val));
	close(pin[1]);

	//fork child process
	int ret;
	ret = fork();
	if(ret == 0)
	{
		execvp("/proc/self/exe", args);
	}
	else if(ret < 0)
	{
		perror("subshell execvp");
		exit(1);
	}
	else
	{
		waitpid(ret, NULL, 0);
	}
	//restoring default stdin/stdout
	dup2(tempin, 0);
	dup2(tempout, 1);
	close(tempin);
	close(tempout);

	//subshell writes command output to pout[1]
	//read output from pout[0] into buffer (size 4096)
	read(pout[0], buffer, 4096);
	close(pout[0]);

	//buffer is correct here.
	//call myunputc(int c) from end of buffer to start(reversed)
	//ask ta about it
	for(int i = strlen(buffer) - 1; i >= 0; i--)
	{
		if(buffer[i] == '\n')
		{
			myunputc(' ');
		}
		else
		{
			myunputc(buffer[i]);
		}
	}
	memset(buffer, 0, 4096);
}

"source"[ ]*[^ \n\t]* {
	//source:
	//////////////////
	//this section based on a stackoverflow post on how to split
	//a string into words based on whitespace delimiter
	string str = yytext;	
	string buf;                 // Have a buffer string
	stringstream ss(str);       // Insert the string into a stream

	vector<string> tokens; // Create vector to hold our words

	while (ss >> buf) 
	{
		tokens.push_back(buf);
	}

	string arg = tokens[1];
	///////////////////
	
	yyin = fopen(arg.c_str(), "r");
	if(!yyin)
	{
		fprintf(stderr, "myshell: %s\n", strerror(errno));
		yyin = stdin;
	}
	else
	{
		source = true;
		YY_BUFFER_STATE buffer = yy_create_buffer(yyin, YY_BUF_SIZE);
		yypush_buffer_state(buffer);
		yyparse();
	}
}

<<EOF>> {
	source = false;
	yypop_buffer_state();
	yyterminate();
}

\"[^\n\"]*\" {
	//remove double quotes from text
	yylval.string_val = strdup(yytext);
	memcpy(yylval.string_val, &yytext[1], yyleng - 2);
	yylval.string_val[yyleng - 2] = '\0';
	yylval.cpp_string = new string(yylval.string_val);
	return WORD;
}

\'[^\n\']*\' {
	//remove single quotes from text
	yylval.string_val = strdup(yytext);
	memcpy(yylval.string_val, &yytext[1], yyleng - 2);
	yylval.string_val[yyleng - 2] = '\0';
	yylval.cpp_string = new string(yylval.string_val);
	return WORD;
}

(\\.|[^ \t\n<>&|])+ {
	/* Assume that file names have only alpha chars */
	yylval.cpp_string = new string(yytext);
	int size = (int) yylval.cpp_string->size();
	for(int i = 0; i < size; i ++)
	{
		if(yylval.cpp_string[0][i] == '\\')
		{
			yylval.cpp_string->erase(i, 1);
		}
	}
	return WORD;
}
