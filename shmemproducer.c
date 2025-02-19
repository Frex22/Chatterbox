#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<string.h>

#define SHM_SIZE 1024 /* make it a 1K shared memory segment */

int main() {
    int shmid;
    char *shmaddr;
    key_t key;

    // generate a unique key using ftok
    if(key = ftok("shmfile", 890) == -1) {
        perror("ftok");
        exit(1);
    }

    // create a shared memory segment
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666) < 0)) {
        perror("shmget");
        exit(1);
    }

    // attach the shared memory segment

    if ((shmaddr = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    // write to shared memory
    //strcpy vs strncpy
    //strcpy is a function that copies a string from one location to another.
    //strncpy is a function that copies a string from one location to another, 
    //but it also takes a third argument that specifies the maximum number of characters to copy.
    //read qna to know about why we using -1
    strncpy(shmaddr, "My first shared memory program", SHM_SIZE - 1);
    shmaddr[SHM_SIZE - 1] = '\0';
    printf ("Data written to shared memory %s\n", shmaddr);

    // reading from shared memory
    printf("Data read from shared memory %s\n", shmaddr);

    // detach the shared memory segment
    if (shmdt(shmaddr)== -1) {
        perror("shmdt");
        exit(1);
    }

    // remove the shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    return 0;

}
