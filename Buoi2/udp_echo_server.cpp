#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    int server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in addr, cliaddr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(argv[1]));

    bind(server, (struct sockaddr *)&addr, sizeof(addr));

    char buffer[1024];
    socklen_t len = sizeof(cliaddr);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        
        int n = recvfrom(server, buffer, sizeof(buffer), 0, (struct sockaddr *)&cliaddr, &len);
        
        if (n > 0) {
            sendto(server, buffer, n, 0, (struct sockaddr *)&cliaddr, len);
        }
    }

    close(server);
    return 0;
}