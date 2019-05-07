#include "mysh.h"

int main(int argc, char **argv) {

	// Main loop of shell

	while(1){

		// Declare every variable that will be used
		// pid : the process id used to determine the child/parent process and which process to wait
		// waitPid : the process id return from the waitpid command of the parent
		// input : the character array (string) of the input from the user
		// leftcommand : the character array (string) of the left command of the pipe
		// rightcommand : the character array (string) of the right command of the pipe
		// argsl : the array of the leftcommand tokens used by execvp
		// argsr : the array of the rightcommand tokens used by execvp
		// fd : the array that will hold the 2 file descriptors for file redirections (fd[0]-input , fd[1]-output)
		// pipfds : the array that will hold the 2 file descriptors for piping (pipfds[0]-input , pipfds[1]-output)
		// returnValue : used for error handling of the pipe() command
		// exitCode : used for error handling between the functions of functions.h
		// status : used by waitpid command to get the status of the process waiting
		// rinl : used by the child process to determine if the current leftcommand has an input file redirection
		// routl : used by the child process to determine if the current leftcommand has an output file redirection
		// rinr : used by the child process to determine if the current rightcommand has an input file redirection
		// routr : used by the child process to determine if the current rightcommand has an output file redirection
		// len : used to store the length of the input
		// i : used as index

		pid_t pid,waitPid;
		char* input;
		char * leftcommand;
		char * rightcommand;
		char** argsl;
		char ** argsr;
		int* fd;
		int pipfds[2];
		int rinl=0,rinr=0,routl=0,routr=0;
		int returnValue, exitCode,status;
		unsigned i;

		// Print the prompt and get the user's input
		input = getInput("mysh4",&exitCode);
		
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

		// Check if double pipe exists in input - error
		if(strstr(input,"||")!=NULL){
			// Free the data allocated from getInput
			free(input);
			// Print error
			fprintf(stderr,"Error: Invalid input, symbol '||'\n");
			// Goto start of main loop
			continue;
		}

		// Check if input ends with "|" or "|" followed by whitespace chars -- error
		i = strlen(input) - 1;
		// Ignore whitespace chars
		while(isspace(input[i])) i--;
		if(input[i]=='|'){
			// Free the data allocated from getInput
			free(input);
			// Print error		
			fprintf(stderr,"Error: Input ending with '|'\n");
			// Goto start of main loop
			continue;
		}

		// Check if input starts with "|" -- error
		if(input[0]=='|'){
			// Free the data allocated from getInput
			free(input);
			// Print error		
			fprintf(stderr,"Error: Input starting with '|'\n");
			// Goto start of main loop
			continue;
		}
		
		// Split the input into two parts/tokens separated by the delimiter "|"
		// Put the first token in leftcommand
		leftcommand = strtok(input,"|");

		// Check if leftcommand is empty - error
		if(checkEmptyCommand(leftcommand)){
			// Free the data allocated from getInput
			free(input);
			// Print error		
			fprintf(stderr,"Error: Invalid input before '|'\n");
			// Goto start of main loop
			continue;
		}		

		// Put the second token in rightcommand
		rightcommand = strtok(NULL,"|");

		// Check that there are no other piped commands -- error
		if(strtok(NULL,"|")!=NULL){
			// Free the data allocated from getInput
			free(input);
			// Print error		
			fprintf(stderr,"Error: Invalid input, more than 2 piped commands\n");
			// Goto start of main loop
			continue;
		}

		// Pipe the process, put the pipe file descriptors in pipfds array and return the value
		// of the function to returnValue for error handling
		returnValue = pipe(pipfds);

		if (returnValue == -1) {
			// Error with pipe command
			// Print error
			fprintf(stderr,"ERROR: Pipe command failed.\n");
			// Free the data allocated from getInput
			free(input);
			// End loop - Terminate the shell
			exit(1);
		}

		// Fill the array with the current leftcommand's tokens
		argsl = tokenizeCommand(leftcommand,&exitCode);

		// Check exitCode to handle errors
		if(exitCode==-1){
			// Print error
			fprintf(stderr,"Error: There was an error in the tokenization!\n");
			// Free the data allocated from getInput
			free(input);
			// Goto start of loop (inquire next input)
			continue;
		}

		// Find if there are I/O file redirections in current leftcommand and 
		// get the file descriptors for the file streams opened
		fd = findFileRedirections(argsl,&rinl,&routl,&exitCode);

		// Handle possible errors using exitCode
		// Exitcode: 0 no problems
		//			-1 error with input (invalid char after <,>,>>)
		if(exitCode==-1){
			// Print error
			fprintf(stderr,"Error: There was an error in the input!\n");
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated from tokenizeCommand
			free(argsl);
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
			free(argsl);
			// Free the data allocated from findFileRedirections
			free(fd);
			// End loop - Terminate the shell
			exit(1);
		}

		if(pid == 0){
			// Child process

			// If an input file redirection has been found in the leftcommand
			if(rinl){
				// Duplicate the file descriptor in fd[0] to stdin
				dup2(fd[0],0);
				// Close the open file descriptor in fd[0]
				close(fd[0]);
				// Set rinl to 0 for next command
				rinl = 0;
			}
			// If an output file redirection has been found in the leftcommand
			if(routl){
				// Duplicate the file descriptor in fd[1] to stdout
				dup2(fd[1],1);
				// Close the open file descriptor in fd[1]
				close(fd[1]);
				// Set routl to 0 for next command
				routl = 0;
			}else if(rightcommand!=NULL){
				// If there is a piped right command send the output to it
				// Duplicate the file descriptor in pipfds[1] to stdout
            	dup2( pipfds[1], 1 );
			}
			// Execute the command
			execvp(*argsl,argsl);
			
			// There was an error
			// Print the error
			fprintf(stderr,"Execvp error : '%s' : ",*argsl);
            perror("");
			// Terminate the shell
			exit(1);


		}else{
			// Parent process

			// Wait until the child process executes the command
			waitPid = waitpid(pid,&status,0);
			// Close the open file descriptor in pipfds[1]
			close(pipfds[1]);
			// Free the data allocated from tokenizeCommand
			free(argsl);
			// Free the data allocated from findFileRedirections
			free(fd);
			// There was an error with waitpid
			if (waitPid == -1) {
				// Print the error
				perror("ERROR: Waitpid failed.\n");
				// Terminate the shell
				exit(1);
			}

		}
		// If there isn't a right piped command
		if(rightcommand==NULL){
			// Close the open file descriptor in pipfds[0]
			close(pipfds[0]);
			// Free the data allocated from getInput 
			free(input);
			// Goto start of loop - get next command
			continue;
		}

		// Fill the array with the current rightcommand's tokens
		argsr = tokenizeCommand(rightcommand,&exitCode);

		// Check exitCode to handle errors
		if(exitCode==-1){
			// Print error
			fprintf(stderr,"Error: There was an error in the tokenization!\n");
			// Free the data allocated from getInput
			free(input);
			// Goto start of loop (inquire next input)
			continue;
		}

		// Find if there are I/O file redirections in current rightcommand and 
		// get the file descriptors for the file streams opened
		fd = findFileRedirections(argsr,&rinr,&routr,&exitCode);

		
		// Handle possible errors using exitCode
		// Exitcode: 0 no problems
		//			-1 error with input (invalid char after <,>,>>)
		if(exitCode==-1){
			// Print error
			fprintf(stderr,"Error: There was an error in the input!\n");
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated from tokenizeCommand
			free(argsr);
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
			free(argsr);
			// Free the data allocated from findFileRedirections
			free(fd);
			// End loop - Terminate the shell
			exit(1);
		}

		if(pid == 0){
			// Child process

			// If an input file redirection has been found in the rightcommand
			if(rinr){
				// Duplicate the file descriptor in fd[0] to stdin
				dup2(fd[0],0);
				// Close the open file descriptor in fd[0]
				close(fd[0]);
				// Set rinr to 0 for next command
				rinr = 0;
			}else{
				// If there is no file input redirection then the piped rightcommand takes input from the leftcommand's output
				// Duplicate the file descriptor in pipfds[0] to stdin
            	dup2( pipfds[0], 0 );
			}
			// If an output file redirection has been found in the rightcommand
			if(routr){
				// Duplicate the file descriptor in fd[1] to stdout
				dup2(fd[1],1);
				// Close the open file descriptor in fd[1]
				close(fd[1]);
				// Set routr to 0 for next command
				routr = 0;
			}

			// Execute the command
			execvp(*argsr,argsr);
			// There was an error
			// Print the error
			fprintf(stderr,"Execvp error : '%s' : ",*argsr);
            perror("");
			// Terminate the shell
			exit(1);


		}else{
			// Parent process

			// Wait until the child process executes the command
			waitPid = waitpid(pid,&status,0);
			// Close the open file descriptor in pipfds[0]
			close(pipfds[0]);
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated from tokenizeCommand
			free(argsr);
			// Free the data allocated from findFileRedirections
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