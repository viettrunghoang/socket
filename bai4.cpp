#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char mssv[10];
    char ho_ten[50];
    char ngay_sinh[15];
    float diem_tb;
} SinhVien;

int main(int argc, char *argv[]) {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_len);

        SinhVien sv;
        int ret = recv(client, &sv, sizeof(sv), 0);
        if (ret > 0) {

            char *ip = inet_ntoa(client_addr.sin_addr);
            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);
            char time_str[25];
            strftime(time_str, 25, "%Y-%m-%d %H:%M:%S", tm_info);

            printf("%s %s %s %s %s %.2f\n", ip, time_str, sv.mssv, sv.ho_ten, sv.ngay_sinh, sv.diem_tb);
            
            FILE *f = fopen(argv[2], "a");
            if (f) {
                fprintf(f, "%s %s %s %s %s %.2f\n", ip, time_str, sv.mssv, sv.ho_ten, sv.ngay_sinh, sv.diem_tb);
                fclose(f);
            }
        }
        close(client);
    }
    return 0;
}