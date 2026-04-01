# Makefile for CSC209 Assignment 3 - Chat Server/Client
# Builds both server and client executables

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -D_POSIX_C_SOURCE=200809L -Iinclude
PORT ?= 4242

# Source files
SERVER_SRCS = src/main_server.c src/server.c src/protocol.c
CLIENT_SRCS = src/main_client.c src/client.c src/protocol.c

# Object files
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# Executables
SERVER_EXEC = server
CLIENT_EXEC = client

# Default target
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Build server
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Build client
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run server with specified port
run-server: $(SERVER_EXEC)
	./$(SERVER_EXEC) -p $(PORT)

# Run client with specified port
run-client: $(CLIENT_EXEC)
	./$(CLIENT_EXEC) -h localhost -p $(PORT)

# Clean build artifacts
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_EXEC) $(CLIENT_EXEC)

# Phony targets
.PHONY: all clean run-server run-client