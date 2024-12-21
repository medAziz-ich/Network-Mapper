#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>

typedef unsigned int in_addr_t;
typedef unsigned short int int16;
typedef unsigned char int8;

in_addr_t current, ending;
int16 port;

in_addr_t generate() {
    if (current >= ending) {
        return 0;
    } else {
        current++;
    }
    return current;
}

bool tcpconnect(in_addr_t ip, int16 port) {
    struct sockaddr_in sock;
    int s, ret;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("Socket creation failed");
        return false;
    }

    sock.sin_family = AF_INET;
    sock.sin_port = htons(port);
    sock.sin_addr.s_addr = ip;

    ret = connect(s, (struct sockaddr *)&sock, sizeof(sock));
    if (ret == -1) {
        // Connection failed
        close(s);
        return false;
    } else {
        // Connection successful
        printf("Connection successful to IP: %s on port %d\n", inet_ntoa(*(struct in_addr *)&ip), port);
        close(s);
        return true;
    }
}

int main(int argc, char *argv[]) {
    in_addr_t ip;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <port> [start ip addr] [end ip addr]\n", argv[0]);
        return -1;
    }

    port = (int16)atoi(argv[1]);

    if (argc > 2) {
        current = inet_addr(argv[2]);
        ending = (argc > 3) ? inet_addr(argv[3]) : inet_addr("253.255.254.255");

        if ((current == INADDR_NONE) || (ending == INADDR_NONE) || (ending < current)) {
            fprintf(stderr, "usage: %s <port> [start ip addr] [end ip addr]\n", argv[0]);
            return -1;
        }
    } else {
        current = inet_addr("0.0.0.0");
        ending = inet_addr("255.255.255.255");
    }

    printf("Port: %d\n", port);
    printf("Start IP: %s\n", inet_ntoa(*(struct in_addr *)&current));
    printf("End IP: %s\n", inet_ntoa(*(struct in_addr *)&ending));

    printf("Scanning from %s to %s on port %d...\n", inet_ntoa(*(struct in_addr *)&current),
           inet_ntoa(*(struct in_addr *)&ending), port);

    while ((ip = generate())) {
        tcpconnect(ip, port);
    }

    printf("Scan complete.\n");

    return 0;
}
