#include <stdio.h>
#include <string.h>
#include "../lib/commandlinereader.h"
#include "../lib/types.h"

#define BUFFERSIZE 200

int main(int argc, char const *argv[])
{
	long maxChildren = -1;
	char** argVector[2];
	char* buffer[BUFFERSIZE];


	if (argc == 2){
		if(sscanf(argv[1],"%ld", &maxChildren)!=1){
			printf("Invalid Args\n");
			exit (1);
		}
	}
	

	while(TRUE){
		if(readLineArguments(argVector, 2, buffer, BUFFERSIZE) == -1)
			goto DISPLAY_ERROR;

		if (strcmp(argVector[0], "run") == 0){
			
		}
		else if (strcmp(argVector[0], "exit") == 0){
			...
		}
		else
			goto DISPLAY_ERROR;

		DISPLAY_ERROR:
		puts("Invalid Comands");
	}

	
	return 0;
}