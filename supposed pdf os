Below is the revised project description with an added aim and a clear one-line statement of the problem to be solved.

---

# **1. Topic**  
**IPC-Driven Chat Application**

---

# **2. Group Members**  
- Aakash Singh
- Dev Trivedi
- Kush Yarwar 

---

# **3. Rationale**  

**Aim:**  
The aim of this project is to design and implement a robust multi-process chat application that efficiently manages concurrent communications while demonstrating key operating system principles using C.

**Problem Statement:**  
This project aims to solve the problem of coordinating real-time communication among multiple processes in a concurrent system, ensuring data consistency and persistence.

**Motivation:**  
The motivation behind this project is to develop a multi-process chat application that not only allows real-time messaging between clients but also serves as an educational tool to demonstrate core operating system concepts. In particular, our project emphasizes:

- **IPC Mechanisms:** We utilize message queues to facilitate asynchronous communication between processes. This allows for efficient message passing between the server and multiple clients.
- **Synchronization:** With multiple processes potentially accessing a shared chat log simultaneously, we implement synchronization primitives (such as POSIX semaphores) to prevent race conditions and ensure data consistency.
- **Memory Management (Virtual Memory):** By leveraging `mmap`, we persist the chat history to a file that is memory-mapped. This integration of virtual memory concepts demonstrates how large or persistent data can be managed efficiently by the operating system.

This project addresses the challenge of coordinating concurrent processes in a real-time system, showcasing both interprocess communication and memory management techniques in Linux.

---

# **4. What Functions the Program Provides**  

- **Real-Time Messaging:**  
  Clients can send and receive messages through a central server using System V message queues.
  
- **Shared Chat Log:**  
  The server maintains a live chat log stored in shared memory, which all clients can access. This log is updated in real time as new messages are received.
  
- **Synchronization of Access:**  
  To avoid conflicts when multiple processes access the shared chat log, synchronization mechanisms (POSIX semaphores) ensure that only one process can write to or read from the critical sections at a time.
  
- **Persistent Chat History:**  
  Utilizing virtual memory via `mmap`, the chat log is backed by a file, ensuring that the conversation history is preserved even if the application is restarted.
  
- **Scalability:**  
  The architecture supports multiple clients, simulating a chat room environment, and demonstrates how a system can be designed for concurrent process communication.

---

# **5. Project Plan**

**Week 1: Project Setup & Design**  
- Finalize project requirements and design the system architecture.  
- Set up the development environment on a Linux machine.  
- Define the data structures for messages and the shared chat log.

**Week 2: Message Queue Implementation**  
- Develop and test a simple demo to send and receive messages using System V message queues.  
- Integrate `ftok` to dynamically generate keys for IPC.

**Week 3: Shared Memory Integration**  
- Create a shared memory segment to hold the chat log.  
- Develop routines for attaching, writing to, and reading from the shared memory in separate processes.

**Week 4: Synchronization Mechanisms**  
- Integrate POSIX semaphores to control access to the shared memory.  
- Test for race conditions and ensure data consistency under concurrent access.

**Week 5: Virtual Memory for Persistent Storage**  
- Implement `mmap` to create a persistent, file-backed chat log.  
- Integrate this with the existing shared memory mechanism and test for persistence across process restarts.

**Week 6: Integration, Testing & Documentation**  
- Merge all components into a cohesive chat application.  
- Conduct extensive multi-process testing to ensure robust performance.  
- Finalize documentation, user guides, and prepare the project for submission.

---

This project plan ensures that by the end of the six-week period, a fully functional, real-time chat application will be developed, integrating key OS topics such as IPC mechanisms, synchronization, and memory management through virtual memory techniques.
