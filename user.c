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

#define MAX_TIME 999999999

int main(int argc, char* argv[]){

    // Time variables for time incrementing and extra time
    int increment;
    int extra;
    int sec = cPTR->sec;
    int nanosec = cPTR->nanosec;

    // allocating the shared memory and message queue
    allocateSHRMEM();
    msgConfigure();

    // setting increment to the random time gotten from master
    increment = atoi(argv[0]);

    // making timer and checking if it is okay
    if ((nanosec + increment) > MAX_TIME){
        extra = (nanosec + increment) - MAX_TIME;
        sec += 1;
        nanosec = extra;
    }
    else {
        nanosec += increment;
    }

    https://www.tldp.org/LDP/tlk/kernel/processes.html
    while(requested == 0){
    //send message to message queue
        if(cPTR->sec >= sec && cPTR->nanosec >= nanosec){
            sprintf(message.msg_text, "%d", getpid());
            message.msg_type = 1;
            msgsnd(msgID, &message, sizeof(message), 0);
            requested = 1;
        }
    }

    // release the shared memory
    shmdt(cPTR);
    shmdt(rPTR);
    exit(0);
}