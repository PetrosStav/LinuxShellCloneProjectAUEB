#include "mysh.h"

char* getInput(const char* myshName, int* exitCode){
	
	// Declare variables for:
	// input (buffer) : char*
	// buffersize (size) : size_t
	// length of input(len) : size_t
	
	char * buffer;
	size_t len,size;

	// Print the prompt using myshName
	printf("%s> ",myshName);
	// Initialize the buffer as NULL - needed for getline
	buffer = NULL;
	// Get input from stdin and put it in buffer
	// getline returns length of input into len
	len = getline(&buffer,&size,stdin);
	// If EOF is the first character (Ctrl+D)
	if(len == -1){
		// Print that the shell has exitedc
		printf("Ctrl+D\n%s exited.\n",myshName);
		// Change exitCode for error handling
		*exitCode = -1;
		// Free the data from getline
		free(buffer);
		// Return NULL to caller
		return NULL;
	}
	// Change the last character from newline to NULL character
	// so that there is no problem executing the command
	buffer[len-1] = '\0';
	// If length of input exceeds the MAX_INPUT
	if(len > MAX_INPUT){
		// Print the error
		printf("Error: The maximum characters of input (255) is exceeded!\n");
		// Change exitCode for error handling
		*exitCode = 1;
		// Free the data from getline
		free(buffer);
		// Return NULL to caller
		return NULL;
	}else{
		// Everything worked fine, change exitCode accordingly
		*exitCode = 0;
	}
	// return the pointer of the input (buffer) to the caller
	return buffer;

}

char** tokenizeCommand(char* input,int* exitCode){

	// Declare variables for:
	// the NULL ended array which will hold the tokenized command (args) : char**
	// the char pointer for the tokens : char*
	// i for the loop : unsigned
	// count for data allocation : usigned

	char* token;
	char** args;
	unsigned i,count;

	// Initialize exitCode to 0
	*exitCode = 0;

	// Find how many tokens there are
	count = 1;
	token = input;
	while(*token!='\0'){
		// If there is a whitespace character increment the counter
		if(*token==' ' || *token=='\t') count++;
		// Skill all whitespace characters
		while(*token==' ' || *token=='\t') token++;
		// Move to the next character
		token++;
	}
	
	// Allocate memory for the array of tokens
	args = (char **)malloc((count+1)* sizeof(char*));

	// Check that malloc gave us data
	if(args==NULL){
		// Error malloc didn't allocate data
		// Change exitCode to signal caller
		*exitCode = -1;
		return NULL;
	}
	
	// Get first token from input using space delimiter
	token = strtok(input," \t");

	// Initialize the index to 0
	i = 0;
	// While there are more tokens put them into the array
	while(token!=NULL){
		// Put the token into the array at index i
		args[i] = token;
		// Get the next token from the input using space delimiter
		token = strtok(NULL," \t");
		// Increment the index
		i++;
	}

	// Set the last pointer of the array to NULL
	args[i] = NULL;

	// Return the pointer to the array of tokens
	return args;

}

int* findFileRedirections(char** args,int* rin,int* rout,int* exitCode){
	
	// Declare variables for:
	// the mode of the file stream (permissions,etc) (mode) : mode_t
	// the array that will hold the 2 file descriptors (fd) : int*
	// i for the loop : unsigned
	// idx to put NULL into args so that the command stops there : unsigned
	
	mode_t mode;
	int* fd;
	unsigned i,idx;

	// Initialize idx, i, exitCode to 0
	idx = 0;
	i = 0;
	*exitCode = 0;
	// Allocate memory for the array of the file descriptors
	fd = (int*) malloc(2*sizeof(int));

	// Check that malloc gave us data
	if(fd==NULL){
		// Error malloc didn't allocate data
		// Change exitCode to signal caller
		*exitCode = -1;
		return NULL;
	}	
	
	// Set permissions for the files if the output is redirected
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	// While the array has tokens loop
	while(args[i] != NULL){
		
		// If the token is "<" -- input redirection to file
		if(strcmp(args[i],"<")==0){
			// Check that the next token is not NULL or ">",">>" in which cases there would be an error
			if(args[i+1] == NULL || strcmp(args[i+1],">")==0 || strcmp(args[i+1],">>")==0){
				// There is an error with the input
				// Change exitCode for error handling
				*exitCode = -1;
				// Return the array of the file descriptors
				return fd;
			}
			// If the idx has not been changed
			if(idx==0){
				// Set the idx to the current i
				idx = i;
			}
			// Check for previous open file and close it's descriptor
			if(*rin == 1) close(fd[0]);
			// Open an input file stream and put the file descriptor into fd[0] -- open a file with name args[i+1]
			fd[0] = open(args[i+1], O_RDONLY);
			// Modify rin to signal the caller that an input redirection will be applied
			*rin = 1;
			// Increment i
			i++;
		}else if(strcmp(args[i],">")==0){
			// If the token is ">" -- output redirection to file -- create file
			// Check that the next token is not NULL or "<",">>" in which cases there would be an error
			if(args[i+1] == NULL || strcmp(args[i+1],"<")==0 || strcmp(args[i+1],">>")==0){
				// There is an error with the input
				// Change exitCode for error handling
				*exitCode = -1;
				// Return the array of the file descriptors
				return fd;
			}
			// If the idx has not been changed
			if(idx==0){
				// Set the idx to the current i
				idx = i;
			}
			// Check for previous open file and close it's descriptor
			if(*rout == 1) close(fd[1]);
			// Open an output file stream and put the file descriptor into fd[1] -- create a file with name args[i+1]
			fd[1] = creat(args[i+1], mode);
			// Modify rout to signal the caller that an input redirection will be applied
			*rout = 1;
			// Increment i
			i++;
		}else if(strcmp(args[i],">>")==0){
			// If the token is ">>" -- output redirection to file -- append/create file
			// Check that the next token is not NULL or ">","<" in which cases there would be an error
			if(args[i+1] == NULL || strcmp(args[i+1],">")==0 || strcmp(args[i+1],"<")==0){
				// There is an error with the input
				// Change exitCode for error handling
				*exitCode = -1;
				// Return the array of the file descriptors
				return fd;
			}
			// If the idx has not been changed
			if(idx==0){
				// Set the idx to the current i
				idx = i;
			}
			// Check for previous open file and close it's descriptor
			if(*rout == 1) close(fd[1]);
			// Open an output file stream and put the file descriptor into fd[1] -- append/create a file with name args[i+1]
			fd[1] = open(args[i+1], O_WRONLY | O_APPEND | O_CREAT, mode);
			// Modify rout to signal the caller that an input redirection will be applied
			*rout = 1;
			// Increment i
			i++;
		}
		// Increment i
		i++;
		
	}
	// If idx has been changed from 0
	if(idx != 0){
		// Set the array at index idx to NULL so that when the command is executed with execvp
		// it will stop there and not take into account the redirections as command, in which
		// case where would be an error
		args[idx] = NULL;
	}
	// Return the array of the file descriptors
	return fd;
}

int checkEmptyCommand(char* command){
	unsigned i;
	// Initialize i to 0
	i=0;
	// Loop the char array until the null character
	while(command[i]!='\0'){
		// Check if character is whitespace
		if(!isspace(command[i])) return 0;
		// Goto next character
		i++;
	}
	return 1;
}

unsigned countChars(char* input, char c){
	unsigned count;
	// Initialize count to 0
	count = 0;
	// Until you find the null character which ends the char array
	while(*input!='\0'){
		// If there is a character c increment the counter
		if(*input==c) count++;
		// Move to the next character
		input++;
	}
	return count;
}
