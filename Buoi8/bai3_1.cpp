#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8888
#define BUFFER_SIZE 1024
#define STORAGE_DIR "./server_files"

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    char response[8192];
    
    DIR *dir = opendir(STORAGE_DIR);
    struct dirent *entry;
    struct stat file_stat;
    
    char file_list[4096] = "";
    int file_count = 0;

    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", STORAGE_DIR, entry->d_name);
            
            if (stat(full_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
                strcat(file_list, entry->d_name);
                strcat(file_list, "\r\n");
                file_count++;
            }
        }
        closedir(dir);
    }
    if (file_count == 0) {
        char *err_msg = "ERROR No files to download\r\n";
        send(client_fd, err_msg, strlen(err_msg), 0);
        close(client_fd);
        return;
    }

    snprintf(response, sizeof(response), "OK %d\r\n%s\r\n", file_count, file_list);
    send(client_fd, response, strlen(response), 0);

    while (1) {
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) break;

        buffer[bytes_read] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strlen(buffer) == 0) continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", STORAGE_DIR, buffer);

        FILE *f = fopen(filepath, "rb");
        if (f == NULL) {
            char *err_fnf = "ERROR File not found hoặc yêu cầu gửi lại tên file\r\n";
            send(client_fd, err_fnf, strlen(err_fnf), 0);
            continue; 
        }

        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        char header[128];
        snprintf(header, sizeof(header), "OK %ld\r\n", file_size);
        send(client_fd, header, strlen(header), 0);
        char file_buffer[4096];
        int read_bytes;
        while ((read_bytes = fread(file_buffer, 1, sizeof(file_buffer), f)) > 0) {
            send(client_fd, file_buffer, read_bytes, 0);
        }

        fclose(f);
        break;
    }

    close(client_fd);
}

int main() {
    mkdir(STORAGE_DIR, 0777);

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listener, 10);
    
    signal(SIGCHLD, SIG_IGN); 

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd < 0) continue;

        if (fork() == 0) { 
            close(listener);
            handle_client(client_fd);
            exit(0);
        }
        
        close(client_fd);
    }

    close(listener);
    return 0;
}