#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    unsigned long ul = 1;
    if (ioctl(sockfd, FIONBIO, &ul) < 0) {
        perror("ioctl FIONBIO failed");
        exit(1);
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in servaddr = {0}, destaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port_s);
    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(port_d);
    inet_pton(AF_INET, ip_d, &destaddr.sin_addr);

    char buffer[1024];
    printf("UDP Non-blocking Chat started...\n");

    while (1) {
        struct sockaddr_in remote_addr;
        socklen_t addr_len = sizeof(remote_addr);
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, 
                         (struct sockaddr *)&remote_addr, &addr_len);
        
        if (n > 0) {
            buffer[n] = '\0';
            printf("\rReceived: %s", buffer);
            printf("You: "); fflush(stdout);
        } else if (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("recvfrom error");
            break;
        }

        if (fgets(buffer, sizeof(buffer), stdin)) {
            sendto(sockfd, buffer, strlen(buffer), 0, 
                   (struct sockaddr *)&destaddr, sizeof(destaddr));
            printf("You: "); fflush(stdout);
        }

    }

    close(sockfd);
    return 0;
}
