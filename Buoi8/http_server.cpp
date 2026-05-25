#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define NUM_THREADS 8

int listener;
pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread_worker(void *arg) {
    char buffer[BUFFER_SIZE];
    char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";

    while (1) {
        pthread_mutex_lock(&accept_mutex);
        int client_fd = accept(listener, NULL, NULL);
        pthread_mutex_unlock(&accept_mutex);

        if (client_fd < 0) continue;

        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("New client connected: %d\n", client_fd);
            puts(buffer);
            send(client_fd, msg, strlen(msg), 0);
        }
        close(client_fd);
    }
    return NULL;
}

int main() {
    listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listener, 10);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_worker, NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    close(listener);
    return 0;
}