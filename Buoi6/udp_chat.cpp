#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <strings.h>

#define BUFFER_SIZE 1024


int main(int argc, char *argv[]) {
    int local_port = atoi(argv[1]);
    char *remote_ip = argv[2];
    int remote_port = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in local_addr, remote_addr;
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(local_port);

    if (bind(sockfd, (const struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind port error");
        exit(1);
    }

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port);
    if (inet_pton(AF_INET, remote_ip, &remote_addr.sin_addr) <= 0) {
        perror("IP error");
        exit(1);
    }

    printf("Using port %d.\n", local_port);
    printf("> ");
    fflush(stdout);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);

        int max_fd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) break;

        if (FD_ISSET(sockfd, &readfds)) {
            struct sockaddr_in sender_addr;
            socklen_t len = sizeof(sender_addr);
            
            memset(buffer, 0, BUFFER_SIZE);
            int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&sender_addr, &len);
            if (n > 0) {
                buffer[n] = '\0';

                printf("Bạn bè: %s\n", buffer);
                
                if (strcasecmp(buffer, "exit") == 0) {
                    printf("Bạn bè đã thoát.\n");
                    break;
                }
                printf("> ");
                fflush(stdout); 
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                
                if (strlen(buffer) == 0) {
                    printf("> ");
                    fflush(stdout);
                    continue;
                }

                sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr));

                if (strcasecmp(buffer, "exit") == 0) break;
                
                printf("> ");
                fflush(stdout);
            }
        }
    }

    close(sockfd);
    return 0;
}