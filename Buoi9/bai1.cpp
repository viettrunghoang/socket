#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>

using namespace std;

string get_param(const string& data, const string& key) {
    size_t start = data.find(key + "=");
    if (start == string::npos) return "";
    start += key.length() + 1;
    size_t end = data.find('&', start);
    if (end == string::npos) end = data.find(' ', start);
    if (end == string::npos) end = data.length();
    return data.substr(start, end - start);
}

string handle_calculation(string a_str, string b_str, string op) {
    if (a_str.empty() || b_str.empty() || op.empty()) {
        return "<h2>Vui l&ograve;ng cung c&#7845;p &#273;&#7911; tham s&#7889; (a, b, op)</h2>";
    }

    double a = stod(a_str);
    double b = stod(b_str);
    double result = 0;
    string op_symbol = "";

    if (op == "add") { result = a + b; op_symbol = "+"; }
    else if (op == "sub") { result = a - b; op_symbol = "-"; }
    else if (op == "mul") { result = a * b; op_symbol = "*"; }
    else if (op == "div") { 
        if (b == 0) return "<h2>L&#7895;i: Kh&ocirc;ng th&#7875; chia cho 0!</h2>";
        result = a / b; op_symbol = "/"; 
    }
    else { return "<h2>To&aacute;n t&#7915; kh&ocirc;ng h&#7907;p l&#7879;! D&ugrave;ng add, sub, mul, div.</h2>"; }

    stringstream html;
    html << "<h2>K&#7871;t qu&#7843; ph&eacute;p t&iacute;nh:</h2>";
    html << "<h3>" << a << " " << op_symbol << " " << b << " = " << result << "</h3>";
    return html.str();
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[4096] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    cout << "Server C++ dang chay tren cong 8080...\n";

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        memset(buffer, 0, 4096);
        read(new_socket, buffer, 4096);
        string request(buffer);

        string a, b, op;
        
        if (request.find("GET") == 0) {

            size_t first_line_end = request.find("\r\n");
            string first_line = request.substr(0, first_line_end);
            a = get_param(first_line, "a");
            b = get_param(first_line, "b");
            op = get_param(first_line, "op");
        } 
        else if (request.find("POST") == 0) {
            size_t body_start = request.find("\r\n\r\n");
            if (body_start != string::npos) {
                string body = request.substr(body_start + 4);
                a = get_param(body, "a");
                b = get_param(body, "b");
                op = get_param(body, "op");
            }
        }

        string html_content = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Calculator</title></head><body style='font-family: Arial; text-align: center; margin-top: 50px;'>" 
                              + handle_calculation(a, b, op) + "</body></html>";

        stringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html; charset=UTF-8\r\n";
        response << "Content-Length: " << html_content.length() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << html_content;

        string final_response = response.str();
        
        send(new_socket, final_response.c_str(), final_response.length(), 0);
        close(new_socket);
    }

    return 0;
}