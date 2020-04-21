//Amy Seidel
//CS4760 - OS
//Project 5
//All the resources I have referenced are outlined in the README or in the function description, many were repeated from project 4

#include "helper.h"
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

//Macros
#define MAX_RAND 5
#define MAX_TIME 999999999

//prototypes
void checkclock();
void sig(int val);
static void timer();

void makeProcess(int pidArray[]);
void checkMessage();
void processStat(int pid);

void printlogAllocated(int processNum, int requestNum);
void printLog(int processNum, int requestNum);
void printLogBlocked(int processNum, int requestNum);
void printlogDeadlocked(int processNum, int requestNum);
void printlogReleased(int processNum, int requestNum);


int main(int argc, char* argv[]) {

    //getting the getopt values and printing help screen
    int val;
    while ((val = getopt(argc,argv, "h"))!=-1){
        switch (val){
            case 'h':
                printf("HELP: Enter ./oss to run program. Log file is stored in logfile.txt\n");
                return 1;

            default:
                printf("%s: Invalid argument \n", argv[0]);
                printf("Please type ./oss -h to see the help menu");
                return -1;
        }

    }

    // allocating
    allocateSHRMEM();
    msgConfigure();

    //signals and alarms, timeouts
    signal(SIGINT, sig);
    signal(SIGALRM, timer);
    alarm(2);


    //initlize clock, create the process and check the message queue for requests
    while(1){
        checkclock();
        makeProcess(pidArray);
        checkMessage();
    }
}


//function for the interruptHandler - http://www.signal.u.se/Staff/pd/DSP/Doc/ctools/apxc.pdf
void sig(int val){

    // kill children
    int i;
    for(i = 0; i < 18; i++){
        if(pidArray[i] != 0){
            signal(SIGQUIT, SIG_IGN);
            kill(pidArray[i], SIGQUIT);
        }
    }

    // free shared memory
    shmdt(cPTR);
    shmdt(rPTR);

    //freeing the memory
    shmctl(cID, IPC_RMID, NULL);
    shmctl(rID, IPC_RMID, NULL);
    msgctl(msgID, IPC_RMID, NULL);

    printf("^C termination\n");
    exit(0);
}

//same as above, just terminates after a certain time
static void timer(){

    // kill children
    int i;
    for(i = 0; i < 18; i++){
        if(pidArray[i] != 0){
            signal(SIGQUIT, SIG_IGN);
            kill(pidArray[i], SIGQUIT);
        }
    }

    // free shared memory
    shmdt(cPTR);
    shmdt(rPTR);

    shmctl(cID, IPC_RMID, NULL);
    shmctl(rID, IPC_RMID, NULL);
    msgctl(msgID, IPC_RMID, NULL);

    printf("Sucessfully exited after 2 seconds.\n");
    exit(EXIT_SUCCESS);
}
void checkclock(){

    int increment = 100000;
    int extra;
    //checking the clock
    if ((cPTR->nanosec + increment) > MAX_TIME){
        extra = (cPTR->nanosec + increment) - MAX_TIME;
        cPTR->sec += 1;
        cPTR->nanosec = extra;
    }
    else {
        cPTR->nanosec += increment;
    }
}

void checkMessage(){

    int pid_m;

    msgrcv(msgID, &message, sizeof(message), 1, IPC_NOWAIT);

    // write the message
    if(message.msg_text[0] != '0') {
        pid_m = atoi(message.msg_text);
        processStat(pid_m);
    }
    strcpy(message.msg_text, "0");
}


//creating the processes
void makeProcess(int pidArray[]){

    int i, j;
    for(i = 0; i < MAX_PROCESS; i++){
        if(pidArray[i] == 0) {
            for(j = 0; j < 20; j++){
                rPTR->request[i][j] = (rand() % MAX_RAND) + 1;
            }

            int chance = (rand() % 100) + 1;

            if(chance >= 80){
                rPTR->childResource[i] = 0;
            }
            else if (chance >= 40){
                rPTR->childResource[i] = 1;
            }
            else{
                rPTR->childResource[i] = 2;
            }

            randTime[i] = (rand() % 100000000) + 1000000;

            char msg[10];

            sprintf(msg, "%d", randTime[i]);

            // fork to user executable
            if ((pidArray[i] = fork()) == 0) {
                execl("./user", msg, NULL);
            }
        }
    }
}

// process exits! complete the process
void processStat(int pid){

    int i;
    int num;
    int processNum;
    int requestNum;

    for(i = 0; i < 18; i++){
        if(pidArray[i] == pid){
            num = rPTR->childResource[i];
            processNum = i;
            requestNum = (rand() % 20);
            printLog(processNum, requestNum);
        }
    }

    //make sure only size 1 or 2 can be allocated
    if(num == 1 || num == 2){

        //check if resources are available
        if(rPTR->request[processNum][requestNum] <= rPTR->resource_table[requestNum]){
            rPTR->allocate_table[processNum][requestNum] = rPTR->request[processNum][requestNum];
            rPTR->resource_table[requestNum] -= rPTR->request[processNum][requestNum];
            printlogAllocated(processNum, requestNum);
        }
        else {
            //sadly they are blocked
            int j;
            int amount = 0;
            for(j = 0; j < 18; i++){
                if(blockedQueue[j] == 0){
                    blockedQueue[j] = pid;
                    amount = 1;
                    // write blocked to log
                    printLogBlocked(processNum, requestNum);
                }
                if(amount == 1){
                    i = 18;
                }
            }
        }

    }
        //terminate these processes
    else if(num == 0) {
        // kill process
        requested = 1;
        pidArray[processNum] = 0;
    }
}




//added print log functions for easier clean up
void printlogAllocated(int processNum, int requestNum){

    FILE *fp = fopen("logfile.txt", "a+");
    fprintf(fp, "Master granting P%d request R%d at time %d:%d\n", processNum, requestNum, cPTR->sec, cPTR->nanosec);
    fclose(fp);

}

void printLogBlocked(int processNum, int requestNum){

    FILE *fp = fopen("logfile.txt", "a+");
    fprintf(fp, "Master blocking P%d for requesting R%d at time %d:%d\n", processNum, requestNum, cPTR->sec, cPTR->nanosec);
    fclose(fp);

}
void printLog(int processNum, int requestNum){

    FILE *fp = fopen("logfile.txt", "a+");
    fprintf(fp, "Master has detected Process P%d requesting R%d at time %d:%d\n", processNum, requestNum, cPTR->sec, cPTR->nanosec);
    fclose(fp);

}
void printlogReleased(int processNum, int requestNum){

    FILE *fp = fopen("logfile.txt", "a+");
    fprintf(fp, "Master r has acknowledged P%d releasing R%d at time %d:%d\n", processNum, requestNum, cPTR->sec, cPTR->nanosec);
    fclose(fp);
}
void printlogDeadlocked(int processNum, int requestNum){

    FILE *fp = fopen("logfile.txt", "a+");
    fprintf(fp, "Master running deadlock detection at time %d:%d\n", cPTR->sec, cPTR->nanosec);
    fclose(fp);
}


