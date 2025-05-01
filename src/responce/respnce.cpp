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

void sendHeaders(int clientFd, RouteResult& routeResult, HttpRequest& req) {
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
    RouteResult& routeResult = req.routeResult;

    if (!req.sendingFile) {
        if (routeResult.shouldRDR) {
            sendRedirect(clientFd, routeResult.redirectLocation, req);
            return 1;
        }

        sendHeaders(clientFd, routeResult, req);

        if (routeResult.resFd == -1) {
            const string& body = routeResult.responseBody;
            ssize_t sent = send(clientFd, body.c_str(), body.size(), 0);
            return 1;
        } else {
            req.sendingFile = true;
            req.bytesSentSoFar = 0;
        }
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = pread(routeResult.resFd, buffer, BUFFER_SIZE, req.bytesSentSoFar);
    if (bytesRead <= 0) {
        if (bytesRead == -1)
            cout << "Error reading file: " << strerror(errno) << endl;
        return 1;
    }

    ssize_t sent = send(clientFd, buffer, bytesRead, 0);
    if (sent == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        perror("send");
        return 1;
    }

    req.bytesSentSoFar += sent;
    cout << "bytes sent: " << sent << endl;

    if ((size_t)sent < (size_t)bytesRead) {
        // Partial send, wait for next EPOLLOUT
        return 0;
    }

    if (req.bytesSentSoFar >= getContentLength(routeResult.fullPath)) {
        return 1; // Done sending
    }

    return 0; // Still more to send
}


void handle_client_write(int fd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    HttpRequest& req = requestmp[fd];

    try {
        if (!req.mtroute.redirect.empty()) {
            sendRedirect(fd, req.mtroute.redirect, req);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
        if (req.method == "GET") {
            int get = getMethode(fd, epollFd, req, requestmp);
            if (get) {
                int file_fd = req.routeResult.resFd;
                closeOrSwitch(fd, epollFd, req, requestmp);
                if (file_fd != -1)
                    close(file_fd);
                return;
            }
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
