CC = gcc
CFLAGS = -Wall -g -pthread
LDFLAGS = -lrt -pthread

all: server client test_sys

server: chat_server.c
	$(CC) $(CFLAGS) -o chat_server chat_server.c $(LDFLAGS)

client: chat_client.c
	$(CC) $(CFLAGS) -o chat_client chat_client.c $(LDFLAGS)

test_sys: test_chat_sys.c
	$(CC) $(CFLAGS) -o test_chat_sys test_chat_sys.c $(LDFLAGS)

clean:
	rm -f chat_server chat_client test_chat_sys *.o

# RUN TESTS
run_test: test_sys
	./test_chat_sys

# MEMORY LEAK CHECK WITH VALGRIND
memcheck: chat_server chat_client
	valgrind --leak-check=full ./chat_server & \
	SERVER_PID=$$!; \
	sleep 2; \
	valgrind --leak-check=full ./chat_client testuser; \
	kill $$SERVER_PID

# Run the server with default settings
run-server: chat_server
	./chat_server

# Create IPC key files if they don't exist
setup:
	touch server.key client.key log.key test_queue.key test_shm.key

# Full cleanup including IPC resources
fullclean: clean
	ipcs -q | grep $(USER) | awk '{print $$2}' | xargs -r ipcrm -q
	ipcs -m | grep $(USER) | awk '{print $$2}' | xargs -r ipcrm -m
	rm -f chat_server.log

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build server, client and tests"
	@echo "  server       - Build only the server"
	@echo "  client       - Build only the client"
	@echo "  test_sys     - Build the test file"
	@echo "  run_test     - Run the test suite"
	@echo "  memcheck     - Check for memory leaks with Valgrind"
	@echo "  clean        - Remove built files"
	@echo "  fullclean    - Remove built files and clean up IPC resources"
	@echo "  setup        - Create necessary key files"
	@echo "  run-server   - Run the chat server"

.PHONY: all clean run_test memcheck run-server setup fullclean help test_sys


# This Makefile is used to compile the chat server and client programs.
# It uses gcc as the compiler and includes flags for warnings, debugging, and threading.
# The all target builds both the server and client executables.
# The clean target removes the compiled executables and object files.
# To use this Makefile, run 'make' to compile the programs and 'make clean' to remove the compiled files.
# The server and client source files are chat_server.c and chat_client.c respectively.
# The LDFLAGS variable includes the real-time library and pthread library for threading support.
# The CFLAGS variable includes the -Wall flag to enable all compiler warnings and the -g flag for debugging information.
# The $(CC) variable is used to specify the compiler, which is set to gcc.
# The $(CFLAGS) variable is used to specify the compiler flags, which include -Wall for warnings and -g for debugging.
# The $(LDFLAGS) variable is used to specify the linker flags, which include -lrt for real-time support and -pthread for threading support.
# The server target builds the chat_server executable from the chat_server.c source file.
# The client target builds the chat_client executable from the chat_client.c source file.
# The clean target removes the compiled executables and object files.
# The .PHONY target specifies that all and clean are not actual files, but rather commands to be executed.


