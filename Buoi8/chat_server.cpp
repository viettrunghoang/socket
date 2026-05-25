#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

typedef struct {
    int fd;
    char name[64];
    int registered;
} Client;

Client *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast(const char *message, int sender_fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->registered && clients[i]->fd != sender_fd) {
            send(clients[i]->fd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    Client *cli = (Client *)arg;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE + 128];

    while (1) {
        int bytes_read = recv(cli->fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) break;

        buffer[bytes_read] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strlen(buffer) == 0) continue;

        if (!cli->registered) {
            if (strncmp(buffer, "client_id: ", 11) == 0 && strlen(buffer) > 11) {
                strcpy(cli->name, buffer + 11);
                cli->registered = 1;
                char *welcome = "Dang ky ten thanh cong.\n";
                send(cli->fd, welcome, strlen(welcome), 0);
            } else {
                char *ask_name = "Sai cu phap. Vui long gui lai theo dang 'client_id: client_name'\n";
                send(cli->fd, ask_name, strlen(ask_name), 0);
            }
        } else {
            snprintf(response, sizeof(response), "%s: %s\n", cli->name, buffer);
            broadcast(response, cli->fd);
        }
    }

    close(cli->fd);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == cli) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    free(cli);
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

        Client *cli = (Client *)malloc(sizeof(Client));
        cli->fd = client_fd;
        cli->registered = 0;
        memset(cli->name, 0, sizeof(cli->name));

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == NULL) {
                clients[i] = cli;
                pthread_t tid;
                pthread_create(&tid, NULL, handle_client, (void *)cli);
                pthread_detach(tid);
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    close(listener);
    return 0;
}