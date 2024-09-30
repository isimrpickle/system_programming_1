#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "dynamicqueue.h"

															//Ρόλος client




int main (int argc,char *argv[]){

char * fifo1="Commander_to_Server",*text_file="jobExecutorServer.txt", *fifo2="Server_to_Commander";
int child,nwrite; //for error handling...
int isfifo_open,isfifo_open2;
int total_length=0; //for dynamically memory allocation of the buffer
struct stat st; //using it to check if there is already a fifo or a txt file.
char txtstring[25]; //the only fixed size array in jobCommander, it's more than enough to read from the txt file. It will be used to get the Server pid
pid_t childpid;
char* token;
FILE *txt;
	for (int i = 1; i < argc; i++) {
        total_length += strlen(argv[i]); 
    }

	//creating fifo if it doesn't exist already
	if (stat(fifo1, &st) != 0)
        mkfifo(fifo1, 0666);


	if(stat(text_file,&st)!=0){
		child=fork();
		if(child==0)
			execl("./jobExecutorServer","jobExecutorServer",NULL);

		}
	
	while(stat(text_file,&st)!=0){} //when the text file get created we open it
		txt=fopen(text_file,"r");  
		while(fgets(txtstring,25,txt) == NULL){clearerr(txt);} //no sleep in my program🤓 when the Server writes into the txt file we tokenize the string and take the pid.

		fgets(txtstring,25,txt);
		token=strtok(txtstring,":");
		token=strtok(NULL,":");
		childpid=atoi(token);
		fclose(txt);
	
	


	//copying the aguments into the msgbuf arrray

	//the msgbuf will contain all the command line arguments but between every argument there would be a space. We're gonna use it later for tokenizing
	char *msgbuf=malloc((total_length+1)*sizeof(char));
	int baseline=0; //will be used as an index
	for(int i=1;i<argc;i++){ //we ignore the ./jobCommander and the keyword after(issueJob,poll etc.) from terminal
		if(baseline>0){
			msgbuf[baseline]=' '; //i am doing this because arg[v] have just words so the string would be without space(also, i will tokenize the string in the Server, and I need the space as a deliminator).
			baseline++;
		}
		strcpy(msgbuf+baseline,argv[i]); //baseline is being used as index of where the new argument should start being written.
		baseline+=strlen(argv[i]);
	}

	kill(childpid,SIGUSR1); //signaling the server for_reading=1
     if ( (isfifo_open=open(fifo1, O_WRONLY)) < 0){ //opening fifo
		perror("file open error"); exit(3);
	}
	
	
		//first, we write the size of the message that we're gonna pass and then the actual message.
	if (argc<2) { printf("you forgot to write a job... \n"); exit(1); }

		if ((nwrite=write(isfifo_open, &baseline, sizeof(baseline))) == -1)

			{ perror("Error in Writing"); exit(2); }
			
		if ((nwrite=write(isfifo_open, msgbuf,baseline)) == -1) //on the other side we're sending the keyword and the word right after. So if we did "./jobCommander issueJob ls"
																//we're gonna send "issueJob ls"
			{ perror("Error in Writing"); exit(2); }

			free(msgbuf); //freeing 
		
		


		
		if(strcmp(argv[1],"setConcurrency")!=0){ //Concurrency does not return anything, so we do not open the 2nd fifo if the keyword is "setConcurrency"
		//again, we follow the same logic, but now in reverse, reading first the size of the message from the server,and then reading the actual message. 

			isfifo_open2 = open(fifo2, O_RDONLY); //then open it
			if (isfifo_open2 < 0) {
				perror("fifo2 open problem\n");
				exit(2);
			}
			if (read(isfifo_open2, &baseline, sizeof(int)) < 0) {
				perror("problem in reading message size");
				close(isfifo_open2);
				exit(3);
			}
			char*Serverbuffer;
			Serverbuffer=malloc((baseline+1)*sizeof(char));
			if (!Serverbuffer) {
				perror("memory allocation failed");
				close(isfifo_open2);
				exit(4);
			}

			if (read(isfifo_open2, Serverbuffer, baseline) < 0) {
				perror("problem in reading message size");
				close(isfifo_open2);
				exit(3);
			}
			Serverbuffer[baseline]='\0'; //add null character in msgbuff

			printf("\n  the Commander says:\n %s \n",Serverbuffer);
			
			fflush(stdout);
			free(Serverbuffer);
			close(isfifo_open2);
		}
		
}

