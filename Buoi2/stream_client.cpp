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
    printf("Da ket noi voi server\n");

    char buffer[1024];
    while (1) {
        printf("Nhap du lieu: ");
        fgets(buffer, sizeof(buffer), stdin);
        
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "exit") == 0) break;

        send(client, buffer, strlen(buffer), 0);
    }
    close(client);


    return 0;
}