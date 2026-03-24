#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {

    int server_sk = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(argv[1]));

    bind(server_sk, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_sk, 5);

    while (1) {
        int client_sk = accept(server_sk, NULL, NULL);

        char buffer[2048] = {0};
        recv(client_sk, buffer, sizeof(buffer), 0);

        printf("Du lieu nhan duoc:\n%s", buffer);

        close(client_sk);
    }

    return 0;
}