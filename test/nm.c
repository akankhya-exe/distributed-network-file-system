#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <asm-generic/socket.h>

#define PORT 5000

// hardcoded lookup table for now — no hashmap yet
char* lookup(char *filename) {
    if (strcmp(filename, "files/hello.txt") == 0)
        return "127.0.0.1:9001";
    return "ERROR: file not found";
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = { 0 };

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd, SOL_SOCKET,
               SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    if ((new_socket = accept(server_fd, (struct sockaddr*)&address,
                             &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // read filename from client
    read(new_socket, buffer, 1024 - 1);
    printf("NM received request for: %s\n", buffer);

    // look up which SS has it and reply
    char *response = lookup(buffer);
    send(new_socket, response, strlen(response), 0);
    printf("NM replied with: %s\n", response);

    close(new_socket);
    close(server_fd);
    return 0;
}