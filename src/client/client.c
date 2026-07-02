#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

#define BUF 4096
#define MAX_USERNAME 64
#define MAX_FILENAME 128

// parse NM response for SS IP and port
int parse_ss_info(const char *response, char *ss_ip, int *ss_port){
    if (sscanf(response, "OK SS_IP:%63s SS_PORT:%d", ss_ip, ss_port) == 2){
        return 0;
    }

    return -1;
}

// helper function to read file from ss server
int read_file_from_ss(const char *filename, const char *username, const char *ss_ip, int ss_port){
    // connect to ss
    int ss_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ss_fd < 0){
        printf("Error: Cannot create socket for ss connection");
        return -1;
    }

    struct sockaddr_in ss_ddr;
    ss_addr.sin_family = AF_INET;
    ss_addr.sin_port = htons(ss_port);
    if (inet_pton(AF_INET, ss_ip, &ss_addr.sin_addr) != 1){
        close(ss_fd);
        printf("Error: Invalid SS address");
        return -1;
    }

    if (connect(ss_fd, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) != 0) {
        close(ss_fd);
        printf("ERROR: Cannot connect to Storage Server at %s:%d.\n", ss_ip, ss_port);
        return -1;
    }
    
    // send read request to ss
    char request[512];
    snprintf(request, sizeof(request), "READ %s %s\n", filename, username);
    send(ss_fd, request, strlen(request), 0);

    // receive response
    char buffer[BUF];
    ssize_t n = recv(ss_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0){
        close(ss_fd);
        printf("Error: No response from storage server\n");
        return -1;
    }

    buffer[n] = '\0';

    // Check if it's an error
    if (strncmp(buffer, "ERROR:", 6) == 0) {
        printf("%s", buffer);
        close(ss_fd);
        return -1;
    }
    
    // Print the file content
    printf("%s", buffer);
    if (n > 0 && buffer[n-1] != '\n') {
        printf("\n");
    }

    close(ss_fd);
    return 0;
}

int main(int argc, char *argv[]){
    char nm_ip[64];
    int nm_port = 0;

    // prompting for arguments not provided
    if (argc < 3) {
        printf("Enter Naming Server IP address: ");
        if (fgets(nm_ip, sizeof(nm_ip), stdin)) {
            nm_ip[strcspn(nm_ip, "\n")] = 0; // Remove newline
        }
        if (strlen(nm_ip) == 0) {
            strcpy(nm_ip, "127.0.0.1");
            printf("Using default: 127.0.0.1\n");
        }

        printf("Enter Naming Server port: ");
        char port_str[16];
        if (fgets(port_str, sizeof(port_str), stdin)) {
            nm_port = atoi(port_str);
        }
        if (nm_port <= 0) {
            nm_port = 7000;
            printf("Using default: 7000\n");
        }
    }

    // ip and port provided via command line
    if (argc >= 3) {
        strncpy(nm_ip, argv[1], sizeof(nm_ip) - 1);
        nm_ip[sizeof(nm_ip) - 1] = '\0';
        nm_port = atoi(argv[2]);
    }

    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nm_port);
    inet_pton(AF_INET, nm_ip, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to NM at %s:%d\n", nm_ip, nm_port);

    // registration
    char username[100];
    printf("Enter username: ");
    scanf("%99s", username);
    getchar(); // remove newline

    // receive welcome message + registration ack (can come together or separately)
    int n = recv(sock, msg, sizeof(msg) - 1, 0);
    if (n > 0){
        msg[n] = '\0';
        // check if both came in one recv
        char *reg_ack = strstr(msg, "OK: Registered");
        if (reg_ack){
            // print everything from the registration ack onwards
            printf("NM> %s", reg_ack);
        }else{
            // got welcome message, recv again for ack
             n = recv(sock, msg, sizeof(msg)-1, 0);
            if (n > 0) {
                msg[n] = '\0';
                printf("NM> %s", msg);
            }
        }
    }

     printf("You may now enter commands.\n");

     // main loop
      while (1) {
        printf("> ");
        fflush(stdout);

        char cmd[BUF];
        if (!fgets(cmd, sizeof(cmd), stdin)) break;

        if (strncmp(cmd, "EXIT", 4) == 0) {
            send(sock, "EXIT\n", 5, 0);
            break;
        }

        // handle READ command
        if (strncmp(cmd, "READ ", 5) == 0) {
            char filename[MAX_FILENAME];
            if (sscanf(cmd + 5, "%127s", filename) != 1) {
                printf("Usage: READ <filename>\n");
                continue;
            }
            
            // send READ request to NM
            send(sock, cmd, strlen(cmd), 0);
            
            // receive ss info from nm
            char response[256];
            n = recv(sock, response, sizeof(response) - 1, 0);
            if (n <= 0) {
                printf("ERROR: Connection lost.\n");
                break;
            }
            response[n] = '\0';
            
            // check for error
            if (strncmp(response, "ERROR:", 6) == 0) {
                printf("%s", response);
                continue;
            }
            
            // parse ss ip and port
            char ss_ip[64];
            int ss_port;
            if (parse_ss_info(response, ss_ip, &ss_port) != 0) {
                printf("ERROR: Invalid response from Name Server.\n");
                continue;
            }
            
            // connect to ss and read file
            read_file_from_ss(filename, username, ss_ip, ss_port);
            continue;
        }

        
        // handle VIEWCHECKPOINT command
        if (strncmp(cmd, "VIEWCHECKPOINT ", 15) == 0) {
            char filename[MAX_FILENAME];
            char checkpoint_tag[64];
            if (sscanf(cmd + 15, "%127s %63s", filename, checkpoint_tag) != 2) {
                printf("Usage: VIEWCHECKPOINT <filename> <checkpoint_tag>\n");
                continue;
            }
            
            // send VIEWCHECKPOINT request to NM
            send(sock, cmd, strlen(cmd), 0);
            
            // receive SS info from NM
            char response[256];
            n = recv(sock, response, sizeof(response) - 1, 0);
            if (n <= 0) {
                printf("ERROR: Connection lost.\n");
                break;
            }
            response[n] = '\0';
            
            // check for error
            if (strncmp(response, "ERROR:", 6) == 0) {
                printf("%s", response);
                continue;
            }
            
            // parse SS IP, port and checkpoint tag
            char ss_ip[64];
            int ss_port;
            char tag[64];
            if (sscanf(response, "OK SS_IP:%63s SS_PORT:%d CHECKPOINT:%63s", ss_ip, &ss_port, tag) != 3) {
                printf("ERROR: Invalid response from Name Server.\n");
                continue;
            }
            
            // connect to SS
            int ss_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (ss_fd < 0) {
                printf("ERROR: Cannot create socket for SS connection.\n");
                continue;
            }
            
            struct sockaddr_in ss_addr;
            ss_addr.sin_family = AF_INET;
            ss_addr.sin_port = htons(ss_port);
            if (inet_pton(AF_INET, ss_ip, &ss_addr.sin_addr) != 1) {
                close(ss_fd);
                printf("ERROR: Invalid SS address.\n");
                continue;
            }
            
            if (connect(ss_fd, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) != 0) {
                close(ss_fd);
                printf("ERROR: Cannot connect to Storage Server at %s:%d.\n", ss_ip, ss_port);
                continue;
            }
            
            // send VIEWCHECKPOINT request to SS
            char request[512];
            snprintf(request, sizeof(request), "VIEWCHECKPOINT %s %s %s\n", filename, checkpoint_tag, username);
            send(ss_fd, request, strlen(request), 0);
            
            // receive checkpoint content
            char buffer[4096];
            ssize_t bytes = recv(ss_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) {
                close(ss_fd);
                printf("ERROR: No response from Storage Server.\n");
                continue;
            }
            
            buffer[bytes] = '\0';
            
            // check for errors
            if (strncmp(buffer, "ERROR:", 6) == 0) {
                printf("%s", buffer);
            } else {
                // print the checkpoint content
                printf("%s", buffer);
                if (bytes > 0 && buffer[bytes-1] != '\n') {
                    printf("\n");
                }
            }
            
            close(ss_fd);
            continue;
        }

        // handle WRITE command with multi-line input
        if (strncmp(cmd, "WRITE ", 6) == 0) {
            char filename[MAX_FILENAME];
            int sentence_num;
            if (sscanf(cmd + 6, "%127s %d", filename, &sentence_num) != 2) {
                printf("Usage: WRITE <filename> <sentence_number>\n");
                continue;
            }
            
            // send WRITE request to NM
            send(sock, cmd, strlen(cmd), 0);
            
            // receive SS info from NM
            char response[256];
            n = recv(sock, response, sizeof(response) - 1, 0);
            if (n <= 0) {
                printf("ERROR: Connection lost.\n");
                break;
            }
            response[n] = '\0';
            
            // check for error
            if (strncmp(response, "ERROR:", 6) == 0) {
                printf("%s", response);
                continue;
            }
            
            // parse SS IP and port
            char ss_ip[64];
            int ss_port;
            if (parse_ss_info(response, ss_ip, &ss_port) != 0) {
                printf("ERROR: Invalid response from Name Server.\n");
                continue;
            }
            
            // connect to SS
            int ss_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (ss_fd < 0) {
                printf("ERROR: Cannot create socket for SS connection.\n");
                continue;
            }
            
            struct sockaddr_in ss_addr;
            ss_addr.sin_family = AF_INET;
            ss_addr.sin_port = htons(ss_port);
            if (inet_pton(AF_INET, ss_ip, &ss_addr.sin_addr) != 1) {
                close(ss_fd);
                printf("ERROR: Invalid SS address.\n");
                continue;
            }
            
            if (connect(ss_fd, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) != 0) {
                close(ss_fd);
                printf("ERROR: Cannot connect to Storage Server at %s:%d.\n", ss_ip, ss_port);
                continue;
            }
            
            // send WRITE header to SS
            char write_cmd[512];
            snprintf(write_cmd, sizeof(write_cmd), "WRITE %s %s %d\n", 
                     filename, username, sentence_num);
            send(ss_fd, write_cmd, strlen(write_cmd), 0);
            
            // wait for initial response from SS (could be error or ready for updates)
            // set a timeout to check if SS sends an error immediately
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 200000; // 200ms timeout
            setsockopt(ss_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
            
            char initial_response[1024];
            n = recv(ss_fd, initial_response, sizeof(initial_response) - 1, 0);
            
            // reset to blocking mode
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            setsockopt(ss_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
            
            // check if we got an error response
            if (n > 0) {
                initial_response[n] = '\0';
                if (strncmp(initial_response, "ERROR:", 6) == 0) {
                    printf("%s", initial_response);
                    close(ss_fd);
                    continue;
                }
                // otherwise, this might be a premature response, ignore it
            }
            
            // enter multi-line mode for word updates
            printf("Enter word updates (<word_index> <content>), type ETIRW to finish:\n");
            while (1) {
                printf("Client: ");
                fflush(stdout);
                
                char update[2048];
                if (!fgets(update, sizeof(update), stdin)) break;
                
                send(ss_fd, update, strlen(update), 0);
                
                if (strncmp(update, "ETIRW", 5) == 0) {
                    break;
                }
                
                // check for any error responses from SS after each update
                tv.tv_sec = 0;
                tv.tv_usec = 50000; // 50ms timeout
                setsockopt(ss_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
                
                char err_check[512];
                ssize_t err_n = recv(ss_fd, err_check, sizeof(err_check) - 1, 0);
                if (err_n > 0) {
                    err_check[err_n] = '\0';
                    if (strncmp(err_check, "ERROR:", 6) == 0 || strncmp(err_check, "WARNING:", 8) == 0) {
                        printf("%s", err_check);
                    }
                }
                
                // reset to blocking
                tv.tv_sec = 0;
                tv.tv_usec = 0;
                setsockopt(ss_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
            }
            
            // receive final response
            char ss_response[BUF];
            n = recv(ss_fd, ss_response, sizeof(ss_response) - 1, 0);
            if (n > 0) {
                ss_response[n] = '\0';
                printf("%s", ss_response);
            }
            
            close(ss_fd);
            continue;
        }

         // handle STREAM command
        if (strncmp(cmd, "STREAM ", 7) == 0) {
            char filename[MAX_FILENAME];
            if (sscanf(cmd + 7, "%127s", filename) != 1) {
                printf("Usage: STREAM <filename>\n");
                continue;
            }
            
            // send STREAM request to NM
            send(sock, cmd, strlen(cmd), 0);
            
            // receive SS info from NM
            char response[256];
            n = recv(sock, response, sizeof(response) - 1, 0);
            if (n <= 0) {
                printf("ERROR: Connection lost.\n");
                break;
            }
            response[n] = '\0';
            
            // check for error
            if (strncmp(response, "ERROR:", 6) == 0) {
                printf("%s", response);
                continue;
            }
            
            // parse SS IP and port
            char ss_ip[64];
            int ss_port;
            if (parse_ss_info(response, ss_ip, &ss_port) != 0) {
                printf("ERROR: Invalid response from Name Server.\n");
                continue;
            }
            
            // connect to SS
            int ss_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (ss_fd < 0) {
                printf("ERROR: Cannot create socket for SS connection.\n");
                continue;
            }
            
            struct sockaddr_in ss_addr;
            ss_addr.sin_family = AF_INET;
            ss_addr.sin_port = htons(ss_port);
            if (inet_pton(AF_INET, ss_ip, &ss_addr.sin_addr) != 1) {
                close(ss_fd);
                printf("ERROR: Invalid SS address.\n");
                continue;
            }
            
            if (connect(ss_fd, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) != 0) {
                close(ss_fd);
                printf("ERROR: Cannot connect to Storage Server at %s:%d.\n", ss_ip, ss_port);
                continue;
            }
            
            // send STREAM request to SS
            char stream_cmd[512];
            snprintf(stream_cmd, sizeof(stream_cmd), "STREAM %s %s\n", filename, username);
            send(ss_fd, stream_cmd, strlen(stream_cmd), 0);
            
            // receive and display streaming content (word by word with delays)
            printf("Streaming %s:\n", filename);
            char buffer[BUF];
            while (1) {
                n = recv(ss_fd, buffer, sizeof(buffer) - 1, 0);
                if (n <= 0) break;
                buffer[n] = '\0';
                printf("%s", buffer);
                fflush(stdout);
            }
            printf("\n");
            
            close(ss_fd);
            continue;
        }
        
        // handle UNDO command
        if (strncmp(cmd, "UNDO ", 5) == 0) {
            char filename[MAX_FILENAME];
            if (sscanf(cmd + 5, "%127s", filename) != 1) {
                printf("Usage: UNDO <filename>\n");
                continue;
            }
            
            // send UNDO request to NM
            send(sock, cmd, strlen(cmd), 0);
            
            // receive SS info from NM
            char response[256];
            n = recv(sock, response, sizeof(response) - 1, 0);
            if (n <= 0) {
                printf("ERROR: Connection lost.\n");
                break;
            }
            response[n] = '\0';
            
            // check for error
            if (strncmp(response, "ERROR:", 6) == 0) {
                printf("%s", response);
                continue;
            }
            
            // parse SS IP and port
            char ss_ip[64];
            int ss_port;
            if (parse_ss_info(response, ss_ip, &ss_port) != 0) {
                printf("ERROR: Invalid response from Name Server.\n");
                continue;
            }
            
            // connect to SS
            int ss_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (ss_fd < 0) {
                printf("ERROR: Cannot create socket for SS connection.\n");
                continue;
            }
            
            struct sockaddr_in ss_addr;
            ss_addr.sin_family = AF_INET;
            ss_addr.sin_port = htons(ss_port);
            if (inet_pton(AF_INET, ss_ip, &ss_addr.sin_addr) != 1) {
                close(ss_fd);
                printf("ERROR: Invalid SS address.\n");
                continue;
            }
            
            if (connect(ss_fd, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) != 0) {
                close(ss_fd);
                printf("ERROR: Cannot connect to Storage Server at %s:%d.\n", ss_ip, ss_port);
                continue;
            }
            
            // send UNDO request to SS
            char undo_cmd[512];
            snprintf(undo_cmd, sizeof(undo_cmd), "UNDO %s %s\n", filename, username);
            send(ss_fd, undo_cmd, strlen(undo_cmd), 0);
            
            // receive response
            char ss_response[BUF];
            n = recv(ss_fd, ss_response, sizeof(ss_response) - 1, 0);
            if (n > 0) {
                ss_response[n] = '\0';
                if (strncmp(ss_response, "OK", 2) == 0) {
                    printf("File '%s' successfully restored from backup.\n", filename);
                } else {
                    printf("%s", ss_response);
                }
            }
            
            close(ss_fd);
            continue;
        }

        send(sock, cmd, strlen(cmd), 0);

        // For VIEW commands, we may receive multi-line responses
        // Receive all data until no more is available
        char full_response[8192] = {0};
        int total_received = 0;
        
        // Set a short timeout for recv to detect end of transmission
        struct timeval tv;
        tv.tv_sec = 2;  // 2 second timeout for normal commands
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        
        while (total_received < sizeof(full_response) - 1) {
            n = recv(sock, msg, sizeof(msg)-1, 0);
            if (n <= 0) {
                if (total_received == 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    // Timeout with no data received
                    printf("NM> (waiting for response...)\n");
                    // Try again with longer timeout
                    tv.tv_sec = 5;
                    tv.tv_usec = 0;
                    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
                    continue;
                }
                if (total_received == 0) {
                    printf("NM disconnected.\n");
                    // Reset timeout before breaking
                    tv.tv_sec = 0;
                    tv.tv_usec = 0;
                    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
                    break;
                }
                // Timeout or error, but we got some data, so continue
                break;
            }
            
            // Copy received data to full_response
            if (total_received + n < sizeof(full_response)) {
                memcpy(full_response + total_received, msg, n);
                total_received += n;
            } else {
                // Buffer full, copy what fits
                int remaining = sizeof(full_response) - total_received - 1;
                memcpy(full_response + total_received, msg, remaining);
                total_received += remaining;
                break;
            }
            
            // For non-VIEW/EXEC commands, break after first recv
            if (strncmp(cmd, "VIEW", 4) != 0 && strncmp(cmd, "EXEC", 4) != 0) {
                break;
            }
        }
        
        // Reset timeout to blocking mode
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        
        if (total_received > 0) {
            full_response[total_received] = '\0';
            printf("NM> %s", full_response);
        }
    }

    close(sock);
    return 0;
}