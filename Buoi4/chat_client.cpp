#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nLoi: Dia chi IP khong hop le\n");
        return -1;
    }

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    printf("---Da ket noi---\n");

    fd_set readfds;
    int max_sd;

    while (1) {

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        FD_SET(sock, &readfds);
        max_sd = sock > STDIN_FILENO ? sock : STDIN_FILENO;

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("Select error");
            break;
        }

        if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(sock, buffer, BUFFER_SIZE - 1);
            if (valread == 0) {
                printf("\nServer da dong ket noi.\n");
                break;
            } else if (valread < 0) {
                perror("Read error");
                break;
            }
            
            printf("%s", buffer);
            fflush(stdout); 
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                send(sock, buffer, strlen(buffer), 0);
            }
        }
    }

    close(sock);
    return 0;
}