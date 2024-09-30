#define _XOPEN_SOURCE 700//necessary for sigaction, if your'e interested check https://stackoverflow.com/questions/6491019/struct-sigaction-incomplete-error
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "dynamicqueue.h"

											//SERVER
int pid;
int running_processes=0;
int for_reading=0;
struct Queue* active_processes;

void signal_handler(int sig){
	kill(getpid(),SIGCONT); // for unpausing
	
	for_reading = 1;
}

void signal_sigchld(int sig){
	while ((pid = waitpid(-1, NULL, WNOHANG))) { //inspired by https://stackoverflow.com/questions/1510922/waiting-for-all-child-processes-before-parent-resumes-execution-unix
		if(pid>0)
		{ //only when a child terminates 
			running_processes--;
			dequeue(active_processes);
			updating_queueposition(active_processes);
		}
    	if (errno == ECHILD) {
       		break;
   }
}
	
}




int main(int argc, char *argv[]){
	char *fifo1 ="Commander_to_Server",*fifo2="Server_to_Commander", *text_file="jobExecutorServer.txt";
	int serverpid;
	int fd1,fd2,  nwrite;
	struct stat st; //using it to check if there is already a fifo or a txt file.

	//creating fifo2 if it doesn't exist already,fifo 1 is created before the fork in the Commander.
	if (stat(fifo2, &st) != 0)
        mkfifo(fifo2, 0666);


	FILE *jobExecutorServer=fopen(text_file,"w+");
	if (jobExecutorServer==0){
		printf("the file cannot be created");   //in case there is something wrong with the creation of txt file
		exit(-1);
	}


	serverpid=getpid(); //getting the pid of the server process
	fprintf(jobExecutorServer,"the pid is:%d",serverpid);
	fclose(jobExecutorServer); //without closing it instantly, the buffer is not freed so the pid is never written into the txt file unless the whole procesee finishes.
	
	// this is for unpausing the server
	struct sigaction signal_action = {0};
	signal_action.sa_flags= SA_RESTART;
	signal_action.sa_handler=&signal_handler;
	sigaction(SIGUSR1,&signal_action,NULL);

	//this is for sigchld handling.
	struct sigaction sigchld_action = {0};
	sigchld_action.sa_flags= SA_RESTART;
	sigchld_action.sa_handler=&signal_sigchld;
	sigaction(SIGCHLD,&sigchld_action,NULL);

	struct queuejob
	{
		char jobID[6]; //one of the very few static arrays 
		char* job;
		int queuePosition;
		pid_t pid;
	};

	//initialization of queue and job struct that will be put in the queue.
	struct Queue* queue,*queuedprocesses;//*active_processes;
	queue=create_queue();
	struct queuejob command;
	queuedprocesses=create_queue();
	active_processes=create_queue();
	if (argc>2) { printf("Usage: receivemessage & \n"); exit(1); }
	char **arguments ;
	char *token;
	char* data;
	char *msgbuf;
	int concurrent_processes=1; //default is 1
	int jobCounter=1; //for jobID
	
	for (;;) {
		if(for_reading==1)	
        {
			int message_size = 0;
        	fd1 = open(fifo1, O_RDONLY);
        	if (fd1 < 0) {
            perror("fifo1 open problem");
            exit(2);
        	}
    	 	if (read(fd1, &message_size, sizeof(int)) < 0) {
            perror("problem in reading message size");
            close(fd1);
            exit(3);
       	 }


        	msgbuf = malloc((message_size + 1) * sizeof(char));
        	if (!msgbuf) {
          	  perror("memory allocation failed");
           	 close(fd1);
            	exit(4);
       	 }

        if (read(fd1, msgbuf, message_size) < 0) {
            perror("problem in reading message");
            free(msgbuf);
            close(fd1);
            exit(5);
        }
		close(fd1); //closing the fifo

        msgbuf[message_size] = '\0'; // Add null terminator to the received message
        printf("Received message: %s\n", msgbuf);


		int spaces = 0;
		for (int i=0; msgbuf[i]!='\0'; i++){
    		if(msgbuf[i]==' ')
        		spaces++;
		}


		arguments = malloc((spaces+2)*sizeof(char*)); // allocate one more for null termination
		if (arguments==0) {
			perror("memory allocation failed");
			close(fd1); // same logic as before
			exit(4);
		}



		token = malloc((message_size + spaces) * sizeof(char));
		token=strtok(msgbuf," ");
		data=malloc((message_size + 1) * sizeof(char));
		data[0]='\0'; //If I don't do that the string starts with gibberish. More on that here: https://stackoverflow.com/questions/69851898/gibberish-data-being-stored-in-memory-while-concatenating-the-strings-using-str
		for(int i=0; i<=spaces; i++){ //we're using spaces because it's the same amount as the args.
			arguments[i] = token;
			
			if(i>=1){
				strcat(data,token); //strtok modifies the buffer which divides. That's why we copy it into data, so we still have access to it later.
				if(i<spaces)
					strcat(data," "); //if it's not the last argument leave a space for the next one.
			}
			token = strtok(NULL," ");
			
		}	
		
	
		arguments[spaces+1] = NULL;
		if(strcmp(arguments[0],"issueJob")==0){
			int total_length=0;
			command.job=data;		
			command.jobID[0]='\0'; // from the malloc the string starts with gibberish that's why we initialize it with \0
			command.queuePosition=0;
			command.pid=0;
			if(sprintf(command.jobID,"job_%d",jobCounter++)<0){ //initializing and formatting the  command.jobID
				perror("\n sprintf error"); 
				exit(-2); 
			}


			total_length=strlen(command.job)+strlen(command.jobID)+4;//+1 for the null space and +1 for the queuePosition and +2 for the ","
			
			char*Serverbuffer=malloc((total_length+1)*sizeof(char));
			if(Serverbuffer==NULL)
				perror("malloc problem in Serverbuffer");
			Serverbuffer[0]='\0';

				if(sprintf(Serverbuffer,"%s,%s,%d,%d",command.jobID,command.job,command.queuePosition,command.pid)<1)
				{
					perror("\n sprintf error doing the Serverbuffer");
					exit(-2);
				}
				
				fd2 = open(fifo2, O_WRONLY);
					if (fd2 < 0) {
					perror("fifo1 open problem");
					exit(2);
				}

			if ((nwrite=write(fd2, &total_length,sizeof(total_length))) == -1) //passing the size that the commander will need to allocate
			{
				perror("Error in Writing"); 
				exit(2); 
			}
			
			Serverbuffer[total_length]='\0';
			
			if ((nwrite=write(fd2, Serverbuffer,total_length)) == -1) //passing the triplet
			{ 
				perror("Error in Writing");
				exit(2); 
			}
			
			
			enqueue(queue,data);
			command.queuePosition=queuedprocesses->size;
			command.queuePosition=queuedprocesses->size;
			sprintf(Serverbuffer,"%s,%s,%d,%d",command.jobID,command.job,command.queuePosition,command.pid);
			enqueue(queuedprocesses,Serverbuffer);
			//free(Serverbuffer); // we won't use it again.
		}
		
		
		else if(strcmp(arguments[0],"setConcurrency") == 0){
			concurrent_processes=atoi(arguments[1]);
			printf("\nThe concurrency is set to: %d",concurrent_processes);
			fflush(stdout);
		}	
		
		
	else if (strcmp(arguments[0], "stop") == 0) {
    int queueFlag = 0, message_sent = 0;
    char *job_in_question = malloc(25 * sizeof(char)); // allocate memory for the job message
    if (job_in_question==0) {
        perror("Memory allocation failed");
        exit(1);
    }


    char *stopping_job = arguments[1]; // the job we want to terminate/remove
    struct node* new_node;

    // Iterate through active processes queue first
    for (new_node = active_processes->front; new_node != NULL; new_node = new_node->next_node) {
        char* job_copy = strdup(new_node->job); // copy the job string
        if (!job_copy) {
            perror("Memory allocation failed");
            exit(1);
        }

        char *job_xx = strtok(job_copy, ","); 
        char *token = strtok(NULL, ",");

        if (strcmp(job_xx, stopping_job) == 0) {
            token = strtok(NULL, ",");
            token = strtok(NULL, ",");
            int pid = atoi(token);
			printf("I am before the termination\n");
            if (kill(pid, SIGTERM) != 0) {
                perror("Failed to terminate the process");
            }

            dequeue_node(active_processes, stopping_job);
            updating_queueposition(active_processes);
            sprintf(job_in_question, "job_%s terminated", job_xx);
            queueFlag = 1;
			printf("I AM BEFORE BREAK;\n");
            break;
        }
        
    }

    int buffer_size = strlen(job_in_question) + 1; // +1 for null terminator

    if (queueFlag == 1) {
        fd2 = open(fifo2, O_WRONLY);
        if (fd2 < 0) {
            perror("Failed to open fifo2");
            exit(2);
        }

        if (write(fd2, &buffer_size, sizeof(buffer_size)) == -1) {
            perror("Error writing buffer size");
            close(fd2);
            exit(2);
        }

        if (write(fd2, job_in_question, buffer_size) == -1) {
            perror("Error writing job message");
            close(fd2);
            exit(2);
        }
        close(fd2);
        message_sent = 1;
    } else {
        // Iterate through queued processes
        for (new_node = queuedprocesses->front; new_node != NULL; new_node = new_node->next_node) {
            char* copy_job = strdup(new_node->job); 
            if (copy_job==0) {
                perror("Memory allocation failed");
                exit(1);
            }
			printf("I am inside the queued processes");
            char *job_xx = strtok(copy_job, ",");
            if (strcmp(job_xx, stopping_job) == 0) {	
				printf("found it!!!\n");
				printf("the job_xx is: %s",job_xx);
                sprintf(job_in_question, "%s removed", job_xx);
                job_xx = strtok(NULL, ",");
				printf("the job in quuestion is  %s\n",job_in_question);
               
				
                buffer_size = strlen(job_in_question) + 1;
				printf("the buffer size is %d \n",buffer_size);
				fflush(stdout);
                fd2 = open(fifo2, O_WRONLY);
                if (fd2 < 0) {
                    perror("Failed to open fifo2");
                    free(copy_job);
                    exit(2);
                }
				printf("the job in question is %s \n",job_in_question);
				fflush(stdout);

                if (write(fd2, &buffer_size, sizeof(buffer_size)) == -1) {
                    perror("Error writing buffer size");
					free(copy_job);
                    close(fd2);
                    exit(2);
                }

                if (write(fd2, job_in_question, buffer_size) == -1) {
                    perror("Error writing job message");
                    free(copy_job);
                    close(fd2);
                    exit(2);
                }
                close(fd2);
                free(copy_job);
                message_sent = 1;
				dequeue_node(queuedprocesses, stopping_job);
                updating_queueposition(queuedprocesses);
                dequeue_node(queue, job_xx);
                updating_queueposition(queue);
                break;
            }
            free(copy_job);
        }

        if (message_sent == 0) {
            char* error_message = "The job you wanted to stop was not found"; 
            buffer_size = strlen(error_message) + 1;
            fd2 = open(fifo2, O_WRONLY);
            if (fd2 < 0) {
                perror("Failed to open fifo2");
                exit(2);
            }

            if (write(fd2, &buffer_size, sizeof(buffer_size)) == -1) {
                perror("Error writing buffer size");
                close(fd2);
                exit(2);
            }

            if (write(fd2, error_message, buffer_size) == -1) {
                perror("Error writing error message");
                close(fd2);
                exit(2);
            }
            close(fd2);
        }
    }

}

			
			
			
			
			
			
			
			
			
			
			

		

	
		else if(strcmp(arguments[0],"poll")==0)
		{		
			int max_jobSize = 100; // just to be safe,probably it's gonna be less
        	char* tripletString = malloc((active_processes->size+queuedprocesses->size) * max_jobSize * sizeof(char)); // 
			if(tripletString==0){
				perror("Malloc issue");
				exit(-1);
			}
			if(strcmp(arguments[1],"running")==0){
			if(active_processes->size>0)
			{
				
				struct node* new_node;
        		
				strcpy(tripletString,"the running processes are:\n"); //we're gonna use strcat, which needs the buffer to be initialized or else there may be gibberish.
					for(new_node=active_processes->front;new_node!=NULL;new_node=new_node->next_node)
					{
						
						strcat(tripletString,new_node->job);
						strcat(tripletString,"\n");
					}
				
				
			

			}
		}
			if(strcmp(arguments[1],"queued")==0)
			{
				if(queuedprocesses->size>0)
				{
					
					struct node* new_node;
					
					strcpy(tripletString,"the queued processes are:\n"); //we're gonna use strcat, which needs the buffer to be initialized or else there may be gibberish.
						for(new_node=queuedprocesses->front;new_node!=NULL;new_node=new_node->next_node)
						{
							
							strcat(tripletString,new_node->job);
							strcat(tripletString,"\n");
						}
					
					
				

				}
			}


			int message_size=strlen(tripletString);
			

			fd2 = open(fifo2, O_WRONLY);
					if (fd2 < 0) {
						perror("fifo1 open problem");
						free(tripletString);
						exit(2);
					}		
				if ((nwrite=write(fd2, &message_size,sizeof(message_size))) == -1) //passing the size that the commander will need to allocate
				{
					perror("Error in Writing"); 
					free(tripletString);
					exit(2); 
				}

				if ((nwrite=write(fd2, tripletString,message_size)) == -1) //passing the triplet
				{ 
					perror("Error in Writing");
					free(tripletString);
					exit(2); 
				}
				free(tripletString);

		}
		
		else if(strcmp(arguments[0],"exit")==0)
		{
			char* exit_message=malloc(31*sizeof(char));
			exit_message="jobExecutorServer terminated.";
			int message_size=strlen(exit_message);

			fd2 = open(fifo2, O_WRONLY);
					if (fd2 < 0) {
						perror("fifo1 open problem");
						exit(2);
					}		
				if ((nwrite=write(fd2, &message_size,sizeof(message_size))) == -1) //passing the size that the commander will need to allocate
				{
					perror("Error in Writing"); 
					exit(2); 
				}

				if ((nwrite=write(fd2, exit_message,message_size)) == -1) //passing the triplet
				{ 
					perror("Error in Writing");
					exit(2); 
				}
				break;
		}

		else{
			char *emergency_message;
			emergency_message="\nyou probably mistyped a keyword,try again\n";
			fd2 = open(fifo2, O_WRONLY);
			if (fd2 < 0) 
			{
				perror("fifo1 open problem");
				exit(2);
			}		
			int message_size=strlen(emergency_message);
				if ((nwrite=write(fd2, &message_size,sizeof(message_size))) == -1) //passing the size that the commander will need to allocate
				{
					perror("Error in Writing"); 
					exit(2); 
				}

				if ((nwrite=write(fd2, emergency_message,message_size)) == -1) //passing the triplet
				{ 
					perror("Error in Writing");
					exit(2); 
				}

		}
	for_reading=0;
}
		
		

		while(queuedprocesses->size>0 && running_processes<concurrent_processes){ 
			char* queuejob=dequeue(queue);
			dequeue(queuedprocesses); //also dequeing the triplets
			updating_queueposition(queuedprocesses);
			//queuedprocesses=update_queue(queuedprocesses);
			int i=0;
			char*queueargument=dequeue(queue);
			queueargument=strtok(queuejob," ");
			//tokenizing again, this time from the queue.
			while(queueargument!=NULL){
			arguments[i] = queueargument; //overwting arguments from queue so it can be used in execvp
			queueargument = strtok(NULL," ");
			i++;
			}	
			arguments[i]=NULL;

			running_processes++;
			char* buffer=malloc(15*sizeof(char));
			pid_t child=fork();
			
			if(child==0){
				
				if(execvp(arguments[0], arguments)==-1){
					perror("something is wrong in execvp:");
					exit(-1);
				
				}
				
				
				exit(1);
			}		
			else if(child>0){
				command.pid=child;
				command.queuePosition=active_processes->size;
				sprintf(buffer,"%s,%s,%d,%d",command.jobID,command.job,command.queuePosition,command.pid);
				enqueue(active_processes,buffer);
				free(buffer);
			}	
		}
	
		
		
		
	   pause();
		
	}
	remove(text_file);
	delete_queue(queue);
	delete_queue(queuedprocesses);
	delete_queue(active_processes);
	
}
	
	

