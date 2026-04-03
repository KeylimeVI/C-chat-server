#ifndef SERVER_H
#define SERVER_H

#include <sys/select.h>
#include <stddef.h>

#define DEFAULT_PORT 4242
#define MAX_CLIENTS 100
#define BUFFER_SIZE 4096
#define MAX_CHANNELS 100
#define MAX_HISTORY 100

typedef struct message_node {
    char *username;
    char *message;
    struct message_node *next;
} message_node_t;

typedef struct channel {
    char name[32];
    struct client *members;
    message_node_t *history;
    int history_count;
    struct channel *next;
} channel_t;

typedef struct client {
    int fd;
    char username[64];
    char channel[32];
    char color[16];
    int authenticated;
    struct client *next;
    struct client *next_in_channel;
} client_t;

typedef struct server_state {
    int server_fd;
    int port;
    client_t *clients;
    channel_t *channels;
    fd_set master_set;
    int max_fd;
} server_state_t;

int server_init(server_state_t *state, int port);
void server_cleanup(server_state_t *state);

client_t *client_add(server_state_t *state, int client_fd);
void client_remove(server_state_t *state, int client_fd);
client_t *client_find_by_fd(server_state_t *state, int fd);
client_t *client_find_by_username(server_state_t *state, const char *username);

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

void handle_new_connection(server_state_t *state);
void handle_client_message(server_state_t *state, int client_fd);
void broadcast_message(server_state_t *state, const char *username, const char *message);
void send_server_message(int client_fd, const char *message);
void send_error_message(int client_fd, const char *error_message);

void handle_color_message(server_state_t *state, client_t *client, const char *color);
void broadcast_color_change(server_state_t *state, client_t *client, const char *color);

void handle_channel_join(server_state_t *state, client_t *client, const char *channel_name);
void handle_channel_create(server_state_t *state, client_t *client, const char *channel_name);
void handle_channel_list(server_state_t *state, client_t *client);

void server_run(server_state_t *state);

#endif // SERVER_H