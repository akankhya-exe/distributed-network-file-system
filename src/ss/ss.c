#include "common.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>


static int ss_register_once(void){
    int fd = nm_connect();
    if (fd < 0){
        fprintf(stdout, "[SS] Failed to connect to NM at %s:%d\n", NM_IP, NM_PORT);
        return -1;
    }

    char ip[64];
    get_local_ip(ip);

    char files[1024][256];
    int nfiles = list_files(files, 1024);

    dprintf(fd,
            "SS_REGISTER\n"
            "IP: %s\n"
            "CLIENT_PORT: %d\n"
            "FILES: %d\n"
            "END\n",
            ip, SS_CLIENT_PORT, nfiles);
    fflush(stdout);

    char buf[256];
    int r = recv(fd, buf, sizeof(buf) - 1, 0);

    if (r <= 0) {
        fprintf(stdout, "[SS] No response from NM.\n");
        close(fd);
        return -1;
    }

    buf[r] = '\0';
    if (strncmp(buf, "OK", 2) == 0){
        fprintf(stdout, "[SS] Registration successful with NM (%s:%d)\n", NM_IP, NM_PORT);
        close(fd);
        return 0;
    }
    else {
        fprintf(stdout, "[SS] Unexpected NM reply: %s\n", buf);
        close(fd);
        return -1;
    }

}

// runs as a thread in the background so ss can continue other tasks
static void* register_thread(void* arg) {
    (void)arg;
    int backoff_ms = 500;
    const int max_backoff_ms = 8000;

    while (1){
        if (ss_register_once()==0){
            fprintf(stdout, "[SS] Registered successfully with NM.\n");
            fflush(stdout);
            break;
        }

        fprintf(stdout, "[SS] Registration failed, retrying in %d ms\n", backoff_ms);
        fflush(stdout);
        usleep(backoff_ms*1000);
        if (backoff_ms < max_backoff_ms){ // exponential backoff to prevent bombarding the nm with requests
            backoff_ms *= 2;
        }
    }

    return NULL;
}