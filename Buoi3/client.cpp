#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    connect(client, (struct sockaddr *)&addr, sizeof(addr));

    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        recv(client, buffer, sizeof(buffer) - 1, 0);

        printf("%s", buffer);
        fflush(stdout);

        if (strstr(buffer, "Dia chi email") != NULL) {
            continue; 
        }

        char input[256];
        fgets(input, sizeof(input), stdin);
        
        input[strcspn(input, "\n")] = 0;
        send(client, input, strlen(input), 0);
    }

    close(client);
    return 0;
}