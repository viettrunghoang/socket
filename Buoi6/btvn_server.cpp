#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_CLIENTS 50
#define MAX_TOPICS_PER_CLIENT 10 
#define BUFFER_SIZE 2048
#define TOPIC_SIZE 64
#define PORT 9000

struct Client {
    int fd;
    char topics[MAX_TOPICS_PER_CLIENT][TOPIC_SIZE];
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
        memset(clients[i].topics, 0, sizeof(clients[i].topics));
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Lỗi tạo socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Lỗi bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Lỗi listen");
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
        if (activity < 0) continue;

        if (FD_ISSET(server_fd, &readfds)) {
            int addrlen = sizeof(address);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Lỗi accept");
                exit(EXIT_FAILURE);
            }

            printf("> Client ket noi (FD: %d)\n", new_socket);
            const char *welcome = "Ket noi thanh cong.\n> ";
            send(new_socket, welcome, strlen(welcome), 0);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    memset(clients[i].topics, 0, sizeof(clients[i].topics));
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
                    printf("> Client (FD: %d) da ngat ket noi.\n", sd);
                    close(sd);
                    clients[i].fd = 0;
                } else {
                    buffer[valread] = '\0';
                    trim_newline(buffer);
                    if (strlen(buffer) == 0) continue;

                    char response[BUFFER_SIZE];
                    memset(response, 0, BUFFER_SIZE);

                    if (strncmp(buffer, "SUB ", 4) == 0) {
                        char req_topic[TOPIC_SIZE];
                        if (sscanf(buffer + 4, "%63s", req_topic) == 1) {
                            int success = 0;
                            for (int t = 0; t < MAX_TOPICS_PER_CLIENT; t++) {
                                if (strlen(clients[i].topics[t]) == 0) {
                                    strncpy(clients[i].topics[t], req_topic, TOPIC_SIZE - 1);
                                    success = 1;
                                    break;
                                }
                            }
                            if (success) {
                                snprintf(response, sizeof(response), "[Server] Da dang ky: %s\n> ", req_topic);
                            } else {
                                strcpy(response, "[Server] Loi: Da dat gioi han chu de.\n> ");
                            }
                        } else {
                            strcpy(response, "[Server] Cu phap sai.\n> ");
                        }
                        send(sd, response, strlen(response), 0);
                    }
                    
                    else if (strncmp(buffer, "UNSUB ", 6) == 0) {
                        char req_topic[TOPIC_SIZE];
                        if (sscanf(buffer + 6, "%63s", req_topic) == 1) {
                            int found = 0;
                            for (int t = 0; t < MAX_TOPICS_PER_CLIENT; t++) {
                                if (strcmp(clients[i].topics[t], req_topic) == 0) {
                                    clients[i].topics[t][0] = '\0';
                                    found = 1;
                                    break;
                                }
                            }
                            if (found) {
                                snprintf(response, sizeof(response), "[Server] Da huy dang ky: %s\n> ", req_topic);
                            } else {
                                snprintf(response, sizeof(response), "[Server] Ban chua dang ky chu de: %s\n> ", req_topic);
                            }
                        } else {
                            strcpy(response, "[Server] Cu phap sai.\n> ");
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
                            snprintf(broadcast_msg, sizeof(broadcast_msg), "\n[%s]: %s\n> ", req_topic, msg_ptr);
                            
                            int count = 0;

                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (clients[j].fd > 0) {
                                    for (int t = 0; t < MAX_TOPICS_PER_CLIENT; t++) {
                                        if (strcmp(clients[j].topics[t], req_topic) == 0) {
                                            send(clients[j].fd, broadcast_msg, strlen(broadcast_msg), 0);
                                            count++;
                                            break;
                                        }
                                    }
                                }
                            }
                            snprintf(response, sizeof(response), "[Server] Da gui den %d clients.\n> ", count);
                        } else {
                            strcpy(response, "[Server] Cu phap sai.\n> ");
                        }
                        send(sd, response, strlen(response), 0);
                    }
                    
                    else {
                        strcpy(response, "[Server] Lenh khong hop le.\n> ");
                        send(sd, response, strlen(response), 0);
                    }
                }
            }
        }
    }
    return 0;
}