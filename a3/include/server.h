#ifndef SERVER_H
#define SERVER_H

#include <sys/select.h>
#include <stddef.h>

// Server configuration
#define DEFAULT_PORT 4242
#define MAX_CLIENTS 100
#define BUFFER_SIZE 4096
#define MAX_CHANNELS 100
#define MAX_HISTORY 100  // Maximum number of messages to store in channel history

// Message history node structure
typedef struct message_node {
    char *username;             // Username who sent the message (or "SERVER" for notifications)
    char *message;              // The message content
    struct message_node *next;  // Next message in history
} message_node_t;

// Channel structure
typedef struct channel {
    char name[32];              // Channel name
    struct client *members;     // Linked list of members
    message_node_t *history;    // Linked list of message history
    int history_count;          // Number of messages in history
    struct channel *next;       // Next channel in linked list
} channel_t;

// Client state structure
typedef struct client {
    int fd;                     // Socket file descriptor
    char username[64];          // Client username (empty if not set)
    char channel[32];           // Current channel (empty if not in channel)
    char color[16];             // Color for username display
    int authenticated;          // Whether client has sent JOIN message
    struct client *next;        // Next client in linked list
    struct client *next_in_channel; // Next client in channel member list
} client_t;

// Server state structure
typedef struct server_state {
    int server_fd;              // Server socket file descriptor
    int port;                   // Port number
    client_t *clients;          // Linked list of connected clients
    channel_t *channels;        // Linked list of channels
    fd_set master_set;          // Master file descriptor set for select()
    int max_fd;                 // Maximum file descriptor number
} server_state_t;

// Server initialization and cleanup
int server_init(server_state_t *state, int port);
void server_cleanup(server_state_t *state);

// Client management
client_t *client_add(server_state_t *state, int client_fd);
void client_remove(server_state_t *state, int client_fd);
client_t *client_find_by_fd(server_state_t *state, int fd);
client_t *client_find_by_username(server_state_t *state, const char *username);

// Channel management
channel_t *channel_create(server_state_t *state, const char *channel_name);
channel_t *channel_find(server_state_t *state, const char *channel_name);
void channel_add_client(channel_t *channel, client_t *client);
void channel_remove_client(channel_t *channel, client_t *client);
void channel_broadcast(server_state_t *state, channel_t *channel, client_t *sender, const char *message, int exclude_fd);
void channel_add_to_history(channel_t *channel, client_t *sender, const char *message);
void channel_send_history(channel_t *channel, int client_fd);
void channel_free_history(channel_t *channel);
void channel_list_clients(server_state_t *state, channel_t *channel, char *buffer, size_t buffer_size);
int channel_count_members(channel_t *channel);
void channel_list_all(server_state_t *state, char *buffer, size_t buffer_size);

// Message handling
void handle_new_connection(server_state_t *state);
void handle_client_message(server_state_t *state, int client_fd);
void broadcast_message(server_state_t *state, const char *username, const char *message);
void send_server_message(int client_fd, const char *message);
void send_error_message(int client_fd, const char *error_message);

// Color handling
void handle_color_message(server_state_t *state, client_t *client, const char *color);
void broadcast_color_change(server_state_t *state, client_t *client, const char *color);

// Channel message handling
void handle_channel_join(server_state_t *state, client_t *client, const char *channel_name);
void handle_channel_create(server_state_t *state, client_t *client, const char *channel_name);
void handle_channel_list(server_state_t *state, client_t *client);

// Main server loop
void server_run(server_state_t *state);

#endif // SERVER_H