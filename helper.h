#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/types.h>

//global macros
#define MAX_PROCESS 18
#define MAX_RESOURCES 20


//keys for shared memory
#define C_SHMKEY 643783
#define RD_SHMKEY 643786
#define M_KEY 643789

//IDs for the shared memory for clock and resource
int cID;
int rID;
int msgID;

//arrays to store the running/blocked processes
int pidArray[18] = {};
int randTime[18] = {};
int blockedQueue[18] = {};
int requested = 0;



// clock struct
typedef struct myTime {
    unsigned int sec;
    unsigned int nanosec;
} myTime;


//struct for resource descriptor https://www.geeksforgeeks.org/bankers-algorithm-in-operating-system-2/
typedef struct resrcDes {
    //using baker's algorithm
    pid_t pids[MAX_PROCESS];
    int childResource[MAX_PROCESS];
    int nanosRequest[MAX_PROCESS];
    int request[MAX_PROCESS][MAX_RESOURCES];
    int resource_table[MAX_RESOURCES];
    int allocate_table[MAX_PROCESS][MAX_RESOURCES];
    int max[MAX_PROCESS][MAX_RESOURCES];

} resrcDes;

// messege queue struct
struct msgQ {
    long msg_type;
    char msg_text[100];
    int timeSlice;
} message;


// pointers for shared mem https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_shared_memory.htm
myTime *cPTR;
resrcDes *rPTR;

//https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_message_queues.htm
void msgConfigure(){
    msgID = msgget(M_KEY, 0666 | IPC_CREAT);
}

// allocating the shared mem https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_shared_memory.htm
void allocateSHRMEM() {

    //shared mem for resource descriptor
    rID = shmget(RD_SHMKEY, sizeof(resrcDes), IPC_CREAT|0777);
    if(rID < 0)
    {
        perror("ERROR: Resource Descriptor shared memeroy  error in master \n");
        exit(errno);
    }
    rPTR = shmat(rID, NULL, 0);
    if(rPTR < 0){
        perror("ERROR: Resource Descriptor memeroy error in oss\n");
        exit(errno);
    }

    //shared mem for system clock
    cID = shmget(C_SHMKEY, sizeof(myTime), IPC_CREAT|0777);
    if(cID < 0)
    {
        perror("ERROR: System Clock memeroy error in master \n");
        exit(errno);
    }
    cPTR = shmat(cID, NULL, 0);
    if(cPTR < 0){
        perror("ERROR: System Clock memeroy error in oss\n");
        exit(errno);
    }



}

