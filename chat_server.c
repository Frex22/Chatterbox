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
#include <time.h>

#define MAX_CLIENTS 10
#define MAX_USERNAME 32
#define MSG_SIZE 256
#define LOG_SIZE (1024 * 1024)  /* 1MB for logs */

/* Message types */
#define MSG_TYPE_CONNECT 1
#define MSG_TYPE_DISCONNECT 2
#define MSG_TYPE_CHAT 3
#define MSG_TYPE_ACK 4

/* Client status */
#define CLIENT_INACTIVE 0
#define CLIENT_ACTIVE 1

/* Message structure */
typedef struct {
    long mtype;
    char username[MAX_USERNAME];
    char content[MSG_SIZE];
    time_t timestamp;
} Message;

/* Client structure */
typedef struct {
    int active;
    char username[MAX_USERNAME];
    int queue_id;
    pid_t pid;
} Client;

/* Log buffer structure */
typedef struct {
    size_t total_size;
    size_t used_size;
    int write_position;
    pthread_mutex_t mutex;
    char data[];  /* Flexible array member */
} LogBuffer;

/* Global variables */
Client clients[MAX_CLIENTS];
int server_queue_id;
LogBuffer *log_buffer;
int shm_id;
int running = 1;
pthread_t receiver_tid, log_sync_tid;


/* Function prototypes */
void initialize_server();
void cleanup_resources();
int add_client(const char *username, int queue_id, pid_t pid);
void remove_client(const char *username);
void broadcast_message(Message *msg, int exclude_index);
void handle_message(Message *msg);
void add_to_log(Message *msg);
void *message_receiver(void *arg);
void *log_sync_thread(void *arg);
void handle_signal(int sig);
//CHANGE New fun added
void handle_alarm(int sig);
void force_server_shutdown();

int main() {
    printf("Starting chat server V2...\n");
    
    /* Set up signal handler */
    signal(SIGINT, handle_signal);

    //CHANGE adding alarm sig handler

    signal(SIGALRM, handle_alarm);
    
    /* Initialize server resources */
    initialize_server();
    
    /* Start message receiver thread */
    if (pthread_create(&receiver_tid, NULL, message_receiver, NULL) != 0) {
        perror("Failed to create message receiver thread");
        cleanup_resources();
        exit(1);
    }
    
    /* Start log synchronization thread */
    if (pthread_create(&log_sync_tid, NULL, log_sync_thread, NULL) != 0) {
        perror("Failed to create log sync thread");
        running = 0;
        pthread_join(receiver_tid, NULL);
        cleanup_resources();
        exit(1);
    }
    
    /* Main server loop - can be used for server commands */
    char command[64];
    while (running) {
        printf("Server> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        command[strcspn(command, "\n")] = 0;  /* Remove newline character */

        /* Process server commands here if needed */
        if (strncmp(command, "quit", 4) == 0) {
            printf("Shutting down server...\n");
            running = 0;
            //CHANGE
            force_server_shutdown();
        } else if (strncmp(command, "list", 4) == 0) {
            /* List connected clients */
            printf("Connected clients:\n");
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].active) {
                    printf("  %s\n", clients[i].username);
                }
            }
        }
    }
    
    /* Wait for threads to finish */
    pthread_join(receiver_tid, NULL);
    pthread_join(log_sync_tid, NULL);
    
    /* Clean up resources */
    cleanup_resources();
    
    printf("Server shutdown complete\n");
    return 0;
}

/* Initialize server resources */
void initialize_server() {
    /* Initialize client array */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = CLIENT_INACTIVE;
    }
    
    /* Create server message queue */
    key_t server_key = ftok("server.key", 'S');
    if (server_key == -1) {
        perror("ftok server key");
        exit(1);
    }
    
    msgctl(msgget(server_key, 0666), IPC_RMID, NULL);  // Remove existing queue if any

    server_queue_id = msgget(server_key, 0666 | IPC_CREAT);
    if (server_queue_id == -1) {
        perror("msgget server queue");
        exit(1);
    }
    
    /* Create shared memory for logs */
    key_t shm_key = ftok("log.key", 'L');
    if (shm_key == -1) {
        perror("ftok shm key");
        exit(1);
    }

    shmctl(shmget( shm_key, 0, 0666), IPC_RMID, NULL);
    
    shm_id = shmget(shm_key, sizeof(LogBuffer) + LOG_SIZE, IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }
    
    /* Attach to shared memory */
    log_buffer = (LogBuffer *)shmat(shm_id, NULL, 0);
    if (log_buffer == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    
    /* Initialize log buffer */
    log_buffer->total_size = LOG_SIZE;
    log_buffer->used_size = 0;
    log_buffer->write_position = 0;
    
    /* Initialize mutex for log buffer */
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&log_buffer->mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    
    printf("Server initialized successfully\n");
}

//DEFINING FORCE SERVER SHUT DOWN
void force_server_shutdown(){
    printf("Forcefully shutting down server...\n");
    /*First attempt to wakeup receiver thread*/
    Message wake_msg;
    wake_msg.mtype = 999; // Arbitrary type to wake up receiver because it is blocked in msgrcv
    strcpy(wake_msg.username, "SERVER");
    strcpy(wake_msg.content, "Server is shutting down");
    msgsnd(server_queue_id, &wake_msg, sizeof(Message) - sizeof(long), IPC_NOWAIT);
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    /*set alaram as fallback*/
    alarm(5);

    /* Wait for receiver thread to finish */
    printf("Waiting for receiver thread to finish...\n");
    
}

void handle_alarm(int sig) {
    printf("Alarm triggered, shutting down...\n");
    exit(1);
}

/* Clean up server resources */
void cleanup_resources() {    
    /* Notify clients about server shutdown */
    Message shutdown_msg;
    shutdown_msg.mtype = MSG_TYPE_DISCONNECT;
    strcpy(shutdown_msg.username, "SERVER");
    strcpy(shutdown_msg.content, "Server is shutting down");
    shutdown_msg.timestamp = time(NULL);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            msgsnd(clients[i].queue_id, &shutdown_msg, sizeof(Message) - sizeof(long), IPC_NOWAIT); //changed to non blocking send check IPC_NOWAIT definition for more details
        }
    }
    
    usleep(500000);

    if (server_queue_id != -1) {
        msgctl(server_queue_id, IPC_RMID, NULL);
    }

    /* Destroy mutex and detach from shared memory */
    if (log_buffer != (void *)-1) {
        pthread_mutex_destroy(&log_buffer->mutex);
        shmdt(log_buffer);
    }
    
    /* Remove shared memory segment */
    if (shm_id != -1) {
        shmctl(shm_id, IPC_RMID, NULL);
    }
    
    printf("Resources cleaned up\n");
}

/* Add a new client */
int add_client(const char *username, int queue_id, pid_t pid) {
    int index = -1;
    
    /* Find empty slot in clients array */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        return -1;  /* No slots available */
    }
    
    /* Check if username already exists */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].username, username) == 0) {
            return -1;  /* Username already taken */
        }
    }
    
    /* Initialize client info */
    clients[index].active = CLIENT_ACTIVE;
    strncpy(clients[index].username, username, MAX_USERNAME - 1);
    clients[index].username[MAX_USERNAME - 1] = '\0';  /* Ensure null termination */
    clients[index].queue_id = queue_id;
    clients[index].pid = pid;
    
    /* Send welcome message */
    Message welcome_msg;
    welcome_msg.mtype = MSG_TYPE_ACK;
    strcpy(welcome_msg.username, "SERVER");
    sprintf(welcome_msg.content, "Welcome %s! You've joined the chat.", username);
    welcome_msg.timestamp = time(NULL);
    
    msgsnd(queue_id, &welcome_msg, sizeof(Message) - sizeof(long), 0);
    
    /* Notify other clients about the new user */
    Message join_msg;
    join_msg.mtype = MSG_TYPE_CHAT;
    strcpy(join_msg.username, "SERVER");
    sprintf(join_msg.content, "%s has joined the chat.", username);
    join_msg.timestamp = time(NULL);
    
    broadcast_message(&join_msg, index);
    add_to_log(&join_msg);
    
    printf("Client '%s' connected\n", username);
    return index;
}

/* Remove a client */
void remove_client(const char *username) {
    int index = -1;
    
    /* Find client in array */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].username, username) == 0) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        return;  /* Client not found */
    }
    
    /* Mark as inactive */
    clients[index].active = CLIENT_INACTIVE;
    
    /* Broadcast disconnect message */
    Message disconnect_msg;
    disconnect_msg.mtype = MSG_TYPE_CHAT;
    strcpy(disconnect_msg.username, "SERVER");
    sprintf(disconnect_msg.content, "%s has left the chat.", username);
    disconnect_msg.timestamp = time(NULL);
    
    broadcast_message(&disconnect_msg, -1);  /* Broadcast to all */
    add_to_log(&disconnect_msg);
    
    printf("Client '%s' disconnected\n", username);
}

/* Broadcast message to all connected clients */
void broadcast_message(Message *msg, int exclude_index) {
    /* Iterate through clients array */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        /* Send message to each active client except exclude_index */
        if (clients[i].active && i != exclude_index) {
            /*changed to non blocking */
            if (msgsnd(clients[i].queue_id, msg, sizeof(Message) - sizeof(long), IPC_NOWAIT) == -1) {
                if (errno == EINVAL || errno == EIDRM) {
                    printf("Client %s disconnected, removing from list\n", clients[i].username);
                    clients[i].active = CLIENT_INACTIVE;  /* Mark as inactive */
                } else {
                perror("msgsnd broadcast");
             }
            }
        }
    }
}

/* Handle incoming message based on type */
void handle_message(Message *msg) {
    int client_index = -1;
    
    /* Check message type */
    switch (msg->mtype) {
        case MSG_TYPE_CONNECT: {
            /* Extract client queue ID from content (assuming it's stored there) */
            int client_queue_id;
            pid_t client_pid;
            /*validation of message format*/

           if (sscanf(msg->content, "%d %d", &client_queue_id, &client_pid) != 2){
                printf("Invalid connect message format from %s\n", msg->username);
                return;  /* Invalid format */
            }
            
            /* Check if client is already connected */
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].active && strcmp(clients[i].username, msg->username) == 0) {
                    printf("Client %s is already connected\n", msg->username);
                    return;  /* Already connected */
                }
           }
            
            /* Add the client */
            int result = add_client(msg->username, client_queue_id, client_pid);
            if(result == -1) {
                printf("Failed to add client %s, no slots available or username taken\n", msg->username);
                Message error_msg;
                error_msg.mtype = MSG_TYPE_ACK;
                strcpy(error_msg.username, "SERVER");
                sprintf(error_msg.content, "Failed to connect: No slots available or username taken.");
                error_msg.timestamp = time(NULL);
                
                msgsnd(client_queue_id, &error_msg, sizeof(Message) - sizeof(long), 0);
            }
            break;
        }
        
        case MSG_TYPE_DISCONNECT:
            /* Handle client disconnection */
            remove_client(msg->username);
            break;
            
        case MSG_TYPE_CHAT:
            /* Handle chat message */
            printf("Chat from %s: %s\n", msg->username, msg->content);
            
            /* Find sender's index to exclude from broadcast (optional) */
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].active && strcmp(clients[i].username, msg->username) == 0) {
                    client_index = i;
                    break;
                }
            }
            
            /* Broadcast message to all other clients */
            broadcast_message(msg, client_index);  /* Send to all clients */
            
            /* Add message to log */
            add_to_log(msg);
            break;
            
        default:
            printf("Received message with unknown type: %ld\n", msg->mtype);
    }
}

/* Add a message to the log buffer */
void add_to_log(Message *msg) {
    /* Format message with timestamp */
    char log_entry[MAX_USERNAME + MSG_SIZE + 64];
    time_t now = msg->timestamp ? msg->timestamp : time(NULL);
    struct tm *timeinfo = localtime(&now);
    
    int len = sprintf(log_entry, "[%02d:%02d:%02d] <%s>: %s\n",
                     timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                     msg->username, msg->content);
    
    /* Add to log buffer in thread-safe manner */
    pthread_mutex_lock(&log_buffer->mutex);
    
    /* Check if we need to wrap around (circular buffer) */
    if (log_buffer->used_size + len > log_buffer->total_size) {
        /* Simple approach: clear the buffer if it's full */
        /*CHANGE Improved Circular buffer*/
        size_t half_size = log_buffer->total_size /2;
        memmove(log_buffer->data, log_buffer->data + half_size, log_buffer->used_size - half_size);
        log_buffer->used_size -= half_size;
        log_buffer->write_position = log_buffer->used_size;
    }
    
    /* Copy log entry to buffer */
    memcpy(&log_buffer->data[log_buffer->write_position], log_entry, len);
    log_buffer->write_position += len;
    log_buffer->used_size += len;
    
    pthread_mutex_unlock(&log_buffer->mutex);
}

/* Thread to receive incoming messages */
void *message_receiver(void *arg) {
    Message msg;
    
    while (running) {
        ssize_t result = msgrcv(server_queue_id, &msg, sizeof(Message) - sizeof(long), 0, IPC_NOWAIT);
        
        if (result == -1) {
            if (errno == ENOMSG) {
                /* No message available, sleep a bit and check running flag */
                usleep(100000);  /* 100ms */
                continue;
            } else if (errno == EINTR) {
                continue;  /* Interrupted by signal */
            } else {
                perror("msgrcv");
                if (!running) break;  /* Exit if we're shutting down */
                usleep(100000);  /* Wait a bit before trying again */
                continue;
            }
        } else {
            // CHANGE: Added handling for special shutdown message
            if (msg.mtype == 999) {
                if (!running) break;  /* Exit if we're shutting down */
                continue;
            }
            
            /* Process the message */
            handle_message(&msg);
        }
       
    }
    
    return NULL;
}

/* Thread to periodically sync logs */
void *log_sync_thread(void *arg) {
    FILE *log_file = NULL;
    
    while (running) {
        if (!running) break;
        /* Open log file for appending */
        log_file = fopen("chat_server.log", "a");
        if (!log_file) {
            perror("Failed to open log file");
            //CHANGE /* Try again after delay */
            for(int i=0; i<10 && running; i++){
                usleep(100000);  /* 100ms */
            }
            continue;
        }
        
        
        /* Lock the mutex to access log buffer */
        pthread_mutex_lock(&log_buffer->mutex);
        
        /* Write logs to file */
        if (log_buffer->used_size > 0) {
            fwrite(log_buffer->data, 1, log_buffer->used_size, log_file);
            //CHANGE
            fflush(log_file);
            /* Dont Clear the buffer after writing */
            log_buffer->used_size = 0;
            log_buffer->write_position = 0;
        }
        
        pthread_mutex_unlock(&log_buffer->mutex);
        
        /* Close the file */
        fclose(log_file);
        for (int i = 0; i<10 && running; i++){
            usleep(500000);
        }
    }
    printf("Log sync thread exiting...\n");
    return NULL;
}

/* Signal handler */
void handle_signal(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;

    //CHANGE
    force_server_shutdown();
}