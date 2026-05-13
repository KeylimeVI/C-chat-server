#ifndef CLIENT_H
#define CLIENT_H

#include <sys/select.h>

#define BUFFER_SIZE 4096

typedef struct client_state {
    int sockfd;
    char username[64];
    char color[16];
    int connected;
    int authenticated;
    fd_set master_set;
    int max_fd;
} client_state_t;

int client_init(client_state_t *state, const char *hostname, int port);
void client_cleanup(client_state_t *state);

int client_connect(client_state_t *state, const char *hostname, int port);
void client_disconnect(client_state_t *state);

int send_join_message(client_state_t *state, const char *username);
int send_chat_message(client_state_t *state, const char *message);
int send_leave_message(client_state_t *state);
int send_color_message(client_state_t *state, const char *color);

int handle_server_messages(client_state_t *state);
void handle_server_message(const char *message);
void handle_chat_message(const char *username, const char *message);
void handle_error_message(const char *error_message);
void handle_color_message(const char *username, const char *color);

void print_prompt(void);
void process_user_input(client_state_t *state, const char *input);

void client_run(client_state_t *state);

#endif // CLIENT_H