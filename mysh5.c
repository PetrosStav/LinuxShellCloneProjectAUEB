#include "mysh.h"

int main(int argc, char **argv) {

	// Main loop of shell

	while(1){

		// Declare every variable that will be used
		// pid : the process id used to determine the child/parent process and which process to wait
		// waitPid : the process id return from the waitpid command of the parent
		// commands : the array that contains all piped commands
		// tempcommands : used to free the data if realloc in commands array fails
		// args : the array of the command tokens used by execvp
		// inputarray : an array that stores every input given from the user so that each is freed from the memory
		// input : the character array (string) of the input from the user
		// nextcomm : used to check for more piped commands and to fill the commands array
		// nextpcomm : used to check for more prompted piped commands and to fill the commands array with the extra commands
		// fd : the array that will hold the 2 file descriptors for file redirections (fd[0]-input , fd[1]-output)
		// pipfds : the array that will hold the 2 file descriptors for piping (pipfds[0]-input , pipfds[1]-output)
		// fd_input : holds the input file descriptor of the previous command in the loop
		// rin : used by the child process to determine if the current command has an input file redirection
		// rout : used by the child process to determine if the current command has an output file redirection
		// returnValue : used for error handling of the pipe() command
		// exitCode : used for error handling between the functions of mysh.h
		// status : used by waitpid command to get the status of the process waiting
		// moreComms : used for handling more piped commands
		// emptyComm : used for handling empty piped commands
		// i,j : used for the loop as index
		// ncount : used as a coutner for the new value at each extra piped command
		// count : used as a counter for the number of commands (char*) to allocate
		// ia_idx : used as index for inputarray
		// len : used for the length of the input

		pid_t pid,waitPid;
		
		char** commands;
		char** tempcommands;
		char** args;
		char** inputarray;
		char* input;
		char* nextcomm;
		char* nextpcomm;
		int* fd;
		int pipfds[2];
		int fd_input = 0;
		int rin=0,rout=0;
		int returnValue, exitCode,status,moreComms,emptyComm;
		unsigned i,j,ncount,count,ia_idx;
		size_t len;

		// Initialize ia_idx
		ia_idx = 0;
		
		// Allocate memory for 1 input for inputarray
		inputarray = (char**)malloc(1*sizeof(char*));
		
		// Print the prompt and get the user's input
		input = getInput("mysh5",&exitCode);
		
		// Put input in inputarray
		inputarray[ia_idx++] = input;

		if(exitCode==1) {
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated to inputarray
			free(inputarray);
			// Goto start of the loop
			continue;
		}
		if(exitCode==-1) {
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated to inputarray
			free(inputarray);
			// End loop - Terminate the shell
			break;
		}

		// If the command from user's input is the command "exit" terminate the shell
		if(strncmp(input,"exit",4)==0){
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated to inputarray
			free(inputarray);
			// End loop - Terminate the shell
			break;
		}

		// Check if input starts with "|" -- error
		if(input[0]=='|'){
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated to inputarray
			free(inputarray);
			// Print error
			fprintf(stderr,"Error: Input starting with '|'\n");
			// Goto start of main loop
			continue;
		}

		// Check if double pipe exists in input - error
		if(strstr(input,"||")!=NULL){
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated to inputarray
			free(inputarray);
			// Print error
			fprintf(stderr,"Error: Invalid input, symbol '||'\n");
			// Goto start of main loop
			continue;
		}

		// Check if input is empty or whitespace characters
		if(checkEmptyCommand(input)){
            // Free the data allocated from getInput
			free(input);
			// Free the data allocated to inputarray
			free(inputarray);
			// Goto start of main loop
			continue;
		}

		// Get more piped commands (check input ending with "|")
		nextcomm = input;
		// Find the length of the input
		len = strlen(nextcomm);

		// Check if the line ends with "|" ignoring whitespace characters and change variable moreComms accordingly
		i = len-1;
		// Ignore whitespace chars
		while(isspace(nextcomm[i])) i--;
		// If there is a pipe symbol change the variable to prompt the user for more commands
		if(nextcomm[i]=='|') moreComms = 1;
		else moreComms = 0;

		// Split input to piped commands using delimiter "|"
		
		// Find how many commands there are
		count = 1 + countChars(nextcomm,'|');

		// Allocate memory for commands
		commands = (char**)malloc((count+1)*sizeof(char*));
		if(commands==NULL){
			// Can't allocate memory
			// Print error
			fprintf(stderr,"Error: Not enough memory\n");
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated to inputarray
			free(inputarray);
			continue;
		}

        // Set i to 0
		i=0;

		// Get first command and put it into the commands array
		commands[i] = strtok(input,"|");

		// Check if first command is empty - error
		if(checkEmptyCommand(commands[i])){
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated to inputarray
			free(inputarray);
			// Free the data from the commands array
			free(commands);
			// Print error
			fprintf(stderr,"Error: Invalid input before '|'\n");
			// Goto start of main loop
			continue;
		}

		// We have not found any empty commands yet
		emptyComm = 0;

		// Increment i
		i++;

		// While there are more piped commands
		while((nextcomm = strtok(NULL,"|")) != NULL){
			// Check that the command isn't whitespace chars
			if(checkEmptyCommand(nextcomm)){
				// If we found the '|' in the end and it is the last command
				if(moreComms && i==count-1){
					// Break the loop
					break;
				}
				// It has only whitespace chars
				emptyComm = 1;
				// Break the loop
				break;
			}
			// Put command into commands array
            		commands[i++] = nextcomm;

		}
		// If we have found an empty command
		if(emptyComm){
			// Free the data allocated from getInput
			free(input);
			// Free the data allocated to inputarray
			free(inputarray);
			// Free the data from the commands array
			free(commands);
			// Print error
			fprintf(stderr,"Error: Input has empty piped command.\n");
			// Goto main loop - get next input
			continue;
		}

		// While the user ends the input with "|" and there is no empty command
		emptyComm = 0;
		while(moreComms && !emptyComm){
			// Get the next piped command
			nextcomm = getInput("",&exitCode);
			// We have one more input - reallocate size of inputarray
			inputarray = (char**)realloc(inputarray,(1+ia_idx)*sizeof(char*));
			// Put next input in inputarray
			inputarray[ia_idx++] = nextcomm;
			
			if(exitCode==1) {
				// Free data from all input
				for(j=0;j<ia_idx;j++){
					free(inputarray[j]);
				}
				free(inputarray);
				
				// Free the data from the commands array
				free(commands);
				// Break the loop
				break;
			}
			if(exitCode==-1) {
				// Free data from all input
				for(j=0;j<ia_idx;j++){
					free(inputarray[j]);
				}
				free(inputarray);
				// Free the data from the commands array
				free(commands);
				// Break the loop
				break;
			}
			// Find the length of the next prompted command
			len = strlen(nextcomm);
			// Check if input is empty
			if(checkEmptyCommand(nextcomm)){
				// Free the data allocated from getInput
				free(nextcomm);
				// goto start of while to prompt again for input
				continue;
			}

			// Check if input starts with "|" -- error
			if(nextcomm[0]=='|'){
				// Free data from all input
				for(j=0;j<ia_idx;j++){
					free(inputarray[j]);
				}
				free(inputarray);
				// Free the data from the commands array
				free(commands);
				// Print error
				fprintf(stderr,"Error: Input starting with '|'\n");
				// Goto start of main loop
				exitCode = -1;
				break;
			}

			// Check if double pipe exists in input - error
			if(strstr(nextcomm,"||")!=NULL){
				// Free data from all input
				for(j=0;j<ia_idx;j++){
					free(inputarray[j]);
				}
				free(inputarray);
				// Free the data from the commands array
				free(commands);
				// Print error
				fprintf(stderr,"Error: Invalid input, symbol '||'\n");
				// Goto start of main loop
				exitCode = -1;
				break;
			}
			// Check if the line ends with "|" ignoring whitespace characters and change variable moreComms accordingly
			j = len-1;
			// Ignore whitespace chars
			while(isspace(nextcomm[j])) j--;
			// If there is a pipe symbol change the variable to prompt the user for more commands
			if(nextcomm[j]=='|') moreComms = 1;
			else moreComms = 0;
			
			// Check how many commands there are in the extra input
			j = 1 + countChars(nextcomm,'|');
			count += j-1;
			ncount = j;
			
			// Set ncount to the amount that increased in count (new count - old count)
			ncount = j;

			// Reallocate the data in commands
			tempcommands = commands;
			commands = (char**)realloc(commands,(count+1)*sizeof(char*));
			if(commands==NULL){
				// Can't allocate memory
				// Print error
				printf("Error: Not enough memory\n");
				// Free data from all input
				for(j=0;j<ia_idx;j++){
					free(inputarray[j]);
				}
				free(inputarray);
				// Free the data from the commands array (here we have a copy of that pointer)
				free(tempcommands);
				// Goto start of main loop
				exitCode = -1;
				break;
			}

			// Enter the command in the array
			nextpcomm = strtok(nextcomm,"|");

			// Check if first command is empty - error
			if(checkEmptyCommand(nextpcomm)){
				// Free data from all input
				for(j=0;j<ia_idx;j++){
					free(inputarray[j]);
				}
				free(inputarray);
				// Free the data from the commands array
				free(commands);
				// Print error
				fprintf(stderr,"Error: Invalid input before '|'\n");
				// Goto start of main loop
				exitCode = -1;
				break;
			}
			// Set j to 0
			j = 1;
			// While there are piped commands / tokens in input
			while(nextpcomm != NULL){
				// Check if they are empty
				if(checkEmptyCommand(nextpcomm)){
					// If we found the '|' in the end and it is the last command
					if(moreComms && j==ncount){
						// Break the loop
						break;
					}
					// Empty command - signal error
					emptyComm = 1;
					// Break the loop
					break;
				}
				// Increment j
				j++;
				// Add command to commands array
				commands[i++] = nextpcomm;
				// Get next token from command
				nextpcomm = strtok(NULL,"|");
			}


		}
		// If we have found an empty command from the extra commands
		if(emptyComm){
			// Free data from all input
			for(j=0;j<ia_idx;j++){
				free(inputarray[j]);
			}
			free(inputarray);
			// Free the data from the commands array
			free(commands);
			// Print error
			fprintf(stderr,"Error: Input has empty piped command.\n");
			// Goto main loop - get next input
			continue;
		}
		// If an error occured then goto start of main loop
		if(exitCode==1||exitCode==-1) continue;

		// Put NULL into the last position of the commands array, so that the loop
		// afterwards knows where to stop
		commands[i] = NULL;

		//Set i to 0
		i = 0;
		// While there are piped commands in the commands array
		while(commands[i] != NULL){

			// Initialize rin and rout to 0 for every command
			rin=0;
			rout=0;

			// Pipe the process, put the pipe file descriptors in pipfds array and return the value
			// of the function to returnValue for error handling
            returnValue = pipe(pipfds);
			
			// Check for error in pipe
			if(returnValue==-1){
				// Print error
				perror("Error: There was an error with pipe()");
				// Free the data from the commands array
				free(commands);
				// Goto start of loop (inquire next input)
				break;
			}

			// Fill the array with the current command's tokens
            args = tokenizeCommand(commands[i],&exitCode);

		// Check exitCode to handle errors
		if(exitCode==-1){
			// Print error
			fprintf(stderr,"Error: There was an error in the tokenization!\n");
			// Free the data from the commands array
			free(commands);
			// Goto start of loop (inquire next input)
			break;
		}

			// Find if there are I/O file redirections in current command and
			// get the file descriptors for the file streams opened
			fd = findFileRedirections(args,&rin,&rout,&exitCode);

			// Handle possible errors using exitCode
			// Exitcode: 0 no problems
			//			-1 error with input (invalid char after <,>,>>)
			if(exitCode==-1){
				// Print error
				printf("Error: There was an error in the input!\n");
				// Free the data allocated from tokenizeCommand
				free(args);
				// Free the data from the commands array
				free(commands);
				// Free the data allocated from findFileRedirections
				if(fd!=NULL) free(fd);
				// Goto start of main loop (inquire next input)
				break;
			}

			// Fork the process

            pid = fork();

			if(pid < 0){
				// There was an error with fork

				// Print the error
				perror("Error with fork()\n");
				// Free data from all input
				for(j=0;j<ia_idx;j++){
					free(inputarray[j]);
				}
				free(inputarray);
				// Free the data allocated from tokenizeCommand
				free(args);
				// Free the data allocated from findFileRedirections
				free(fd);
				// Free the data from the commands array
				free(commands);
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
				}else{
					// If there is no file input redirection then the piped command takes input from the previous piped command's output
					// Duplicate the file descriptor in fd_input to stdin
                   	dup2(fd_input,0);
				}
				// If an output file redirection has been found in the command
				if(rout){
					// Duplicate the file descriptor in fd[1] to stdout
					dup2(fd[1],1);
					// Close the open file descriptor in fd[1]
					close(fd[1]);
					// Set rout to 0 for next command
					rout = 0;
				}else if(commands[i+1]!=NULL){
					// If there is a piped right command send the output to it
					// Duplicate the file descriptor in pipfds[1] to stdout
                    dup2(pipfds[1],1);
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
				// Free the data allocated from tokenizeCommand
				free(args);
				// Free the data allocated from findFileRedirections
				free(fd);
				// If the previous input file descriptor is not stdin
				if(fd_input!=0){
					// Close the open file descriptor in fd_input
                  	close(fd_input);
				}
				// Set fd_input to open file descriptor pipfds[0] for the next piped command's input
				fd_input = pipfds[0];
				// Close the open file descriptor in pipfds[1]
				close(pipfds[1]);
				//
               	i++;
				// If there was an error with waitpid
				if (waitPid == -1) {
					// Print the error
					perror("ERROR: Waitpid failed.\n");
					// Free data from all input
					for(j=0;j<ia_idx;j++){
						free(inputarray[j]);
					}
					free(inputarray);
					// Free the data from the commands array
					free(commands);
					// Terminate the shell
					exit(1);
				}

			}


		}
		// Close the last open file descriptor
		close(pipfds[0]);
		// Free data from all input
		for(j=0;j<ia_idx;j++){
			free(inputarray[j]);
		}
		free(inputarray);
		// Free the data from the commands array
		free(commands);


	}

	return 0;
}
