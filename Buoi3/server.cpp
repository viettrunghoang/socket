#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Server run\n");

    int clients[64], states[64], nclients = 0;
    char names[64][50], buf[256];

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client != -1) {
            ioctl(client, FIONBIO, &ul);
            clients[nclients] = client;
            states[nclients] = 1;
            send(client, "Ho ten: ", 8, 0);
            nclients++;
        }

        for (int i = 0; i < nclients; i++) {
            int len = recv(clients[i], buf, sizeof(buf) - 1, 0);
            
            if (len > 0) {
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0;

                if (strlen(buf) == 0) continue;

                if (states[i] == 1) {
                    strcpy(names[i], buf);
                    states[i] = 2;
                    send(clients[i], "MSSV: ", 6, 0);
                } 
                else if (states[i] == 2) {
                    char email[128], init[20] = {0};
                    int idx = 0;
                    
                    //Chu cai dau moi tu
                    for (int k = 0; names[i][k]; k++) {
                        names[i][k] = tolower(names[i][k]);
                        if (k == 0 || (names[i][k-1] == ' ' && names[i][k] != ' ')) {
                            init[idx++] = names[i][k];
                        }
                    }

                    //Tach ten
                    char *ten = strrchr(names[i], ' ');
                    if (ten) {
                        ten++;         
                        init[idx-1] = 0; 
                    } else {
                        ten = names[i];
                        init[0] = 0;
                    }

                    int mlen = strlen(buf);
                    char *ms = (mlen >= 6) ? (buf + mlen - 6) : buf;

                    sprintf(email, "Email: %s.%s%s@sis.hust.edu.vn\n", ten, init, ms);
                    send(clients[i], email, strlen(email), 0);

                    close(clients[i]);
                    clients[i] = clients[nclients - 1];
                    states[i] = states[nclients - 1];
                    strcpy(names[i], names[nclients - 1]);
                    nclients--;
                    i--;
                }
            } 
            else if (len == 0) {
                close(clients[i]);
                clients[i] = clients[nclients - 1];
                states[i] = states[nclients - 1];
                strcpy(names[i], names[nclients - 1]);
                nclients--;
                i--;
            }
        }
        usleep(10000);
    }
    return 0;
}