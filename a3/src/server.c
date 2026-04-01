#include "server.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

// Global server state for signal handler
static server_state_t *g_server_state = NULL;

// Signal handler for graceful shutdown
static void signal_handler(int sig) {
    if (g_server_state != NULL) {
        printf("\nReceived signal %d, shutting down server...\n", sig);
        server_cleanup(g_server_state);
        exit(0);
    }
}

// Initialize server socket and state
int server_init(server_state_t *state, int port) {
    int opt = 1;
    struct sockaddr_in server_addr;
    
    // Initialize state
    memset(state, 0, sizeof(server_state_t));
    state->port = port;
    state->clients = NULL;
    state->max_fd = 0;
    
    // Create server socket
    state->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (state->server_fd < 0) {
        perror("socket");
        return -1;
    }
    
    // Set socket options to reuse address
    if (setsockopt(state->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(state->server_fd);
        return -1;
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind socket
    if (bind(state->server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(state->server_fd);
        return -1;
    }
    
    // Listen for connections
    if (listen(state->server_fd, 10) < 0) {
        perror("listen");
        close(state->server_fd);
        return -1;
    }
    
    // Initialize file descriptor sets
    FD_ZERO(&state->master_set);
    FD_SET(state->server_fd, &state->master_set);
    state->max_fd = state->server_fd;
    
    // Set up signal handler for graceful shutdown
    g_server_state = state;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Server initialized on port %d\n", port);
    return 0;
}

// Clean up server resources
void server_cleanup(server_state_t *state) {
    client_t *client, *next;
    
    printf("Cleaning up server resources...\n");
    
    // Close all client connections
    for (client = state->clients; client != NULL; client = next) {
        next = client->next;
        close(client->fd);
        free(client);
    }
    state->clients = NULL;
    
    // Close server socket
    if (state->server_fd >= 0) {
        close(state->server_fd);
        state->server_fd = -1;
    }
    
    printf("Server cleanup complete\n");
}

// Add a new client to the server
client_t *client_add(server_state_t *state, int client_fd) {
    client_t *new_client = malloc(sizeof(client_t));
    if (new_client == NULL) {
        perror("malloc");
        return NULL;
    }
    
    // Initialize client
    memset(new_client, 0, sizeof(client_t));
    new_client->fd = client_fd;
    new_client->username[0] = '\0';
    new_client->authenticated = 0;
    
    // Add to linked list
    new_client->next = state->clients;
    state->clients = new_client;
    
    // Add to master set and update max_fd
    FD_SET(client_fd, &state->master_set);
    if (client_fd > state->max_fd) {
        state->max_fd = client_fd;
    }
    
    printf("New client connected (fd=%d)\n", client_fd);
    return new_client;
}

// Remove a client from the server
void client_remove(server_state_t *state, int client_fd) {
    client_t *client, *prev = NULL;
    
    for (client = state->clients; client != NULL; prev = client, client = client->next) {
        if (client->fd == client_fd) {
            // Remove from linked list
            if (prev == NULL) {
                state->clients = client->next;
            } else {
                prev->next = client->next;
            }
            
            // Remove from master set
            FD_CLR(client_fd, &state->master_set);
            
            // Close socket
            close(client_fd);
            
            // Notify other clients if authenticated
            if (client->authenticated && strlen(client->username) > 0) {
                char notification[MAX_MESSAGE_LEN];
                snprintf(notification, sizeof(notification), 
                        "*** %s has left the chat ***", client->username);
                broadcast_message(state, "SERVER", notification);
            }
            
            printf("Client disconnected (fd=%d, username=%s)\n", 
                   client_fd, client->username[0] ? client->username : "<none>");
            
            free(client);
            break;
        }
    }
}

// Find client by file descriptor
client_t *client_find_by_fd(server_state_t *state, int fd) {
    client_t *client;
    
    for (client = state->clients; client != NULL; client = client->next) {
        if (client->fd == fd) {
            return client;
        }
    }
    
    return NULL;
}

// Find client by username
client_t *client_find_by_username(server_state_t *state, const char *username) {
    client_t *client;
    
    for (client = state->clients; client != NULL; client = client->next) {
        if (client->authenticated && strcmp(client->username, username) == 0) {
            return client;
        }
    }
    
    return NULL;
}

// Handle new incoming connection
void handle_new_connection(server_state_t *state) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd;
    
    // Accept new connection
    client_fd = accept(state->server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        perror("accept");
        return;
    }
    
    // Add new client
    if (client_add(state, client_fd) == NULL) {
        close(client_fd);
        return;
    }
    
    // Send welcome message
    send_server_message(client_fd, "Welcome to the chat server! Please join with /join <username>");
    
    printf("New connection from %s:%d (fd=%d)\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);
}

// Handle JOIN message from client
static void handle_join_message(server_state_t *state, client_t *client, const join_data_t *join_data) {
    // Check if username is already taken
    if (client_find_by_username(state, join_data->username) != NULL) {
        send_error_message(client->fd, "Username already taken");
        return;
    }
    
    // Set username and mark as authenticated
    strncpy(client->username, join_data->username, MAX_USERNAME_LEN - 1);
    client->username[MAX_USERNAME_LEN - 1] = '\0';
    client->authenticated = 1;
    
    // Send acknowledgment
    send_server_message(client->fd, "Joined successfully!");
    send_message(client->fd, MSG_TYPE_ACK, NULL, 0);
    
    // Notify all clients
    char notification[MAX_MESSAGE_LEN];
    snprintf(notification, sizeof(notification), 
            "*** %s has joined the chat ***", client->username);
    broadcast_message(state, "SERVER", notification);
    
    printf("Client authenticated (fd=%d, username=%s)\n", client->fd, client->username);
}

// Handle CHAT message from client
static void handle_chat_message(server_state_t *state, client_t *client, const chat_data_t *chat_data) {
    // Check if client is authenticated
    if (!client->authenticated) {
        send_error_message(client->fd, "You must join first with /join <username>");
        return;
    }
    
    // Broadcast message to all clients
    broadcast_message(state, client->username, chat_data->message);
    
    printf("Message from %s: %s\n", client->username, chat_data->message);
}

// Handle client message
void handle_client_message(server_state_t *state, int client_fd) {
    client_t *client = client_find_by_fd(state, client_fd);
    message_header_t header;
    
    if (client == NULL) {
        // Client not found, remove it
        client_remove(state, client_fd);
        return;
    }
    
    // Read message header
    if (receive_message_header(client_fd, &header) < 0) {
        // Connection closed or error
        client_remove(state, client_fd);
        return;
    }
    
    // Handle based on message type
    switch (header.type) {
        case MSG_TYPE_JOIN: {
            if (header.length != sizeof(join_data_t)) {
                send_error_message(client_fd, "Invalid JOIN message format");
                break;
            }
            
            join_data_t join_data;
            if (receive_message_data(client_fd, &join_data, sizeof(join_data)) < 0) {
                client_remove(state, client_fd);
                return;
            }
            
            handle_join_message(state, client, &join_data);
            break;
        }
        
        case MSG_TYPE_CHAT: {
            if (header.length != sizeof(chat_data_t)) {
                send_error_message(client_fd, "Invalid CHAT message format");
                break;
            }
            
            chat_data_t chat_data;
            if (receive_message_data(client_fd, &chat_data, sizeof(chat_data)) < 0) {
                client_remove(state, client_fd);
                return;
            }
            
            handle_chat_message(state, client, &chat_data);
            break;
        }
        
        case MSG_TYPE_LEAVE: {
            // Client wants to leave gracefully
            client_remove(state, client_fd);
            break;
        }
        
        default:
            send_error_message(client_fd, "Unknown message type");
            break;
    }
}

// Broadcast message to all connected clients
void broadcast_message(server_state_t *state, const char *username, const char *message) {
    client_t *client;
    chat_data_t chat_data;
    
    // Prepare chat message
    strncpy(chat_data.username, username, MAX_USERNAME_LEN - 1);
    chat_data.username[MAX_USERNAME_LEN - 1] = '\0';
    strncpy(chat_data.message, message, MAX_MESSAGE_LEN - 1);
    chat_data.message[MAX_MESSAGE_LEN - 1] = '\0';
    
    // Send to all authenticated clients
    for (client = state->clients; client != NULL; client = client->next) {
        if (client->authenticated) {
            send_message(client->fd, MSG_TYPE_CHAT, &chat_data, sizeof(chat_data));
        }
    }
}

// Send server message to a specific client
void send_server_message(int client_fd, const char *message) {
    server_data_t server_data;
    
    strncpy(server_data.message, message, MAX_MESSAGE_LEN - 1);
    server_data.message[MAX_MESSAGE_LEN - 1] = '\0';
    
    send_message(client_fd, MSG_TYPE_SERVER, &server_data, sizeof(server_data));
}

// Send error message to a specific client
void send_error_message(int client_fd, const char *error_message) {
    error_data_t error_data;
    
    strncpy(error_data.error_message, error_message, MAX_MESSAGE_LEN - 1);
    error_data.error_message[MAX_MESSAGE_LEN - 1] = '\0';
    
    send_message(client_fd, MSG_TYPE_ERROR, &error_data, sizeof(error_data));
}

// Main server event loop
void server_run(server_state_t *state) {
    fd_set read_fds;
    int fd;
    
    printf("Server running. Press Ctrl+C to stop.\n");
    
    while (1) {
        // Copy master set because select() modifies it
        read_fds = state->master_set;
        
        // Wait for activity on any socket
        if (select(state->max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                // Interrupted by signal, continue
                continue;
            }
            perror("select");
            break;
        }
        
        // Check all file descriptors for activity
        for (fd = 0; fd <= state->max_fd; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == state->server_fd) {
                    // New connection
                    handle_new_connection(state);
                } else {
                    // Client activity
                    handle_client_message(state, fd);
                }
            }
        }
    }
}