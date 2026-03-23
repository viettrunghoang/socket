#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {


    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("failed");
        return 1;
    }

    char welcome_buf[256];
    int n = recv(client, welcome_buf, sizeof(welcome_buf) - 1, 0);
    if (n > 0) {
        welcome_buf[n] = 0;
        printf("Server: %s\n", welcome_buf);
    }
    char buf[256];
    printf("Noi dung: ");
    fgets(buf, sizeof(buf), stdin);
    send(client, buf, strlen(buf), 0);
    close(client);
    
    return 0;
}