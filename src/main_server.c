#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Function to print usage information. Deepseek wrote this I'm lazy.
static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -p PORT    Port number to listen on (default: %d)\n", DEFAULT_PORT);
    printf("  -h         Show this help message\n");
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    int opt;

    // Parse command line arguments, Deepseek wrote this
    while ((opt = getopt(argc, argv, "p:h")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                if (port <= 0 || port > 65535) {
                    fprintf(stderr, "Error: Invalid port number '%s'. Port must be between 1 and 65535.\n", optarg);
                    return 1;
                }
                break;
            case 'h':
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

    printf("Starting chat server on port %d...\n", port);

    // Initialize server state
    server_state_t server_state;
    if (server_init(&server_state, port) < 0) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }

    // Run server
    server_run(&server_state);

    // Cleanup (should not reach here normally)
    server_cleanup(&server_state);

    return 0;
}
