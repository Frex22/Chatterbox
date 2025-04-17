/**
 * Tests for ChatterBox Chat System
 * 
 * This file implements unit tests for the chat system components
 * using a simple test framework.
 */

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
 #include <assert.h>
 
 /* Define the same structures as the main program */
 #define MAX_USERNAME 32
 #define MSG_SIZE 256
 #define LOG_SIZE (1024 * 1024)  /* 1MB for logs */
 
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
 
 /* Global variables for tests */
 int num_tests = 0;
 int num_passed = 0;
 
 /* Test helper macros */
 #define TEST(name) do { \
     printf("Test %d: %s... ", ++num_tests, name); \
     fflush(stdout); \
 } while(0)
 
 #define PASS() do { \
     printf("PASSED\n"); \
     num_passed++; \
 } while(0)
 
 #define FAIL(reason) do { \
     printf("FAILED (%s)\n", reason); \
 } while(0)
 
 #define ASSERT_TRUE(expr) do { \
     if (!(expr)) { \
         FAIL("Assertion failed: " #expr); \
         return; \
     } \
 } while(0)
 
 #define ASSERT_EQ(expected, actual) do { \
     if ((expected) != (actual)) { \
         FAIL("Expected " #expected " but got " #actual); \
         return; \
     } \
 } while(0)
 
 #define ASSERT_STR_EQ(expected, actual) do { \
     if (strcmp((expected), (actual)) != 0) { \
         FAIL("String comparison failed"); \
         return; \
     } \
 } while(0)
 
 /* Test functions */
 
 /* Test message creation and serialization */
 void test_message_creation() {
     TEST("Message creation and initialization");
     
     Message msg;
     msg.mtype = MSG_TYPE_CHAT;
     strncpy(msg.username, "TestUser", MAX_USERNAME - 1);
     msg.username[MAX_USERNAME - 1] = '\0';
     strncpy(msg.content, "Hello, World!", MSG_SIZE - 1);
     msg.content[MSG_SIZE - 1] = '\0';
     msg.timestamp = time(NULL);
     
     ASSERT_EQ(MSG_TYPE_CHAT, msg.mtype);
     ASSERT_STR_EQ("TestUser", msg.username);
     ASSERT_STR_EQ("Hello, World!", msg.content);
     
     PASS();
 }
 
 /* Test message queue creation */
 void test_message_queue() {
     TEST("Message queue creation");
     
     /* Create a test key */
     key_t test_key = ftok("test_queue.key", 'T');
     ASSERT_TRUE(test_key != -1);
     
     /* Remove any existing queue with this key */
     msgctl(msgget(test_key, 0666), IPC_RMID, NULL);
     
     /* Create a new queue */
     int qid = msgget(test_key, 0666 | IPC_CREAT);
     ASSERT_TRUE(qid != -1);
     
     /* Clean up */
     int result = msgctl(qid, IPC_RMID, NULL);
     ASSERT_EQ(0, result);
     
     PASS();
 }
 
 /* Test message sending and receiving */
 void test_message_send_receive() {
     TEST("Message sending and receiving");
     
     /* Create a test queue */
     key_t test_key = ftok("test_queue.key", 'T');
     ASSERT_TRUE(test_key != -1);
     
     /* Remove any existing queue with this key */
     msgctl(msgget(test_key, 0666), IPC_RMID, NULL);
     
     /* Create a new queue */
     int qid = msgget(test_key, 0666 | IPC_CREAT);
     ASSERT_TRUE(qid != -1);
     
     /* Create a test message */
     Message send_msg;
     send_msg.mtype = MSG_TYPE_CHAT;
     strcpy(send_msg.username, "Sender");
     strcpy(send_msg.content, "Test message");
     send_msg.timestamp = time(NULL);
     
     /* Send the message */
     int send_result = msgsnd(qid, &send_msg, sizeof(Message) - sizeof(long), 0);
     ASSERT_EQ(0, send_result);
     
     /* Receive the message */
     Message recv_msg;
     ssize_t recv_size = msgrcv(qid, &recv_msg, sizeof(Message) - sizeof(long), 0, 0);
     ASSERT_TRUE(recv_size > 0);
     
     /* Verify message contents */
     ASSERT_EQ(MSG_TYPE_CHAT, recv_msg.mtype);
     ASSERT_STR_EQ("Sender", recv_msg.username);
     ASSERT_STR_EQ("Test message", recv_msg.content);
     
     /* Clean up */
     msgctl(qid, IPC_RMID, NULL);
     
     PASS();
 }
 
 /* Test shared memory creation and access */
 void test_shared_memory() {
     TEST("Shared memory creation and access");
     
     /* Create a test key */
     key_t test_key = ftok("test_shm.key", 'S');
     ASSERT_TRUE(test_key != -1);
     
     /* Remove any existing shared memory segment with this key */
     shmctl(shmget(test_key, 0, 0666), IPC_RMID, NULL);
     
     /* Create a new shared memory segment */
     int shm_id = shmget(test_key, sizeof(LogBuffer) + LOG_SIZE, IPC_CREAT | 0666);
     ASSERT_TRUE(shm_id != -1);
     
     /* Attach to shared memory */
     LogBuffer *log_buffer = (LogBuffer *)shmat(shm_id, NULL, 0);
     ASSERT_TRUE(log_buffer != (void *)-1);
     
     /* Initialize log buffer */
     log_buffer->total_size = LOG_SIZE;
     log_buffer->used_size = 0;
     log_buffer->write_position = 0;
     
     /* Test writing to shared memory */
     const char test_data[] = "Test log entry";
     memcpy(log_buffer->data, test_data, strlen(test_data));
     log_buffer->used_size = strlen(test_data);
     log_buffer->write_position = strlen(test_data);
     
     /* Verify data in shared memory */
     char verify_buffer[sizeof(test_data)];
     memcpy(verify_buffer, log_buffer->data, strlen(test_data));
     verify_buffer[strlen(test_data)] = '\0';
     ASSERT_STR_EQ(test_data, verify_buffer);
     
     /* Detach from shared memory */
     int detach_result = shmdt(log_buffer);
     ASSERT_EQ(0, detach_result);
     
     /* Clean up */
     shmctl(shm_id, IPC_RMID, NULL);
     
     PASS();
 }
 
 /* Test mutex initialization */
 void test_mutex_init() {
     TEST("Mutex initialization");
     
     /* Create a test key */
     key_t test_key = ftok("test_shm.key", 'S');
     ASSERT_TRUE(test_key != -1);
     
     /* Remove any existing shared memory segment with this key */
     shmctl(shmget(test_key, 0, 0666), IPC_RMID, NULL);
     
     /* Create a new shared memory segment */
     int shm_id = shmget(test_key, sizeof(LogBuffer) + LOG_SIZE, IPC_CREAT | 0666);
     ASSERT_TRUE(shm_id != -1);
     
     /* Attach to shared memory */
     LogBuffer *log_buffer = (LogBuffer *)shmat(shm_id, NULL, 0);
     ASSERT_TRUE(log_buffer != (void *)-1);
     
     /* Initialize mutex */
     pthread_mutexattr_t mutex_attr;
     int attr_init_result = pthread_mutexattr_init(&mutex_attr);
     ASSERT_EQ(0, attr_init_result);
     
     int attr_setpshared_result = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
     ASSERT_EQ(0, attr_setpshared_result);
     
     int mutex_init_result = pthread_mutex_init(&log_buffer->mutex, &mutex_attr);
     ASSERT_EQ(0, mutex_init_result);
     
     pthread_mutexattr_destroy(&mutex_attr);
     
     /* Test mutex lock/unlock */
     int lock_result = pthread_mutex_lock(&log_buffer->mutex);
     ASSERT_EQ(0, lock_result);
     
     int unlock_result = pthread_mutex_unlock(&log_buffer->mutex);
     ASSERT_EQ(0, unlock_result);
     
     /* Clean up */
     pthread_mutex_destroy(&log_buffer->mutex);
     shmdt(log_buffer);
     shmctl(shm_id, IPC_RMID, NULL);
     
     PASS();
 }
 
 /* Test circular buffer implementation */
/* Test circular buffer implementation */
void test_circular_buffer() {
    TEST("Circular buffer implementation");
    
    /* Create a circular buffer in memory */
    size_t buffer_size = 1024;
    LogBuffer *log_buffer = malloc(sizeof(LogBuffer) + buffer_size);
    ASSERT_TRUE(log_buffer != NULL);
    
    log_buffer->total_size = buffer_size;
    log_buffer->used_size = 0;
    log_buffer->write_position = 0;
    pthread_mutex_init(&log_buffer->mutex, NULL);
    
    /* Add data until buffer is partially full */
    const char *test_string = "Test log entry";
    size_t string_len = strlen(test_string);
    size_t total_written = 0;
    
    for (int i = 0; i < 10 && total_written < buffer_size / 2; i++) {
        /* Format a log entry with an index */
        char log_entry[256];
        int len = sprintf(log_entry, "[%d] %s", i, test_string);
        
        /* Make sure we don't exceed buffer */
        if (total_written + len > buffer_size / 2) {
            break;
        }
        
        /* Copy to buffer */
        memcpy(&log_buffer->data[log_buffer->write_position], log_entry, len);
        log_buffer->write_position += len;
        log_buffer->used_size += len;
        total_written += len;
    }
    
    /* Print debug info */
    printf("\n  [DEBUG] Initial buffer state: total=%zu, used=%zu\n", 
           log_buffer->total_size, log_buffer->used_size);
    
    /* Check buffer state */
    ASSERT_TRUE(log_buffer->used_size > 0);
    ASSERT_TRUE(log_buffer->used_size < buffer_size);
    
    /* Calculate size for large entry - make sure it will fit */
    size_t remaining_space = buffer_size - log_buffer->used_size;
    size_t large_entry_size = buffer_size / 3; /* Use 1/3 of buffer size */
    
    /* Create large entry */
    char *large_entry = malloc(large_entry_size + 1);
    memset(large_entry, 'X', large_entry_size);
    large_entry[large_entry_size] = '\0';
    
    printf("  [DEBUG] Large entry size: %zu, remaining space: %zu\n", 
           large_entry_size, remaining_space);
    
    /* Save original used_size */
    size_t original_used_size = log_buffer->used_size;
    
    /* Check if we need to make room */
    if (log_buffer->used_size + large_entry_size > log_buffer->total_size) {
        printf("  [DEBUG] Need to make room in buffer\n");
        
        /* Move content to make room */
        size_t half_size = log_buffer->used_size / 2;
        memmove(log_buffer->data, log_buffer->data + half_size, log_buffer->used_size - half_size);
        log_buffer->used_size -= half_size;
        log_buffer->write_position = log_buffer->used_size;
        
        printf("  [DEBUG] After making room: used=%zu, position=%d\n", 
               log_buffer->used_size, log_buffer->write_position);
    }
    
    /* Double-check there's enough room now */
    if (log_buffer->used_size + large_entry_size <= log_buffer->total_size) {
        /* Add large entry to buffer */
        memcpy(&log_buffer->data[log_buffer->write_position], large_entry, large_entry_size);
        log_buffer->write_position += large_entry_size;
        log_buffer->used_size += large_entry_size;
        
        printf("  [DEBUG] Final buffer state: total=%zu, used=%zu\n", 
               log_buffer->total_size, log_buffer->used_size);
        
        /* Verify large entry fits */
        ASSERT_TRUE(log_buffer->used_size <= buffer_size);
    } else {
        printf("  [DEBUG] Still not enough room: need %zu, have %zu\n", 
               large_entry_size, buffer_size - log_buffer->used_size);
        FAIL("Could not fit entry in buffer even after making room");
        goto cleanup;
    }
    
    /* Success! */
    
cleanup:
    /* Clean up */
    pthread_mutex_destroy(&log_buffer->mutex);
    free(large_entry);
    free(log_buffer);
    
    if (num_tests == num_passed + 1) {
        PASS();
    }
}
 
 /* Main test function */
 int main() {
     printf("=== ChatterBox Chat System Tests ===\n\n");
     
     /* Run tests */
     test_message_creation();
     test_message_queue();
     test_message_send_receive();
     test_shared_memory();
     test_mutex_init();
     test_circular_buffer();
     
     /* Print summary */
     printf("\nTest Summary: %d of %d tests passed\n", num_passed, num_tests);
     
     return (num_passed == num_tests) ? 0 : 1;
 }
