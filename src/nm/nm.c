#include "common.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>

#define MAX_FILES 100
#define MAX_STORAGE_SERVERS 10
#define MAX_CLIENTS 50
#define MAX_REGISTERED_USERS 100

static StorageServerInfo storage_servers[MAX_STORAGE_SERVERS];
static int ss_count = 0;


void handle_ss_register(int client_fd){
    char buf[2048];
    ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0){
        close(client_fd);
        return;
    }

    buf[n] = '\0';
    printf("[NM] Received SS registration request:\n%s\n", buf);

    char ip[INET_ADDRSTRLEN] = "127.0.0.1";
    int client_port = 0;
    int nfiles = 0;

    char *line = strtok(buf, '\n');
    while (line != NULL){
        if (strncmp(line, "IP:", 3) == 0){
            sscanf(line + 4, "%15s", ip);
        }else if (strncmp(line, "CLIENT_PORT:", 12) == 0){
            sscanf(line + 13, "%d", &client_port);
        }else if (strncmp(line, "FILES:", 6) == 0){
            sscanf(line + 7, "%d", &nfiles);
        }

        line = strtok(NULL, "\n");
    }

    // mutex lock to make sure storage server list update has no race conditions
    pthead_mutex_lock(&lock);
    
    // check if ss already registered
    int ss_index = -1;
    for(int i = 0; i < ss_count; i++){
        if(storage_servers[i].client_port == client_port){
            ss_index = i;
            break;
        }
    }

    if (ss_index == -1 && ss_count < MAX_STORAGE_SERVERS) {
        ss_index = ss_count++;
    }

    if (ss_index != -1){
        strncpy(storage_servers[ss_index].ip, ip, INET_ADDRSTRLEN);
        storage_servers[ss_index].port = client_port; // same port for now, both external everyday users and nm communication are on on port, port separation later
        storage_servers[ss_index].client_port = client_port;
        storage_servers[ss_index].active = 1;
        snprintf(storage_servers[ss_index].storage_path, 256, "./data");
        
        printf("[NM] Registered SS: %s:%d (files: %d)\n", ip, client_port, nfiles);
    }

    pthread_mutex_unlock(&lock);
    
    dprintf(client_fd, "OK\n");
    printf("[NM] Sent OK to storage server\n");
    
    char logmsg[256];
    snprintf(logmsg, sizeof(logmsg), "SS_REGISTER %s:%d", ip, client_port);
    log_event("NM", logmsg);
    
    close(client_fd);
}