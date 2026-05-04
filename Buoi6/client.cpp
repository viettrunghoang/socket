#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 65432
#define BUFFER_SIZE 1024

void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len-1] == '\n') str[len-1] = '\0';
    if (len > 1 && str[len-2] == '\r') str[len-2] = '\0';
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        exit(EXIT_FAILURE);
    }

    memset(buffer, 0, BUFFER_SIZE);
    read(client_socket, buffer, BUFFER_SIZE - 1);
    printf("Server: %s", buffer);

    while (1) {
        printf("Nhap chuoi (hoac 'exit' de thoat): ");
        memset(buffer, 0, BUFFER_SIZE);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        trim_newline(buffer);
        if (strlen(buffer) == 0) continue;

        send(client_socket, buffer, strlen(buffer), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE - 1);
        
        if (valread <= 0) {
            printf("Ket noi da bi dong.\n");
            break;
        }

        printf("Server phan hoi: %s", buffer);

        if (strncmp(buffer, "Tam biet!", 9) == 0) {
            break;
        }
    }

    close(client_socket);
    return 0;
}