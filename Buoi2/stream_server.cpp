#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(argv[1]));

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);
    int client = accept(server, NULL, NULL);

    char buffer[1024];
    int match_length = 0;
    int total_count = 0;
    char target[] = "0123456789";

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(client, buffer, sizeof(buffer), 0);
        
        if (bytes_read <= 0) break;

        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == target[match_length]) {
                match_length++;
                if (match_length == 10) {
                    total_count++;
                    match_length = 0;
                }
            } else {
                if (buffer[i] == '0') {
                    match_length = 1;
                } else {
                    match_length = 0;
                }
            }
        }
        printf("Tong so lan xuat hien '0123456789': %d\n", total_count);
    }

    close(client);
    close(server);
    return 0;
}