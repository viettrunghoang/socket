#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 9000
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024
#define DB_FILE "database.txt"
#define STATE_DISCONNECTED 0
#define STATE_WAIT_LOGIN 1
#define STATE_LOGGED_IN 2

struct Client {
    int fd;
    int state; 
};

void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len-1] == '\n') str[len-1] = '\0';
    if (len > 1 && str[len-2] == '\r') str[len-2] = '\0';
}

int check_login(const char *user, const char *pass) {
    FILE *file = fopen(DB_FILE, "r");

    char line[256];
    char db_user[128], db_pass[128];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%127s %127s", db_user, db_pass) == 2) {
            if (strcmp(user, db_user) == 0 && strcmp(pass, db_pass) == 0) {
                fclose(file);
                return 1;
            }
        }
    }
    fclose(file);
    return 0;
}

int main() {
    int server_fd, new_socket, max_sd, activity;
    struct sockaddr_in address;
    fd_set readfds;
    
    struct Client clients[MAX_CLIENTS];
    

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = 0;
        clients[i].state = STATE_DISCONNECTED;
    }


    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

    printf("Telnet Server port %d...\n", PORT);

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

            printf("Client connected: fd %d\n", new_socket);

            char *welcome_msg = "\nVui long dang nhap\nUsername Password: ";
            send(new_socket, welcome_msg, strlen(welcome_msg), 0);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    clients[i].state = STATE_WAIT_LOGIN;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].fd;

            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                char buffer[BUFFER_SIZE] = {0};
                int valread = read(sd, buffer, BUFFER_SIZE - 1);

                if (valread <= 0) {
                    printf("Client fd %d disconnected\n", sd);
                    close(sd);
                    clients[i].fd = 0;
                    clients[i].state = STATE_DISCONNECTED;
                } else {
                    buffer[valread] = '\0';
                    trim_newline(buffer);
                    
                    if (strlen(buffer) == 0) continue;
                    if (clients[i].state == STATE_WAIT_LOGIN) {
                        char user[128], pass[128];
                        
                        if (sscanf(buffer, "%127s %127s", user, pass) == 2) {
                            if (check_login(user, pass)) {
                                clients[i].state = STATE_LOGGED_IN;
                                char *success = "Dang nhap thanh cong!\ncmd> ";
                                send(sd, success, strlen(success), 0);
                                printf("Client fd %d logged in as '%s'\n", sd, user);
                            } else {
                                char *fail = "Sai tai khoan hoac mat khau.\n";
                                send(sd, fail, strlen(fail), 0);
                            }
                        } else {
                            char *format_err = "Cu phap sai.\n ";
                            send(sd, format_err, strlen(format_err), 0);
                        }
                    } 
                    else if (clients[i].state == STATE_LOGGED_IN) {
                        char sys_cmd[BUFFER_SIZE + 50];
                        char out_filename[32];
                        
                        snprintf(out_filename, sizeof(out_filename), "out_%d.txt", sd);
                        snprintf(sys_cmd, sizeof(sys_cmd), "%s > %s 2>&1", buffer, out_filename);
                        system(sys_cmd);

                        FILE *f_out = fopen(out_filename, "r");
                        if (f_out) {
                            char result_buffer[1024];
                            int bytes_read;
                            while ((bytes_read = fread(result_buffer, 1, sizeof(result_buffer), f_out)) > 0) {
                                send(sd, result_buffer, bytes_read, 0);
                            }
                            fclose(f_out);
                            remove(out_filename);
                        } else {
                            char *err = "Loi doc file ket qua.\n";
                            send(sd, err, strlen(err), 0);
                        }

                        char *prompt = "cmd> ";
                        send(sd, prompt, strlen(prompt), 0);
                    }
                }
            }
        }
    }
    return 0;
}