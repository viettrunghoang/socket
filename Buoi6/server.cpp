#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <strings.h> 

#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024
#define PORT 65432

struct Client {
    int fd;
};

void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len-1] == '\n') str[len-1] = '\0';
    if (len > 1 && str[len-2] == '\r') str[len-2] = '\0';
}

void encrypt_string(char *text) {
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c >= 'a' && c <= 'z') {
            text[i] = (c == 'z') ? 'a' : c + 1;
        } else if (c >= 'A' && c <= 'Z') {
            text[i] = (c == 'Z') ? 'A' : c + 1;
        } else if (c >= '0' && c <= '9') {
            text[i] = '9' - (c - '0');
        }
    }
}

int main() {
    int server_fd, new_socket, max_sd, activity;
    int client_count = 0;
    struct sockaddr_in address;
    fd_set readfds;
    
    struct Client clients[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = 0;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server dang chay tai port: %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].fd;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            int addrlen = sizeof(address);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            client_count++;
            printf("Ket noi moi! IP: %s, Port: %d. Tong client: %d\n", 
                   inet_ntoa(address.sin_addr), ntohs(address.sin_port), client_count);

            char welcome_msg[BUFFER_SIZE];
            snprintf(welcome_msg, sizeof(welcome_msg), "Xin chao. Hien co %d clients dang ket noi.\n", client_count);
            send(new_socket, welcome_msg, strlen(welcome_msg), 0);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].fd;

            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                char buffer[BUFFER_SIZE] = {0};
                int valread = read(sd, buffer, BUFFER_SIZE - 1);

                if (valread == 0) {
                    socklen_t addrlen = sizeof(address); 
                    getpeername(sd, (struct sockaddr*)&address, &addrlen);
                    
                    client_count--;
                    printf("Client ngat ket noi! IP: %s, Port: %d. Tong client: %d\n", 
                            inet_ntoa(address.sin_addr), ntohs(address.sin_port), client_count);
                    
                    close(sd);
                    clients[i].fd = 0;
                } 
                else {
                    buffer[valread] = '\0';
                    trim_newline(buffer);

                    if (strlen(buffer) == 0) continue;

                    if (strcasecmp(buffer, "exit") == 0) {
                        const char *goodbye = "Tam biet!\n";
                        send(sd, goodbye, strlen(goodbye), 0);
                        
                        client_count--;
                        printf("Client da gui 'exit' va thoat. Tong client: %d\n", client_count);
                        
                        close(sd);
                        clients[i].fd = 0;
                    } 

                    else {
                        encrypt_string(buffer);
                        char response[BUFFER_SIZE + 2];
                        snprintf(response, sizeof(response), "%s\n", buffer);
                        
                        send(sd, response, strlen(response), 0);
                    }
                }
            }
        }
    }
    return 0;
}