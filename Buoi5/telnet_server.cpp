#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>

#define PORT 9000
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define DB_FILE "database.txt"

#define STATE_WAIT_LOGIN 0
#define STATE_LOGGED_IN 1

struct ClientInfo {
    int fd;
    int state; 
};

int check_credentials(const char *username, const char *password) {
    FILE *file = fopen(DB_FILE, "r");

    char line[256];
    char db_user[128];
    char db_pass[128];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s %s", db_user, db_pass) == 2) {
            if (strcmp(username, db_user) == 0 && strcmp(password, db_pass) == 0) {
                fclose(file);
                return 1;
            }
        }
    }

    fclose(file);
    return 0;
}

int main() {
    int listener;
    struct sockaddr_in server_addr;

    listener = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (listen(listener, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Telnet Server dang chay tai port %d...\n", PORT);

    struct pollfd fds[MAX_CLIENTS + 1];
    struct ClientInfo clients[MAX_CLIENTS + 1];
    int nfds = 1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    while (1) {
        int poll_count = poll(fds, nfds, -1);
        if (poll_count < 0) {
            perror("Poll error");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                
                if (fds[i].fd == listener) {
                    int new_fd = accept(listener, NULL, NULL);
                    if (new_fd < 0) continue;

                    if (nfds < MAX_CLIENTS + 1) {
                        fds[nfds].fd = new_fd;
                        fds[nfds].events = POLLIN;
                        
                        clients[nfds].fd = new_fd;
                        clients[nfds].state = STATE_WAIT_LOGIN;
                        nfds++;

                        char *prompt = "Vui long dang nhap:\n";
                        send(new_fd, prompt, strlen(prompt), 0);
                        printf("Client (fd: %d) ket noi.\n", new_fd);
                    } else {
                        close(new_fd);
                    }
                } 

                else {
                    char buffer[BUFFER_SIZE];
                    int client_fd = fds[i].fd;
                    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytes_read <= 0) {
                        printf("Client (fd: %d) ngat ket noi.\n", client_fd);
                        close(client_fd);
                        
                        fds[i] = fds[nfds - 1];
                        clients[i] = clients[nfds - 1];
                        nfds--;
                        i--; 
                    } 
                    else {
                        buffer[bytes_read] = '\0';
                        buffer[strcspn(buffer, "\r\n")] = 0;
                        if (strlen(buffer) == 0) continue;

                        if (clients[i].state == STATE_WAIT_LOGIN) {
                            char user[128] = {0}, pass[128] = {0};

                            if (sscanf(buffer, "%s %s", user, pass) == 2) {
                                if (check_credentials(user, pass)) {
                                    clients[i].state = STATE_LOGGED_IN;
                                    char *msg = "Dang nhap thanh cong\n";
                                    send(client_fd, msg, strlen(msg), 0);
                                    printf("Client (fd: %d) dang nhap thanh cong user: %s\n", client_fd, user);
                                } else {
                                    char *msg = "Tai khoan hoac mat khau sai\n";
                                    send(client_fd, msg, strlen(msg), 0);
                                }
                            } else {
                                char *msg = "Sai cu phap\n";
                                send(client_fd, msg, strlen(msg), 0);
                            }
                        } 
                        else if (clients[i].state == STATE_LOGGED_IN) {
                            char out_filename[64];
                            char sys_cmd[BUFFER_SIZE + 100];

                            snprintf(out_filename, sizeof(out_filename), "out_%d.txt", client_fd);
                            snprintf(sys_cmd, sizeof(sys_cmd), "%s > %s 2>&1", buffer, out_filename);
                            system(sys_cmd);

                            FILE *out_file = fopen(out_filename, "r");
                            if (out_file) {
                                char read_buf[BUFFER_SIZE];
                                int read_bytes;
                                while ((read_bytes = fread(read_buf, 1, sizeof(read_buf), out_file)) > 0) {
                                    send(client_fd, read_buf, read_bytes, 0);
                                }
                                fclose(out_file);
                            } else {
                                char *msg = "Khong the doc ket qua lenh.\n";
                                send(client_fd, msg, strlen(msg), 0);
                            }
                            remove(out_filename);
                            send(client_fd, "\n> ", 3, 0);
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}