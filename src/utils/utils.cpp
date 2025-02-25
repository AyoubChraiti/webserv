#include "../../inc/header.hpp"

void sysCallFail() {
    perror("syscall Error");
    exit(1);
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// both of these are from the config parsing ..

bool isValidDirectory(const string &path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode));
}

bool isValidFile(const string &path) {
    return (access(path.c_str(), F_OK) == 0);
}


void sendErrorResponse(int fd, int statusCode, const string& message) {
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
