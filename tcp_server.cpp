#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    int client = accept(listener, NULL, NULL);

    FILE *f1 = fopen(argv[2], "r");
    char msg[256];
    if (f1) {
        fgets(msg, sizeof(msg), f1);
        send(client, msg, strlen(msg), 0);
        fclose(f1);
    }
    char buf[256];
    int ret = recv(client, buf, sizeof(buf) - 1, 0);
    if (ret > 0) {
        buf[ret] = 0;
        FILE *f2 = fopen(argv[3], "w");
        if (f2) {
            fputs(buf, f2);
            fclose(f2);
        }
    }

    close(client);
    close(listener);
    return 0;
}
