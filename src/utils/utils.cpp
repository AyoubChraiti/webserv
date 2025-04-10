#include "../../inc/request.hpp"

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

const char *HttpRequest::RequestException::what() const throw ()
{
    return errorResponse.c_str();
}

HttpRequest::RequestException::RequestException (string msg , int status) : errorStr(msg) , statusCode(status)
{
    errorResponse += errorStr + '\n' + to_string(statusCode);
}

string sendErrorResponse(const char *e, int clientSocket)
{
    string str = e;
    size_t  pos = str.find_last_of("\n");
    string errorMessage = str.substr(0, pos);
    int statusCode = StringStream(str.substr(pos).c_str());
    std::string response = 
        "HTTP/1.1 " + std::to_string(statusCode) + " " + errorMessage + "\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>" + std::to_string(statusCode) + " " + errorMessage + "</title></head>\n"
        "<body>\n"
        "<h1>" + errorMessage + "</h1>\n"
        "<p>Error code: " + std::to_string(statusCode) + "</p>\n"
        "</body>\n"
        "</html>";
    
    send(clientSocket, response.c_str(), response.length(), 0);
    return "Client :" + errorMessage;
}


size_t StringStream(string string)
{
    size_t num;
    stringstream ss (string);
    ss >> num;
    return num;
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