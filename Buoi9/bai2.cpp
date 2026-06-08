#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

using namespace std;
string url_decode(string str) {
    string ret;
    int i;
    for (i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            int ii;
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ret += static_cast<char>(ii);
            i += 2;
        } else if (str[i] == '+') {
            ret += ' ';
        } else {
            ret += str[i];
        }
    }
    return ret;
}

string get_mime_type(string path) {
    size_t dot_pos = path.find_last_of(".");
    if (dot_pos == string::npos) return "application/octet-stream";
    string ext = path.substr(dot_pos);
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".txt" || ext == ".c" || ext == ".cpp") return "text/plain";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".mp3") return "audio/mpeg";
    if (ext == ".wav") return "audio/wav";
    if (ext == ".mp4") return "video/mp4";
    return "application/octet-stream";
}

void handle_client(int client_socket) {
    char buffer[4096] = {0};
    read(client_socket, buffer, 4096);
    string request(buffer);

    if (request.find("GET") != 0) {
        close(client_socket);
        return;
    }

    size_t path_start = request.find(" ") + 1;
    size_t path_end = request.find(" ", path_start);
    string path = request.substr(path_start, path_end - path_start);
    path = url_decode(path);
    string os_path = "." + path;
    if (os_path == "./") os_path = ".";

    struct stat path_stat;
    stat(os_path.c_str(), &path_stat);

    stringstream response;

    if (S_ISDIR(path_stat.st_mode)) {
        response << "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n";
        response << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Index of " << path << "</title></head><body>";
        response << "<h2>Index of " << path << "</h2><hr><ul>";

        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(os_path.c_str())) != NULL) {
            // Luôn thêm nút Back nếu không phải thư mục gốc
            if (path != "/") {
                response << "<li><b><a href='../'>../ (L&ecirc;n m&#7897;t c&#7845;p)</a></b></li>";
            }
            while ((ent = readdir(dir)) != NULL) {
                string item_name = ent->d_name;
                if (item_name == "." || item_name == "..") continue;

                string full_item_path = os_path + "/" + item_name;
                struct stat item_stat;
                stat(full_item_path.c_str(), &item_stat);

                string link = path;
                if (link.back() != '/') link += "/";
                link += item_name;

                // In đậm cho thư mục, in nghiêng cho file
                if (S_ISDIR(item_stat.st_mode)) {
                    response << "<li><b><a href='" << link << "'>&#128193; " << item_name << "/</a></b></li>";
                } else {
                    response << "<li><i><a href='" << link << "'>&#128196; " << item_name << "</a></i></li>";
                }
            }
            closedir(dir);
        }
        response << "</ul><hr></body></html>";
        string resp_str = response.str();
        send(client_socket, resp_str.c_str(), resp_str.length(), 0);

    } else if (S_ISREG(path_stat.st_mode)) {
        ifstream file(os_path, ios::binary | ios::ate);
        if (file.is_open()) {
            size_t size = file.tellg();
            file.seekg(0, ios::beg);

            string mime_type = get_mime_type(os_path);

            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: " << mime_type << "\r\n";
            response << "Content-Length: " << size << "\r\n";
            response << "Connection: close\r\n\r\n";
            
            string header = response.str();
            send(client_socket, header.c_str(), header.length(), 0);

            char file_buffer[8192];
            while (file.read(file_buffer, sizeof(file_buffer)) || file.gcount() > 0) {
                send(client_socket, file_buffer, file.gcount(), 0);
            }
            file.close();
        } else {
            string not_found = "HTTP/1.1 404 Not Found\r\n\r\n404 File Not Found";
            send(client_socket, not_found.c_str(), not_found.length(), 0);
        }
    } else {
        string not_found = "HTTP/1.1 404 Not Found\r\n\r\n404 Path Not Found";
        send(client_socket, not_found.c_str(), not_found.length(), 0);
    }

    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed"); exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed"); exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed"); exit(EXIT_FAILURE);
    }

    cout << "HTTP File Server (C++) dang chay tai cong 8080...\n";
    cout << "Doc file tai thu muc hien tai: " << get_current_dir_name() << "\n";

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            continue;
        }
        handle_client(new_socket);
    }
    return 0;
}