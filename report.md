# Project Contribution Report: ChatterBox IPC Chat System

## My Contributions: Aakash

### 1. Project Ideation and Architecture Design

As the architect on the ChatterBox project, I was responsible for conceptualizing the entire system and designing its core architecture. My initial vision focused on creating a distributed, reliable chat system that would showcase fundamental OS concepts:

- **Proposed the Multi-tier Architecture**: Designed the separation between client and server components with distinct responsibilities
- **Defined IPC Protocol**: Created the message-type based protocol (connect, disconnect, chat, acknowledgment) enabling seamless communication
- **Designed Buffer Management**: Conceived the circular buffer approach for log management in shared memory
- **Established Thread Model**: Architected the threading model for concurrent message handling

### 2. Complete Client Implementation (`chat_client.c`)

I took full ownership of implementing the client component, which serves as the user-facing interface for the chat system:

#### Core Communication Infrastructure

```c
int initialize_client(const char *user) {
    /* Set username */
    strncpy(username, user, MAX_USERNAME-1);
    username[MAX_USERNAME-1] = '\0';

    /* Create or get server queue key */
    key_t server_key = ftok("server.key", 'S');
    if (server_key == -1) {
        perror("ftok");
        return -1;
    }

    /* Connect to server queue */
    server_queue_id = msgget(server_key, 0666);
    if (server_queue_id == -1) {
        perror("msgget server queue");
        return -1;
    }
    
    /* Create client queue */
    key_t client_key = ftok("client.key", getpid());
    client_queue_id = msgget(client_key, 0666 | IPC_CREAT);
    
    /* Send connection message */
    Message connect_msg;
    connect_msg.mtype = MSG_TYPE_CONNECT;
    strncpy(connect_msg.username, username, MAX_USERNAME - 1);
    sprintf(connect_msg.content, "%d %d", client_queue_id, getpid());
    connect_msg.timestamp = time(NULL);
    msgsnd(server_queue_id, &connect_msg, sizeof(Message) - sizeof(long), 0);
    
    return 0;
}
```

#### Multi-threaded Message Reception

Implemented a robust message receiver thread with:

- **Non-blocking message reception** to prevent deadlocks during shutdown:
  ```c
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
      }
  }
  ```

- **Advanced error handling** with appropriate responses to different error conditions
- **Message formatting and display** with timestamp localization

#### User Interface and Command System

Developed a clean console interface with support for:

- **Chat message sending**: Handles formatting and transmission of user chat messages
- **Command processing**: Implemented full command system (`quit`, `logs`)
- **Command flexibility**: Added support for both slash and non-slash commands (`/quit` or `quit`)
- **Interactive prompt**: Maintained proper input prompt state after receiving messages

#### Shared Memory Access for Log Viewing

Implemented the log viewing functionality that accesses server logs from shared memory:

```c
void view_logs() {
    key_t shm_key = ftok("log.key", 'L');
    int shm_id = shmget(shm_key, 0, 0666);
    void *shm_addr = shmat(shm_id, NULL, SHM_RDONLY);
    
    LogBuffer *log_buffer = (LogBuffer *)shm_addr;
    
    if (log_buffer->used_size > 0) {
        printf("\n===== CHAT LOGS =====\n");
        
        // Added safety bounds check
        size_t safe_size = (log_buffer->used_size <= log_buffer->total_size) ? 
                          log_buffer->used_size : log_buffer->total_size;
        
        write(STDOUT_FILENO, log_buffer->data, safe_size);
        printf("\n====================\n");
    } else {
        printf("No logs available\n");
    }
    
    shmdt(shm_addr);
}
```

#### Robust Signal Handling

Added comprehensive signal handling for proper client shutdown:

```c
void handle_signal(int sig) {
    printf("\nReceived signal %d, disconnecting...\n", sig);
    running = 0;

    // Added wakeup mechanism to unblock receiver thread
    pthread_kill(receiver_tid, SIGUSR1);
}

void handle_usr1(int sig) {
    // Empty handler, just used to interrupt blocking system calls
}
```

### 3. Thread Management and Synchronization

Implemented sophisticated thread coordination techniques:

- **Thread Creation and Joining**: Proper lifecycle management for worker threads
  ```c
  /* Start message receiver thread */
  if (pthread_create(&receiver_tid, NULL, message_receiver, NULL) != 0) {
      perror("Failed to create message receiver thread");
      cleanup_resources();
      return 1;
  }
  
  /* Later during shutdown */
  pthread_join(receiver_tid, NULL);
  ```

- **Thread Wakeup Mechanism**: Used signals to unblock threads during shutdown:
  ```c
  // Wake up blocked thread
  pthread_kill(receiver_tid, SIGUSR1);
  ```

- **Atomic Flag Coordination**: Used `running` flag to coordinate thread shutdown

### 4. Resource Management and Cleanup

Implemented comprehensive resource management to prevent leaks:

```c
void cleanup_resources() {
    printf("Cleaning up resources...\n");
    if (server_queue_id != -1 && running) {
        // Send disconnect message
        Message disconnect_msg;
        disconnect_msg.mtype = MSG_TYPE_DISCONNECT;
        strcpy(disconnect_msg.username, username);
        strcpy(disconnect_msg.content, "");
        disconnect_msg.timestamp = time(NULL);
        msgsnd(server_queue_id, &disconnect_msg, sizeof(Message) - sizeof(long), IPC_NOWAIT);
    }
    
    // Wake up blocked threads
    if (client_queue_id != -1) {
        Message wakeup_msg;
        wakeup_msg.mtype = 999;
        msgsnd(client_queue_id, &wakeup_msg, sizeof(Message) - sizeof(long), IPC_NOWAIT);
        usleep(100000);
        
        // Remove client message queue
        msgctl(client_queue_id, IPC_RMID, NULL);
    }
    
    printf("Disconnected from server\n");
}
```

### 5. Build System and CI/CD Pipeline

I created the Makefile for the project with:

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread
DEPS = common.h
OBJS = chat_server.o chat_client.o

all: chat_server chat_client

chat_server: chat_server.o
	$(CC) $(CFLAGS) -o $@ $^

chat_client: chat_client.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o chat_server chat_client *.log
	
test: test_chat_sys
	./test_chat_sys
	
test_chat_sys: test_chat_sys.o
	$(CC) $(CFLAGS) -o $@ $^
```

Also established the GitHub Actions CI/CD pipeline to:
- Run automated builds on each commit
- Execute the test suite
- Archive build artifacts for release

```yaml
name: ChatterBox CI

on:
  push:
    branches: [ main, development ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Build
      run: make all
      
    - name: Test
      run: |
        make test
        
    - name: Archive artifacts
      uses: actions/upload-artifact@v2
      with:
        name: chat-binaries
        path: |
          chat_server
          chat_client
```

### 6. Testing Framework

I created comprehensive tests to verify system functionality:

- **Unit Tests**: For message structure integrity and protocol conformance
- **Integration Tests**: For client-server communication flow
- **Stress Tests**: For handling multiple rapid message exchanges

Key test cases included:
- Connection establishment with valid/invalid parameters
- Message sending with boundary conditions
- Graceful handling of server disconnection
- Resource cleanup verification

## Technical Challenges and Solutions

### Challenge 1: Blocked Threads During Shutdown

**Problem**: In early versions, the message receiver thread would block indefinitely in `msgrcv()`, preventing clean shutdown.

**Solution**: Implemented a three-part solution:
1. Used non-blocking message reception with `IPC_NOWAIT`
2. Added a signal handler for `SIGUSR1`
3. Implemented a wake-up mechanism using signals:
   ```c
   pthread_kill(receiver_tid, SIGUSR1);
   ```

### Challenge 2: Race Conditions in Cleanup

**Problem**: Race conditions during cleanup could result in accessing already freed resources.

**Solution**: 
1. Added proper sequence validation using the `running` flag
2. Added delay between wake-up message and queue removal:
   ```c
   msgsnd(client_queue_id, &wakeup_msg, sizeof(Message) - sizeof(long), IPC_NOWAIT);
   usleep(100000);  // Give time for thread to process the message
   msgctl(client_queue_id, IPC_RMID, NULL);
   ```

### Challenge 3: Shared Memory Access Synchronization

**Problem**: Reading from shared memory was susceptible to data corruption if the server was writing logs at the same time.

**Solution**: Added boundary checking to prevent access violations:
```c
size_t safe_size = (log_buffer->used_size <= log_buffer->total_size) ? 
                  log_buffer->used_size : log_buffer->total_size;
```

## Key Innovations

1. **Flexible Command Processing**: Added support for commands with or without the leading slash
   ```c
   if (strcmp(buffer, "/quit") == 0 || strcmp(buffer, "quit") == 0) {
       // Quit handling
   }
   ```

2. **Unblocking Message Queue**: Implemented a special wake-up message type (999) to unblock threads
   ```c
   wakeup_msg.mtype = 999; // Special type to wake up blocked threads
   msgsnd(client_queue_id, &wakeup_msg, sizeof(Message) - sizeof(long), IPC_NOWAIT);
   ```

3. **Enhanced Error Classification**: Differentiated between error types for appropriate handling
   ```c
   if (errno == ENOMSG) {
       // No message available
   } else if (errno == EINTR) {
       // Interrupted by signal
   } else if (errno == EIDRM || errno == EINVAL) {
       // Queue removed or invalid
   }
   ```

## Future Enhancements

Based on insights gained during implementation, I've identified several opportunities for future enhancement:

1. **Message Encryption**: Add end-to-end encryption for secure communication
2. **Enhanced UI**: Implement curses-based interface with separate message history and input areas
3. **Reconnect Mechanism**: Add automatic reconnection on server restart
4. **Message Persistence**: Add client-side message caching for offline operation
5. **User Authentication**: Add username/password authentication

## Conclusion

My contributions to ChatterBox focused on creating a robust, user-friendly client implementation with proper resource management and threading. The client component successfully:

- Establishes reliable communication through System V message queues
- Handles user input and server messages concurrently
- Provides access to chat history through shared memory
- Manages resources properly with comprehensive cleanup procedures
- Implements graceful shutdown with thread coordination

This project demonstrates practical application of key operating system concepts including inter-process communication, shared memory management, threading, and synchronization primitives in a real-world application context.
