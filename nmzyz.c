#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#define MAX_THREADS 10

typedef struct {
    in_addr_t ip;
    int16_t port;
} ScanTask;

void *scan_port(void *arg) {
    ScanTask *task = (ScanTask *)arg;
    struct sockaddr_in sock;
    int s, ret;
    char ip_str[INET_ADDRSTRLEN];
    
    inet_ntop(AF_INET, &task->ip, ip_str, INET_ADDRSTRLEN);
    
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return NULL;
    }

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
    
    sock.sin_family = AF_INET;
    sock.sin_port = htons(task->port);
    sock.sin_addr.s_addr = task->ip;
    
    ret = connect(s, (struct sockaddr *)&sock, sizeof(sock));
    
    if (ret == 0) {
        printf("[+] Open port: %s:%d\n", ip_str, task->port);
    }
    
    close(s);
    free(task);
    return NULL;
}

void scan_range(in_addr_t start_ip, in_addr_t end_ip, int16_t start_port, int16_t end_port) {
    pthread_t threads[MAX_THREADS];
    int thread_count = 0;

    for (in_addr_t ip = ntohl(start_ip); ip <= ntohl(end_ip); ip++) {
        for (int16_t port = start_port; port <= end_port; port++) {
            ScanTask *task = malloc(sizeof(ScanTask));
            task->ip = htonl(ip);
            task->port = port;
            
            pthread_create(&threads[thread_count++], NULL, scan_port, task);
            
            if (thread_count >= MAX_THREADS) {
                for (int i = 0; i < MAX_THREADS; i++) {
                    pthread_join(threads[i], NULL);
                }
                thread_count = 0;
            }
        }
    }
    
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <start IP> <end IP> <start port> <end port>\n", argv[0]);
        return -1;
    }
    
    struct in_addr start_addr, end_addr;
    
    if (inet_pton(AF_INET, argv[1], &start_addr) != 1 ||
        inet_pton(AF_INET, argv[2], &end_addr) != 1) {
        fprintf(stderr, "Invalid IP address format.\n");
        return -1;
    }
    
    int16_t start_port = atoi(argv[3]);
    int16_t end_port = atoi(argv[4]);
    
    if (start_port <= 0 || end_port <= 0 || start_port > end_port) {
        fprintf(stderr, "Invalid port range.\n");
        return -1;
    }
    
    scan_range(start_addr.s_addr, end_addr.s_addr, start_port, end_port);
    return 0;
}
