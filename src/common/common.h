# ifndef COMMON_H
# define COMMON_H

// Common includes
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <errno.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <pthread.h>
# include <time.h>

// Constants
# define MAX_FILES 100
# define MAX_USERS 50
# define MAX_FILENAME 128
# define MAX_USERNAME 64
# define MAX_COMMAND 512
# define MAX_RESPONSE 2048
# define PORT_NM 8080 // default name server port
# define PORT_SS_BASE 9000 // default storage server base port
# define PORT_CLIENT_BASE 10000 // client base port

# define BACKLOG 10 // length of queue for socket listen
# define CACHE_SIZE 16 // max cache capacity for LRU used for file searching

// Folder structure constants
# define MAX_FOLDERS 100
# define MAX_FOLDERNAME 128
# define MAX_PATH 256

// Checkpoint constants
# define MAX_CHECKPOINTS 1000
# define MAX_CHECKPOINT_TAG 64

// Access request constants
# define MAX_ACCESS_REQUESTS 500

// Operation codes to be shared between NM, SS and Clients

typedef enum{
    OP_CREATE, OP_READ, OP_VIEW, OP_DELETE, OP_INFO, OP_WRITE, OP_UNDO, OP_STREAM, OP_LIST,
    OP_ACCESS_ADD_R, OP_ACCESS_ADD_W, OP_ACCESS_REM, OP_EXEC, OP_LIST_USERS, OP_CREATEFOLDER, OP_VIEWFOLDER,
    OP_VIEWFOLDER, OP_MOVE, OP_CHECKPOINT, OP_VIEWCHECKPOINT, OP_REVERT, OP_LISTCHECKPOINTS,
    OP_REQUESTACCESS, OP_VIEWREQUESTS, OP_APPROVEREQUEST, OP_DENYREQUEST, OP_UNKNOWN
} OperationCode;

// Error codes
typedef enum{
    ERR_SUCCESS = 0, ERR_FILE_NOT_FOUND = 1, ERR_ACCESS_DENIED = 2, ERR_FILE_EXISTS = 3,
    ERR_STORAGE_FULL = 4, ERR_INVALID_ARGS = 5, ERR_SENTENCE_LOCKED = 6, ERR_INDEX_OUT_OF_RANGE = 7,
    ERR_IO_ERROR = 8, ERR_SS_UNAVAILABLE = 9, ERR_FOLDER_NOT_FOUND = 10, ERR_FOLDER_EXISTS = 11,
    ERR_CHECKPOINT_NOT_FOUND = 12, ERR_CHECKPOINT_EXISTS = 13, ERR_REQUEST_NOT_FOUND = 14,
    ERR_REQUEST_EXISTS = 15, ERR_UNKNOWN = 99
} ErrorCode;

// Access Levels
typedef enum{
    ACCESS_NONE = 0, ACCESS_READ = 1, ACCESS_WRITE = 2, ACCESS_RW = 3
} AccessLevel;

// Data structures

// 1. Sentence lock structure for concurrent WRITE operations
typedef struct{
    int sentence_index;
    char username[MAX_USERNAME];
    time_t lock_time;
    int active; // 1 if locked, 0 if free
} SentenceLock;

// 2. File metadata stored at NM
typedef struct{
    char filename[MAX_FILENAME];
    char owner[MAX_USERNAME];
    int ss_port; // To indicate which storage server holds it
    time_t created_at;
    time_t last_modified;
    time_t last_accessed;
    size_t size_bytes;
    int word_count;
    int char_count;
    char folder_path[MAX_PATH]; // absolute path, "/" for root

    // Access control list
    struct{
        char username[MAX_USERNAME];
        AccessLevel access;
    } acl[MAX_USERS];

    int acl_count;
} FileMeta;

// 3. Folder ""
typedef struct{
    char foldername[MAX_FOLDERNAME];
    char owner[MAX_USERNAME];
    char parent_path[MAX_PATH]; // parent folder path
    char full_path[MAX_PATH]; // path of current folder
    time_t created_at;
    int active; // 1 if active, 0 if deleted
} FolderMeta;

// 4. Checkpoint ""
typedef struct{
    char filename[MAX_FILENAME];
    char checkpoint_tag[MAX_CHECKPOINT_TAG];
    char username[MAX_USERNAME]; // Creator of checkpoint
    time_t created_at;
    int ss_port; // Storage server holding the checkpoint
    int active; // 1 if exists, 0 if deleted
} CheckpointMeta;

// 5. Access request metadata stored at the NM
typedef struct{
    char filename[MAX_FILENAME];
    char requester[MAX_USERNAME]; // User requesting access
    AccessLevel access_type; // READ or WRITE access requested
    time_t requested_at;
    int pending; // 1 if pending, 0 if processed
} AccessRequest;

// 6. Connected storage servers
typedef struct{
    char ip[INET_ADDRSTRLEN];
    int port;
    int client_port; // port for clients to connected 
    int active;
    char storage_path[256]; // base path for files
} StorageServerInfo;

// 7. Connected clients
typedef struct{
    char username[MAX_USERNAME];
    char ip[INET_ADDRSTRLEN];
    int port;
    int active;
    time_t connected_at;
} ClientInfo;

// Message protocol

// 1. Simple text-based communication protocol
typedef struct{
    char username[MAX_USERNAME];
    OperationCode opcode;
    char filename[MAX_FILENAME];
    char data[MAX_RESPONSE];
} Message;

// 2. Storage server Response/Request structure
typedef struct{
    char command[32]; // "READ", "WRITE"...etc.
    char filename[MAX_FILENAME];
    char username[MAX_USERNAME];
    int sentence_number; // for WRITE
    int word_index; // for WRITE
} SSRequest;

typedef struct{
    int success; // 1 for success, 0 for error
    int error_code; // ErrorCode enum value
    char message[256]; // Error message for status
    size_t data_len;
    char data[MAX_RESPONSE];
} SSResponse;

// 3. WRITE opeartion update structure
typedef struct{
    int word_index;
    char content[512];
} WriteUpdate;

// HashMap for O(1) lookups
# define HASHMAP_SIZE 256

typedef struct HashNode{
    char filename[MAX_FILENAME];
    int file_index; // Index in files[] array
    struct HashNode *next; // Collision handling
} HashNode;

// LRU cache
typedef struct CacheEntry{
    char filename[MAX_FILENAME];
    int file_index;
    time_t access_time;
    int valid; // 1 if valid, 0 if invalid
} CacheEntry;

// Function Prototypes

int create_server_socket(int port);
int accept_connection(int server_fd);
int send_message(int sockfd, Message *msg);
int recv_message(int sockfd, Message *msg);
void log_event(const char *source, const char *event);
void log_event_with_client(const char *source, const char *client_ip, int client_port, const char *event);
const char* get_timestamp();
const char* opcode_to_string(OperationCode code);
AccessLevel parse_access_flag(const char *flag);
const char* error_code_to_string(ErrorCode code);

#endif