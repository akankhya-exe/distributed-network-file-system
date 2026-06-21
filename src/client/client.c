#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>


#define NM_PORT 5000
#define NM_IP "127.0.0.1"

int connect_to(char *ip, int port) {
    int fd;
    struct sockaddr_in addr;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("invalid address");
        return -1;
    }
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connection failed");
        return -1;
    }
    return fd;
}

int main() {
    char filename[256];
    char nm_response[1024] = { 0 };
    char file_contents[4096] = { 0 };

    // get filename from user
    printf("Enter filename: ");
    scanf("%s", filename);

    // step 1: ask NM where the file is
    int nm_fd = connect_to(NM_IP, NM_PORT);
    send(nm_fd, filename, strlen(filename), 0);
    read(nm_fd, nm_response, sizeof(nm_response) - 1);
    close(nm_fd);
    printf("NM says: %s\n", nm_response);

    // step 2: parse "127.0.0.1:9001" into IP and port
    char ss_ip[64];
    int ss_port;
    char *colon = strchr(nm_response, ':');
    strncpy(ss_ip, nm_response, colon - nm_response);
    ss_ip[colon - nm_response] = '\0';
    ss_port = atoi(colon + 1);

    // step 3: connect to SS and get the file
    int ss_fd = connect_to(ss_ip, ss_port);
    send(ss_fd, filename, strlen(filename), 0);
    int bytes = read(ss_fd, file_contents, sizeof(file_contents) - 1);
    close(ss_fd);

    printf("\n--- File Contents ---\n%s\n", file_contents);
    return 0;
}