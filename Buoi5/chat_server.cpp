#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024


struct ClientInfo {
    int fd;
    char name[64];
    int is_registered;
};

void get_current_time(char *time_str, size_t max_size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(time_str, max_size, "%Y/%m/%d %I:%M:%S%p", tm_info);
}

int main() {
    int listener;
    struct sockaddr_in server_addr;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

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

    printf("Chat Server dang chay tai port %d...\n", PORT);

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
                    if (new_fd < 0) {
                        perror("Accept error");
                        continue;
                    }

                    if (nfds < MAX_CLIENTS + 1) {
                        fds[nfds].fd = new_fd;
                        fds[nfds].events = POLLIN;
                        
                        clients[nfds].fd = new_fd;
                        clients[nfds].is_registered = 0;
                        memset(clients[nfds].name, 0, sizeof(clients[nfds].name));
                        
                        nfds++;

                        char *prompt = "Vui long nhap ten theo cu phap:\n";
                        send(new_fd, prompt, strlen(prompt), 0);
                        printf("Client moi ket noi (fd: %d)\n", new_fd);
                    } else {
                        printf("Server da dat toi da so luong client.\n");
                        close(new_fd);
                    }
                } 
                else {
                    char buffer[BUFFER_SIZE];
                    int bytes_read = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytes_read <= 0) {
                        printf("Client (fd: %d) da ngat ket noi.\n", fds[i].fd);
                        close(fds[i].fd);

                        fds[i] = fds[nfds - 1];
                        clients[i] = clients[nfds - 1];
                        nfds--;
                        i--; 
                    } 
                    else {
                        buffer[bytes_read] = '\0';
                        buffer[strcspn(buffer, "\r\n")] = 0;
                        if (strlen(buffer) == 0) continue;
                        if (clients[i].is_registered == 0) {
                            char temp_name[64];

                            if (strncmp(buffer, "client_id: ", 11) == 0) {

                                if (sscanf(buffer + 11, "%s", temp_name) == 1) {
                                    strcpy(clients[i].name, temp_name);
                                    clients[i].is_registered = 1;
                                    
                                    char *success_msg = "Thanh cong, co the bat dau chat\n";
                                    send(clients[i].fd, success_msg, strlen(success_msg), 0);
                                    printf("Client (fd: %d) dang ky thanh cong voi ten: %s\n", fds[i].fd, temp_name);
                                } else {
                                    char *err = "Ten khong hop le, thu lai:\n";
                                    send(clients[i].fd, err, strlen(err), 0);
                                }
                            } else {
                                char *err = "Sai cu phap!\n";
                                send(clients[i].fd, err, strlen(err), 0);
                            }
                        } 
                        else {
                            char time_str[32];
                            get_current_time(time_str, sizeof(time_str));
                            
                            char send_buffer[BUFFER_SIZE + 128];
                            snprintf(send_buffer, sizeof(send_buffer), "%s %s: %s\n", 
                                     time_str, clients[i].name, buffer);

                            for (int j = 1; j < nfds; j++) {
                                if (j != i && clients[j].is_registered == 1) {
                                    send(clients[j].fd, send_buffer, strlen(send_buffer), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}