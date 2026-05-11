#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define DB_FILE "database.txt"

int check_credentials(const char *username, const char *password) {
    FILE *file = fopen(DB_FILE, "r");
    if (!file) return 0;

    char line[256], db_user[128], db_pass[128];
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

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    char user[128], pass[128];
    int logged_in = 0;

    char *prompt = "Vui long dang nhap (user pass):\n";
    send(client_fd, prompt, strlen(prompt), 0);

    while (!logged_in) {
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            close(client_fd);
            return;
        }
        buffer[bytes_read] = '\0';

        if (sscanf(buffer, "%s %s", user, pass) == 2) {
            if (check_credentials(user, pass)) {
                logged_in = 1;
                char *msg = "Dang nhap thanh cong\n> ";
                send(client_fd, msg, strlen(msg), 0);
            } else {
                char *msg = "Sai tai khoan. Thu lai:\n";
                send(client_fd, msg, strlen(msg), 0);
            }
        }
    }

    while (1) {
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) break;
        
        buffer[bytes_read] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0; 
        if (strlen(buffer) == 0) {
            send(client_fd, "> ", 2, 0);
            continue;
        }

        char out_filename[64];
        char sys_cmd[BUFFER_SIZE + 100];
        snprintf(out_filename, sizeof(out_filename), "out_%d.txt", getpid());
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
            send(client_fd, "Loi thuc thi lenh.\n", 19, 0);
        }
        remove(out_filename);
        send(client_fd, "\n> ", 3, 0);
    }
    close(client_fd);
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

    signal(SIGCHLD, SIG_IGN);

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd < 0) continue;

        if (fork() == 0) { 
            close(listener);
            handle_client(client_fd);
            exit(0);
        }
        
        close(client_fd);
    }

    close(listener);
    return 0;
}