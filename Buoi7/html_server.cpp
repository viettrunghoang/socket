#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define PORT 8080
#define NUM_WORKERS 4
#define BUFFER_SIZE 1024

void worker_process(int listener) {
    char buf[BUFFER_SIZE];
    char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban (tu worker %d)</h1></body></html>";
    char response[BUFFER_SIZE];
    
    snprintf(response, sizeof(response), msg, getpid());

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret > 0) {
            buf[ret] = '\0';
            send(client, response, strlen(response), 0);
        }
        
        close(client);
    }
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

    for (int i = 0; i < NUM_WORKERS; i++) {
        if (fork() == 0) {
            worker_process(listener);
            exit(0);
        }
    }

    while (wait(NULL) > 0);

    close(listener);
    return 0;
}