#include "../../inc/responce.hpp"

void sendRedirect(int fd, const string& location, HttpRequest& req) {
    stringstream response;

    response << "HTTP/1.1 301 Moved Permanently\r\n";
    response << "Location: " << location << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: 0\r\n";
    response << "Connection: " << req.connection << "\r\n";
    response << "\r\n";

    string responseStr = response.str();
    size_t bytes = send(fd, responseStr.c_str(), responseStr.size(), 0);

    cout << "bytes sent: " << bytes << endl;
}

size_t getContentLength(const string& path) {
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == -1) {
        perror("stat");
        return 0;
    }
    return fileStat.st_size;
}

int sendFileInChunks(int clientFd, std::ifstream& fileStream, off_t fileSize) {
    char buffer[BUFFER_SIZE];
    off_t bytesSent = 0;

    while (bytesSent < fileSize) {
        // Read a chunk of the file
        fileStream.read(buffer, BUFFER_SIZE);
        size_t bytesRead = fileStream.gcount();

        if (bytesRead == 0)
            break;

        std::stringstream chunkHeader;
        chunkHeader << std::hex << bytesRead << "\r\n";  // Hexadecimal size of the chunk

        std::string chunkHeaderStr = chunkHeader.str();
        if (send(clientFd, chunkHeaderStr.c_str(), chunkHeaderStr.size(), 0) == -1) {
            perror("send chunk size header");
            return -1;
        }

        if (send(clientFd, buffer, bytesRead, 0) == -1) {
            perror("send chunk data");
            return -1;
        }

        const char* crlf = "\r\n";
        if (send(clientFd, crlf, 2, 0) == -1) {
            perror("send CRLF after chunk");
            return -1;
        }

        bytesSent += bytesRead;
    }

    const char* finalChunk = "0\r\n\r\n";
    if (send(clientFd, finalChunk, 5, 0) == -1) {
        perror("send final chunk");
        return -1;
    }
    return 0;  // Success
}

void sendHeaders(int clientFd, RouteResult& routeResult) {
    stringstream response;
    response << "HTTP/1.1 " << routeResult.statusCode << " " << routeResult.statusText << "\r\n";
    response << "Content-Type: " << routeResult.contentType << "\r\n";

    if (routeResult.resFd != -1) {
        response << "Content-Length: " << getContentLength(routeResult.fullPath) << "\r\n";
    }
    else {
        response << "Content-Length: " << routeResult.responseBody.size() << "\r\n";
    }

    response << "Connection: " << "close" << "\r\n";
    response << "\r\n";

    string headerStr = response.str();
    send(clientFd, headerStr.c_str(), headerStr.size(), 0);
}

int getMethode(int clientFd, int epollFd, HttpRequest& req, map<int, HttpRequest>& requestmp) {
    RouteResult routeResult = handleRouting(clientFd, req);

    if (routeResult.shouldRDR) {
        cout << "Redirecting to: " << routeResult.redirectLocation << endl;
        sendRedirect(clientFd, routeResult.redirectLocation, req);
        return 0;
    }

    sendHeaders(clientFd, routeResult);

    if (routeResult.resFd != -1) {
        char buffer[BUFFER_SIZE];

        while (true) {
            size_t bytesRead = read(routeResult.resFd, buffer, BUFFER_SIZE);

            if (bytesRead <= 0)
                break;

            streamsize totalSent = 0;
            while (totalSent < bytesRead) {
                ssize_t sent = send(clientFd, buffer + totalSent, bytesRead - totalSent, 0);
                if (sent == -1) {
                    if (errno == EPIPE || errno == ECONNRESET) {
                        cout << "Socket is not ready for writing, try again later" << endl;
                        break; // Non-blocking mode, try again later
                    }
                    perror("send body (file)");
                    close(routeResult.resFd);
                    return -1;
                }
                totalSent += sent;
            }
        }
        close(routeResult.resFd);
    }
    else {
        const string& body = routeResult.responseBody;
        ssize_t totalSent = 0;
        while (totalSent < (ssize_t)body.size()) {
            ssize_t sent = send(clientFd, body.c_str() + totalSent, body.size() - totalSent, 0);
            if (sent == -1) {
                cout << "Send failed: " << strerror(errno) << endl;
                return -1;
            }
            totalSent += sent;
        }
    }
    return 0;
}

void handle_client_write(int fd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    HttpRequest req = requestmp[fd];

    try {
        if (!req.mtroute.redirect.empty()) {
            sendRedirect(fd, req.mtroute.redirect, req);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
        if (req.method == "GET") {
            getMethode(fd, epollFd, req, requestmp);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what(), req.conf);
        requestmp.erase(fd);
        struct epoll_event ev;
        ev.data.fd = fd;
        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev) == -1)
            cout << "epoll ctl error in the client write\n";
        close(fd);
        return;
    }
}
