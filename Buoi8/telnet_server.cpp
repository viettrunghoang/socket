#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define DB_FILE "databases.txt"

int check_login(const char *username, const char *password) {
    FILE *f = fopen(DB_FILE, "r");
    if (!f) return 0;

    char db_user[64], db_pass[64];
    while (fscanf(f, "%59s %59s", db_user, db_pass) == 2) {
        if (strcmp(username, db_user) == 0 && strcmp(password, db_pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void send_file_content(int client_fd, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;

    char file_buf[BUFFER_SIZE];
    while (fgets(file_buf, sizeof(file_buf), f) != NULL) {
        send(client_fd, file_buf, strlen(file_buf), 0);
    }
    fclose(f);
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char username[64], password[64];
    int logged_in = 0;

    char *msg_auth = "Vui long nhap user va pass theo dang 'username password':\n";
    send(client_fd, msg_auth, strlen(msg_auth), 0);

    while (1) {
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) break;

        buffer[bytes_read] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strlen(buffer) == 0) continue;

        if (!logged_in) {
            if (sscanf(buffer, "%63s %63s", username, password) == 2) {
                if (check_login(username, password)) {
                    logged_in = 1;
                    char *success = "Dang nhap thanh cong. Hay nhap lenh:\n";
                    send(client_fd, success, strlen(success), 0);
                } else {
                    char *fail = "Loi dang nhap. Vui long thu lai:\n";
                    send(client_fd, fail, strlen(fail), 0);
                }
            } else {
                char *invalid = "Sai cu phap. Nhap lai 'username password':\n";
                send(client_fd, invalid, strlen(invalid), 0);
            }
        } else {
            char out_file[64];
            snprintf(out_file, sizeof(out_file), "out_%d.txt", client_fd);

            char cmd[BUFFER_SIZE + 128];
            snprintf(cmd, sizeof(cmd), "%s > %s", buffer, out_file);
            
            system(cmd);

            send_file_content(client_fd, out_file);
            remove(out_file);
        }
    }

    close(client_fd);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listener, 10);

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd < 0) continue;

        int *arg = malloc(sizeof(int));
        *arg = client_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, arg);
        pthread_detach(tid);
    }

    close(listener);
    return 0;
}