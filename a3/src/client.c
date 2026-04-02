#include "client.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

// Global client state for signal handler
static client_state_t *g_client_state = NULL;

// Signal handler for graceful shutdown
static void signal_handler(int sig) {
    if (g_client_state != NULL) {
        printf("\nReceived signal %d, disconnecting...\n", sig);
        client_cleanup(g_client_state);
        exit(0);
    }
}

// Initialize client state
int client_init(client_state_t *state, const char *hostname, int port) {
    // Initialize state
    memset(state, 0, sizeof(client_state_t));
    strcpy(state->color, "white");  // Default color
    state->sockfd = -1;
    state->connected = 0;
    state->authenticated = 0;
    state->username[0] = '\0';
    state->max_fd = 0;

    // Set up signal handler
    g_client_state = state;
    signal(SIGINT, signal_handler);

    // Connect to server
    return client_connect(state, hostname, port);
}

// Clean up client resources
void client_cleanup(client_state_t *state) {
    if (state->connected) {
        // Send leave message if authenticated
        if (state->authenticated) {
            send_leave_message(state);
        }

        // Close socket
        close(state->sockfd);
        state->sockfd = -1;
        state->connected = 0;
        state->authenticated = 0;
    }

    printf("Client cleanup complete\n");
}

// Connect to server
int client_connect(client_state_t *state, const char *hostname, int port) {
    struct sockaddr_in server_addr;
    struct hostent *server;

    // Create socket
    state->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (state->sockfd < 0) {
        perror("socket");
        return -1;
    }

    // Get server address
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Error: No such host '%s'\n", hostname);
        close(state->sockfd);
        state->sockfd = -1;
        return -1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    server_addr.sin_port = htons(port);

    // Connect to server
    if (connect(state->sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(state->sockfd);
        state->sockfd = -1;
        return -1;
    }

    // Set up file descriptor sets
    FD_ZERO(&state->master_set);
    FD_SET(STDIN_FILENO, &state->master_set);  // Standard input
    FD_SET(state->sockfd, &state->master_set); // Server socket

    state->max_fd = (STDIN_FILENO > state->sockfd) ? STDIN_FILENO : state->sockfd;
    state->connected = 1;

    printf("Connected to server %s:%d\n", hostname, port);
    return 0;
}

// Disconnect from server
void client_disconnect(client_state_t *state) {
    if (state->connected) {
        close(state->sockfd);
        state->sockfd = -1;
        state->connected = 0;
        state->authenticated = 0;
        state->username[0] = '\0';

        // Update file descriptor sets
        FD_CLR(state->sockfd, &state->master_set);
        state->max_fd = STDIN_FILENO;

        printf("Disconnected from server\n");
    }
}

// Send JOIN message to server
int send_join_message(client_state_t *state, const char *username) {
    join_data_t join_data;

    if (!state->connected) {
        fprintf(stderr, "Error: Not connected to server\n");
        return -1;
    }

    // Check username length
    if (strlen(username) >= MAX_USERNAME_LEN) {
        fprintf(stderr, "Error: Username too long (max %d characters)\n", MAX_USERNAME_LEN - 1);
        return -1;
    }

    // Prepare join data
    strncpy(join_data.username, username, MAX_USERNAME_LEN - 1);
    join_data.username[MAX_USERNAME_LEN - 1] = '\0';

    // Send message
    if (send_message(state->sockfd, MSG_TYPE_JOIN, &join_data, sizeof(join_data)) < 0) {
        perror("send_message");
        client_disconnect(state);
        return -1;
    }

    // Update local state
    strncpy(state->username, username, MAX_USERNAME_LEN - 1);
    state->username[MAX_USERNAME_LEN - 1] = '\0';

    printf("Joining as '%s'...\n", username);
    fflush(stdout);
    return 0;
}

// Send channel join message to server
int send_channel_join_message(client_state_t *state, const char *channel_name) {
    channel_join_data_t join_data;
    
    if (!state->connected) {
        fprintf(stderr, "Error: Not connected to server\n");
        return -1;
    }
    
    if (!state->authenticated) {
        fprintf(stderr, "Error: You must join with a username first\n");
        return -1;
    }
    
    // Check channel name length
    if (strlen(channel_name) >= MAX_CHANNEL_LEN) {
        fprintf(stderr, "Error: Channel name too long (max %d characters)\n", MAX_CHANNEL_LEN - 1);
        return -1;
    }
    
    // Prepare join data
    strncpy(join_data.channel, channel_name, MAX_CHANNEL_LEN - 1);
    join_data.channel[MAX_CHANNEL_LEN - 1] = '\0';
    
    // Send message
    if (send_message(state->sockfd, MSG_TYPE_CHANNEL_JOIN, &join_data, sizeof(join_data)) < 0) {
        perror("send_message");
        client_disconnect(state);
        return -1;
    }
    
    printf("Joining channel '%s'...\n", channel_name);
    fflush(stdout);
    return 0;
}

// Send channel create message to server
int send_channel_create_message(client_state_t *state, const char *channel_name) {
    channel_create_data_t create_data;
    
    if (!state->connected) {
        fprintf(stderr, "Error: Not connected to server\n");
        return -1;
    }
    
    if (!state->authenticated) {
        fprintf(stderr, "Error: You must join with a username first\n");
        return -1;
    }
    
    // Check channel name length
    if (strlen(channel_name) >= MAX_CHANNEL_LEN) {
        fprintf(stderr, "Error: Channel name too long (max %d characters)\n", MAX_CHANNEL_LEN - 1);
        return -1;
    }
    
    // Prepare create data
    strncpy(create_data.channel, channel_name, MAX_CHANNEL_LEN - 1);
    create_data.channel[MAX_CHANNEL_LEN - 1] = '\0';
    
    // Send message
    if (send_message(state->sockfd, MSG_TYPE_CHANNEL_CREATE, &create_data, sizeof(create_data)) < 0) {
        perror("send_message");
        client_disconnect(state);
        return -1;
    }
    
    printf("Creating channel '%s'...\n", channel_name);
    fflush(stdout);
    return 0;
}

// Send channel list request to server
int send_channel_list_message(client_state_t *state) {
    if (!state->connected) {
        fprintf(stderr, "Error: Not connected to server\n");
        return -1;
    }
    
    if (!state->authenticated) {
        fprintf(stderr, "Error: You must join with a username first\n");
        return -1;
    }
    
    // Send message
    if (send_message(state->sockfd, MSG_TYPE_CHANNEL_LIST, NULL, 0) < 0) {
        perror("send_message");
        client_disconnect(state);
        return -1;
    }
    
    printf("Requesting channel list...\n");
    fflush(stdout);
    return 0;
}

// Send CHAT message to server
int send_chat_message(client_state_t *state, const char *message) {
    chat_data_t chat_data;

    if (!state->connected) {
        fprintf(stderr, "Error: Not connected to server\n");
        return -1;
    }

    if (!state->authenticated) {
        fprintf(stderr, "Error: You must join first with /join <username>\n");
        return -1;
    }

    // Check message length
    if (strlen(message) >= MAX_MESSAGE_LEN) {
        fprintf(stderr, "Error: Message too long (max %d characters)\n", MAX_MESSAGE_LEN - 1);
        return -1;
    }

    // Prepare chat data
    strncpy(chat_data.username, state->username, MAX_USERNAME_LEN - 1);
    chat_data.username[MAX_USERNAME_LEN - 1] = '\0';
    strncpy(chat_data.message, message, MAX_MESSAGE_LEN - 1);
    chat_data.message[MAX_MESSAGE_LEN - 1] = '\0';

    // Send message
    if (send_message(state->sockfd, MSG_TYPE_CHAT, &chat_data, sizeof(chat_data)) < 0) {
        perror("send_message");
        client_disconnect(state);
        return -1;
    }

    return 0;
}

// Send COLOR message to server
int send_color_message(client_state_t *state, const char *color) {
    color_data_t color_data;

    if (!state->connected) {
        fprintf(stderr, "Error: Not connected to server\n");
        return -1;
    }

    if (!state->authenticated) {
        fprintf(stderr, "Error: You must join first with /join <username>\n");
        return -1;
    }

    // Check color length
    if (strlen(color) >= 32) {
        fprintf(stderr, "Error: Color name too long\n");
        return -1;
    }

    // Prepare color data
    strncpy(color_data.username, state->username, MAX_USERNAME_LEN - 1);
    color_data.username[MAX_USERNAME_LEN - 1] = '\0';
    strncpy(color_data.color, color, sizeof(color_data.color) - 1);
    color_data.color[sizeof(color_data.color) - 1] = '\0';

    // Send message
    if (send_message(state->sockfd, MSG_TYPE_COLOR, &color_data, sizeof(color_data)) < 0) {
        perror("send_message");
        client_disconnect(state);
        return -1;
    }

    return 0;
}

// Send LEAVE message to server
int send_leave_message(client_state_t *state) {
    if (!state->connected) {
        return 0; // Already disconnected
    }

    if (send_message(state->sockfd, MSG_TYPE_LEAVE, NULL, 0) < 0) {
        perror("send_message");
    }

    return 0;
}

// Handle server messages
int handle_server_messages(client_state_t *state) {
    message_header_t header;

    // Note: The caller should check FD_ISSET before calling this function
    // No need to check here since we're already in a select() loop

    // Read message header
    if (receive_message_header(state->sockfd, &header) < 0) {
        // Connection closed or error
        printf("Server disconnected\n");
        client_disconnect(state);
        return -1;
    }

    // Handle based on message type
    switch (header.type) {
        case MSG_TYPE_SERVER: {
            if (header.length != sizeof(server_data_t)) {
                fprintf(stderr, "Error: Invalid server message format\n");
                break;
            }

            server_data_t server_data;
            if (receive_message_data(state->sockfd, &server_data, sizeof(server_data)) < 0) {
                printf("Server disconnected\n");
                client_disconnect(state);
                return -1;
            }

            handle_server_message(server_data.message);
            break;
        }

        case MSG_TYPE_CHAT: {
            if (header.length != sizeof(chat_data_t)) {
                fprintf(stderr, "Error: Invalid chat message format\n");
                break;
            }

            chat_data_t chat_data;
            if (receive_message_data(state->sockfd, &chat_data, sizeof(chat_data)) < 0) {
                printf("Server disconnected\n");
                client_disconnect(state);
                return -1;
            }

            handle_chat_message(chat_data.username, chat_data.message);
            break;
        }

        case MSG_TYPE_CHANNEL_INFO: {
            if (header.length != sizeof(channel_info_data_t)) {
                fprintf(stderr, "Error: Invalid channel info message format\n");
                break;
            }
            
            channel_info_data_t info_data;
            if (receive_message_data(state->sockfd, &info_data, sizeof(info_data)) < 0) {
                printf("Server disconnected\n");
                client_disconnect(state);
                return -1;
            }
            
            // Handle channel info (could display channel details)
            printf("Channel: %s (%d users)\n", info_data.channel, info_data.user_count);
            if (strlen(info_data.users) > 0) {
                printf("Users: %s\n", info_data.users);
            }
            break;
        }

        case MSG_TYPE_COLOR: {
            if (header.length != sizeof(color_data_t)) {
                fprintf(stderr, "Error: Invalid COLOR message format\n");
                break;
            }

            color_data_t color_data;
            if (receive_message_data(state->sockfd, &color_data, sizeof(color_data)) < 0) {
                printf("Server disconnected\n");
                client_disconnect(state);
                return -1;
            }

            handle_color_message(color_data.username, color_data.color);
            break;
        }

        case MSG_TYPE_ERROR: {
            if (header.length != sizeof(error_data_t)) {
                fprintf(stderr, "Error: Invalid error message format\n");
                break;
            }

            error_data_t error_data;
            if (receive_message_data(state->sockfd, &error_data, sizeof(error_data)) < 0) {
                printf("Server disconnected\n");
                client_disconnect(state);
                return -1;
            }

            handle_error_message(error_data.error_message);
            break;
        }

        case MSG_TYPE_ACK: {
            // Acknowledgment received - mark as authenticated
            if (!state->authenticated) {
                state->authenticated = 1;
                printf("Successfully joined as '%s'\n", state->username);
                fflush(stdout);
            }
            break;
        }

        default:
            fprintf(stderr, "Error: Unknown message type received: %u\n", header.type);
            break;
    }

    return 0;
}

// Handle server message
void handle_server_message(const char *message) {
    printf("\r\x1b[2K%s\n", message);
    fflush(stdout);
}

// Handle chat message
void handle_chat_message(const char *username, const char *message) {
    // Special handling for server notifications
    if (strcmp(username, "SERVER") == 0) {
        printf("\r\x1b[2K%s\n", message);
        fflush(stdout);
    } else {
        printf("\r\x1b[2K%s\n> %s\n", username, message);
        fflush(stdout);
    }
}

// Handle error message
void handle_error_message(const char *error_message) {
    printf("\r\x1b[2K[ERROR] %s\n", error_message);
    fflush(stdout);
}

// Handle COLOR message from server
void handle_color_message(const char *username, const char *color) {
    // Color messages are informational - the client already displays usernames with color codes
    // This function could be used to update local color cache if needed
    // For now, just acknowledge receipt
    (void)username;
    (void)color;
}

// Print command prompt
void print_prompt(void) {
    printf("> ");
    fflush(stdout);
}

// Process user input
void process_user_input(client_state_t *state, const char *input) {
    char command[64];
    char argument[256];

    // Skip leading whitespace
    while (*input == ' ' || *input == '\t') {
        input++;
    }

    // Check for empty input
    if (*input == '\0' || *input == '\n') {
        return;
    }

    // Parse command
    if (sscanf(input, "/%63s %255[^\n]", command, argument) == 2) {
        // Command with argument
        if (strcmp(command, "join") == 0) {
            send_join_message(state, argument);
        } else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            printf("\r\x1b[2KGoodbye!\n");
            client_cleanup(state);
            exit(0);
        } else if (strcmp(command, "channel") == 0) {
            send_channel_join_message(state, argument);
        } else if (strcmp(command, "create") == 0) {
            send_channel_create_message(state, argument);
        } else if (strcmp(command, "color") == 0) {
            send_color_message(state, argument);
        } else {
            printf("\r\x1b[2KUnknown command: /%s\n", command);
            printf("Available commands: /join <username>, /channel <name>, /create <name>, /color <color>, /list, /quit\n");
        }
    } else if (sscanf(input, "/%63s", command) == 1) {
        // Command without argument
        if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            printf("\r\x1b[2KGoodbye!\n");
            client_cleanup(state);
            exit(0);
        } else if (strcmp(command, "join") == 0) {
            printf("\r\x1b[2KUsage: /join <username>\n");
        } else if (strcmp(command, "channel") == 0) {
            printf("\r\x1b[2KUsage: /channel <channelname>\n");
        } else if (strcmp(command, "create") == 0) {
            printf("\r\x1b[2KUsage: /create <channelname>\n");
        } else if (strcmp(command, "color") == 0) {
            printf("\r\x1b[2KUsage: /color <color>\n");
            printf("Available colors: white, red, orange, yellow, green, turquoise, blue, purple\n");
        } else if (strcmp(command, "list") == 0) {
            send_channel_list_message(state);
        } else {
            printf("\r\x1b[2KUnknown command: /%s\n", command);
            printf("Available commands: /join <username>, /channel <name>, /create <name>, /color <color>, /list, /quit\n");
        }
    } else {
        // Regular chat message
        // Remove trailing newline if present
        char message[MAX_MESSAGE_LEN];
        strncpy(message, input, MAX_MESSAGE_LEN - 1);
        message[MAX_MESSAGE_LEN - 1] = '\0';

        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }

        // Clear the typing line: move up one line, then clear
        printf("\x1b[1A\r\x1b[2K");
        fflush(stdout);
        send_chat_message(state, message);
    }
}

// Main client event loop
void client_run(client_state_t *state) {
    fd_set read_fds;
    char input_buffer[BUFFER_SIZE];

    printf("Chat Client\n");
    printf("Type /join <username> to join the chat\n");
    printf("Type /channel <name> to join a channel\n");
    printf("Type /create <name> to create a channel\n");
    printf("Type /color <color> to change your username color\n");
    printf("Type /list to list all channels\n");
    printf("Type /quit to exit\n");
    printf("Type a message and press Enter to send\n\n");

    while (state->connected) {
        // Print prompt at start of each loop iteration
        print_prompt();
        
        // Copy master set because select() modifies it
        read_fds = state->master_set;
        
        // Wait for activity (no timeout - wait for input)
        int activity = select(state->max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        if (activity < 0) {
            if (errno == EINTR) {
                // Interrupted by signal, continue
                continue;
            }
            perror("select");
            break;
        }
        
        // Check for user input first
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
                // EOF (Ctrl+D)
                printf("\nGoodbye!\n");
                client_cleanup(state);
                exit(0);
            }
            
            process_user_input(state, input_buffer);
        }
        
        // Check for server messages after processing user input
        if (state->connected && FD_ISSET(state->sockfd, &read_fds)) {
            if (handle_server_messages(state) < 0) {
                // Connection lost
                break;
            }
        }
    }

    if (!state->connected) {
        printf("\nDisconnected from server. Press Enter to exit...\n");
        getchar(); // Wait for user to press Enter
    }
}
