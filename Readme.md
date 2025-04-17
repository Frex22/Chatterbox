# 🗣️ ChatterBox: A Multi-User Chat System

## What is ChatterBox?

ChatterBox is a terminal-based multi-user chat application built using System V IPC mechanisms and POSIX threads. It's a perfect example of how operating system principles can be applied to create practical software!

## 🧠 OS Concepts Covered

- **Process Communication** using System V message queues
- **Shared Memory** using System V shared memory segments for chat logs
- **Multi-threading** with POSIX threads for concurrent operations
- **Thread Synchronization** using POSIX mutex for shared memory access
- **Signal Handling** for graceful shutdown and thread coordination
- **Resource Management** with proper cleanup procedures
- **Non-blocking I/O** with IPC_NOWAIT flag for robust communication

## 🚀 Getting Started

### Prerequisites

- GCC compiler
- Linux/Unix-based operating system (tested on Ubuntu)
- Make utility

### Building the Application

```bash
# Clone the repository
git clone https://github.com/Frex22/OS-Project-DAK.git
cd OS-Project-DAK
# Build the server and client
make all
```

### Running ChatterBox

#### 1. Start the Server

```bash
./chat_server
```

The server will start and display a prompt where you can enter commands:
- `list` - Show all connected clients
- `quit` - Shutdown the server

#### 2. Connect Clients

In separate terminal windows, start one or more clients:

```bash
./chat_client YourUsername
```

Replace `YourUsername` with your desired chat name.

#### 3. Chat Commands

Once connected, you can:
- Type any message to chat with everyone
- Type `logs` to view chat history
- Type `quit` to disconnect

## 🎮  Features

- 👥 Connect with multiple users simultaneously
- 📜 View chat history even if you just joined
- 🔔 Real-time message delivery
- ⏱️ Timestamps on all messages
- 🔐 System handles crashes gracefully
- 🌐 Distributed architecture that can run across multiple machines

## 🏗️ Architecture

```
┌─────────────┐        ┌──────────────┐
│  Client 1   │◄─────► │              │
└─────────────┘        │              │
                       │    Server    │◄────► Shared Memory (Logs)
┌─────────────┐        │              │
│  Client 2   │◄─────► │              │
└─────────────┘        └──────────────┘
       ...                    ▲
                              │
┌─────────────┐               │
│  Client N   │◄──────────────┘
└─────────────┘
```

- **Server**: Manages client connections and broadcasts messages
- **Clients**: Send and receive messages via their own message queues
- **Shared Memory**: Stores chat history accessible to all clients

## 🛠️ Technical Deep Dive

### System V IPC Mechanisms

ChatterBox leverages two key System V IPC mechanisms:

1. **Message Queues**:
   - Server queue (central communication point)
   - Individual client queues (one per connected client)
   - Message types for different operations (connect, disconnect, chat)
   - Non-blocking operations using IPC_NOWAIT flag

2. **Shared Memory**:
   - 1MB shared memory segment for storing chat logs
   - Flexible array member for dynamic buffer allocation
   - Circular buffer implementation to manage memory usage
   - Process-shared mutex for synchronization

### POSIX Threads Implementation

The application uses multiple threads with specific responsibilities:

**Server Threads**:
- **Message Receiver Thread**: Handles incoming messages from all clients
- **Log Sync Thread**: Periodically writes chat logs to disk

**Client Thread**:
- **Message Receiver Thread**: Processes incoming messages in the background

### Synchronization Mechanisms

- **Process-shared POSIX Mutex**: Protects the shared log buffer from concurrent access
- **Thread Signaling**: Uses SIGUSR1 signals to wake up blocked threads during shutdown
- **Atomic Flag**: The `running` variable coordinates thread shutdown

### Error Handling and Recovery

- Non-blocking operations with proper error recovery
- Error classification (ENOMSG, EINTR, EIDRM, EINVAL)
- Graceful shutdown with resource cleanup sequence
- Client reconnection detection and management

## 📝 Lessons Learned

Building this chat system demonstrates several important concepts:
- The power of IPC mechanisms for building distributed applications
- The importance of proper resource management
- How multi-threading can improve application responsiveness
- Techniques for robust error handling and recovery

## 📚 References

- [POSIX Threads Programming](https://computing.llnl.gov/tutorials/pthreads/)
- [System V IPC Mechanisms](https://tldp.org/LDP/lpg/node7.html)
- [Linux Shared Memory](https://www.geeksforgeeks.org/ipc-shared-memory/)

## Personell
Implemnted coded and created by
Aakash: Ideation, Threads, Client logic
Dev: Server, Syncronization, Cleanup Procedures
Kush: Server, Shared Memory
---

Created with 💖 for Operating Systems enthusiasts everywhere!
