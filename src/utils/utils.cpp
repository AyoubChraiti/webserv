#include "../../inc/header.hpp"

void sysCallFail() {
    perror("syscall Error");
    exit(1);
}

void sendErrorResponse(int fd, int statusCode, const string& message) { // will change this.
    string statusText;
    switch (statusCode) {
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        case 411: statusText = "Length Required"; break;
        case 500: statusText = "Internal Server Error"; break;
        case 501: statusText = "Not Implemented"; break;
        case 505: statusText = "HTTP Version Not Supported"; break;
        default:  statusText = "Error"; break;
    }

    string response =
        "HTTP/1.1 " + to_string(statusCode) + " " + statusText + "\r\n"
        "Content-Length: " + to_string(message.length()) + "\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n" + message;

    cerr << "Error: '" << message << "' sent to the client" << endl;

    send(fd, response.c_str(), response.length(), 0);
    close(fd);
}

string getIp(string hostname) {
    struct addrinfo hints, *res;
    struct sockaddr_in *addr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname.c_str(), NULL, &hints, &res);
    if (status != 0)
        sysCallFail();

    addr = (struct sockaddr_in *)res->ai_addr;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);

    in_addr_t ip = addr->sin_addr.s_addr; // will i need this later?
    freeaddrinfo(res);
    return string(ip_str);
}