#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <errno.h>

#define MAX_USERNAME 32
#define MSG_SIZE 256

/* Message types */
#define MSG_TYPE_CONNECT 1
#define MSG_TYPE_DISCONNECT 2
#define MSG_TYPE_CHAT 3
#define MSG_TYPE_ACK 4

/* Message structure */
typedef struct {
    long mtype;
    char username[MAX_USERNAME];
    char content[MSG_SIZE];
    time_t timestamp;
} Message;

/* Log buffer structure */
typedef struct {
    size_t total_size;
    size_t used_size;
    int write_position;
    pthread_mutex_t mutex;
    char data[];
} LogBuffer;

/* Global variables */
int server_queue_id;
int client_queue_id;
char username[MAX_USERNAME];
int running = 1;
pthread_t receiver_tid; /* Thread ID for message receiver */

/* Function prototypes */
int initialize_client(const char *user);
void cleanup_resources();
void *message_receiver(void *arg);
void send_message(const char *content);
void view_logs();
void handle_signal(int sig);

void handle_usr1(int sig); // Signal handler for SIGUSR1

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <username>\n", argv[0]);
        return 1;
    }
    
    /* Set up signal handler */
    signal(SIGINT, handle_signal);
    //CHANGE added signal handler for SIGUSR1
    signal(SIGUSR1, handle_usr1);
    
    /* Initialize client */
    if (initialize_client(argv[1]) != 0) {
        return 1;
    }
    
    /* Start message receiver thread */
    if (pthread_create(&receiver_tid, NULL, message_receiver, NULL) != 0) {
        perror("Failed to create message receiver thread");
        cleanup_resources();
        return 1;
    }
    
    /* Main loop for sending messages */
    char buffer[MSG_SIZE];
    printf("You: ");
    fflush(stdout);

    while (running) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }
        
        /* Remove newline */
        buffer[strcspn(buffer, "\n")] = '\0';
        //CHANGE
         // CHANGE: Accept commands with or without slash
         if (strcmp(buffer, "/quit") == 0 || strcmp(buffer, "quit") == 0) {
            printf("Exiting...\n");
            running = 0;
            
            // CHANGE: Send signal to wake up message receiver thread
            pthread_kill(receiver_tid, SIGUSR1);
            
            // CHANGE: Break out of the loop immediately
            break;
        } else if (strcmp(buffer, "/logs") == 0 || strcmp(buffer, "logs") == 0) {
            view_logs();
            printf("You: ");
            fflush(stdout);
        } else {
            send_message(buffer);
            printf("You: ");
            fflush(stdout);
        }
       
    }
    
    /* Wait for receiver thread to finish */
    pthread_join(receiver_tid, NULL);
    
    /* Clean up resources */
    cleanup_resources();
    
    return 0;
}

/* Initialize client resources */
int initialize_client(const char *user) {
    //TODO: Implement client initialization
    /* - Set username */
    strncpy(username, user, MAX_USERNAME-1);
    username[MAX_USERNAME-1] = '\0'; /* Ensure null termination */


    /* - Create or get server queue key */
    key_t server_key = ftok("server.key", 'S');
    if (server_key == -1) {
        perror("ftok");
        return -1;
    }

    /* - Connect to server queue */
    server_queue_id = msgget(server_key, 0666);
    if (server_queue_id == -1) {
        perror("msgget server queue");
        return -1;
    }

    /* create client key*/
    key_t client_key = ftok("client.key", getpid());
    if (client_key == -1) {
        perror("ftok client key");
        return -1;
    }
    /* - Create client queue */
    client_queue_id = msgget(client_key, 0666 | IPC_CREAT);
    if (client_queue_id == -1) {
        perror("msgget client queue");
        return -1;
    }
    /* - Send connection message */
    Message connect_msg;
    connect_msg.mtype = MSG_TYPE_CONNECT;
    strncpy(connect_msg.username, username, MAX_USERNAME - 1);
    connect_msg.username[MAX_USERNAME - 1] = '\0'; /* Ensure null termination */
    sprintf(connect_msg.content, "%d %d", client_queue_id, getpid());
    connect_msg.timestamp = time(NULL);
    if (msgsnd(server_queue_id, &connect_msg, sizeof(Message) - sizeof(long), 0) == -1) {
        perror("msgsnd connect");
        return -1;
    }
    printf("Connected to server as %s\n", username);
    return 0;
}

/* Clean up client resources */
void cleanup_resources() {
    /* TODO: Implement cleanup logic */
    /* - Send disconnect message */
    /*improving this function*/
    printf("Cleaning up resources...\n");
    if (server_queue_id != -1 && running) // checks if server queue is valid
    { 
        Message disconnect_msg;
        disconnect_msg.mtype = MSG_TYPE_DISCONNECT;
        strcpy(disconnect_msg.username, username);
        strcpy(disconnect_msg.content, "");
        disconnect_msg.timestamp = time(NULL);

        //adding non blocking send
        msgsnd(server_queue_id, &disconnect_msg, sizeof(Message) - sizeof(long), IPC_NOWAIT);
    }
    /* - Remove message queue */
    /*adding wakeup mechanim to unblock server thread*/
    if (client_queue_id != -1) {
        Message  wakeup_msg;
        wakeup_msg.mtype = 999; // arbitrary type to wake up server
        msgsnd(client_queue_id, &wakeup_msg, sizeof(Message) - sizeof(long), IPC_NOWAIT);
        usleep(100000);

        msgctl(client_queue_id, IPC_RMID, NULL);
    }

    printf("Disconnected from server\n");
}


/* Thread to receive incoming messages */
void *message_receiver(void *arg) {
    /* TODO: Implement message receiving logic */
    Message received_msg;
    size_t bytes_received;
    char timestamp_str[20];
    struct tm *tm_info;
    printf("Message receiver thread started\n");
    while (running) {
        /*Receive message from client queue*/
        /*changing from blocking to non blockinf w timeout*/

        bytes_received = msgrcv(client_queue_id, &received_msg, sizeof(Message) - sizeof(long), 0, IPC_NOWAIT);
        if(bytes_received == -1) {
            if (errno == ENOMSG) {
                usleep(100000); // Sleep for 100ms
                continue;
            } else if (errno == EINTR) {
                continue;
            } else if (errno == EIDRM || errno == EINVAL) {
                printf("Message queue removed or invalid\n");
                break;
            } else{
            perror("msgrcv");
            continue; //CHANGE: Continue instead of breaking 
            }
        }

        //CHANGE
        if (received_msg.mtype == 999) {
            printf("Received wakeup signal, exiting...\n");
            continue; // Ignore this message
        }

        /* Format timestamp */
        tm_info = localtime(&received_msg.timestamp);
        strftime(timestamp_str, sizeof(timestamp_str), "%H:%M:%S", tm_info);

        /*Process message based on type*/
        switch(received_msg.mtype){
            case MSG_TYPE_ACK:
                printf("\n[%s] [SERVER] %s\n", timestamp_str, received_msg.content);
                break;
            
            case MSG_TYPE_CHAT:
                if (strcmp(received_msg.username, "SERVER") == 0) {
                    printf("\n[%s] [SERVER] %s\n", timestamp_str, received_msg.content);
    
                }
                else {
                    printf("\n[%s] [%s] %s\n", timestamp_str, received_msg.username, received_msg.content);
                }
                break;
            /*CHANGE: Added case for disconnect message*/
            case MSG_TYPE_DISCONNECT:
                printf("\n[%s] Server is shutting down. Disconnecting...\n", timestamp_str);
                running = 0;  /* Set running to false to exit main loop */
                return NULL;  /* Exit thread immediately */
                
            default:
                printf("\nUnknown message type: %ld\n", received_msg.mtype);
                break;           
        }       
        printf("You:");
        fflush(stdout); // Ensure prompt is displayed immediately
    
    }   
    /* - Loop to receive messages from client queue */
    /* - Display messages to user */
    printf("Message receiver thread exiting\n");
    return NULL;
}

/* Send a chat message to the server */
void send_message(const char *content) {
    /* TODO: Implement message sending logic */
    /* - Create and initialize message structure */
    Message chat_msg;
    chat_msg.mtype = MSG_TYPE_CHAT;
    strncpy(chat_msg.username, username, MAX_USERNAME - 1);
    chat_msg.username[MAX_USERNAME - 1] = '\0'; /* Ensure null termination */

    strncpy(chat_msg.content, content, MSG_SIZE - 1);
    chat_msg.content[MSG_SIZE - 1] = '\0'; /* Ensure null termination */
    chat_msg.timestamp = time(NULL);

    /* - Send to server queue */
    if (msgsnd(server_queue_id, &chat_msg, sizeof(Message)- sizeof(long), 0) == -1) {
        if (errno == EINVAL || errno == EIDRM) {
            printf("Server queue removed or invalid\n");
            running = 0; // Set running to false to exit main loop
            pthread_kill (receiver_tid, SIGUSR1); // Wake up receiver thread
        } else {
            perror("msgsnd chat");
        }
    }
}

/* View chat logs from shared memory */
void view_logs() {
    /* TODO: Implement log viewing logic */
    key_t shm_key = ftok("log.key", 'L');
    if(shm_key == -1) {
        perror("ftok log key");
        return;
    }
    /* - Get shared memory segment */
    int shm_id = shmget(shm_key, 0, 0666);
    if(shm_id == -1){
        perror("shmget logs");
        return;
    }
    /* - Attach to shared mem */
     void *shm_addr = shmat(shm_id, NULL, SHM_RDONLY);
    if(shm_addr == (void *)-1){
        perror("shmat logs");
        return;
    }
    /* - Read logs from shared memory */
    LogBuffer *log_buffer = (LogBuffer *)shm_addr;

    //CHANGE
    printf("Log buffer info: total_size=%zu, used_size=%zu, write_position=%d\n", 
        log_buffer->total_size, log_buffer->used_size, log_buffer->write_position);
    
        if (log_buffer->used_size > 0) {
            printf("\n===== CHAT LOGS =====\n");
            
            // CHANGE: Added safety bounds check
            size_t safe_size = (log_buffer->used_size <= log_buffer->total_size) ? 
                               log_buffer->used_size : log_buffer->total_size;
            
            if (write(STDOUT_FILENO, log_buffer->data, safe_size) == -1) {
                perror("Failed to write logs");
            }
            
            printf("\n====================\n");
        } else {
            printf("No logs available\n");
        }  
    shmdt(shm_addr);
}


/* Signal handler */
void handle_signal(int sig) {
    printf("\nReceived signal %d, disconnecting...\n", sig);
    running = 0;

    //change added wakeup mechanism to unblock server thread
    pthread_kill(receiver_tid, SIGUSR1);
}

void handle_usr1(int sig) {

}
