#include <stdlib.h>
#include <stdio.h>
#include <string.h>
struct queuejob
	{
		char jobID[6]; 
		char* job;
		int queuePosition;
        int pid;
	};

struct node
{
    char* job; // the string command  from jobCommander
    struct node* next_node; //the pointer that'll show to the next node of the queue
};

struct Queue{
struct node *rear; //πίσω μέρος ουράς
struct node *front; //μπροστινό μέρος ουράς
int size;
};



struct Queue* create_queue(){ //creating the queue. 
    struct Queue * temp= (struct Queue*)malloc(sizeof(struct Queue)); // queue variable that will be used to allocate the neccesary space and initialize the queue.
   temp->front=temp->rear=NULL; //at the initialization both the front and read are zero
   temp->size = 0; //initializing size
   return temp;
    
}

struct node * create_node(){ //returns a new node
    struct node* temp;
    temp= (struct node*)malloc(sizeof(struct node)); //initiazing an empty node
    temp->job=NULL;
    temp->next_node=NULL;
    return temp;
}

void enqueue(struct Queue* queue, char* jobString){
    if (jobString == NULL) {
    printf("Error: jobString is NULL\n");
    exit(-1);
    }
    struct node* new_node=create_node();
    if (new_node == NULL) {
    printf("Error: Failed to allocate memory for new node\n");
    exit(-1);
}
    int length=strlen(jobString);
    new_node->job=malloc(length*sizeof(char));
    strcpy(new_node->job,jobString);//passing the jobstring as a job to the node
    if (new_node->job == NULL) {
        printf("Error: Failed to allocate memory for job string\n");
        free(new_node);
        exit(-1);
    }
    if (queue->rear==NULL){ //if the queue is empty/we are adding the 1st element
        queue->front=queue->rear=new_node; //it's only one element so the pointer doesn't have anywhere to point
        
    }
    else{
        queue->rear->next_node = new_node;//updating the last element before the current enqueuing
        queue->rear = new_node; //current rear is new_node (the next_node now is null, it will be changed in the next enqueuing....)
    }
    queue->size++;
}

//dequeing will return the job of the npde
char* dequeue(struct Queue* queue){
    if(queue->front==NULL){
        return '\0';
    }
    struct node* new_node=queue->front;
    char *data=new_node->job;
    queue->front=queue->front->next_node; //checking the value of next node to see if we are dequeuing the only element of the queue
    if(queue->front==NULL){ //if we are dequeuing the last element we need to set rear=front
        queue->rear=NULL;
    }
    free(new_node);
    queue->size--; //decrementing the size
    return data; // return the data of the node that got dequeued
}



//in this function will reach each node and free it individually. Then
// we will free the struct Queue as a whole
void delete_queue(struct Queue* queue){
struct node* new_node; //we will need to create a new node in roder to reach every node in the queue.
for(new_node=queue->front;new_node!=NULL;new_node=queue->front){
    free(new_node); //freeing the specific node
    queue->front=queue->front->next_node; //setting the front node of the queue as the next node,so we fully iterate it
}
free(queue);//after we have freed each node seperately, we free the struct Queue.
}


void print_queue(struct Queue* queue){
    struct node* new_node;
    int i=0;
    if (queue->front==NULL)
        printf("the queue is wrong");
    for(new_node=queue->front;new_node!=NULL;new_node=new_node->next_node){
        i++;
      //  printf("\n the %d node has %s job\n ",i,new_node->job);
        fflush(stdout);
    }
}

//we keep track of the previous nodes, so when we find the node to remove, the next_node
//pointer of the previous node will show into the next node of the one that got dequeued.
//there is no need to be a char* type, but when  itried to convert it into a void the stop 
//function was freezing, so i let it as it is.. I do not use the returned job anywhere..
char* dequeue_node(struct Queue* queue, char* data) {
    struct node* new_node;
    struct node* previous_node = NULL;
    char *buffer=malloc(20*sizeof(char)); //will return the "job_xx terminated"
    for(new_node = queue->front; new_node != NULL; new_node = new_node->next_node) 
    {   
        char* baseline = strchr(new_node->job, ',');
        if(strcmp(new_node->job, data) == 0 || (baseline != NULL && strstr(new_node->job, data) !=NULL)) { //if the string of data is found inside the string ",job_1,job,0"
            printf("I am freeing the node %s\n", new_node->job);
            char *job_xx=strstr(new_node->job, data); //this returns job,queueposition (i am not even sure why, i though it would return ontly the job.)
            char* token=strtok(job_xx,",");
            sprintf(buffer,"%s removed",token); // now with the tokenization we're good
            if(previous_node == NULL) { // if we're removing the first node
                queue->front = new_node->next_node; //then the 2nd element becomes queue->front,no fhurther changes needed,the next_node pointer remains right
            } else { // if the removing node is in the middle or at the end
                previous_node->next_node = new_node->next_node; //the node before the current one shows to th node after the current one
            }
            free(new_node); // deqeueuing the node which job matches char* data
            queue->size--; // decrementing queue's size 
            printf("The queue size while dequeueing is: %d\n", queue->size);
            return buffer;
        }
        previous_node = new_node; //before we move to the next node, we keep the current_node
    }
    return NULL;
}






//making sure that when we remove a node!=queue->front, the queueposition will be updated
void updating_queueposition(struct Queue* queue)
{   
    int queueposition=0;
    struct node* new_node=queue->front;
    int collumn=0; 
    while(new_node!=NULL)
    {
        //with this we're checking in which queueposition we currently are
        int i=0;
        char foratoi[2];
        for(i=0;new_node->job[i]!='\0';i++)
        {
            if(new_node->job[i+1] == ',' && new_node->job[i+2] != '\0') //the format is: "job_xx,job,queueposition,pid_t"
            {
                foratoi[1] = '\0'; //atoi cannot accept a single char without termination character,that;s why we created a "string" with null-char.
                foratoi[0] = new_node->job[i];
                queueposition = atoi(foratoi); //converted the last char which is a number in a string format to integer.
                if(collumn <queueposition) 
                {
                    char str[12]; // Buffer big enough for 11-character int plus NUL
                    sprintf(str, "%d", collumn); //adding the number as a string
                    new_node->job[i+2] = str[0];
                }
            }
        }
        new_node = new_node->next_node; //we're going to check the string of the next node
        collumn++; //we're moving onto the next one
    }
}

