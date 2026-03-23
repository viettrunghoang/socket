#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

typedef struct {
    char mssv[10];
    char ho_ten[50];
    char ngay_sinh[15];
    float diem_tb;
} SinhVien;

int main(int argc, char *argv[]) {
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    connect(client, (struct sockaddr *)&addr, sizeof(addr));

    SinhVien sv;
    printf("MSSV: "); scanf("%s", sv.mssv); getchar();
    printf("Ho ten: "); fgets(sv.ho_ten, 50, stdin);
    sv.ho_ten[strcspn(sv.ho_ten, "\n")] = 0;
    printf("Ngay sinh: "); scanf("%s", sv.ngay_sinh);
    printf("Diem TB: "); scanf("%f", &sv.diem_tb);

    send(client, &sv, sizeof(sv), 0);

    close(client);
    return 0;
}