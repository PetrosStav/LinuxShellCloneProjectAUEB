// Include everything needed for the C functions used by the shell

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#define MAX_INPUT 255

// getInput
//
// Prints the prompt (first parameter) and gets the user's input
// Alters exitCode (second parameter) to handle errors
// Returns pointer to char array of input
// Allocates data for the input
// Data must be freed to avoid memory leak

char* getInput(const char* myshName, int* exitCode);

// tokenizeCommand
//
// Gets an input (command)
// Tokenizes it using space delimiter
// Makes a NULL ended array with the tokens
// Returns the NULL ended array
// Allocates data for the array
// Data must be freed to avoid memory leak

char** tokenizeCommand(char* input,int* exitCode); 

// findFileRedirections
//
// Gets the NULL ended array from the tokenizeCommand and searches
// for I/O redirections to files (<,>,>>), opens the file streams
// Alters rin and rout according to the redirections found
// Alters exitCode for error handling
// Return pointer to array containing the file descriptors:
// fd[0] - input
// fd[1] - output
// Allocates data for the array
// Data must be freed to avoid memory leak

int* findFileRedirections(char** args,int* rin,int* rout,int* exitCode);

// checkEmptyCommand
//
// Checks if a command given is only whitespace characters
// Return int as:
// 0 - not empty command
// 1 - empty command

int checkEmptyCommand(char* command);

// countChars
//
// Counts how many occurences of char c are in the given input
// Returns unsigned count of occurences

unsigned countChars(char* input,char c);
