#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024
#define PORT 8080

struct Client {
    int fd;
    int is_logged_in;
    char id[32];
    char name[32];
};

void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len-1] == '\n') str[len-1] = '\0';
    if (len > 1 && str[len-2] == '\r') str[len-2] = '\0';
}

int main() {
    int server_fd, new_socket, max_sd, activity;
    struct sockaddr_in address;
    fd_set readfds;
    
    struct Client clients[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = 0;
        clients[i].is_logged_in = 0;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0)

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

  
    bind(server_fd, (struct sockaddr *)&address, sizeof(address))

    // if (listen(server_fd, 10) < 0) {
    //     perror("Listen failed");
    //     exit(EXIT_FAILURE);
    // }

    printf("Server run at port:  %d...\n", PORT);

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
        if ((activity < 0)) {
            perror("Select error");
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            int addrlen = sizeof(address);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New connection, socket fd is %d, ip is: %s, port: %d\n", 
                   new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            char *welcome = "Vui long nhap cu phap\n ";
            send(new_socket, welcome, strlen(welcome), 0);

            // Thêm socket mới vào mảng clients
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    clients[i].is_logged_in = 0;
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
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&sizeof(address));
                    printf("Host disconnected, ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    clients[i].fd = 0;
                    clients[i].is_logged_in = 0;
                } 
                else {
                    buffer[valread] = '\0';
                    trim_newline(buffer);

                    if (strlen(buffer) == 0) continue;
                    if (!clients[i].is_logged_in) {
                        char temp_id[32], temp_name[32];
                        
                        if (sscanf(buffer, "%31[^:]: %31s", temp_id, temp_name) == 2) {
                            strcpy(clients[i].id, temp_id);
                            strcpy(clients[i].name, temp_name);
                            clients[i].is_logged_in = 1;
                            
                            char *success_msg = "Thanh cong.\n> ";
                            send(sd, success_msg, strlen(success_msg), 0);
                            printf("Client logged in: ID=%s, Name=%s\n", clients[i].id, clients[i].name);
                        } else {
                            char *error_msg = "Khong thanh cong\n> ";
                            send(sd, error_msg, strlen(error_msg), 0);
                        }
                    } 
                    else {
                        time_t t = time(NULL);
                        struct tm *tm_info = localtime(&t);
                        char time_str[64];
                        strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", tm_info);

                        char send_buffer[BUFFER_SIZE + 128];
                        snprintf(send_buffer, sizeof(send_buffer), "%s %s: %s\n> ", time_str, clients[i].id, buffer);

                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].fd > 0 && clients[j].is_logged_in && clients[j].fd != sd) {
                                send(clients[j].fd, send_buffer, strlen(send_buffer), 0);
                            }
                        }
                        
                        char *prompt = "> ";
                        send(sd, prompt, strlen(prompt), 0);
                    }
                }
            }
        }
    }
    return 0;
}

