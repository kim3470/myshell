/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_LINE 2048
extern char * strdup(const char *);
extern void tty_raw_mode(void);
extern void reset_input_mode(void);

// Buffer where line is stored
int left_length;
char left_buffer[MAX_BUFFER_LINE];

//holder buffer and length for area past cursor (stored in reverse index)
int right_length;
char right_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char * history [] = {
	"ls -al | grep x", 
	"ps -e",
	"cat read-line-example.c",
	"vi hello.c",
	"make",
	"ls -al | grep xxx | grep yyy"
};
int history_length = sizeof(history)/sizeof(char *);

void read_left_print_usage()
{
	char * usage = "\n"
		" ctrl-?       Print usage\n"
		" Backspace    Deletes last character\n"
		" ctrl-H       Deletes last character\n"
		" ctrl-D       Deletes current character\n"
		" ctrl-A       Move cursor to start of line\n"
		" ctrl-E       Move cursor to end of line\n"
		" left arrow   Move cursor one position to the left\n"
		" right arrow  Move cursor one position to the right\n"
		" up/down arrow history not implemented\n";
	write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

	// Set terminal in raw mode
	tty_raw_mode();

	left_length = 0;
	right_length = 0;

	// Read one line until enter is typed
	while (1) {

		// Read one character in raw mode.
		char ch;
		read(0, &ch, 1);

		if (ch>=32 && ch != 127) {
			// It is a printable character but not backspace. 

			// Do echo
			write(1,&ch,1);

			// If max number of character reached return.
			if (left_length==MAX_BUFFER_LINE-2) break; 

			// add char to left buffer.
			left_buffer[left_length]=ch;
			left_length++;

			// if chars exist to the right, shift right buffer over.
			if (right_length != 0) 
			{
				for (int i = 0; i < right_length; i++) 
				{
					write(1, &right_buffer[i] ,1);
				}
			}
			//write empty characters past
			for (int i = right_length - 1; i >= 0; i--) 
			{
				char c = 8;
				write(1, &c ,1);
			}
		}
		////////////////// ENTER //////////////////
		if (ch==10) {
			// <Enter> was typed. Return line
			//if characters exist to right, copy them to the left buffer
			if (right_length != 0) 
			{
				for (int i = 0; i < right_length; i++) 
				{
					left_buffer[left_length] = right_buffer[i];
					left_length++;
				}
			}
			right_length = 0;
			
			//clear right buffer at enter
			memset(right_buffer, 0, right_length);

			// Print newline
			write(1,&ch,1);

			break;
		}
		////////////////// CTRL - ? //////////////////
		else if (ch == 31) {
			read_left_print_usage();
			left_buffer[0]=0;
			break;
		}
		////////////////// BACKSPACE / CTRL - H //////////////////
		else if (ch == 127 || ch == 8) {
			if(left_length == 0)
			{
				continue;
			}

			// Go back one character
			ch = 8;
			write(1,&ch,1);

			//write all of the current right buffer to the line
			for(int i = 0; i < right_length; i++)
			{
				write(1, &right_buffer[i], 1);
			}

			// Write a space to erase the last character read
			ch = ' ';
			write(1,&ch,1);
		
			for (int i = 0; i < right_length + 1; i++) 
			{
				//go back for every character in the right buffer
				char c = 8;
				write(1, &c ,1);
			}

			// Remove one character from buffer
			left_length--;
		}
		////////////////// CTRL - D //////////////////
		else if (ch == 4) {
			//if there are no characters to delete, just continue
			if(right_length == 0)
			{
				continue;
			}
			else if(right_length == 0 && left_length == 0)
			{
				continue;
			}
			//if no chars on left, just go right and then backspace
			else if(left_length == 0 && right_length != 0)
			{
				// right
				// Move cursor one to the right
				//otherwise write "\033[1C" to the terminal
				write(1, "\033[1C", 5);
				//append first character in right buffer to left buffer
				left_buffer[left_length] = right_buffer[0];
				//increment left length
				left_length++; 
				//set right buffer by removing first character
				char * temp = right_buffer + 1;
				memmove(right_buffer, temp, strlen(temp) + 1);
				right_length--; 
				// backspace
				// Go back one character
				ch = 8;
				write(1,&ch,1);

				//write all of the current right buffer to the line
				for(int i = 0; i < right_length; i++)
				{
					write(1, &right_buffer[i], 1);
				}

				// Write a space to erase the last character read
				ch = ' ';
				write(1,&ch,1);
			
				for (int i = 0; i < right_length + 1; i++) 
				{
					//go back for every character in the right buffer
					char c = 8;
					write(1, &c ,1);
				}

				// Remove one character from buffer
				left_length--;
			}
			else
			{
				// right
				// Move cursor one to the right
				//otherwise write "\033[1C" to the terminal
				write(1, "\033[1C", 5);
				//append first character in right buffer to left buffer
				left_buffer[left_length] = right_buffer[0];
				//increment left length
				left_length++; 
				//set right buffer by removing first character
				char * temp = right_buffer + 1;
				memmove(right_buffer, temp, strlen(temp) + 1);
				right_length--; 
				// backspace
				// Go back one character
				ch = 8;
				write(1,&ch,1);

				//write all of the current right buffer to the line
				for(int i = 0; i < right_length; i++)
				{
					write(1, &right_buffer[i], 1);
				}

				// Write a space to erase the last character read
				ch = ' ';
				write(1,&ch,1);
			
				for (int i = 0; i < right_length + 1; i++) 
				{
					//go back for every character in the right buffer
					char c = 8;
					write(1, &c ,1);
				}

				// Remove one character from buffer
				left_length--;
				// left
				// Move cursor one to the left
				//otherwise write "\033[1D]" to the terminal
				ch = 8;
				write(1,&ch,1);
				//prepend last character in left buffer to right buffer
				char c = left_buffer[left_length - 1];
				char * temp2 = strdup(right_buffer);
				sprintf(right_buffer, "%c", c);
				sprintf(right_buffer + 1, "%s", temp2);
				free(temp2);
				
				//increment right length
				right_length++;
				//delete a character from the left length
				left_length--;

			}
			
		}
		////////////////// CTRL - A //////////////////
		else if (ch == 1) {
			//Move cursor to the left until left length is 0
			while(left_length != 0)
			{
				// Move cursor one to the left
				//otherwise write "\033[1D]" to the terminal
				ch = 8;
				write(1,&ch,1);
				//prepend last character in left buffer to right buffer
				char c = left_buffer[left_length - 1];
				char * temp = strdup(right_buffer);
				sprintf(right_buffer, "%c", c);
				sprintf(right_buffer + 1, "%s", temp);
				free(temp);
				
				//increment right length
				right_length++;
				//delete a character from the left length
				left_length--;
			}
		}
		////////////////// CTRL - E //////////////////
		else if (ch == 5) {
			//Move cursor to the right until right length is 0
			while(right_length != 0)
			{
				// Move cursor one to the right
				//if no more text on right side, stop
				//otherwise write "\033[1C" to the terminal
				write(1, "\033[1C", 5);
				//append first character in right buffer to left buffer
				left_buffer[left_length] = right_buffer[0];
				//increment left length
				left_length++; 
				//set right buffer by removing first character
				char * temp = right_buffer + 1;
				memmove(right_buffer, temp, strlen(temp) + 1);
				right_length--; 
			}
		}
		////////////////// ARROWS SECTION //////////////////
		else if (ch==27) {
			// Escape sequence. Read two chars more
			//
			// HINT: Use the program "keyboard-example" to
			// see the ascii code for the different chars typed.
			//
			char ch1; 
			char ch2;
			read(0, &ch1, 1);
			read(0, &ch2, 1);
			////////////////// RIGHT //////////////////
			if (ch1==91 && ch2==67) {
				// Move cursor one to the right
				//if no more text on right side, stop
				if(right_length == 0)
				{
					continue;
				}
				//otherwise write "\033[1C" to the terminal
				write(1, "\033[1C", 5);
				//append first character in right buffer to left buffer
				left_buffer[left_length] = right_buffer[0];
				//increment left length
				left_length++; 
				//set right buffer by removing first character
				char * temp = right_buffer + 1;
				memmove(right_buffer, temp, strlen(temp) + 1);
				right_length--; 
			}
			////////////////// LEFT //////////////////
			else if (ch1==91 && ch2==68) {
				// Move cursor one to the left
				//if no more text on left side, stop
				if(left_length == 0)
				{
					continue;
				}
				//otherwise write "\033[1D]" to the terminal
				ch = 8;
				write(1,&ch,1);
				//prepend last character in left buffer to right buffer
				char c = left_buffer[left_length - 1];
				char * temp = strdup(right_buffer);
				sprintf(right_buffer, "%c", c);
				sprintf(right_buffer + 1, "%s", temp);
				free(temp);
				
				//increment right length
				right_length++;
				//delete a character from the left length
				left_length--;
			}
		}

	}

	// Add eol and null char at the end of string
	left_buffer[left_length]=10;
	left_length++;
	left_buffer[left_length]=0;

	//clear right buffer
	memset(right_buffer, 0, right_length);

	reset_input_mode();
	return left_buffer;
}

