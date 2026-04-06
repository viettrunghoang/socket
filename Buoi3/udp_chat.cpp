#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in my_addr, dest_addr;
    char buffer[BUF_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(port_s);

    bind(sockfd, (const struct sockaddr *)&my_addr, sizeof(my_addr))

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port_d);
    inet_pton(AF_INET, ip_d, &dest_addr.sin_addr) 

    fd_set readfds;
    int max_fd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

    printf("Chat start\n");
    printf("Listen port: %d\n", port_s);
    printf("Address: %s:%d\n", ip_d, port_d);
    printf("Chat: \n\n");
    printf("You: ");
    fflush(stdout);

    while (1) {
        FD_ZERO(&readfds);
        
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);


        if (FD_ISSET(sockfd, &readfds)) {
            struct sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);
            int n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0, (struct sockaddr *)&sender_addr, &sender_len);
            
            if (n > 0) {
                buffer[n] = '\0';
                printf("\r[Reicept from %s:%d]: %s", inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port), buffer);
                printf("You: ");
                fflush(stdout);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buffer, BUF_SIZE, stdin) != NULL) {
                sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&dest_addr, sizeof(dest_addr));
                printf("You: ");
                fflush(stdout);
            }
        }
    }

    close(sockfd);
    return 0;
}