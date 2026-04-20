#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr)
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    struct pollfd fds[2];
    
    fds[0].fd = STDIN_FILENO; 
    fds[0].events = POLLIN;

    fds[1].fd = sock;         
    fds[1].events = POLLIN;

    while (1) {
        int poll_count = poll(fds, 2, -1);
        if (poll_count < 0) {
            perror("Poll error");
            break;
        }

        if (fds[0].revents & POLLIN) {
            if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {

                send(sock, buffer, strlen(buffer), 0);
            }
        }
        if (fds[1].revents & POLLIN) {
            int bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            
            if (bytes_read == 0) {
                printf("\nServer da ngat ket noi.\n");
                break;
            } else if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("%s", buffer);
            } else {
                perror("Loi nhan du lieu");
                break;
            }
        }
    }

    // Đóng socket khi thoát
    close(sock);
    return 0;
}