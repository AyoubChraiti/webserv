#include "../../inc/responce.hpp"

bool isDirectory(const string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return (st.st_mode & S_IFDIR) != 0;
    }
    return false;
}

bool fileExists(const string& path) {
    ifstream file(path.c_str());
    bool exists = file.good();
    file.close();
    return exists;
}

// void handle_client_write(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestStates) {
//     map<int, HttpRequest>::iterator it = requestStates.find(clientFd);
//     if (it == requestStates.end()) { // wont even need this ig
//         close(clientFd);
//         epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
//         return;
//     }
//     HttpRequest& req = it->second;
// }


/* temporary */

void handle_client_write(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestStates) {
    // Find the request state for this client
    map<int, HttpRequest>::iterator it = requestStates.find(clientFd);
    if (it == requestStates.end()) {
        close(clientFd);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
        return;
    }

    HttpRequest& req = it->second;

    // Only handle GET requests
    if (req.method != "GET") {
        close(clientFd);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
        requestStates.erase(clientFd);
        return;
    }

    // Map path to a file (e.g., remove leading '/' and use as filename)
    string filepath = req.path.substr(1); // e.g., "/index.html" -> "index.html"
    if (filepath.empty()) filepath = "www/index.html"; // Default file

    // Open the file
    ifstream file(filepath, ios::binary);
    if (!file.is_open()) {
        // File not found, send 404
        string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 14\r\n\r\nFile not found";
        send(clientFd, response.c_str(), response.size(), 0);
        close(clientFd);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
        requestStates.erase(clientFd);
        return;
    }

    // Get file size
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    // Prepare HTTP response headers
    string response = "HTTP/1.1 200 OK\r\nContent-Length: " + to_string(fileSize) + "\r\n\r\n";

    // Send headers
    send(clientFd, response.c_str(), response.size(), 0);

    // Send file content
    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount()) {
        send(clientFd, buffer, file.gcount(), 0);
    }

    // Cleanup
    file.close();
    close(clientFd);
    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    requestStates.erase(clientFd);
}