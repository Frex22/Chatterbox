CC = gcc
CFLAGS = -Wall -g -pthread
LDFLAGS = -lrt -pthread

all: server client

server: chat_server.c
	$(CC) $(CFLAGS) -o chat_server chat_server.c $(LDFLAGS)

client: chat_client.c
	$(CC) $(CFLAGS) -o chat_client chat_client.c $(LDFLAGS)

clean:
	rm -f chat_server chat_client *.o

.PHONY: all clean


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

