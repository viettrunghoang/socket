#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    char buffer[1024];
    socklen_t len = sizeof(addr);

    while (1) {
        printf("Nhap du lieu: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0; 

        if (strcmp(buffer, "exit") == 0) break;
        sendto(client, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, len);

        memset(buffer, 0, sizeof(buffer));
        int n = recvfrom(client, buffer, sizeof(buffer), 0, NULL, NULL);
        if (n > 0) {
            printf("Echo tu Server: %s\n", buffer);
        }
    }

    close(client);
    return 0;
}