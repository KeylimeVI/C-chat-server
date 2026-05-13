# CSC209 Project Documentation: Chat Server System

Noa Higuchi

## 5.1 Project Overview

My project implements a multi-user chat server (Category 2). The system consists of a central server that handles multiple concurrent client connections using TCP sockets and the `select()` system call for non-blocking I/O.

### Key Features:

- **Multi-user chat**: Multiple clients can connect at the same time and exchange messages  
- **Channel-based communication**: Users can create and join different chat channels  
- **User authentication**: Clients must join with a unique username  
- **Color customization**: Users can change their display color  
- **Message history**: Server maintains recent message history per channel  
- **Graceful disconnection**: Clean shutdown handling for both clients and server

### User Interaction:

Users interact with the system through the terminal. After starting the client program, users can:

1. Join the chat with a username using `/join <username>` (see `client_run()` in `src/client.c:512-525`)  
2. Send text messages with the Enter key (handled by `process_user_input()` in `src/client.c`)  
3. Manage channels with `/create`, `/channel`, and `/list` commands (implemented in `handle_channel_*()` functions in `src/server.c`)  
4. Change their username’s color with `/color <color>` (see `handle_color_message()` in `src/server.c`)  
5. Exit the program with `/quit` (calls `send_leave_message()` in `src/client.c`)

### Input/Output:

- **Server**: Listens on TCP port 4242 by default (configurable), accepts connections, routes messages  
- **Client**: Connects to server, provides command-line interface for users  
- **Protocol**: Binary message format with headers and structured payloads

## 5.2 Build Instructions

### Prerequisites:

- GCC compiler  
- Standard C library  
- Unix-like operating system

### Build Commands:

make

Or build with a specific port:

PORT=9999 make

### Running the Project:

Terminal 1: Start the server

./server  
(defaults to 4242 but can change port with the \-p flag)

Terminal 2: Start a client (repeat for additional clients)

./client  
(optional flags: \-h \<host-ip\> \-p \<port\>)

### Command Line Arguments:

**Server:** (parsed in `src/main_server.c:main()`)

- `-p PORT` \- Port number to listen on (default: 4242\)  
- `-h` \- Show help message

**Client:** (parsed in `src/main_client.c:main()`)

- `-h HOST` \- Server hostname or IP address (default: localhost)  
- `-p PORT` \- Server port number (default: 4242\)  
- `-?` \- Show help message

### Client Commands (after starting):

- `/join <username>` \- Join the chat with specified username (calls `send_join_message()` in `src/client.c`, joins the channel general by default)  
- `/channel <name>` \- Join an existing channel (sends `MSG_TYPE_CHANNEL_JOIN`)  
- `/create <name>` \- Create and join a new channel (sends `MSG_TYPE_CHANNEL_CREATE`)  
- `/color <color>` \- Change username color (red, orange, yellow, green, turquoise, blue, purple, white) (sends `MSG_TYPE_COLOR`)  
- `/list` \- List all available channels (sends `MSG_TYPE_CHANNEL_LIST`)  
- `/quit` \- Exit the client (calls `send_leave_message()`)  
- `[any text]` \- Send a chat message to current channel (calls `send_chat_message()`)

## 5.3 Architecture Diagram

![][image1]

**Process Relationships:**

- **Server Process**: Single process that listens on TCP port 4242, accepts client connections, and routes messages between clients in the same channels.  
- **Client Processes**: Multiple independent processes that connect to the server, authenticate with usernames, and exchange messages through the server.  
- **Relationship**: All clients connect to the single server. Clients do not connect directly to each other. The server acts as a message router.

**Socket Connections:**

- **Type**: TCP sockets (data order and correctness guaranteed unlike UDP)  
- **Port**: 4242 (configurable via Makefile PORT variable)  
- **Persistence**: Each client maintains one persistent TCP connection to the server for the duration of its session.  
- **Concurrency**: Server handles multiple concurrent connections using select() system call.

**Data Flow Through Connections:** Each TCP socket carries message types bidirectionally between client and server, including authentication, chat messages, channel management, system notifications, and error messages.

## 5.4 Communication Protocol

All messages follow a binary format with an 8-byte header followed by a variable-length payload (defined in `include/protocol.h`):

typedef struct {

    uint32\_t type;    // Message type (MSG\_TYPE\_\*)

    uint32\_t length;  // Payload length in bytes

} message\_header\_t;

### Message Types and Specifications: 

### 1\. JOIN (Client → Server)

- **Data**: `username` (64 bytes)  
- **Semantics**: Join chat with username → added to "general"  
- **Error**: Duplicate username → ERROR

### 2\. CHAT (↔ Bidirectional)

- **Data**: `channel` (64) \+ `sender` (32) \+ `content` (1024) \= 1120 bytes  
- **Semantics**: Client→Server \= send to channel; Server→Clients \= broadcast (excluding sender)  
- **Error**: Unauthenticated → ERROR

### 3\. SERVER (Server → Client)

- **Data**: `message` (1024 bytes)  
- **Semantics**: System notifications (welcome, join/leave announcements)  
- **Error**: None (informational)

### 4\. ERROR (Server → Client)

- **Data**: `error_msg` (1024 bytes)  
- **Semantics**: Error notification  
- **Error**: Client displays to user

### 5\. ACK (Server → Client)

- **Data**: None (0 bytes)  
- **Semantics**: Success confirmation  
- **Error**: None

### 6\. CHANNEL\_JOIN (Client → Server)

- **Data**: `channel_name` (32 bytes)  
- **Semantics**: Request to join channel  
- **Error**: Invalid channel → ERROR

### 7\. CHANNEL\_CREATE (Client → Server)

- **Data**: `channel_name` (32 bytes)  
- **Semantics**: Create new channel and join it  
- **Error**: Duplicate name → ERROR

### 8\. CHANNEL\_LIST (Client → Server)

- **Data**: None  
- **Semantics**: Request channel list  
- **Error**: None (always succeeds)

### 9\. COLOR (Client → Server)

- **Data**: `username` (64) \+ `color` (32) \= 96 bytes  
- **Semantics**: Change display color → server updates and notifies channel  
- **Error**: Invalid color → ERROR

### 10\. LEAVE (Client → Server)

- **Data**: None  
- **Semantics**: Graceful disconnect → server removes client and notifies channel  
- **Error**: None

## 5.5 Concurrency Model

### Server Concurrency:

The server uses a single-threaded event loop with `select()` to handle multiple clients concurrently:

1. **File Descriptor Management:**  
     
   - `server_fd`: Listening socket for new connections  
   - `client_fds[]`: One socket per connected client  
   - `master_set`: fd\_set containing all monitored file descriptors

   

2. **Event Loop (`server_run()` in `src/server.c:915-940`):**  
     
   while (1) {  
     
       read\_fds \= state-\>master\_set;  // Copy fd\_set  
     
       select(state-\>max\_fd \+ 1, \&read\_fds, NULL, NULL, NULL);  
     
         
     
       for (fd \= 0; fd \<= state-\>max\_fd; fd++) {  
     
           if (FD\_ISSET(fd, \&read\_fds)) {  
     
               if (fd \== state-\>server\_fd) {  
     
                   handle\_new\_connection(state);  // New client  
     
               } else {  
     
                   handle\_client\_message(state, fd);  // Existing client  
     
               }  
     
           }  
     
       }  
     
   }  
     
   **English description:**  
     
1. Copy the master file descriptor set  
2. Wait for activity on any socket using `select()`  
3. Check each socket for activity  
4. If listening socket: accept new client connection  
5. If client socket: read and process client message  
6. Repeat

**Key point:** Single thread handles all clients concurrently via event-driven I/O.

3. **Non-blocking Design:**  
     
   - Server never blocks on any single client operation  
   - All socket operations are non-blocking via `select()`  
   - Message processing is atomic inside the event loop

### Client Concurrency:

Each client uses `select()` to handle both user input and server messages concurrently:

1. **File Descriptor Monitoring:**  
     
   - `STDIN_FILENO` (0): User input from terminal  
   - `sockfd`: Server socket for incoming messages

   

2. **Race Condition Prevention:**  
     
   - User input processed before server messages in each loop  
   - Line clearing (`\r\x1b[2K`) prevents duplicate message display  
   - Atomic message display ensures clean output

## 5.6 Error Handling and Robustness

### 1\. Client Disconnection Mid-Message

**Scenario:** Client disconnects unexpectedly while server is reading a message.  
**Handling:** `receive_message_header()` or `receive_message_data()` (in `src/protocol.c`) returns \-1 or 0\. Server detects EOF/error in `handle_client_message()` (line \~ in `src/server.c`), calls `client_remove()` to clean up resources, and broadcasts leave notification to channel.

**Code Reference:** `handle_client_message()` in `src/server.c` checks return values from receive functions and calls `client_remove()` on error.

### 2\. Malformed Command from Client

**Scenario:** Client sends invalid command or malformed message.  
**Handling:** Server validates message structure in `handle_client_message()` (in `src/server.c`) and command syntax. Invalid requests receive `MSG_TYPE_ERROR` response via `send_error_message()`. Client remains connected.

**Code Reference:** Command parsing in `process_user_input()` (in `src/client.c`) and message validation in `handle_client_message()` (in `src/server.c`).

### 3\. Network Failure During Broadcast

**Scenario:** Server tries to broadcast to a client that has disconnected.  
**Handling:** `send_message()` (in `src/protocol.c`) returns \-1 on write error. Server detects failed send in `channel_broadcast()` (in `src/server.c`), calls `client_remove()` for that client, and continues broadcasting to remaining clients.

**Code Reference:** `channel_broadcast()` in `src/server.c` checks `send_message()` return value and handles errors.

## 5.7 Team Contributions

Only me. AI was partially used for this report, and in some parts of the code as specified in comments.

[image1]: # CSC209 Project Documentation: Chat Server System

Noa Higuchi

## 5.1 Project Overview

My project implements a multi-user chat server (Category 2). The system consists of a central server that handles multiple concurrent client connections using TCP sockets and the `select()` system call for non-blocking I/O.

### Key Features:

- **Multi-user chat**: Multiple clients can connect at the same time and exchange messages  
- **Channel-based communication**: Users can create and join different chat channels  
- **User authentication**: Clients must join with a unique username  
- **Color customization**: Users can change their display color  
- **Message history**: Server maintains recent message history per channel  
- **Graceful disconnection**: Clean shutdown handling for both clients and server

### User Interaction:

Users interact with the system through the terminal. After starting the client program, users can:

1. Join the chat with a username using `/join <username>` (see `client_run()` in `src/client.c:512-525`)  
2. Send text messages with the Enter key (handled by `process_user_input()` in `src/client.c`)  
3. Manage channels with `/create`, `/channel`, and `/list` commands (implemented in `handle_channel_*()` functions in `src/server.c`)  
4. Change their username’s color with `/color <color>` (see `handle_color_message()` in `src/server.c`)  
5. Exit the program with `/quit` (calls `send_leave_message()` in `src/client.c`)

### Input/Output:

- **Server**: Listens on TCP port 4242 by default (configurable), accepts connections, routes messages  
- **Client**: Connects to server, provides command-line interface for users  
- **Protocol**: Binary message format with headers and structured payloads

## 5.2 Build Instructions

### Prerequisites:

- GCC compiler  
- Standard C library  
- Unix-like operating system

### Build Commands:

make

Or build with a specific port:

PORT=9999 make

### Running the Project:

Terminal 1: Start the server

./server  
(defaults to 4242 but can change port with the \-p flag)

Terminal 2: Start a client (repeat for additional clients)

./client  
(optional flags: \-h \<host-ip\> \-p \<port\>)

### Command Line Arguments:

**Server:** (parsed in `src/main_server.c:main()`)

- `-p PORT` \- Port number to listen on (default: 4242\)  
- `-h` \- Show help message

**Client:** (parsed in `src/main_client.c:main()`)

- `-h HOST` \- Server hostname or IP address (default: localhost)  
- `-p PORT` \- Server port number (default: 4242\)  
- `-?` \- Show help message

### Client Commands (after starting):

- `/join <username>` \- Join the chat with specified username (calls `send_join_message()` in `src/client.c`, joins the channel general by default)  
- `/channel <name>` \- Join an existing channel (sends `MSG_TYPE_CHANNEL_JOIN`)  
- `/create <name>` \- Create and join a new channel (sends `MSG_TYPE_CHANNEL_CREATE`)  
- `/color <color>` \- Change username color (red, orange, yellow, green, turquoise, blue, purple, white) (sends `MSG_TYPE_COLOR`)  
- `/list` \- List all available channels (sends `MSG_TYPE_CHANNEL_LIST`)  
- `/quit` \- Exit the client (calls `send_leave_message()`)  
- `[any text]` \- Send a chat message to current channel (calls `send_chat_message()`)

## 5.3 Architecture Diagram

![][image1]

**Process Relationships:**

- **Server Process**: Single process that listens on TCP port 4242, accepts client connections, and routes messages between clients in the same channels.  
- **Client Processes**: Multiple independent processes that connect to the server, authenticate with usernames, and exchange messages through the server.  
- **Relationship**: All clients connect to the single server. Clients do not connect directly to each other. The server acts as a message router.

**Socket Connections:**

- **Type**: TCP sockets (data order and correctness guaranteed unlike UDP)  
- **Port**: 4242 (configurable via Makefile PORT variable)  
- **Persistence**: Each client maintains one persistent TCP connection to the server for the duration of its session.  
- **Concurrency**: Server handles multiple concurrent connections using select() system call.

**Data Flow Through Connections:** Each TCP socket carries message types bidirectionally between client and server, including authentication, chat messages, channel management, system notifications, and error messages.

## 5.4 Communication Protocol

All messages follow a binary format with an 8-byte header followed by a variable-length payload (defined in `include/protocol.h`):

typedef struct {

    uint32\_t type;    // Message type (MSG\_TYPE\_\*)

    uint32\_t length;  // Payload length in bytes

} message\_header\_t;

### Message Types and Specifications: 

### 1\. JOIN (Client → Server)

- **Data**: `username` (64 bytes)  
- **Semantics**: Join chat with username → added to "general"  
- **Error**: Duplicate username → ERROR

### 2\. CHAT (↔ Bidirectional)

- **Data**: `channel` (64) \+ `sender` (32) \+ `content` (1024) \= 1120 bytes  
- **Semantics**: Client→Server \= send to channel; Server→Clients \= broadcast (excluding sender)  
- **Error**: Unauthenticated → ERROR

### 3\. SERVER (Server → Client)

- **Data**: `message` (1024 bytes)  
- **Semantics**: System notifications (welcome, join/leave announcements)  
- **Error**: None (informational)

### 4\. ERROR (Server → Client)

- **Data**: `error_msg` (1024 bytes)  
- **Semantics**: Error notification  
- **Error**: Client displays to user

### 5\. ACK (Server → Client)

- **Data**: None (0 bytes)  
- **Semantics**: Success confirmation  
- **Error**: None

### 6\. CHANNEL\_JOIN (Client → Server)

- **Data**: `channel_name` (32 bytes)  
- **Semantics**: Request to join channel  
- **Error**: Invalid channel → ERROR

### 7\. CHANNEL\_CREATE (Client → Server)

- **Data**: `channel_name` (32 bytes)  
- **Semantics**: Create new channel and join it  
- **Error**: Duplicate name → ERROR

### 8\. CHANNEL\_LIST (Client → Server)

- **Data**: None  
- **Semantics**: Request channel list  
- **Error**: None (always succeeds)

### 9\. COLOR (Client → Server)

- **Data**: `username` (64) \+ `color` (32) \= 96 bytes  
- **Semantics**: Change display color → server updates and notifies channel  
- **Error**: Invalid color → ERROR

### 10\. LEAVE (Client → Server)

- **Data**: None  
- **Semantics**: Graceful disconnect → server removes client and notifies channel  
- **Error**: None

## 5.5 Concurrency Model

### Server Concurrency:

The server uses a single-threaded event loop with `select()` to handle multiple clients concurrently:

1. **File Descriptor Management:**  
     
   - `server_fd`: Listening socket for new connections  
   - `client_fds[]`: One socket per connected client  
   - `master_set`: fd\_set containing all monitored file descriptors

   

2. **Event Loop (`server_run()` in `src/server.c:915-940`):**  
     
   while (1) {  
     
       read\_fds \= state-\>master\_set;  // Copy fd\_set  
     
       select(state-\>max\_fd \+ 1, \&read\_fds, NULL, NULL, NULL);  
     
         
     
       for (fd \= 0; fd \<= state-\>max\_fd; fd++) {  
     
           if (FD\_ISSET(fd, \&read\_fds)) {  
     
               if (fd \== state-\>server\_fd) {  
     
                   handle\_new\_connection(state);  // New client  
     
               } else {  
     
                   handle\_client\_message(state, fd);  // Existing client  
     
               }  
     
           }  
     
       }  
     
   }  
     
   **English description:**  
     
1. Copy the master file descriptor set  
2. Wait for activity on any socket using `select()`  
3. Check each socket for activity  
4. If listening socket: accept new client connection  
5. If client socket: read and process client message  
6. Repeat

**Key point:** Single thread handles all clients concurrently via event-driven I/O.

3. **Non-blocking Design:**  
     
   - Server never blocks on any single client operation  
   - All socket operations are non-blocking via `select()`  
   - Message processing is atomic inside the event loop

### Client Concurrency:

Each client uses `select()` to handle both user input and server messages concurrently:

1. **File Descriptor Monitoring:**  
     
   - `STDIN_FILENO` (0): User input from terminal  
   - `sockfd`: Server socket for incoming messages

   

2. **Race Condition Prevention:**  
     
   - User input processed before server messages in each loop  
   - Line clearing (`\r\x1b[2K`) prevents duplicate message display  
   - Atomic message display ensures clean output

## 5.6 Error Handling and Robustness

### 1\. Client Disconnection Mid-Message

**Scenario:** Client disconnects unexpectedly while server is reading a message.  
**Handling:** `receive_message_header()` or `receive_message_data()` (in `src/protocol.c`) returns \-1 or 0\. Server detects EOF/error in `handle_client_message()` (line \~ in `src/server.c`), calls `client_remove()` to clean up resources, and broadcasts leave notification to channel.

**Code Reference:** `handle_client_message()` in `src/server.c` checks return values from receive functions and calls `client_remove()` on error.

### 2\. Malformed Command from Client

**Scenario:** Client sends invalid command or malformed message.  
**Handling:** Server validates message structure in `handle_client_message()` (in `src/server.c`) and command syntax. Invalid requests receive `MSG_TYPE_ERROR` response via `send_error_message()`. Client remains connected.

**Code Reference:** Command parsing in `process_user_input()` (in `src/client.c`) and message validation in `handle_client_message()` (in `src/server.c`).

### 3\. Network Failure During Broadcast

**Scenario:** Server tries to broadcast to a client that has disconnected.  
**Handling:** `send_message()` (in `src/protocol.c`) returns \-1 on write error. Server detects failed send in `channel_broadcast()` (in `src/server.c`), calls `client_remove()` for that client, and continues broadcasting to remaining clients.

**Code Reference:** `channel_broadcast()` in `src/server.c` checks `send_message()` return value and handles errors.

## 5.7 Team Contributions

Only me. AI was partially used for this report, and in some parts of the code as specified in comments.

[image1]: <data:image/png;base64,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eVdVv6n83nvvc213vZx++mly0EHtZfvtt09qflWZbG2QovwrXbqULF++zGu5br31Vhk16jPZZZedpWfPE+WYY46WffaJka5169YXpZiWtiGQDQEjWtngKL4PGzZskGrVqrsOqaff2N60aVP58ccfZPDg132nxds/fGuQQao4oUB+xo8fL19//XW2bLHk8MwzA4WjKLxDMf7hDdHKlSv7FxmKcTazZY2l5zDYQrCOO+44ufjiC512s5q4ppDNb3F4UFX566+/5O2333aTh3dl5513lrPPPlNq1art8lv45AhyX7FiRSkoQ32mT/vll1/kvffedxPED4SJU8uWe8mJJ57gtPT7emJdUOlbvAVXtkWN7XbbbecnKMnsF4xoJRPNFIuLzo5K06RJYznnnLOlf//LpVy5snL//Q/Kc88974jWJE800HKlmOhJEUdVhfxz1hdHUyxduszHSwc9ZswXMnbseP9cnH/KlCktZ5xxupDfRYsWF+esZssbJGuD02oFS96g3XPPPd1y2o3C9zeDfXG7QjA5qZ1JBMuI/fpdJocccrBUqFDea24LK7/lypWTxo0bFoqpX39HycysIosXL/ITqi+//MJpvZbLTjvtUCjpF1Y+LZ3CqU/Um7Jlk/viR16JVmG1V0tnGxHgqAUIFp+66NbtKP+K9IEHHigTJkyQ2267wxMsOqTSTgW/jVGnlXdw2G+//YTN/ezfYFnl7bff8YPNvHnz5aOPPpZSpYp39d/gVDdt27b1Gg20esU9v9EKCuGIn0DwDT7eYsMt6rc43lP/2Qbw8MOP+q0BvXr1ErQ92BdmfqmDhWnIm6r6ds59YaZtaW3wfU2640C9SbYp3iNNstFK4fio3MzaO3XqJFdc0V+OPLKrzJw5yy8LvvTSy26m941wLENGRvEucnCoU6eO9Ot3ubRqtY//zuKQIa/LH3/8IcOGDZNp06b7ZYbp06encGkmR7SqVTNl//3b+UNlSwK5iKIGyaIuYMc92p177rlf2CS/YMECrEuEmTlzpn9j79NPP5MuXbrIhRdeIDVq1CimebdsGQKpiUDxHnVTE/OkSaWqbimwnNSqVUuOPvpotyxyk7RosYe8+eab8r//3eaWxsbKwoULZfXq1UlLM5UjYrmCww3PP/88mTLlF3nsscflt99+EwZa5GY2f9NNN8v773/gN0pjV1yNqrq6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>
