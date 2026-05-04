#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_CLIENTS 50
#define BUFFER_SIZE 2048
#define TOPIC_SIZE 64
#define PORT 9000


struct Client {
    int fd;
    char topic[TOPIC_SIZE];
};

int main() {
    int server_fd, new_socket, max_sd, activity;
    struct sockaddr_in address;
    fd_set readfds;
    
    struct Client clients[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = 0;
        memset(clients[i].topic, 0, TOPIC_SIZE);
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

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

            printf("[+] Client ket noi tu IP: %s\n", inet_ntoa(address.sin_addr));
            
            const char *welcome = "Ket noi thanh cong. Hay gui lenh SUB hoac PUB.\n> ";
            send(new_socket, welcome, strlen(welcome), 0);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    memset(clients[i].topic, 0, TOPIC_SIZE);
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
                    printf("[-] Client (FD: %d) da ngat ket noi.\n", sd);
                    close(sd);
                    clients[i].fd = 0;
                    memset(clients[i].topic, 0, TOPIC_SIZE);
                } 
                else {
                    buffer[valread] = '\0';
                    if (strlen(buffer) == 0) continue;

                    char response[BUFFER_SIZE];
                    memset(response, 0, BUFFER_SIZE);

                    if (strncmp(buffer, "SUB ", 4) == 0) {
                        char req_topic[TOPIC_SIZE];
                        if (sscanf(buffer + 4, "%63s", req_topic) == 1) {
                            strncpy(clients[i].topic, req_topic, TOPIC_SIZE - 1);
                            snprintf(response, sizeof(response), "[Server] Dang ky thanh cong chu de: %s\n> ", req_topic);
                            printf("[LOG] FD %d dang ky chu de '%s'\n", sd, req_topic);
                        } else {
                            strcpy(response, "[Server] Cu phap sai. Mau: SUB <topic>\n> ");
                        }
                        send(sd, response, strlen(response), 0);
                    }
                    
                    else if (strncmp(buffer, "PUB ", 4) == 0) {
                        char req_topic[TOPIC_SIZE];
                        char *msg_ptr = NULL;
                        sscanf(buffer + 4, "%63s", req_topic);
                
                        msg_ptr = strchr(buffer + 4, ' ');
                        
                        if (msg_ptr != NULL && strlen(msg_ptr + 1) > 0) {
                            msg_ptr++;
                            
                            char broadcast_msg[BUFFER_SIZE];
                            snprintf(broadcast_msg, sizeof(broadcast_msg), "\n[Tin nhan tu %s]: %s\n> ", req_topic, msg_ptr);
                            
                            int count = 0;
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                // Nếu client hợp lệ VÀ đăng ký đúng topic (bỏ qua người gửi nếu muốn, ở đây cứ gửi hết)
                                if (clients[j].fd > 0 && strcmp(clients[j].topic, req_topic) == 0) {
                                    send(clients[j].fd, broadcast_msg, strlen(broadcast_msg), 0);
                                    count++;
                                }
                            }
                            snprintf(response, sizeof(response), "[Server] Da chuyen tiep tin nhan den %d clients.\n> ", count);
                            printf("[LOG] Dinh tuyen: '%s' -> %d clients thuoc chu de '%s'\n", msg_ptr, count, req_topic);
                        } else {
                            strcpy(response, "[Server] Cu phap sai. Mau: PUB <topic> <msg>\n> ");
                        }
                        send(sd, response, strlen(response), 0);
                    }
                    // Lệnh không hợp lệ
                    else {
                        strcpy(response, "[Server] Lenh khong hop le. Dung SUB hoac PUB.\n> ");
                        send(sd, response, strlen(response), 0);
                    }
                }
            }
        }
    }
    return 0;
}