#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>


typedef enum {
    MSG_TYPE_JOIN = 1,
    MSG_TYPE_CHAT = 2,
    MSG_TYPE_LEAVE = 3,
    MSG_TYPE_SERVER = 4,
    MSG_TYPE_ERROR = 5,
    MSG_TYPE_ACK = 6,
    MSG_TYPE_CHANNEL_JOIN = 7,
    MSG_TYPE_CHANNEL_CREATE = 8,
    MSG_TYPE_CHANNEL_LIST = 9,
    MSG_TYPE_CHANNEL_INFO = 10,
    MSG_TYPE_COLOR = 11,
} message_type_t;


#define MAX_USERNAME_LEN 64
#define MAX_CHANNEL_LEN 32
#define MAX_MESSAGE_LEN 1024
#define MAX_CHANNELS 100


typedef struct {
    uint32_t type;
    uint32_t length;
} message_header_t;


typedef struct {
    char username[MAX_USERNAME_LEN];
} join_data_t;


typedef struct {
    char channel[MAX_CHANNEL_LEN];
} channel_join_data_t;


typedef struct {
    char channel[MAX_CHANNEL_LEN];
} channel_create_data_t;


typedef struct {
    char channel[MAX_CHANNEL_LEN];
    int user_count;
    char users[MAX_USERNAME_LEN * 10];
} channel_info_data_t;


typedef struct {
    int channel_count;
    char channels[MAX_CHANNEL_LEN * MAX_CHANNELS];
} channel_list_data_t;


typedef struct {
    char username[MAX_USERNAME_LEN];
    char channel[MAX_CHANNEL_LEN];
    char message[MAX_MESSAGE_LEN];
} chat_data_t;


typedef struct {
    char message[MAX_MESSAGE_LEN];
} server_data_t;


typedef struct {
    char error_message[MAX_MESSAGE_LEN];
} error_data_t;


typedef struct {
    char username[MAX_USERNAME_LEN];
    char color[32];
} color_data_t;


int send_message(int fd, message_type_t type, const void *data, uint32_t data_len);
int receive_message_header(int fd, message_header_t *header);
int receive_message_data(int fd, void *data, uint32_t data_len);

#endif // PROTOCOL_H