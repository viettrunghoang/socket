#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {

    int client_sk = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    connect(client_sk, (struct sockaddr *)&addr, sizeof(addr));

    char buffer[2048] = {0};
    char path[256];
    getcwd(path, sizeof(path));//path
    strcat(buffer, path);
    strcat(buffer, "\n");

    //file list
    DIR *d = opendir("."); 
    struct dirent *entry;
    struct stat st;

    if (d) {
        while ((entry = readdir(d)) != NULL) {
            if (entry->d_type == DT_REG) {
                stat(entry->d_name, &st);
                char line[256];
                sprintf(line, "%s - %ld bytes\n", entry->d_name, st.st_size);
                strcat(buffer, line);
            }
        }
        closedir(d);
    }

    send(client_sk, buffer, strlen(buffer), 0);
    close(client_sk);

    return 0;
}