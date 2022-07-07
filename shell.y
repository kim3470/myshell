
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN NEWLINE PIPE AMPERSAND LESS GREAT GREATGREAT TWOGREAT GREATAMPERSAND GREATGREATAMPERSAND 

%{
//#define yylex yylex
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>
#include <sys/types.h>
#include <regex.h>
#include "shell.hh"

void yyerror(const char * s);
int yylex();
#define MAXFILENAME 1024
using namespace std;
vector<string> sorted;

void expandWildcard(char * pref, char * suffix) {
	//if suffix is empty and prefix has no wildcards, put prefix in argument
    if (suffix[0] == 0 ) 
	{
		if(!strchr(pref, '*'))
		{
			if(find(sorted.begin(), sorted.end(), pref) == sorted.end())
			{
				string temp = pref;
				sorted.push_back(temp);
			}
		}
        return;
    }
	//temp buffer for prefix
	char prefix[MAXFILENAME];
	//if pref arg is empty (for formatting):
	if (pref[0] == 0)
	{
		//if suffix is a component, inc suffix and make prefix pref + /
		//so we can work with just the suffix text
		if (suffix[0] == '/')
		{
			sprintf(prefix, "%s/", pref);
			suffix++;
		}
		//if suffix isn't a component, make prefix pref arg with no slash
		else
		{
			strcpy(prefix, pref);
		}
	}
	//if pref arg isn't empty, copy over previous prefix with a slash
	else
	{
		sprintf(prefix, "%s/", pref);
	}
	//get next component in suffix
	//advance suffix pointer until next slash
	char * s = strchr(suffix, '/');
	//temporary component buffer
	char component[MAXFILENAME];
	//if suffix isn't empty, copy up to first slash
	if (s != NULL)
	{
		strncpy(component, suffix, s - suffix);
		//terminate component with null
		component[s - suffix] = 0;
		//forward suffix
		suffix = s + 1;
	}
	else //if last portion of path copy entire thing into component
	{
		strcpy(component, suffix);
		suffix += strlen(suffix);
	}

	//now expand the component
	char newprefix[2 * MAXFILENAME]; // buffer for the new prefix
	//if component has no more wildcards left
	if (!strchr(component, '*') && !strchr(component, '?'))
	{
		//add current prefix and newest component to newprefix and then recurse on it
		if (prefix[0] == 0)
		{
			strcpy(newprefix, component);
		}
		else
		{
			sprintf(newprefix, "%s/%s", pref, component);
		}
		expandWildcard(newprefix, suffix);
		return;
	}
	//if component has wildcards
	//convert component to regex
	/*
	1. convert wildcard to regular expression
	* -> [^\.]*
	? -> [^\.]
	. -> \.
	also add ^ at beginning and $ at end to match start/end of word
	*/
	//allocate space for regex:
	char * reg = (char *) malloc(2 * strlen(component) + 10);
	char * a = component;
	char * r = reg;
	*r = '^'; r++; //match beginning of line
	while (*a)
	{
		if (*a == '*')
		{
			*r = '.'; r++;
			*r = '*'; r++;
		}
		else if (*a == '?')
		{
			*r = '.'; r++;
		}
		else if (*a == '.')
		{
			*r = '\\'; r++;
			*r = '.'; r++;
		}
		else
		{
			*r = *a; r++;
		}
		a++;
	}
	*r = '$'; r++; *r = 0; // match end of line and add null char
	//2. compile regular expression
	regex_t re;
	int result = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);
	if(result != 0) 
	{
		perror("regcomp");
		free(reg);
		regfree(&re);
		return;
    }

	char * open; //directory
	//if prefix is empty then list current directory
	if(prefix[0] == 0)
	{
		open = (char *)".";
	}
	else
	{
		open = prefix;
	}

	//3. list directory and add entries that match regex as args
	DIR * dir = opendir(open);
	if(dir == NULL)
	{
		free(reg);
		regfree(&re);
		return;
	}
	struct dirent * ent;
	const char * name;
	bool matched = false;
	while((ent = readdir(dir)) != NULL)
	{
		name = ent->d_name;
		//check if name matches regex
		if(regexec(&re, name, 1, NULL, 0) == 0)
		{
			matched = true;
			//if prefix is empty set newprefix to just name
			if(prefix[0] == 0)
			{
				strcpy(newprefix, name);
			}
			else //otherwise make format prefix/name
			{
				sprintf(newprefix, "%s/%s", pref, name);
			}
			//if regex looks for hidden files
			if(reg[1] == '.')
			{
				//and dirent name is hidden, recurse on newprefix and suffix
				if (name[0] != '.') 
				{
					expandWildcard(newprefix, suffix);
				}
			}
			else //if not looking for hidden, recurse on expanded prefix + suffix
			{
				expandWildcard(newprefix, suffix);
			}
		}
		if(!matched)
		{
			if (prefix[0] == 0)
			{
				strcpy(newprefix, component);
			}
			else
			{
				sprintf(newprefix, "%s/%s", pref, component);
			}
			expandWildcard(newprefix, suffix);
		}
	}
	free(reg);
	regfree(&re);
	closedir(dir);
}
%}

%%
goal:
	command_list;

command_list:
	command_line
	| command_list command_line
	;

command_line:
	pipe_list io_modifier_list background_opt NEWLINE {
		//printf("   Yacc: Execute command\n");
		Shell::_currentCommand.execute();
	}
	| NEWLINE /*accept empty cmd line*/ {
		if(isatty(0) && !source) {Shell::prompt();}
	}
	| error NEWLINE {yyerrok;}
	;

pipe_list:
	cmd_and_args
	| pipe_list PIPE cmd_and_args
	;

cmd_and_args:
	cmd_word arg_list {
		Shell::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

cmd_word:
	WORD {
		//printf("   Yacc: insert command \"%s\"\n", $1->c_str());
		Command::_currentSimpleCommand = new SimpleCommand();
		Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

arg_list:
	arg_list WORD {
		//printf("   Yacc: insert argument \"%s\"\n", $2->c_str());
		//expand wildcards if necessary
		//ie: *.c -> all that end in .c
		//ie: *src* -> all with "src" in filename
		//Command::_currentSimpleCommand->insertArgument( $2 );
		//expandWildcardsIfNecessary( $2 );
		//if no wildcard necessary
		if(!strchr($2->c_str(), '*') && !strchr($2->c_str(), '?'))
		{
			Command::_currentSimpleCommand->insertArgument($2);
		}
		else
		{
			expandWildcard((char *) "", (char *) $2->c_str());
			std::sort(sorted.begin(), sorted.end());
			for (auto s : sorted) 
			{
				Command::_currentSimpleCommand->insertArgument(new std::string(s));
			}
			sorted.clear();

		}
	}
	| /* can be empty */
	;

io_modifier_list:
	io_modifier_list io_modifier
	| /*empty*/
	;

io_modifier:
	GREATGREAT WORD {
		//if output has already been redirected print error
		//else set redirect
		//check using the command table statuses.
		if (!Shell::_currentCommand._outFile)
		{
			//printf("   Yacc: insert (append) output \"%s\"\n", $2->c_str());
			Shell::_currentCommand._outFile = $2;
			Shell::_currentCommand._append = true;
		}
		else
		{
			printf("Ambiguous output redirect.\n");
		}
	}
	| GREAT WORD {
		if (!Shell::_currentCommand._outFile)
		{
			//printf("   Yacc: insert output \"%s\"\n", $2->c_str());
			Shell::_currentCommand._outFile = $2;
		}
		else
		{
			printf("Ambiguous output redirect.\n");
		}
	}
	| GREATGREATAMPERSAND WORD {
		if (!Shell::_currentCommand._outFile && !Shell::_currentCommand._errFile)
		{
			//printf("   Yacc: insert output and error \"%s\"\n", $2->c_str());
			Shell::_currentCommand._outFile = $2;
			Shell::_currentCommand._errFile = $2;
			Shell::_currentCommand._append = true;
		}
		else if (Shell::_currentCommand._outFile)
		{
			printf("Ambiguous output redirect.\n");
		}
		else if (Shell::_currentCommand._errFile)
		{
			printf("Ambiguous error redirect.\n");
		}
	}
	| GREATAMPERSAND WORD {
		if (!Shell::_currentCommand._outFile && !Shell::_currentCommand._errFile)
		{
			//printf("   Yacc: insert output and error \"%s\"\n", $2->c_str());
			Shell::_currentCommand._outFile = $2;
			Shell::_currentCommand._errFile = $2;
		}
		else if (Shell::_currentCommand._outFile)
		{
			printf("Ambiguous output redirect.\n");
		}
		else if (Shell::_currentCommand._errFile)
		{
			printf("Ambiguous error redirect.\n");
		}
	}
	| LESS WORD {
		if (!Shell::_currentCommand._inFile)
		{
			//printf("   Yacc: insert input \"%s\"\n", $2->c_str());
			Shell::_currentCommand._inFile = $2;
		}
		else
		{
			printf("Ambiguous input redirect.\n");
		}
	}
	| TWOGREAT WORD {
		if (!Shell::_currentCommand._errFile)
		{
			//printf("   Yacc: insert error \"%s\"\n", $2->c_str());
			Shell::_currentCommand._errFile = $2;
		}
		else
		{
			printf("Ambiguous error redirect.\n");
		}
	}
	;

background_opt:
	AMPERSAND {
		Shell::_currentCommand._background = true;
	}
	| /*empty*/
	;

%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif
