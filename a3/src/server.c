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

// Helper function to initialize a new client
static void client_init(client_t *client, int fd) {
    memset(client, 0, sizeof(client_t));
    client->fd = fd;
    client->username[0] = '\0';
    client->channel[0] = '\0';
    client->authenticated = 0;
    client->next = NULL;
    client->next_in_channel = NULL;
}

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
    state->channels = NULL;
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
    client_init(new_client, client_fd);

    // Add to linked list
    new_client->next = state->clients;
    state->clients = new_client;

    // Add to default "general" channel if it exists
    channel_t *general_channel = channel_find(state, "general");
    if (general_channel == NULL) {
        // Create default general channel
        general_channel = channel_create(state, "general");
    }
    if (general_channel != NULL) {
        channel_add_client(general_channel, new_client);
        strncpy(new_client->channel, "general", sizeof(new_client->channel) - 1);
        new_client->channel[sizeof(new_client->channel) - 1] = '\0';
    }

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
            // Remove client from current channel if in one
            if (strlen(client->channel) > 0) {
                channel_t *channel = channel_find(state, client->channel);
                if (channel != NULL) {
                    channel_remove_client(channel, client);
                }
            }

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

// Handle JOIN message from client (username join)
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

    // Notify clients in the same channel
    if (strlen(client->channel) > 0) {
        char notification[MAX_MESSAGE_LEN];
        snprintf(notification, sizeof(notification),
                "*** %s has joined the chat ***", client->username);

        channel_t *channel = channel_find(state, client->channel);
        if (channel != NULL) {
            channel_broadcast(state, channel, "SERVER", notification, -1);
        }
    }

    printf("Client authenticated (fd=%d, username=%s, channel=%s)\n",
           client->fd, client->username, client->channel);
}

// Handle CHAT message from client
static void handle_chat_message(server_state_t *state, client_t *client, const chat_data_t *chat_data) {
    // Check if client is authenticated
    if (!client->authenticated) {
        send_error_message(client->fd, "You must join first with /join <username>");
        return;
    }

    // Check if client is in a channel
    if (strlen(client->channel) == 0) {
        send_error_message(client->fd, "You must be in a channel to send messages");
        return;
    }

    // Broadcast message to clients in the same channel
    channel_t *channel = channel_find(state, client->channel);
    if (channel != NULL) {
        channel_broadcast(state, channel, client->username, chat_data->message, -1);
    }

    printf("Message from %s in channel %s: %s\n", client->username, client->channel, chat_data->message);
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

        case MSG_TYPE_CHANNEL_JOIN: {
            if (header.length != sizeof(channel_join_data_t)) {
                send_error_message(client_fd, "Invalid CHANNEL_JOIN message format");
                break;
            }

            channel_join_data_t join_data;
            if (receive_message_data(client_fd, &join_data, sizeof(join_data)) < 0) {
                client_remove(state, client_fd);
                return;
            }

            handle_channel_join(state, client, join_data.channel);
            break;
        }

        case MSG_TYPE_CHANNEL_CREATE: {
            if (header.length != sizeof(channel_create_data_t)) {
                send_error_message(client_fd, "Invalid CHANNEL_CREATE message format");
                break;
            }

            channel_create_data_t create_data;
            if (receive_message_data(client_fd, &create_data, sizeof(create_data)) < 0) {
                client_remove(state, client_fd);
                return;
            }

            handle_channel_create(state, client, create_data.channel);
            break;
        }

        case MSG_TYPE_CHANNEL_LIST: {
            handle_channel_list(state, client);
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
    chat_data.channel[0] = '\0';  // Empty for broadcast messages
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

// ==================== Channel Management Functions ====================

// Create a new channel
channel_t *channel_create(server_state_t *state, const char *channel_name) {
    // Check if channel already exists
    if (channel_find(state, channel_name) != NULL) {
        return NULL;
    }

    // Check channel name length
    if (strlen(channel_name) >= MAX_CHANNEL_LEN) {
        return NULL;
    }

    // Create new channel
    channel_t *new_channel = malloc(sizeof(channel_t));
    if (new_channel == NULL) {
        perror("malloc");
        return NULL;
    }

    memset(new_channel, 0, sizeof(channel_t));
    strncpy(new_channel->name, channel_name, sizeof(new_channel->name) - 1);
    new_channel->name[sizeof(new_channel->name) - 1] = '\0';
    new_channel->members = NULL;
    new_channel->history = NULL;
    new_channel->history_count = 0;

    // Add to linked list
    new_channel->next = state->channels;
    state->channels = new_channel;

    printf("Channel created: %s\n", channel_name);
    return new_channel;
}

// Find a channel by name
channel_t *channel_find(server_state_t *state, const char *channel_name) {
    channel_t *channel;

    for (channel = state->channels; channel != NULL; channel = channel->next) {
        if (strcmp(channel->name, channel_name) == 0) {
            return channel;
        }
    }

    return NULL;
}

// Add a client to a channel
void channel_add_client(channel_t *channel, client_t *client) {
    // Remove client from current channel first if needed
    // (handled by caller)

    // Add to channel's member list
    client->next_in_channel = channel->members;
    channel->members = client;

    printf("Client %s added to channel %s\n", client->username, channel->name);
}

// Remove a client from a channel
void channel_remove_client(channel_t *channel, client_t *client) {
    client_t *curr, *prev = NULL;

    for (curr = channel->members; curr != NULL; prev = curr, curr = curr->next_in_channel) {
        if (curr == client) {
            if (prev == NULL) {
                channel->members = curr->next_in_channel;
            } else {
                prev->next_in_channel = curr->next_in_channel;
            }
            curr->next_in_channel = NULL;
            printf("Client %s removed from channel %s\n", client->username, channel->name);
            return;
        }
    }
}

// Broadcast a message to all clients in a channel
void channel_broadcast(server_state_t *state, channel_t *channel, const char *username, const char *message, int exclude_fd) {
    (void)state; // Unused parameter
    client_t *client;
    chat_data_t chat_data;



    // Prepare chat message
    strncpy(chat_data.username, username, MAX_USERNAME_LEN - 1);
    chat_data.username[MAX_USERNAME_LEN - 1] = '\0';
    strncpy(chat_data.channel, channel->name, MAX_CHANNEL_LEN - 1);
    chat_data.channel[MAX_CHANNEL_LEN - 1] = '\0';
    strncpy(chat_data.message, message, MAX_MESSAGE_LEN - 1);
    chat_data.message[MAX_MESSAGE_LEN - 1] = '\0';

    // Send to all clients in the channel
    for (client = channel->members; client != NULL; client = client->next_in_channel) {
        if (client->authenticated && client->fd != exclude_fd) {

            send_message(client->fd, MSG_TYPE_CHAT, &chat_data, sizeof(chat_data));
        }

        // Add a message to channel history
        void channel_add_to_history(channel_t *channel, const char *username, const char *message) {
            // Create new history node
            message_node_t *new_node = malloc(sizeof(message_node_t));
            if (new_node == NULL) {
                perror("malloc");
                return;
            }
    
            // Allocate and copy username
            new_node->username = strdup(username);
            if (new_node->username == NULL) {
                perror("strdup");
                free(new_node);
                return;
            }
    
            // Allocate and copy message
            new_node->message = strdup(message);
            if (new_node->message == NULL) {
                perror("strdup");
                free(new_node->username);
                free(new_node);
                return;
            }
    
            new_node->next = NULL;
    
            // Add to end of history list
            if (channel->history == NULL) {
                channel->history = new_node;
            } else {
                message_node_t *current = channel->history;
                while (current->next != NULL) {
                    current = current->next;
                }
                current->next = new_node;
            }
    
            channel->history_count++;
    
            // Enforce MAX_HISTORY limit by removing oldest messages
            while (channel->history_count > MAX_HISTORY && channel->history != NULL) {
                message_node_t *oldest = channel->history;
                channel->history = oldest->next;
                free(oldest->username);
                free(oldest->message);
                free(oldest);
                channel->history_count--;
            }
        }

        // Send channel history to a client
        void channel_send_history(channel_t *channel, int client_fd) {
            message_node_t *current = channel->history;
            chat_data_t chat_data;
    
            // No history to send
            if (current ==
    }
}

// List clients in a channel
void channel_list_clients(server_state_t *state, channel_t *channel, char *buffer, size_t buffer_size) {
    (void)state; // Unused parameter
    client_t *client;
    int count = 0;
    size_t pos = 0;

    buffer[0] = '\0';

    for (client = channel->members; client != NULL; client = client->next_in_channel) {
        if (client->authenticated) {
            int written;
            if (count == 0) {
                written = snprintf(buffer + pos, buffer_size - pos, "%s", client->username);
            } else {
                written = snprintf(buffer + pos, buffer_size - pos, ", %s", client->username);
            }

            if (written < 0 || (size_t)written >= buffer_size - pos) {
                break;
            }

            pos += written;
            count++;
        }
    }

    if (count == 0) {
        snprintf(buffer, buffer_size, "(empty)");
    }
}

// Count members in a channel
int channel_count_members(channel_t *channel) {
    client_t *client;
    int count = 0;

    for (client = channel->members; client != NULL; client = client->next_in_channel) {
        if (client->authenticated) {
            count++;
        }
    }

    return count;
}

// List all channels
void channel_list_all(server_state_t *state, char *buffer, size_t buffer_size) {
    channel_t *channel;
    int count = 0;
    size_t pos = 0;

    buffer[0] = '\0';

    for (channel = state->channels; channel != NULL; channel = channel->next) {
        int member_count = channel_count_members(channel);
        int written;

        if (count == 0) {
            written = snprintf(buffer + pos, buffer_size - pos, "%s (%d users)",
                              channel->name, member_count);
        } else {
            written = snprintf(buffer + pos, buffer_size - pos, ", %s (%d users)",
                              channel->name, member_count);
        }

        if (written < 0 || (size_t)written >= buffer_size - pos) {
            break;
        }

        pos += written;
        count++;
    }

    if (count == 0) {
        snprintf(buffer, buffer_size, "No channels available");
    }
}

// ==================== Channel Message Handlers ====================

// Handle channel join request
void handle_channel_join(server_state_t *state, client_t *client, const char *channel_name) {
    // Check if client is authenticated
    if (!client->authenticated) {
        send_error_message(client->fd, "You must join with a username first");
        return;
    }

    // Check if channel exists
    channel_t *channel = channel_find(state, channel_name);
    if (channel == NULL) {
        send_error_message(client->fd, "Channel does not exist");
        return;
    }

    // Remove client from current channel if in one
    if (strlen(client->channel) > 0) {
        channel_t *old_channel = channel_find(state, client->channel);
        if (old_channel != NULL) {
            channel_remove_client(old_channel, client);

            // Notify old channel
            char leave_msg[MAX_MESSAGE_LEN];
            snprintf(leave_msg, sizeof(leave_msg),
                    "*** %s has left the channel ***", client->username);
            channel_broadcast(state, old_channel, "SERVER", leave_msg, -1);
        }
    }

    // Add client to new channel
    channel_add_client(channel, client);
    strncpy(client->channel, channel_name, sizeof(client->channel) - 1);
    client->channel[sizeof(client->channel) - 1] = '\0';

    // Send success message
    char success_msg[MAX_MESSAGE_LEN];
    snprintf(success_msg, sizeof(success_msg),
            "Joined channel '%s'", channel_name);
    send_server_message(client->fd, success_msg);

    // Notify new channel
    char join_msg[MAX_MESSAGE_LEN];
    snprintf(join_msg, sizeof(join_msg),
            "*** %s has joined the channel ***", client->username);
    channel_broadcast(state, channel, "SERVER", join_msg, client->fd);

    printf("Client %s joined channel %s\n", client->username, channel_name);
}

// Handle channel create request
void handle_channel_create(server_state_t *state, client_t *client, const char *channel_name) {
    // Check if client is authenticated
    if (!client->authenticated) {
        send_error_message(client->fd, "You must join with a username first");
        return;
    }

    // Check channel name validity
    if (strlen(channel_name) == 0) {
        send_error_message(client->fd, "Channel name cannot be empty");
        return;
    }

    if (strlen(channel_name) >= MAX_CHANNEL_LEN) {
        send_error_message(client->fd, "Channel name too long");
        return;
    }

    // Check if channel already exists
    if (channel_find(state, channel_name) != NULL) {
        send_error_message(client->fd, "Channel already exists");
        return;
    }

    // Create new channel
    channel_t *channel = channel_create(state, channel_name);
    if (channel == NULL) {
        send_error_message(client->fd, "Failed to create channel");
        return;
    }

    // Remove client from current channel if in one
    if (strlen(client->channel) > 0) {
        channel_t *old_channel = channel_find(state, client->channel);
        if (old_channel != NULL) {
            channel_remove_client(old_channel, client);

            // Notify old channel
            char leave_msg[MAX_MESSAGE_LEN];
            snprintf(leave_msg, sizeof(leave_msg),
                    "*** %s has left the channel ***", client->username);
            channel_broadcast(state, old_channel, "SERVER", leave_msg, -1);
        }
    }

    // Add client to new channel
    channel_add_client(channel, client);
    strncpy(client->channel, channel_name, sizeof(client->channel) - 1);
    client->channel[sizeof(client->channel) - 1] = '\0';

    // Send success message
    char success_msg[MAX_MESSAGE_LEN];
    snprintf(success_msg, sizeof(success_msg),
            "Created and joined channel '%s'", channel_name);
    send_server_message(client->fd, success_msg);

    printf("Client %s created channel %s\n", client->username, channel_name);
}

// Handle channel list request
void handle_channel_list(server_state_t *state, client_t *client) {
    char channel_list[MAX_CHANNEL_LEN * MAX_CHANNELS];

    channel_list_all(state, channel_list, sizeof(channel_list));

    // Send channel list to client
    send_server_message(client->fd, channel_list);

    printf("Sent channel list to client %s\n", client->username);
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
