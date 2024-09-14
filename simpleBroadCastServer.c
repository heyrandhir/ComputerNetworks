#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT_TO_LISTEN "3490"
#define MAX_BUFFER_SIZE 1024

void capitalize_string(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

int main() {
    int sockfd;
    struct addrinfo hints, *res, *p;
    int status;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char buffer[MAX_BUFFER_SIZE];
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  
    hints.ai_socktype = SOCK_DGRAM;  
    hints.ai_flags = AI_PASSIVE;  

    if ((status = getaddrinfo(NULL, PORT_TO_LISTEN, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // Loop through all the results and bind to the first we can
    for(p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(res);  

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    printf("server: waiting for data...\n");

    while(1) { 
        addr_len = sizeof their_addr;
        int bytes_received = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE-1, 0,
                                      (struct sockaddr *)&their_addr, &addr_len);
        if (bytes_received == -1) {
            perror("recvfrom");
            continue;
        }

        buffer[bytes_received] = '\0';

        // Get the sender's address
        void *addr;
        if (their_addr.ss_family == AF_INET) {
            addr = &(((struct sockaddr_in*)&their_addr)->sin_addr);
        } else {
            addr = &(((struct sockaddr_in6*)&their_addr)->sin6_addr);
        }
        inet_ntop(their_addr.ss_family, addr, s, sizeof s);

        printf("server: got packet from %s\n", s);
        printf("server: packet contains \"%s\"\n", buffer);

        capitalize_string(buffer);

        printf("server: capitalized message: \"%s\"\n", buffer);

        if (sendto(sockfd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&their_addr, addr_len) == -1) {
            perror("sendto");
        }
    }

    close(sockfd);

    return 0;
}