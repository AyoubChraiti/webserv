#include "../../inc/responce.hpp"

void sendRedirect(int fd, const string& location, HttpRequest* req) {
    stringstream response;

    response << "HTTP/1.1 301 Moved Permanently\r\n";
    response << "Location: " << location << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: 0\r\n";
    response << "Connection: " << req->connection << "\r\n";
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

void sendHeaders(int clientFd, RouteResult& routeResult, HttpRequest* req) {
    stringstream response;
    response << "HTTP/1.1 " << routeResult.statusCode << " " << routeResult.statusText << "\r\n";
    response << "Content-Type: " << routeResult.contentType << "\r\n";
    if (routeResult.resFd != -1) {
        response << "Content-Length: " << getContentLength(routeResult.fullPath) << "\r\n";
    }
    else {
        response << "Content-Length: " << routeResult.responseBody.size() << "\r\n";
    }
    response << "Connection: " << req->connection << "\r\n";
    response << "\r\n";

    send(clientFd, response.str().c_str(), response.str().size(), 0);
    req->headerSent = true;
}

void handle_client_write(int fd, int epollFd, mpserv& conf, map<int, HttpRequest *>& requestmp) {
    map<int, HttpRequest*>::iterator it = requestmp.find(fd);
    if (it == requestmp.end()) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
        delete requestmp[fd];
        close(fd);
        return;
    }
    
    HttpRequest* req = it->second;
    try {
        if (req->isCGI) {
            send(fd, req->outputCGI.c_str(), req->outputCGI.length(), 0);
            if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                perror("epoll_ctl");
            }
            delete requestmp[fd];
            requestmp.erase(fd);
            close(fd);
            return;
        }
        if (!req->mtroute.redirect.empty()) {
            cout << "----------------> WE HEREEEEEEEEEEEEEEEEEEEE" << endl;
            sendRedirect(fd, req->mtroute.redirect, req);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
        if (req->method == "GET") {
            int get = getMethode(fd, epollFd, req, requestmp);
            if (get) {
                int file_fd = req->routeResult.resFd;
                closeOrSwitch(fd, epollFd, req, requestmp);
                if (file_fd != -1)
                    close(file_fd);
                return;
            }
        }
        if (req->method == "DELETE") {
            deleteMethod(fd, epollFd, req, requestmp);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what(), req->conf);
        delete requestmp[fd];
        requestmp.erase(fd);
        struct epoll_event ev;
        ev.data.fd = fd;
        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev) == -1)
            cout << "epoll ctl error in the client write\n";
        close(fd);
        return;
    }
}