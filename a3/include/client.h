#ifndef CLIENT_H
#define CLIENT_H

#include <sys/select.h>

// Client configuration
#define BUFFER_SIZE 4096

// Client state structure
typedef struct client_state {
    int sockfd;                     // Socket file descriptor
    char username[32];              // Client's username
    int connected;                  // Whether client is connected to server
    int authenticated;              // Whether client has joined with username
    fd_set master_set;              // File descriptor set for select()
    int max_fd;                     // Maximum file descriptor number
} client_state_t;

// Client initialization and cleanup
int client_init(client_state_t *state, const char *hostname, int port);
void client_cleanup(client_state_t *state);

// Connection management
int client_connect(client_state_t *state, const char *hostname, int port);
void client_disconnect(client_state_t *state);

// Message sending
int send_join_message(client_state_t *state, const char *username);
int send_chat_message(client_state_t *state, const char *message);
int send_leave_message(client_state_t *state);

// Message receiving and handling
int handle_server_messages(client_state_t *state);
void handle_server_message(const char *message);
void handle_chat_message(const char *username, const char *message);
void handle_error_message(const char *error_message);

// User interface
void print_prompt(void);
void process_user_input(client_state_t *state, const char *input);

// Main client loop
void client_run(client_state_t *state);

#endif // CLIENT_H