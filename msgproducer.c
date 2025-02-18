#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<string.h>
#include<stdlib.h>

#define MSGSZ 128

// Define the message structure
/* int msqid;
This variable is used to store the message queue identifier returned by the msgget() function. 
The identifier is needed to refer to the specific message queue in subsequent IPC operations such as sending or receiving messages.

key_t key;
This variable holds a unique key that identifies the message queue. Typically, you obtain this key using the ftok() function. 
The key is used by msgget() to either create a new message queue or access an existing one.

message_buf sbuf;
This is an instance of a custom structure (presumably defined elsewhere in your code) that holds the message details. 
Usually, such a structure contains at least a long field for the message type and a character array for the message text. 
It serves as the container for the message you want to send or receive.

size_t buf_length;
This variable is used to store the length of the message contained in sbuf. 
When you send a message using msgsnd(), you need to specify the number of bytes to be sent. 
The size_t type is ideal for this since it’s designed for sizes and counts.

size_t and key_t are not built-in language keywords like int or char; rather, 
they are typedefs provided by standard libraries.

size_t is typically defined in headers such as <stddef.h> or <stdlib.h>. 
It is an unsigned integer type used to represent the size of objects and is widely used in functions 
like malloc(), strlen(), and others that work with memory sizes or array indices.

key_t is defined in <sys/types.h> and is used in the context of System V IPC mechanisms. 
It represents a key value used to uniquely identify an IPC object like a message queue, shared memory segment, or semaphore set.*/

typedef struct msgbuf {
    long mtype;
    char mtext[MSGSZ];
} message_buf;

int main(void) {
    int msqid;
    key_t key;
    message_buf sbuf;
    size_t buf_length;

/*using ftok to create an unique key which will help identify the messasge*/

    key = ftok("msgqueue.key", 87);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // create or get a message queue
    if ((msqid = msgget(key, 0666 | IPC_CREAT)) < 0) {
        perror("msgget");
        exit(1);
    };

    // prepare to send message

    sbuf.mtype = 1;  // Set message type to 1 for identification/filtering
    strcpy(sbuf.mtext, "My First Message"); // Copy "My First Message" into the message text buffer
    buf_length = strlen(sbuf.mtext) + 1; //// Calculate the full length of the message (including the null terminator)

    // sending the message
    // Attempt to send the message stored in 'sbuf' to the message queue 'msqid'
    // using a non-blocking call (IPC_NOWAIT). If msgsnd returns a value < 0,
    // it indicates an error, so error handling can be performed.
    if(msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT) < 0) {
        perror("msgsnd");
        exit(1);
    } else{
        printf("Message Sent:%s\n", sbuf.mtext);
    }

    return 0;
}
    


