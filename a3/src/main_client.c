#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Default values
#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT 4242

// Function to print usage information, Deepseek wrote this
static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -h HOST    Server hostname or IP address (default: %s)\n", DEFAULT_HOST);
    printf("  -p PORT    Server port number (default: %d)\n", DEFAULT_PORT);
    printf("  -?         Show this help message\n");
}

int main(int argc, char *argv[]) {
    const char *hostname = DEFAULT_HOST;
    int port = DEFAULT_PORT;
    int opt;

    // Parse command line arguments, Deepseek wrote this
    while ((opt = getopt(argc, argv, "h:p:?")) != -1) {
        switch (opt) {
            case 'h':
                hostname = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                if (port <= 0 || port > 65535) {
                    fprintf(stderr, "Error: Invalid port number '%s'. Port must be between 1 and 65535.\n", optarg);
                    return 1;
                }
                break;
            case '?':
                print_usage(argv[0]);
                return 0;
            default:
                fprintf(stderr, "Error: Unknown option '-%c'\n", opt);
                print_usage(argv[0]);
                return 1;
        }
    }

    // Check for extra arguments
    if (optind < argc) {
        fprintf(stderr, "Error: Unexpected argument '%s'\n", argv[optind]);
        print_usage(argv[0]);
        return 1;
    }

    printf("Connecting to chat server at %s:%d...\n", hostname, port);

    client_state_t client_state;
    if (client_init(&client_state, hostname, port) < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        return 1;
    }

    client_run(&client_state);

    client_cleanup(&client_state);

    return 0;
}
