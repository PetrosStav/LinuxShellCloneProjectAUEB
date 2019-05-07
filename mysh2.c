#include "mysh.h"

int main(int argc, char **argv) {

	// Main loop of shell

	while(1){
		
		// Declare every variable that will be used
		// pid : the process id used to determine the child/parent process and which process to wait
		// waitPid : the process id return from the waitpid command of the parent
		// input : the character array (string) of the input from the user
		// args : the array of the command tokens used by execvp
		// exitCode : used for error handling between the functions of functions.h
		// status : used by waitpid command to get the status of the process waiting

		pid_t pid,waitPid;
		char* input;
		char** args;
		int exitCode,status;

		// Print the prompt and get the user's input
		input = getInput("mysh2",&exitCode);
		
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
			// End loop - Terminate the shell
			exit(1);
		}

		if(pid == 0){
			// Child process
			
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