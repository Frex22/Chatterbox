#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<string.h>
#include<stdlib.h>

#define MSGSZ 128
typedef struct msgbuf {
    long mtype;
    char mtext[MSGSZ];
} message_buf;

int main (void) {
    int msqid;
    key_t key;
    message_buf rbuf;

    //generate unique key using ftok
    key = ftok("msgqueue.key",87);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    //connect to message queue using key
    /*Attempt to access the existing message queue identified by 'key' 
    with read/write permissions (0666)*/
    // If the message queue cannot be accessed (e.g., it doesn't exist), 
    //msgget returns -1, triggering an error.
     
    if ((msqid = msgget(key, 0666)) < 0) {
        perror("msgget");
        exit(1);
    }

    //receive message from message queue
    /*The msgrcv function receives a message from the message queue specified by msqid.*/
    // Attempt to receive a message from the message queue 'msqid' and store it in 'rbuf'.
    // - 'MSGSZ' specifies the maximum size (in bytes) of the message text to be received.
    // - '1' is the message type filter: only messages with mtype equal to 1 will be received.
    // - '0' indicates no special flags (i.e., the call will block until a matching message is available).
    // If msgrcv returns a negative value, it indicates an error occurred during the receive operation.
    if (msgrcv(msqid, &rbuf, MSGSZ, 1, 0) < 0){
        perror("msgrcv");
        exit(1);
    }

    //print the message
    printf("Message received: %s\n", rbuf.mtext);
    printf("Message Acknowledged\n");

    return 0;

}


