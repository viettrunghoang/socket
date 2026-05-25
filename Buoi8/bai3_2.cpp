#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 9000
#define BUFFER_SIZE 1024

typedef struct {
    int client1_fd;
    int client2_fd;
} Pair;

void *relay_messages(void *arg) {
    Pair *p = (Pair *)arg;
    int src = p->client1_fd;
    int dest = p->client2_fd;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_read = recv(src, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) break;
        if (send(dest, buffer, bytes_read, 0) <= 0) break;
    }

    close(src);
    close(dest);
    free(p);
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

    int waiting_client = -1;

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd < 0) continue;

        if (waiting_client == -1) {
            waiting_client = client_fd;
        } else {
            int c1 = waiting_client;
            int c2 = client_fd;
            waiting_client = -1;

            Pair *p1 = (Pair *)malloc(sizeof(Pair));
            p1->client1_fd = c1;
            p1->client2_fd = c2;

            Pair *p2 = (Pair *)malloc(sizeof(Pair));
            p2->client1_fd = c2;
            p2->client2_fd = c1;

            pthread_t t1, t2;
            pthread_create(&t1, NULL, relay_messages, (void *)p1);
            pthread_create(&t2, NULL, relay_messages, (void *)p2);

            pthread_detach(t1);
            pthread_detach(t2);
        }
    }

    close(listener);
    return 0;
}