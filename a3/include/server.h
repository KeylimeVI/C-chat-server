#ifndef SERVER_H
#define SERVER_H

#include <sys/select.h>

// Server configuration
#define DEFAULT_PORT 4242
#define MAX_CLIENTS 100
#define BUFFER_SIZE 4096

// Client state structure
typedef struct client {
    int fd;                     // Socket file descriptor
    char username[32];          // Client username (empty if not set)
    int authenticated;          // Whether client has sent JOIN message
    struct client *next;        // Next client in linked list
} client_t;

// Server state structure
typedef struct server_state {
    int server_fd;              // Server socket file descriptor
    int port;                   // Port number
    client_t *clients;          // Linked list of connected clients
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

// Message handling
void handle_new_connection(server_state_t *state);
void handle_client_message(server_state_t *state, int client_fd);
void broadcast_message(server_state_t *state, const char *username, const char *message);
void send_server_message(int client_fd, const char *message);
void send_error_message(int client_fd, const char *error_message);

// Main server loop
void server_run(server_state_t *state);

#endif // SERVER_H