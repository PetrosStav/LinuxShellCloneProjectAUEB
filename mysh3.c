#include "mysh.h"

int main(int argc, char **argv) {

	// Main loop of shell

	while(1){

		// Declare every variable that will be used
		// pid : the process id used to determine the child/parent process and which process to wait
		// waitPid : the process id return from the waitpid command of the parent
		// input : the character array (string) of the input from the user
		// args : the array of the command tokens used by execvp
		// fd : the array that will hold the 2 file descriptors for file redirections (fd[0]-input , fd[1]-output)
		// exitCode : used for error handling between the functions of functions.h
		// status : used by waitpid command to get the status of the process waiting
		// rin : used by the child process to determine if the current command has an input file redirection
		// rout : used by the child process to determine if the current command has an output file redirection
	
		pid_t pid,waitPid;
		char* input;
		char** args;
		int* fd;
		int exitCode,status;
		int rin=0,rout=0;

		// Print the prompt and get the user's input
		input = getInput("mysh3",&exitCode);
		
		// Handle possible errors using exitCode
		// Exitcode: -1 EOF - Ctrl + D
		// 			 1 input exceeds maximum characters
		// 			 0 no problems
		
		if(exitCode==1) {
			// Free the data allocated from getInput
			free(input);
			// Goto start of the loop
			continue;
		}
		if(exitCode==-1) {
			// Free the data allocated from getInput
			free(input);
			// End loop - Terminate the shell
			break;
		}


		// If the command from user's input is the command "exit" terminate the shell
		if(strncmp(input,"exit",4)==0){
			// Free the data allocated from getInput
			free(input);
			// End loop - Terminate the shell
			break;
		}
		
		// Check if input is empty or whitespace characters
		if(checkEmptyCommand(input)){
            // Free the data allocated from getInput
			free(input);
			// Goto start of main loop
			continue;
		}

		// Fill the array with the current command's tokens
		args = tokenizeCommand(input,&exitCode);

		// Check exitCode to handle errors
		if(exitCode==-1){
			// Print error
			fprintf(stderr,"Error: There was an error in the tokenization!\n");
			// Free the data allocated from getInput
			free(input);
			// Goto start of loop (inquire next input)
			continue;
		}

		// Find if there are I/O file redirections in current command and 
		// get the file descriptors for the file streams opened
		fd = findFileRedirections(args,&rin,&rout,&exitCode);

		// Handle possible errors using exitCode
		// Exitcode: 0 no problems
		//			-1 error with input (invalid char after <,>,>>)
		if(exitCode==-1){
			// Print error
			fprintf(stderr,"Error: There was an error in the input!\n");
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated from tokenizeCommand
			free(args);
			// Free the data allocated from findFileRedirections
			if(fd!=NULL) free(fd);
			// Goto start of loop (inquire next input)
			continue;
		}
		
		// Fork the process
		
		pid = fork();

		if(pid < 0){
			// There was an error with fork
			
			// Print the error
			perror("Error with fork()\n");
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated from tokenizeCommand
			free(args);
			// Free the data allocated from findFileRedirections
			free(fd);
			// End loop - Terminate the shell
			exit(1);
		}

		if(pid == 0){
			// Child process

			// If an input file redirection has been found in the command
			if(rin){
				// Duplicate the file descriptor in fd[0] to stdin
				dup2(fd[0],0);
				// Close the open file descriptor in fd[0]
				close(fd[0]);
				// Set rin to 0 for next command
				rin = 0;
			}
			// If an output file redirection has been found in the command
			if(rout){
				// Duplicate the file descriptor in fd[1] to stdout
				dup2(fd[1],1);
				// Close the open file descriptor in fd[1]
				close(fd[1]);
				// Set rout to 0 for next command
				rout = 0;
			}

			// Execute the command
			execvp(*args,args);
			
			// There was an error
			// Print the error
			fprintf(stderr,"Execvp error : '%s' : ",*args);
            perror("");
			// Terminate the shell
			exit(1);


		}else{
			// Parent process

			// Wait until the child process executes the command
			waitPid = waitpid(pid,&status,0);
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated from tokenizeCommand
			free(args);
			// There was an error with waitpid
			free(fd);
			// There was an error with waitpid
			if (waitPid == -1) {
				// Print the error
				perror("ERROR: Waitpid failed.\n");
				// Terminate the shell
				exit(1);
			}

		}

	}

	return 0;
}