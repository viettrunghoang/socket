#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define PORT 9000
#define BUFFER_SIZE 256

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char format_str[64];
    char time_str[128];
    char response[256];

    while (1) {
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) break;

        buffer[bytes_read] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strlen(buffer) == 0) continue;

        if (strncmp(buffer, "GET_TIME ", 9) == 0) {
            strcpy(format_str, buffer + 9);

            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);
            int valid_format = 1;

            if (strcmp(format_str, "dd/mm/yyyy") == 0) {
                strftime(time_str, sizeof(time_str), "%d/%m/%Y", tm_info);
            } else if (strcmp(format_str, "dd/mm/yy") == 0) {
                strftime(time_str, sizeof(time_str), "%d/%m/%y", tm_info);
            } else if (strcmp(format_str, "mm/dd/yyyy") == 0) {
                strftime(time_str, sizeof(time_str), "%m/%d/%Y", tm_info);
            } else if (strcmp(format_str, "mm/dd/yy") == 0) {
                strftime(time_str, sizeof(time_str), "%m/%d/%y", tm_info);
            } else {
                valid_format = 0;
            }

            if (valid_format) {
                snprintf(response, sizeof(response), "%s\n", time_str);
                send(client_fd, response, strlen(response), 0);
            } else {
                char *msg = "Loi: Dinh dang thoi gian khong ho tro.\n";
                send(client_fd, msg, strlen(msg), 0);
            }
        } else {
            char *msg = "Loi: Sai cu phap. Hay dung GET_TIME [format].\n";
            send(client_fd, msg, strlen(msg), 0);
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