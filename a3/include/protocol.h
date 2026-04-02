#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Message types for differentiating between different kinds of messages
typedef enum {
    MSG_TYPE_JOIN = 1,      // Client joining with username
    MSG_TYPE_CHAT = 2,      // Chat message to channel
    MSG_TYPE_LEAVE = 3,     // Client leaving/disconnecting
    MSG_TYPE_SERVER = 4,    // Server notification/broadcast
    MSG_TYPE_ERROR = 5,     // Error message from server
    MSG_TYPE_ACK = 6,       // Acknowledgment
    MSG_TYPE_CHANNEL_JOIN = 7,  // Client joining a channel
    MSG_TYPE_CHANNEL_CREATE = 8, // Client creating a channel
    MSG_TYPE_CHANNEL_LIST = 9,   // Client requesting channel list
    MSG_TYPE_CHANNEL_INFO = 10,  // Server sending channel info
    MSG_TYPE_COLOR = 11,         // Client changing color or server broadcasting color change
    // Future command types will be added here
} message_type_t;

// Maximum lengths for various fields
#define MAX_USERNAME_LEN 64
#define MAX_CHANNEL_LEN 32
#define MAX_MESSAGE_LEN 1024
#define MAX_CHANNELS 100

// Message header - fixed size, always sent first
typedef struct {
    uint32_t type;          // message_type_t encoded as uint32_t
    uint32_t length;        // Length of data payload (network byte order)
} message_header_t;

// Join message data (client -> server)
typedef struct {
    char username[MAX_USERNAME_LEN];
} join_data_t;

// Channel join message data (client -> server)
typedef struct {
    char channel[MAX_CHANNEL_LEN];
} channel_join_data_t;

// Channel create message data (client -> server)
typedef struct {
    char channel[MAX_CHANNEL_LEN];
} channel_create_data_t;

// Channel info message data (server -> client)
typedef struct {
    char channel[MAX_CHANNEL_LEN];
    int user_count;
    char users[MAX_USERNAME_LEN * 10]; // Comma-separated list of usernames
} channel_info_data_t;

// Channel list message data (server -> client)
typedef struct {
    int channel_count;
    char channels[MAX_CHANNEL_LEN * MAX_CHANNELS]; // Comma-separated list
} channel_list_data_t;

// Chat message data (client -> server, server -> client)
typedef struct {
    char username[MAX_USERNAME_LEN];
    char channel[MAX_CHANNEL_LEN];
    char message[MAX_MESSAGE_LEN];
} chat_data_t;

// Server message data (server -> client)
typedef struct {
    char message[MAX_MESSAGE_LEN];
} server_data_t;

// Error message data (server -> client)
typedef struct {
    char error_message[MAX_MESSAGE_LEN];
} error_data_t;

// Color change message data (client -> server, server -> client)
typedef struct {
    char username[MAX_USERNAME_LEN];
    char color[32];
} color_data_t;

// Function prototypes for protocol handling
int send_message(int fd, message_type_t type, const void *data, uint32_t data_len);
int receive_message_header(int fd, message_header_t *header);
int receive_message_data(int fd, void *data, uint32_t data_len);

#endif // PROTOCOL_H