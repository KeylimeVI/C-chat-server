#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

// Helper function to read exactly n bytes from a socket
static int read_exact(int fd, void *buf, size_t n) {
    size_t total_read = 0;
    ssize_t nread;
    
    while (total_read < n) {
        nread = read(fd, (char*)buf + total_read, n - total_read);
        if (nread <= 0) {
            if (nread == 0) {
                // EOF - connection closed
                return -1;
            }
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                // Would block or interrupted, try again
                continue;
            }
            // Real error
            return -1;
        }
        total_read += nread;
    }
    return 0;
}

// Helper function to write exactly n bytes to a socket
static int write_exact(int fd, const void *buf, size_t n) {
    size_t total_written = 0;
    ssize_t nwritten;
    
    while (total_written < n) {
        nwritten = write(fd, (const char*)buf + total_written, n - total_written);
        if (nwritten <= 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return -1;
        }
        total_written += nwritten;
    }
    return 0;
}

int send_message(int fd, message_type_t type, const void *data, uint32_t data_len) {
    message_header_t header;
    
    header.type = htonl((uint32_t)type);
    header.length = htonl(data_len);
    
    if (write_exact(fd, &header, sizeof(header)) < 0) {
        return -1;
    }
    
    if (data_len > 0 && data != NULL) {
        if (write_exact(fd, data, data_len) < 0) {
            return -1;
        }
    }
    
    return 0;
}

int receive_message_header(int fd, message_header_t *header) {
    if (read_exact(fd, header, sizeof(message_header_t)) < 0) {
        return -1;
    }
    
    header->type = ntohl(header->type);
    header->length = ntohl(header->length);
    
    return 0;
}

int receive_message_data(int fd, void *data, uint32_t data_len) {
    if (data_len == 0) {
        return 0; // No data to read
    }
    
    if (data == NULL) {
        return -1;
    }
    
    if (read_exact(fd, data, data_len) < 0) {
        return -1;
    }
    
    return 0;
}