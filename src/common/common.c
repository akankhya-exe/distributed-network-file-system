# include "common.h"
# include <sys/stat.h>
# include <time.h>
# include <stdio.h>
# include <stdlib.h>

const char* get_timestamp(){
    static char buffer[64];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);
    return buffer;
}

void log_event(const char *component, const char *message) {
    mkdir("logs", 0777);
    char filename[64];
    snprintf(filename, sizeof(filename), "logs/%s.log", component);

    FILE *fp = fopen(filename, "a");
    if (!fp) {
        perror("log_event fopen");
        return;
    }

    const char *timestamp = get_timestamp();
    fprintf(fp, "[%s] [%s] %s\n", timestamp, component, message);
    
    // Print to terminal
    printf("[%s] [%s] %s\n", timestamp, component, message);
    
    fclose(fp);
}

void log_event_with_client(const char *component, const char *client_ip, int client_port, const char *message) {
    mkdir("logs", 0777);
    char filename[64];
    snprintf(filename, sizeof(filename), "logs/%s.log", component);

    FILE *fp = fopen(filename, "a");
    if (!fp) {
        perror("log_event_with_client fopen");
        return;
    }

    const char *timestamp = get_timestamp();
    fprintf(fp, "[%s] [%s] [Client: %s:%d] %s\n", timestamp, component, client_ip, client_port, message);
    
    printf("[%s] [%s] [Client: %s:%d] %s\n", timestamp, component, client_ip, client_port, message);
    
    fclose(fp);
}

const char* error_code_to_string(ErrorCode code) {
    switch (code) {
        case ERR_SUCCESS: return "Success";
        case ERR_FILE_NOT_FOUND: return "File not found";
        case ERR_ACCESS_DENIED: return "Access denied";
        case ERR_FILE_EXISTS: return "File already exists";
        case ERR_STORAGE_FULL: return "Storage full";
        case ERR_INVALID_ARGS: return "Invalid arguments";
        case ERR_SENTENCE_LOCKED: return "Sentence locked by another user";
        case ERR_INDEX_OUT_OF_RANGE: return "Index out of range";
        case ERR_IO_ERROR: return "I/O error";
        case ERR_SS_UNAVAILABLE: return "Storage server unavailable";
        case ERR_FOLDER_NOT_FOUND: return "Folder not found";
        case ERR_FOLDER_EXISTS: return "Folder already exists";
        case ERR_CHECKPOINT_NOT_FOUND: return "Checkpoint not found";
        case ERR_CHECKPOINT_EXISTS: return "Checkpoint already exists";
        case ERR_REQUEST_NOT_FOUND: return "Access request not found";
        case ERR_REQUEST_EXISTS: return "Access request already exists";
        default: return "Unknown error";
    }
}

AccessLevel parse_access_flag(const char *flag) {
    if (strcmp(flag, "-R") == 0 || strcmp(flag, "-r") == 0) {
        return ACCESS_READ;
    } else if (strcmp(flag, "-W") == 0 || strcmp(flag, "-w") == 0) {
        return ACCESS_WRITE;
    } else if (strcmp(flag, "-RW") == 0 || strcmp(flag, "-rw") == 0) {
        return ACCESS_RW;
    }
    return ACCESS_NONE;
}